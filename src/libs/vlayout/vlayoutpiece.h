/************************************************************************
 **
 **  @file   vlayoutdetail.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   2 1, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#ifndef VLAYOUTDETAIL_H
#define VLAYOUTDETAIL_H

#include <QCoreApplication>
#include <QDate>
#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QSharedDataPointer>
#include <QString>
#include <QTypeInfo>
#include <QVector>
#include <QtGlobal>

#include "../vmisc/typedef.h"
#include "../vwidgets/vpiecegrainline.h"
#include "qpainterpath.h"
#include "vabstractpiece.h"

#include <optional>
using EdgeIndex = std::optional<int>;

class VLayoutPieceData;
class VLayoutPiecePath;
class QGraphicsItem;
class QGraphicsPathItem;
class VPiece;
class VAbstractPattern;
class VLayoutPoint;
class VFoldLine;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wsuggest-final-types")
QT_WARNING_DISABLE_GCC("-Wsuggest-final-methods")

class VLayoutPiece : public VAbstractPiece
{
    Q_DECLARE_TR_FUNCTIONS(VLayoutPiece) // NOLINT

public:
    VLayoutPiece();
    VLayoutPiece(const VLayoutPiece &detail);

    ~VLayoutPiece() override;

    auto operator=(const VLayoutPiece &detail) -> VLayoutPiece &;

    VLayoutPiece(VLayoutPiece &&detail) noexcept;
    auto operator=(VLayoutPiece &&detail) noexcept -> VLayoutPiece &;

    static auto Create(const VPiece &piece, vidtype id, const VContainer *pattern,
                       VAbstractPattern *doc = nullptr) -> VLayoutPiece;
    static auto ConvertPassmarks(const VPiece &piece, const VContainer *pattern) -> QVector<VLayoutPassmark>;

    auto GetUniqueID() const -> QString override;

    auto GetMappedContourPoints() const -> QVector<VLayoutPoint>;
    auto GetMappedFullContourPoints(bool togetherWithNotches = false,
                                    bool drawMode = false,
                                    bool layoutAllowance = false) const -> QVector<VLayoutPoint>;
    auto GetFullContourPoints(bool togetherWithNotches = false,
                              bool drawMode = false,
                              bool layoutAllowance = false) const -> QVector<VLayoutPoint>;
    auto GetContourPoints() const -> QVector<VLayoutPoint>;
    void SetContourPoints(const QVector<VLayoutPoint> &points, bool hideMainPath = false);

    auto GetMappedSeamAllowancePoints() const -> QVector<VLayoutPoint>;
    auto GetMappedFullSeamAllowancePoints(bool togetherWithNotches = false,
                                          bool drawMode = false,
                                          bool layoutAllowance = false) const -> QVector<VLayoutPoint>;
    auto GetFullSeamAllowancePoints(bool togetherWithNotches = false,
                                    bool drawMode = false,
                                    bool layoutAllowance = false) const -> QVector<VLayoutPoint>;
    auto GetSeamAllowancePoints() const -> QVector<VLayoutPoint>;
    void SetSeamAllowancePoints(const QVector<VLayoutPoint> &points, bool seamAllowance = true);

    auto GetMappedLayoutAllowancePoints() const -> QVector<QPointF>;
    auto GetLayoutAllowancePoints() const -> QVector<QPointF>;
    void SetLayoutAllowancePoints(bool togetherWithNotches);

    auto GetMappedExternalContourPoints() const -> QVector<VLayoutPoint>;
    auto GetExternalContourPoints() const -> QVector<VLayoutPoint>;

    auto GetMappedPassmarks() const -> QVector<VLayoutPassmark>;
    auto GetPassmarks() const -> QVector<VLayoutPassmark>;
    void SetPassmarks(const QVector<VLayoutPassmark> &passmarks);

    auto GetPlaceLabels() const -> QVector<VLayoutPlaceLabel>;
    void SetPlaceLabels(const QVector<VLayoutPlaceLabel> &labels);

    auto MappedInternalPathsForCut(bool cut) const -> QVector<VLayoutPiecePath>;
    auto GetInternalPaths() const -> QVector<VLayoutPiecePath>;
    void SetInternalPaths(const QVector<VLayoutPiecePath> &internalPaths);

    void SetGrainline(const VGrainlineData &geom, const VContainer *pattern);
    auto GetGrainline() const -> VPieceGrainline;
    auto GetGrainline() -> VPieceGrainline &;
    auto GetMappedGrainlineShape() const -> GrainlineShape;
    auto GetGrainlineShape() const -> GrainlineShape;
    auto GetMappedGrainlineMainLine() const -> QLineF;
    auto GetGrainlineMainLine() const -> QLineF;
    auto IsGrainlineEnabled() const -> bool;
    auto IsGrainlineVisible() const -> bool;

    auto GetMatrix() const -> QTransform;
    void SetMatrix(const QTransform &matrix);

    auto GetLayoutWidth() const -> qreal;
    void SetLayoutWidth(qreal value);

    auto GetQuantity() const -> quint16;
    void SetQuantity(quint16 value);

    auto GetId() const -> vidtype;
    void SetId(vidtype id);

    auto IsVerticallyFlipped() const -> bool;
    void SetVerticallyFlipped(bool value);

    auto IsHorizontallyFlipped() const -> bool;
    void SetHorizontallyFlipped(bool value);

    void SetGradationId(const QString &id);
    auto GetGradationId() const -> QString;

    auto GetXScale() const -> qreal;
    void SetXScale(qreal xs);

    auto GetYScale() const -> qreal;
    void SetYScale(qreal ys);

    auto GetMappedCorrectedMirrorLine(bool togetherWithNotches = false) const -> QLineF;
    auto GetCorrectedMirrorLine(bool togetherWithNotches = false) const -> QLineF;

    auto GetMappedSeamMirrorLine() const -> QLineF;
    auto GetSeamMirrorLine() const -> QLineF;
    void SetSeamMirrorLine(const QLineF &line);

    auto GetMappedSeamAllowanceMirrorLine() const -> QLineF;
    auto GetSeamAllowanceMirrorLine() const -> QLineF;
    void SetSeamAllowanceMirrorLine(const QLineF &line);

    auto GetFoldLineHeight() const -> qreal;
    void SetFoldLineHeight(qreal height);

    auto GetFoldLineWidth() const -> qreal;
    void SetFoldLineWidth(qreal width);

    auto GetFoldLineCenterPosition() const -> qreal;
    void SetFoldLineCenterPosition(qreal center);

    auto GetFoldLineOutlineFont() const -> QFont;
    void SetFoldLineOutlineFont(const QFont &font);


    /// Whether this drawing draws the fold-line caption. The mark — the line and its two arrows —
    /// is untouched: only the text leaves, and at the drawing rather than by hiding an item.
    /// @see VLayoutPieceData::m_showFoldLineLabel — not persisted, on purpose.
    auto IsShowFoldLineLabel() const -> bool;
    void SetShowFoldLineLabel(bool show);

    /**
     * @brief FoldLineCaption est le texte que la légende de pli dessine sur ce morceau.
     *
     * Le texte de la pièce quand elle en a un, et la légende générique résolue par l'application
     * autrement — celle-ci l'écrit dans les langues du cartouche, ce que cette bibliothèque ne sait
     * pas faire seule. Résolu une fois, à la construction du morceau : le rendu n'a pas à
     * redemander à l'application à chaque image.
     *
     * @see VLayoutPieceData::m_foldLineCaption — not persisted, on purpose: a layout file carries
     * the piece, never a rendering decision taken at one moment.
     */
    auto FoldLineCaption() const -> QString;
    void SetFoldLineCaption(const QString &caption);

    void Translate(const QPointF &p);
    void Translate(qreal dx, qreal dy);
    void Scale(qreal sx, qreal sy);
    void Rotate(const QPointF &originPoint, qreal degrees);
    void Mirror(const QLineF &edge);
    void Mirror();

    auto DetailEdgesCount() const -> vsizetype;
    auto LayoutEdgesCount() const -> vsizetype;

    auto LayoutEdge(int i) const -> QLineF;
    auto LayoutEdgeByPoint(const QPointF &p1) const -> EdgeIndex;

    auto MappedDetailBoundingRect() const -> QRectF;
    auto DetailBoundingRect() const -> QRectF;
    auto MappedLayoutBoundingRect() const -> QRectF;
    auto Diagonal() const -> qreal;

    auto FoldLine() const -> VFoldLine;

    static auto BoundingRect(QVector<QPointF> points) -> QRectF;

    static auto GrainlinePath(const GrainlineShape &shape) -> QPainterPath;

    auto isNull() const -> bool;
    auto Square() const -> qint64;

    auto MappedContourPath(bool togetherWithNotches, bool showLayoutAllowance) const -> QPainterPath;
    auto ContourPath(bool togetherWithNotches, bool showLayoutAllowance) const -> QPainterPath;
    auto MappedLayoutAllowancePath() const -> QPainterPath;

    void DrawMiniature(QPainter &painter, bool togetherWithNotches) const;

    /// @brief Build the piece as one graphics item.
    /// @param grabSurface when true the main item is an OHitShapePathItem, so the caller may give
    ///        it a surface wider than the path it draws. The Graded canvas cuts the contour into
    ///        open stretches for its trace pattern, which leaves no surface at all (OCornerTrace).
    Q_REQUIRED_RESULT auto GetItem(bool textAsPaths, bool togetherWithNotches, bool showLayoutAllowance,
                                   bool grabSurface = false) const -> QGraphicsItem *;

    auto IsLayoutAllowanceValid(bool togetherWithNotches) const -> bool;

    auto BiggestEdge() const -> qreal;


    auto MapPlaceLabelShape(PlaceLabelImg shape) const -> PlaceLabelImg;

    static auto MapPassmark(VLayoutPassmark passmark, const QTransform &matrix, bool mirror) -> VLayoutPassmark;

    auto LineMatrix(const QPointF &topLeft, qreal angle, const QPointF &linePos, qreal maxLineWidth) const
        -> QTransform;

