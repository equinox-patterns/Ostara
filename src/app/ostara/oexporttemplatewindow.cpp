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
#include <QShowEvent>
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
    //
    // The size only. Where it goes is settled in showEvent(), because a constructor is too early
    // to know it — see the comment there.
    if (const QScreen *targetScreen = TargetScreen())
    {
        const QRect available = targetScreen->availableGeometry();
        resize(available.width() * 9 / 10, available.height() * 9 / 10);
    }
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateWindow::TargetScreen() const -> const QScreen *
{
    const QWidget *origin = parentWidget();
    const QScreen *screen = origin != nullptr ? origin->screen() : nullptr;
    return screen != nullptr ? screen : QGuiApplication::primaryScreen();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    if (m_centred)
    {
        return;
    }
    m_centred = true;

    const QScreen *screen = TargetScreen();
    if (screen == nullptr)
    {
        return;
    }

    // Centred here rather than in the constructor, and on the frame rather than on the size that
    // was asked for. Two separate reasons, and each of them alone is enough to look off-centre:
    //
    //  - `resize()` in a constructor is a REQUEST. The editor's own minimum size can overrule it,
    //    and the window then grows from a position computed for a size it never got — downwards
    //    and to the right, since the top-left is what was placed.
    //  - `move()` places the FRAME, title bar included, while `resize()` sizes the client area.
    //    Centring the client size therefore leaves the whole window half a title bar too low.
    //
    // frameGeometry() has both settled, and this is the idiom the rest of the application uses
    // (dialognewpattern.cpp:53).
    const QRect available = screen->availableGeometry();
    QRect frame = frameGeometry();
    frame.moveCenter(available.center());

    // A window larger than the screen cannot be centred on it. Pinned to the top and the left
    // rather than centred, so that what it loses is the far edge instead of both.
    if (frame.height() > available.height())
    {
        frame.moveTop(available.top());
    }
    if (frame.width() > available.width())
    {
        frame.moveLeft(available.left());
    }

    move(frame.topLeft());
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
