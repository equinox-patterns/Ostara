/************************************************************************
 **
 **  @file   preferencespatternpage.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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

#include "preferencespatternpage.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vwidgets/vmousewheelwidgetadjustmentguard.h"
#include "ui_preferencespatternpage.h"

//---------------------------------------------------------------------------------------------------------------------
PreferencesPatternPage::PreferencesPatternPage(QWidget *parent)
  : QWidget(parent),
    ui(new Ui::PreferencesPatternPage)
{
    ui->setupUi(this);
    RetranslateUi();

    // Prevent stealing focus when scrolling
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->doubleSpinBoxLineWidth);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->comboBoxLineWidthUnit);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->defaultSeamAllowance);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->spinBoxOpacity);

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

    //----------------------- Unit setup
    // set default unit
    const Unit defUnit = QLocale().measurementSystem() == QLocale::MetricSystem ? Unit::Mm : Unit::Inch;
    if (const qint32 indexUnit = ui->comboBoxLineWidthUnit->findData(static_cast<int>(defUnit)); indexUnit != -1)
    {
        ui->comboBoxLineWidthUnit->setCurrentIndex(indexUnit);
    }

    connect(ui->comboBoxLineWidthUnit, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]()
            {
                const auto lineUnit = static_cast<Unit>(ui->comboBoxLineWidthUnit->currentData().toInt());
                const qreal value = UnitConvertor(ui->doubleSpinBoxLineWidth->value(), m_oldLineUnit, lineUnit);
                ui->doubleSpinBoxLineWidth->setDecimals(lineUnit == Unit::Mm ? 1 : 6);
                ui->doubleSpinBoxLineWidth->setMinimum(
                    UnitConvertor(ui->doubleSpinBoxLineWidth->minimum(), m_oldLineUnit, lineUnit));
                ui->doubleSpinBoxLineWidth->setMaximum(
                    UnitConvertor(ui->doubleSpinBoxLineWidth->maximum(), m_oldLineUnit, lineUnit));
                ui->doubleSpinBoxLineWidth->setValue(value);
                m_oldLineUnit = lineUnit;
            });

    m_oldLineUnit = static_cast<Unit>(ui->comboBoxLineWidthUnit->currentData().toInt());
    ui->doubleSpinBoxLineWidth->setDecimals(m_oldLineUnit == Unit::Mm ? 1 : 6);
    ui->doubleSpinBoxLineWidth->setMinimum(UnitConvertor(VCommonSettings::MinimalLineWidth(), Unit::Mm, m_oldLineUnit));
    ui->doubleSpinBoxLineWidth->setMaximum(UnitConvertor(VCommonSettings::MaximalLineWidth(), Unit::Mm, m_oldLineUnit));
    ui->doubleSpinBoxLineWidth->setValue(UnitConvertor(settings->GetLineWidth(), Unit::Mm, m_oldLineUnit));

    InitDefaultSeamAllowance();

    ui->checkBoxSewLineOnDrawing->setChecked(settings->GetSewLineOnDrawing());
    ui->doublePassmarkCheck->setChecked(settings->IsDoublePassmark());
    ui->checkBoxHideMainPath->setChecked(settings->IsHideMainPath());

    ui->spinBoxOpacity->setValue(settings->GetBackgroundImageDefOpacity());
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesPatternPage::~PreferencesPatternPage()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesPatternPage::Apply() -> QStringList
{
    QStringList preferences;

    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    settings->SetLineWidth(UnitConvertor(ui->doubleSpinBoxLineWidth->value(), m_oldLineUnit, Unit::Mm));

    settings->SetDefaultSeamAllowance(ui->defaultSeamAllowance->value());

    settings->SetSewLineOnDrawing(ui->checkBoxSewLineOnDrawing->isChecked());
    settings->SetHideMainPath(ui->checkBoxHideMainPath->isChecked());
    if (settings->IsDoublePassmark() != ui->doublePassmarkCheck->isChecked())
    {
        settings->SetDoublePassmark(ui->doublePassmarkCheck->isChecked());
        VAbstractValApplication::VApp()->getCurrentDocument()->LiteParseTree(Document::LiteParse);
    }

    settings->SetBackgroundImageDefOpacity(ui->spinBoxOpacity->value());

    return preferences;
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesPatternPage::InitDefaultSeamAllowance()
{
    ui->defaultSeamAllowance->setValue(VAbstractValApplication::VApp()->ValentinaSettings()->GetDefaultSeamAllowance());
    ui->defaultSeamAllowance->setSuffix(
        UnitsToStr(StrToUnits(VAbstractValApplication::VApp()->ValentinaSettings()->GetUnit()), true));
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesPatternPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // retranslate designer form (single inheritance approach)
        RetranslateUi();
        ui->retranslateUi(this);
    }

    // remember to call base class implementation
    QWidget::changeEvent(event);
}


//---------------------------------------------------------------------------------------------------------------------
void PreferencesPatternPage::InitUnits()
{
    ui->comboBoxLineWidthUnit->addItem(tr("Millimiters"), static_cast<int>(Unit::Mm));
    ui->comboBoxLineWidthUnit->addItem(tr("Inches"), static_cast<int>(Unit::Inch));
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesPatternPage::RetranslateUi()
{
    const QSignalBlocker blocker(ui->comboBoxLineWidthUnit);
    const int unit = ui->comboBoxLineWidthUnit->currentData().toInt();
    ui->comboBoxLineWidthUnit->clear();
    InitUnits();
    ui->comboBoxLineWidthUnit->setCurrentIndex(ui->comboBoxLineWidthUnit->findData(unit));
}

