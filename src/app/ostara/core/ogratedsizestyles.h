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
#include <QPen>
#include <QString>
#include <QVector>

class VAbstractPattern;
class VCommonSettings;
class VContainer;

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

/**
 * @brief A style's dashes in pixels, at a resolution the caller names.
 *
 * A style stores its dashes in points — the unit the user works in — and every drawing site needs
 * them in pixels. **The resolution is not a property of the style, so it is not decided here**: the
 * canvas and the export draw at print resolution, the styles dock draws at screen resolution, and
 * the assembly-mark frame draws at whatever resolution its own grid is drawn at. What the sites
 * share is the conversion, not its scale, and folding one scale into this function would make the
 * others wrong.
 *
 * @param pxPerPt how many pixels one point is where the caller draws — `PrintDPI / 72.0` for
 *                anything meant for paper.
 */
auto DashLengthsPx(const QVector<qreal> &dashesPt, qreal pxPerPt) -> QVector<qreal>;

/**
 * @brief Give @p pen the trace of a size style: its ink, and its dash pitch.
 *
 * The pen keeps its width, its join and whatever else the caller set; what a style owns is the
 * colour and the dashes. **A Qt dash pattern counts in multiples of the pen width**, so the pixel
 * lengths are divided by it here — the exact inverse of what the SVG export multiplies back.
 *
 * The cap goes flat, so a dash ends where it says it does: Qt's default stretches each one by half
 * a width, and the SVG export caps at butt. A solid line keeps the square cap it has always had.
 *
 * **One function, because four things draw the same trace** — the graded canvas, the graded export,
 * the styles dock, and now a cover page's sample of it. Two of them disagreeing is a page that
 * previews one way and prints another.
 */
void ApplyToPen(QPen &pen, const QColor &color, const QVector<qreal> &dashesPt, qreal pxPerPt);

/**
 * @brief The (table, axis) context a style pass needs — resolved WITHOUT parsing the
 *        .omt: canonical path + declared (or active-range) size axis + in-library flag.
 *        The axis VALUES are only needed by the dock/dialog (GradedSizeStyleContext).
 */
struct StyleAxis
{
    QString canonical;
    QString axisName;
    bool inLibrary = false;
    bool ready = false;
};

/**
 * @brief Resolve which (table, axis) a pattern's size styles are stored under.
 *
 * Styles live at the software level, keyed by the measurements table they were authored
 * against — a pattern has to be brought back to that key before anything can be read.
 * A table outside the library has no stable key, so @c inLibrary tells the caller not to
 * style at all rather than to style against a name nobody authored anything under.
 *
 * @param activeRangeName the range the caller works on — the axis fallback when the table
 *                        declares none (external table, or data written before the
 *                        declaration existed)
 */
auto ResolveStyleAxis(VAbstractPattern *doc, const VContainer *pattern,
                      const QString &activeRangeName) -> StyleAxis;

/** @brief Styles stored for (canonical, axis), keyed by size-value label. */
auto StylesForAxis(const VCommonSettings *settings, const QString &canonicalPath,
                   const QString &axisName) -> QHash<QString, OGradedSizeStyle>;

/** @brief Replace the whole (canonical, axis) style set (empty set clears it). */
void SaveAxisStyles(VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                    const QHash<QString, OGradedSizeStyle> &styles);

} // namespace OGradedSizeStyles

#endif // OGRADEDSIZESTYLES_H
