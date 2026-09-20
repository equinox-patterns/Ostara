/************************************************************************
 **
 **  @file   ocoverpagesizes.cpp
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

#include "ocoverpagesizes.h"

#include "core/ogratedsizestyles.h"
#include "core/osizetable.h"

#include "vmisc/defglobal.h"

#include <QHash>

auto OCoverPageSizes(const OSizeTable *table, const OCoverGradedAxis &range,
                     const VCommonSettings *settings) -> QVector<OCoverSizeLine>
{
    QVector<OCoverSizeLine> lines;
    if (table == nullptr || !range.HasRange())
    {
        return lines;
    }

    const QStringList values = table->AxisValues(range.axis);
    if (values.isEmpty())
    {
        return lines;
    }

    // The two ends are positions in the table's own order, and a range that is not *in* that order
    // is not a range this table can draw: the axis is the only thing that knows what comes first.
    const int first = static_cast<int>(values.indexOf(range.minValue));
    const int last = static_cast<int>(values.indexOf(range.maxValue));
    if (first < 0 || last < first)
    {
        return lines;
    }

    // **The styles are read once, for the whole axis**, and keyed by the size's own word — the same
    // key the Size Styles dock writes them under, which is what makes a style authored there show
    // up here without anything being copied between the two.
    const QHash<QString, OGradedSizeStyle> styles =
        OGradedSizeStyles::StylesForAxis(settings, table->SourcePath(), range.axis);

    lines.reserve(last - first + 1);
    for (int i = first; i <= last; ++i)
    {
        const QString label = values.at(i);
        OCoverSizeLine line;
        line.label = label;

        const QString alias = range.names.value(label);
        line.shown = alias.isEmpty() ? label : alias;

        const auto style = styles.constFind(label);
        if (style != styles.cend() && style->HasColor())
        {
            line.color = style->color;
            // Points become pixels here rather than at the drawing site: the painter works in print
            // pixels and knows nothing about the unit the user authored the pattern in.
            line.dashes = OGradedSizeStyles::DashLengthsPx(style->dashes, PrintDPI / 72.0);
        }
        else
        {
            // **No style, no ink of its own**: a size nothing was said about is drawn the way the
            // dock draws it — a solid black line — rather than left out. A page that listed the
            // sizes it has styles for would be a page that lies about the range.
            line.color = QColor(Qt::black);
        }
        lines.append(line);
    }
    return lines;
}
