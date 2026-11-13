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
#include <QShortcut>
#include <QMessageBox>

OInfoBlockWindow::OInfoBlockWindow(QWidget *parent) : QMainWindow(parent)
{
    // Explicitly promote to top-level window (matches MeasurementWindow pattern).
    // Without this, on macOS the native QFileDialog sheet attached during Save
    // can leave the window deactivated behind its parent MainWindow.
    setWindowModality(Qt::NonModal);

    // Le type « dialogue » dit au système ce que cette fenêtre est : un dialogue de la fenêtre
    // principale, comme toutes les autres boîtes de l'application — et non une fenêtre ordinaire, que
    // le gestionnaire de fenêtres peut laisser derrière un plein écran (X11, Wayland) ou afficher
    // plein écran (macOS, où une fenêtre ouverte depuis une fenêtre en plein écran hérite de son
    // espace).
    setWindowFlags(windowFlags() | Qt::Dialog);

    // Un dialogue, donc **modal** : la fenêtre principale est bloquée tant que l'éditeur est ouvert,
    // comme les options d'une ligne. Sophie, 13-11-2026.
    setWindowModality(Qt::ApplicationModal);

    // Une fenêtre bloquante sans issue au clavier serait un piège : la barre de titre en donne une,
    // la touche Échap doit en donner une seconde, comme toutes les boîtes de l'application.
    auto *const escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escape, &QShortcut::activated, this, &QWidget::close);

    m_editor = new OInfoBlockEditor(this);
    setCentralWidget(m_editor);
    setWindowTitle(tr("Information block[*]"));

    connect(m_editor, &OInfoBlockEditor::DocumentModified, this, [this]() {
        setWindowModified(m_editor->IsModified());
    });
    connect(m_editor, &OInfoBlockEditor::TemplateSaved, this, &OInfoBlockWindow::TemplateSaved);
    connect(m_editor, &OInfoBlockEditor::FoldLineTextChanged, this, &OInfoBlockWindow::FoldLineTextChanged);
    connect(m_editor, &OInfoBlockEditor::StatusMessage, this, [this](const QString &message) {
        statusBar()->showMessage(message, 3000);
    });

    const QByteArray savedGeometry = VAbstractValApplication::VApp()
                                          ->ValentinaSettings()
                                          ->value(QStringLiteral("infoBlockWindowGeometry"))
                                          .toByteArray();
    bool restored = false;

    if (!savedGeometry.isEmpty())
    {
        restored = restoreGeometry(savedGeometry);

        // ⚠️ `saveGeometry()` enregistre aussi l'ÉTAT de la fenêtre, pas seulement sa taille et sa
        // place. Une fenêtre mise en plein écran ou agrandie un jour se rouvrirait ainsi — et un
        // dialogue en plein écran n'est plus un dialogue. L'état d'hier ne décide donc pas de la
        // taille d'aujourd'hui : on repart de la taille par défaut.
        if (restored && (isFullScreen() || isMaximized()))
        {
            setWindowState(Qt::WindowNoState);
            restored = false;
        }
    }

    if (!restored)
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
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
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
