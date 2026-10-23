/************************************************************************
 **
 **  @file   ogradedrangerenames.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The renames of the graded size ranges of this session, so that work holding
 **          the name a range had can still find it.
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

#ifndef OGRADEDRANGERENAMES_H
#define OGRADEDRANGERENAMES_H

#include <QString>

/**
 * @brief The renames of the graded size ranges of this session.
 *
 * A range is addressed by its NAME — it is the key of `SizeRanges()`, and every place that
 * resolves one resolves it that way. Renaming a range is therefore a move in the key space,
 * and anything that took the old name before the move is left holding a name that no longer
 * resolves.
 *
 * One such thing cannot be repaired where it lives: `SaveGradedRangeContent` keeps two copies
 * of the range, taken when the command was pushed, and writes them back under their own name
 * on undo/redo. Qt gives no access to the commands on the stack, so the names they hold
 * cannot be rewritten — and an undo that finds nothing returns in silence, which is a user
 * pressing Ctrl+Z and watching nothing happen.
 *
 * Hence this journal: the rename is written down once, here, and the command resolves its
 * name through it. Session-scoped, like the other name a range is followed by
 * (`MainWindow::m_activeGradedRangeName`), and forgotten when a pattern is opened or closed.
 */
namespace OGradedRangeRenames
{
/** @brief Write down that the range named @p oldName is now named @p newName. */
void Remember(const QString &oldName, const QString &newName);

/**
 * @brief The name @p name leads to, following the renames written down so far.
 *
 * A name nothing was written down for resolves to itself — that is the common case, and it
 * is what makes this safe to call on every write. The walk carries the names it has already
 * been through: renaming A to B and then B back to A is allowed, and a naive walk would
 * follow that loop for ever.
 */
auto Resolve(const QString &name) -> QString;

/**
 * @brief Take back the rename that started at @p oldName — the undo of a rename.
 *
 * The reverse rename is written down separately by @ref Remember. Keeping BOTH entries would
 * make a loop in the journal (A→B and B→A), and the walk stops at a loop: a command holding B
 * would still be sent to B, which no longer exists. One entry per move, then.
 */
void Forget(const QString &oldName);

/** @brief Forget everything — a pattern was opened or closed, and its ranges went with it. */
void Forget();

}  // namespace OGradedRangeRenames

#endif  // OGRADEDRANGERENAMES_H
