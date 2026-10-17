/************************************************************************
 **
 **  @file   vdxfengine.h
 **  @author Valentina Zhuravska <zhuravska19(at)gmail.com>
 **  @date   12 8, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#ifndef VDXFENGINE_H
#define VDXFENGINE_H

#include <QPaintEngine>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QtGlobal>
#include <string>

#include "../vmisc/def.h"
#include "../vmisc/defglobal.h"
#include "dxfdef.h"
#include "libdxfrw/drw_base.h"

class QTextStream;
class dx_iface;
class DRW_Text;
class VLayoutPiece;
class DRW_Entity;
class dx_ifaceBlock;
class VLayoutPoint;
class VLayoutPiecePath;
class DRW_Point;
class DRW_ASTMNotch;
struct VLayoutPassmark;
class DRW_ATTDEF;
class DRW_Circle;

class VDxfEngine final : public QPaintEngine
{
    friend class VDxfPaintDevice;

public:
    using QPaintEngine::drawEllipse;
    using QPaintEngine::drawLines;
    using QPaintEngine::drawPolygon;

    VDxfEngine();
    ~VDxfEngine() override;

    auto type() const -> Type override;

    auto begin(QPaintDevice *pdev) -> bool override;
    auto end() -> bool override;

    void updateState(const QPaintEngineState &state) override;

    void drawPath(const QPainterPath &path) override;
    void drawLines(const QLineF *lines, int lineCount) override;
    void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
    void drawEllipse(const QRectF &rect) override;
    void drawTextItem(const QPointF &p, const QTextItem &textItem) override;
    void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;

    auto GetSize() const -> QSize;
    void SetSize(const QSize &value);

    auto GetResolution() const -> double;
    void SetResolution(double value);

    auto GetFileName() const -> QString;
    void SetFileName(const QString &value);

    auto GetVersion() const -> DRW::Version;
    void SetVersion(DRW::Version version);

    void SetBinaryFormat(bool binary);
    auto IsBinaryFormat() const -> bool;

    auto GetPenStyle() -> std::string;
    auto GetPenColor() -> int;

    void SetMeasurement(const VarMeasurement &var);
    void SetInsunits(const VarInsunits &var);

    auto GetXScale() const -> qreal;
    void SetXScale(const qreal &xscale);

    auto GetYScale() const -> qreal;
    void SetYScale(const qreal &yscale);

    void SetBoundaryTogetherWithNotches(bool value);
    auto IsBoundaryTogetherWithNotches() const -> bool;

    auto DxfApparelCompatibility() const -> DXFApparelCompatibility;
    void SetDxfApparelCompatibility(DXFApparelCompatibility mode);

    /// @brief IsShowMainPath tells whether the pieces must be written as if their main path were visible,
    /// whatever the pattern says. The pieces are copied for the export, so neither the pattern nor the
    /// canvas is touched.
    auto IsShowMainPath() const -> bool;
    void SetShowMainPath(bool value);

    auto ErrorString() const -> QString;

private:
    Q_DISABLE_COPY_MOVE(VDxfEngine) // NOLINT

    QSize m_size{};
    double m_resolution{PrintDPI};
    QString m_fileName{};
    DRW::Version m_version{DRW::AC1009};
    bool m_binary{false};
    QTransform m_matrix{};
    QSharedPointer<dx_iface> m_input{};
    VarMeasurement m_varMeasurement{VarMeasurement::Metric};
    VarInsunits m_varInsunits{VarInsunits::Millimeters};
    DRW_Text *m_textBuffer{nullptr};
    qreal m_xscale{1};
    qreal m_yscale{1};
    bool m_togetherWithNotches{false};
    DXFApparelCompatibility m_compatibilityMode{DXFApparelCompatibility::STANDARD};
    bool m_showMainPath{false};

    Q_REQUIRED_RESULT auto FromPixel(double pix, const VarInsunits &unit) const -> double;
    Q_REQUIRED_RESULT auto ToPixel(double val, const VarInsunits &unit) const -> double;

    auto ExportToAAMA(const QVector<VLayoutPiece> &details) -> bool;
    void ExportAAMAOutline(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMADraw(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMADrawSewLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMADrawInternalPaths(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMADrawPlaceLabels(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMAIntcut(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMANotch(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMAGrainline(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMADrill(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportAAMADrawFoldLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);

    auto ExportToASTM(const QVector<VLayoutPiece> &details) -> bool;
    void ExportASTMPieceBoundary(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMSewLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMDrawInternalPaths(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMDrawPlaceLabels(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMInternalCutout(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMAnnotationText(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMDrill(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMNotches(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMMirrorLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);
    void ExportASTMDrawFoldLine(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail);

    void ExportAnnotationText(const QSharedPointer<dx_ifaceBlock> &detailBlock, const VLayoutPiece &detail,
                              const UTF8STRING &layer);

    Q_REQUIRED_RESULT auto ExportASTMNotch(const VLayoutPassmark &passmark) -> DRW_ASTMNotch *;
    Q_REQUIRED_RESULT auto ExportASTMNotchDataDependecy(const VLayoutPassmark &passmark, const UTF8STRING &notchLayer,
                                                        const VLayoutPiece &detail) -> DRW_ATTDEF *;

    /// @brief ExportTurnPoints writes the turn points (layer 2) of a shape made only of such points,
    /// like a fold line.
    void ExportTurnPoints(const QSharedPointer<dx_ifaceBlock> &detailBlock, const QVector<VLayoutPoint> &points) const;

    /// @brief ExportTurnAndCurvePoints writes the turn points (layer 2) and the curve points (layer 3) of
    /// a contour, in the order the contour runs. AAMA describes an edge by those points; interleaved, the
    /// reader knows which vertex of the edge each of them belongs to. Grouped by layer instead, the edge
    /// is re-fitted as a single curve and is deformed — the curves of a collar, typically.
    void ExportTurnAndCurvePoints(const QSharedPointer<dx_ifaceBlock> &detailBlock,
                                  const QVector<VLayoutPoint> &points) const;

    Q_REQUIRED_RESULT auto AAMAPolygon(const QVector<VLayoutPoint> &polygon, const UTF8STRING &layer, bool forceClosed)
        -> DRW_Entity *;
    Q_REQUIRED_RESULT auto AAMALine(const QLineF &line, const UTF8STRING &layer) -> DRW_Entity *;
    Q_REQUIRED_RESULT auto AAMAText(const QPointF &pos, const QString &text, const UTF8STRING &layer,
                                    qreal height = -1) -> DRW_Entity *;
    Q_REQUIRED_RESULT auto AAMAPoint(const QPointF &pos, const UTF8STRING &layer) const -> DRW_Point *;
    Q_REQUIRED_RESULT auto AAMACircle(const QPointF &pos, const UTF8STRING &layer, qreal radius) const -> DRW_Circle *;

    template <class P, class V, class C>
    Q_REQUIRED_RESULT auto CreateAAMAPolygon(const QVector<C> &polygon, const UTF8STRING &layer, bool forceClosed)
        -> P *;

    static auto NotchPrecedingPoint(const QVector<VLayoutPoint> &boundary, QPointF notchBase, QPointF &point) -> bool;

    void AAMADrawFoldLineTwoArrows(const QVector<QVector<QPointF>> &points,
                                   const QSharedPointer<dx_ifaceBlock> &detailBlock);

    void ASTMDrawFoldLineTwoArrows(const QVector<QVector<QPointF>> &points,
                                   const QSharedPointer<dx_ifaceBlock> &detailBlock);

    Q_REQUIRED_RESULT auto IsClo3DCompatibility() const -> bool;

    /// @brief InternalPathPoints returns the points of an internal path as the DXF must carry them.
    /// In the Clo3D/Style3D compatibility mode the continuation of the path to the cutting contour is
    /// dropped: a dart is then written with its own points only, so that its two branches end on the
    /// outline, which is what Style3D needs to recognise it. Elsewhere the continuation is kept, as
    /// other CAD systems expect a dart drawn through the seam allowance.
    Q_REQUIRED_RESULT auto InternalPathPoints(const VLayoutPiecePath &path) const -> QVector<VLayoutPoint>;

    /// @brief InternalPaths lists the internal paths of a piece as the DXF must carry them. In the
    /// Clo3D/Style3D compatibility mode a dart (an internal path whose ends are continued to the cutting
    /// contour) is split into its two branches: Style3D traces a dart from exactly two baselines sharing
    /// their apex vertex, and refuses a single polyline carrying both of them.
    Q_REQUIRED_RESULT auto InternalPaths(const VLayoutPiece &detail, bool cut) const -> QVector<VLayoutPiecePath>;

    /// @brief DeepestPointIndex returns the index of the point of a path the furthest from the line
    /// joining its two ends — the apex of a dart. Returns -1 when there is none.
    Q_REQUIRED_RESULT static auto DeepestPointIndex(const QVector<VLayoutPoint> &points) -> int;

    auto PieceOutline(const VLayoutPiece &detail) const -> QVector<VLayoutPoint>;

    /// @brief ContourWithCutPaths rend le contour de la pièce, **découpes fusionnées comprises**.
    ///
    /// En mode Clo3D/Style3D, un chemin « Cut on fabric » n'est pas un trait posé à l'intérieur de la
    /// pièce : c'est une **pince coupée** — le contour descend jusqu'à sa pointe et remonte, et le V
    /// est retiré de la pièce. Style3D ne sait lire une découpe que là : une entité à part, quel que
    /// soit son calque, reste une ligne interne — mesuré sur ses propres fichiers, puis sur un DXF
    /// fabriqué à la main par Sophie, qui est la référence. Les découpes ainsi fusionnées ne sont donc
    /// PAS réécrites à côté : voir @ref IsMergedInContour, que les écrivains de découpes interrogent
    /// pour les écarter.
    ///
    /// ⚠️ Le contour arrive **déjà** au bout de sa fabrique (déduplication, crans fusionnés s'il y a
    /// lieu) : la fusion se fait sur la polyligne finale, et c'est la même que le calque 1 et la
    /// couture écrivent tous les deux.
    Q_REQUIRED_RESULT auto ContourWithCutPaths(const VLayoutPiece &detail, QVector<VLayoutPoint> contour) const
        -> QVector<VLayoutPoint>;

    /// @brief IsMergedInContour dit si @p path est entré dans le contour — donc s'il ne doit PAS être
    /// écrit une seconde fois comme découpe interne. La condition est celle de la fusion, au mot près :
    /// une découpe (« Cut on fabric ») dont les deux extrémités tombent sur le contour de coupe.
    Q_REQUIRED_RESULT auto IsMergedInContour(const VLayoutPiece &detail, const VLayoutPiecePath &path) const -> bool;

    /// @brief IsClosedPath dit si les points d'une découpe forment une BOUCLE — c'est-à-dire un TROU.
    ///
    /// Le modèle n'a pas de drapeau « fermé » (`VLayoutPiecePath` ne porte que ses points, ses accroches
    /// au contour et son miroir) : c'est la géométrie qui le dit, et c'est ainsi que le patron l'a
    /// dessiné — le dernier point revient sur le premier.
    ///
    /// ⚠️ Un trou s'écrit **fermé** (drapeau 70 = 1) et sur le **calque 11**, sans quoi Style3D le lit
    /// comme une ligne interne. Mesuré sur son propre fichier de référence — voir ExportAAMAIntcut.
    Q_REQUIRED_RESULT static auto IsClosedPath(const QVector<VLayoutPoint> &points) -> bool;

    /// @brief SplicePathInContour fait entrer @p points dans @p contour : le contour quitte son tracé
    /// au premier point du chemin, le suit jusqu'à son dernier point, et reprend. L'arc court entre les
    /// deux points d'accroche — l'ouverture de la pince — est remplacé par le chemin lui-même.
    static auto SplicePathInContour(QVector<VLayoutPoint> &contour, const QVector<VLayoutPoint> &points) -> bool;
};

#endif // VDXFENGINE_H
