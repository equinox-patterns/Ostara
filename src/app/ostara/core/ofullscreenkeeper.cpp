/************************************************************************
 **
 **  @file   ofullscreenkeeper.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le filet qui rend le plein écran du système quand une dialogue l'a fait perdre.
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

#include "ofullscreenkeeper.h"

#include "ofullscreen.h"

#include "../vmisc/vabstractvalapplication.h"

#include <QApplication>
#include <QDialog>
#include <QEvent>
#include <QWidget>

//---------------------------------------------------------------------------------------------------------------------
void OFullScreenKeeper::Install()
{
    static OFullScreenKeeper *keeper = nullptr;

    if (keeper != nullptr)
    {
        return;
    }

    keeper = new OFullScreenKeeper(qApp);
    qApp->installEventFilter(keeper);
}

//---------------------------------------------------------------------------------------------------------------------
OFullScreenKeeper::OFullScreenKeeper(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreenKeeper::MainWindow() -> QWidget *
{
    VAbstractValApplication *const app = VAbstractValApplication::VApp();

    if (app == nullptr)
    {
        return nullptr;
    }

    return app->getMainWindow();
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreenKeeper::TopLevelDialog(QObject *watched) -> QWidget *
{
    auto *const widget = qobject_cast<QWidget *>(watched);

    if (widget == nullptr || !widget->isWindow())
    {
        return nullptr;
    }

    if (qobject_cast<QDialog *>(widget) != nullptr || widget->windowType() == Qt::Dialog)
    {
        return widget;
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreenKeeper::AnotherDialogVisible(const QWidget *closing) -> bool
{
    const QWidgetList windows = QApplication::topLevelWidgets();

    for (QWidget *const window : windows)
    {
        if (window == closing || !window->isVisible())
        {
            continue;
        }

        if (qobject_cast<QDialog *>(window) != nullptr || window->windowType() == Qt::Dialog)
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
bool OFullScreenKeeper::eventFilter(QObject *watched, QEvent *event)
{
    // Le filtre est posé sur l'application entière : il voit chaque peinture et chaque mouvement de
    // souris. L'élagage passe donc en tête, avant de résoudre la fenêtre principale.
    const QEvent::Type type = event->type();

    if (type != QEvent::Show && type != QEvent::Hide && type != QEvent::WindowStateChange)
    {
        return QObject::eventFilter(watched, event);
    }

    QWidget *const window = MainWindow();

    if (window == nullptr)
    {
        // L'intention n'a plus d'objet : la fenêtre à rendre au plein écran n'existe pas, ou plus.
        m_screenToGiveBack = false;
        return QObject::eventFilter(watched, event);
    }

    if (m_restoring)
    {
        return QObject::eventFilter(watched, event);
    }

    if (watched == window && type == QEvent::WindowStateChange)
    {
        // La sortie demandée par l'utilisatrice ne se défait pas : c'est elle qui a quitté le plein
        // écran, par le menu, pendant qu'une dialogue était ouverte. Rien à rendre.
        if (OFullScreen::IsGivingBack(window))
        {
            m_screenToGiveBack = false;
            return QObject::eventFilter(watched, event);
        }

        // The compositor that took the full screen away is not obliged to announce it before the
        // dialog is mapped: under X11 and under Wayland, the loss arrives while the dialog is up.
        if (!window->isFullScreen() && AnotherDialogVisible(nullptr))
        {
            m_screenToGiveBack = true;
        }

        return QObject::eventFilter(watched, event);
    }

    QWidget *const dialog = TopLevelDialog(watched);

    if (dialog == nullptr)
    {
        return QObject::eventFilter(watched, event);
    }

    if (type == QEvent::Show)
    {
        if (window->isFullScreen())
        {
            m_screenToGiveBack = true;
        }

        return QObject::eventFilter(watched, event);
    }

    // ⚠️ **Sur Hide seulement, jamais sur Close.** Un filtre d'événements passe avant le gestionnaire
    // de l'objet : au moment du Close, la dialogue n'a pas encore répondu, et celle qui refuse de se
    // fermer (`event->ignore()`, le cas « modifications non enregistrées / Annuler ») verrait l'écran
    // rendu alors qu'elle est toujours ouverte. Hide est le seul événement qui dit que la dialogue a
    // réellement quitté l'écran.
    //
    // A dialog that gives way to the next one must not hand the screen back in between: the next one
    // would take it away again, and the window would blink once per dialog.
    if (AnotherDialogVisible(dialog))
    {
        return QObject::eventFilter(watched, event);
    }

    // Consommé à la première occasion, qu'on rende l'écran ou non : ce qui a été perdu l'a été.
    const bool giveBack = m_screenToGiveBack && !window->isFullScreen() && !OFullScreen::IsActive(window);
    m_screenToGiveBack = false;

    if (!giveBack)
    {
        return QObject::eventFilter(watched, event);
    }

    m_restoring = true;
    const bool taken = OFullScreen::Enter(window);
    m_restoring = false;

    if (!taken)
    {
        // L'écran n'a pas pu être repris — pas d'écran connu, par exemple. L'intention reste posée
        // pour la prochaine dialogue plutôt que d'être perdue en silence.
        m_screenToGiveBack = true;
    }

    return QObject::eventFilter(watched, event);
}
