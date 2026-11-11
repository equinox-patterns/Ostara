/************************************************************************
 **
 **  @file   vtoolsplinepath.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 17/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef VTOOLSPLINEPATH_H
#define VTOOLSPLINEPATH_H

#include <QGraphicsItem>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QPair>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vmisc/def.h"
#include "osplinepathportions.h"
#include "vtoolabstractcurve.h"

class VSplinePath;
class VSimpleCurve;
template <class T> class QSharedPointer;

struct VToolSplinePathInitData : VToolAbstractCurveInitData
{
    QVector<quint32> points{};  // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<QString> a1{};      // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<QString> a2{};      // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<QString> l1{};      // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<QString> l2{};      // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<OAxis> axes{}; // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 duplicate{0};       // NOLINT(misc-non-private-member-variables-in-classes)

    /** The interior nodes this path is cut on (see OSplinePathPortions), and the ids of the runs they make.
     *  Read from the file on a parse, empty when the user has cut nothing. */
    QVector<int> cutNodes{}; // NOLINT(misc-non-private-member-variables-in-classes)
    /// Keyed by OSplinePathPortions::RunKey — or LegacyRunKey for a file written before the runs had two ends.
    QMap<QString, OSplinePathPortions::StoredPiece> portionIds{}; // NOLINT(misc-non-private-member-variables-in-classes)
};

/**
 * @brief The VToolSplinePath class tool for creation spline path.
 */
class VToolSplinePath : public VToolAbstractBezier
{
    Q_OBJECT // NOLINT

public:
    ~VToolSplinePath() override = default;
    void SetDialog() override;
    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolSplinePath *;
    static auto Create(VToolSplinePathInitData &initData, const QSharedPointer<VSplinePath> &path)
        -> VToolSplinePath *;
    static auto Create(VToolSplinePathInitData &initData) -> VToolSplinePath *;
    static const QString ToolType;
    static const QString OldToolType;
    static void UpdatePathPoints(VAbstractPattern *doc, QDomElement &element, const VSplinePath &path);
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::SplinePath)
    };

    auto getSplinePath() const -> VSplinePath;
    void setSplinePath(const VSplinePath &splPath);

    /**
     * @brief Shows this curve without writing it nor touching the undo stack: the living preview of the axis
     * mode, where the user must be able to walk away leaving nothing behind. The single command is pushed at
     * validation, by setSplinePath().
     */
    void PreviewPath(const VSplinePath &splPath);

    auto AxisSlotCount() const -> int override;
    auto AxisSlot(int slot) const -> OAxis override;
    void PreviewAxis(int slot, const OAxis &axis) override;
    void CommitAxis() override;
    void RevertAxis() override;

    void ShowVisualization(bool show) override;

    void ShowHandles(bool show) override;

    /** @brief Opens the edition dialog — the dock's "Edit curve" button calls it, as the pleat's own does. */
    void OpenOptionsDialog();

    // ---- The runs the user cut this path into, at its own nodes -------------------------------------------
    //
    // A path carries the points it was drawn through; those points are frontiers already. Checking one of the
    // interior ones in the dock does not cut anything: it says "the curve stops here", and the path is shown
    // as the runs between the frontiers — n checked nodes, n + 1 runs, each exactly the sub-splines between
    // two nodes. The mother curve is hidden while at least one node is cut, as a cut tool hides the curve it
    // replaces; unchecking the last box gives it back whole.
    //
    // The runs are objects of the pattern in their own right (a name, a place in the container, a layer), but
    // they have no tool of their own: they are derived from the mother on every parse, and the dock edits them
    // through it. That is what keeps them impossible to leave orphaned or out of step with their curve.

    /** @brief Is @a node cut? False for the two ends, which have nothing to separate. */
    auto IsCutNode(int node) const -> bool;

    /** @brief Cuts the path at @a node, or puts it back together there. Saved with the tool. */
    void SetCutNode(int node, bool cut);

    /** @brief The runs the path is shown as, in the order it runs; empty when no node is cut. */
    auto PortionRuns() const -> QVector<QPair<int, int>>;

signals:
    /**
     * @brief RefreshLine refresh control line.
     * @param indexSpline position spline in spline list.
     * @param pos position point in spline.
     * @param controlPoint new position control point.
     * @param splinePoint new position spline point.
     */
    void RefreshLine(const qint32 &indexSpline, SplinePointPosition pos, const QPointF &controlPoint,
                     const QPointF &splinePoint);

