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
 **  Modified for Ostara (fork of Valentina) on 10/10/2026.
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

#ifndef VPIECE_H
#define VPIECE_H

#include <QSharedDataPointer>
#include <QtGlobal>

#include "../vgeometry/osmoothpath.h"
#include "../vlayout/vabstractpiece.h"
#include "ipieceinfo.h"

class VPieceData;
class VPieceNode;
class VPiecePath;
class VGrainlineData;
class VContainer;
class QPainterPath;
class VPointF;
class VPassmark;
class OInfoBlockData;
class OAnnotationData;

class VPiece : public VAbstractPiece, public IPieceInfo
{
    Q_DECLARE_TR_FUNCTIONS(VPiece) // NOLINT

public:
    VPiece();
    VPiece(const VPiece &piece);

    ~VPiece() override;

    auto operator=(const VPiece &piece) -> VPiece &;

    VPiece(VPiece &&piece) noexcept;
    auto operator=(VPiece &&piece) noexcept -> VPiece &;

    auto GetPath() const -> VPiecePath;
    auto GetPath() -> VPiecePath &;
    void SetPath(const VPiecePath &path);

    auto MainPathPoints(const VContainer *data) const -> QVector<VLayoutPoint>;
    auto FullMainPathPoints(const VContainer *data) const -> QVector<VLayoutPoint>;
    auto UniteMainPathPoints(const VContainer *data) const -> QVector<VLayoutPoint>;
    auto MainPathNodePoints(const VContainer *data, bool showExcluded = false) const -> QVector<VPointF>;
    auto SeamAllowancePoints(const VContainer *data) const -> QVector<VLayoutPoint>;
    auto FullSeamAllowancePoints(const VContainer *data) const -> QVector<VLayoutPoint>;
    auto CuttingPathPoints(const VContainer *data) const -> QVector<QPointF>;
    auto PassmarksLines(const VContainer *data) const -> QVector<QLineF>;

    auto Passmarks(const VContainer *data) const -> QVector<VPassmark>;

    auto CurvesPainterPath(const VContainer *data) const -> QVector<QPainterPath>;

    auto MainPathPath(const VContainer *data) const -> QPainterPath;
    auto FullMainPathPath(const VContainer *data) const -> QPainterPath;
    static auto MainPathPath(const QVector<QPointF> &points) -> QPainterPath;

    auto SeamAllowancePath(const VContainer *data) const -> QPainterPath;
    auto FullSeamAllowancePath(const VContainer *data) const -> QPainterPath;
    template <class T> auto SeamAllowancePath(const QVector<T> &points) const -> QPainterPath;
    auto PassmarksPath(const VContainer *data) const -> QPainterPath;
    auto PlaceLabelPath(const VContainer *data) const -> QPainterPath;

    auto IsSeamAllowanceValid(const VContainer *data) const -> bool;

    auto IsInLayout() const -> bool;
    void SetInLayout(bool inLayout);

    auto IsUnited() const -> bool;
    void SetUnited(bool united);

    auto GetFormulaSAWidth() const -> QString;
    void SetFormulaSAWidth(const QString &formula, qreal value);

    auto GetGradedAlignH() const -> int;
    void SetGradedAlignH(int val);
    auto GetGradedAlignV() const -> int;
    void SetGradedAlignV(int val);

    auto GetGradedAlignPointId() const -> quint32;
    void SetGradedAlignPointId(quint32 id);

    auto GetName() const -> QString override;

    auto GetPieceNumber() const -> uint override;
    void SetPieceNumber(uint number) override;

    auto GetCutFabricQty() const -> int override;
    void SetCutFabricQty(int qty) override;
    auto IsCutFabricOnFold() const -> bool override;
    void SetCutFabricOnFold(bool on) override;

    auto GetCutInterfacingQty() const -> int override;
    void SetCutInterfacingQty(int qty) override;
    auto IsCutInterfacingOnFold() const -> bool override;
    void SetCutInterfacingOnFold(bool on) override;

    auto GetCutLiningQty() const -> int override;
    void SetCutLiningQty(int qty) override;
    auto IsCutLiningOnFold() const -> bool override;
    void SetCutLiningOnFold(bool on) override;

    auto GetFreeText() const -> QString override;
    void SetFreeText(const QString &text) override;

