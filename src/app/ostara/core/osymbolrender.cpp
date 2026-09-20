/************************************************************************
 **
 **  @file   osymbolrender.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  A mark drawn on its own — the one place a symbol becomes ink.
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

#include "osymbolrender.h"

#include "vmisc/def.h"
#include "vgeometry/vlayoutplacelabel.h"
#include "vlayout/vabstractpiece.h"
#include "vlayout/vfoldline.h"
#include "vpatterndb/vpassmark.h"
#include "vwidgets/vpiecegrainline.h"

#include <QPainter>
#include <QPolygonF>
#include <QTransform>

#include <algorithm>

namespace
{
/**
 * @brief The size a symbol has on the page, in print px — and the size it is built at.
 *
 * **Not a display convention — the real thing.** A mark lying on a piece takes its length from that
 * piece, and a symbol stands for every piece there is, so the size is settled here and a legend is
 * drawn at it. A preview showing a symbol larger than it will print would be showing a symbol
 * nobody will ever see.
 *
 * **The size is the mark's longer side**, whichever way it lies: a legend reads in rows, so the two
 * lines are drawn along the row rather than across it, and a grain line's 1.5 cm is its length and
 * not its height.
 *
 * **Two numbers per mark, because part of a mark does not scale with it.** The arrowheads a line
 * mark carries are a fixed fifteen pixels whatever line they stand on — that is the canvas's own
 * drawing, and not this file's to change. So what decides how large they look is the *ratio* between
 * the two: the longer a mark is built, the smaller its arrows come out once it is brought to the
 * size it prints at. Built at the size it is drawn, it would keep them at their full fifteen pixels.
 */
constexpr qreal kGrainlineSize = MmToPixel(15.0);   // 1.5 cm long
constexpr qreal kGrainlineBuild = MmToPixel(22.5);  // built half again as long: smaller arrowheads
constexpr qreal kFoldLineSize = MmToPixel(15.0);    // 1.5 cm wide
constexpr qreal kFoldLineBuild = MmToPixel(30.0);   // built twice as long: halved arrowheads

/// How far apart the two arrows of a fold marker stand, as a share of the axis they stand on — what
/// the canvas uses for a fold on a piece, and kept as it is: that part of the mark reads well.
constexpr qreal kFoldArrowsSpan = 0.9;

/**
 * @brief How far across the axis each of those arrows runs, as a share of it.
 *
 * **A symbol is read at a glance and has to look like what it stands for.** The canvas draws a
 * fold's arrows a whole centimetre long, which is right for a fold measured on a piece and wrong
 * here: the mark would stand taller than it needs to, where a fold marker is a line with two arrows
 * across it.
 */
constexpr qreal kFoldArrowsLine = 1.0 / 3.0;

constexpr qreal kNotchLength = MmToPixel(5.0);       // 0.5 cm
constexpr qreal kPlaceLabelBox = MmToPixel(5.0);     // 0.5 cm

/// The air between two marks of the same legend line — not the same gap as the one before the words,
/// which is the page painter's (@ref OCartouchePainter::kMarksGap): this one separates two things of
/// the same kind, that one separates the marks from what they mean.
constexpr qreal kMarkGap = 12.0;

