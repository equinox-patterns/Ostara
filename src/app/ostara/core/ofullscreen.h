/************************************************************************
 **
 **  @file   ofullscreen.h
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

#ifndef OFULLSCREEN_H
#define OFULLSCREEN_H

#include <QtGlobal>

class QWidget;

/**
 * @brief Le plein écran qui appartient à l'application, et non au gestionnaire de fenêtres.
 *
 * Sous macOS et Windows, demander le plein écran à Qt suffit : le système le pose et le tient tant
 * que l'utilisatrice ne le quitte pas. Il peut néanmoins le rendre de lui-même — Échap, bouton vert —
 * et `IsActive()` constate alors la reprise, au lieu de se souvenir d'une prise qui n'existe plus.
 * Sous X11 c'est le gestionnaire de fenêtres qui le possède — il peut le retirer quand une
 * dialogue s'affiche ou se déplace, et rien dans l'application ne peut l'en empêcher, parce que cet
 * état n'est pas le sien.
 *
 * D'où deux prises d'écran, et deux seulement :
 *
 * - **Native** : `showFullScreen()`. Le gestionnaire de fenêtres pose l'état et le tient. C'est le
 *   bon choix partout où il tient parole — macOS, Windows, et Wayland, où le protocole interdit à
 *   un client de se placer lui-même à l'écran.
 * - **Frameless** : la fenêtre perd son cadre et prend la géométrie de l'écran. Aucun état n'est
 *   confié à personne, donc rien ne peut être repris : c'est la seule prise d'écran qui survit à
 *   une dialogue sous X11.
 *
 * Les deux se rendent de la même façon, et la fenêtre retrouve la géométrie qu'elle avait avant.
 * Ce que l'utilisatrice voit est identique dans les deux cas : un écran plein, sans barre de titre.
 *
 * ⚠️ Appelé depuis le fil de l'interface uniquement : les trois fonctions touchent à des propriétés de
 * widget, à des drapeaux de fenêtre et à l'état d'écran, tous réservés au fil qui possède la fenêtre.
 */
namespace OFullScreen
{
/** @brief Comment l'écran est pris. */
enum class Takeover : quint8
{
    /// Demandé au gestionnaire de fenêtres. Il le tient — ou pas.
    Native,
    /// Pris par la fenêtre elle-même : plus de cadre, la géométrie de l'écran. Personne d'autre ne
    /// peut le reprendre.
    Frameless
};

/// @brief La prise d'écran que cette plateforme sait tenir : Frameless sous X11, Native partout
/// ailleurs.
///
/// @details Wayland est explicitement renvoyé vers Native : un client Wayland ne place pas ses
/// propres fenêtres, la géométrie qu'on lui donnerait serait ignorée par le compositeur.
auto DefaultTakeover() -> Takeover;

/// @brief Vrai si @a window occupe l'écran par l'intermédiaire de ce module.
///
/// @details La réponse est le fait et non le souvenir : si le système a repris son plein écran à la
/// fenêtre — Échap sous macOS, ou le bouton vert — la fonction répond faux, et l'entrée du menu se
/// décoche au lieu de mentir.
auto IsActive(const QWidget *window) -> bool;

/// @brief Prend l'écran pour @a window, en retenant la géométrie à rendre.
/// @returns faux si la fenêtre ne peut pas être prise (nulle, ou sans écran connu).
auto Enter(QWidget *window, Takeover takeover) -> bool;

/// @brief Raccourci pour Enter(window, DefaultTakeover()).
auto Enter(QWidget *window) -> bool;

/// @brief Vrai quand l'application elle-même est en train de rendre l'écran.
///
/// @details Vrai pendant toute la sortie d'`Exit()` : c'est la fenêtre qui a déclenché le changement
/// d'état, pas le système. Le filet s'en sert pour ne pas confondre « l'utilisatrice a quitté le plein
/// écran par le menu » avec « le compositeur me l'a pris » — la première ne doit jamais être défaite à
/// la fermeture d'une dialogue.
auto IsGivingBack(const QWidget *window) -> bool;

/// @brief Rend à @a window la géométrie qu'elle avait avant Enter(), et rend l'écran.
///
/// @details Sans effet si la fenêtre n'est pas prise par ce module : la fonction peut donc être
/// appelée sans vérifier d'abord.
void Exit(QWidget *window);
} // namespace OFullScreen

#endif // OFULLSCREEN_H
