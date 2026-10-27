/************************************************************************
 **
 **  @file   preferencesgeneralpage.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 27/10/2026.
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

#include "preferencesgeneralpage.h"
#include "../../core/vapplication.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/def.h"
#include "../vmisc/literals.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vwidgets/vmousewheelwidgetadjustmentguard.h"
#include "ui_preferencesgeneralpage.h"
#include "vcommonsettings.h"
#include "vtheme.h"

#include <QCoreApplication>
#include <QPushButton>
#include <QTimer>

//---------------------------------------------------------------------------------------------------------------------
PreferencesGeneralPage::PreferencesGeneralPage(QWidget *parent)
  : QWidget(parent),
    ui(new Ui::PreferencesGeneralPage)
{
    ui->setupUi(this);
    RetranslateUi();

    // Prevent stealing focus when scrolling
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->autoTime);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->langCombo);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->unitCombo);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(ui->comboBoxThemeMode);

    ui->autoSaveCheck->setChecked(VAbstractValApplication::VApp()->ValentinaSettings()->GetAutosaveState());

    InitLanguages(ui->langCombo);
    // ⚠️ **`currentIndexChanged`, pas `activated`.** `activated` ne se lève que sur une sélection
    // confirmée dans la liste déroulante : sur un geste qui ne la confirme pas, le drapeau restait
    // faux et la langue n'était plus jamais enregistrée — le changement ne prenait pas du tout.
    // `currentIndexChanged` est sûr ici : rien ne touche ce combo par programme après cette
    // connexion (vérifié), donc il ne se lève que sur une action de l'utilisatrice.
    connect(ui->langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]() { m_langChanged = true; });

    //----------------------- Unit setup
    // set default unit
    if (const qint32 indexUnit =
            ui->unitCombo->findData(VAbstractValApplication::VApp()->ValentinaSettings()->GetUnit());
        indexUnit != -1)
    {
        ui->unitCombo->setCurrentIndex(indexUnit);
    }

    connect(ui->unitCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]() { m_unitChanged = true; });

    //----------------------------- Pattern Editing
    connect(ui->resetWarningsButton, &QPushButton::released, this,
            []()
            {
                VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

                settings->SetConfirmItemDelete(true);
                settings->SetConfirmFormatRewriting(true);
                settings->SetAskRegenerateOutputs(true);
            });

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

    ui->checkBoxFreeCurve->setChecked(settings->IsFreeCurveMode());
    ui->checkBoxTranslateFormula->setChecked(settings->IsTranslateFormula());

    // Theme
    SetThemeModeComboBox();
    const int index = ui->comboBoxThemeMode->findData(static_cast<int>(settings->GetThemeMode()));
    if (index != -1)
    {
        ui->comboBoxThemeMode->setCurrentIndex(index);
    }

    // Native dialogs
    ui->checkBoxDontUseNativeDialog->setChecked(settings->IsDontUseNativeDialog());
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesGeneralPage::~PreferencesGeneralPage()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesGeneralPage::Apply() -> QStringList
{
    QStringList preferences;
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetAutosaveState(ui->autoSaveCheck->isChecked());
    settings->SetAutosaveTime(ui->autoTime->value());

    QTimer *autoSaveTimer = VApplication::VApp()->getAutoSaveTimer();
    // autoSaveTimer may be null if MainWindow hasn't been created yet
    // (e.g., Preferences opened from WelcomeScreen before any pattern is loaded).
    if (autoSaveTimer != nullptr)
    {
        ui->autoSaveCheck->isChecked() ? autoSaveTimer->start(std::chrono::minutes{ui->autoTime->value()})
                                       : autoSaveTimer->stop();
    }

    if (auto themeMode = static_cast<VThemeMode>(ui->comboBoxThemeMode->currentData().toInt());
        settings->GetThemeMode() != themeMode)
    {
        settings->SetThemeMode(themeMode);
        VTheme::Instance()->ResetThemeSettings();
        // Force immediate repaint — the Settings dialog is still open and the
        // modal event loop may defer paint events otherwise.
        QCoreApplication::processEvents();
    }

    if (settings->IsDontUseNativeDialog() != ui->checkBoxDontUseNativeDialog->isChecked())
    {
        settings->SetDontUseNativeDialog(ui->checkBoxDontUseNativeDialog->isChecked());
    }

    settings->SetFreeCurveMode(ui->checkBoxFreeCurve->isChecked());
    settings->SetTranslateFormula(ui->checkBoxTranslateFormula->isChecked());

    if (m_langChanged)
    {
        // ⚠️ **Le drapeau retombe, comme son voisin.** Il ne retombait jamais : une fois la
        // liste touchée, chaque « Appliquer » de la session — et « Appliquer » puis « OK » en font
        // deux — réécrivait la langue depuis la liste, sans que l'utilisatrice ait rien redemandé.
        // C'est par là que le réglage a pu repartir en anglais tout seul.
        m_langChanged = false;
        const auto locale = qvariant_cast<QString>(ui->langCombo->currentData());
        if (locale != settings->GetLocale())
        {
            settings->SetLocale(locale);
            VAbstractApplication::VApp()->LoadTranslation(locale);
        }
    }

    if (m_unitChanged)
    {
        const auto unit = qvariant_cast<QString>(ui->unitCombo->currentData());
        settings->SetUnit(unit);
        m_unitChanged = false;
        preferences.append(tr("default unit"));
    }

    return preferences;
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesGeneralPage::changeEvent(QEvent *event)
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
void PreferencesGeneralPage::SetThemeModeComboBox()
{
    ui->comboBoxThemeMode->clear();
    ui->comboBoxThemeMode->addItem(tr("Light", "theme"), static_cast<int>(VThemeMode::Light));
    ui->comboBoxThemeMode->addItem(tr("Dark", "theme"), static_cast<int>(VThemeMode::Dark));
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesGeneralPage::InitUnits()
{
    ui->unitCombo->addItem(tr("Centimeters"), unitCM);
    ui->unitCombo->addItem(tr("Millimiters"), unitMM);
    ui->unitCombo->addItem(tr("Inches"), unitINCH);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesGeneralPage::RetranslateUi()
{
    const QSignalBlocker blocker(ui->unitCombo);
    const auto unit = qvariant_cast<QString>(ui->unitCombo->currentData());
    ui->unitCombo->clear();
    InitUnits();
    ui->unitCombo->setCurrentIndex(ui->unitCombo->findData(unit));
}
