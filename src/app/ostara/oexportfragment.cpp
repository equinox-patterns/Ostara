/************************************************************************
 **
 **  @file   oexportfragment.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Splitting a piece on the mat grid — the puzzle, cut by the
 **          cells of a domestic plotter's mat.
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

#include "oexportfragment.h"

#include <QPolygonF>

#include <clipper2/clipper.h>
#include <clipper2/clipper.rectclip.h>

#include <cmath>

namespace
{
/// Clipper2 counts in whole numbers. One pixel is kClipScale units — the same fixed-point scale the
/// nesting engine works in, so that a coordinate rounded here and a coordinate rounded there cannot
/// disagree by a fraction of a pixel and put a cut on the wrong side of a border.
constexpr double kClipScale = 1e6;

auto ToPoint64(const QPointF &point) -> Clipper2Lib::Point64
{
    return {static_cast<int64_t>(std::llround(point.x() * kClipScale)),
            static_cast<int64_t>(std::llround(point.y() * kClipScale))};
}

auto ToPath64(const QVector<QPointF> &points) -> Clipper2Lib::Path64
{
    Clipper2Lib::Path64 path;
    path.reserve(static_cast<size_t>(points.size()));

    for (const QPointF &point : points)
    {
        path.push_back(ToPoint64(point));
    }

    return path;
}

auto FromPath64(const Clipper2Lib::Path64 &path) -> QVector<QPointF>
{
    QVector<QPointF> points;
    points.reserve(static_cast<int>(path.size()));

    for (const Clipper2Lib::Point64 &point : path)
    {
        points.append(QPointF(static_cast<double>(point.x) / kClipScale,
                              static_cast<double>(point.y) / kClipScale));
    }

    return points;
}

/// The window, in the units Clipper2 counts in.
///
/// Rounded TO NEAREST, and deliberately not outwards: two neighbouring cells share a border, and a
/// window widened by a fraction on each side would make their parts overlap by twice that fraction.
/// The parts must touch exactly — that shared line IS the cut — so the border has to land on the
/// same whole number from both sides, which is what rounding to nearest gives.
auto ToRect64(const QRectF &rect) -> Clipper2Lib::Rect64
{
    return {static_cast<int64_t>(std::llround(rect.left() * kClipScale)),
            static_cast<int64_t>(std::llround(rect.top() * kClipScale)),
            static_cast<int64_t>(std::llround(rect.right() * kClipScale)),
            static_cast<int64_t>(std::llround(rect.bottom() * kClipScale))};
}

/// Whether a stretched line belongs to @p polygon.
///
/// Asked at several points along it, never at one: a grainline drawn straight across the opening of
/// a C-shaped piece runs through material, then through air, then through material again, and a
/// single sample could fall in the air and lose the line altogether.
///
/// ⚠️ The line is carried WHOLE by every part of its cell that holds any of it, so a line spanning
/// two parts of one cell keeps its stretch over the air between them. Two parts in one cell happen
/// only where a C-shaped outline reaches into it twice, and the cut line is not affected — but the
/// stroke is. Recorded in the spec rather than papered over.
auto Holds(const QVector<QPointF> &polygon, const QVector<QPointF> &line) -> bool
{
    if (polygon.size() < 3 || line.size() < 2)
    {
        return false;
    }

    const QPolygonF shape(polygon);
    constexpr int kSamples = 8;

    for (qsizetype i = 1; i < line.size(); ++i)
    {
        const QPointF from = line.at(i - 1);
        const QPointF to = line.at(i);

        for (int sample = 0; sample <= kSamples; ++sample)
        {
            const double t = static_cast<double>(sample) / kSamples;
            if (shape.containsPoint(from + (to - from) * t, Qt::OddEvenFill))
            {
                return true;
            }
        }
    }

    return false;
}

auto Turn(const QVector<QPointF> &points, const QTransform &turn) -> QVector<QPointF>
{
    QVector<QPointF> out;
    out.reserve(points.size());

    for (const QPointF &point : points)
    {
        out.append(turn.map(point));
    }

    return out;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportFragment::Split(const QVector<QPointF> &outline, const QVector<QVector<QPointF>> &lines,
                            const OExportPlotter::PiecePlan &plan) -> QVector<Part>
{
    QVector<Part> parts;

    if (outline.size() < 3 || plan.cuts.isEmpty())
    {
        return parts;
    }

    // The piece's geometry into the frame the cells live in. The turn is applied here and nowhere
    // else, so the outline and the cuts cannot end up measured against different boxes.
    const Clipper2Lib::Path64 cutLine = ToPath64(Turn(outline, plan.turn));

    Clipper2Lib::Paths64 pieceLines;
    pieceLines.reserve(static_cast<size_t>(lines.size()));
    for (const QVector<QPointF> &line : lines)
    {
        // Le tour est le même que celui du contour, et appliqué de la même façon : une polyligne
        // entière, pas un segment à la fois.
        pieceLines.push_back(ToPath64(Turn(line, plan.turn)));
    }

    for (int row = 0; row < plan.rows; ++row)
    {
        for (int column = 0; column < plan.columns; ++column)
        {
            const QRectF &cell = plan.cuts.at(row * plan.columns + column);
            const Clipper2Lib::Rect64 window = ToRect64(cell);

            // RectClip is Clipper2's fast path for exactly this: a path against an axis-aligned
            // rectangle. It gives back one polygon per blob, which is what a part is — a C-shaped
            // piece reaching into one cell twice leaves two parts there, and the machine would cut
            // two shapes.
            Clipper2Lib::RectClip64 clipper(window);
            const Clipper2Lib::Paths64 pieces = clipper.Execute({cutLine});

            // The same window, for the lines: RectClipLines clips OPEN paths and returns the pieces
            // of them that fall inside — a grainline crossing a border comes back in two.
            Clipper2Lib::RectClipLines64 lineClipper(window);
            const Clipper2Lib::Paths64 linePieces = lineClipper.Execute(pieceLines);

            for (const Clipper2Lib::Path64 &piece : pieces)
            {
                if (piece.size() < 3)
                {
                    continue; // a sliver with no surface is not a part
                }

                Part part;
                part.outline = FromPath64(piece);

                for (const Clipper2Lib::Path64 &linePiece : linePieces)
                {
                    if (linePiece.size() < 2)
                    {
                        continue;
                    }

                    const QVector<QPointF> points = FromPath64(linePiece);
                    if (Holds(part.outline, points))
                    {
                        part.lines.append(points);
                    }
                }

                parts.append(part);
            }
        }
    }

    return parts;
}