protected:
    void SetGrainline(const VPieceGrainline &grainline);

private:
    QSharedDataPointer<VLayoutPieceData> d;

    auto DetailPath() const -> QVector<VLayoutPoint>;

    Q_REQUIRED_RESULT auto GetMainItem(bool togetherWithNotches, bool showLayoutAllowance,
                                       bool grabSurface = false) const -> QGraphicsPathItem *;
    Q_REQUIRED_RESULT auto GetExternalContourPathItem() const -> QGraphicsPathItem *;

    void CreateGrainlineItem(QGraphicsItem *parent) const;
    void CreateFoldLineItem(QGraphicsItem *parent, bool textAsPaths) const;

    template <class T> auto Map(QVector<T> points) const -> QVector<T>;
    auto Map(const GrainlineShape &shape) const -> GrainlineShape;

    auto Edge(const QVector<QPointF> &path, int i) const -> QLineF;
    auto EdgeByPoint(const QVector<QPointF> &path, const QPointF &p1) const -> EdgeIndex;

    auto CorrectSeamAllowanceMirrorLine() const -> QLineF;
};

QT_WARNING_POP

Q_DECLARE_TYPEINFO(VLayoutPiece, Q_MOVABLE_TYPE); // NOLINT

#endif // VLAYOUTDETAIL_H
