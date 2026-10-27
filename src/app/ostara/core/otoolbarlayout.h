/************************************************************************
 **
 **  @file   otoolbarlayout.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef OTOOLBARLAYOUT_H
#define OTOOLBARLAYOUT_H

#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief How the user arranged the Draw-mode palette: which groups, in which order, holding
 * which tools.
 *
 * **The tools that are in no group are the ones that are not shown.** There is no second list:
 * `hidden` is the complement of the groups, taken from the catalogue. Storing both would let the
 * two disagree — a tool in a group *and* in the hidden list, or a tool in neither and therefore
 * lost — and the palette would have to pick a winner silently.
 *
 * A shipped group keeps its `key`, so its name follows the interface language. Renaming a group
 * clears the key: from then on the name is the user's, in whatever language it was typed.
 *
 * Nothing here ever reaches the .ostara: this is a preference of the software, not of the
 * pattern.
 */
namespace OToolBarLayout
{
struct Group
{
    QString key;  // the shipped group it still is; empty once renamed
    QString name; // as stored — only read when key is empty
    QStringList tools;

    /// The name to write on the palette: the shipped one when the group still is a shipped one.
    auto DisplayName() const -> QString;
};

struct Layout
{
    QVector<Group> groups;

    /// Every tool that lives in a group, in order. The rest of the catalogue is not shown.
    auto Assigned() const -> QStringList;

    /// Index of the group holding @p toolId, -1 when it is not shown.
    auto GroupIndex(const QString &toolId) const -> int;

    /// Whether this is exactly the organisation Ostara ships with.
    auto IsDefault() const -> bool;
};

/// The organisation Ostara ships with: the seven groups of the Draw mode.
auto Default() -> Layout;

/// The stored organisation. Anything unreadable — an old format, a hand-edited file, a tool that
/// no longer exists — falls back to the default rather than to an empty palette.
auto Load() -> Layout;

/// Store the organisation as a compact JSON string.
void Save(const Layout &layout);

/// Forget it: the default organisation comes back.
void Reset();
} // namespace OToolBarLayout

#endif // OTOOLBARLAYOUT_H
