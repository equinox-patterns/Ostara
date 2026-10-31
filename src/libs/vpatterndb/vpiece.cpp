/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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
// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "vpiece.h"
#include "../ifc/exception/vexceptioninvalidnotch.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/osmoothpath.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vlayoutplacelabel.h"
#include "../vgeometry/vplacelabelitem.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/testpath.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/variables/vpiecearea.h"
#include "../vpatterndb/vpiecenode.h"
#include "vcontainer.h"
#include "vgeometrydef.h"
#include "vpassmark.h"
#include "vpiece_p.h"

#include <algorithm>

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainterPath>
#include <QSharedPointer>
#include <QTemporaryFile>

using namespace Qt::Literals::StringLiterals;

namespace
{
/// @brief The hundredth of a millimetre a seam-allowance width is compared on.
///
/// **The same step as `OSeamAllowanceLibrary::kComparisonSteps`, and it has to stay the same one.**
/// A width arrives from arithmetic and not from a keyboard — a centimetre and a half comes back
/// from the pattern's unit as a number that is only nearly fifteen — so both sides are brought to
/// the same step before they are compared. Two functions rounding on two different steps would stop
/// agreeing about what the piece is made of.
constexpr qreal kSAWidthSteps = 100.0;

//---------------------------------------------------------------------------------------------------------------------
auto RoundedMm(qreal mm) -> qreal
{
    return static_cast<qreal>(qRound64(mm * kSAWidthSteps)) / kSAWidthSteps;
}

//---------------------------------------------------------------------------------------------------------------------
auto IsPassmarksPossible(const QVector<VPieceNode> &path) -> bool
{
    int countPointNodes = 0;
    int countOthers = 0;

    for (const auto &node : path)
    {
        if (node.IsExcluded())
        {
            continue; // skip node
        }

        node.GetTypeTool() == Tool::NodePoint ? ++countPointNodes : ++countOthers;
    }

    return countPointNodes >= 3 || (countPointNodes >= 1 && countOthers >= 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto RotatePath(const QVector<VPieceNode> &path, vsizetype index) -> QVector<VPieceNode>
{
    if (index < 0 || index >= path.size())
    {
        return path;
    }

    return path.mid(index) + path.mid(0, index);
}

//---------------------------------------------------------------------------------------------------------------------
void AddRegularPoint(const VContainer *data,
                     const QVector<VPieceNode> &nodes,
                     vsizetype nodeIndex,
                     const QLineF &mirrorLine,
                     QVector<VSAPoint> &pointsEkv)
{
    VSAPoint ekvPoint = VPiecePath::PreparePointEkv(nodes, nodeIndex, data);

    if (!mirrorLine.isNull())
    {
        if (VFuzzyComparePoints(ekvPoint, mirrorLine.p1()))
        {
            ekvPoint.SetSAAfter(0);
        }
        else if (VFuzzyComparePoints(ekvPoint, mirrorLine.p2()))
        {
            ekvPoint.SetSABefore(0);
        }
        else
        {
            // A fold modelled with intermediate collinear nodes must stay at width 0 over its whole run,
            // not only at p1/p2: zero both sides for any point lying on the fold segment.
            const QPointF direction = mirrorLine.p2() - mirrorLine.p1();
            const qreal lengthSquared = QPointF::dotProduct(direction, direction);
            if (lengthSquared > 0)
            {
                const QPointF fromStart = ekvPoint.ToQPointF() - mirrorLine.p1();
                const qreal t = QPointF::dotProduct(fromStart, direction) / lengthSquared;
                const QPointF projection = mirrorLine.p1() + direction * t;
                if (t > -0.01 && t < 1.01
                    && QLineF(ekvPoint.ToQPointF(), projection).length() <= accuracyPointOnLine * 2)
                {
                    ekvPoint.SetSAAfter(0);
                    ekvPoint.SetSABefore(0);
                }
            }
        }
    }

    pointsEkv.append(ekvPoint);
}

//---------------------------------------------------------------------------------------------------------------------
auto IsCSAStart(const QVector<CustomSARecord> &records, quint32 id) -> int
{
    for (int i = 0; i < records.size(); ++i)
    {
        if (records.at(i).startPoint == id)
        {
            return i;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
void CheckAndStartCSA(const VContainer *data,
                      const VPieceNode &node,
                      const QVector<CustomSARecord> &records,
                      qreal width,
                      QVector<VSAPoint> &pointsEkv,
                      int &recordIndex,
                      bool &insertingCSA)
{
    recordIndex = IsCSAStart(records, node.GetId());
    if (recordIndex != -1 && records.at(recordIndex).includeType == PiecePathIncludeType::AsCustomSA)
    {
        insertingCSA = true;

        const VPiecePath path = data->GetPiecePath(records.at(recordIndex).path);
        QVector<VSAPoint> customPoints = path.SeamAllowancePoints(data, width, records.at(recordIndex).reverse);

        for (auto &point : customPoints)
        {
            point.SetAngleType(PieceNodeAngle::ByLengthCurve);
            point.SetSABefore(0);
            point.SetSAAfter(0);
            point.SetCustomSA(true);
        }

        pointsEkv += customPoints;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void CheckAndEndCSA(const VContainer *data,
                    const VPieceNode &node,
                    const QVector<CustomSARecord> &records,
                    QVector<VSAPoint> &pointsEkv,
                    int &recordIndex,
                    bool &insertingCSA)
{
    if (records.at(recordIndex).endPoint == node.GetId())
    {
        insertingCSA = false;
        recordIndex = -1;

        pointsEkv.append(VPiecePath::PreparePointEkv(node, data));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ProcessPointNode(const VContainer *data,
                      const QVector<VPieceNode> &nodes,
                      vsizetype nodeIndex,
                      const QVector<CustomSARecord> &records,
                      qreal width,
                      const QLineF &mirrorLine,
                      QVector<VSAPoint> &pointsEkv,
                      int &recordIndex,
                      bool &insertingCSA)
{
    const VPieceNode &node = nodes.at(nodeIndex);

    if (not insertingCSA)
    {
        AddRegularPoint(data, nodes, nodeIndex, mirrorLine, pointsEkv);
        CheckAndStartCSA(data, node, records, width, pointsEkv, recordIndex, insertingCSA);
    }
    else
    {
        CheckAndEndCSA(data, node, records, pointsEkv, recordIndex, insertingCSA);
    }
}
} // anonymous namespace

//---------------------------------------------------------------------------------------------------------------------
VPiece::VPiece()
  : VAbstractPiece(),
    d(new VPieceData(PiecePathType::PiecePath))
{
}

//---------------------------------------------------------------------------------------------------------------------
COPY_CONSTRUCTOR_IMPL_2(VPiece, VAbstractPiece)

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::operator=(const VPiece &piece) -> VPiece &
{
    if (&piece == this)
    {
        return *this;
    }
    VAbstractPiece::operator=(piece);
    d = piece.d;
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
VPiece::VPiece(VPiece &&piece) noexcept
  : VAbstractPiece(std::move(piece)),
    d(std::move(piece.d)) // NOLINT(bugprone-use-after-move)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::operator=(VPiece &&piece) noexcept -> VPiece &
{
    VAbstractPiece::operator=(piece);
    std::swap(d, piece.d);
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
VPiece::~VPiece() = default;

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPath() const -> VPiecePath
{
    return d->m_path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPath() -> VPiecePath &
{
    return d->m_path;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetPath(const VPiecePath &path)
{
    d->m_path = path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::MainPathPoints(const VContainer *data) const -> QVector<VLayoutPoint>
{
    //    DumpPiece(*this, data, QStringLiteral("input.json.XXXXXX"));  // Uncomment for dumping test data

    VPiecePath mainPath = GetPath();
    mainPath.SetName(QCoreApplication::translate("VPiece", "Main path of piece %1").arg(GetName()));

    QVector<VLayoutPoint> points = mainPath.PathPoints(data);
    points = CheckLoops(CorrectEquidistantPoints(points)); // A path can contains loops

    //    DumpVector(TurnPointList(points), QStringLiteral("output.json.XXXXXX")); // Uncomment for dumping test data

    //    DumpVector(points, QStringLiteral("output.json.XXXXXX")); // Uncomment for dumping test data
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::FullMainPathPoints(const VContainer *data) const -> QVector<VLayoutPoint>
{
    //    DumpPiece(*this, data, QStringLiteral("input.json.XXXXXX"));  // Uncomment for dumping test data

    VPiecePath mainPath = GetPath();
    mainPath.SetName(QCoreApplication::translate("VPiece", "Main path of piece %1").arg(GetName()));

    QVector<VLayoutPoint> points = mainPath.PathPoints(data);

    QLineF const mirrorLine = SeamMirrorLine(data);
    if (!mirrorLine.isNull() && IsShowFullPiece())
    {
        QString mirrorP1Label;
        QString mirrorP2Label;
        if (const QVector<VPointF> mirrorLinePoints = SeamMirrorLinePoints(data); mirrorLinePoints.size() >= 2)
        {
            mirrorP1Label = mirrorLinePoints.constFirst().name();
            mirrorP2Label = mirrorLinePoints.constLast().name();
        }
        points = VAbstractPiece::FullSeamPath(points, mirrorLine, GetName(), mirrorP1Label, mirrorP2Label);
    }

    points = CheckLoops(CorrectEquidistantPoints(points)); // A path can contains loops

    //    DumpVector(points, QStringLiteral("output.json.XXXXXX")); // Uncomment for dumping test data
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::UniteMainPathPoints(const VContainer *data) const -> QVector<VLayoutPoint>
{
    QVector<VLayoutPoint> points = VPiecePath::NodesToPoints(data, GetUnitedPath(data), GetName());
    points = CheckLoops(CorrectEquidistantPoints(points)); // A path can contains loops
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::MainPathNodePoints(const VContainer *data, bool showExcluded) const -> QVector<VPointF>
{
    return GetPath().PathNodePoints(data, showExcluded);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamAllowancePoints(const VContainer *data) const -> QVector<VLayoutPoint>
{
    return SeamAllowancePointsWithRotation(data, -1);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::FullSeamAllowancePoints(const VContainer *data) const -> QVector<VLayoutPoint>
{
    return FullSeamAllowancePointsWithRotation(data, -1);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::CuttingPathPoints(const VContainer *data) const -> QVector<QPointF>
{
    QVector<QPointF> points;
    if (IsSeamAllowance())
    {
        CastTo(FullSeamAllowancePoints(data), points);
        return points;
    }

    CastTo(FullMainPathPoints(data), points);
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::PassmarksLines(const VContainer *data) const -> QVector<QLineF>
{
    QVector<VPassmark> const passmarks = Passmarks(data);
    QVector<QLineF> lines;
    for (const auto &passmark : passmarks)
    {
        if (not passmark.IsNull())
        {
            lines += passmark.FullPassmark(*this, data);
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::Passmarks(const VContainer *data) const -> QVector<VPassmark>
{
    const QVector<VPieceNode> unitedPath = GetUnitedPath(data);
    if (not IsPassmarksPossible(unitedPath))
    {
        return {};
    }

    QVector<VPassmark> passmarks;

    for (int i = 0; i < unitedPath.size(); ++i)
    {
        const VPieceNode &node = unitedPath.at(i);
        if (node.IsExcluded() || !node.IsPassmark() || !node.IsPassmarkVisible(data))
        {
            continue; // skip node
        }

        const vsizetype previousIndex = VPiecePath::FindInLoopNotExcludedUp(i, unitedPath);
        const vsizetype nextIndex = VPiecePath::FindInLoopNotExcludedDown(i, unitedPath);

        passmarks += CreatePassmark(unitedPath, previousIndex, i, nextIndex, data);
    }

    return passmarks;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::CurvesPainterPath(const VContainer *data) const -> QVector<QPainterPath>
{
    return GetPath().CurvesPainterPath(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::MainPathPath(const VContainer *data) const -> QPainterPath
{
    QVector<QPointF> points;
    CastTo(MainPathPoints(data), points);
    return VPiece::MainPathPath(points);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::FullMainPathPath(const VContainer *data) const -> QPainterPath
{
    QVector<QPointF> points;
    CastTo(FullMainPathPoints(data), points);
    return VPiece::MainPathPath(points);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::MainPathPath(const QVector<QPointF> &points) -> QPainterPath
{
    QPainterPath path;

    if (not points.isEmpty())
    {
        // The contour is cut into a polyline by the geometry; it is handed over as the smooth curve those
        // points describe, so that it stays smooth at any zoom. See OSmoothPath::Path().
        path = OSmoothPath::Path(points, /*close=*/true);
        path.setFillRule(Qt::WindingFill);

#if !defined(V_NO_ASSERT)
        // uncomment for debug
        // QFont font;
        // font.setPixelSize(1);
        // for (qint32 i = 0; i < points.count(); ++i)
        // {
        //     path.addEllipse(points.at(i).x() - accuracyPointOnLine,
        //                     points.at(i).y() - accuracyPointOnLine,
        //                     accuracyPointOnLine * 2.,
        //                     accuracyPointOnLine * 2.);
        //     path.addText(points.at(i).x() - accuracyPointOnLine,
        //                  points.at(i).y() - accuracyPointOnLine,
        //                  font,
        //                  QString::number(i + 1));
        // }
#endif
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamAllowancePath(const VContainer *data) const -> QPainterPath
{
    return SeamAllowancePath(SeamAllowancePoints(data));
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::FullSeamAllowancePath(const VContainer *data) const -> QPainterPath
{
    return SeamAllowancePath(FullSeamAllowancePoints(data));
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::PassmarksPath(const VContainer *data) const -> QPainterPath
{
    const QVector<QLineF> passmarks = PassmarksLines(data);
    QPainterPath path;

    if (not passmarks.isEmpty())
    {
        for (qint32 i = 0; i < passmarks.count(); ++i)
        {
            path.moveTo(passmarks.at(i).p1());
            path.lineTo(passmarks.at(i).p2());
        }

        path.setFillRule(Qt::WindingFill);
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::PlaceLabelPath(const VContainer *data) const -> QPainterPath
{
    QPainterPath path;
    for (auto placeLabel : d->m_placeLabels)
    {
        try
        {
            const auto label = data->GeometricObject<VPlaceLabelItem>(placeLabel);
            if (!label->IsVisible())
            {
                continue;
            }

            VLayoutPlaceLabel const layoutLabel(*label);
            path.addPath(LabelShapePath(layoutLabel));

            if (const QLineF mirrorLine = SeamMirrorLine(data);
                !label->IsNotMirrored() && IsShowFullPiece() && !mirrorLine.isNull())
            {
                PlaceLabelImg shape = VAbstractPiece::PlaceLabelShape(layoutLabel);
                const QTransform matrix = VGObject::FlippingMatrix(mirrorLine);
                for (auto &points : shape)
                {
                    std::transform(points.begin(),
                                   points.end(),
                                   points.begin(),
                                   [&matrix](const VLayoutPoint &point) -> VLayoutPoint
                                   { return MapPoint(point, matrix); });
                }

                path.addPath(LabelShapePath(shape));
            }
        }
        catch (const VExceptionBadId &e)
        {
            // The piece may list a mark the container does not hold yet: during an undo/redo the parse
            // that recreates it is deferred, and the piece is drawn before that parse runs. Expected
            // and self-healing — kept out of the user's face, since a qWarning pops a modal box
            // (see VApplication's message handler).
            qDebug() << e.ErrorMessage();
        }
    }
    path.setFillRule(Qt::WindingFill);
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
static auto HasSeamOnAllowanceBoundary(const QVector<QPointF> &mainPath, const QVector<QPointF> &allowance) -> bool
{
    // A genuine zero seam allowance (e.g. a fold edge at 0, or any seam the user set to 0) makes the seam
    // path run exactly ON the seam-allowance outline. The two point sets are not index-paired (different
    // build pipelines) and the seam usually lies along an SA EDGE rather than on its vertices (fold seam at
    // 0: SA and seam share the fold line), so compare each seam point against every closed SA edge.
    if (allowance.size() < 3)
    {
        return false;
    }

    for (const QPointF &mp : mainPath)
    {
        for (int j = 0; j < allowance.size(); ++j)
        {
            if (IsPointOnLineSegment(mp, allowance.at(j), allowance.at((j + 1) % allowance.size())))
            {
                return true;
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsSeamAllowanceValid(const VContainer *data) const -> bool
{
    if (IsSeamAllowance())
    {
        QVector<QPointF> mainPathPoints;
        CastTo<QPointF>(UniteMainPathPoints(data), mainPathPoints);

        QVector<QPointF> seamAllowancePoints;
        CastTo<QPointF>(SeamAllowancePoints(data), seamAllowancePoints);

        // A true zero seam allowance puts the seam path ON the seam allowance outline, which the strict
        // IsInsidePolygon() test inside IsAllowanceValid() rejects (the seam allowance itself being zero is
        // legitimate — the user typed 0). Detect that case geometrically and fall back to the guards that
        // still matter: the seam allowance must remain a usable polygon, and an explicitly unfolded piece
        // must resolve to a real full outline — not silently collapse to the ~4-point zero-margin trace
        // (the seam-allowance mirror line truncation bug).
        if (not HasSeamOnAllowanceBoundary(mainPathPoints, seamAllowancePoints))
        {
            return VAbstractPiece::IsAllowanceValid(mainPathPoints, seamAllowancePoints);
        }

        if (seamAllowancePoints.size() < 3)
        {
            return false;
        }

        if (IsShowFullPiece())
        {
            QVector<QPointF> fullSeamAllowance;
            CastTo<QPointF>(FullSeamAllowancePoints(data), fullSeamAllowance);
            if (fullSeamAllowance.size() < 4)
            {
                return false;
            }
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsInLayout() const -> bool
{
    return d->m_inLayout;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetInLayout(bool inLayout)
{
    d->m_inLayout = inLayout;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsExcludedFromTotal() const -> bool
{
    return d->m_excludedFromTotal;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetExcludedFromTotal(bool excluded)
{
    d->m_excludedFromTotal = excluded;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsUnited() const -> bool
{
    return d->m_united;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetUnited(bool united)
{
    d->m_united = united;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetFormulaSAWidth() const -> QString
{
    return d->m_formulaWidth;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetFormulaSAWidth(const QString &formula, qreal value)
{
    SetSAWidth(value);
    const qreal width = GetSAWidth();
    width >= 0 ? d->m_formulaWidth = formula : d->m_formulaWidth = '0'_L1;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetGradedAlignH() const -> int
{
    return d->m_gradedAlignH;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetGradedAlignH(int val)
{
    d->m_gradedAlignH = val;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetGradedAlignV() const -> int
{
    return d->m_gradedAlignV;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetGradedAlignV(int val)
{
    d->m_gradedAlignV = val;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetGradedAlignPointId() const -> quint32
{
    return d->m_gradedAlignPointId;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetGradedAlignPointId(quint32 id)
{
    d->m_gradedAlignPointId = id;
}

//---------------------------------------------------------------------------------------------------------------------
// Disambiguates GetName between VAbstractPiece (virtual now) and IPieceInfo.
auto VPiece::GetName() const -> QString
{
    return VAbstractPiece::GetName();
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPieceNumber() const -> uint
{
    return d->m_pieceNumber;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetPieceNumber(uint number)
{
    d->m_pieceNumber = number;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetCutFabricQty() const -> int
{
    return d->m_cutFabricQty;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCutFabricQty(int qty)
{
    d->m_cutFabricQty = qty;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsCutFabricOnFold() const -> bool
{
    return d->m_cutFabricOnFold;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCutFabricOnFold(bool on)
{
    d->m_cutFabricOnFold = on;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetCutInterfacingQty() const -> int
{
    return d->m_cutInterfacingQty;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCutInterfacingQty(int qty)
{
    d->m_cutInterfacingQty = qty;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsCutInterfacingOnFold() const -> bool
{
    return d->m_cutInterfacingOnFold;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCutInterfacingOnFold(bool on)
{
    d->m_cutInterfacingOnFold = on;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetCutLiningQty() const -> int
{
    return d->m_cutLiningQty;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCutLiningQty(int qty)
{
    d->m_cutLiningQty = qty;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsCutLiningOnFold() const -> bool
{
    return d->m_cutLiningOnFold;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCutLiningOnFold(bool on)
{
    d->m_cutLiningOnFold = on;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetFreeText() const -> QString
{
    return d->m_freeText;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetFreeText(const QString &text)
{
    d->m_freeText = text;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetInternalPaths() const -> QVector<quint32>
{
    return d->m_internalPaths;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetInternalPaths() -> QVector<quint32> &
{
    return d->m_internalPaths;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetInternalPaths(const QVector<quint32> &iPaths)
{
    d->m_internalPaths = iPaths;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetCustomSARecords() const -> QVector<CustomSARecord>
{
    return d->m_customSARecords;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetCustomSARecords() -> QVector<CustomSARecord> &
{
    return d->m_customSARecords;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetCustomSARecords(const QVector<CustomSARecord> &records)
{
    d->m_customSARecords = records;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPlaceLabels() const -> QVector<quint32>
{
    return d->m_placeLabels;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPlaceLabels() -> QVector<quint32> &
{
    return d->m_placeLabels;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetPlaceLabels(const QVector<quint32> &labels)
{
    d->m_placeLabels = labels;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetSAMarginGroupNodeIds() const -> QVector<QVector<quint32>>
{
    return d->m_saMarginGroupNodeIds;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetSAMarginGroupNodeIds(const QVector<QVector<quint32>> &groups)
{
    d->m_saMarginGroupNodeIds = groups;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetSAMarginGroupNames() const -> QVector<QString>
{
    return d->m_saMarginGroupNames;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetSAMarginGroupNames(const QVector<QString> &names)
{
    d->m_saMarginGroupNames = names;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetSAMarginGroupColorIndices() const -> QVector<int>
{
    return d->m_saMarginGroupColorIndices;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetSAMarginGroupColorIndices(const QVector<int> &indices)
{
    d->m_saMarginGroupColorIndices = indices;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetSAMarginGroupShowStates() const -> QVector<bool>
{
    return d->m_saMarginGroupShowStates;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetSAMarginGroupShowStates(const QVector<bool> &states)
{
    d->m_saMarginGroupShowStates = states;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::Dependencies() const -> QList<quint32>
{
    QList<quint32> list = d->m_path.Dependencies();
    list.reserve(list.size() + d->m_customSARecords.size() + d->m_internalPaths.size() + d->m_placeLabels.size());

    for (const auto &record : d->m_customSARecords)
    {
        list.append(record.path);
    }

    for (const auto &value : d->m_internalPaths)
    {
        list.append(value);
    }

    for (const auto &value : d->m_placeLabels)
    {
        list.append(value);
    }

    return list;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetGrainlineGeometry(const VGrainlineData &data)
{
    d->m_glGrainline = data;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetGrainlineGeometry full access to the grainline geometry object
 * @return reference to grainline geometry object
 */
auto VPiece::GetGrainlineGeometry() -> VGrainlineData &
{
    return d->m_glGrainline;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetGrainlineGeometry returns the read-only reference to the grainline geometry object
 * @return reference to grainline geometry object
 */
auto VPiece::GetGrainlineGeometry() const -> const VGrainlineData &
{
    return d->m_glGrainline;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetInfoBlockData(const OInfoBlockData &data)
{
    d->m_infoBlocks = data;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetInfoBlockData returns the read-only reference to the per-piece information block state
 * @return reference to the information block state
 */
auto VPiece::GetInfoBlockData() const -> const OInfoBlockData &
{
    return d->m_infoBlocks;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetInfoBlockData full access to the per-piece information block state
 * @return reference to the information block state
 */
auto VPiece::GetInfoBlockData() -> OInfoBlockData &
{
    return d->m_infoBlocks;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetAnnotationData(const OAnnotationData &data)
{
    d->m_annotations = data;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetAnnotationData returns the read-only reference to the per-piece annotations
 * @return reference to the annotations
 */
auto VPiece::GetAnnotationData() const -> const OAnnotationData &
{
    return d->m_annotations;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetAnnotationData full access to the per-piece annotations
 * @return reference to the annotations
 */
auto VPiece::GetAnnotationData() -> OAnnotationData &
{
    return d->m_annotations;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetThumbnailData(const OThumbnailData &data)
{
    d->m_thumbnail = data;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetThumbnailData returns the read-only reference to the per-piece thumbnail state
 * @return reference to the thumbnail state
 */
auto VPiece::GetThumbnailData() const -> const OThumbnailData &
{
    return d->m_thumbnail;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDetail::GetThumbnailData full access to the per-piece thumbnail state
 * @return reference to the thumbnail state
 */
auto VPiece::GetThumbnailData() -> OThumbnailData &
{
    return d->m_thumbnail;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetMirrorLineStartPoint(quint32 id)
{
    d->m_mirrorLineStartPoint = id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetMirrorLineStartPoint() const -> quint32
{
    return d->m_mirrorLineStartPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetMirrorLineEndPoint(quint32 id)
{
    d->m_mirrorLineEndPoint = id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetMirrorLineEndPoint() const -> quint32
{
    return d->m_mirrorLineEndPoint;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamAllowancePointsWithRotation(const VContainer *data, vsizetype makeFirst) const -> QVector<VLayoutPoint>
{
    SCASSERT(data != nullptr);

    if (not IsSeamAllowance())
    {
        return {};
    }

    const qreal width = ToPixel(GetSAWidth(), *data->GetPatternUnit());
    const QVector<VPieceNode> unitedPath = makeFirst > 0 ? RotatePath(GetUnitedPath(data), makeFirst)
                                                         : GetUnitedPath(data);
    const QVector<VSAPoint> pointsEkv = BuildSeamAllowancePoints(data, unitedPath, width);

    return Equidistant(pointsEkv, width, GetName());
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::FullSeamAllowancePointsWithRotation(const VContainer *data, vsizetype makeFirst) const
    -> QVector<VLayoutPoint>
{
    QVector<VLayoutPoint> points = SeamAllowancePointsWithRotation(data, makeFirst);

    if (QLineF const mirrorLine = SeamAllowanceMirrorLine(data); !mirrorLine.isNull() && IsShowFullPiece())
    {
        QString mirrorP1Label;
        QString mirrorP2Label;
        if (const QVector<VPointF> mirrorLinePoints = SeamMirrorLinePoints(data); mirrorLinePoints.size() >= 2)
        {
            mirrorP1Label = mirrorLinePoints.constFirst().name();
            mirrorP2Label = mirrorLinePoints.constLast().name();
        }

        points = VAbstractPiece::FullSeamAllowancePath(points, mirrorLine, GetName(), mirrorP1Label, mirrorP2Label);

        const bool removeFirstAndLast = false;
        points = CorrectEquidistantPoints(points, removeFirstAndLast);
        points = CheckLoops(points); // A path can contains loops
    }

    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsManualFoldHeight() const -> bool
{
    return d->m_manualFoldHeight;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetManualFoldHeight(bool value)
{
    d->m_manualFoldHeight = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsManualFoldWidth() const -> bool
{
    return d->m_manualFoldWidth;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetManualFoldWidth(bool value)
{
    d->m_manualFoldWidth = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsManualFoldCenter() const -> bool
{
    return d->m_manualFoldCenter;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetManualFoldCenter(bool value)
{
    d->m_manualFoldCenter = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetFormulaFoldHeight() const -> QString
{
    return d->m_formulaFoldHeight;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetFormulaFoldHeight(const QString &value)
{
    d->m_formulaFoldHeight = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetFormulaFoldWidth() const -> QString
{
    return d->m_formulaFoldWidth;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetFormulaFoldWidth(const QString &value)
{
    d->m_formulaFoldWidth = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetFormulaFoldCenter() const -> QString
{
    return d->m_formulaFoldCenter;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::SetFormulaFoldCenter(const QString &value)
{
    d->m_formulaFoldCenter = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetUnitedPath(const VContainer *data) const -> QVector<VPieceNode>
{
    SCASSERT(data != nullptr)

    QVector<VPieceNode> united = d->m_path.GetNodes();

    const QVector<CustomSARecord> records = FilterRecords(GetValidRecords());

    for (const auto &record : records)
    {
        if (record.includeType != PiecePathIncludeType::AsMainPath)
        {
            continue;
        }

        const int indexStartPoint = VPiecePath::indexOfNode(united, record.startPoint);
        const int indexEndPoint = VPiecePath::indexOfNode(united, record.endPoint);

        if (indexStartPoint == -1 || indexEndPoint == -1)
        {
            continue;
        }

        QVector<VPieceNode> midBefore;
        QVector<VPieceNode> midAfter;
        if (indexStartPoint <= indexEndPoint)
        {
            midBefore = united.mid(0, indexStartPoint + 1);
            midAfter = united.mid(indexEndPoint, united.size() - midBefore.size());
        }
        else
        {
            midBefore = united.mid(indexEndPoint, indexStartPoint + 1);
        }

        QVector<VPieceNode> customNodes = data->GetPiecePath(record.path).GetNodes();
        if (record.reverse)
        {
            customNodes = Reverse(customNodes);
        }

        for (auto &customNode : customNodes)
        {
            // Additionally reverse all curves
            if (record.reverse)
            {
                // don't make a check because node point will ignore the change
                customNode.SetReverse(not customNode.GetReverse());
            }

            customNode.SetMainPathNode(false);
        }

        united = midBefore + customNodes + midAfter;
    }
    return united;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::DistinctSAWidths(const VContainer *data) const -> QVector<qreal>
{
    SCASSERT(data != nullptr)

    // A piece cut without a seam allowance has no margin to state. Naming a zero here would
    // announce a margin nobody sews: the piece is cut on its sewing line.
    if (not IsSeamAllowance())
    {
        return {};
    }

    const Unit *unit = data->GetPatternUnit();
    if (unit == nullptr)
    {
        return {};
    }

    // **The width a point inherits when it overrides nothing.** It is one of the piece's real
    // margins, so the panel offers it like any other — a piece whose every point was given a width
    // of its own still offers the default it was drawn from.
    const qreal inherited = RoundedMm(UnitConvertor(GetSAWidth(), *unit, Unit::Mm));

    QVector<qreal> widths{inherited};

    // **The path the engine offsets, not the raw one**: a custom margin is worked into the piece by
    // splicing its nodes here, and the widths it brings are the piece's widths too.
    for (const VPieceNode &node : GetUnitedPath(data))
    {
        // A point that is not drawn has no margin to read, and only a point node carries a width of
        // its own — the setters of the other kinds quietly store nothing.
        if (node.IsExcluded() || node.GetTypeTool() != Tool::NodePoint)
        {
            continue;
        }

        for (const qreal width : {node.GetSABefore(data), node.GetSAAfter(data)})
        {
            // **The sentinel is not a zero.** A negative answer means "I inherit", and resolves to
            // the piece's own width; a zero means the user asked for no margin right there. Reading
            // one as the other would drop the default from every piece in the pattern.
            const qreal mm = width < 0.0 ? inherited : RoundedMm(UnitConvertor(width, *unit, Unit::Mm));
            if (not widths.contains(mm))
            {
                widths.append(mm);
            }
        }
    }

    // Smallest first: an order the piece itself owns, and one a panel can be read down.
    std::sort(widths.begin(), widths.end());
    return widths;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetValidRecords() const -> QVector<CustomSARecord>
{
    QVector<CustomSARecord> records;
    records.reserve(d->m_customSARecords.size());
    for (const auto &record : d->m_customSARecords)
    {
        const int indexStartPoint = d->m_path.indexOfNode(record.startPoint);
        const int indexEndPoint = d->m_path.indexOfNode(record.endPoint);

        if (record.startPoint > NULL_ID && record.path > NULL_ID && record.endPoint > NULL_ID &&
            indexStartPoint != -1 && not d->m_path.at(indexStartPoint).IsExcluded() && indexEndPoint != -1 &&
            not d->m_path.at(indexEndPoint).IsExcluded())
        {
            records.append(record);
        }
    }
    return records;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::FilterRecords(QVector<CustomSARecord> records) const -> QVector<CustomSARecord>
{
    if (records.size() < 2)
    {
        return records;
    }

    QVector<VPieceNode> path = d->m_path.GetNodes();
    QVector<CustomSARecord> filteredRecords;
    for (auto record : std::as_const(records))
    {
        const int indexStartPoint = VPiecePath::indexOfNode(path, record.startPoint);
        const int indexEndPoint = VPiecePath::indexOfNode(path, record.endPoint);

        if (indexStartPoint == -1 || indexEndPoint == -1)
        {
            continue;
        }

        QVector<VPieceNode> midBefore;
        QVector<VPieceNode> midAfter;
        if (indexStartPoint <= indexEndPoint)
        {
            midBefore = path.mid(0, indexStartPoint + 1);
            midAfter = path.mid(indexEndPoint, path.size() - midBefore.size());
        }
        else
        {
            midBefore = path.mid(indexEndPoint, indexStartPoint + 1);
        }

        path = midBefore + midAfter;
        filteredRecords.append(record);
    }

    return filteredRecords;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetNodeSAPoints(const QVector<VPieceNode> &path, vsizetype index, const VContainer *data) const
    -> QVector<VSAPoint>
{
    SCASSERT(data != nullptr)

    if (index < 0 || index >= path.size())
    {
        return {};
    }

    const VPieceNode &node = path.at(index);
    QVector<VSAPoint> points;

    // The seam allowance contour zeroes the fold edge at the mirror endpoints (BuildSeamAllowancePoints /
    // AddRegularPoint). The passmark SA points must use the same zeroed widths, otherwise a notch sitting on
    // the fold sees a global width on the fold side and its base points away from the real (zero) outline.
    const QLineF mirrorLine = SeamMirrorLine(data);

    if (node.GetTypeTool() == Tool::NodePoint)
    {
        VSAPoint ekvPoint = VPiecePath::PreparePointEkv(path, index, data);

        if (!mirrorLine.isNull())
        {
            if (VFuzzyComparePoints(ekvPoint, mirrorLine.p1()))
            {
                ekvPoint.SetSAAfter(0);
            }
            else if (VFuzzyComparePoints(ekvPoint, mirrorLine.p2()))
            {
                ekvPoint.SetSABefore(0);
            }
        }

        points.append(ekvPoint);
    }
    else
    {
        const QSharedPointer<VAbstractCurve> curve = data->GeometricObject<VAbstractCurve>(node.GetId());
        const qreal width = ToPixel(GetSAWidth(), *data->GetPatternUnit());

        points += VPiecePath::CurveSeamAllowanceSegment(data, path, curve, index, node.GetReverse(), width,
                                                        mirrorLine, GetName());
    }
    return points;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPassmarkSAPoint(const QVector<VPieceNode> &path, vsizetype index, const VContainer *data,
                                VSAPoint &point) const -> bool
{
    SCASSERT(data != nullptr)

    const QVector<VSAPoint> points = GetNodeSAPoints(path, index, data);

    if (points.isEmpty() || points.size() > 1)
    {
        return false;
    }

    point = points.constFirst();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPassmarkPreviousSAPoints(const QVector<VPieceNode> &path, vsizetype index,
                                         const VSAPoint &passmarkSAPoint, const VContainer *data, VSAPoint &point,
                                         vsizetype passmarkIndex) const -> bool
{
    SCASSERT(data != nullptr)

    const QVector<VSAPoint> points = GetNodeSAPoints(path, index, data);

    if (points.isEmpty())
    {
        const QString errorMsg = tr("Cannot calculate a notch for point '%1' in piece '%2'.")
                                     .arg(VPiecePath::NodeName(path, passmarkIndex, data), GetName());
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        return false; // Something wrong
    }

    bool found = false;
    auto nodeIndex = points.size() - 1;
    do
    {
        const VSAPoint &previous = points.at(nodeIndex);
        QLineF const line(passmarkSAPoint, previous);
        if (line.length() > accuracyPointOnLine)
        {
            point = previous;
            found = true;
        }
        --nodeIndex;
    } while (nodeIndex >= 0 && not found);

    if (not found)
    {
        // No warning here because of valid case of passmark collapse
        return false; // Something wrong
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GetPassmarkNextSAPoints(const QVector<VPieceNode> &path, vsizetype index, const VSAPoint &passmarkSAPoint,
                                     const VContainer *data, VSAPoint &point, vsizetype passmarkIndex) const -> bool
{
    SCASSERT(data != nullptr)

    const QVector<VSAPoint> points = GetNodeSAPoints(path, index, data);

    if (points.isEmpty())
    {
        const QString errorMsg = tr("Cannot calculate a notch for point '%1' in piece '%2'.")
                                     .arg(VPiecePath::NodeName(path, passmarkIndex, data), GetName());
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        return false; // Something wrong
    }

    bool found = false;
    int nodeIndex = 0;
    do
    {
        const VSAPoint &next = points.at(nodeIndex);
        QLineF const line(passmarkSAPoint, next);
        if (line.length() >= ToPixel(1, Unit::Mm))
        {
            point = next;
            found = true;
        }
        ++nodeIndex;

    } while (nodeIndex < points.size() && not found);

    if (not found)
    {
        // No warning here because of valid case of passmark collapse
        return false; // Something wrong
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::IsPassmarkVisible(const QVector<VPieceNode> &path, vsizetype passmarkIndex) const -> bool
{
    if (passmarkIndex < 0 || passmarkIndex >= path.size())
    {
        return false;
    }

    const VPieceNode &node = path.at(passmarkIndex);
    if (node.GetTypeTool() != Tool::NodePoint || not node.IsPassmark() || node.IsExcluded())
    {
        return false;
    }

    const QVector<CustomSARecord> records = FilterRecords(GetValidRecords());
    if (records.isEmpty())
    {
        return true;
    }

    return std::all_of(records.begin(),
                       records.end(),
                       [&](const auto &record) -> auto
                       {
                           if (record.includeType == PiecePathIncludeType::AsCustomSA)
                           {
                               const int indexStartPoint = VPiecePath::indexOfNode(path, record.startPoint);
                               const int indexEndPoint = VPiecePath::indexOfNode(path, record.endPoint);
                               return passmarkIndex <= indexStartPoint || passmarkIndex >= indexEndPoint;
                           }
                           return true;
                       });
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::CreatePassmark(const QVector<VPieceNode> &path, vsizetype previousIndex, vsizetype passmarkIndex,
                            vsizetype nextIndex, const VContainer *data) const -> VPassmark
{
    SCASSERT(data != nullptr);

    if (not IsPassmarkVisible(path, passmarkIndex))
    {
        return {};
    }

    VSAPoint passmarkSAPoint;
    if (not GetPassmarkSAPoint(path, passmarkIndex, data, passmarkSAPoint))
    {
        const QString errorMsg = tr("Cannot calculate a notch for point '%1' in piece '%2'.")
                                     .arg(VPiecePath::NodeName(path, passmarkIndex, data), GetName());
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        return {};
    }

    VSAPoint previousSAPoint;
    if (not GetPassmarkPreviousSAPoints(path, previousIndex, passmarkSAPoint, data, previousSAPoint, passmarkIndex))
    {
        // No check here because it will cover valid cases
        return {}; // Something wrong
    }

    VSAPoint nextSAPoint;
    if (not GetPassmarkNextSAPoints(path, nextIndex, passmarkSAPoint, data, nextSAPoint, passmarkIndex))
    {
        // No check here because it will cover valid cases
        return {}; // Something wrong
    }

    if (const QLineF mirrorLine = SeamMirrorLine(data);
        !mirrorLine.isNull() && IsShowFullPiece()
        && IsPointOnLineviaPDP(passmarkSAPoint.ToQPointF(), mirrorLine.p1(), mirrorLine.p2(), accuracyPointOnLine * 2))
    {
        const QTransform matrix = VGObject::FlippingMatrix(mirrorLine);

        if (!IsPointOnLineviaPDP(previousSAPoint.ToQPointF(), mirrorLine.p1(), mirrorLine.p2(), accuracyPointOnLine * 2))
        {
            QPointF const newPos = matrix.map(previousSAPoint.ToQPointF());
            nextSAPoint.setX(newPos.x());
            nextSAPoint.setY(newPos.y());
            nextSAPoint.SetSABefore(previousSAPoint.GetSAAfter());
            nextSAPoint.SetSAAfter(previousSAPoint.GetSABefore());
        }
        else if (!IsPointOnLineviaPDP(nextSAPoint.ToQPointF(), mirrorLine.p1(), mirrorLine.p2(), accuracyPointOnLine * 2))
        {
            QPointF const newPos = matrix.map(nextSAPoint.ToQPointF());
            previousSAPoint.setX(newPos.x());
            previousSAPoint.setY(newPos.y());
            previousSAPoint.SetSABefore(nextSAPoint.GetSAAfter());
            previousSAPoint.SetSAAfter(nextSAPoint.GetSABefore());
        }
    }

    if (passmarkSAPoint.IsManualPasskmarkLength() && passmarkSAPoint.GetPasskmarkLength() <= 0)
    {
        return {};
    }

    if (passmarkSAPoint.IsManualPasskmarkLength() && passmarkSAPoint.GetPasskmarkLength() <= accuracyPointOnLine)
    {
        const QString infoMsg = tr("Notch for point '%1' in piece '%2' will be disabled. Manual length is less than "
                                   "allowed value.")
                                    .arg(VPiecePath::NodeName(path, passmarkIndex, data), GetName());
        qInfo() << VAbstractValApplication::warningMessageSignature + infoMsg;
        return {};
    }

    const VPieceNode &passmarkNode = path.at(passmarkIndex);
    return VPassmark({.previousSAPoint = previousSAPoint,
                      .passmarkSAPoint = passmarkSAPoint,
                      .nextSAPoint = nextSAPoint,
                      .saWidth = ToPixel(GetSAWidth(), *data->GetPatternUnit()),
                      .nodeName = VPiecePath::NodeName(path, passmarkIndex, data),
                      .pieceName = GetName(),
                      .passmarkLineType = passmarkNode.GetPassmarkLineType(),
                      .passmarkAngleType = passmarkNode.GetPassmarkAngleType(),
                      .isMainPathNode = passmarkNode.IsMainPathNode(),
                      .isShowSecondPassmark = passmarkNode.IsShowSecondPassmark(),
                      .passmarkIndex = passmarkIndex,
                      .id = passmarkNode.GetId(),
                      .globalPassmarkLength = ToPixel(GlobalPassmarkLength(data), *data->GetPatternUnit()),
                      .globalPassmarkWidth = ToPixel(GlobalPassmarkWidth(data), *data->GetPatternUnit()),
                      .notMirrored = passmarkNode.IsPassmarkNotMirrored()});
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::Area(const QVector<QPointF> &shape, const VContainer *data) const -> qreal
{
    SCASSERT(data != nullptr)

    const qreal mainArea = qAbs(VAbstractPiece::SumTrapezoids(shape)) / 2.0;

    qreal internalPathArea = 0;
    const QVector<quint32> pathsId = GetInternalPaths();
    for (auto id : pathsId)
    {
        const VPiecePath path = data->GetPiecePath(id);
        if (path.GetType() != PiecePathType::InternalPath || not path.IsVisible(data->DataVariables()) ||
            not path.IsCutPath())
        {
            continue;
        }

        QVector<QPointF> points;
        CastTo(path.PathPoints(data, shape), points);
        if (points.isEmpty() || not VFuzzyComparePoints(points.constFirst(), points.constLast()))
        {
            continue;
        }

        internalPathArea += qAbs(VAbstractPiece::SumTrapezoids(points)) / 2.0;
    }

    return mainArea - internalPathArea;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::BuildSeamAllowancePoints(const VContainer *data, const QVector<VPieceNode> &unitedPath, qreal width) const
    -> QVector<VSAPoint>
{
    QVector<VSAPoint> pointsEkv;
    int recordIndex = -1;
    bool insertingCSA = false;
    const QLineF mirrorLine = SeamMirrorLine(data);
    const QVector<CustomSARecord> records = FilterRecords(GetValidRecords());

    for (int i = 0; i < unitedPath.size(); ++i)
    {
        const VPieceNode &node = unitedPath.at(i);
        if (node.IsExcluded())
        {
            continue; // skip excluded node
        }

        ProcessNode(data, unitedPath, node, i, records, width, mirrorLine, pointsEkv, recordIndex, insertingCSA);
    }

    return pointsEkv;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::ProcessNode(const VContainer *data,
                         const QVector<VPieceNode> &unitedPath,
                         const VPieceNode &node,
                         int nodeIndex,
                         const QVector<CustomSARecord> &records,
                         qreal width,
                         const QLineF &mirrorLine,
                         QVector<VSAPoint> &pointsEkv,
                         int &recordIndex,
                         bool &insertingCSA) const
{
    switch (node.GetTypeTool())
    {
        case Tool::NodePoint:
            ProcessPointNode(data,
                             unitedPath,
                             nodeIndex,
                             records,
                             width,
                             mirrorLine,
                             pointsEkv,
                             recordIndex,
                             insertingCSA);
            break;
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
            ProcessCurveNode(data, unitedPath, node, nodeIndex, width, mirrorLine, pointsEkv, insertingCSA);
            break;
        default:
            qDebug() << "Get wrong tool type. Ignore." << static_cast<char>(node.GetTypeTool());
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::ProcessCurveNode(const VContainer *data,
                              const QVector<VPieceNode> &unitedPath,
                              const VPieceNode &node,
                              int nodeIndex,
                              qreal width,
                              const QLineF &mirrorLine,
                              QVector<VSAPoint> &pointsEkv,
                              bool insertingCSA) const
{
    if (not insertingCSA)
    {
        const QSharedPointer<VAbstractCurve> curve = data->GeometricObject<VAbstractCurve>(node.GetId());

        pointsEkv += VPiecePath::CurveSeamAllowanceSegment(data,
                                                           unitedPath,
                                                           curve,
                                                           nodeIndex,
                                                           node.GetReverse(),
                                                           width,
                                                           mirrorLine,
                                                           GetName());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GlobalPassmarkLength(const VContainer *data) const -> qreal
{
    QString const passmarkLengthVariable =
        VAbstractValApplication::VApp()->getCurrentDocument()->GetPassmarkLengthVariable();
    if (passmarkLengthVariable.isEmpty())
    {
        return 0;
    }

    qreal length = 0;

    try
    {
        QSharedPointer<VInternalVariable> const var = data->GetVariable<VInternalVariable>(passmarkLengthVariable);
        length = *var->GetValue();

        if (VAbstractValApplication::VApp()->toPixel(length) <= accuracyPointOnLine)
        {
            const QString errorMsg = QObject::tr("Invalid global value for a passmark length. Piece '%1'. Length is "
                                                 "less than minimal allowed.")
                                         .arg(GetName());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
    }
    catch (const VExceptionBadId &)
    {
        length = 0;
    }

    return length;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::GlobalPassmarkWidth(const VContainer *data) const -> qreal
{
    QString const passmarkWidthVariable =
        VAbstractValApplication::VApp()->getCurrentDocument()->GetPassmarkWidthVariable();
    if (passmarkWidthVariable.isEmpty())
    {
        return 0;
    }

    qreal width = 0;

    try
    {
        QSharedPointer<VInternalVariable> const var = data->GetVariable<VInternalVariable>(passmarkWidthVariable);
        width = *var->GetValue();

        if (VAbstractValApplication::VApp()->toPixel(width) <= accuracyPointOnLine)
        {
            const QString errorMsg = QObject::tr("Invalid global value for a passmark width. Piece '%1'. Width is "
                                                 "less than minimal allowed.")
                                         .arg(GetName());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
    }
    catch (const VExceptionBadId &)
    {
        width = 0;
    }

    return width;
}

//---------------------------------------------------------------------------------------------------------------------
#if !defined(V_NO_ASSERT)
// Use for writing tests
//---------------------------------------------------------------------------------------------------------------------
auto VPiece::MainPathToJson() const -> QJsonObject
{
    QJsonObject pieceObject{
        {"seamAllowance", IsSeamAllowance()},
        {"saWidth", GetSAWidth()},
    };

    QJsonArray nodesArray;
    for (qint32 i = 0; i < d->m_path.CountNodes(); ++i)
    {
        QJsonObject const nodeObject{
            {"id", static_cast<qint64>(d->m_path.at(i).GetId())},
            {"type", static_cast<int>(d->m_path.at(i).GetTypeTool())},
            {"reverse", d->m_path.at(i).GetReverse()},
        };

        nodesArray.append(nodeObject);
    }
    pieceObject["nodes"_L1] = nodesArray;

    return pieceObject;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::DBToJson(const VContainer *data) const -> QJsonObject
{
    QJsonArray itemsArray;
    for (qint32 i = 0; i < d->m_path.CountNodes(); ++i)
    {
        itemsArray.append(data->GetGObject(d->m_path.at(i).GetId())->ToJson());
    }

    QJsonObject dbObject{{"items", itemsArray}};

    return dbObject;
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::DumpPiece(const VPiece &piece, const VContainer *data, const QString &templateName)
{
    SCASSERT(data != nullptr)
    QTemporaryFile temp;       // Go to tmp folder to find dump
    temp.setAutoRemove(false); // Remove dump manually

    if (not templateName.isEmpty())
    {
        temp.setFileTemplate(QDir::tempPath() + QDir::separator() + templateName);
    }

    if (temp.open())
    {
#if defined(Q_OS_LINUX)
        //        On Linux, QTemporaryFile will attempt to create unnamed temporary
        //        files. If that succeeds, open() will return true but exists() will be
        //        false. If you call fileName() or any function that calls it,
        //        QTemporaryFile will give the file a name, so most applications will
        //        not see a difference.
        temp.fileName(); // call to create a file on disk
#endif
        QJsonObject testCase{
            {"bd", piece.DBToJson(data)},
            {"piece", piece.MainPathToJson()},
        };

        QJsonObject const json{
            {"testCase", testCase},
        };

        QJsonDocument const document(json);

        QTextStream out(&temp);
        out << document.toJson();
        out.flush();
    }
}
#endif // !defined(V_NO_ASSERT)

//---------------------------------------------------------------------------------------------------------------------
void VPiece::TestInternalPathCuttingPathIntersection(const VContainer *data) const
{
    SCASSERT(data != nullptr)

    const QVector<QPointF> cuttingPoints = CuttingPathPoints(data);
    const QPainterPath contourPath = VGObject::PainterPath(cuttingPoints);

    // Internal path for cutting must not intersect cutting contour and be inside of it.
    const QVector<quint32> pathsId = GetInternalPaths();
    for (auto id : pathsId)
    {
        const VPiecePath path = data->GetPiecePath(id);
        if (path.GetType() != PiecePathType::InternalPath || not path.IsVisible(data->DataVariables()) ||
            not path.IsCutPath())
        {
            continue;
        }

        QVector<QPointF> points;
        CastTo(path.PathPoints(data, cuttingPoints), points);
        if (points.isEmpty() || not VFuzzyComparePoints(points.constFirst(), points.constLast()))
        {
            continue;
        }

        const QPainterPath internalPath = VGObject::PainterPath(points);

        if (internalPath.intersects(contourPath))
        {
            const QString errorMsg = QObject::tr("Piece '%1'. Internal path '%2' intersects with cutting "
                                                 "contour.")
                                         .arg(GetName(), path.GetName());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VExceptionObjectError(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
            continue;
        }

        if (not contourPath.contains(internalPath))
        {
            const QString errorMsg = QObject::tr("Piece '%1'. Internal path '%2' not inside of cutting "
                                                 "contour.")
                                         .arg(GetName(), path.GetName());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VExceptionObjectError(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::TestInternalPathsIntersections(const VContainer *data) const
{
    SCASSERT(data != nullptr)

    const QVector<quint32> pathsId = GetInternalPaths();

    if (pathsId.isEmpty())
    {
        return;
    }

    const QVector<QPointF> cuttingPoints = CuttingPathPoints(data);

    // Internal pieces for cutting must not intersect
    QSet<QPair<int, int>> pairs;
    for (int k = 0; k < pathsId.size(); ++k)
    {
        const VPiecePath path1 = data->GetPiecePath(pathsId.at(k));

        if (path1.GetType() != PiecePathType::InternalPath || not path1.IsVisible(data->DataVariables()) ||
            not path1.IsCutPath())
        {
            continue;
        }

        QVector<QPointF> pointsPath1;
        CastTo(path1.PathPoints(data, cuttingPoints), pointsPath1);
        if (pointsPath1.isEmpty() || not VFuzzyComparePoints(pointsPath1.constFirst(), pointsPath1.constLast()))
        {
            continue;
        }

        const QPainterPath painterPath1 = VGObject::PainterPath(pointsPath1);

        for (int i = 0; i < pathsId.size(); ++i)
        {
            if (k == i || pairs.contains(qMakePair(k, i)) || pairs.contains(qMakePair(i, k)))
            {
                continue;
            }

            const VPiecePath path2 = data->GetPiecePath(pathsId.at(i));

            if (path2.GetType() != PiecePathType::InternalPath || not path2.IsVisible(data->DataVariables()) ||
                not path2.IsCutPath())
            {
                continue;
            }

            QVector<QPointF> pointsPath2;
            CastTo(path2.PathPoints(data, cuttingPoints), pointsPath2);
            if (pointsPath2.isEmpty() || not VFuzzyComparePoints(pointsPath2.constFirst(), pointsPath2.constLast()))
            {
                continue;
            }

            const QPainterPath painterPath2 = VGObject::PainterPath(pointsPath2);

            pairs.insert(qMakePair(k, i));
            pairs.insert(qMakePair(i, k));

            if (painterPath1.intersects(painterPath2))
            {
                const QString errorMsg = QObject::tr("Piece '%1'. Internal path '%2' intersects with internal path "
                                                     "'%3'.")
                                             .arg(GetName(), path1.GetName(), path2.GetName());
                VAbstractApplication::VApp()->IsPedantic()
                    ? throw VExceptionObjectError(errorMsg)
                    : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPiece::TestInternalPaths(const VContainer *data) const
{
    TestInternalPathCuttingPathIntersection(data);
    TestInternalPathsIntersections(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::ExternalArea(const VContainer *data) const -> qreal
{
    return Area(CuttingPathPoints(data), data);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamLineArea(const VContainer *data) const -> qreal
{
    QVector<QPointF> shape;
    CastTo(MainPathPoints(data), shape);
    return Area(shape, data);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamMirrorLinePoints(const VContainer *data) const -> QVector<VPointF>
{
    if (d->m_mirrorLineStartPoint == d->m_mirrorLineEndPoint || d->m_mirrorLineStartPoint == NULL_ID
        || d->m_mirrorLineEndPoint == NULL_ID)
    {
        return {};
    }

    try
    {
        const QSharedPointer<VPointF> startPoint = data->GeometricObject<VPointF>(d->m_mirrorLineStartPoint);
        const QSharedPointer<VPointF> endPoint = data->GeometricObject<VPointF>(d->m_mirrorLineEndPoint);

        return {*startPoint, *endPoint};
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamMirrorLine(const VContainer *data) const -> QLineF
{
    const QVector<VPointF> points = SeamMirrorLinePoints(data);
    if (points.size() < 2)
    {
        return {};
    }

    return {points.constFirst().toQPointF(), points.constLast().toQPointF()};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::SeamAllowanceMirrorLine(const VContainer *data) const -> QLineF
{
    QLineF const seamMirrorLine = SeamMirrorLine(data);
    if (seamMirrorLine.isNull())
    {
        return {};
    }

    QVector<QPointF> points;
    CastTo(SeamAllowancePoints(data), points);

    return VAbstractPiece::SeamAllowanceMirrorLine(seamMirrorLine, points);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPiece::ShortNameRegExp() -> QString
{
    static QString regex;

    if (regex.isEmpty())
    {
        const QList<QLocale> allLocales =
            QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);

        QString positiveSigns;
        QString negativeSigns;
        QString decimalPoints;
        QString groupSeparators;

        for (const auto &locale : allLocales)
        {
            if (not positiveSigns.contains(LocalePositiveSign(locale)))
            {
                positiveSigns.append(LocalePositiveSign(locale));
            }

            if (not negativeSigns.contains(LocaleNegativeSign(locale)))
            {
                negativeSigns.append(LocaleNegativeSign(locale));
            }

            if (not decimalPoints.contains(LocaleDecimalPoint(locale)))
            {
                decimalPoints.append(LocaleDecimalPoint(locale));
            }

            if (not groupSeparators.contains(LocaleGroupSeparator(locale)))
            {
                groupSeparators.append(LocaleGroupSeparator(locale));
            }
        }

        negativeSigns.replace('-'_L1, "\\-"_L1);
        groupSeparators.remove('\''_L1);

        // Same regexp in pattern.xsd shema file. Don't forget to synchronize.
        //  \p{Zs} - \p{Space_Separator}
        //  Here we use permanent start of string and end of string anchors \A and \z to match whole pattern as one
        //  string. In some cases, a user may pass multiline or line that ends with a new line. To cover case with a new
        //  line at the end of string use /z anchor.
        regex = QStringLiteral("\\A([^\\p{Zs}*\\/&|!<>^\\n\\()%1%2%3%4=?:;\"]){0,}\\z")
                    .arg(negativeSigns, positiveSigns, decimalPoints, groupSeparators);
    }

    return regex;
}
