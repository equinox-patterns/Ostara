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
/// **Pourquoi l'échange est explicite.** Une ligne de menu survolée prend un fond accent — sombre
/// dans les deux thèmes (Icons.md §2, « règle ligne foncée ») : l'icône du thème, dessinée en
/// #2b2b2b, s'y éteint. Un `QIcon` sait porter une variante par mode (`Active`), mais les styles ne
/// la peignent pas pour une entrée de menu — d'où l'échange sur `QMenu::hovered`, motif déjà en
/// place dans les menus des nœuds (`vnodepoint.cpp`), des pièces et du dock Details. Il est ici une
/// fois, pour tous les menus.
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
