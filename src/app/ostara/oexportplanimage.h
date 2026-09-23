/************************************************************************
 **
 **  @file   oexportplanimage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — the size an image of the plan is asked for.
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

#ifndef OEXPORTPLANIMAGE_H
#define OEXPORTPLANIMAGE_H

#include <QRectF>
#include <QSize>

/// @brief What an image of the plan measures — the arithmetic, and nothing else.
///
/// The user gives ONE side; the other follows the plan's own proportions, and this is where
/// that figure is worked out. No scene, no painter, no file, no widget — the rule of
/// @ref OExportArtboards and @ref OExportSheets, for the same reason: a quantity that can be
/// wrong by a multiplication has to be readable on its own.
///
/// The dialog asks this before it accepts a size, and the writer asks it again to write:
/// ONE arithmetic, so the figure read on screen and the geometry of the file cannot come
/// from two readings.
namespace OExportPlanImage
{
/// Which side of the plan the physical length measures. The other is computed, never asked for.
enum class Axis : qint8
{
    Width,
    Height
};

/// The plan in the two forms this feature needs — the rectangle of the scene, and the
/// millimetres the dialog speaks in. ONE object, so that the two are read together.
struct Plan
{
    QRectF rectPx{};

    qreal widthMm{0.0};
    qreal heightMm{0.0};
};

/// The image to write: how many pixels, counted at which resolution.
struct Target
{
    /// Never smaller than 1 × 1 on either side. A null QImage is not a failure a writer can
    /// report — it is an EMPTY FILE, and an empty file reads as a bug rather than a refusal.
    QSize sizePx{};

    /// The resolution the pixels were counted in. A PNG writes its own physical size from it;
    /// an SVG's page is a physical size already and does not read it.
    qreal dpi{0.0};

    auto IsValid() const -> bool { return !sizePx.isEmpty() && dpi > 0.0; }
};

/// @brief The plan the canvas drew, in both of the forms above.
///
/// @param drawnRect the rectangle the canvas recorded as it drew — the one place that decides
///        what the scene shows. The millimetres are derived here rather than by each caller.
auto PlanOf(const QRectF &drawnRect) -> Plan;

/// @brief The side that is NOT pinned, in MILLIMETRES; 0 when it cannot be worked out.
///
/// Unit-free on purpose: the ratio of two lengths of one plan has no unit to convert, so the
/// dialog — which knows the plan in millimetres — and the writer — which knows it in the
/// scene's pixels — get the same answer from the same call.
auto OtherSideMm(const Plan &plan, qreal targetMm, Axis axis) -> qreal;

/// @brief The image to write for @p plan pinned to @p targetMm on @p axis, counted at @p dpi.
///
/// An INVALID Target — rather than a guess — when the plan, the length or the resolution is
/// empty or negative. The caller refuses on it; nothing is written from a Target that is not
/// valid.
///
/// @param dpi PrintDPI for an SVG, whose page IS a physical size; the user's own figure for a
///        PNG, whose pixels ARE the size.
auto SizeFor(const Plan &plan, qreal targetMm, Axis axis, qreal dpi) -> Target;

/// The most pixels an image may hold. Above it QImage refuses the allocation AND says so with
/// a qWarning, which in Ostara opens a modal box before the caller can report the refusal its
/// own way. The dialog asks this before accepting, so that the case stays unreachable and the
/// writer never has to.
constexpr qint64 kMaxPixels = 100LL * 1000 * 1000;
} // namespace OExportPlanImage

#endif // OEXPORTPLANIMAGE_H
