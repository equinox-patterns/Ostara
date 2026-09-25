/************************************************************************
 **
 **  @file   mainwindow_dialogs.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26/08/2026
 **
 **  @brief  Dialog-opening UI slots extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/ODockManager.h"
#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/dialogfinalmeasurements.h"
#include "dialogs/dialogincrements.h"
#include "dialogs/dialogpreferences.h"
#include "vabstractapplication.h"
#include "widgets/InfoBlock/OInfoBlockWindow.h"
#include "oexporttemplatewindow.h"
#include "../vmisc/dialogs/dialogselectlanguage.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QComboBox>
#include <QCursor>
#include <QGuiApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QPointer>
#include <QScopedPointer>

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AskDefaultSettings()
{
    if (VApplication::IsGUIMode())
    {
        VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
        if (not settings->IsLocaleSelected())
        {
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            DialogSelectLanguage dialog(this);
            QGuiApplication::restoreOverrideCursor();
            if (dialog.exec() == QDialog::Accepted)
            {
                QString const locale = dialog.Locale();
                settings->SetLocale(locale);
                VAbstractApplication::VApp()->LoadTranslation(locale);
            }
        }

        // VGAnalyticsLib removed — no tracking, no privacy modal
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionTable_triggered()
{
    // Because of bug on Mac with Qt 5.11 closing this dialog causes a crash. Instead of closing we will keep
    // dialog in memory.
    if (m_dialogTable.isNull())
    {
        m_dialogTable = new DialogIncrements(pattern, doc, this);
        connect(m_dialogTable.data(), &DialogIncrements::UpdateProperties, m_dockManager->toolOptions(),
                &VToolOptionsPropertyBrowser::RefreshOptions);
        m_dialogTable->show();
    }
    else
    {
        m_dialogTable->FullUpdateFromFile();
        m_dialogTable->RestoreAfterClose(); // Redo some moves after close
        m_dialogTable->isVisible() ? m_dialogTable->activateWindow() : m_dialogTable->show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionFinalMeasurements_triggered()
{
    if (m_dialogFMeasurements.isNull())
    {
        m_dialogFMeasurements = new DialogFinalMeasurements(doc, this);
        m_dialogFMeasurements->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_dialogFMeasurements.data(), &DialogFinalMeasurements::finished, this,
                [this](int result)
                {
                    if (result == QDialog::Accepted)
                    {
                        doc->SetFinalMeasurements(m_dialogFMeasurements->FinalMeasurements());
                        emit doc->UpdatePatternLabel();
                    }
                    m_dialogFMeasurements->close();
                });
        m_dialogFMeasurements->show();
    }
    else
    {
        m_dialogFMeasurements->activateWindow();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionInfoBlock_triggered(int page)
{
    if (m_infoBlockWindow.isNull())
    {
        auto *window = new OInfoBlockWindow(this);
        m_infoBlockWindow = window;
        window->setAttribute(Qt::WA_DeleteOnClose);
        // When a template is saved, refresh the canvas blocks (the tools read the
        // template file from disk; CheckLayout re-runs UpdateInfoBlocks on every piece).
        connect(window, &OInfoBlockWindow::TemplateSaved, this, []() {
            if (VAbstractPattern *doc = VAbstractValApplication::VApp()->getCurrentDocument())
            {
                doc->CheckLayout();
            }
        });
        window->OpenOnPage(page);
        window->show();
    }
    else
    {
        m_infoBlockWindow->OpenOnPage(page);
        m_infoBlockWindow->activateWindow();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionExportTemplates_triggered(int tab)
{
    if (m_exportTemplateWindow.isNull())
    {
        auto *window = new OExportTemplateWindow(this);
        m_exportTemplateWindow = window;
        window->setAttribute(Qt::WA_DeleteOnClose);

        // A template that was just saved is the one the plan on screen is drawn with. The canvas
        // is redrawn on it — and only redrawn: the connection is to a template being written,
        // never to the run that produced the plan.
        connect(window, &OExportTemplateWindow::TemplateSaved, this, &MainWindow::RefreshExportDrawing);
        window->OpenOnTab(tab);
        window->show();
    }
    else
    {
        m_exportTemplateWindow->OpenOnTab(tab);
        m_exportTemplateWindow->activateWindow();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::PatternPieceName(QString &name) -> bool
{
    QScopedPointer<QInputDialog> const dlg(new QInputDialog(this));
    dlg->setInputMode(QInputDialog::TextInput);
    dlg->setLabelText(tr("Pattern piece:"));
    dlg->setTextEchoMode(QLineEdit::Normal);
    dlg->setWindowTitle(tr("Enter a new label for the pattern piece."));
    dlg->resize(300, 100);
    dlg->setTextValue(name);
    QString nameDraw;
    while (true)
    {
        const bool bOk = dlg->exec();
        nameDraw = dlg->textValue();
        if (not bOk)
        {
            return false;
        }

        if (nameDraw.isEmpty())
        {
            continue;
        }

        if (m_comboBoxDraws->findText(nameDraw) == -1)
        {
            name = nameDraw;
            break; // unique name
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::Preferences()
{
    // Calling constructor of the dialog take some time. Because of this user have time to call the dialog twice.
    static QPointer<DialogPreferences> guard; // Prevent any second run
    if (guard.isNull())
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        auto *preferences = new DialogPreferences(this);
        // QScopedPointer needs to be sure any exception will never block guard
        QScopedPointer<DialogPreferences> const dlg(preferences);
        guard = preferences;
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, &MainWindow::WindowsLocale); // Must be first
        connect(dlg.data(), &DialogPreferences::UpdateProperties, m_dockManager->toolOptions(),
                &VToolOptionsPropertyBrowser::RefreshOptions);
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, &MainWindow::ToolBarStyles);
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, &MainWindow::ToolboxIconSize);
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, [this]() { emit doc->FullUpdateFromFile(); });
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, [this]() { emit doc->CheckLayout(); });
        connect(dlg.data(), &DialogPreferences::UpdateProperties, ui->view,
                &VMainGraphicsView::ResetScrollingAnimation);
        connect(dlg.data(), &DialogPreferences::RequestFullParse, this, &MainWindow::FullParseFile,
                Qt::QueuedConnection); // Queued: the reparse tears the tool tree down in the middle of Apply().
        QGuiApplication::restoreOverrideCursor();

        if (guard->exec() == QDialog::Accepted)
        {
            InitAutoSave();

            // ⚠️ **La retraduction est faite ICI, de façon synchrone.** Qt poste un `LanguageChange`
            // quand un traducteur est installé, et la doc dit qu'il parcourt tout l'arbre des widgets
            // — mais il n'arrive pas jusqu'à cette fenêtre dans cette application : constaté au
            // journal, la bascule de langue ne retraduisait rien du tout. La page Préférences a déjà
            // installé le nouveau traducteur ; on reprend donc la fenêtre nous-mêmes, au seul moment
            // où l'on sait que la langue vient de changer. Idempotent : sans changement de langue,
            // ce n'est qu'un passage à vide.
            RetranslateAll();
        }
    }
}