    auto GetInternalPaths() const -> QVector<quint32>;
    auto GetInternalPaths() -> QVector<quint32> &;
    void SetInternalPaths(const QVector<quint32> &iPaths);

    auto GetCustomSARecords() const -> QVector<CustomSARecord>;
    auto GetCustomSARecords() -> QVector<CustomSARecord> &;
    void SetCustomSARecords(const QVector<CustomSARecord> &records);

    auto GetPlaceLabels() const -> QVector<quint32>;
    auto GetPlaceLabels() -> QVector<quint32> &;
    void SetPlaceLabels(const QVector<quint32> &labels);

    auto GetSAMarginGroupNodeIds() const -> QVector<QVector<quint32>>;
    void SetSAMarginGroupNodeIds(const QVector<QVector<quint32>> &groups);
    auto GetSAMarginGroupNames() const -> QVector<QString>;
    void SetSAMarginGroupNames(const QVector<QString> &names);
    auto GetSAMarginGroupColorIndices() const -> QVector<int>;
    void SetSAMarginGroupColorIndices(const QVector<int> &indices);
    auto GetSAMarginGroupShowStates() const -> QVector<bool>;
    void SetSAMarginGroupShowStates(const QVector<bool> &states);

    auto Dependencies() const -> QList<quint32>;

    void SetGrainlineGeometry(const VGrainlineData &data);
    auto GetGrainlineGeometry() -> VGrainlineData &;
    auto GetGrainlineGeometry() const -> const VGrainlineData &;

    void SetInfoBlockData(const OInfoBlockData &data) override;
    auto GetInfoBlockData() const -> const OInfoBlockData & override;
    auto GetInfoBlockData() -> OInfoBlockData & override;

    void SetAnnotationData(const OAnnotationData &data) override;
    auto GetAnnotationData() const -> const OAnnotationData & override;
    auto GetAnnotationData() -> OAnnotationData & override;

    void SetThumbnailData(const OThumbnailData &data) override;
    auto GetThumbnailData() const -> const OThumbnailData & override;
    auto GetThumbnailData() -> OThumbnailData & override;

    void SetMirrorLineStartPoint(quint32 id);
    auto GetMirrorLineStartPoint() const -> quint32;

    void SetMirrorLineEndPoint(quint32 id);
    auto GetMirrorLineEndPoint() const -> quint32;

    auto GetUnitedPath(const VContainer *data) const -> QVector<VPieceNode>;

    /**
     * @brief The distinct seam-allowance widths this piece is cut with, in millimetres, smallest first.
     *
     * **A width belongs to a run, not to the piece.** Ostara cannot know how many independent runs
     * the user means to label, so it reports what the piece is made of and lets the panel decide how
     * many times each width is printed.
     *
     * @note Empty when the piece carries no seam allowance: there is then nothing to state, and a
     * zero would name a margin nobody sews.
     */
    auto DistinctSAWidths(const VContainer *data) const -> QVector<qreal>;

    auto SeamAllowancePointsWithRotation(const VContainer *data, vsizetype makeFirst) const -> QVector<VLayoutPoint>;
    auto FullSeamAllowancePointsWithRotation(const VContainer *data, vsizetype makeFirst) const
        -> QVector<VLayoutPoint>;

    auto IsManualFoldHeight() const -> bool;
    void SetManualFoldHeight(bool value);

    auto IsManualFoldWidth() const -> bool;
    void SetManualFoldWidth(bool value);

    auto IsManualFoldCenter() const -> bool;
    void SetManualFoldCenter(bool value);

    auto GetFormulaFoldHeight() const -> QString;
    void SetFormulaFoldHeight(const QString &value);

    auto GetFormulaFoldWidth() const -> QString;
    void SetFormulaFoldWidth(const QString &value);

    auto GetFormulaFoldCenter() const -> QString;
    void SetFormulaFoldCenter(const QString &value);

    static void DumpPiece(const VPiece &piece, const VContainer *data, const QString &templateName = QString());

    void TestInternalPaths(const VContainer *data) const;

    static auto ShortNameRegExp() -> QString;
    auto ExternalArea(const VContainer *data) const -> qreal;
    auto SeamLineArea(const VContainer *data) const -> qreal;

    auto SeamMirrorLinePoints(const VContainer *data) const -> QVector<VPointF>;
    auto SeamMirrorLine(const VContainer *data) const -> QLineF;
    auto SeamAllowanceMirrorLine(const VContainer *data) const -> QLineF;

private:
    QSharedDataPointer<VPieceData> d;

