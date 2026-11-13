/************************************************************************
 **
 **  @file   ofullscreen.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Le plein écran que l'application possède elle-même, et qui ne lui est jamais repris.
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

#include "ofullscreen.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QScreen>
#include <QString>
#include <QVariant>
#include <QWidget>

namespace
{
/// The takeover in force, or absent when the window is not taken by this module.
constexpr auto kTakeoverProperty = "oFullScreenTakeover";

/// The frame geometry the window had before it was taken, so that Exit() can give it back whole.
constexpr auto kGeometryProperty = "oFullScreenGeometry";

/// The window state the window had before it was taken — maximized, mostly.
constexpr auto kStateProperty = "oFullScreenState";

/// Posé le temps d'un Exit() : la perte d'écran qui suit vient de la fenêtre, pas du système.
constexpr auto kGivingBackProperty = "oFullScreenGivingBack";

//---------------------------------------------------------------------------------------------------------------------
/// La géométrie de repli quand celle d'avant ne peut pas être rendue : la zone utile de l'écran
/// courant, avec un dixième d'air de chaque côté — la fenêtre reste utilisable et déplaçable.
void FallbackGeometry(QWidget *window)
{
    QScreen *const screen = window->screen();

    if (screen == nullptr)
    {
        return;
    }

    const QRect area = screen->availableGeometry();
    const int marginX = area.width() / 10;
    const int marginY = area.height() / 10;
    window->setGeometry(area.adjusted(marginX, marginY, -marginX, -marginY));
}

//---------------------------------------------------------------------------------------------------------------------
auto RememberedTakeover(const QWidget *window, bool *found) -> OFullScreen::Takeover
{
    bool numeric = false;
    const int raw = window->property(kTakeoverProperty).toInt(&numeric);

    // Une propriété présente mais illisible — un type inattendu, une valeur hors de l'énumération —
    // ne doit pas se lire comme « pris en natif » par un cast de zéro. Elle ne dit rien du tout.
    *found = numeric
             && (raw == static_cast<int>(OFullScreen::Takeover::Native)
                 || raw == static_cast<int>(OFullScreen::Takeover::Frameless));

    return static_cast<OFullScreen::Takeover>(raw);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreen::DefaultTakeover() -> Takeover
{
    // A Wayland client does not place its own windows: the compositor decides where they are and how
    // big they are, so a geometry we set ourselves would simply be ignored. Only X11 lets a window
    // take the screen on its own, which is exactly what we need there — and only there.
    if (QGuiApplication::platformName().startsWith(QStringLiteral("xcb")))
    {
        return Takeover::Frameless;
    }

    return Takeover::Native;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreen::IsActive(const QWidget *window) -> bool
{
    if (window == nullptr)
    {
        return false;
    }

    bool found = false;
    const Takeover takeover = RememberedTakeover(window, &found);

    if (!found)
    {
        return false;
    }

    // Le fait, pas le souvenir. Le système peut reprendre son plein écran à la fenêtre — Échap sous
    // macOS, ou le bouton vert — sans que l'application en soit prévenue autrement que par l'état de
    // la fenêtre. Une case de menu qui resterait cochée après ça mentirait, et l'entrée suivante ne
    // ferait plus rien puisqu'elle se croirait déjà prise.
    if (takeover == Takeover::Native)
    {
        return window->isFullScreen();
    }

    return window->windowFlags().testFlag(Qt::FramelessWindowHint);
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreen::Enter(QWidget *window, Takeover takeover) -> bool
{
    if (window == nullptr)
    {
        return false;
    }

    if (IsActive(window))
    {
        return true;
    }

    QScreen *const screen = window->screen();

    if (screen == nullptr)
    {
        // Aucun écran connu : ni la prise sans cadre, ni `saveGeometry()` — qui déréférence l'écran —
        // ne peuvent aboutir. Mieux vaut ne rien prendre que laisser une fenêtre sans cadre, que
        // personne ne déplace et que le hasard dimensionne.
        return false;
    }

    // ⚠️ **Une fenêtre déjà en plein écran hors du module** — le bouton vert de macOS, par exemple.
    // `saveGeometry()` enregistrerait l'état plein écran lui-même, et la sortie le rétablirait : le
    // module ne pourrait plus jamais rendre la fenêtre, chaque cycle réenregistrant et restaurant le
    // plein écran. En natif le système garde déjà la géométrie d'avant, il n'y a rien à mémoriser ;
    // en sans-cadre il faut repartir d'un état normal pour en avoir une.
    const bool alreadyFullScreen = window->isFullScreen();

    if (alreadyFullScreen && takeover == Takeover::Frameless)
    {
        window->showNormal();
    }

    if (!alreadyFullScreen || takeover == Takeover::Frameless)
    {
        window->setProperty(kGeometryProperty, window->saveGeometry());
        window->setProperty(kStateProperty, static_cast<int>(window->windowState()));
    }

    window->setProperty(kTakeoverProperty, static_cast<int>(takeover));

    if (takeover == Takeover::Native)
    {
        window->showFullScreen();
        return true;
    }

    // setWindowFlag() takes the window off screen, and Qt only lays it out again when it is shown:
    // the flag, then the geometry, then show — in that order, or the window comes back at its old size.
    window->setWindowFlag(Qt::FramelessWindowHint, true);
    window->setGeometry(screen->geometry());
    window->show();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreen::Enter(QWidget *window) -> bool
{
    return Enter(window, DefaultTakeover());
}

//---------------------------------------------------------------------------------------------------------------------
auto OFullScreen::IsGivingBack(const QWidget *window) -> bool
{
    return window != nullptr && window->property(kGivingBackProperty).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void OFullScreen::Exit(QWidget *window)
{
    if (window == nullptr)
    {
        return;
    }

    bool found = false;
    const Takeover takeover = RememberedTakeover(window, &found);
    if (!found)
    {
        return;
    }

    const bool hadGeometry = window->property(kGeometryProperty).isValid();
    const QByteArray geometry = window->property(kGeometryProperty).toByteArray();
    const auto previousState = static_cast<Qt::WindowStates>(window->property(kStateProperty).toInt());

    // Posé avant le premier changement d'état : `setWindowState()` émet `WindowStateChange` de façon
    // synchrone, et le filet doit savoir que la perte qui arrive est la nôtre, pas celle du système.
    window->setProperty(kGivingBackProperty, true);

    window->setProperty(kTakeoverProperty, QVariant());
    window->setProperty(kGeometryProperty, QVariant());
    window->setProperty(kStateProperty, QVariant());

    const bool frameless = takeover == Takeover::Frameless;

    if (frameless)
    {
        // Le cadre se repose **avant** la géométrie : c'est lui qui décide de la place de la zone
        // cliente, et le remettre après ferait perdre la bordure et la barre de titre qu'on vient de
        // rendre. setWindowFlag() retire la fenêtre de l'écran, d'où le show() à la fin.
        window->setWindowFlag(Qt::FramelessWindowHint, false);
    }
    else
    {
        window->showNormal();
    }

    if (hadGeometry)
    {
        // `restoreGeometry()` rend faux — et sans rien dire — quand la géométrie vient d'un autre
        // écran : un écart de largeur de plus de 25 % suffit, et l'écran d'avant peut avoir disparu.
        // Ignorer ce refus laisserait la fenêtre à la taille de l'écran, avec son cadre retrouvé.
        const bool restored = window->restoreGeometry(geometry);

        if (!restored && !previousState.testFlag(Qt::WindowMaximized))
        {
            FallbackGeometry(window);
        }
    }

    if (previousState.testFlag(Qt::WindowMaximized) && !window->isMaximized())
    {
        window->showMaximized();
    }

    if (frameless)
    {
        window->show();
    }

    window->setProperty(kGivingBackProperty, QVariant());
}
