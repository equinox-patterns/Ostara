/************************************************************************
 **
 **  @file   otoolarmholecurve.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/08/2026
 **
 **  @brief  Armhole curve tool — 2-segment cubic Bezier path through 4
 **          user-selected points (armhole, chest/back width, perpendicular,
 **          shoulder) with 3 auto-computed control handles.
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

#ifndef OTOOLARMHOLECURVE_H
#define OTOOLARMHOLECURVE_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vmisc/def.h"
#include "vtoolabstractcurve.h"

class VPointF;
class VSimpleCurve;
template <class T> class QSharedPointer;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")

/// Result of SolveHandles — the 3 auto-computed control handles for the armhole curve.
struct ArmholeHandles
{
    VPointF h1a{}; // Segment 1, C1 = V = (Carrure.x, Em.y) — horizontal from armhole
    VPointF h1b{}; // Segment 1, C2 = V1 — incoming to chest/back width, 0.1cm before it
    VPointF h2a{}; // Segment 2, C1 — outgoing from chest/back width (mirrored by GetSpline)
};

struct OToolArmholeCurveInitData : VDrawToolInitData
{
    QSharedPointer<VCubicBezierPath> path{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** Whether the curve is shown as its two pieces — armhole → chest/back width → shoulder — instead of
     *  as one curve, and the ids the container gave those two pieces. Read from the file on a parse. */
    bool segmentAtCarrure{false};           // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 firstSegmentId{NULL_ID};        // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 secondSegmentId{NULL_ID};       // NOLINT(misc-non-private-member-variables-in-classes)
};

QT_WARNING_POP

class OToolArmholeCurve : public VToolAbstractBezier
{
    Q_OBJECT // NOLINT

public:
    ~OToolArmholeCurve() override = default;
    static auto Create(OToolArmholeCurveInitData initData) -> OToolArmholeCurve *;
    static const QString ToolType;
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::ArmholeCurve)
    };

    auto FirstPointName() const -> QString;   // armhole (path[0])
    auto SecondPointName() const -> QString;  // chest/back width (path[3])
    auto ThirdPointName() const -> QString;   // perpendicular (path[5])
    auto FourthPointName() const -> QString;  // shoulder (path[6])

    auto GetPath() const -> VCubicBezierPath;
    void SetPath(const VCubicBezierPath &path);

    /** @brief Is this curve shown as the two pieces it is made of at the chest/back width point? */
    auto IsSegmentAtCarrure() const -> bool;

    /**
     * @brief Shows the curve as its two pieces — armhole → chest/back width, chest/back width → shoulder —
     *        or gives it back whole.
     *
     * The curve already carries the joint: the two cubic segments meet at the chest/back width point, which
     * is an anchor of both. Cutting there is not a cut, it is naming what the geometry already is — so the
     * two pieces are exactly the two sub-bezier segments, and nothing is recomputed.
     */
    void SetSegmentAtCarrure(bool segment);

    /// The node the curve is cut on when it is shown as its two pieces: the joint of its two cubic segments, counted
    /// as the nodes of a path are (0 the armhole, 1 the chest/back width, 2 the shoulder). The copies an operation
    /// makes of the curve are cut on the same node (see OCopyPieces).
    static constexpr int CarrureNode = 1;

    void ShowVisualization(bool show) override;
    void AllowHover(bool enabled) override;
    void AllowSelecting(bool enabled) override;

    /// Handle spread around the chest/back width point (t), as a ratio of |Carrure − V|. The
    /// departing handle sits t·|Carrure−V| beyond the chest/back width point, i.e. at
    /// (1 + t)·|Carrure−V| from V — so t = 0.60 is the manual construction's "×1.6" from V.
    static constexpr double kDefaultHandleRatio = 0.60;
    /// Incoming handle ratio (β) used when the perpendicular is skipped — the original solver value.
    static constexpr double kPerpendicularRatio = 0.15;

    /// Compute the 3 auto-computed control handles for the 2-segment armhole curve.
    ///
    /// Segment 1 (armhole → chest/back width):
    ///   C1 = V = (Carrure.x, Em.y), C2 = V1 = Carrure − 0.1cm along V→Carrure
    /// Segment 2 (chest/back width → shoulder):
    ///   C1 = Carrure + t·(Carrure−V) — stored length = t·|V−Carrure|,
    ///        direction auto-mirrored by VCubicBezierPath::GetSpline() for G1 continuity
    ///
    /// The perpendicular point (segment 2 C2) and shoulder (segment 2 P4) are
    /// user-selected, not computed.
    ///
    /// @param t  handle spread around the chest/back width point (0.60 = the manual construction's
    ///           ×1.6 from V, which is where this value comes from).
    static auto SolveHandles(const VPointF &em, const VPointF &carrure, double t) -> ArmholeHandles;

    /// True when the perpendicular has been skipped: the user picked the shoulder point twice, so
    /// the third and fourth slots hold the same id. This is also the file marker — the XML then
    /// carries point6 == point7. An absent point6 (NULL_ID) counts as skipped too, so that a file
    /// missing the attribute still loads instead of failing hard.
    static auto IsPerpendicularSkipped(quint32 perpendicularId, quint32 shoulderId) -> bool;

    /// Segment 2 C2 when the perpendicular is skipped: shoulder + β·(carrure − shoulder), i.e. a
    /// point on the Carrure→Shoulder chord, which makes the curve arrive along the shoulder line.
    /// The returned point keeps the shoulder's identity (name, id, mode) — that is what makes
    /// SetPathAttributes() write point6 == point7 without any change there.
    static auto SolveSkippedPerpendicular(const VPointF &carrure, const VPointF &shoulder) -> VPointF;

    /// Build the 7-point path — the only factory, shared by the creation collector and the parser.
    /// It resolves the skipped-perpendicular case, so callers never have to know about it.
    ///
    /// @param perpendicular  the user-selected perpendicular, or the shoulder point when skipped
    static auto BuildPath(const VPointF &armhole, const VPointF &carrure, const VPointF &perpendicular,
                          const VPointF &shoulder) -> VCubicBezierPath;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void GroupVisibility(quint32 object, bool visible) override;
    void FullUpdateFromFile() override;
    void RebuildDerivedObjects() override;
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void SetVisualization() override;
    void RefreshGeometry() override;

