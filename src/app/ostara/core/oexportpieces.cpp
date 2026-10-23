/************************************************************************
 **
 **  @file   oexportpieces.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/10/2026
 **
 **  @brief  Export mode — the pieces as a cutting plan has to read them.
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

#include "oexportpieces.h"

#include "../ogradedsvgwriter.h"  // OGradedSvg::OwnerDecorFor
#include "../oexportgeometry.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vwidgets/vpiecegrainline.h"
#include "../vpatterndb/vcontainer.h"
#include "oexportcuttingdirection.h"
#include "oexportcuttingguard.h"

#include <QDomDocument>
#include <QHash>
#include <QLineF>
#include <QStringList>

namespace
{
/// Three decimals of a pixel — a thirtieth of a millimetre. The nesting itself rounds vertices at
/// 1e-6 px, so this is the drawing's precision and not the engine's: what a reader rebuilds from
/// here is a shape to plan with, and a plan is placed in metres of cloth.
constexpr int kDecimals = 3;

auto Number(qreal value) -> QString
{
    return QString::number(static_cast<double>(value), 'f', kDecimals);
}

/// A contour as an SVG path, in the pattern's own frame.
auto PathData(const QVector<VLayoutPoint> &points) -> QString
{
    if (points.size() < 3)
    {
        return QString();
    }

    QStringList commands;
    commands.reserve(static_cast<int>(points.size()) + 1);

    bool first = true;
    for (const VLayoutPoint &point : points)
    {
        commands.append(QStringLiteral("%1 %2,%3")
                            .arg(first ? QStringLiteral("M") : QStringLiteral("L"), Number(point.x()),
                                 Number(point.y())));
        first = false;
    }
    commands.append(QStringLiteral("Z"));

    return commands.join(QLatin1Char(' '));
}

/// The extent of a piece's outline, in the frame the payload writes — the shape a reader places.
auto OutlineBox(const VLayoutPiece &piece) -> QRectF
{
    const QVector<VLayoutPoint> points = piece.GetMappedExternalContourPoints();
    if (points.isEmpty())
    {
        return {};
    }

    qreal lowX = points.first().x();
    qreal highX = lowX;
    qreal lowY = points.first().y();
    qreal highY = lowY;
    for (const VLayoutPoint &point : points)
    {
        lowX = std::min(lowX, point.x());
        highX = std::max(highX, point.x());
        lowY = std::min(lowY, point.y());
        highY = std::max(highY, point.y());
    }
    return QRectF(QPointF(lowX, lowY), QPointF(highX, highY));
}

/// The midpoint of a piece's fold axis, or NaN when it has none.
auto FoldSeat(const VLayoutPiece &piece) -> QPointF
{
    const QLineF fold = OExportCuttingDirection::MappedFoldLine(piece);
    return fold.isNull() ? QPointF(qQNaN(), qQNaN()) : fold.center();
}

/// A translation by @p delta, as a transform.
auto Shift(const QPointF &delta) -> QTransform
{
    QTransform transform;
    transform.translate(delta.x(), delta.y());
    return transform;
}

/// Where the rest of the decor lands: the same offset from the piece's CENTRE.
///
/// One authoring, one seat, applied to every size — but the seat has to follow the piece, because the
/// sizes do not occupy the same place in the document. Exact at the reference by construction: the
/// two extents are the same and the shift is zero, so nothing moves where it was drawn.
auto CentreSeat(const QRectF &from, const QRectF &to) -> QTransform
{
    if (from.isNull() || to.isNull())
    {
        return {};
    }
    return Shift(to.center() - from.center());
}

/// Where the FOLD's ink lands: the same place on the fold's own axis.
///
/// ⚠️ **A fold mark is not centred on anything.** It runs along the edge that goes on the cloth's
/// fold, and that edge moves with the size while the centre barely does — so a centre seat left the
/// mark floating beside the edge, visibly not touching it, and dragged its caption with it. The axis
/// is the only seat that keeps it on the edge it belongs to. A piece with no fold keeps the centre.
auto FoldSeatShift(const QPointF &from, const QPointF &to) -> QTransform
{
    if (qIsNaN(from.x()) || qIsNaN(to.x()))
    {
        return {};
    }
    return Shift(to - from);
}

/// A line as « x1,y1 x2,y2 ».
auto LineData(const QLineF &line) -> QString
{
    return QStringLiteral("%1,%2 %3,%4")
        .arg(Number(line.x1()), Number(line.y1()), Number(line.x2()), Number(line.y2()));
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPieces::Of(QDomDocument &doc, const QVector<OExportSizeLayer> &layers,
                       const QStringList &labels, const VContainer *pattern,
                       const QList<OGradedSvg::LayoutObject> &decor) -> QDomElement
{
    QDomElement pieces = doc.createElement(QStringLiteral("pieces"));
    // Version 2 — the decor travels with every piece, one copy per size. A version-1 payload carries
    // the same pieces with no decor at all, and a reader must not take it for a payload whose sizes
    // simply have nothing to draw: it would then place ten pieces and not one grain line.
    pieces.setAttribute(QStringLiteral("version"), QStringLiteral("2"));

    // The decor is looked up by the owner's ID, never by its name: two owners may share a name — the
    // visible layer hands out unique ids for exactly that reason — and a name-keyed table would give
    // one piece another's cartouche in silence. Two tables, because a piece and a group are two kinds
    // of owner with two different id spaces, and only the group has members to stand in for.
    QHash<quint32, const OGradedSvg::LayoutObject *> pieceDecorById;
    QHash<quint32, const OGradedSvg::LayoutObject *> groupDecorById;
    for (const OGradedSvg::LayoutObject &object : decor)
    {
        if (object.ownerId == 0)
        {
            continue;
        }
        if (object.isGroup)
        {
            groupDecorById.insert(object.ownerId, &object);
        }
        else
        {
            pieceDecorById.insert(object.ownerId, &object);
        }
    }

    // The reference seat, per piece: the first layer is the one the decor was resolved on.
    QHash<quint32, QRectF> referenceBox;
    QHash<quint32, QPointF> referenceFold;
    if (!layers.isEmpty())
    {
        for (const OExportPieceGeometry &geometry : layers.first().pieces)
        {
            referenceBox.insert(geometry.id, OutlineBox(geometry.piece));
            referenceFold.insert(geometry.id, FoldSeat(geometry.piece));
        }
    }

    for (int index = 0; index < layers.size(); ++index)
    {
        const OExportSizeLayer &layer = layers.at(index);
        // The name the drawing gave this layer, which is how a reader finds the piece again. The
        // layer's own label is the smaller answer and is only a fallback.
        const QString label = index < labels.size() ? labels.at(index) : layer.label;

        for (const OExportPieceGeometry &geometry : layer.pieces)
        {
            QDomElement piece = doc.createElement(QStringLiteral("piece"));

            // ⚠️ The key, and not the name alone: it is what the drawing's own `<g id="…">` carries,
            // and the only thing that ties this element to that group — a name repeats across size
            // layers, and a group's name repeats across groups.
            piece.setAttribute(QStringLiteral("key"), geometry.key);
            piece.setAttribute(QStringLiteral("name"), geometry.name);
            piece.setAttribute(QStringLiteral("size"), label);
            piece.setAttribute(QStringLiteral("id"), QString::number(geometry.id));
            if (geometry.groupId != 0)
            {
                piece.setAttribute(QStringLiteral("group"), QString::number(geometry.groupId));
            }

            // ── La forme de la pièce ────────────────────────────────────────────────────────────
            //
            // ⚠️ **In the PATTERN's frame, and not where the drawing put it.** A plan places the
            // pieces again, so a contour already moved onto a sheet is a plan already made — and,
            // worse, a contour turned by a placement no longer faces the grain angle written below
            // it. This is the same reading the nesting makes, of the same getter, so a reader that
            // rebuilds the plan starts from exactly the shape the engine was given.
            const QString outline = PathData(geometry.piece.GetMappedExternalContourPoints());
            if (!outline.isEmpty())
            {
                piece.setAttribute(QStringLiteral("outline"), outline);
            }

            // ── La direction ────────────────────────────────────────────────────────────────────
            //
            // Resolved here when the generation did not keep one: a print run never asks for a
            // direction, so the field is left empty and the piece still HAS one. Reading it here
            // rather than at nesting time costs one pure function, and it is the same reading the
            // cutting plan makes — `PieceDirection`, never a second opinion on the fold.
            auto direction = geometry.direction;
            if (!direction.known)
            {
                direction = OExportCuttingDirection::PieceDirection(geometry.piece);
                if (direction.known && geometry.mirrored)
                {
                    // A reflection about a vertical axis sends an angle `a` to `180 − a`, and it
                    // lives in the matrix — which leaves the stored grainline untouched.
                    direction = OExportCuttingDirection::Mirrored(direction);
                }
            }
            if (direction.known)
            {
                piece.setAttribute(QStringLiteral("grain"), Number(direction.angle));
                // The arrow type is the whole of « the same way round »: it says whether the piece
                // has one axis or two, so it says what that option is allowed to keep.
                piece.setAttribute(QStringLiteral("arrows"),
                                    QString::number(static_cast<int>(direction.arrows)));
            }

            // ── La ligne de droit-fil ──────────────────────────────────────────────────────────
            //
            // The ANGLE above is what a plan READS; the line is what a plan DRAWS. Written in the
            // same frame as the outline — mapped, so a piece that carries a mirror has its line
            // where the piece actually is — because a segment in any other frame would sit beside
            // the piece the moment the plan lays it somewhere else.
            QLineF grainline;
            if (geometry.grainline.resolved)
            {
                // The frozen line wins: it was resolved while the document was still on the size
                // that matters, and the live piece would answer for whatever size it is on now.
                grainline = geometry.grainline.mainLine;
            }
            else if (geometry.piece.GetGrainline().IsEnabled())
            {
                grainline = geometry.piece.GetGrainline().GetMainLine();
            }
            if (!grainline.isNull())
            {
                piece.setAttribute(QStringLiteral("grainLine"),
                                    LineData(geometry.piece.GetMatrix().map(grainline)));
            }

            // ── Le pli ──────────────────────────────────────────────────────────────────────────
            const QLineF fold = OExportCuttingDirection::MappedFoldLine(geometry.piece);
            if (!fold.isNull())
            {
                piece.setAttribute(QStringLiteral("onFold"), QStringLiteral("yes"));
                // The axis, and never the pair of arrows the drawing carries: this is the edge that
                // goes on the fold of the cloth.
                piece.setAttribute(QStringLiteral("foldAxis"), LineData(fold));

                const bool storedUnfolded = geometry.piece.IsShowFullPiece();
                piece.setAttribute(QStringLiteral("stored"), storedUnfolded
                                                                  ? QStringLiteral("unfolded")
                                                                  : QStringLiteral("folded"));

                // ⚠️ **The form the drawing does not carry.** A half piece is a closed contour, so
                // nothing tells it apart from a whole one — and a plan laid from the wrong one is
                // half a garment that looks finished. Flipping the piece's own flag and reading its
                // contour again is the mirror-and-union, done here, once: a reader never does it.
                VLayoutPiece other = geometry.piece;
                other.SetShowFullPiece(!storedUnfolded);
                const QString outline = PathData(other.GetMappedExternalContourPoints());
                if (!outline.isEmpty())
                {
                    piece.setAttribute(QStringLiteral("outlineAlt"), outline);
                }
            }

            // ── Les quantités ───────────────────────────────────────────────────────────────────
            //
            // Read on the piece, then on its group, and 0 means « not cut in that cloth at all » —
            // a figure the cartouche holds and the drawing never does. Left out when the pattern is
            // not at hand: a missing figure is a fact, a guessed one is not.
            if (pattern != nullptr && geometry.id != 0)
            {
                piece.setAttribute(
                    QStringLiteral("cutFabric"),
                    QString::number(OExportCuttingGuard::CutQuantity(
                        geometry.id, pattern, OExportCuttingGuard::Material::Fabric)));
                piece.setAttribute(
                    QStringLiteral("cutLining"),
                    QString::number(OExportCuttingGuard::CutQuantity(
                        geometry.id, pattern, OExportCuttingGuard::Material::Lining)));
            }

            // ── Le décor, recopié ───────────────────────────────────────────────────────────────
            //
            // The very ink the visible decor layer draws — same serializer, other frame — copied
            // into this record. Ten sizes give ten copies of ONE resolved drawing, and that is
            // deliberate: the copy is what lets a reader find the children of the size it was asked
            // for and place them without computing anything.
            //
            // A member of a group has no decor of its own (Graded and Details both hand it to the
            // group), so its record stays without one and the `<group>` below carries it.
            const auto entry = pieceDecorById.constFind(geometry.id);
            if (entry != pieceDecorById.cend() && entry.value() != nullptr)
            {
                // Where this size put the same piece, and therefore where the reference's seat goes.
                const QRectF here = OutlineBox(geometry.piece);
                const QRectF there = referenceBox.value(geometry.id);
                const QTransform projection = CentreSeat(there, here);
                const QTransform foldProjection =
                    FoldSeatShift(referenceFold.value(geometry.id), FoldSeat(geometry.piece));
                const QDomElement ink =
                    OGradedSvg::OwnerDecorFor(doc, *entry.value(), projection, foldProjection);
                if (!ink.isNull())
                {
                    piece.appendChild(ink);
                }
            }

            pieces.appendChild(piece);
        }
    }

    // ── Le décor des groupes ────────────────────────────────────────────────────────────────────
    //
    // Written beside the pieces, once per group, because a grouped member draws nothing of its own:
    // the group hosts the grain line, the fold and the cartouche for all of its members. A reader
    // reads a member's own `<decor>` when it has one and the group's otherwise — the same rule the
    // drawing applies, stated once here so nobody has to re-derive it.
    if (pattern != nullptr)
    {
        if (const auto *groups = pattern->DataPieceGroups(); groups != nullptr)
        {
            for (auto it = groups->cbegin(); it != groups->cend(); ++it)
            {
                const auto entry = groupDecorById.constFind(it.key());
                if (entry == groupDecorById.cend() || entry.value() == nullptr)
                {
                    continue;
                }
                const QDomElement ink = OGradedSvg::OwnerDecorFor(doc, *entry.value());
                if (ink.isNull())
                {
                    continue;
                }
                QDomElement group = doc.createElement(QStringLiteral("group"));
                group.setAttribute(QStringLiteral("id"), QString::number(it.key()));
                group.appendChild(ink);
                pieces.appendChild(group);
            }
        }
    }

    return pieces;
}
