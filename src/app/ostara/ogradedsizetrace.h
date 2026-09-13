/************************************************************************
 **
 **  @file   ogradedsizetrace.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/09/2026
 **
 **  @brief  The graded size-style pass: the trace color and the pt→px dashes one
 **          size of a graded scene carries, applied to a silhouette subtree. Shared
 **          by the live Graded canvas and the headless per-size export, so a size is
 **          painted the same way on screen and in the exported SVG.
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

#ifndef OGRADEDSIZETRACE_H
#define OGRADEDSIZETRACE_H

#include "core/ogratedsizestyles.h"

#include <QColor>
#include <QHash>
#include <QString>

class QGraphicsItem;

namespace OGradedSizeTrace
{
/**
 * @brief Where a pass sits relative to the items it walks.
 *
 * The two consumers ask the same question — *does this pass own the default rendering?* —
 * and answer it differently, so it is the whole of their difference.
 */
enum class Scope
{
    /// Items just captured: the caller has painted them with the theme color moments ago,
    /// the pass only overrides what a style covers, and there is no native pen to remember
    /// because nothing it paints survives it. The headless export works this way.
    FreshItems,
    /// Items that outlive the pass: it owns the default rendering — it re-tints what no
    /// style covers, and restores the native pen it overrode — and it remembers that pen.
    /// The live canvas works this way, because it replays on a theme change, on the
    /// dialog's OK and on the "Show in canvas" toggle.
    LiveScene,
};

/**
 * @brief Apply the size styles to one silhouette root and its subtree.
 *
 * @p root carries kGradedSizeRole, put there at capture. Its label picks the style, and the
 * whole subtree is styled with it: a size style applies to the traces of a size, never item
 * by item. Decor traces (grainline, fold line) and the interactive children (grainline,
 * info blocks, annotations) are never restyled.
 *
 * @param styles       the styles of the axis, keyed by size label — the caller's hash, which
 *                     must outlive the call and not be mutated during it
 * @param scope        whether this pass owns the default rendering
 * @param defaultColor read only under Scope::LiveScene — the theme's piece color, what an
 *                     item no style covers is painted with
 */
void ApplyToSubtree(QGraphicsItem *root, const QHash<QString, OGradedSizeStyle> &styles, Scope scope,
                    const QColor &defaultColor = QColor());
}  // namespace OGradedSizeTrace

#endif  // OGRADEDSIZETRACE_H
