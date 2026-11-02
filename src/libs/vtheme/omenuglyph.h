/************************************************************************
 **
 **  @file   omenuglyph.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Les glyphes d'un menu : l'icône du thème au repos, sa variante
 **          claire au survol.
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

#ifndef OMENUGLYPH_H
#define OMENUGLYPH_H

#include <QString>

class QAction;
class QMenu;
class QObject;

/// Les glyphes d'un menu, et le survol.
///
/// **Le problème.** Une ligne de menu survolée prend un fond accent — sombre dans les deux thèmes
/// (Icons.md §2, « règle ligne foncée ») : l'icône du thème, dessinée en #2b2b2b, s'y éteint. Il faut
/// donc, sous le curseur, la variante claire du jeu sombre.
///
/// **Deux chemins le donnent, et les deux marchent.** Fusion demande `QIcon::Active` pour la ligne
/// survolée (`qfusionstyle.cpp`, `act = state & State_Selected` puis `if (act && !dis) mode =
/// QIcon::Active` — vérifié dans Qt 6.11.1), si bien qu'un `QIcon` portant `Normal` + `Active` suffit :
/// c'est ce que fait `MakeCellIcon` pour le dock Layers, en trois lignes. L'autre chemin — celui d'ici —
/// est de remplacer l'icône entière sur `QMenu::hovered`, et il ne dépend pas du style.
///
/// **Pourquoi celui-ci.** Ce fichier est né d'un motif déjà recopié **quatre fois** — menus des nœuds
/// (`vnodepoint.cpp`), des pièces (`vtoolseamallowance.cpp`), du cadre de groupe
/// (`opiecegroupholderitem.cpp`) et du dock Details (`vwidgetdetails.cpp`) : le même échange explicite,
/// aux mêmes propriétés, à chaque endroit. Il est ici **une fois**, pour tous les menus, et un menu
/// nouveau le prend au lieu d'en écrire une cinquième copie. Une entrée qui n'a pas besoin de survivre
/// à un changement de style peut, elle, se contenter du `QIcon::Active`.
namespace OMenuGlyph
{
/// Pose @p baseName comme icône de repos de @p action, et sa variante `_dark` pour le survol.
///
/// Le nom est un nom de base de `share/themes/icons/` : la variante du thème vient de
/// `VTheme::ThemedIcon()`, celle du survol est toujours celle du jeu sombre.
void SetGlyph(QAction *action, const QString &baseName);

/// Échange l'icône survolée dans @p menu. Une seule fois par menu.
void InstallHover(QMenu *menu, QObject *context);

/// Donne nos glyphes aux entrées d'un menu que Qt a construit lui-même — les menus contextuels des
/// champs texte (`QLineEdit`, `QPlainTextEdit`, `QTextEdit`, `QAbstractSpinBox`).
///
/// Ces menus sont créés par Qt à l'intérieur de ses propres gestionnaires, sans qu'on puisse les
/// recevoir : mais Qt **nomme** ses actions (`edit-cut`, `edit-paste`, `select-all`…, voir
/// `qlineedit.cpp` et `qwidgettextcontrol.cpp`), et c'est ce nom qui les reconnaît ici — pas leur
/// texte, qui suit la langue. Le menu paraît, on l'habille, et rien de son comportement ne change.
void StyleStandardMenu(QMenu *menu);
} // namespace OMenuGlyph

#endif // OMENUGLYPH_H
