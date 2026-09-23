/************************************************************************
 **
 **  @file   oexportgeometry.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — resolve every piece of the pattern for every size
 **          of a size range, straight from the model.
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

#ifndef OEXPORTGEOMETRY_H
#define OEXPORTGEOMETRY_H

#include <QPointF>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtNumeric>

#include "../vlayout/vlayoutpiece.h"
#include "core/oexportcuttingdirection.h"

class OSizerange;
class VContainer;
class VMeasurements;
class VPattern;

/// @brief One piece of the pattern, resolved for one size of the range.
struct OExportPieceGeometry
{
    /// Footprint key. Pieces of a group share it: the group is nested as one rigid block.
    QString key;
    QString name;
    QString groupName{}; ///< empty when the piece does not belong to a group
    VLayoutPiece piece;  ///< geometry resolved for this size, in pixels

    quint32 id{0};      ///< piece id, the stable identity — names are not unique
    quint32 groupId{0}; ///< 0 when the piece is not part of a group

    /// Pixels. Shifts this size onto the others of the same piece, so that unioning
    /// them into one footprint means something. Set by OExportAlignment; applied by
    /// the engine and by the canvas alike.
    QPointF alignDelta{};

    /// Pattern-frame X of the unit's forced alignment point for this size, or NaN when
    /// the unit has none. Resolved at collection time, because the container only ever
    /// holds the size being collected — OExportAlignment runs later, on the document
    /// put back on its own size.
    qreal alignPointX{qQNaN()};

    /// The piece's direction, resolved for a cutting plan and left unknown otherwise.
    ///
    /// ⚠️ **A grouped member takes the GROUP's direction, never its own.** `VLayoutPiece::Create`
    /// does not mask a member's grainline — it reads `IsEnabled()` and nothing else — so the flag
    /// can be true while the direction belongs to the group. The masking is done on the item.
    OExportCuttingDirection::Direction direction{};

    /// Whether this piece is cut ON THE FOLD — its fold line is one of its own edges.
    ///
    /// **Read on the LINE, never on the marking's type**: `FoldLineType` is born at
    /// `TwoArrowsTextAbove`, so "the type is not None" is true of every piece ever created — the
    /// error that left the guard silent the first time (§15.2). What says a piece has a fold is
    /// `SeamAllowanceMirrorLine` being there, and that is how the canvas reads it too.
    ///
    /// On a folded plan such a piece is laid **folded**, with that line glued to the fabric's fold;
    /// on a flat plan it is laid unfolded, as it always was.
    bool onFold{false};
};

/// @brief Every piece of the pattern, resolved for one size of the range.
struct OExportSizeLayer
{
    QString label;
    QVector<OExportPieceGeometry> pieces;
};

/// @brief Turns a size range of the pattern into resolved geometry.
///
/// The pattern is evaluated one size at a time — the container is shared, and
/// re-parsing it is exactly what the application does when the user switches
/// size. The document is left on the size it was found on.
///
/// This is what replaces reading the graded SVG: the geometry never leaves the
/// application, so there is nothing to write and nothing to parse back.
class OExportGeometry
{
public:
    /// @brief One size of a range: which dimension carries it, its base, its label.
    struct OExportSizeRef
    {
        int axisIndex{0}; ///< 0 = A, 1 = B, 2 = C
        qreal base{0.0};
        QString label;
    };

    /// @brief The base a range pins each axis at — one per slot. Zero means the range
    ///        pins nothing there and the document's own value stands.
    struct OExportFixedBases
    {
        qreal a{0.0};
        qreal b{0.0};
        qreal c{0.0};
    };

    OExportGeometry(VContainer *container, VPattern *doc,
                    const QSharedPointer<VMeasurements> &measurements);

    /// @brief The sizes a range covers, in order. Empty when the range does not
    ///        exist, is empty, or the measurements table carries no usable axis.
    ///
    /// It stops at naming the sizes on purpose: applying one is the application's
    /// own business, and it has exactly one way of doing it.
    auto RangeSizes(const QString &rangeName) -> QVector<OExportSizeRef>;

    /// @brief The value each axis is pinned at by a range — the geometry a range
    ///        describes, and the values the pieces must be resolved with.
    ///
    /// Walking a range means putting the document on each of its sizes; the axes it does
    /// not grade keep the value the range pins them at. Taking the document's own value
    /// instead sends an axis it does not carry back to the measurement file's base, and
    /// the exported geometry is then not the one the range names. The graded capture
    /// feeds its pieces exactly these values.
    auto RangeFixedBases(const QString &rangeName) -> OExportFixedBases;

    /// @brief Resolve every piece of the pattern at the size the document is on.
    ///
    /// Nothing about the measurements is touched: this is the geometry as Details
    /// and Layout already hold it. One layer comes back, never several.
    ///
    /// The caller is responsible for putting the document on the size it wants —
    /// through the path the application itself uses — before calling this.
    auto CollectCurrentSize(const QString &label) -> QVector<OExportSizeLayer>;

    /// @brief Why the last traversal produced nothing usable. Empty when it worked.
    ///
    /// A silent empty result is indistinguishable from a pattern with no piece:
    /// whoever calls this has to be able to say what actually happened.
    auto LastDiagnostic() const -> QString;

    /// @brief Whether the run is a cutting plan.
    ///
    /// A cutting plan lays pieces on fabric, and fabric has a grain — so the pieces are prepared
    /// here rather than later: their direction is resolved while the container is still at hand,
    /// and it is the only thing this flag changes today. A print run is left exactly as it was.
    void SetCuttingPlan(bool cutting);

    /// @brief Keep only these pieces; empty keeps every piece the layout holds.
    ///
    /// **This is how a cutting plan decomposes a group.** A group serves printing, where two
    /// pieces fill one shape on a sheet; on fabric a piece is cut once. The plan keeps the member
    /// the user chose and drops the others, so the footprint keyed on the group covers that one
    /// piece instead of the union of all its members.
    void SetPieceFilter(const QSet<quint32> &pieceIds);

    /// @brief Whether the fabric this cutting plan is laid on is folded in two.
    ///
    /// It decides two things here, and both are about the same piece: a piece cut on the fold is
    /// laid **folded** — half, its fold line on the fold — instead of being unfolded as a flat plan
    /// unfolds it; and a piece cut twice is laid **once**, because the fold mirrors it. Without a
    /// cutting plan this flag changes nothing.
    void SetFoldedFabric(bool folded);

private:
    /// The graded axis of a range, and the sizes it covers.
    struct AxisSelection
    {
        int index{1};           ///< position of the graded axis among the table's dimensions
        QVector<qreal> bases{}; ///< the sizes of the range, on that axis
    };

    auto SelectAxis(const OSizerange &range) const -> AxisSelection;
    auto LabelOf(double base, int axisIndex) const -> QString;
    void CollectPieces(OExportSizeLayer &layer) const;
    auto GroupNameOf(quint32 pieceId) const -> QString;

    VContainer *m_container{nullptr};
    VPattern *m_doc{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};
    /// @brief Pieces to keep. Empty keeps them all — see SetPieceFilter.
    QSet<quint32> m_pieceFilter{};
    /// @brief Whether this traversal prepares a cutting plan — see SetCuttingPlan.
    bool m_cuttingPlan{false};
    /// @brief Whether that cutting plan is laid on folded fabric — see SetFoldedFabric.
    bool m_foldedFabric{false};
    /// @brief Diagnostics are a side channel: they report on a const traversal.
    mutable QStringList m_diagnostics{};
};

#endif // OEXPORTGEOMETRY_H
