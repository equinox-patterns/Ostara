/************************************************************************
 **
 **  @file   oexporttemplatewindow.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Modeless « Export templates » window wrapping the template editor.
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

#include "oexporttemplatewindow.h"

#include "oexporttemplateeditor.h"

#include <QCloseEvent>
#include <QEvent>
#include <QGuiApplication>
#include <QMessageBox>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSize>

OExportTemplateWindow::OExportTemplateWindow(QWidget *parent) : QMainWindow(parent)
{
    // Explicitly promote to top-level window (matches OInfoBlockWindow pattern).
    // Without this, on macOS a native QFileDialog sheet attached later can leave the
    // window deactivated behind its parent MainWindow.
    setWindowModality(Qt::NonModal);

    m_editor = new OExportTemplateEditor(this);
    setCentralWidget(m_editor);
    setWindowTitle(tr("Export templates"));

    // No geometry is stored, unlike OInfoBlockWindow: this window always opens at the size it
    // computes here — nine tenths of the screen. The user may resize it; the next launch
    // starts from this size again, not from theirs.
    const QScreen *targetScreen = parent != nullptr ? parent->screen() : nullptr;
    if (targetScreen == nullptr)
    {
        targetScreen = QGuiApplication::primaryScreen();
    }

    if (targetScreen != nullptr)
    {
        const QRect available = targetScreen->availableGeometry();
        const QSize wanted(available.width() * 9 / 10, available.height() * 9 / 10);
        resize(wanted);
        move(available.center() - QPoint(wanted.width() / 2, wanted.height() / 2));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateWindow::OpenOnTab(int tab)
{
    m_editor->OpenOnTab(tab);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateWindow::changeEvent(QEvent *event)
{
    // A child widget does not receive QEvent::LanguageChange on its own.
    if (event->type() == QEvent::LanguageChange)
    {
        setWindowTitle(tr("Export templates"));
        m_editor->RetranslateUI();
    }

    QMainWindow::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateWindow::closeEvent(QCloseEvent *event)
{
    if (m_editor == nullptr || !m_editor->IsModified())
    {
        QMainWindow::closeEvent(event);
        return;
    }

    // Three answers, and the one the keyboard reaches first is the one that keeps the work.
    // Closing is the only move here that cannot be taken back, so it is never what a stray
    // Return does.
    const auto answer =
        QMessageBox::warning(this, tr("Unsaved template"),
                             tr("This template has been changed and not saved.\n\n"
                                "Save it before closing?"),
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                             QMessageBox::Save);

    if (answer == QMessageBox::Cancel)
    {
        event->ignore();
        return;
    }

    if (answer == QMessageBox::Save)
    {
        m_editor->SaveTemplate();

        // A save that did not happen — no name, a name already taken, a folder that cannot be
        // written — has left the change unsaved, and the window must not close on it. The editor
        // says why in its own message.
        if (m_editor->IsModified())
        {
            event->ignore();
            return;
        }
    }

    QMainWindow::closeEvent(event);
}