    auto GetValidRecords() const -> QVector<CustomSARecord>;
    auto FilterRecords(QVector<CustomSARecord> records) const -> QVector<CustomSARecord>;

    auto GetNodeSAPoints(const QVector<VPieceNode> &path, vsizetype index, const VContainer *data) const
        -> QVector<VSAPoint>;

    auto GetPassmarkSAPoint(const QVector<VPieceNode> &path, vsizetype index, const VContainer *data,
                            VSAPoint &point) const -> bool;
    auto GetPassmarkPreviousSAPoints(const QVector<VPieceNode> &path, vsizetype index, const VSAPoint &passmarkSAPoint,
                                     const VContainer *data, VSAPoint &point, vsizetype passmarkIndex) const -> bool;
    auto GetPassmarkNextSAPoints(const QVector<VPieceNode> &path, vsizetype index, const VSAPoint &passmarkSAPoint,
                                 const VContainer *data, VSAPoint &point, vsizetype passmarkIndex) const -> bool;

    auto IsPassmarkVisible(const QVector<VPieceNode> &path, vsizetype passmarkIndex) const -> bool;

    auto CreatePassmark(const QVector<VPieceNode> &path, vsizetype previousIndex, vsizetype passmarkIndex,
                        vsizetype nextIndex, const VContainer *data) const -> VPassmark;

    auto MainPathToJson() const -> QJsonObject;
    auto DBToJson(const VContainer *data) const -> QJsonObject;

    auto GlobalPassmarkLength(const VContainer *data) const -> qreal;
    auto GlobalPassmarkWidth(const VContainer *data) const -> qreal;

    void TestInternalPathCuttingPathIntersection(const VContainer *data) const;
    void TestInternalPathsIntersections(const VContainer *data) const;

    auto Area(const QVector<QPointF> &shape, const VContainer *data) const -> qreal;

    auto BuildSeamAllowancePoints(const VContainer *data, const QVector<VPieceNode> &unitedPath, qreal width) const
        -> QVector<VSAPoint>;
    void ProcessNode(const VContainer *data,
                     const QVector<VPieceNode> &unitedPath,
                     const VPieceNode &node,
                     int nodeIndex,
                     const QVector<CustomSARecord> &records,
                     qreal width,
                     const QLineF &mirrorLine,
                     QVector<VSAPoint> &pointsEkv,
                     int &recordIndex,
                     bool &insertingCSA) const;
    void ProcessCurveNode(const VContainer *data,
                          const QVector<VPieceNode> &unitedPath,
                          const VPieceNode &node,
                          int nodeIndex,
                          qreal width,
                          const QLineF &mirrorLine,
                          QVector<VSAPoint> &pointsEkv,
                          bool insertingCSA) const;
};

Q_DECLARE_TYPEINFO(VPiece, Q_MOVABLE_TYPE); // NOLINT

//---------------------------------------------------------------------------------------------------------------------
template <class T> inline auto VPiece::SeamAllowancePath(const QVector<T> &points) const -> QPainterPath
{
    QPainterPath ekv;

    // seam allowence
    if (IsSeamAllowance() && not points.isEmpty())
    {
        // The allowance is a point-set offset: its own line cannot be a curve, but it can be drawn as the
        // smooth curve those points describe instead of a chain of segments. See OSmoothPath::Path().
        QVector<QPointF> casted;
        casted.reserve(points.count());
        for (const T &point : points)
        {
            casted.append(point);
        }

        ekv = OSmoothPath::Path(casted, /*close=*/false);
        ekv.setFillRule(Qt::WindingFill);

#if !defined(V_NO_ASSERT)
        // uncomment for debug
//            QFont font;
//            font.setPixelSize(1);
//            for (qint32 i = 0; i < points.count(); ++i)
//            {
//                ekv.addEllipse(points.at(i).x()-accuracyPointOnLine, points.at(i).y()-accuracyPointOnLine,
//                               accuracyPointOnLine*2., accuracyPointOnLine*2.);
//                ekv.addText(points.at(i).x()-accuracyPointOnLine, points.at(i).y()-accuracyPointOnLine, font,
//                            QString::number(i+1));
//                if (i > 250)
//                {
//                    break;
//                }
//            }
#endif
    }

    return ekv;
}

#endif // VPIECE_H
