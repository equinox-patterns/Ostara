/************************************************************************
 **
 **  @file   oexportplotterwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The Plotter tab of the Export dialog — the size to cut, and
 **          the mat a domestic plotter cuts it on.
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

#include "oexportplotterwidget.h"
#include "oexportgroupmemberswidget.h"

#include "../core/opiecegrouplayoutflags.h"
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
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
/// A size is compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportPlotterWidget::OExportPlotterWidget(const VContainer *data,
                                           const QSharedPointer<VMeasurements> &measurements,
                                           QWidget *parent)
  : QWidget(parent),
    m_data(data),
    m_measurements(measurements)
{
    BuildUi();

    // The labels are written by Retranslate and nowhere else: a tab that never ran it would come up
    // with every field unlabelled — and with the mat fields empty, since it is also what puts the
    // stored millimetres back on screen.
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPlotterWidget::BuildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *form = new QFormLayout();
    layout->addLayout(form);

    PopulateDimensions(form);

    // ── Le tapis ─────────────────────────────────────────────────────────
    //
    // La seule question que cet onglet pose vraiment. Rien ne pourrait la modéliser : il y a autant
    // de zones utiles que de modèles de machine, et le nombre que la machine coupe vraiment n'est
    // écrit que dans son propre logiciel. L'utilisatrice le lit là et le tape ici, une fois — le run
    // le garde, et c'est lui qui décide de tout l'éclatement.
    auto *matRow = new QHBoxLayout();

    m_matWidthEdit = new QLineEdit(this);
    connect(m_matWidthEdit, &QLineEdit::editingFinished, this, &OExportPlotterWidget::CommitMat);
    matRow->addWidget(m_matWidthEdit);

    m_matCrossLabel = new QLabel(this);
    matRow->addWidget(m_matCrossLabel);

    m_matHeightEdit = new QLineEdit(this);
    connect(m_matHeightEdit, &QLineEdit::editingFinished, this, &OExportPlotterWidget::CommitMat);
    matRow->addWidget(m_matHeightEdit);

    // **Le combo est rempli et posé AVANT d'être connecté.** Sinon `currentIndexChanged` part à la
    // construction, sur des champs encore vides, et `UpdateMatUnit` lirait zéro. C'est le piège que
    // l'onglet Cutting a déjà payé.
    m_matUnitCombo = MakeComboBox(this);
    for (const Unit &unit : OExportUnitFormat::FabricWidthUnits())
    {
        m_matUnitCombo->addItem(UnitsToStr(unit, true), static_cast<int>(unit));
    }
    m_matUnitCombo->setCurrentIndex(m_matUnitCombo->findData(static_cast<int>(m_matUnit)));
    connect(m_matUnitCombo, &QComboBox::currentIndexChanged, this,
            [this](int) { UpdateMatUnit(); });
    matRow->addWidget(m_matUnitCombo);

    matRow->addStretch();

    m_matLabel = new QLabel(this);
    form->addRow(m_matLabel, matRow);

    // ── Les groupes ──────────────────────────────────────────────────────
    //
    // Le plotter DÉCOMPOSE les groupes : chaque membre est une pièce à couper, une par une. C'est la
    // réponse du plotter à la question que le tissu pose autrement — sur une laize, un groupe se
    // coupe une fois, et l'onglet Cutting demande alors LEQUEL de ses membres. Ici il n'y a rien à
    // choisir : il y a des pièces à écarter. D'où une case par membre, cochée d'avance.
    //
    // La question est portée par un widget à part : le pré-vol du lot la repose, une fois, pour tous
    // les favoris de plotter (voir OExportGroupMembersWidget).
    m_groupMembers = new OExportGroupMembersWidget(m_data, this);
    connect(m_groupMembers, &OExportGroupMembersWidget::Changed, this, &OExportPlotterWidget::Changed);
    layout->addWidget(m_groupMembers);

    layout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPlotterWidget::PopulateDimensions(QFormLayout *form)
{
    if (m_measurements.isNull())
    {
        // A table without axes describes a single size: the document is already on it, and there is
        // nothing to pick along. The run reads the geometry as it stands.
        return;
    }

    // One row per dimension of the measurement table, in the table's own order — the same rows,
    // from the same table, as the Print and Cutting tabs offer for a single size.
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

        // Which axis the row feeds. The document is put on a size by naming its axes — X, Y, W — so
        // the row's rank in the table cannot say it: a table missing Y shifts every row below it by
        // one slot. The dimension with no slot of its own (Z) is still shown, and feeds nothing.
        const int dimensionIndex = static_cast<int>(it.key());
        m_dimensionCombos.append(combo);
        m_dimensionSlots.append(dimensionIndex < 3 ? dimensionIndex : -1);

        connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterWidget::ExcludedPieces() const -> QSet<quint32>
{
    return m_groupMembers != nullptr ? m_groupMembers->ExcludedPieces() : QSet<quint32>();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPlotterWidget::CommitMat()
{
    bool widthOk = false;
    bool heightOk = false;

    const qreal width = OExportUnitFormat::Parse(m_matWidthEdit->text(), m_matUnit, &widthOk);
    const qreal height = OExportUnitFormat::Parse(m_matHeightEdit->text(), m_matUnit, &heightOk);

    // Un champ illisible garde ce qu'il y avait : c'est un tapis, et le remettre à zéro ferait un
    // run qui ne coupe rien. Les deux champs sont réaffichés pour que l'utilisatrice voie ce qui est
    // retenu plutôt que ce qu'elle a tapé.
    if (widthOk && width > 0.0)
    {
        m_matWidthMm = width;
    }
    if (heightOk && height > 0.0)
    {
        m_matHeightMm = height;
    }

    m_matWidthEdit->setText(OExportUnitFormat::Format(m_matWidthMm, m_matUnit));
    m_matHeightEdit->setText(OExportUnitFormat::Format(m_matHeightMm, m_matUnit));

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPlotterWidget::UpdateMatUnit()
{
    // Lu avec l'ANCIENNE unité, PUIS changée : lire après le changement convertirait un tapis de
    // 11,5 pouces en 11,5 centimètres — le tapis changerait de taille sans que personne ne l'ait
    // demandé. C'est le défaut que l'onglet Print a payé avant celui-ci.
    CommitMat();

    m_matUnit = static_cast<Unit>(m_matUnitCombo->currentData().toInt());

    m_matWidthEdit->setText(OExportUnitFormat::Format(m_matWidthMm, m_matUnit));
    m_matHeightEdit->setText(OExportUnitFormat::Format(m_matHeightMm, m_matUnit));

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterWidget::MatWidthMm() const -> qreal { return m_matWidthMm; }

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterWidget::MatHeightMm() const -> qreal { return m_matHeightMm; }

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterWidget::MatUnit() const -> Unit { return m_matUnit; }

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterWidget::DimensionBase(int index) const -> qreal
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
auto OExportPlotterWidget::DimensionLabel(int index) const -> QString
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
auto OExportPlotterWidget::IsValid() const -> bool
{
    // Un tapis qui n'a pas de surface ne découpe rien, et le moteur n'a pas à le découvrir.
    if (m_matWidthMm <= 0.0 || m_matHeightMm <= 0.0)
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

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPlotterWidget::Retranslate()
{
    // Chaque ligne d'axe est NOMMÉE par la table de mesures, exactement comme la barre du haut du mode
    // Details et comme l'onglet Cutting la nomment : c'est la table qui dit comment un axe s'appelle,
    // et un second nom ici serait une chose de plus à tenir en accord avec elle. Un rang — « Size 2 » —
    // ne nomme rien : il dirait « le deuxième », pas « la stature ».
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

    m_matLabel->setText(tr("Cutting mat:"));
    m_matLabel->setToolTip(tr("The area your plotter can really cut — what its own software shows "
                              "you, not the size of the mat itself.\n\n"
                              "A piece larger than it is cut into pieces at the mat borders, to be "
                              "put back together like a puzzle. A smaller mat means more, smaller, "
                              "pieces: that is how you keep a safety margin."));
    m_matCrossLabel->setText(QStringLiteral("×"));

    // Les champs sont réécrits depuis les millimètres stockés : sans ça ils s'ouvriraient vides.
    m_matWidthEdit->setText(OExportUnitFormat::Format(m_matWidthMm, m_matUnit));
    m_matHeightEdit->setText(OExportUnitFormat::Format(m_matHeightMm, m_matUnit));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPlotterWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
