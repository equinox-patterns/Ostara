/************************************************************************
 **
 **  @file   ocoverpagesizes.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The sizes a cover page lists, from the range it was given.
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

#ifndef OCOVERPAGESIZES_H
#define OCOVERPAGESIZES_H

#include <QColor>
#include <QString>
#include <QVector>

#include "core/ocovergradedaxis.h"

class OSizeTable;
class VCommonSettings;

/**
 * @brief One size as the page writes it: its word, what it is called, and the trace it is drawn
 * with.
 *
 * **The three travel together because they are one line.** A cover page that lists the sizes of a
 * range says *which size, under which name, drawn with which pen* — the name and the pen are asked
 * about separately in the editor and land on the same line on the page, so what the two produce is
 * carried as one thing rather than three lists that have to stay the same length.
 */
struct OCoverSizeLine
{
    QString label;           ///< the table's own word for the size, verbatim.
    QString shown;           ///< what the page writes: the alias when there is one, else the label.
    QColor color;            ///< the size's trace ink; black when the size has no style.
    QVector<qreal> dashes;   ///< its dashes IN PRINT PIXELS; empty = solid.

    /// The width a trace sample is drawn with, in print pixels. A pen, not a rule: it is content.
    static constexpr qreal kTracePenWidthPx = 1.0;
};

/**
 * @brief The sizes a range holds, in the table's own order.
 *
 * The two ends are labels of the axis, so the range is read by walking the axis and keeping what
 * stands between them — no arithmetic on the words, because a table may label its sizes « XS » or
 * « 38/40 » as readily as « 38 », and only the table knows their order.
 *
 * The trace comes from the styles the user authored for that (table, axis) in the Size Styles dock;
 * a size with no style is drawn in black, solid, which is what the dock shows for it too. Reading
 * them needs the table's own path, which the table knows — see OSizeTable::SourcePath.
 *
 * Empty when the range names no axis, when the table has none, or when either end is not one of
 * the axis's values: half a range is not a range, and a page that listed a guess would be worse
 * than a page that lists nothing.
 */
Q_REQUIRED_RESULT auto OCoverPageSizes(const OSizeTable *table, const OCoverGradedAxis &range,
                                       const VCommonSettings *settings) -> QVector<OCoverSizeLine>;

#endif // OCOVERPAGESIZES_H