//---------------------------------------------------------------------------------------------------------------------
auto SegmentPath(const QLineF &line) -> QPainterPath
{
    QPainterPath path(line.p1());
    path.lineTo(line.p2());
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto PolygonPath(const QPolygonF &polygon) -> QPainterPath
{
    // Closed, because these are filled: an arrowhead left open would be filled anyway, but the pen
    // and the fill would then disagree about where its edge is.
    QPainterPath path;
    path.addPolygon(polygon);
    path.closeSubpath();
    return path;
}

// Declared before the marks that use it, and defined further down beside what it needs: a mark is
// built and then brought to the size it prints at, so the drawing reads in the order a mark is made.
auto ScaledToSize(OSymbolRender::Drawing drawing, qreal size) -> OSymbolRender::Drawing;

//---------------------------------------------------------------------------------------------------------------------
auto NotchDrawing(const OSymbolShapes::Shape &shape) -> OSymbolRender::Drawing
{
    VPiecePassmarkData data;
    data.passmarkLineType = static_cast<PassmarkLineType>(shape.type);
    // A "Check mark" is one type and two shapes; what tells them apart is the opening, and it is
    // carried by the point rather than by the type — see OSymbolShapes.
    data.passmarkSAPoint.SetPassmarkClockwiseOpening(shape.clockwise);

    // **The seam edge is at the bottom and the notch stands on it.** A notch is cut into a seam
    // allowance and points into the piece: the baseline leaves the edge at the origin and rises.
    const QLineF baseLine(QPointF(0, 0), QPointF(0, -kNotchLength));

    OSymbolRender::Drawing drawing;
    for (const QLineF &line : VPassmark::ShapeLines(data, {baseLine}))
    {
        drawing.strokes.append(SegmentPath(line));
    }
    return drawing;
}

//---------------------------------------------------------------------------------------------------------------------
auto GrainlineDrawing() -> OSymbolRender::Drawing
{
    // The line is traced and the arrowheads are filled, which is how the canvas draws one
    // (VGrainlineItem::paint): the line is ink of the same colour as the mark, not a fourth thing.
    //
    // **Lying along the row rather than across it.** A legend reads in rows, and a grain line stood
    // up would take a centimetre and a half of height in a list of words; turned, it takes the width
    // of what it says and the height of its own arrowheads.
    //
    // **The angle is in radians**, and the conversion from the degrees a pattern is written in
    // happens before this call (VAbstractPiece::FindGrainlineGeometry) — MainLine hands it straight
    // to cos() and sin(). Nought lays the line along x; passing 90 would stand it up.
    const VPieceGrainline grainline(QPointF(), kGrainlineBuild, 0.0,
                                    GrainlineArrowDirection::twoWaysUpDown);

    OSymbolRender::Drawing drawing;
    drawing.strokes.append(SegmentPath(grainline.GetMainLine()));

    if (grainline.IsArrowUpEnabled())
    {
        drawing.fills.append(PolygonPath(grainline.ArrowUp()));
    }

    if (grainline.IsArrowDownEnabled())
    {
        drawing.fills.append(PolygonPath(grainline.ArrowDown()));
    }

    return ScaledToSize(drawing, kGrainlineSize);
}

//---------------------------------------------------------------------------------------------------------------------
auto FoldLineDrawing() -> OSymbolRender::Drawing
{
    // Two arrows standing on either side of the fold, and filled: that is FoldLineType::TwoArrows as
    // the canvas draws it. The "with text" variant is deliberately not offered — its text is what
    // the legend is for, and it would be written twice.
    // **On its side, like the grain line**: the fold runs along x and the two arrows stand on it,
    // each running across it. A legend reads in rows, and a fold marker stood up would push every
    // word below it down the page.
    const QLineF axis(QPointF(-kFoldLineBuild / 2.0, 0), QPointF(kFoldLineBuild / 2.0, 0));
    VFoldLine foldLine(axis, FoldLineType::TwoArrows);

    // **Both given rather than left automatic.** The canvas's own length for those arrows is a whole
    // centimetre and its span most of the axis — right for a fold measured on a piece, wrong for a
    // mark read at a glance, which would stand taller than it needed to. See kFoldArrowsLine.
    foldLine.SetWidth(kFoldLineBuild * kFoldArrowsSpan);
    foldLine.SetHeight(kFoldLineBuild * kFoldArrowsLine);

    // **Traced as well as filled, which is how the canvas draws it**: RefreshScale hands the mark a
    // pen *and* a brush, so the two arrows are filled and the line they stand on is traced. Filling
    // alone would show two arrowheads floating on nothing — which is what a fold line looks like
    // when its line is missing.
    const QVector<QPainterPath> paths = foldLine.FoldLinePath();

    OSymbolRender::Drawing drawing;
    drawing.strokes = paths;
    drawing.fills = paths;
    return ScaledToSize(drawing, kFoldLineSize);
}

//---------------------------------------------------------------------------------------------------------------------
auto PlaceLabelDrawing(const OSymbolShapes::Shape &shape) -> OSymbolRender::Drawing
{
    VLayoutPlaceLabel label;
    label.SetCenter(QPointF(0, 0));
    label.SetType(static_cast<PlaceLabelType>(shape.type));
    label.SetRotationMatrix(QTransform());
    // The box is given the way a mark's own item gives it — anchored at the origin, the shape drawn
    // around the position — so a symbol obeys the same rule as a mark lying on a piece.
    label.SetBox(QRectF(0, 0, kPlaceLabelBox, kPlaceLabelBox));

    OSymbolRender::Drawing drawing;
    drawing.strokes.append(VAbstractPiece::LabelShapePath(label));
    return drawing;
}

//---------------------------------------------------------------------------------------------------------------------
auto Bounds(const OSymbolRender::Drawing &drawing) -> QRectF
{
    QRectF box;
    bool found = false;

    const auto Include = [&box, &found](const QPainterPath &path)
    {
        const QRectF rect = path.boundingRect();
        box = found ? box.united(rect) : rect;
        found = true;
    };

    for (const QPainterPath &path : drawing.strokes)
    {
        Include(path);
    }

    for (const QPainterPath &path : drawing.fills)
    {
        Include(path);
    }

    return box;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The same drawing, brought to a size along its longer side.
 *
 * **This is what makes a mark's parts keep their proportions**, and the only reason a mark is built
 * at one size and drawn at another: what a line mark carries that does not scale with it — its
 * arrowheads — is scaled here, with everything else.
 *
 * **Its longer side, and not its height.** A mark may lie either way — a grain line along the row or
 * across it — and what is asked of it is how much room it takes, which is the side it is long on. A
 * mark measured by its height would come out four times its size the day it was turned on its side.
 */
auto ScaledToSize(OSymbolRender::Drawing drawing, qreal size) -> OSymbolRender::Drawing
{
    const QRectF bounds = Bounds(drawing);
    const qreal longSide = (std::max)(bounds.width(), bounds.height());
    if (longSide <= 0.0 || size <= 0.0)
    {
        return drawing;
    }

    const qreal scale = size / longSide;
    const QTransform transform = QTransform::fromScale(scale, scale);
    for (QPainterPath &path : drawing.strokes)
    {
        path = transform.map(path);
    }
    for (QPainterPath &path : drawing.fills)
    {
        path = transform.map(path);
    }
    return drawing;
}

//---------------------------------------------------------------------------------------------------------------------
auto FitTransform(const QRectF &box, const QRectF &rect) -> QTransform
{
    // A mark is fitted on the side that binds and never stretched: a notch drawn as wide as a square
    // is a notch nobody recognises. A mark may have no width at all — a single line has none — which
    // is why the two sides are asked for one at a time.
    qreal scale = 1.0;
    if (box.width() > 0 && box.height() > 0)
    {
        scale = (std::min)(rect.width() / box.width(), rect.height() / box.height());
    }
    else if (box.width() > 0)
    {
        scale = rect.width() / box.width();
    }
    else if (box.height() > 0)
    {
        scale = rect.height() / box.height();
    }

    QTransform transform;
    transform.translate(rect.center().x(), rect.center().y());
    transform.scale(scale, scale);
    transform.translate(-box.center().x(), -box.center().y());
    return transform;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolRender::Build(const OSymbolShapes::Shape &shape) -> Drawing
{
    switch (shape.family)
    {
        case OSymbolShapes::Family::Grainline:
            return GrainlineDrawing();
        case OSymbolShapes::Family::FoldLine:
            return FoldLineDrawing();
        case OSymbolShapes::Family::Passmark:
            return NotchDrawing(shape);
        case OSymbolShapes::Family::PlaceLabel:
            return PlaceLabelDrawing(shape);
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolRender::Compose(const QVector<OSymbolShapes::Shape> &shapes) -> OCartoucheSymbolMarks
{
    OCartoucheSymbolMarks block;

    QVector<Drawing> drawings;
    drawings.reserve(shapes.size());
    qreal width = 0.0;
    qreal height = 0.0;

    for (const OSymbolShapes::Shape &shape : shapes)
    {
        Drawing drawing = Build(shape);
        const QRectF bounds = Bounds(drawing);
        width += bounds.width();
        height = (std::max)(height, bounds.height());
        drawings.append(drawing);
    }

    if (drawings.isEmpty())
    {
        return block;
    }

    width += kMarkGap * static_cast<qreal>(drawings.size() - 1);

    qreal x = 0.0;
    for (const Drawing &drawing : drawings)
    {
        const QRectF bounds = Bounds(drawing);
        // **Each mark is centred on the block's middle**, not stood on a common baseline: a legend
        // mixes marks of different heights — a grain line is twice a notch — and what makes them one
        // row is their middles.
        const QTransform place = QTransform::fromTranslate(x - bounds.left(),
                                                           (height - bounds.height()) / 2.0 - bounds.top());

        for (const QPainterPath &path : drawing.strokes)
        {
            block.strokes.append(place.map(path));
        }
        for (const QPainterPath &path : drawing.fills)
        {
            block.fills.append(place.map(path));
        }

        x += bounds.width() + kMarkGap;
    }

    // A mark may have no width at all — a single line has none — and a block reported as nothing wide
    // would be given no room by a caller laying the line out. A pen width is what a line takes on
    // paper, so that is the floor, as NaturalSize has it.
    block.size = QSizeF((std::max)(width, 1.0), (std::max)(height, 1.0));
    return block;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolRender::NaturalSize(const OSymbolShapes::Shape &shape) -> QSizeF
{
    const QRectF box = Bounds(Build(shape));

    // A straight line has no thickness, and a caller giving room in proportion would give it none.
    // A pen width is what a line actually takes on paper, so that is the floor.
    return {(std::max)(box.width(), 1.0), (std::max)(box.height(), 1.0)};
}

//---------------------------------------------------------------------------------------------------------------------
void OSymbolRender::Draw(QPainter &painter, const QRectF &rect, const OSymbolShapes::Shape &shape,
                         const QColor &color, qreal penWidth)
{
    const Drawing drawing = Build(shape);
    if (drawing.strokes.isEmpty() && drawing.fills.isEmpty())
    {
        return;
    }

    const QTransform fit = FitTransform(Bounds(drawing), rect);

    // The paths are mapped rather than the painter transformed, so that the pen keeps the width the
    // caller asked for: a pen under a scaled painter would be scaled with it, and a symbol drawn in
    // a small rectangle would come out with a hairline where a line was wanted. **The mark keeps its
    // proportions either way** — only the ink's thickness is the caller's.
    QPainterStateGuard guard(&painter);

    painter.setPen(QPen(color, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    for (const QPainterPath &path : drawing.strokes)
    {
        painter.drawPath(fit.map(path));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    for (const QPainterPath &path : drawing.fills)
    {
        painter.drawPath(fit.map(path));
    }
}
