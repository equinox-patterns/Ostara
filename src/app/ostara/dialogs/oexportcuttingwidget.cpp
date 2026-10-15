/************************************************************************
 **
 **  @file   oexportcuttingwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  The Cutting plan tab of the Export dialog — the width, and one
 **          piece per group.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "oexportcuttingwidget.h"
#include "oexportgroupchoicewidget.h"

#include "../core/opiecegrouplayoutflags.h"
#include "../core/oexportcuttingguard.h"
#include "../core/oexportunitformat.h"
#include "../ogradedgeometry.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QRadioButton>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace
{
/// A size is compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;

/// Quatre colonnes de cases par axe — vingt valeurs. Au-delà, quatre colonnes de plus EN DESSOUS.
constexpr int kGridColumns = 4;

/// Ce dont le contenu d'un choix est en retrait sous sa radio, pour qu'on voie qu'il en dépend.
constexpr int kOptionIndent = 24;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportCuttingWidget::OExportCuttingWidget(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                           QWidget *parent)
  : QWidget(parent),
    m_data(data),
    m_measurements(measurements)
{
    BuildUi();

    // The labels are written by Retranslate and nowhere else: a tab that never ran it would come
    // up with every field unlabelled.
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::BuildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *form = new QFormLayout();
    m_form = form;
    layout->addLayout(form);

    // ── Combien de tailles ───────────────────────────────────────────────
    //
    // Deux choix exclusifs, et **le contenu de celui qui n'est pas actif est grisé, jamais caché** :
    // l'utilisatrice voit de quoi l'autre choix est fait et ce qui la retient (doctrine écrite,
    // Nomenclature §245). Le formulaire ne pose même pas la question sur une table sans axes — il
    // n'y a alors qu'une taille, celle du document.
    // **Un seul cadre, et le contenu de chaque choix SOUS sa radio.** Les combos posés en haut de
    // l'onglet ne se rattachaient à rien, et deux cadres voisins portant le même titre ne se
    // comprenaient pas — ce que la radio commande doit être en dessous d'elle.
    m_sizeModeBox = new QGroupBox(this);
    auto *modeLayout = new QVBoxLayout(m_sizeModeBox);

    m_singleRadio = new QRadioButton(m_sizeModeBox);
    m_singleRadio->setChecked(true);
    modeLayout->addWidget(m_singleRadio);

    m_singleContent = new QWidget(m_sizeModeBox);
    auto *singleForm = new QFormLayout(m_singleContent);
    singleForm->setContentsMargins(kOptionIndent, 0, 0, 0);
    modeLayout->addWidget(m_singleContent);
    PopulateDimensions(singleForm);

    m_severalRadio = new QRadioButton(m_sizeModeBox);
    modeLayout->addWidget(m_severalRadio);

    m_severalContent = new QWidget(m_sizeModeBox);
    auto *severalLayout = new QVBoxLayout(m_severalContent);
    severalLayout->setContentsMargins(kOptionIndent, 0, 0, 0);
    modeLayout->addWidget(m_severalContent);
    PopulateSizeGrids(severalLayout);

    connect(m_singleRadio, &QRadioButton::toggled, this, [this](bool) { UpdateSizeMode(); });
    layout->addWidget(m_sizeModeBox);

    // ── How wide the fabric is ───────────────────────────────────────────
    auto *widthRow = new QHBoxLayout();
    m_fabricWidthEdit = new QLineEdit(this);
    connect(m_fabricWidthEdit, &QLineEdit::editingFinished, this, &OExportCuttingWidget::CommitFabricWidth);
    widthRow->addWidget(m_fabricWidthEdit);

    // **Le combo est rempli et posé AVANT d'être connecté.** Sinon `currentIndexChanged` part à la
    // construction, sur un champ encore vide, et `UpdateFabricWidthUnit` lirait zéro. L'onglet Print
    // n'y échappe que par l'ordre de son constructeur — on ne s'appuie pas là-dessus.
    m_fabricWidthUnitCombo = MakeComboBox(this);
    for (const Unit &unit : OExportUnitFormat::FabricWidthUnits())
    {
        m_fabricWidthUnitCombo->addItem(UnitsToStr(unit, true), static_cast<int>(unit));
    }
    m_fabricWidthUnitCombo->setCurrentIndex(
        m_fabricWidthUnitCombo->findData(static_cast<int>(m_fabricWidthUnit)));
    connect(m_fabricWidthUnitCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { UpdateFabricWidthUnit(); });
    widthRow->addWidget(m_fabricWidthUnitCombo);

    widthRow->addStretch();

    m_fabricWidthLabel = new QLabel(this);
    form->addRow(m_fabricWidthLabel, widthRow);

    // ── La doublure : un SECOND plan, sur la même lancée ──────────────────
    //
    // Un vêtement doublé se taille deux fois, dans deux étoffes : le tissu et la doublure. Le patron
    // le dit déjà — chaque pièce porte une quantité de coupe par matière — et c'est ce que cette case
    // demande : le même run, deux plans, chacun sur son jeu de pièces et sa laize. Tout le reste est
    // commun, et c'est pourquoi la case est ici, sous la laize, et non dans un onglet à part.
    //
    // Elle n'est proposée que si le patron porte de quoi la remplir, et elle est cochée d'emblée :
    // voir UpdateLiningState, qui est la seule à décider de son état.
    m_liningCheck = new QCheckBox(this);
    connect(m_liningCheck, &QCheckBox::toggled, this,
            [this](bool)
            {
                UpdateLiningWidthRow();
                emit Changed();
            });
    form->addRow(m_liningCheck);

    // La laize de la doublure : **sa** largeur, dans **son** unité. Elle suit celle du tissu tant que
    // l'utilisatrice n'y a pas touché — c'est « identique par défaut » — et prend son indépendance
    // dès la première saisie.
    auto *liningRow = new QHBoxLayout();
    m_liningWidthEdit = new QLineEdit(this);
    connect(m_liningWidthEdit, &QLineEdit::editingFinished, this,
            &OExportCuttingWidget::CommitLiningWidth);
    liningRow->addWidget(m_liningWidthEdit);

    // Rempli et posé AVANT d'être connecté, comme celui du tissu et pour la même raison.
    m_liningWidthUnitCombo = MakeComboBox(this);
    for (const Unit &unit : OExportUnitFormat::FabricWidthUnits())
    {
        m_liningWidthUnitCombo->addItem(UnitsToStr(unit, true), static_cast<int>(unit));
    }
    m_liningWidthUnitCombo->setCurrentIndex(
        m_liningWidthUnitCombo->findData(static_cast<int>(m_liningWidthUnit)));
    connect(m_liningWidthUnitCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { UpdateLiningWidthUnit(); });
    liningRow->addWidget(m_liningWidthUnitCombo);
    liningRow->addStretch();

    m_liningWidthLabel = new QLabel(this);
    form->addRow(m_liningWidthLabel, liningRow);

    // ── Le tissu plié ────────────────────────────────────────────────────
    // La couture grand public plie le tissu en deux dans le sens de la laize : on trace les moitiés
    // sur le tissu plié, et le pli donne la seconde. Posée ici, juste sous la laize, parce que
    // c'est la laize qu'elle change — le nesting tourne alors sur la moitié.
    m_foldedCheck = new QCheckBox(this);
    connect(m_foldedCheck, &QCheckBox::toggled, this, [this](bool) { emit Changed(); });
    form->addRow(m_foldedCheck);

    // ── Le sens ──────────────────────────────────────────────────────────
    // Quelle que soit la pièce : aucune n'est retournée d'un demi-tour. Le tissu a un sens —
    // velours, carreaux, imprimé — et toutes les pièces doivent le prendre du même côté.
    m_oneWayCheck = new QCheckBox(this);
    connect(m_oneWayCheck, &QCheckBox::toggled, this, [this](bool) { emit Changed(); });
    form->addRow(m_oneWayCheck);

    // ── L'espace entre les pièces ────────────────────────────────────────
    // Zéro par défaut : sur du tissu, deux pièces peuvent se toucher bord à bord, et rien ne
    // justifie de les écarter. Le champ existe parce que certaines matières en demandent.
    auto *gapRow = new QHBoxLayout();
    m_gapEdit = new QLineEdit(this);
    connect(m_gapEdit, &QLineEdit::editingFinished, this, &OExportCuttingWidget::CommitGap);
    gapRow->addWidget(m_gapEdit);

    m_gapUnit = new QLabel(this);
    gapRow->addWidget(m_gapUnit);
    gapRow->addStretch();

    m_gapLabel = new QLabel(this);
    form->addRow(m_gapLabel, gapRow);

    // ── L'espace depuis les bords ────────────────────────────────────────
    // Zéro par défaut, comme l'espacement. **C'est une marge d'erreur** — un tissu n'est pas
    // toujours coupé droit — donc chaque rectangle du plan la garde sur **trois côtés : tous sauf
    // le pli**, qui est droit par construction. Sur du tissu plié, c'est le nesting qui la répartit
    // par rectangle : voir `SeatMargins`, dans oexportnesting.cpp.
    auto *edgeRow = new QHBoxLayout();
    m_edgeMarginEdit = new QLineEdit(this);
    connect(m_edgeMarginEdit, &QLineEdit::editingFinished, this,
            &OExportCuttingWidget::CommitEdgeMargin);
    edgeRow->addWidget(m_edgeMarginEdit);

    m_edgeMarginUnit = new QLabel(this);
    edgeRow->addWidget(m_edgeMarginUnit);
    edgeRow->addStretch();

    m_edgeMarginLabel = new QLabel(this);
    form->addRow(m_edgeMarginLabel, edgeRow);

    UpdateSizeMode();

    // ── Which member of each group ───────────────────────────────────────
    //
    // La question est portée par un widget à part : le pré-vol du lot la repose, une fois, pour
    // tous les favoris de coupe. Deux lectures concurrentes de la même liste finiraient par
    // diverger, et c'est la recette de peuplement qui est délicate, pas la boîte.
    m_groupChoice = new OExportGroupChoiceWidget(m_data, this);
    // Le choix des groupes décide de la disponibilité de la doublure : c'est le même jeu de pièces,
    // lu sur l'autre matière. Un membre de groupe qui n'est plus celui qu'on taille peut emporter la
    // seule pièce de doublure du patron — la case s'éteint alors, et le plan avec elle.
    connect(m_groupChoice, &OExportGroupChoiceWidget::Changed, this,
            [this]()
            {
                UpdateLiningState();
                emit Changed();
            });
    layout->addWidget(m_groupChoice);

    // **En dernier, quand tout ce qu'elle lit existe** : la disponibilité se lit sur les pièces et
    // sur le choix des groupes, donc une case réglée plus tôt aurait jugé un formulaire inachevé.
    UpdateLiningState();

    layout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::PopulateDimensions(QFormLayout *form)
{
    if (m_measurements.isNull())
    {
        // A table without axes describes a single size: the document is already on it, and there
        // is nothing to pick along. The run reads the geometry as it stands.
        return;
    }

    // One row per dimension of the measurement table, in the table's own order — the same rows,
    // from the same table, as the Print tab offers for a single size.
    const VDimensions dimensions = m_measurements->Dimensions();
    for (auto it = dimensions.constBegin(); it != dimensions.constEnd(); ++it)
    {
        const MeasurementDimension_p &dimension = it.value();

        auto *combo = MakeComboBox(this);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        const DimesionLabels labels = dimension->Labels();
        for (const qreal base : dimension->ValidBases())
        {
            QString text;
            for (auto label = labels.constBegin(); label != labels.constEnd(); ++label)
            {
                if (qAbs(label.key() - base) < kBaseTolerance)
                {
                    text = label.value();
                    break;
                }
            }
            if (text.isEmpty())
            {
                text = QString::number(base);
            }

            combo->addItem(text, base);
        }

        auto *rowLabel = new QLabel(this);
        form->addRow(rowLabel, combo);
        m_dimensionLabels.append(rowLabel);

        // Which axis the row feeds. The document is put on a size by naming its axes — X, Y, W —
        // so the row's rank in the table cannot say it: a table missing Y shifts every row below
        // it by one slot. The dimension with no slot of its own (Z) is still shown, and feeds
        // nothing.
        const int dimensionIndex = static_cast<int>(it.key());
        m_dimensionCombos.append(combo);
        m_dimensionSlots.append(dimensionIndex < 3 ? dimensionIndex : -1);

        connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::PopulateSizeGrids(QVBoxLayout *layout)
{
    if (m_measurements.isNull())
    {
        return;
    }

    const VDimensions dimensions = m_measurements->Dimensions();
    for (auto it = dimensions.constBegin(); it != dimensions.constEnd(); ++it)
    {
        const MeasurementDimension_p &dimension = it.value();
        const DimesionLabels labels = dimension->Labels();

        SizeGrid grid;
        const int dimensionIndex = static_cast<int>(it.key());
        grid.slot = dimensionIndex < 3 ? dimensionIndex : -1;

        // Un **QLabel** nomme l'axe, pas un cadre : les cadres s'empilaient sous le titre du
        // dessus, et deux fois « Sizes to cut » ne se lisaient pas.
        auto *axisLabel = new QLabel(this);
        grid.label = axisLabel;
        layout->addWidget(axisLabel);

        auto *box = new QWidget(this);
        auto *cells = new QGridLayout(box);
        cells->setContentsMargins(0, 0, 0, 0);

        int index = 0;
        for (const qreal base : dimension->ValidBases())
        {
            QString text;
            for (auto label = labels.constBegin(); label != labels.constEnd(); ++label)
            {
                if (qAbs(label.key() - base) < kBaseTolerance)
                {
                    text = label.value();
                    break;
                }
            }
            if (text.isEmpty())
            {
                text = QString::number(base);
            }

            auto *check = new QCheckBox(text, box);
            // **Toutes cochées d'entrée** : qui choisit « plusieurs tailles » veut les tailles de
            // son patron, et décocher ce qu'elle ne coupe pas est plus court que tout cocher.
            check->setChecked(true);
            connect(check, &QCheckBox::toggled, this, [this](bool) { emit Changed(); });

            // Quatre colonnes — vingt valeurs — et quatre colonnes de plus EN DESSOUS au-delà :
            // une liste d'un seul tenant serait plus haute que l'écran, et un tableau de plus de
            // vingt tailles ne se rencontre pas.
            cells->addWidget(check, index / kGridColumns, index % kGridColumns);

            grid.boxes.append(check);
            grid.bases.append(base);
            ++index;
        }

        grid.box = box;
        layout->addWidget(box);
        m_sizeGrids.append(grid);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::UpdateSizeMode()
{
    const bool single = IsSingleSize();
    const bool hasAxes = HasAxes();

    // Le formulaire ne pose pas la question quand il n'y a qu'une réponse : une table sans axes
    // décrit une seule taille, celle du document.
    m_sizeModeBox->setVisible(hasAxes);

    // **Grisé, jamais caché** : l'utilisatrice voit de quoi l'autre choix est fait et ce qui la
    // retient. C'est la doctrine de l'utilisateur guidé, et le précédent est OExportPrintWidget.
    // Le contenu suit sa radio : grisé quand elle n'est pas active, jamais caché.
    m_singleContent->setEnabled(single);
    m_severalContent->setEnabled(not single);

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::HasAxes() const -> bool
{
    return not m_sizeGrids.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::IsSingleSize() const -> bool
{
    // Sans axes il n'y a pas de question à poser, donc une seule taille — c'est ce que le run
    // signifie alors, et le dire ici évite un cas particulier à chaque lecteur.
    return not HasAxes() || m_singleRadio->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::CheckedBases(int index) const -> QVector<qreal>
{
    QVector<qreal> bases;
    for (const SizeGrid &grid : m_sizeGrids)
    {
        if (grid.slot != index)
        {
            continue;
        }

        for (int i = 0; i < grid.boxes.size(); ++i)
        {
            if (grid.boxes.at(i)->isChecked())
            {
                bases.append(grid.bases.at(i));
            }
        }
    }
    return bases;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::CheckedLabels(int index) const -> QStringList
{
    QStringList labels;
    for (const SizeGrid &grid : m_sizeGrids)
    {
        if (grid.slot != index)
        {
            continue;
        }

        for (const QCheckBox *box : grid.boxes)
        {
            if (box->isChecked())
            {
                labels.append(box->text());
            }
        }
    }
    return labels;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::CommitFabricWidth()
{
    bool ok = false;
    const qreal widthMm = OExportUnitFormat::Parse(m_fabricWidthEdit->text(), m_fabricWidthUnit, &ok);

    // A laize of zero or less is not a narrow fabric, it is a field that was not filled: the
    // previous value stands rather than running the nesting on nothing.
    if (ok && widthMm > 0.0)
    {
        m_fabricWidthMm = widthMm;
    }

    m_fabricWidthEdit->setText(OExportUnitFormat::Format(m_fabricWidthMm, m_fabricWidthUnit));

    // La doublure non saisie suit le tissu : sa ligne se réaffiche donc avec la laize qui vient
    // d'être lue. Après une saisie à elle, cette ligne ne fait rien — voir `LiningWidthMm`.
    if (not m_liningWidthEdited)
    {
        ShowLiningWidth();
    }

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::UpdateFabricWidthUnit()
{
    // ⚠️ **Lire avec l'ANCIENNE unité, puis réafficher avec la NOUVELLE — dans cet ordre.**
    // L'ordre inverse relit les mêmes chiffres dans l'unité qui vient d'être choisie : « 140 » cm
    // deviendrait 140 pouces, la laize serait multipliée par 2,54, et **pas un chiffre ne bougerait
    // à l'écran**. C'est le défaut de l'onglet Print (`oexportprintwidget.cpp:482`), et le dialogue
    // d'image montre l'ordre juste. Ici la laize est stockée en millimètres : l'unité n'est qu'une
    // façon de l'écrire, elle ne doit jamais changer le tissu.
    bool ok = false;
    const qreal widthMm = OExportUnitFormat::Parse(m_fabricWidthEdit->text(), m_fabricWidthUnit, &ok);
    if (ok && widthMm > 0.0)
    {
        m_fabricWidthMm = widthMm;
    }

    m_fabricWidthUnit = static_cast<Unit>(m_fabricWidthUnitCombo->currentData().toInt());
    m_fabricWidthEdit->setText(OExportUnitFormat::Format(m_fabricWidthMm, m_fabricWidthUnit));

    // La doublure non saisie suit aussi l'UNITÉ du tissu : les deux laizes sont la même étoffe à lire
    // tant qu'elles n'ont pas été séparées, et voir « 55 1/8" » au-dessus de « 140 » en dessous
    // donnerait deux largeurs différentes pour un seul tissu.
    if (not m_liningWidthEdited)
    {
        m_liningWidthUnit = m_fabricWidthUnit;
        ShowLiningWidth();
    }

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::CommitLiningWidth()
{
    bool ok = false;
    const qreal widthMm = OExportUnitFormat::Parse(m_liningWidthEdit->text(), m_liningWidthUnit, &ok);
    if (ok && widthMm > 0.0)
    {
        m_liningWidthMm = widthMm;
        // **C'est ici que la doublure prend son indépendance.** Tant qu'aucune valeur n'a été écrite,
        // elle est celle du tissu ; à partir d'ici elle est à elle, et changer la laize du tissu ne
        // la touche plus. Même règle de validité que pour le tissu : zéro n'est pas une laize étroite,
        // c'est un champ qu'on n'a pas rempli, et l'ancienne valeur tient.
        m_liningWidthEdited = true;
    }

    ShowLiningWidth();
    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::UpdateLiningWidthUnit()
{
    // L'ordre est celui du tissu, et pour la même raison : lire avec l'ANCIENNE unité, réafficher
    // avec la nouvelle. « 140 » cm ne doit pas devenir 140 pouces sans qu'un chiffre bouge.
    bool ok = false;
    const qreal widthMm = OExportUnitFormat::Parse(m_liningWidthEdit->text(), m_liningWidthUnit, &ok);
    if (ok && widthMm > 0.0)
    {
        m_liningWidthMm = widthMm;
    }

    m_liningWidthUnit = static_cast<Unit>(m_liningWidthUnitCombo->currentData().toInt());

    // ⚠️ **Et surtout PAS `m_liningWidthEdited`** : choisir une unité n'est pas donner une largeur.
    // La doublure continue donc de suivre le tissu si elle le suivait — elle s'en détachera quand une
    // valeur sera écrite, pas quand elle sera relue dans une autre langue.
    ShowLiningWidth();
    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::HasLiningPlan() const -> bool
{
    return m_liningCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::LiningWidthMm() const -> qreal
{
    return m_liningWidthEdited ? m_liningWidthMm : m_fabricWidthMm;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::ShowLiningWidth()
{
    // ⚠️ **Le combo est reposé SANS émettre.** C'est la fonction qui RÉAFFICHE, pas celle qui relit :
    // un `currentIndexChanged` émis d'ici ferait entrer la valeur affichée dans `UpdateLiningWidthUnit`,
    // qui la relirait avec l'unité qu'on vient de poser — « 140 » lu comme des pouces, et une laize
    // multipliée par 2,54 sans qu'un chiffre bouge à l'écran. C'est le piège que le combo du tissu
    // referme par l'ordre de ses appels ; ici il est refermé pour de bon.
    const QSignalBlocker blocker(m_liningWidthUnitCombo);
    m_liningWidthUnitCombo->setCurrentIndex(
        m_liningWidthUnitCombo->findData(static_cast<int>(m_liningWidthUnit)));
    m_liningWidthEdit->setText(OExportUnitFormat::Format(LiningWidthMm(), m_liningWidthUnit));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::UpdateLiningState()
{
    const bool available =
        not OExportCuttingGuard::PiecesToCut(MemberChoices(), m_data, OExportCuttingGuard::Material::Lining)
            .isEmpty();

    // **Cochée à l'instant où elle devient disponible**, jamais à chaque passage : le tour d'après
    // remettrait la coche qu'une utilisatrice vient de retirer.
    if (not available)
    {
        m_liningCheck->setChecked(false);
    }
    else if (not m_liningWasAvailable)
    {
        m_liningCheck->setChecked(true);
    }

    m_liningWasAvailable = available;
    m_liningCheck->setEnabled(available);

    UpdateLiningWidthRow();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::UpdateLiningWidthRow()
{
    m_form->setRowVisible(m_liningWidthLabel, m_liningCheck->isChecked());

    // La ligne peut s'ouvrir sur une laize jamais saisie : elle doit alors montrer celle du tissu,
    // qui a pu changer pendant qu'elle était cachée.
    if (not m_liningWidthEdited)
    {
        ShowLiningWidth();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::DisplayUnit() const -> Unit
{
    return m_fabricWidthUnit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::FabricWidthMm() const -> qreal
{
    return m_fabricWidthMm;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::CommitGap()
{
    bool ok = false;
    const qreal gapMm = OExportUnitFormat::Parse(m_gapEdit->text(), Unit::Cm, &ok);

    // **Zéro est légitime** — c'est même le défaut — donc seul un champ illisible est ignoré, et la
    // valeur précédente tient. Un négatif, lui, n'a pas de sens et n'est pas pris.
    if (ok && gapMm >= 0.0)
    {
        m_gapMm = gapMm;
    }

    m_gapEdit->setText(OExportUnitFormat::Format(m_gapMm, Unit::Cm));

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::CommitEdgeMargin()
{
    bool ok = false;
    const qreal edgeMm = OExportUnitFormat::Parse(m_edgeMarginEdit->text(), Unit::Cm, &ok);

    // **Zéro est légitime** — c'est même le défaut — donc seul un champ illisible est ignoré, et la
    // valeur précédente tient. Un négatif, lui, n'a pas de sens et n'est pas pris.
    //
    // La borne HAUTE, elle, ne se lit pas ici mais contre la laize : c'est `IsValid`, et c'est
    // délibéré — elle doit tenir même si la laize change APRÈS l'espace.
    if (ok && edgeMm >= 0.0)
    {
        m_edgeMarginMm = edgeMm;
    }

    m_edgeMarginEdit->setText(OExportUnitFormat::Format(m_edgeMarginMm, Unit::Cm));

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::GapMm() const -> qreal
{
    return m_gapMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::EdgeMarginMm() const -> qreal
{
    return m_edgeMarginMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::MemberChoices() const -> QMap<quint32, quint32>
{
    return m_groupChoice != nullptr ? m_groupChoice->MemberChoices() : QMap<quint32, quint32>();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::IsOneWayLayout() const -> bool
{
    return m_oneWayCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::IsFoldedFabric() const -> bool
{
    return m_foldedCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::DimensionBase(int index) const -> qreal
{
    if (index < 0)
    {
        return -1.0;
    }

    for (int i = 0; i < m_dimensionCombos.size() && i < m_dimensionSlots.size(); ++i)
    {
        if (m_dimensionSlots.at(i) != index)
        {
            continue;
        }

        const QComboBox *combo = m_dimensionCombos.at(i);
        if (combo->currentIndex() == -1)
        {
            return -1.0;
        }

        return combo->currentData().toReal();
    }

    return -1.0; // the table carries no axis for this slot
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::DimensionLabel(int index) const -> QString
{
    if (index < 0)
    {
        return {};
    }

    for (int i = 0; i < m_dimensionCombos.size() && i < m_dimensionSlots.size(); ++i)
    {
        if (m_dimensionSlots.at(i) == index)
        {
            return m_dimensionCombos.at(i)->currentText();
        }
    }

    return {}; // the table carries no axis for this slot — the name skips it
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::IsValid() const -> bool
{
    if (FabricWidthMm() <= 0.0)
    {
        return false;
    }

    // A table without axes has no size to pick along them: the pattern is on its one size, and the
    // geometry is read as it stands. Requiring a dimension to exist would lock that path.
    for (const QComboBox *combo : std::as_const(m_dimensionCombos))
    {
        if (combo->count() == 0)
        {
            return false;
        }
    }

    // Every group must have its piece: a group left unanswered is a piece the plan would have to
    // pick for the user.
    if (m_groupChoice != nullptr && !m_groupChoice->IsComplete())
    {
        return false;
    }

    // En plusieurs tailles, chaque axe doit garder au moins une case cochée : un axe vide ne
    // décrit aucune sortie, et « zéro sortie » ne se demande pas — on décrit ce qu'on veut.
    if (not IsSingleSize())
    {
        for (const SizeGrid &grid : m_sizeGrids)
        {
            if (std::none_of(grid.boxes.cbegin(), grid.boxes.cend(),
                             [](const QCheckBox *box) { return box->isChecked(); }))
            {
                return false;
            }
        }
    }

    // **L'espace depuis les bords ne peut pas manger toute la largeur.**
    //
    // ⚠️ **Un seul chiffre suffit pour les DEUX lectures**, et c'est ce qui rend la règle tenable :
    // le tissu plié tourne sur la demi-laize, et son rectangle le plus étroit — la colonne — a UN
    // bord de pli, gratuit, et un bord de lisière, donc `demi − m` ; le bac déplié a deux bords
    // payants, donc `laize − 2 m`. Les deux sont positifs exactement quand `2 m < laize`. La case
    // « tissu plié » n'entre donc pas dans ce test — et n'y entrera pas le jour où un rectangle
    // paiera un bord de plus.
    //
    // **Nécessaire et non suffisante** : une grande pièce peut encore ne pas tenir. C'est le moteur
    // qui tranche, et le message du run dit alors lequel des deux a parlé.
    //
    // Même doctrine que la laize juste au-dessus : ce n'est pas une valeur limite, c'est un champ
    // qui ne décrit plus de plan. Le bouton se grise, comme pour une laize à zéro.
    //
    // Et ici plutôt que dans le commit, parce que `IsValid` est réévalué à CHAQUE `Changed()` : la
    // règle tient donc même si la laize est modifiée APRÈS l'espace, ce qu'un contrôle à la saisie
    // ne peut pas faire.
    if (2.0 * EdgeMarginMm() >= FabricWidthMm())
    {
        return false;
    }

    // ── Et la doublure a SA laize : la même règle, sur son propre chiffre ─────────────────────────
    //
    // Le run décrit alors DEUX plans, et l'espace depuis les bords est le même pour les deux : une
    // doublure plus étroite que le tissu peut donc être la seule à ne plus décrire de plan. Le test
    // porte sur la laize de la doublure telle qu'elle sera lue — celle du tissu tant qu'elle n'a pas
    // été saisie — et non sur un second chiffre à tenir en accord.
    if (HasLiningPlan())
    {
        if (LiningWidthMm() <= 0.0)
        {
            return false;
        }

        if (2.0 * EdgeMarginMm() >= LiningWidthMm())
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::Retranslate()
{
    m_fabricWidthLabel->setText(tr("Fabric width:"));

    // Le libellé du combo EST l'unité du champ — c'est ce qui remplace le QLabel d'unité d'hier.
    // Relus depuis le catalogue, comme les unités de l'onglet Print.
    for (int i = 0; i < m_fabricWidthUnitCombo->count(); ++i)
    {
        m_fabricWidthUnitCombo->setItemText(
            i, UnitsToStr(static_cast<Unit>(m_fabricWidthUnitCombo->itemData(i).toInt()), true));
    }

    // **Le champ s'ouvre sur la valeur stockée.** Sans cette ligne il s'ouvre VIDE alors que la laize
    // vaut 1400 mm : `setText` n'existait que dans `CommitFabricWidth`, donc rien ne l'écrivait avant
    // la première saisie. Le champ espacement, lui, s'initialise ici depuis toujours.
    //
    // C'est un réaffichage depuis les millimètres, jamais une relecture : passer par `Retranslate`
    // sur un changement de langue ne peut donc pas altérer la laize, même si le champ portait une
    // saisie en cours.
    m_fabricWidthEdit->setText(OExportUnitFormat::Format(m_fabricWidthMm, m_fabricWidthUnit));

    // La doublure : son libellé, son unité, sa valeur. Refaite ici et pas seulement dans ses commits,
    // parce qu'un changement de langue doit la relire comme le reste du formulaire — et parce que sa
    // valeur peut être celle du tissu, qui vient d'être réaffichée juste au-dessus.
    m_liningCheck->setText(tr("Lining"));
    m_liningWidthLabel->setText(tr("Lining width:"));
    for (int i = 0; i < m_liningWidthUnitCombo->count(); ++i)
    {
        m_liningWidthUnitCombo->setItemText(
            i, UnitsToStr(static_cast<Unit>(m_liningWidthUnitCombo->itemData(i).toInt()), true));
    }
    ShowLiningWidth();

    // L'infobulle de la case dépend de sa disponibilité : c'est `UpdateLiningState` qui la pose, et
    // la rappeler ici est la seule façon qu'un changement de langue la reprenne.
    UpdateLiningState();

    m_sizeModeBox->setTitle(tr("Sizes to cut"));
    m_singleRadio->setText(tr("Single size"));
    m_severalRadio->setText(tr("Several sizes"));

    // Chaque axe est nommé par la table de mesures, exactement comme les lignes du mode taille
    // unique — la table dit ce qu'un axe s'appelle, et un second vocabulaire ici serait de trop.
    if (not m_measurements.isNull())
    {
        const VDimensions dimensions = m_measurements->Dimensions();
        int row = 0;
        for (auto it = dimensions.constBegin(); it != dimensions.constEnd() && row < m_sizeGrids.size();
             ++it, ++row)
        {
            m_sizeGrids.at(row).label->setText(it.value()->Name() + QLatin1Char(':'));
        }
    }

    m_gapLabel->setText(tr("Space between pieces:"));
    m_gapUnit->setText(UnitsToStr(Unit::Cm, true));
    m_gapEdit->setText(OExportUnitFormat::Format(m_gapMm, Unit::Cm));

    m_edgeMarginLabel->setText(tr("Space from the edges:"));
    m_edgeMarginUnit->setText(UnitsToStr(Unit::Cm, true));
    m_edgeMarginEdit->setText(OExportUnitFormat::Format(m_edgeMarginMm, Unit::Cm));
    m_edgeMarginLabel->setToolTip(
        tr("The space kept between the pieces and the fabric's edges. It is a margin for error - "
           "cloth is not always cut straight - so every rectangle of the plan keeps it on THREE "
           "sides: all of them except the fold. A piece cut on the fold keeps its fold line on the "
           "fold, which is straight by construction."));

    m_foldedCheck->setText(tr("Folded fabric"));
    m_foldedCheck->setToolTip(
        tr("The fabric is folded in two along its length, and the halves are traced on the folded "
           "cloth - the fold gives the second half. The nesting then runs on HALF the fabric width: "
           "140 entered here means a plan laid out on 70. A piece cut on the fold keeps its fold "
           "line on the fold and stays folded; a piece cut twice is laid once, and the fold mirrors "
           "it."));

    m_oneWayCheck->setText(tr("One-way layout"));
    m_oneWayCheck->setToolTip(
        tr("No piece is turned a half-turn: every piece takes the fabric's direction the same way. "
           "Tick it for a cloth that has one - velvet, checks, a print. A piece cut on the cross "
           "keeps that freedom; it is simply never laid upside down."));

    // Each axis row is named by the measurement table itself, exactly as the Details top bar names
    // them: the table says what an axis is called, and a second name here would be one more thing
    // to keep in step with it.
    if (m_measurements.isNull())
    {
        return;
    }

    const VDimensions dimensions = m_measurements->Dimensions();
    int row = 0;
    for (auto it = dimensions.constBegin(); it != dimensions.constEnd() && row < m_dimensionLabels.size();
         ++it, ++row)
    {
        m_dimensionLabels.at(row)->setText(it.value()->Name() + QLatin1Char(':'));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
