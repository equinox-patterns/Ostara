/************************************************************************
 **
 **  @file   OInfoBlockWindow.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Modeless « Information block » window wrapping the template editor.
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

#include "OInfoBlockWindow.h"

#include "OInfoBlockEditor.h"

#include "vabstractvalapplication.h"
#include "vvalentinasettings.h"

#include <QByteArray>
#include <QCloseEvent>
#include <QEvent>
#include <QMessageBox>

OInfoBlockWindow::OInfoBlockWindow(QWidget *parent) : QMainWindow(parent)
{
    // Explicitly promote to top-level window (matches MeasurementWindow pattern).
    // Without this, on macOS the native QFileDialog sheet attached during Save
    // can leave the window deactivated behind its parent MainWindow.
    setWindowModality(Qt::NonModal);

    m_editor = new OInfoBlockEditor(this);
    setCentralWidget(m_editor);
    setWindowTitle(tr("Information block[*]"));

    connect(m_editor, &OInfoBlockEditor::DocumentModified, this, [this]() {
        setWindowModified(m_editor->IsModified());
    });
    connect(m_editor, &OInfoBlockEditor::TemplateSaved, this, &OInfoBlockWindow::TemplateSaved);
    connect(m_editor, &OInfoBlockEditor::StatusMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message, 3000);
    });

    const QByteArray savedGeometry = VAbstractValApplication::VApp()
                                          ->ValentinaSettings()
                                          ->value(QStringLiteral("infoBlockWindowGeometry"))
                                          .toByteArray();
    if (!savedGeometry.isEmpty())
    {
        restoreGeometry(savedGeometry);
    }
    else
    {
        resize(920, 620);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockWindow::OpenOnPage(int page)
{
    m_editor->OpenOnPage(page);
}

void OInfoBlockWindow::closeEvent(QCloseEvent *event)
{
    if (m_editor->IsModified())
    {
        const auto answer = QMessageBox::warning(
            this, tr("Information block"), tr("The template has unsaved changes. Do you want to save them?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (answer == QMessageBox::Save)
        {
            m_editor->OnSave();
            if (m_editor->IsModified())
            {
                event->ignore();
                return;
            }
        }
        else if (answer == QMessageBox::Cancel)
        {
            event->ignore();
            return;
        }
    }

    VAbstractValApplication::VApp()->ValentinaSettings()->setValue(QStringLiteral("infoBlockWindowGeometry"),
                                                                   saveGeometry());
    QMainWindow::closeEvent(event);
}

void OInfoBlockWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // The editor is a child widget — it does not receive QEvent::LanguageChange
        // on its own, so forward the retranslation to it.
        setWindowTitle(tr("Information block[*]"));
        m_editor->RetranslateUI();
    }

    QMainWindow::changeEvent(event);
}