public slots:
    void ControlPointChangePosition(const qint32 &indexSpline, const SplinePointPosition &position, const QPointF &pos);
    void EnableToolMove(bool move) override;
    void AllowHover(bool enabled) override;
    void AllowSelecting(bool enabled) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;
    void CurveSelected(bool selected);

protected:
    void GroupVisibility(quint32 object, bool visible) override;
    void FullUpdateFromFile() override;
    void RebuildDerivedObjects() override;
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void SetVisualization() override;
    void RefreshCtrlPoints() override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private slots:
    void CurveReleased();

private:
    void HideShownPointLabels(bool hide);

    Q_DISABLE_COPY_MOVE(VToolSplinePath) // NOLINT
    QPointF oldPosition{};
    int splIndex{-1};
    bool moved{false};
    QSharedPointer<VSplinePath> oldMoveSplinePath{};
    QVector<quint32> m_hiddenLabels{};
    QSharedPointer<VSplinePath> newMoveSplinePath{};

    /** @brief The path as it was when the axis preview started, and null when none is running. The preview
     *  writes nothing, so this is the only trace of the state to put back on a refusal. */
    QSharedPointer<VSplinePath> axisPreviewStart{};
    /**
     * @brief controlPoints list pointers of control points.
     */
    QVector<VControlPointSpline *> controlPoints{};

    explicit VToolSplinePath(const VToolSplinePathInitData &initData, QGraphicsItem *parent = nullptr);

    auto IsMovable(int index) const -> bool;
    static void AddPathPoint(VAbstractPattern *doc, QDomElement &domElement, const VSplinePoint &splPoint);
    void UpdateControlPoints(const VSpline &spl, QSharedPointer<VSplinePath> &splPath, qint32 indexSpline) const;
    void SetSplinePathAttributes(QDomElement &domElement, const VSplinePath &path);

    void UndoCommandMove(const VSplinePath &oldPath, const VSplinePath &newPath);

    void InitControlPoints(const VSplinePath *splPath);

    auto GatherToolChanges() const -> ToolChanges;

    /** @brief Rebuilds the runs after @ref m_cutNodes changed, or when the path itself changed. */
    void RebuildPortions();

    /** @brief Shows the runs, or the whole path, and hides whichever of the two is not meant right now. */
    void ApplyPortionVisibility();

    /** @brief Whether the application is asking curves to answer hover. Kept because a cut curve must not — and
     *  unchecking the last box has to give the answer back, not force it on. */
    /** @brief The pattern's container: what this tool writes into.
     *
     *  NOT `VDataTool::data`, which is only this tool's copy of the container — a snapshot taken when the tool was
     *  read. A piece created there exists for this tool alone: drawn, `HasObject` true, and invisible to every
     *  other reader of the pattern (a dialog's copy, an operation, a layer). The cut tools write with
     *  `initData.data`, which is this pointer. */
    VContainer *m_patternData{nullptr};

    bool m_acceptHover{true};

    /** @brief Whether the layer of this curve is shown. The base class keeps its own answer private, and the
     *  runs must follow it: a run drawn under a masked layer is a curve nobody can hide. */
    bool m_layerVisible{true};

    /** @brief The interior nodes the user cut: always inside 1 .. CountPoints()-2, sorted. */
    QVector<int> m_cutNodes{};

    /** @brief The container object of each run, keyed by its two ends (OSplinePathPortions::RunKey). Written to the
     *  file, so a reparse — and a ticked box is one — finds the same object instead of allocating a new one beside
     *  it. It also holds the pieces kept for their users: a run that stopped being one when a box was unticked, but
     *  that an operation, a tool or a formula is built on, stays in the pattern under its id and its name. */
    QMap<QString, OSplinePathPortions::StoredPiece> m_portionIds{};

    /** @brief Forget the pieces that would stop being runs under @a cutNodes and that nothing is built on. Asked
     *  at the moment a box changes, when the graph knows every user — of this curve and of its copies. */
    void PruneUnusedPieces(const QVector<int> &cutNodes);

    /** @brief The items that draw the runs — children of this tool, but top-level items of the scene, so a
     *  layer can show a run while the curve's own layer is hidden (see VToolCut, which does the same). */
    QVector<VSimpleCurve *> m_portionItems{};
};

#endif // VTOOLSPLINEPATH_H
