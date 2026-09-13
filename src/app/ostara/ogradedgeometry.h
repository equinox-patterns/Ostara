/************************************************************************
 **
 **  @file   ogradedgeometry.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded SVG export: geometry helpers shared with the live Graded
 **          canvas. `OGraded::ApplyOffsets` is the reusable "positions" pass of
 **          MainWindow::ApplyGradedLayoutOffsets (reset to the frozen base,
 **          per-piece alignment, rigid-group alignment, per-range content
 **          offsets) — used both by the canvas and by the headless export so the
 **          exported SVG stays pixel-faithful to what is displayed.
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

#ifndef OGRADEDGEOMETRY_H
#define OGRADEDGEOMETRY_H

#include <QPointF>
#include <QRectF>
#include <QString>
#include <QtGlobal>

#include "../vpatterndb/osizerange.h"

class QGraphicsItem;
class VContainer;
class VGrainlineData;
enum class Unit : qint8;

namespace OGraded
{
/// @brief A group is a rigid block from two members up. Below that, its pieces are
///        handled one by one — the rule the canvas, the export and the container all
///        apply.
constexpr int kMinGroupMembers = 2;

/**
 * @brief The point of @p rect a piece aligns on.
 *
 * @param h 1 = left, 2 = centre, 3 = right — any other value means centre.
 * @param v 1 = top, 2 = centre, 3 = bottom — any other value means centre.
 *
 * Shared so the canvas and the export cannot drift apart on what "aligned" means.
 */
auto AnchorPoint(const QRectF &rect, int h, int v) -> QPointF;

/**
 * @brief Align and offset a set of graded items exactly like the live canvas
 *        does (positions pass of ApplyGradedLayoutOffsets).
 *
 * @param items   the graded silhouette roots (each tagged with kGradedPieceIdRole
 *                and kGradedBasePosRole), normally all the items of one range.
 * @param range   the exported range (content dx/dy applied on top).
 * @param pattern the pattern (pieces + groups, alignment source of truth).
 */
void ApplyOffsets(const QList<QGraphicsItem *> &items, const OSizerange &range,
                  const VContainer *pattern);

/// @brief Where an owner's grainline sits and how long it is, for one range.
struct GrainlinePlacement
{
    QPointF pos{};       ///< parent-local, pixels
    qreal lengthPx{0.0}; ///< pixels
};

/**
 * @brief Resolve an owner's grainline for one range.
 *
 * The range's override wins when it exists — storing one is exactly what says "this
 * range puts the grainline here". Without it the owner's own geometry stands, which is
 * what the owner carries outside any range.
 *
 * @param geom           the owner's grainline: a piece's, or a group's shared one
 * @param ov             the range's override for that owner
 * @param isGroup        a group's length and rotation are literals, a piece's are formulas
 * @param pattern        the pattern, variables of those formulas — unused for a group
 * @param unit           pattern unit, for the design length
 * @param fallbackCenter parent-local point used when the resolved position is (0, 0)
 */
auto ResolveGrainline(const VGrainlineData &geom, const OSizerange::GrainlineOverride &ov, bool isGroup,
                      const VContainer *pattern, const Unit &unit, const QPointF &fallbackCenter)
    -> GrainlinePlacement;

/**
 * @brief Rotation of an owner's grainline, in degrees.
 *
 * A piece's rotation is a formula, a group's is a literal — the rule the group holder
 * (OPieceGroupHolderItem::RefreshGrainline) and the Graded decor both apply.
 */
auto GrainlineRotation(const QString &formula, bool isGroup, const VContainer *pattern) -> qreal;

/**
 * @brief The grainline item to hang under @p parent, or nullptr when the shape is empty.
 *
 * Built at @p placement, which is already parent-local: the caller only has to hand it a
 * parent whose frame the placement was resolved in.
 */
auto MakeGrainlineItem(QGraphicsItem *parent, const VGrainlineData &geom,
                       const GrainlinePlacement &placement, qreal rotationDeg) -> QGraphicsItem *;
}  // namespace OGraded

#endif  // OGRADEDGEOMETRY_H
