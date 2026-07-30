/************************************************************************
 **
 **  @file   dialogseamallowance_gradation.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Gradation placeholders for DialogSeamAllowance.
 **  @note   Extracted from dialogseamallowance.cpp by Sophie Denys
 **          <contact@sophiedenys.com> as part of Ostara refactoring.
 **
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "dialogseamallowance.h"
#include "ui_tabpaths.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/variables/vmeasurement.h"

#include <QMenu>
//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitGradationPlaceholdersMenu()
{
    QChar const per('%');
    auto i = m_gradationPlaceholders.constBegin();
    while (i != m_gradationPlaceholders.constEnd())
    {
        auto [first, second] = i.value();
        QAction *action = m_placeholdersMenu->addAction(first);
        action->setData(per + i.key() + per);
        connect(action, &QAction::triggered, this, &DialogSeamAllowance::InsertGradationPlaceholder);
        ++i;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitGradationPlaceholders()
{
    // Pattern tags
    QString const heightValue = QString::number(VAbstractValApplication::VApp()->GetDimensionHeight());
    m_gradationPlaceholders.insert(pl_height, qMakePair(tr("Height", "dimension"), heightValue));
    m_gradationPlaceholders.insert(pl_dimensionX, qMakePair(tr("Dimension X", "dimension"), heightValue));

    QString const sizeValue = QString::number(VAbstractValApplication::VApp()->GetDimensionSize());
    m_gradationPlaceholders.insert(pl_size, qMakePair(tr("Size", "dimension"), sizeValue));
    m_gradationPlaceholders.insert(pl_dimensionY, qMakePair(tr("Dimension Y", "dimension"), sizeValue));

    QString const hipValue = QString::number(VAbstractValApplication::VApp()->GetDimensionHip());
    m_gradationPlaceholders.insert(pl_hip, qMakePair(tr("Hip", "dimension"), hipValue));
    m_gradationPlaceholders.insert(pl_dimensionZ, qMakePair(tr("Dimension Z", "dimension"), hipValue));

    QString const waistValue = QString::number(VAbstractValApplication::VApp()->GetDimensionWaist());
    m_gradationPlaceholders.insert(pl_waist, qMakePair(tr("Waist", "dimension"), waistValue));
    m_gradationPlaceholders.insert(pl_dimensionW, qMakePair(tr("Dimension W", "dimension"), waistValue));

    {
        QString label = VAbstractValApplication::VApp()->GetDimensionHeightLabel();
        m_gradationPlaceholders.insert(
            pl_heightLabel, qMakePair(tr("Height label", "dimension"), not label.isEmpty() ? label : heightValue));
        m_gradationPlaceholders.insert(pl_dimensionXLabel, qMakePair(tr("Dimension X label", "dimension"),
                                                                     not label.isEmpty() ? label : heightValue));

        label = VAbstractValApplication::VApp()->GetDimensionSizeLabel();
        m_gradationPlaceholders.insert(
            pl_sizeLabel, qMakePair(tr("Size label", "dimension"), not label.isEmpty() ? label : sizeValue));
        m_gradationPlaceholders.insert(pl_dimensionYLabel, qMakePair(tr("Dimension Y label", "dimension"),
                                                                     not label.isEmpty() ? label : sizeValue));

        label = VAbstractValApplication::VApp()->GetDimensionHipLabel();
        m_gradationPlaceholders.insert(pl_hipLabel,
                                       qMakePair(tr("Hip label", "dimension"), not label.isEmpty() ? label : hipValue));
        m_gradationPlaceholders.insert(pl_dimensionZLabel, qMakePair(tr("Dimension Z label", "dimension"),
                                                                     not label.isEmpty() ? label : hipValue));

        label = VAbstractValApplication::VApp()->GetDimensionWaistLabel();
        m_gradationPlaceholders.insert(
            pl_waistLabel, qMakePair(tr("Waist label", "dimension"), not label.isEmpty() ? label : waistValue));
        m_gradationPlaceholders.insert(pl_dimensionWLabel, qMakePair(tr("Dimension W label", "dimension"),
                                                                     not label.isEmpty() ? label : waistValue));
    }

    {
        const QMap<QString, QSharedPointer<VMeasurement>> measurements = data.DataMeasurements();
        auto i = measurements.constBegin();
        while (i != measurements.constEnd())
        {
            QString const description = i.value()->GetGuiText().isEmpty() ? i.key() : i.value()->GetGuiText();
            m_gradationPlaceholders.insert(
                pl_measurement + i.key(),
                qMakePair(tr("Measurement: %1").arg(description), QString::number(*i.value()->GetValue())));
            m_gradationPlaceholders.insert(pl_measurement + i.key() + pl_valueAlias,
                                           qMakePair(tr("Measurement %1 value alias").arg(description),
                                                     i.value()->GetValueAlias()));
            ++i;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InsertGradationPlaceholder()
{
    auto *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        uiTabPaths->lineEditGradationLabel->insert(action->data().toString());
        uiTabPaths->lineEditGradationLabel->setFocus();
    }
}
