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
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vmisc/def.h"
#include "vtoolabstractcurve.h"

class VPointF;
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
    VCubicBezierPath *path{nullptr}; // NOLINT(misc-non-private-member-variables-in-classes)
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

    void ShowVisualization(bool show) override;

    /// Default handle spread around the chest/back width point (t), as a ratio of |Carrure − V|.
    static constexpr double kDefaultHandleRatio = 0.50;
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
    /// @param t  handle spread around the chest/back width point (0.1→0.5, default 0.50).
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
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void SetVisualization() override;
    void RefreshGeometry() override;

private:
    Q_DISABLE_COPY_MOVE(OToolArmholeCurve) // NOLINT

    explicit OToolArmholeCurve(const OToolArmholeCurveInitData &initData, QGraphicsItem *parent = nullptr);

    void SetPathAttributes(QDomElement &domElement, const VCubicBezierPath &path);
};

#endif // OTOOLARMHOLECURVE_H
