/************************************************************************
 **
 **  @file   orewriteformulas.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/10/2026
 **
 **  @brief  Rewrite the formulas of a whole pattern, token by token.
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
#ifndef OREWRITEFORMULAS_H
#define OREWRITEFORMULAS_H

#include <QDomElement>
#include <QString>
#include <functional>

class VAbstractPattern;

/** @brief Rewrite every formula of @a root, token by token, through @a map.
 *
 *  The same walk as `RewriteDocumentFormulas`, rooted on an element instead of a document: the block of a piece
 *  is not a document yet when its names change, and its formulas have to be rewritten where it stands — without
 *  reaching into the pattern it came from, which is what rewriting the whole document would do.
 */
auto RewriteFormulas(QDomElement &root, const std::function<QString(const QString &)> &map) -> void;

/** @brief Rewrite every formula of @a doc, token by token, through @a map.
 *
 *  The one place that knows where a formula lives in a pattern: **every** element is visited and **every**
 *  attribute is read, in every block and every piece — a formula lives on the tool's own element as much as on a
 *  `<pathPoint>`, an `<increment>` or a `<node>`, and a list of tags leaves the second kind behind. A name is read
 *  all over a file — a point's formula, a piece's seam allowance, a label — and no dependency graph knows all of
 *  them, which is why renaming something takes the file as a whole.
 *
 *  @a map is handed each token exactly as the calculator reads it and returns what to write in its place. A
 *  formula the parser cannot read is left untouched: reporting it belongs to the calculator.
 */
auto RewriteDocumentFormulas(VAbstractPattern *doc, const std::function<QString(const QString &)> &map) -> void;

#endif // OREWRITEFORMULAS_H
