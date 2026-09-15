/************************************************************************
 **
 **  @file   oexportnamebuilder.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The name a generation is filed under, built from what the run
 **          already knows — never typed.
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

#ifndef OEXPORTNAMEBUILDER_H
#define OEXPORTNAMEBUILDER_H

#include <QString>
#include <QStringList>

/**
 * @brief Builds the name of a generation: what was nested, then how it was printed.
 *
 * The name is derived rather than asked for. The entry dialog used to carry a field for
 * it, which meant the user had to invent a label for something the run already described
 * — and had to keep inventing one, run after run, for names that differ by a single
 * character. The two halves are now read off the settings:
 *
 *   - <b>what</b> — the size range's name, or the axes the single size sits on. Both are
 *     already displayed in the dialog, so the name repeats what the user just chose.
 *   - <b>how</b> — the token of the print model, never translated: it ends up in a file
 *     name, and a file name does not follow the interface language.
 *
 * Nothing here is translated. What the user reads on screen is the combo's label; what
 * the name carries is the token, and the two are allowed to differ — "Tiled Framed" reads
 * better than "Framed" in a list, and "Framed" reads better in a file name.
 */
namespace OExportNameBuilder
{
/**
 * @brief The base name: "<what>-<token>", or just "<token>" when nothing says what.
 *
 * @param rangeName the selected size range, empty on the single-size path
 * @param bases the dimension bases of a single size, in axis order — an axis the
 *              table does not carry contributes nothing, and is skipped rather than
 *              joined as an empty field: "-32-A4" would read as a missing value
 *              instead of an absent one.
 * @param token the print model's token
 */
auto Base(const QString &rangeName, const QStringList &bases, const QString &token) -> QString;

/**
 * @brief @p base made unique among @p taken, by appending " (1)", " (2)", …
 *
 * Two runs of the same settings over the same range are perfectly legitimate — the same
 * range printed on A4 and asked for twice is not a mistake. The entry dialog used to
 * refuse the duplicate outright, which made the second run impossible to ask for without
 * lying about what it was. A suffix tells the two rows apart and says which came first.
 *
 * When @p base is free it is returned untouched, so the common case leaves no trace.
 */
auto Unique(const QString &base, const QStringList &taken) -> QString;
} // namespace OExportNameBuilder

#endif // OEXPORTNAMEBUILDER_H