private:
    Q_DISABLE_COPY_MOVE(OToolArmholeCurve) // NOLINT

    /** @brief Builds the two pieces, or drops them, from the state — the file is the only place ids come from. */
    void RebuildSegments();

    /** @brief Shows one thing and hides the other: the pieces, or the whole curve. */
    void ApplySegmentVisibility();

    /** @brief The pattern's container: what this tool writes into.
     *
     *  NOT `VDataTool::data`, which is only this tool's copy of the container — a snapshot taken when the tool was
     *  read. Objects created there exist for this tool alone: the canvas draws them, `HasObject` answers true, and
     *  every other reader of the pattern — a dialog's copy, an operation's factory, a layer — looks in the
     *  pattern's container and finds nothing. The cut tools write with `initData.data`, which is this pointer. */
    VContainer *m_patternData{nullptr};

    bool m_segmentAtCarrure{false};
    quint32 m_firstSegmentId{NULL_ID};
    quint32 m_secondSegmentId{NULL_ID};

    /** @brief The items that draw the pieces — top-level items of the scene, children of this tool as QObjects,
     *  so a layer can show one piece while the curve's own layer is masked (as VToolCut does with its halves). */
    VSimpleCurve *m_firstCurve{nullptr};
    VSimpleCurve *m_secondCurve{nullptr};

    bool m_layerVisible{true};
    bool m_acceptHover{true};

    explicit OToolArmholeCurve(const OToolArmholeCurveInitData &initData, QGraphicsItem *parent = nullptr);

    void SetPathAttributes(QDomElement &domElement, const VCubicBezierPath &path);
};

#endif // OTOOLARMHOLECURVE_H
