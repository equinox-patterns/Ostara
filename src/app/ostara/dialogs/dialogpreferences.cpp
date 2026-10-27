/************************************************************************
 **
 **  @file   dialogpreferences.cpp
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
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#include "dialogpreferences.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "configpages/preferencesfolderspage.h"
#include "configpages/preferencesgeneralpage.h"
#include "configpages/preferencespatternpage.h"
#include "configpages/preferencesshortcutspage.h"
#include "configpages/preferencestoolbarpage.h"
#include "ui_dialogpreferences.h"

#include <QApplication>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QShowEvent>

//---------------------------------------------------------------------------------------------------------------------
DialogPreferences::DialogPreferences(QWidget *parent)
  : QDialog(parent),
    ui(new Ui::DialogPreferences)
{
    // setupUi() must run before the pages are built: their constructors can re-enter the event loop and
    // changeEvent() would dereference ui->buttonBox while it is still indeterminate (upstream 4703e19e6).
    ui->setupUi(this);

    m_generalPage = new PreferencesGeneralPage;
    m_shortcutsPage = new PreferencesShortcutsPage;
    m_foldersPage = new PreferencesFoldersPage;
    m_toolBarPage = new PreferencesToolBarPage;
    m_patternPage = new PreferencesPatternPage;

#if defined(Q_OS_MAC)
    setWindowFlags(Qt::Window);
#endif

    VAbstractApplication::VApp()->Settings()->GetOsSeparator() ? setLocale(QLocale()) : setLocale(QLocale::c());

    QPushButton  const*bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)
    connect(bOk, &QPushButton::clicked, this, &DialogPreferences::Ok);

    QPushButton  const*bApply = ui->buttonBox->button(QDialogButtonBox::Apply);
    SCASSERT(bApply != nullptr)
    connect(bApply, &QPushButton::clicked, this, &DialogPreferences::Apply);

    ui->pagesWidget->insertWidget(0, m_generalPage);
    ui->pagesWidget->insertWidget(1, m_shortcutsPage);
    ui->pagesWidget->insertWidget(2, m_foldersPage);
    ui->pagesWidget->insertWidget(3, m_toolBarPage);
    ui->pagesWidget->insertWidget(4, m_patternPage);

    connect(ui->contentsWidget, &QListWidget::currentItemChanged, this, &DialogPreferences::PageChanged);
    ui->pagesWidget->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
DialogPreferences::~DialogPreferences()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPreferences::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (event->spontaneous())
    {
        return;
    }

    if (m_isInitialized)
    {
        return;
    }
    // do your init stuff here

    if (QSize const sz = VAbstractApplication::VApp()->Settings()->GetPreferenceDialogSize(); not sz.isEmpty())
    {
        resize(sz);
    }

    // **Centred on the window that opened it.** Only the size is remembered (resizeEvent), never the
    // position: a dialog reopened where it was last left can land over nothing, or on another screen,
    // and the pattern being worked on is always under the main window.
    if (QWidget *const owner = parentWidget() != nullptr ? parentWidget()->window() : nullptr)
    {
        QRect target(QPoint(), size());
        target.moveCenter(owner->frameGeometry().center());

        // Kept on the screen that holds the owner: a dialog taller than the screen would otherwise be
        // pushed off the top, and its buttons with it.
        if (QScreen const *const screen = QGuiApplication::screenAt(owner->frameGeometry().center());
            screen != nullptr)
        {
            const QRect available = screen->availableGeometry();
            target.moveLeft(qMax(available.left(), qMin(target.left(), available.right() - target.width())));
            target.moveTop(qMax(available.top(), qMin(target.top(), available.bottom() - target.height())));
        }

        move(target.topLeft());
    }

    m_isInitialized = true; // first show windows are held
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPreferences::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    // remember the size for the next time this dialog is opened, but only
    // if widget was already initialized, which rules out the resize at
    // dialog creating, which would
    if (m_isInitialized)
    {
        VAbstractApplication::VApp()->Settings()->SetPreferenceDialogSize(size());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPreferences::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // retranslate designer form (single inheritance approach)
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
    }

    // remember to call base class implementation
    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPreferences::PageChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (current == nullptr)
    {
        current = previous;
    }
    int const rowIndex = ui->contentsWidget->row(current);
    ui->pagesWidget->setCurrentIndex(rowIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPreferences::Apply()
{
    QStringList preferences;

    preferences += m_generalPage->Apply();
    preferences += m_shortcutsPage->Apply();
    preferences += m_foldersPage->Apply();
    preferences += m_toolBarPage->Apply();
    preferences += m_patternPage->Apply();

    if (not preferences.isEmpty())
    {
        const QString text =
            tr("Followed %n option(s) require restart to take effect: %1.", "", static_cast<int>(preferences.size()))
                .arg(preferences.join(QStringLiteral(", ")));
        QMessageBox::information(this, QCoreApplication::applicationName(), text);
    }

    m_patternPage->InitDefaultSeamAllowance();

    VAbstractValApplication::VApp()->ValentinaSettings()->GetOsSeparator() ? setLocale(QLocale())
                                                                           : setLocale(QLocale::c());
    emit UpdateProperties();
    setResult(QDialog::Accepted);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPreferences::Ok()
{
    Apply();
    done(QDialog::Accepted);
}
