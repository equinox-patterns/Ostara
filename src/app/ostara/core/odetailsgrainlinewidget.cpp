/************************************************************************
 **
 **  @file   odetailsgrainlinewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Grainline editor widget for the Details mode options dock
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

#include "odetailsgrainlinewidget.h"

#include "otranslation.h"

#include "../vmisc/typedef.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRectF>
#include <QVBoxLayout>

#include <QtMath>

namespace
{
const struct
{
    int length;
    const char *label;
} kGrainlinePresets[] = {
    {5,  QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "5 cm")},
    {10, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "10 cm")},
    {20, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "20 cm")},
    {40, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "40 cm")},
    {80, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "80 cm")},
};
constexpr int kGrainlinePresetCount = static_cast<int>(sizeof(kGrainlinePresets) / sizeof(kGrainlinePresets[0]));

// UI order of the ends choices. On the main line the far end (p2) reads as top and the near end
// (p1) as bottom; on the crossed line p1 is left and p2 right, because the cross is the main line
// turned 90° about its centre and the grainline is created vertical.
const GrainlineEnds kMainEndsOrder[] = {GrainlineEnds::Both, GrainlineEnds::Second, GrainlineEnds::First};
const GrainlineEnds kCrossEndsOrder[] = {GrainlineEnds::Both, GrainlineEnds::First, GrainlineEnds::Second};

auto MainEndsLabel(GrainlineEnds ends) -> const char *
{
    switch (ends)
    {
        case GrainlineEnds::Both:
            return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Both");
        case GrainlineEnds::First:
            return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Bottom");
        case GrainlineEnds::Second:
            return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Top");
    }
    return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Both");
}

auto CrossEndsLabel(GrainlineEnds ends) -> const char *
{
    switch (ends)
    {
        case GrainlineEnds::Both:
            return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Both");
        case GrainlineEnds::First:
            return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Left");
        case GrainlineEnds::Second:
            return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Right");
    }
    return QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Both");
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsGrainlineWidget::ODetailsGrainlineWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ⚠️ **Les textes passent par le poseur.** La classe a un `RetranslateUI()` complet, mais elle
    // est instanciée deux fois — une fois pour la page Pièce, une fois pour la page de groupe — et
    // seul l'appel de la page Pièce existe : l'instance du groupe gardait donc ses huit textes dans
    // la langue du premier affichage. Avec le poseur, les deux instances sont couvertes par le
    // moteur, sans que l'appelant ait à savoir qu'il y en a deux.
    m_grainlineGroup = new QGroupBox();
    OTranslation::SetTitle(this, m_grainlineGroup, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Grainline"));
    auto *grainLayout = new QVBoxLayout(m_grainlineGroup);

    m_activeCheck = new QCheckBox();
    OTranslation::SetText(this, m_activeCheck, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Activate grainline"));
    grainLayout->addWidget(m_activeCheck);

    // Length presets
    m_lenLabel = new QLabel();
    OTranslation::SetText(this, m_lenLabel, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Length"));
    auto *lenLayout = new QHBoxLayout();
    m_lengthGroup = new QButtonGroup(this);
    m_lengthGroup->setExclusive(true);

    for (int i = 0; i < kGrainlinePresetCount; ++i)
    {
        auto *btn = new QPushButton();
        OTranslation::SetText(this, btn, kGrainlinePresets[i].label);
        btn->setCheckable(true);
        btn->setProperty("lengthValue", kGrainlinePresets[i].length);
        m_lengthGroup->addButton(btn, i);
        lenLayout->addWidget(btn);
    }
    grainLayout->addWidget(m_lenLabel);
    grainLayout->addLayout(lenLayout);

    // Rotation
    auto *rotForm = new QFormLayout();
    m_rotationSpin = new QDoubleSpinBox();
    m_rotationSpin->setRange(-360.0, 360.0);
    m_rotationSpin->setDecimals(1);
    m_rotationSpin->setSuffix(QStringLiteral("°"));
    rotForm->addRow(tr("Rotation"), m_rotationSpin);
    m_rotLabel = qobject_cast<QLabel *>(rotForm->labelForField(m_rotationSpin));
    // Le libellé de la ligne appartient au QFormLayout : on l'enregistre auprès du poseur, sans
    // quoi il resterait dans la langue du premier affichage là où `RetranslateUI()` n'est pas appelé.
    if (m_rotLabel != nullptr)
    {
        OTranslation::SetText(this, m_rotLabel, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Rotation"));
    }
    grainLayout->addLayout(rotForm);

    // Line count: one grainline, or the crossed one (the main line turned 90° about its centre).
    auto *lineLayout = new QHBoxLayout();
    m_linesGroup = new QButtonGroup(this);
    m_linesGroup->setExclusive(true);

    m_simpleBtn = new QPushButton();
    OTranslation::SetText(this, m_simpleBtn, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Simple"));
    m_simpleBtn->setCheckable(true);
    m_linesGroup->addButton(m_simpleBtn, 0);
    lineLayout->addWidget(m_simpleBtn);

    m_doubleBtn = new QPushButton();
    OTranslation::SetText(this, m_doubleBtn, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Double"));
    m_doubleBtn->setCheckable(true);
    m_linesGroup->addButton(m_doubleBtn, 1);
    lineLayout->addWidget(m_doubleBtn);

    // Same shape as Length: a label above the row, because three buttons alone say nothing about
    // what they set.
    m_linesLabel = new QLabel();
    OTranslation::SetText(this, m_linesLabel, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Lines"));
    grainLayout->addWidget(m_linesLabel);
    grainLayout->addLayout(lineLayout);

    // Which end(s) of the main line carry an arrowhead.
    m_mainEndsGroup = new QButtonGroup(this);
    m_mainEndsGroup->setExclusive(true);
    auto *mainEndsLayout = new QHBoxLayout();

    for (const GrainlineEnds ends : kMainEndsOrder)
    {
        auto *btn = new QPushButton();
        OTranslation::SetText(this, btn, MainEndsLabel(ends));
        btn->setCheckable(true);
        m_mainEndsGroup->addButton(btn, static_cast<int>(ends));
        mainEndsLayout->addWidget(btn);
    }
    m_mainEndsLabel = new QLabel();
    OTranslation::SetText(this, m_mainEndsLabel, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Arrows"));
    grainLayout->addWidget(m_mainEndsLabel);
    grainLayout->addLayout(mainEndsLayout);

    // The crossed line, same choice. Its row is the visible half of the Double button: hidden
    // while the grainline is a single line, so the dock never shows a choice that cannot act.
    m_crossRow = new QWidget();
    auto *crossRowLayout = new QVBoxLayout(m_crossRow);
    crossRowLayout->setContentsMargins(0, 0, 0, 0);

    m_crossEndsLabel = new QLabel();
    OTranslation::SetText(this, m_crossEndsLabel, QT_TRANSLATE_NOOP("ODetailsGrainlineWidget", "Crossed arrows"));
    crossRowLayout->addWidget(m_crossEndsLabel);

    auto *crossButtonsLayout = new QHBoxLayout();
    m_crossEndsGroup = new QButtonGroup(this);
    m_crossEndsGroup->setExclusive(true);

    for (const GrainlineEnds ends : kCrossEndsOrder)
    {
        auto *btn = new QPushButton();
        OTranslation::SetText(this, btn, CrossEndsLabel(ends));
        btn->setCheckable(true);
        m_crossEndsGroup->addButton(btn, static_cast<int>(ends));
        crossButtonsLayout->addWidget(btn);
    }
    crossRowLayout->addLayout(crossButtonsLayout);

    grainLayout->addWidget(m_crossRow);
    m_crossRow->setVisible(false);

    mainLayout->addWidget(m_grainlineGroup);

    ConnectControls();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::RetranslateUI()
{
    m_grainlineGroup->setTitle(tr("Grainline"));
    m_activeCheck->setText(tr("Activate grainline"));
    m_lenLabel->setText(tr("Length"));
    if (m_rotLabel)
    {
        m_rotLabel->setText(tr("Rotation"));
    }
    m_simpleBtn->setText(tr("Simple"));
    m_doubleBtn->setText(tr("Double"));
    m_linesLabel->setText(tr("Lines"));
    m_mainEndsLabel->setText(tr("Arrows"));
    m_crossEndsLabel->setText(tr("Crossed arrows"));

    for (auto *btn : m_mainEndsGroup->buttons())
    {
        btn->setText(tr(MainEndsLabel(static_cast<GrainlineEnds>(m_mainEndsGroup->id(btn)))));
    }
    for (auto *btn : m_crossEndsGroup->buttons())
    {
        btn->setText(tr(CrossEndsLabel(static_cast<GrainlineEnds>(m_crossEndsGroup->id(btn)))));
    }
    for (auto *btn : m_lengthGroup->buttons())
    {
        const int id = m_lengthGroup->id(btn);
        if (id >= 0 && id < kGrainlinePresetCount)
        {
            btn->setText(tr(kGrainlinePresets[id].label));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::ConnectControls()
{
    connect(m_activeCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (checked)
        {
            // Rotation is still forced to 90 on activation (existing behavior).
            const bool wasUpdating = m_updating;
            m_updating = true;
            m_rotationSpin->setValue(90.0);
            m_updating = wasUpdating;
        }

        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([this, checked](VGrainlineData &geom)
        {
            geom.SetEnabled(checked);
            geom.SetVisible(checked);
            if (!checked)
            {
                return;
            }

            // The pre-selected preset applies only when the grainline has no real
            // length yet (fresh piece, or file fallback "1"). Otherwise the current
            // length is kept, so re-activating does not reset a canvas-resized
            // grainline back to a preset.
            double appliedLength = kGrainlinePresets[1].length; // default 10 cm
            if (auto *btn = m_lengthGroup->checkedButton())
            {
                appliedLength = btn->property("lengthValue").toDouble();
            }
            const QString len = geom.GetLength();
            if (len.isEmpty() || len == QLatin1Char('1'))
            {
                geom.SetLength(QString::number(appliedLength));
            }
            else
            {
                bool ok = false;
                const double existing = len.toDouble(&ok);
                if (ok)
                {
                    appliedLength = existing;
                }
            }
            geom.SetRotation(QStringLiteral("90"));

            // Center the grainline when a provider is given; otherwise leave the
            // position to the canvas (a group holder centers it itself).
            if (m_centerProvider)
            {
                const std::optional<QPointF> centerOpt = m_centerProvider();
                if (centerOpt.has_value())
                {
                    const QPointF center = *centerOpt;
                    // Offset so the grainline midpoint sits at the bbox center
                    // (not its tail). For a vertical (90°) grainline.
                    const qreal rad = qDegreesToRadians(90.0);
                    const QPointF tail(center.x() - appliedLength * qCos(rad) / 2.0,
                                       center.y() + appliedLength * qSin(rad) / 2.0);
                    geom.SetPos(tail);
                }
            }
        });
    });

    connect(m_lengthGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        // Inactive grainline: the click only pre-selects the preset (applied on
        // activation). Active grainline: re-apply the length immediately.
        if (!m_activeCheck->isChecked())
        {
            return;
        }
        const int length = button->property("lengthValue").toInt();
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([length](VGrainlineData &geom)
        {
            geom.SetLength(QString::number(length));
        });
    });

    connect(m_rotationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double value)
    {
        if (m_updating)
        {
            return;
        }
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([value](VGrainlineData &geom)
        {
            geom.SetRotation(QString::number(value));
        });
    });

    // One handler for the three exclusive groups: a click only moves the check mark, the arrow
    // type is recomputed from the three states, so the twelve-value table stays in floatitemdef.h.
    const auto onArrowChoice = [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        ApplyArrowChoice();
    };
    connect(m_linesGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this,
            onArrowChoice);
    connect(m_mainEndsGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this,
            onArrowChoice);
    connect(m_crossEndsGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this,
            onArrowChoice);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::Show(const VGrainlineData &grainline,
                                   std::function<std::optional<QPointF>()> centerProvider)
{
    m_centerProvider = std::move(centerProvider);

    m_updating = true;

    m_activeCheck->setChecked(grainline.IsEnabled());

    UpdatePresetsState(grainline.IsEnabled());

    {
        const QString rotFormula = grainline.GetRotation();
        bool ok = false;
        const double rotValue = rotFormula.toDouble(&ok);
        m_rotationSpin->setValue(ok ? rotValue : 0.0);
    }

    {
        // Every state the file can hold is restored exactly. The two one-arrow values used to be
        // folded into "Simple" and silently rewritten to two arrows on the next click.
        const GrainlineArrows arrows = DecodeGrainlineArrows(grainline.GetArrowType());

        if (auto *btn = m_linesGroup->button(arrows.crossed ? 1 : 0))
        {
            btn->setChecked(true);
        }
        if (auto *btn = m_mainEndsGroup->button(static_cast<int>(arrows.main)))
        {
            btn->setChecked(true);
        }
        if (auto *btn = m_crossEndsGroup->button(static_cast<int>(arrows.cross)))
        {
            btn->setChecked(true);
        }

        UpdateCrossRowVisibility(arrows.crossed);
    }

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::UpdatePresetsState(bool grainlineEnabled)
{
    // Presets stay interactive in both states: when the grainline is active a
    // click re-applies the length; when inactive it is a pre-selection applied
    // on activation. Only make sure a default (10 cm) is selected when the
    // grainline is inactive, so activation always has a value to apply.
    if (!grainlineEnabled && m_lengthGroup->checkedButton() == nullptr)
    {
        if (auto *btn = m_lengthGroup->button(1)) // 10 cm by default
        {
            btn->setChecked(true);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::Clear()
{
    m_centerProvider = {};
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsGrainlineWidget::CurrentArrows() const -> GrainlineArrows
{
    GrainlineArrows arrows;
    arrows.crossed = m_linesGroup->checkedId() == 1;

    const int mainEnds = m_mainEndsGroup->checkedId();
    if (mainEnds >= 0)
    {
        arrows.main = static_cast<GrainlineEnds>(mainEnds);
    }

    const int crossEnds = m_crossEndsGroup->checkedId();
    if (crossEnds >= 0)
    {
        arrows.cross = static_cast<GrainlineEnds>(crossEnds);
    }

    return arrows;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::ApplyArrowChoice()
{
    const GrainlineArrows arrows = CurrentArrows();
    UpdateCrossRowVisibility(arrows.crossed);

    if (!m_modifyPending)
    {
        return;
    }

    const auto arrowType = EncodeGrainlineArrows(arrows);
    m_modifyPending([arrowType](VGrainlineData &geom) { geom.SetArrowType(arrowType); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::UpdateCrossRowVisibility(bool crossed)
{
    if (m_crossRow != nullptr)
    {
        m_crossRow->setVisible(crossed);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::SetModifyPending(std::function<void(std::function<void(VGrainlineData &)>)> modify)
{
    m_modifyPending = std::move(modify);
}
