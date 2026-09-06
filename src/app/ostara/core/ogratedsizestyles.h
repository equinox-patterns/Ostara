/************************************************************************
 **
 **  @file   ogratedsizestyles.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Typed bridge between a graded trace style (OGradedSizeStyle) and the
 **          software-level styles store (VCommonSettings, keyed by the
 **          measurements-table canonical path + size axis). Size styles never live
 **          in the .omt / .ostara.
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

#ifndef OGRADEDSIZESTYLES_H
#define OGRADEDSIZESTYLES_H

#include <QColor>
#include <QHash>
#include <QString>
#include <QVector>

class VCommonSettings;

/**
 * @brief The OGradedSizeStyle class is the trace style of one graded size value:
 *        an optional layer-name override (consumed by the SVG export), an
 *        optional trace color and an optional dash pattern (pairs of
 *        dash,space — empty means solid). Distinguishing stacked sizes on the
 *        Graded canvas and naming/styling the exported SVG layers.
 *
 * Dashes are expressed in POINTS (the Illustrator unit the user works with) and
 * Ostara converts them to px at render/export time (PrintDPI / 72, i.e. 4/3 at
 * 96 dpi). A value is styled on the canvas when @p color is valid; the dashes
 * only complete that color (empty = solid line). A style carrying only a layer
 * name affects the export, never the canvas.
 */
struct OGradedSizeStyle
{
    QString layerName;      // empty → derived name at export
    QColor color;           // invalid → the value keeps the current canvas render
    QVector<qreal> dashes;  // pt, [dash, space, dash, space, ...] — empty = solid

    /** @brief True when the style has a canvas trace color. */
    auto HasColor() const -> bool
    {
        return color.isValid();
    }

    /** @brief True when at least one field carries an override worth storing. */
    auto HasContent() const -> bool
    {
        return HasColor() || !layerName.isEmpty() || !dashes.isEmpty();
    }
};

/**
 * @brief Software-level graded size styles: read/write the whole (table, axis)
 *        style set, exactly like the graded favorites bridge. Only complete
 *        overrides are stored — a value with nothing set simply has no entry.
 */
namespace OGradedSizeStyles
{

/** @brief Styles stored for (canonical, axis), keyed by size-value label. */
auto StylesForAxis(const VCommonSettings *settings, const QString &canonicalPath,
                   const QString &axisName) -> QHash<QString, OGradedSizeStyle>;

/** @brief Replace the whole (canonical, axis) style set (empty set clears it). */
void SaveAxisStyles(VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                    const QHash<QString, OGradedSizeStyle> &styles);

} // namespace OGradedSizeStyles

#endif // OGRADEDSIZESTYLES_H
