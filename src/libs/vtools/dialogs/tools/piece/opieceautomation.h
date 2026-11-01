/************************************************************************
 **
 **  @file   opieceautomation.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Shared curve sense automation for the piece tool: auto-reverse
 **          of curves (clockwise contour) and cut point auto-extension.
 **          Widget-free module used by both the legacy edit dialog and the
 **          new two-step creation dialog.
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

#ifndef OPIECEAUTOMATION_H
#define OPIECEAUTOMATION_H

#include <QObject>
#include <QPair>
#include <QSet>
#include <QVector>

#include "../vmisc/def.h"
#include "../vgeometry/vspline.h"
#include "../vpatterndb/vpiecenode.h"

class VContainer;

/**
 * @brief Widget-free piece contour collector: owns the main-path node sequence
 *        during collection and applies the curve sense automation on each click.
 *
 * Both the legacy edit dialog (DialogSeamAllowance) and the new creation dialog
 * (OPieceDialog) drive this module. It never touches widgets: the hosts keep
 * their own view (a QListWidget, a preview) and react to ListChanged().
 */
class VAbstractPattern;

class OPieceAutomation final : public QObject
{
    Q_OBJECT

public:
    explicit OPieceAutomation(const VContainer *data, QObject *parent = nullptr);

    // ---- A. Curve geometry (read-only) ----
    auto GetCurveEndPoints(quint32 curveId, QPointF &start, QPointF &end) const -> bool;
    auto PointIsOnCurve(quint32 pointId, quint32 curveId) const -> bool;

    /** @brief The pattern the tool reads its cuts from. Without it, a point is only ever a place, and a point a
     *  cut prolonged past the end of its curve is a place that is not on the curve. */
    void SetDocument(const VAbstractPattern *doc);
    auto ComputeReverseFromEntry(const QPointF &entry, quint32 curveId, bool &reverse) const -> bool;
    auto ComputeReverseFromExit(quint32 exitPointId, quint32 curveId, bool &reverse) const -> bool;
    auto CurvePointParam(quint32 curveId, quint32 pointId) const -> qreal;
    auto BoundPointId(int index, int step) const -> quint32;

    // ---- B. Sequence mutations (read/write m_nodes) ----
    auto CurveEntryPoint(quint32 curveId, QPointF &entry) const -> bool;
    auto LastCurveReverse(quint32 curveId, bool &reverse) const -> bool;
    void AddCurveNode(quint32 id, Tool nodeTool);
    void AutoExtendCurve(quint32 pointId);
    void ReconcilePendingReverse();
    void ResolvePendingReverseFromExit(quint32 exitPointId);
    auto CompleteClosingCurve(bool endpointsOnly) -> bool;
    void ShowSenseMessage();

    // ---- C. Click orchestration (the whole click handling) ----
    void HandleObjectChosen(quint32 id, const SceneObject &type);

    // ---- D. Main-path storage (source of truth during collection) ----
    auto Nodes() const -> const QVector<VPieceNode> &;
    void SetNodes(const QVector<VPieceNode> &nodes);
    auto LastId() const -> quint32;
    auto NodeCount() const -> int;
    auto PendingReverseCurves() const -> const QSet<quint32> &;
    void ClearPendingReverse();

signals:
    void ListChanged();
    void ToolTipMessage(const QString &msg);

private:
    /** @brief The cubics a curve is made of: itself for a spline, its sub-curves for a path, nothing for an
     *  arc — an arc is asked with its own geometry, not with cubics. */
    auto CubicPieces(const QSharedPointer<VGObject> &curve) const -> QVector<VSpline>;

    /** @brief True when @a pointId is a cut of @a curveId, or of a curve @a curveId is a piece of. A cut that
     *  falls beyond the end of its curve — the tool prolongs it — is on that curve all the same. */
    auto CutBelongsToFamily(quint32 pointId, quint32 curveId) const -> bool;
    auto CutBelongsToFamilyImpl(quint32 pointId, quint32 curveId) const -> bool;

    /** @brief The pattern object @a id comes from: a copy of a slash-and-spread, or a mirror of any operation,
     *  belongs to the family of the object it copies. Returns @a id itself when it is not a copy. */
    auto ResolveCopy(quint32 id) const -> quint32;

    const VAbstractPattern *m_doc{nullptr};

    /** @brief The distance from @a point to the closest place on @a cubic. */
    static auto NearestOnCubic(const VSpline &cubic, const QPointF &point) -> qreal;

    void AppendNode(const VPieceNode &node);
    void RemoveNodeById(quint32 id);
    void ApplyReverseToCurveRows(quint32 curveId, bool reverse);

    const VContainer *m_data;
    QVector<VPieceNode> m_nodes;
    QSet<quint32> m_pendingReverseCurves;
    bool m_hasPrematureClosingCurve{false};
};

/** @brief Free piece name for the piece tool ("Detail", "Detail_1", ...). */
auto DefaultPieceName(const VContainer *data) -> QString;

#endif // OPIECEAUTOMATION_H
