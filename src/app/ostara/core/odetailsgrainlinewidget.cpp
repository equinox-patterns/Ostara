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

#include "../vgeometry/vpointf.h"
#include "../vmisc/typedef.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/vcontainer.h"

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
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsGrainlineWidget::ODetailsGrainlineWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_grainlineGroup = new QGroupBox(tr("Grainline"));
    auto *grainLayout = new QVBoxLayout(m_grainlineGroup);

    m_activeCheck = new QCheckBox(tr("Activate grainline"));
    grainLayout->addWidget(m_activeCheck);

    // Length presets
    m_lenLabel = new QLabel(tr("Length"));
    auto *lenLayout = new QHBoxLayout();
    m_lengthGroup = new QButtonGroup(this);
    m_lengthGroup->setExclusive(true);

    for (int i = 0; i < kGrainlinePresetCount; ++i)
    {
        auto *btn = new QPushButton(tr(kGrainlinePresets[i].label));
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
    grainLayout->addLayout(rotForm);

    // Arrow type
    auto *arrowLayout = new QHBoxLayout();
    m_arrowGroup = new QButtonGroup(this);
    m_arrowGroup->setExclusive(true);

    m_simpleBtn = new QPushButton(tr("Simple"));
    m_simpleBtn->setCheckable(true);
    m_simpleBtn->setProperty("arrowType", static_cast<int>(GrainlineArrowDirection::twoWaysUpDown));
    m_arrowGroup->addButton(m_simpleBtn, 0);
    arrowLayout->addWidget(m_simpleBtn);

    m_doubleBtn = new QPushButton(tr("Double"));
    m_doubleBtn->setCheckable(true);
    m_doubleBtn->setProperty("arrowType", static_cast<int>(GrainlineArrowDirection::fourWays));
    m_arrowGroup->addButton(m_doubleBtn, 1);
    arrowLayout->addWidget(m_doubleBtn);

    grainLayout->addLayout(arrowLayout);

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
        m_modifyPending([this, checked](VPiece &det)
        {
            auto &geom = det.GetGrainlineGeometry();
            geom.SetEnabled(checked);
            geom.SetVisible(checked);
            if (!checked)
            {
                return;
            }

            // Detach from any existing pins — free-floating grainline.
            geom.SetCenterPin(NULL_ID);
            geom.SetTopPin(NULL_ID);
            geom.SetBottomPin(NULL_ID);

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

            // Center the grainline on the piece bounding box.
            if (m_data != nullptr)
            {
                const QVector<VPointF> points = det.MainPathNodePoints(m_data);
                if (!points.isEmpty())
                {
                    QRectF bbox;
                    for (const auto &p : points)
                    {
                        const QPointF pt = p.toQPointF();
                        bbox = bbox.isNull() ? QRectF(pt, QSizeF(0, 0))
                                             : bbox.united(QRectF(pt, QSizeF(0, 0)));
                    }
                    const QPointF pieceOffset(det.GetMx(), det.GetMy());
                    const QPointF center = bbox.center() - pieceOffset;

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
        m_modifyPending([length](VPiece &det)
        {
            det.GetGrainlineGeometry().SetLength(QString::number(length));
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
        m_modifyPending([value](VPiece &det)
        {
            det.GetGrainlineGeometry().SetRotation(QString::number(value));
        });
    });

    connect(m_arrowGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        const auto arrowType = static_cast<GrainlineArrowDirection>(button->property("arrowType").toInt());
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([arrowType](VPiece &det) { det.GetGrainlineGeometry().SetArrowType(arrowType); });
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::Show(const VPiece &det, const VContainer *data)
{
    m_data = data;

    const VGrainlineData &grainline = det.GetGrainlineGeometry();

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
        const auto arrowType = grainline.GetArrowType();
        const int btnIdx = (arrowType == GrainlineArrowDirection::fourWays) ? 1 : 0;
        if (auto *btn = m_arrowGroup->button(btnIdx))
        {
            btn->setChecked(true);
        }
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
    m_data = nullptr;
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGrainlineWidget::SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify)
{
    m_modifyPending = std::move(modify);
}
