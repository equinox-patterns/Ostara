/************************************************************************
 **
 **  @file   oexportprojectorwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Export mode — l'onglet Projecteur.
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

#include "oexportprojectorwidget.h"

#include "../core/oexportunitformat.h"
#include "../core/ogradedsizeranges.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"

#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>

#include <algorithm>

namespace
{
/// Les libellés et les bases viennent tous deux du tableau de mesures : ils se comparent avec une
/// tolérance, jamais exactement.
constexpr qreal kBaseTolerance = 1e-9;

/// **Le plancher de l'espace entre pièces : 5 cm.** En dessous, ce n'est plus un plan projecteur,
/// c'est un plan de coupe — et la décision est celle du format, pas un réglage (`Projector.md` §9).
constexpr qreal kGapFloorMm = 50.0;

/// Ce que l'axe sans slot propre (Z) reçoit : rien. La même convention que les deux autres onglets.
constexpr int kNoSlot = -1;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportProjectorWidget::OExportProjectorWidget(const VContainer *data,
                                               const QSharedPointer<VMeasurements> &measurements,
                                               const OGradedSizeRanges::Context &context, QWidget *parent)
  : QWidget(parent),
    m_data(data),
    m_measurements(measurements),
    m_rangeContext(context)
{
    BuildUi();
    PopulateSizeRanges();
    UpdateSourceVisibility();
    WriteLengths();
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::BuildUi()
{
    auto *layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // ── L'unité de lecture ───────────────────────────────────────────────────────────────────────
    //
    // Cm ou pouces, comme partout ailleurs. Ce n'est pas une conversion : les deux écarts restent
    // stockés en millimètres, et l'unité ne dit que les mots dans lesquels ils se lisent.
    m_unitCombo = MakeComboBox(this);
    m_unitCombo->addItem(QString(), static_cast<int>(Unit::Cm));
    m_unitCombo->addItem(QString(), static_cast<int>(Unit::Inch));
    connect(m_unitCombo, &QComboBox::currentIndexChanged, this,
            [this](int)
            {
                const auto unit = static_cast<Unit>(m_unitCombo->currentData().toInt());
                if (unit == m_unit)
                {
                    return;
                }

                // Les champs sont relus dans l'ANCIENNE unité avant de changer, puis réécrits dans la
                // nouvelle : sans cette relecture, une valeur tapée puis jamais validée serait perdue
                // au moment précis où l'on change l'unité.
                ReadLengths(m_unit);
                m_unit = unit;
                WriteLengths();
                emit Changed();
            });
    m_unitLabel = new QLabel(this);
    layout->addRow(m_unitLabel, m_unitCombo);

    // ── Quoi nidifier ────────────────────────────────────────────────────────────────────────────
    m_sourceCombo = MakeComboBox(this);
    m_sourceCombo->addItem(QString(), QStringLiteral("range"));
    m_sourceCombo->addItem(QString(), QStringLiteral("single"));
    connect(m_sourceCombo, &QComboBox::currentIndexChanged, this,
            [this](int)
            {
                UpdateSourceVisibility();
                emit Changed();
            });
    m_sourceLabel = new QLabel(this);
    layout->addRow(m_sourceLabel, m_sourceCombo);

    m_rangeCombo = MakeComboBox(this);
    connect(m_rangeCombo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    m_rangeLabel = new QLabel(this);
    layout->addRow(m_rangeLabel, m_rangeCombo);

    // Les lignes d'axes appartiennent à la question juste au-dessus : elles apparaissent quand la
    // réponse est « une seule taille », et se placent entre elle et les écarts.
    PopulateDimensions();

    // ── Les deux écarts ──────────────────────────────────────────────────────────────────────────
    m_gapEdit = new QLineEdit(this);
    connect(m_gapEdit, &QLineEdit::editingFinished, this, &OExportProjectorWidget::CommitLengths);
    m_gapLabel = new QLabel(this);
    layout->addRow(m_gapLabel, m_gapEdit);

    m_marginEdit = new QLineEdit(this);
    connect(m_marginEdit, &QLineEdit::editingFinished, this, &OExportProjectorWidget::CommitLengths);
    m_marginLabel = new QLabel(this);
    layout->addRow(m_marginLabel, m_marginEdit);

    // ── Ce que le fichier porte et que l'aperçu ne montre pas ───────────────────────────────────
    //
    // ⚠️ **Une phrase, et rien de plus.** Les trois calques de calibration ne sont pas dessinés dans
    // le canvas Ostara ; l'utilisatrice de projecteur, elle, sait à quoi ils servent. Ce qu'elle ne
    // peut pas deviner, c'est qu'ils existent quand même dans le PDF — c'est la seule chose dite ici.
    //
    // Le texte est plus petit et sur toute la largeur : c'est une note, pas un réglage.
    m_calibrationLabel = new QLabel(this);
    m_calibrationLabel->setWordWrap(true);
    m_calibrationLabel->setTextFormat(Qt::PlainText);
    // The smaller size is a theme rule (`template.qss.in`, `QLabel[ostaraNote="true"]`): the QSS is
    // reapplied at polish and overrides whatever `setFont` did, so the widget only declares what it
    // is. Scaling the point size here used to compute -1 × 0.9 — Qt refuses it and warns.
    m_calibrationLabel->setProperty("ostaraNote", true);
    layout->addRow(m_calibrationLabel);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::PopulateSizeRanges()
{
    m_rangeCombo->clear();

    // Le filtre du dock Graded, le même que Print : seules les plages du contexte du patron se
    // proposent — une plage d'une autre stature ne décrit pas ce patron-ci.
    const QHash<QString, OSizerange> *ranges = m_data != nullptr ? m_data->SizeRanges() : nullptr;
    if (ranges != nullptr)
    {
        QStringList names;
        for (auto it = ranges->cbegin(); it != ranges->cend(); ++it)
        {
            if (OGradedSizeRanges::Matches(it.value(), m_rangeContext))
            {
                names.append(it.key());
            }
        }
        names.sort();
        for (const QString &name : names)
        {
            m_rangeCombo->addItem(name, name);
        }
    }

    // « Une plage » a besoin d'une plage ET d'axes pour la parcourir. Sans l'un des deux, le choix
    // n'existe pas : il ne reste qu'une taille unique, et l'offrir quand même promettrait un run
    // que personne ne pourrait décrire.
    if (m_rangeCombo->count() == 0 || !HasAxes())
    {
        const int rangeItem = m_sourceCombo->findData(QStringLiteral("range"));
        if (rangeItem >= 0)
        {
            m_sourceCombo->removeItem(rangeItem);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::PopulateDimensions()
{
    if (m_measurements.isNull())
    {
        return;
    }

    auto *form = qobject_cast<QFormLayout *>(layout());

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

        // Quel axe la ligne alimente. Le document se met à une taille en NOMMANT ses axes — X, Y,
        // W — donc le rang de la ligne ne dit rien : un tableau sans Y décalerait tout ce qui suit.
        const int dimensionIndex = static_cast<int>(it.key());
        m_dimensionCombos.append(combo);
        m_dimensionSlots.append(dimensionIndex < 3 ? dimensionIndex : kNoSlot);

        connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::UpdateSourceVisibility()
{
    const bool single = IsSingleSize();

    m_rangeLabel->setVisible(!single);
    m_rangeCombo->setVisible(!single);

    for (int i = 0; i < m_dimensionCombos.size(); ++i)
    {
        m_dimensionLabels.at(i)->setVisible(single);
        m_dimensionCombos.at(i)->setVisible(single);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::IsSingleSize() const -> bool
{
    // Sans combo, il n'y avait pas de choix à faire : l'onglet n'a retiré « une plage » que
    // lorsqu'elle ne pouvait pas être honorée, ce qui ne laisse que la taille unique.
    if (m_sourceCombo->count() < 2)
    {
        return true;
    }

    return m_sourceCombo->currentData().toString() == QStringLiteral("single");
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::SizeRangeName() const -> QString
{
    if (IsSingleSize())
    {
        return {};
    }

    return m_rangeCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::HasAxes() const -> bool
{
    return !m_measurements.isNull() && !m_measurements->Dimensions().isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::DimensionBase(int index) const -> qreal
{
    for (int i = 0; i < m_dimensionSlots.size(); ++i)
    {
        if (m_dimensionSlots.at(i) == index)
        {
            return m_dimensionCombos.at(i)->currentData().toDouble();
        }
    }

    return -1.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::DimensionLabel(int index) const -> QString
{
    for (int i = 0; i < m_dimensionSlots.size(); ++i)
    {
        if (m_dimensionSlots.at(i) == index)
        {
            return m_dimensionCombos.at(i)->currentText();
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::CommitLengths()
{
    ReadLengths(m_unit);
    WriteLengths();

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::ReadLengths(const Unit &unit)
{
    bool gapOk = false;
    bool marginOk = false;
    const qreal gapMm = OExportUnitFormat::Parse(m_gapEdit->text(), unit, &gapOk);
    const qreal marginMm = OExportUnitFormat::Parse(m_marginEdit->text(), unit, &marginOk);

    if (gapOk)
    {
        // ── Le plancher, appliqué ici et pas seulement affiché ────────────────────────────────────
        //
        // Une valeur plus petite n'est pas « un peu serré » : c'est un autre format. On la relève au
        // plancher plutôt que de refuser la saisie — l'utilisatrice voit ce qu'elle a obtenu, et le
        // champ se réécrit aussitôt avec la vraie valeur.
        m_gapMm = std::max(gapMm, kGapFloorMm);
    }

    if (marginOk && marginMm >= 0.0)
    {
        m_marginMm = marginMm;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::WriteLengths()
{
    const QSignalBlocker gapBlocker(m_gapEdit);
    const QSignalBlocker marginBlocker(m_marginEdit);

    m_gapEdit->setText(OExportUnitFormat::Format(m_gapMm, m_unit));
    m_marginEdit->setText(OExportUnitFormat::Format(m_marginMm, m_unit));
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::GapMm() const -> qreal
{
    return m_gapMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::EdgeMarginMm() const -> qreal
{
    return m_marginMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::DisplayUnit() const -> Unit
{
    return m_unit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjectorWidget::IsValid() const -> bool
{
    // Un écart entre pièces sous le plancher n'existe pas : la lecture le relève, donc il n'y a rien
    // à refuser ici. Ce qui reste à vérifier, c'est que la question « quoi nidifier » a une réponse.
    if (m_gapMm + kBaseTolerance < kGapFloorMm)
    {
        return false;
    }

    if (!IsSingleSize())
    {
        return !SizeRangeName().isEmpty();
    }

    for (const QComboBox *combo : m_dimensionCombos)
    {
        if (combo->count() == 0)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::Retranslate()
{
    // Chaque ligne d'axe est NOMMÉE par la table de mesures, comme la barre du haut du mode Details
    // la nomme : un second nom ici serait une chose de plus à tenir en accord avec elle.
    if (!m_measurements.isNull())
    {
        const VDimensions dimensions = m_measurements->Dimensions();
        int row = 0;
        for (auto it = dimensions.constBegin();
             it != dimensions.constEnd() && row < m_dimensionLabels.size(); ++it, ++row)
        {
            m_dimensionLabels.at(row)->setText(it.value()->Name() + QLatin1Char(':'));
        }
    }

    m_unitLabel->setText(tr("Unit:"));
    m_unitCombo->setItemText(0, tr("cm"));
    m_unitCombo->setItemText(1, tr("inch"));

    m_sourceLabel->setText(tr("What to nest:"));
    m_sourceCombo->setItemText(m_sourceCombo->findData(QStringLiteral("range")), tr("Size range"));
    m_sourceCombo->setItemText(m_sourceCombo->findData(QStringLiteral("single")), tr("Single size"));
    m_rangeLabel->setText(tr("Size range:"));

    m_gapLabel->setText(tr("Space between pieces:"));
    m_gapLabel->setToolTip(tr("Room left between two pieces, to be able to lift one without "
                              "dragging its neighbour.\n\n"
                              "Never less than 5 cm: below that a projected plan is a cutting plan, "
                              "and it is no longer what this format is for."));

    m_marginLabel->setText(tr("Space from the edge:"));
    m_marginLabel->setToolTip(tr("Room kept around the whole plan - a safety margin, and a place to "
                                 "put a hand.\n\n"
                                 "It is the first thing to reduce when the plan does not fit in "
                                 "500 x 500 cm: the pieces themselves rarely are the problem."));

    // Une phrase, sans tiret cadratin : elle dit ce que le fichier porte et que le canvas ne montre
    // pas — ni plus, ni moins.
    m_calibrationLabel->setText(
        tr("The PDF also carries three calibration layers, which the Ostara canvas does not show: "
           "a 5 cm grid, a 3 in grid and a magenta background."));

    // Les champs sont réécrits depuis les millimètres stockés : sinon ils s'ouvriraient vides, ou
    // dans l'unité d'avant.
    WriteLengths();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportProjectorWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
