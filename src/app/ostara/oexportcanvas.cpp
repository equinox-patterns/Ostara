/************************************************************************
 **
 **  @file   oexportcanvas.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — draw the bin and the nested content.
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

#include "oexportcanvas.h"

#include <QFontMetricsF>

#include "core/ogratedsizestyles.h"
#include "core/oexporttemplate.h"
#include "mainwindow_graded_helpers.h"
#include "oexportmarkpainter.h"
#include "ogradedgeometry.h"
#include "ogradedsizetrace.h"
#include "oexportcoverpage.h"
#include "oexportgeometry.h"
#include "oexportfootprint.h"
#include "oexportnesting.h"
#include "oexportsheets.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/ipieceinfo.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockcontent.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/ocartouchepainter.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vpiecegrainline.h"

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QHash>
#include <QPainter>
#include <QPen>
#include <QScopedPointer>
#include <QTransform>
#include <QtMath>
#include <QtNumeric>

namespace
{
// The four bands of the scene, named once. A piece carries no Z of its own and sits at 0;
// the paper goes under everything, and the cut lines over everything — because a cut line
// hidden under a cartouche is a cut line the user cannot cut along.
constexpr qreal kPaperZ = -2.0;
constexpr qreal kBinZ = -1.0;

/// The assembly marks — the template's frame, scissors, marks and pagination — sit between the
/// paper and the pieces, and that band is not a matter of taste: they are the plan's own ground,
/// and the export writes them at that level too (§11.9). A mark drawn over a piece would be a
/// mark printed on the fabric.
constexpr qreal kAssemblyMarksZ = -0.5;

/// The cover page — a page's own ink, and it sits over the marks and under the plan.
///
/// **Over the marks, and that is not taste.** The pagination of a tiled run puts numbers near the
/// sheet's corners, which is exactly where an in-sheet cover block is; a page number written across
/// the cover would be the plan writing on someone else's paper.
///
/// **Under the pieces.** In the PDF the cover page is written in the UNDERLAY, under every layer —
/// so a piece a hand has left on the block is drawn over it there, and the canvas must show the
/// same thing. A cover drawn on top would make the screen a promise the file does not keep.
constexpr qreal kCoverZ = -0.25;

constexpr qreal kDecorZ = 1.0;
constexpr qreal kGridZ = 2.0;

/// The sheets a tiled plan is cut into, drawn as the lines it is cut along.
///
/// One item for the whole grid rather than one rectangle per sheet: the sheets butt edge to
/// edge, so consecutive rectangles would draw every internal line twice — and a single item
/// is also the one object a writer can be handed the day the PDF cuts its pages.
///
/// It is INERT, and that is not decoration. A QGraphicsRectItem answers the mouse and takes
/// the hover by default; in this scene the hover is what raises an item above its neighbours
/// (VMainGraphicsScene::ItemToRaise) and what the click dispatch reads. A grid left answering
/// would be taken for a piece of the pattern, and the clicks meant for one would land on the
/// paper.
class ArtboardGridItem : public QGraphicsItem
{
public:
    ArtboardGridItem(const QRectF &planRect, qreal cellWidthPx, qreal cellHeightPx, int columns, int rows,
                     const OExportSheets::Set &sheets)
      : m_planRect(planRect), m_cellWidthPx(cellWidthPx), m_cellHeightPx(cellHeightPx),
        m_columns(columns), m_rows(rows), m_sheets(sheets)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setAcceptedMouseButtons(Qt::NoButton);
        setZValue(kGridZ);
    }

    auto boundingRect() const -> QRectF override
    {
        // The plan's outer edge is drawn ON the rect, so half the pen falls outside it —
        // rounded up to the whole of it rather than left to a repaint that never comes.
        return m_planRect.adjusted(-kPenWidth, -kPenWidth, kPenWidth, kPenWidth);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setPen(QPen(Qt::black, kPenWidth));

        // Nothing missing: the rectangle and its seams, drawn exactly as they always have been.
        // The outline goes through drawRect, which mitres its four corners, where four separate
        // lines would each poke half a pen past them. A whole plan must not move by a pixel.
        if (m_sheets.count == m_columns * m_rows)
        {
            painter->drawRect(m_planRect);

            for (int i = 1; i < m_columns; ++i)
            {
                const qreal x = m_planRect.left() + i * m_cellWidthPx;
                painter->drawLine(QPointF(x, m_planRect.top()), QPointF(x, m_planRect.bottom()));
            }

            for (int j = 1; j < m_rows; ++j)
            {
                const qreal y = m_planRect.top() + j * m_cellHeightPx;
                painter->drawLine(QPointF(m_planRect.left(), y), QPointF(m_planRect.right(), y));
            }

            return;
        }

        // A sheet was dropped, so the lines stop where the paper does. Each boundary is drawn in
        // the stretches that have a printed sheet beside them, and drawn ONCE — a walk over the
        // sheets would draw every seam from both sides, and a seam drawn twice comes out heavier
        // wherever the view antialiases.
        for (int i = 0; i <= m_columns; ++i)
        {
            const qreal x = m_planRect.left() + i * m_cellWidthPx;

            for (const OExportSheets::Run &run : m_sheets.VerticalEdges(i))
            {
                painter->drawLine(QPointF(x, m_planRect.top() + run.first * m_cellHeightPx),
                                  QPointF(x, m_planRect.top() + (run.last + 1) * m_cellHeightPx));
            }
        }

        for (int j = 0; j <= m_rows; ++j)
        {
            const qreal y = m_planRect.top() + j * m_cellHeightPx;

            for (const OExportSheets::Run &run : m_sheets.HorizontalEdges(j))
            {
                painter->drawLine(QPointF(m_planRect.left() + run.first * m_cellWidthPx, y),
                                  QPointF(m_planRect.left() + (run.last + 1) * m_cellWidthPx, y));
            }
        }
    }

private:
    /// The bin's own pen, so a cut line and the plan's border read as one drawing.
    static constexpr qreal kPenWidth = 1.0;

    QRectF m_planRect;
    qreal m_cellWidthPx{0.0};
    qreal m_cellHeightPx{0.0};
    int m_columns{0};
    int m_rows{0};
    OExportSheets::Set m_sheets;
};

/// The assembly marks of a template, drawn on the plan's own ground.
///
/// One item for the whole plan, and drawn from the tiling as a whole: the frame is the grid's
/// own lines, each drawn once, and a mark sits where sheets meet rather than on each of them.
/// Drawn sheet by sheet, the dash pattern would restart at every edge and every mark would be
/// doubled — the two defects the preview was cured of (§11.2), and the export would print them.
///
/// It OWNS the painter rather than borrowing one. A rebuild tears the scene down and builds it
/// again, so a borrowed painter would have to outlive the item that draws with it and something
/// outside the scene would have to hold it; five SVG files parsed per rebuild cost nothing,
/// while a painter rebuilt per repaint would cost plenty.
///
/// INERT, for the reason ArtboardGridItem is: the hover raises whatever it lands on, and a layer
/// of marks answering the mouse would be taken for a piece of the pattern.
class AssemblyMarksItem : public QGraphicsItem
{
public:
    AssemblyMarksItem(const OExportTemplate &marks, const QPointF &origin,
                      const OExportMarkGrid &grid, bool framed)
      : m_grid(grid), m_origin(origin), m_framed(framed)
    {
        m_decor.SetFrameSettings(marks.frame);
        m_decor.SetMarkSettings(marks.marks);
        m_decor.SetPaginationSettings(marks.pagination);

        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setAcceptedMouseButtons(Qt::NoButton);
        setZValue(kAssemblyMarksZ);
    }

    auto boundingRect() const -> QRectF override
    {
        // The plan's outer edge is drawn ON the grid, so half the frame's pen falls outside it.
        const QRectF plan(m_origin, QSizeF(m_grid.columns * m_grid.sheetWidthPx,
                                           m_grid.rows * m_grid.sheetHeightPx));
        return plan.adjusted(-kPenWidth, -kPenWidth, kPenWidth, kPenWidth);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        // No stand-in outline is asked for: this scene draws its own cut lines, and it stands
        // them down itself wherever the frame is drawn. Two lines in the same place — one of
        // them the user's, at the opacity she set — is the defect the preview was cured of.
        m_decor.Paint(*painter, m_origin, m_grid, m_framed, false);
    }

private:
    /// The frame's own pen, so the plan's outer edge is never clipped by a tight bounding rect.
    static constexpr qreal kPenWidth = 1.0;

    OExportMarkPainter m_decor;
    OExportMarkGrid m_grid;
    QPointF m_origin;
    bool m_framed{false};
};

/// The cover page, drawn at the size it will be printed.
///
/// **It resolves nothing.** The bands arrive already resolved by OExportCoverPage::Of, which read
/// the run, the document and the template — the canvas places and paints, and looking a template up
/// here would be a second place deciding what is drawn.
///
/// INERT, for the reason AssemblyMarksItem and ArtboardGridItem are: the hover raises whatever it
/// lands on, and a page answering the mouse would be taken for a piece of the pattern.
class CoverPageItem : public QGraphicsItem
{
public:
    CoverPageItem(const QRectF &page, const QMap<OCoverPageTemplate::Placement,
                                                 QVector<OCartoucheRenderLine>> &bands)
      : m_page(page), m_bands(bands)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setAcceptedMouseButtons(Qt::NoButton);
        setZValue(kCoverZ);
    }

    auto boundingRect() const -> QRectF override { return m_page; }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        // **Scale 1 — the page is drawn at the size it will be printed**, which is what makes the
        // canvas evidence about the paper rather than a picture of it (nomenclature spec §6, and
        // Sophie's decision of 18-09-2026: a logo of two centimetres is two centimetres on a page
        // of nineteen, on screen as on paper).
        OCoverPageRender::Paint(*painter, m_page, m_bands, 1.0);
    }

private:
    QRectF m_page;
    QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> m_bands;
};

/// The paper of a tiled run: the printed sheets, white, and nothing drawn on them.
///
/// ONE RECTANGLE PER PRINTED SHEET, and not one over the plan. A cell with nothing on it is not
/// printed, and white paper drawn there would be the canvas promising a page the PDF will never
/// write — which is the one thing the canvas must not do. The sheets butt edge to edge, so they
/// meet without overlapping and no seam between two of them is drawn twice.
///
/// The pen is dropped on purpose. A QGraphicsRectItem carries a black one-pixel pen by
/// default, and ArtboardGridItem already draws the plan's outer edge — the two would land on
/// the same line, one drawn twice.
///
/// A rect is a sheet's own and nothing wider: a carrier the scene can see but the eye cannot is
/// what feeds VisibleItemsBoundingRect, and a rect wider than the paper would leave the plan
/// pinned to one edge of the viewport when the result is framed. MakeDecorHost tells the same
/// story from the other end.
void MakePaper(QGraphicsScene *scene, const QRectF &rect)
{
    auto *paper = new QGraphicsRectItem(rect);
    paper->setPen(Qt::NoPen);
    paper->setBrush(QBrush(Qt::white));
    paper->setFlag(QGraphicsItem::ItemIsSelectable, false);
    paper->setFlag(QGraphicsItem::ItemIsMovable, false);
    paper->setAcceptedMouseButtons(Qt::NoButton);
    paper->setZValue(kPaperZ);
    scene->addItem(paper);
}

void MakeArtboardPaper(QGraphicsScene *scene, const OExportArtboards::Grid &artboards,
                       const OExportSheets::Set &sheets)
{
    for (int row = 0; row < artboards.rows; ++row)
    {
        for (int col = 0; col < artboards.columns; ++col)
        {
            if (!sheets.IsPresent(row, col))
            {
                continue;
            }

            // A cell is a sheet, and a sheet is what MakePaper draws. The cover page's extra sheet
            // is the same call with a rectangle that is NOT a cell — which is exactly how it gets
            // paper without ever entering OExportSheets.
            MakePaper(scene, QRectF(artboards.planRect.left() + col * artboards.cellWidthPx,
                                    artboards.planRect.top() + row * artboards.cellHeightPx,
                                    artboards.cellWidthPx, artboards.cellHeightPx));
        }
    }
}

/// Grainline, fold mark and its label are drawn by GetItem on every size, and only the
/// smallest one may show them — that is the rule the Graded canvas applies to its ghost
/// layers, and the layers of a range are no different. Same three types, same hiding.
void HidePerSizeDecor(QGraphicsItem *item)
{
    if (item == nullptr)
    {
        return;
    }

    const QList<QGraphicsItem *> children = item->childItems();
    for (QGraphicsItem *child : children)
    {
        if (child->type() == VGraphicsFillItem::Type || child->type() == VGraphicsFoldLineItem::Type
            || child->type() == QGraphicsSimpleTextItem::Type)
        {
            child->hide();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// The range's grainline override for one owner, or an invalid one when the range holds
/// none. The mode reads the range and never writes to it: an override exists only for an
/// owner the user actually moved in Graded, and without one the owner's own geometry stands.
auto GrainlineOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup)
    -> OSizerange::GrainlineOverride
{
    if (range == nullptr)
    {
        return {};
    }

    if (isGroup)
    {
        const auto it = range->groupContent.constFind(ownerId);
        return it == range->groupContent.cend() ? OSizerange::GrainlineOverride{} : it.value().grainline;
    }

    const auto it = range->pieceContent.constFind(ownerId);
    return it == range->pieceContent.cend() ? OSizerange::GrainlineOverride{} : it.value().grainline;
}

//---------------------------------------------------------------------------------------------------------------------
/// The range's override for one information block of one owner, or an invalid one. Same
/// rule as the grainline: an override exists only for a block the user actually moved in
/// Graded, and this mode reads it without ever writing one.
auto BlockOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup, int block)
    -> OSizerange::BlockOverride
{
    if (range == nullptr)
    {
        return {};
    }

    if (isGroup)
    {
        const auto it = range->groupContent.constFind(ownerId);
        if (it == range->groupContent.cend())
        {
            return {};
        }
        const auto ov = it.value().blocks.constFind(block);
        return ov == it.value().blocks.cend() ? OSizerange::BlockOverride{} : ov.value();
    }

    const auto it = range->pieceContent.constFind(ownerId);
    if (it == range->pieceContent.cend())
    {
        return {};
    }
    const auto ov = it.value().blocks.constFind(block);
    return ov == it.value().blocks.cend() ? OSizerange::BlockOverride{} : ov.value();
}

//---------------------------------------------------------------------------------------------------------------------
/// The range's override for one annotation of one owner, or an invalid one.
auto AnnotationOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup, int index)
    -> OSizerange::AnnotationOverride
{
    if (range == nullptr)
    {
        return {};
    }

    if (isGroup)
    {
        const auto it = range->groupContent.constFind(ownerId);
        if (it == range->groupContent.cend())
        {
            return {};
        }
        const auto ov = it.value().annotations.constFind(index);
        return ov == it.value().annotations.cend() ? OSizerange::AnnotationOverride{} : ov.value();
    }

    const auto it = range->pieceContent.constFind(ownerId);
    if (it == range->pieceContent.cend())
    {
        return {};
    }
    const auto ov = it.value().annotations.constFind(index);
    return ov == it.value().annotations.cend() ? OSizerange::AnnotationOverride{} : ov.value();
}

//---------------------------------------------------------------------------------------------------------------------
/// Whether a piece belongs to a group that is nested as one rigid block. Below two
/// members a group is not a block — the canvas, the export and the container all draw
/// that line in the same place (OGraded::kMinGroupMembers).
auto IsGroupedMember(const VContainer *pattern, quint32 groupId, bool cuttingPlan) -> bool
{
    // ⚠️ **Un plan de coupe DÉCOMPOSE les groupes.** Le membre gardé n'est plus un membre, c'est la
    // pièce qu'on taille — mais il conserve son `groupId`, donc sans cette ligne le canvas
    // continuerait de le priver de son décor et de dessiner celui du groupe autour de lui. Le
    // membre porterait le cartouche de son groupe, avec ses annotations et sa grainline, alors que
    // c'est une autre pièce qui est coupée.
    //
    // Le groupe n'est pas touché pour autant : `group.m_glGrainline` reste intact, et le droit-fil
    // du nesting est résolu bien plus tôt, dans OExportGeometry — le repli sur la grainline du
    // groupe continue donc de servir au moteur, simplement il ne se DESSINE plus.
    if (cuttingPlan)
    {
        return false;
    }

    if (groupId == 0 || pattern == nullptr)
    {
        return false;
    }

    const auto *groups = pattern->DataPieceGroups();
    if (groups == nullptr)
    {
        return false;
    }

    const auto it = groups->constFind(groupId);
    return it != groups->cend() && it.value().pieceIds.size() >= OGraded::kMinGroupMembers;
}

//---------------------------------------------------------------------------------------------------------------------
/// Give the reference layer's piece the grainline the range asks for.
///
/// GetItem builds that grainline from the geometry the piece carries, which is the
/// Details-mode placement — clamped to the piece, and blind to the range. The range is
/// the authority here: its override carries the origin and the length, and the piece's
/// own rotation formula still supplies the angle, since a range stores none.
void ApplyReferenceGrainline(VLayoutPiece &placed, const OExportPieceGeometry &geometry,
                             const OSizerange *range, const Unit &unit, const VContainer *pattern,
                             bool cuttingPlan)
{
    VPieceGrainline &grainline = placed.GetGrainline();

    if (IsGroupedMember(pattern, geometry.groupId, cuttingPlan))
    {
        // A grouped member carries no grainline of its own: the group hosts one for all
        // of them, exactly as Details and Graded do it.
        grainline.SetEnabled(false);
        return;
    }

    // Nothing is drawn for this piece — it carries no grainline, or the one it carries
    // was already refused when the piece was built. A range stores neither an angle nor
    // an arrow type, so it cannot put one back: leaving it alone is the whole answer.
    if (!grainline.IsEnabled() || !grainline.IsVisible())
    {
        return;
    }

    // ⚠️ **The frozen line wins.** It was resolved at generation, while the document was still on the
    // reference layer's size. The live resolution below reads the document, so it would draw whatever
    // size the document is on NOW — which the run no longer guarantees, precisely because the document
    // is now given back at the end of it.
    if (geometry.grainline.resolved)
    {
        grainline.SetMainLine(geometry.grainline.mainLine);
        return;
    }

    const auto *pieces = pattern != nullptr ? pattern->DataPieces() : nullptr;
    if (pieces == nullptr)
    {
        return;
    }

    const auto piece = pieces->constFind(geometry.id);
    if (piece == pieces->cend())
    {
        return;
    }

    const VGrainlineData &geom = piece.value().GetGrainlineGeometry();

    // The line GetItem built ignores the range, but its start point is the Details
    // placement — the fallback for a grainline that was never placed, so it does not land
    // on the document origin.
    const QPointF fallback = grainline.GetMainLine().p1();

    const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, geometry.id, false);
    const OGraded::GrainlinePlacement placement =
        OGraded::ResolveGrainline(geom, ov, false, pattern, unit, fallback);
    const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), false, pattern);

    // The piece hands its grainline out, and the range only ever moves the origin and the
    // length: the angle stays the piece's own formula, which this same call evaluated.
    const VPieceGrainline moved(placement.pos, placement.lengthPx, qDegreesToRadians(rotation),
                                geom.GetArrowType());
    grainline.SetMainLine(moved.GetMainLine());
}

//---------------------------------------------------------------------------------------------------------------------
/// The extent a piece is drawn with, measured before the placement is applied.
///
/// Read from an item built exactly like the one the canvas draws — same pen, same
/// notches — because that is the rectangle these frames have always been measured on.
/// Taking the contour points instead would drop the pen, and move every child of a group
/// by half its width.
auto UnplacedBounds(const VLayoutPiece &piece) -> QRectF
{
    QScopedPointer<QGraphicsItem> probe(piece.GetItem(false, true, false));
    return probe.isNull() ? QRectF() : probe->boundingRect();
}

/// @brief The frame an owner's children are stored in, and the placement that brings it to
///        the scene.
struct DecorFrame
{
    QTransform placement; ///< the owner's frame -> the scene
    qreal rotation{0.0};  ///< the angle inside @c placement, in degrees
    QPointF origin{};     ///< the point of the owner's frame that is its local (0, 0)
    QRectF bounds{};      ///< the owner's extent, in its own frame

    /// The vertical line the owner's children must be mirrored about, in the frame those children
    /// store their coordinates in — or NaN when the owner is not reflected.
    ///
    /// It cannot be folded into @c placement, tempting as that is: the carrier's transform reaches
    /// the cartouche's TEXT along with its box, and a cartouche is read, not looked at. The children
    /// are therefore re-laid one by one, and this is the line they are re-laid about.
    qreal mirrorX{qQNaN()};

    /// Which placement of the result this owner is — what its carrier advertises, so that a
    /// click on the decor reaches the footprint it decorates. -1 for a frame with no
    /// placement behind it, and the carrier then carries no role at all.
    int footprintIndex{-1};
};

//---------------------------------------------------------------------------------------------------------------------
/// The invisible carrier an owner's children hang under.
///
/// It carries the placement the owner itself received, so a child hung at the coordinates
/// the owner stores lands where the owner is: @c host.map(p) equals
/// @c frame.placement.map(frame.origin + p). Without it the carrier would stay
/// axis-aligned on the bounding box of the placed members, which stops being the owner's
/// frame the moment the nesting turns it — and a group is a footprint like any other, so
/// the engine may turn it by any multiple of two degrees.
auto MakeDecorHost(QGraphicsScene *scene, const DecorFrame &frame) -> QGraphicsItem *
{
    // The carrier's rect expressed in its OWN frame — which is frame.bounds brought back by
    // frame.origin, since origin is the point of the owner's frame that is its local (0, 0).
    //
    // Writing (0, 0, w, h) instead assumes that point IS bounds.topLeft(), which holds for a
    // group and not for a piece: a piece's children are stored in the document's coordinates,
    // so its origin is the document origin and its bounds sit elsewhere. The rect then
    // sprawled from the document origin instead of covering the piece — and a carrier is
    // invisible (no pen, no brush) but still VISIBLE to the scene, so it fed
    // VisibleItemsBoundingRect() a rectangle somewhere off the plan. ZoomFitBest frames that
    // union: the zoom came out right for the plan and the framing did not, leaving the plan
    // pinned to one edge of the viewport.
    auto *host = new QGraphicsRectItem(frame.bounds.translated(-frame.origin));
    host->setPos(frame.placement.map(frame.origin));
    host->setRotation(frame.rotation);
    host->setPen(Qt::NoPen);
    host->setBrush(Qt::NoBrush);
    host->setFlag(QGraphicsItem::ItemIsSelectable, false);
    host->setFlag(QGraphicsItem::ItemIsMovable, false);
    host->setAcceptedMouseButtons(Qt::NoButton);
    host->setZValue(kDecorZ);

    // The carrier speaks for its owner: a click lands on a block or an annotation, never on
    // the invisible rectangle, so the footprint has to be reachable from up here.
    if (frame.footprintIndex >= 0)
    {
        host->setData(kExportFootprintRole, frame.footprintIndex);
    }

    scene->addItem(host);
    return host;
}

//---------------------------------------------------------------------------------------------------------------------
/// The mode shows the decor, it never edits it. Both item classes draw their eight resize
/// handles under `m_active` alone — SetReadOnly only drops the movable flag — so an item
/// left active would still paint them and answer the mouse. Manual correction is a subject
/// of its own, with its own spec.
///
/// The cursor is the third promise, and neither of the two lines above takes it back: both
/// constructors set Qt::SizeAllCursor, and Qt applies an item's cursor whenever the mouse is
/// over it, whether or not the item accepts hover events. Left alone, a cartouche advertised
/// a drag it could not do.
template <typename DecorItem> void MakeDecorReadOnly(DecorItem *item)
{
    item->SetActive(false);
    item->SetReadOnly(true);
    item->setAcceptedMouseButtons(Qt::NoButton);
    item->unsetCursor();
    item->show();
}

//---------------------------------------------------------------------------------------------------------------------
/// A decor child re-laid so that it appears MIRRORED about a vertical line: where its box starts, and
/// the angle it is recorded at.
struct MirroredDecor
{
    QPointF topLeft;
    qreal rotation{0.0};
};

/// @brief Mirror one decor child about the vertical line @p axisX — its box and its angle together.
///
/// ⚠️ **Negating the position and the angle is not enough, and the near miss looks right.** The
/// child's position is the box's CORNER — both item classes set their rect at (0, 0) from it — and Qt
/// turns an item about its transform origin, which defaults to that very corner. A reflection
/// reverses the box's own x direction, so the child's new local (0, 0) has to land where its TOP-RIGHT
/// corner was. Without that term the box slides by one rotated width, and a cartouche a width off
/// still reads as "roughly in place" — which is exactly why this is derived rather than eyeballed.
///
/// ⚠️ The angle is only NEGATED, never a negative scale: the child is re-laid, not reflected through
/// a transform, so the glyphs it holds stay upright. A mirrored cartouche is an unreadable one.
///
/// @param rotation the child's RECORDED angle — the one both item classes turn by its negation, the
///                 same reading the SVG writer takes (@c rot.rotate(-block.rotation))
auto MirrorDecor(const QPointF &topLeft, const QSizeF &size, qreal rotation, qreal axisX)
    -> MirroredDecor
{
    QTransform turn;
    turn.rotate(-rotation); // the child's own rotation, as it will be applied to it

    // Where the box's top-right corner lands in the carrier's frame — the point that becomes the new
    // top-left once the box is reflected.
    const QPointF topRight = topLeft + turn.map(QPointF(size.width(), 0.0));
    return {QPointF(2.0 * axisX - topRight.x(), topRight.y()), -rotation};
}

//---------------------------------------------------------------------------------------------------------------------
/// Add an owner's information blocks and annotations under @p host, in @p host's frame.
///
/// The owner is the group when the piece belongs to one — a grouped member carries none of
/// these, exactly as it carries no grainline of its own. Details, the Graded canvas and its
/// SVG export all draw that same line.
///
/// @param centerLocal the point a child that was never placed falls back on, in that frame
/// @param mirrorX the vertical line the children are mirrored about, or NaN when the owner is not
///                reflected — see @ref DecorFrame::mirrorX
auto ResolveOwnerDecor(const IPieceInfo &owner, bool isGroup, quint32 ownerId,
                       const QPointF &centerLocal, qreal mirrorX, const OSizerange *range,
                       const VContainer *pattern, VAbstractPattern *doc) -> OExportPieceDecor
{
    OExportPieceDecor decor;
    if (pattern == nullptr)
    {
        return decor;
    }

    decor.resolved = true;

    // A block exists when the template dispatches a section to it and that section has
    // content: the resolver leaves nothing else in blockContent. Without a document there
    // is no template to resolve, so the blocks are skipped and the annotations are not.
    if (doc != nullptr)
    {
        const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(owner, doc, pattern);
        const OInfoBlockData &info = owner.GetInfoBlockData();

        for (int block : state.blockContent.keys())
        {
            const QVector<OCartoucheRenderLine> lines = state.blockContent.value(block).toVector();
            if (lines.isEmpty())
            {
                continue;
            }

            const OSizerange::BlockOverride ov = BlockOverrideOf(range, ownerId, isGroup, block);
            const OGraded::InfoBlockPlacement placement =
                OGraded::ResolveInfoBlock(info, ov, lines, block, centerLocal);

            OInfoBlockGeom geom;
            geom.pos = placement.topLeft;
            geom.width = placement.size.width();
            geom.height = placement.size.height();
            geom.rotation = placement.rotation;
            geom.fontScale = placement.fontScale;

            // The piece this block belongs to may be reflected — the folded twin, `Force flipping`,
            // a symmetrical copy. The block is hung in the DOCUMENT frame, which no reflection
            // reaches, so it is re-laid here or it stays on the wrong side of the piece.
            if (qIsFinite(mirrorX))
            {
                const MirroredDecor mirror = MirrorDecor(geom.pos, placement.size, geom.rotation, mirrorX);
                geom.pos = mirror.topLeft;
                geom.rotation = mirror.rotation;
            }

            // The same four values the drawing will be given, in the record form the SVG writer
            // takes. Read off `geom` rather than off an item: an item resolves its own layout, and
            // asking it back would be asking a drawing for the numbers it was drawn from. Reading
            // `geom` is also what carries the mirror into the record — screen and paper are given
            // one set of numbers, never two.
            OInfoBlockSvg::Block record;
            record.index = block;
            record.lines = lines;
            record.fontScale = placement.fontScale;
            record.rect = QRectF(geom.pos, placement.size);
            record.rotation = geom.rotation;
            decor.blocks.append(record);
        }
    }

    const QList<OAnnotation> &annotations = owner.GetAnnotationData().Annotations();
    const OAnnotationStyleResolver &styleOf = VToolSeamAllowance::AnnotationStyleResolver();

    for (int i = 0; i < annotations.size(); ++i)
    {
        const OAnnotation &annotation = annotations.at(i);
        if (annotation.text.trimmed().isEmpty())
        {
            continue;
        }

        OAnnotationGeom geom = annotation.geom;
        const OSizerange::AnnotationOverride ov = AnnotationOverrideOf(range, ownerId, isGroup, i);
        if (ov.valid)
        {
            geom = ov.geom;
        }
        // (0, 0) is the sentinel for an annotation that was never moved: it sits on its
        // owner. The rotation never comes from the range — a range does not store one.
        if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
        {
            geom.pos = centerLocal;
        }

        const OAnnotationRenderStyle style = styleOf ? styleOf(annotation.templateName)
                                                     : OAnnotationRenderStyle();
        const qreal rotation = OGraded::AnnotationRotation(annotation.rotationFormula, isGroup, pattern);

        // The box the item will draw, computed BEFORE it exists: the mirror needs its width, and the
        // record needs the same numbers. An auto-sized box takes its size from the natural size of
        // its text — the arithmetic OGradedDecor does for the graded SVG. It is written once here for
        // all three, because if they ever disagree, a cartouche would sit differently on screen and
        // on paper, which is the one thing this whole path exists to prevent.
        QFont font;
        if (!style.fontFamily.isEmpty())
        {
            font.setFamily(style.fontFamily);
        }
        if (style.fontWeight != 0)
        {
            font.setWeight(static_cast<QFont::Weight>(style.fontWeight));
        }
        if (style.fontSize > 0)
        {
            font.setPixelSize(style.fontSize);
        }
        const QRectF natural = QFontMetricsF(font).boundingRect(
            QRectF(0.0, 0.0, 0.0, 0.0), Qt::AlignLeft, annotation.text);

        const qreal fontScale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
        const qreal contentW = natural.width() * fontScale;
        const qreal contentH = natural.height() * fontScale;
        const qreal w = qFuzzyIsNull(geom.boxWidth) ? contentW : qMax(geom.boxWidth, contentW);
        const qreal h = qFuzzyIsNull(geom.boxHeight) ? contentH : qMax(geom.boxHeight, contentH);

        // Same reflection as the information block, and for the same reason: an annotation is hung
        // in the document frame too, which the piece's own mirror never reaches.
        qreal drawnRotation = rotation;
        if (qIsFinite(mirrorX))
        {
            const MirroredDecor mirror = MirrorDecor(geom.pos, QSizeF(w, h), rotation, mirrorX);
            geom.pos = mirror.topLeft;
            drawnRotation = mirror.rotation;
        }

        OInfoBlockSvg::Annotation record;
        record.index = i;
        record.text = annotation.text;
        record.fontFamily = style.fontFamily;
        record.fontWeight = style.fontWeight;
        record.fontSizePx = style.fontSize;
        record.textColor = style.textColor.isEmpty() ? QStringLiteral("#000000") : style.textColor;
        record.backgroundColor = style.backgroundColor;
        record.fontScale = fontScale;
        record.rotation = drawnRotation;
        record.rect = QRectF(geom.pos, QSizeF(w, h));
        decor.annotations.append(record);
    }

    return decor;
}

//---------------------------------------------------------------------------------------------------------------------
/// Draw an owner's RESOLVED decor under @p host — from the records alone.
///
/// No document, no container, no range, no formula: everything this needs was decided when the records
/// were made. That is what lets the same records serve the live canvas, a stored output, and the SVG
/// writer, with no third reading of the truth to disagree with.
///
/// ⚠️ The annotation's box is handed back as its stored width and height rather than as "auto" (the
/// 0 sentinel of @ref OAnnotationGeom): the record holds the box already resolved, and the item takes
/// @c max(box, natural) — which is the box again, so the drawing is identical. What is lost is the
/// ability of this item to follow a later text change, and nothing here edits text.
void DrawOwnerDecor(QGraphicsItem *host, const OExportPieceDecor &decor)
{
    if (host == nullptr)
    {
        return;
    }

    for (const OInfoBlockSvg::Block &record : decor.blocks)
    {
        OInfoBlockGeom geom;
        geom.pos = record.rect.topLeft();
        geom.width = record.rect.width();
        geom.height = record.rect.height();
        geom.rotation = record.rotation;
        geom.fontScale = record.fontScale;

        auto *item = new OInfoBlockItem(record.index, host);
        item->SetBlockGeom(geom);
        item->SetContent(record.lines, record.fontScale);
        MakeDecorReadOnly(item);
    }

    for (const OInfoBlockSvg::Annotation &record : decor.annotations)
    {
        OAnnotationGeom geom;
        geom.pos = record.rect.topLeft();
        geom.boxWidth = record.rect.width();
        geom.boxHeight = record.rect.height();
        geom.fontScale = record.fontScale;

        OAnnotationRenderStyle style;
        style.fontFamily = record.fontFamily;
        style.fontWeight = record.fontWeight;
        style.fontSize = record.fontSizePx;
        style.textColor = record.textColor;
        style.backgroundColor = record.backgroundColor;

        auto *item = new OAnnotationItem(record.index, host);
        item->SetIndex(record.index);
        // SetContent before SetAnnotationGeom: an auto-sized box takes its size from the
        // natural size SetContent measured. Details and Graded both call them in this order.
        item->SetContent(record.text, style);
        item->SetAnnotationGeom(geom, record.rotation);
        MakeDecorReadOnly(item);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// Resolve an owner's decor and draw it in one go — the LIVE path, for the callers that hold no frozen
/// records: the decor drawn over a group, and anything that still reads the document as it stands.
///
/// ⚠️ A caller that CAN hold the records — the pieces of the reference layer — must not come through
/// here: it would resolve at draw time, which is the drift this whole split exists to remove.
auto BuildOwnerDecor(QGraphicsItem *host, const IPieceInfo &owner, bool isGroup, quint32 ownerId,
                     const QPointF &centerLocal, qreal mirrorX, const OSizerange *range,
                     const VContainer *pattern, VAbstractPattern *doc,
                     OGradedSvg::LayoutObject *decor = nullptr) -> void
{
    const OExportPieceDecor records = ResolveOwnerDecor(owner, isGroup, ownerId, centerLocal, mirrorX,
                                                        range, pattern, doc);
    DrawOwnerDecor(host, records);

    if (decor != nullptr)
    {
        decor->blocks = records.blocks;
        decor->annotations = records.annotations;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// Whether an item is decor rather than geometry of the piece it hangs from.
///
/// The very filter the SVG writer routes on: what it can write as a trace or as text.
/// A cartouche and an annotation are neither, and travel as records instead.
auto IsDecorChild(QGraphicsItem *child) -> bool
{
    if (child == nullptr || !child->isVisible())
    {
        return false;
    }

    const int type = child->type();
    return type == QGraphicsPathItem::Type
           || type == VGraphicsFillItem::Type
           || type == VGraphicsFoldLineItem::Type
           || type == QGraphicsSimpleTextItem::Type;
}

//---------------------------------------------------------------------------------------------------------------------
/// The decor of one isolated piece, on the reference layer.
///
/// A piece's children are stored in the document's own coordinates, so its carrier takes
/// the placement as-is and its local origin is the document origin. A piece of a group of
/// one keeps its own decor: below kMinGroupMembers a group is not a block.
void BuildPieceDecor(QGraphicsScene *scene, const OExportPieceGeometry &geometry,
                     const QTransform &placement, qreal rotation, int footprintIndex,
                     const OSizerange *range, const VContainer *pattern, VAbstractPattern *doc,
                     OGradedSvg::LayoutObject *decor = nullptr)
{
    DecorFrame frame;
    frame.placement = placement;
    frame.rotation = rotation;
    frame.origin = QPointF(0.0, 0.0);
    frame.bounds = UnplacedBounds(geometry.piece);
    frame.footprintIndex = footprintIndex;
    // The piece's children are hung in the document frame, which the piece's own reflection never
    // reaches — the outline is drawn through the matrix and they are not. The axis travels with the
    // frame so that the decor can re-lay each of them on the side the piece actually is.
    frame.mirrorX = geometry.mirrorX;

    const auto *pieces = pattern != nullptr ? pattern->DataPieces() : nullptr;

    // ⚠️ **The frozen decor wins, and it is the whole point.** A piece of the reference layer carries
    // its cartouche and its annotations RESOLVED, recorded when the output was generated. Drawing them
    // from here is what keeps an output showing its own size: the live path reads the document, whose
    // children are re-materialised every time it is put on a size — so a run generated at one size and
    // looked at from another drew its outlines frozen and its children alive, at two different sizes.
    const bool frozen = geometry.decor.resolved;

    if (!frozen)
    {
        // The live path needs the piece itself, and it is the document that holds it.
        if (pieces == nullptr)
        {
            return;
        }

        if (pieces->constFind(geometry.id) == pieces->cend())
        {
            return;
        }
    }

    auto *host = MakeDecorHost(scene, frame);
    if (decor != nullptr)
    {
        // The carrier's own transform, which is what maps this owner's frame into the bin.
        // The records below are written in that frame; the items the carrier holds are not,
        // they are already placed — hence two wrappers in the SVG.
        decor->transform = host->sceneTransform();
    }

    if (frozen)
    {
        DrawOwnerDecor(host, geometry.decor);

        if (decor != nullptr)
        {
            decor->blocks = geometry.decor.blocks;
            decor->annotations = geometry.decor.annotations;
        }
    }
    else
    {
        BuildOwnerDecor(host, pieces->constFind(geometry.id).value(), false, geometry.id,
                        frame.bounds.center() - frame.origin, frame.mirrorX, range, pattern, doc,
                        decor);
    }

    if (host->childItems().isEmpty())
    {
        delete host;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// The decor of one group, on the reference layer: its grainline, its cartouche and its
/// annotations.
///
/// A group has no item of its own in this scene: the nesting places its members. Graded
/// builds the same invisible carrier — its origin is the top-left corner of the members,
/// which is the frame the group stores all of its children in.
void BuildGroupDecor(QGraphicsScene *scene, quint32 groupId, const DecorFrame &frame,
                     const OSizerange *range, const Unit &unit, const VContainer *pattern,
                     VAbstractPattern *doc, OGradedSvg::LayoutObject *decor = nullptr)
{
    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (groups == nullptr || frame.bounds.isNull())
    {
        return;
    }

    const auto group = groups->constFind(groupId);
    if (group == groups->cend() || group.value().pieceIds.size() < OGraded::kMinGroupMembers)
    {
        return;
    }

    auto *host = MakeDecorHost(scene, frame);
    const QPointF centerLocal = frame.bounds.center() - frame.origin;

    if (decor != nullptr)
    {
        decor->name = group.value().GetName();
        decor->transform = host->sceneTransform();
    }

    const VGrainlineData &geom = group.value().m_glGrainline;
    if (geom.IsEnabled() && geom.IsVisible())
    {
        const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, groupId, true);
        const OGraded::GrainlinePlacement placement =
            OGraded::ResolveGrainline(geom, ov, true, pattern, unit, centerLocal);
        const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), true, pattern);

        OGraded::MakeGrainlineItem(host, geom, placement, rotation);
    }

    // No mirror for a group: nothing in this path reflects one — the reflection is a property of a
    // PIECE (the folded twin, `Force flipping`, the symmetrical copy), and a grouped member carries
    // no decor of its own. The group's carrier is also built in another frame, where the axis above
    // would not mean anything.
    BuildOwnerDecor(host, group.value(), true, groupId, centerLocal, qQNaN(), range, pattern, doc,
                    decor);

    if (decor != nullptr)
    {
        // The grainline only — the carrier's other children are the cartouche and the
        // annotations, which travel as records, in their owner's frame.
        for (QGraphicsItem *child : host->childItems())
        {
            if (IsDecorChild(child))
            {
                decor->decorChildren.append(child);
            }
        }
    }

    if (host->childItems().isEmpty())
    {
        delete host;
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto ResolvePieceDecor(const OExportPieceGeometry &geometry, const OSizerange *range,
                       const VContainer *pattern, VAbstractPattern *doc) -> OExportPieceDecor
{
    const auto *pieces = pattern != nullptr ? pattern->DataPieces() : nullptr;
    if (pieces == nullptr)
    {
        return {};
    }

    const auto piece = pieces->constFind(geometry.id);
    if (piece == pieces->cend())
    {
        return {};
    }

    // The very centre the live path falls back on for a child that was never placed — measured on the
    // frozen piece, so that the record and a later drawing agree about where "on its owner" is.
    const QPointF centerLocal = UnplacedBounds(geometry.piece).center();
    return ResolveOwnerDecor(piece.value(), false, geometry.id, centerLocal, geometry.mirrorX, range,
                             pattern, doc);
}

//---------------------------------------------------------------------------------------------------------------------
auto ResolvePieceGrainline(const OExportPieceGeometry &geometry, const OSizerange *range,
                           const Unit &unit, const VContainer *pattern, bool cuttingPlan)
    -> OExportPieceGrainline
{
    OExportPieceGrainline frozen;

    // A grouped member is answered by the DRAWING side, which turns the grainline off on the placed
    // piece — that answer depends on nothing the document says about a size, so it is not frozen here.
    if (IsGroupedMember(pattern, geometry.groupId, cuttingPlan))
    {
        return frozen;
    }

    const auto *pieces = pattern != nullptr ? pattern->DataPieces() : nullptr;
    if (pieces == nullptr)
    {
        return frozen;
    }

    const auto piece = pieces->constFind(geometry.id);
    if (piece == pieces->cend())
    {
        return frozen;
    }

    // Read on the FROZEN copy, which is the same object the drawing side will guard on: a piece with
    // no grainline, or one whose grainline was refused when it was built, has nothing to replace.
    const VPieceGrainline &own = geometry.piece.GetGrainline();
    if (!own.IsEnabled() || !own.IsVisible())
    {
        return frozen;
    }

    const VGrainlineData &geom = piece.value().GetGrainlineGeometry();

    // The line GetItem built ignores the range, but its start point is the Details placement — the
    // fallback for a grainline that was never placed, so it does not land on the document origin.
    const QPointF fallback = own.GetMainLine().p1();

    const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, geometry.id, false);
    const OGraded::GrainlinePlacement placement =
        OGraded::ResolveGrainline(geom, ov, false, pattern, unit, fallback);
    const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), false, pattern);

    // The piece hands its grainline out, and the range only ever moves the origin and the length: the
    // angle stays the piece's own formula, which this same call evaluated.
    const VPieceGrainline moved(placement.pos, placement.lengthPx, qDegreesToRadians(rotation),
                                geom.GetArrowType());

    frozen.resolved = true;
    frozen.mainLine = moved.GetMainLine();
    return frozen;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCanvas::Build(QGraphicsScene *scene,
                          const QVector<OExportSizeLayer> &layers,
                          const OExportNestingResult &result,
                          const OSizerange *range,
                          const VContainer *pattern,
                          VAbstractPattern *doc,
                          OExportCanvasContent *content,
                          const OExportArtboards::Grid &artboards,
                          const OExportTemplate *marks,
                          bool framed,
                          const OExportCoverPage::Drawing *cover,
                          const OExportRunFlags &run)
{
    if (scene == nullptr || !result.ok)
    {
        return;
    }

    // Lu une fois, sous son ancien nom : le reste de cette fonction demande la même chose à quatre
    // endroits, et quatre lectures d'un drapeau sont quatre occasions d'en lire un autre.
    const bool cuttingPlan = run.cuttingPlan;

    scene->clear();

    // What was drawn, kept as it is drawn: every item stays a pointer into the scene, and
    // the caller must read it before the scene dies.
    QVector<QVector<QGraphicsItem *>> itemsByLayer;
    QVector<QVector<const OExportPieceGeometry *>> geometriesByLayer;
    if (content != nullptr)
    {
        itemsByLayer.resize(layers.size());
        geometriesByLayer.resize(layers.size());
        content->layers.clear();
        content->decor.clear();
    }

    const Unit *patternUnit = pattern != nullptr ? pattern->GetPatternUnit() : nullptr;
    const Unit unit = patternUnit != nullptr ? *patternUnit : Unit::Mm;

    // The frame of each group, on the reference layer only: the group's decor is drawn
    // once, above all of its members, once the loop has placed them.
    QHash<quint32, DecorFrame> groupFrames;

    // The per-size trace styles, read on the very key the Graded canvas reads them on.
    // They are authored against a measurements table, and a table outside the library has
    // no key to read: the canvas styles nothing there, and an export that disagreed with
    // the screen would be a trap — the mode delivers a document built from what is shown.
    // The "Show in canvas" toggle is deliberately not read here: it is a canvas
    // preference, and the Graded SVG export ignores it for the same reason.
    QHash<QString, OGradedSizeStyle> styles;
    if (auto *settings = VAbstractApplication::VApp()->Settings(); settings != nullptr)
    {
        const OGradedSizeStyles::StyleAxis axis = OGradedSizeStyles::ResolveStyleAxis(
            doc, pattern, range != nullptr ? range->name : QString());
        if (axis.ready && axis.inLibrary)
        {
            styles = OGradedSizeStyles::StylesForAxis(settings, axis.canonical, axis.axisName);
        }
    }

    // The paper: the run's own extent on a run that is one continuous sheet, the sheets it
    // is cut into on a run that is tiled. One decision made once, and never both — the bin's
    // bottom edge would fall in the middle of the last row of sheets, exactly where a whole
    // page has to read as one.
    //
    // Which of those sheets is worth paper is read from the run's own placements: a pure function
    // of two things the caller already handed over, called here and — on the same two — by the PDF
    // writer. So the canvas cannot show a sheet the file will not write, nor hide one it will.
    const OExportCoverPage::Seat coverSeat =
        cover != nullptr ? cover->seat : OExportCoverPage::Seat::None;
    // The margin travels with the page: a reserved block is placed with it, and with the run's own
    // — see Drawing::marginPx. Read once, here, rather than at each of the three calls below.
    const qreal coverMarginPx = cover != nullptr ? cover->marginPx : 0.0;

    // **The reserved block is ink for the one question OExportSheets asks** — which sheets carry
    // something — and for no other. A plan that happens to keep every piece out of cell (0,0) would
    // otherwise not write the sheet its own cover page is drawn on. The extra sheet is deliberately
    // NOT passed: it is not a cell, and it must never be counted as one.
    //
    // Guarded by `artboards.valid` because a roll has no grid and no cells to mark — its single
    // page is the bin, and the cover page stands on it whatever the pieces do.
    const QRectF reserved =
        (coverSeat == OExportCoverPage::Seat::InSheet && artboards.valid)
            ? OExportCoverPage::PaperPx(coverSeat, coverMarginPx, artboards.planRect,
                                        artboards.cellWidthPx, artboards.cellHeightPx)
            : QRectF();

    const OExportSheets::Set sheets = OExportSheets::Compute(artboards, result.placements, reserved);

    if (artboards.valid)
    {
        MakeArtboardPaper(scene, artboards, sheets);

        // The cover page's own sheet, for the three formats that give it one. **Added here and not
        // through OExportSheets**: it is not a cell of the grid, so the plan's ranks stay 1..n, its
        // page count never sees it, and the arbitration that compares printed sheets cannot be moved
        // by it — which is exactly what the A4/Letter formats need, their nesting being untouched.
        if (coverSeat == OExportCoverPage::Seat::ExtraSheet)
        {
            MakePaper(scene, OExportCoverPage::PaperPx(coverSeat, coverMarginPx, artboards.planRect,
                                                       artboards.cellWidthPx,
                                                       artboards.cellHeightPx));
        }
    }
    else
    {
        auto *bin = new QGraphicsRectItem(0, 0, result.binWidthPx, result.usedHeightPx);
        bin->setPen(QPen(Qt::black, 1));
        bin->setBrush(QBrush(Qt::white));
        bin->setZValue(kBinZ);
        scene->addItem(bin);

        // ── Le bord du pli, quand le tissu est plié ─────────────────────────────────────────────
        //
        // Le bac fait alors la DEMI-laize, et l'un de ses deux grands côtés est le pli — celui
        // auquel les pièces au pli sont collées, leur foldline dessus. Sans ce trait, les deux
        // côtés se ressemblent et rien ne dit lequel c'est. C'est le GAUCHE : il faut qu'il soit
        // fixe, et le choix n'a pas d'importance.
        //
        // Il est tracé APRÈS le rectangle, au même plan : à égalité de plan, Qt dessine dans
        // l'ordre d'ajout, donc il passe par-dessus l'arête du bac au lieu de lutter avec elle.
        // ── Le tissu plié : où sont les plis, et comment il se replie ────────────────────────────
        //
        // **Le trait de pli se lit sur les zones, jamais sur une convention.** Un plan à bord
        // unique a une zone — la colonne — dont le pli peut être à gauche comme à droite : c'est
        // la pièce au pli qui en décide, pas le canvas. Un plan à deux bords en a trois.
        //
        // Et les rectangles ne se dessinent **qu'à partir de deux** : une seule zone EST la
        // colonne, et le trait du bac la dessine déjà. **Pointillé noir à 50 %**, purement visuel :
        // la ligne du milieu d'un plan à deux bords est celle où les deux lisières se rejoignent, et
        // aucune pièce ne la traverse — non parce que ce trait l'interdit, mais parce que chaque
        // rectangle est un bac à part et que le moteur ne pose jamais hors de son bac.
        //
        // Tout part dans l'écran, dans le PNG et dans le SVG : l'export d'image rend la scène. Le
        // plan de coupe n'a pas de PDF — il n'y a donc rien d'autre à nourrir.
        for (const OExportFoldedZone &zone : result.foldedZones)
        {
            if (result.foldedZones.size() > 1)
            {
                auto *rect = new QGraphicsRectItem(zone.rect);
                rect->setPen(QPen(QColor(0, 0, 0, 128), 1, Qt::DotLine));
                rect->setBrush(Qt::NoBrush);
                rect->setZValue(kBinZ);
                scene->addItem(rect);
            }

            const qreal foldX = zone.foldOnTheLeft ? zone.rect.left() : zone.rect.right();
            auto *fold = new QGraphicsLineItem(foldX, zone.rect.top(), foldX, zone.rect.bottom());
            fold->setPen(QPen(Qt::black, 1, Qt::DashDotLine));
            fold->setZValue(kBinZ);
            scene->addItem(fold);
        }
    }

    // The assembly marks go on that paper, under everything the run places on it: the plan's own
    // ground. They are the template's, resolved by the caller — this class knows nothing of
    // templates, and a canvas that looked one up would be a second place deciding what is drawn.
    if (artboards.valid && marks != nullptr)
    {
        OExportMarkGrid grid;
        grid.columns = artboards.columns;
        grid.rows = artboards.rows;
        grid.sheetWidthPx = artboards.cellWidthPx;
        grid.sheetHeightPx = artboards.cellHeightPx;
        grid.pxPerMm = ToPixel(1.0, Unit::Mm);
        grid.sheets = sheets;

        scene->addItem(new AssemblyMarksItem(*marks, artboards.planRect.topLeft(), grid, framed));
    }

    // The page itself, over the marks and under the pieces — see kCoverZ for why that band and not
    // another. A roll takes the same path as a sheet: its "plan rect" is the empty one, whose
    // top-left IS the bin's origin, so the block lands at the corner either way.
    if (cover != nullptr && !cover->IsEmpty())
    {
        scene->addItem(new CoverPageItem(
            OExportCoverPage::PagePx(coverSeat, coverMarginPx, artboards.planRect,
                                     artboards.cellWidthPx, artboards.cellHeightPx),
            cover->bands));
    }

    for (int i = 0; i < result.placements.size() && i < result.keys.size(); ++i)
    {
        const OExportPlacement &placement = result.placements.at(i);
        const QString &key = result.keys.at(i);

        for (int l = 0; l < layers.size(); ++l)
        {
            const OExportSizeLayer &layer = layers.at(l);
            // The layers come in the order of the range's sizes, smallest first: the
            // first one is the reference, the only one allowed to carry per-piece decor.
            const bool isReference = (l == 0);

            for (const OExportPieceGeometry &geometry : layer.pieces)
            {
                if (geometry.key != key)
                {
                    continue;
                }

                // The placement formula lives in OExportFootprint and only there. The
                // correction that moves a piece by hand rebuilds the same matrix, and the one
                // thing these two must never do is drift: a piece the canvas draws where the
                // engine did not put it would be exported somewhere else than it is shown.
                // The note on the alignment shift — it belongs to the origin, not after the
                // rotation — travelled with the code, to PieceMatrix.
                const QTransform placementMatrix =
                    OExportFootprint::PieceMatrix(placement, geometry.alignDelta);

                // The placement is baked into the piece's own matrix rather than into a
                // transform on the item: an item transform would drag the bounding rect,
                // the selection and the rubber band along with it.
                //
                // **Composed, never overwritten.** The piece may carry a matrix of its own — that is
                // where a cutting plan's mirror lives, put there while the geometry was collected.
                // Overwriting it would lose the mirror, and the screen would show something other
                // than what was nested. A piece with no matrix of its own — the whole print path —
                // contributes the identity, so this is exactly the matrix it was before.
                VLayoutPiece placed = geometry.piece;
                placed.SetMatrix(geometry.piece.GetMatrix() * placementMatrix);

                // The decor is set on the piece before GetItem runs, because GetItem is
                // what builds it — and it is set on the reference layer only.
                if (isReference)
                {
                    ApplyReferenceGrainline(placed, geometry, range, unit, pattern, cuttingPlan);
                }

                QGraphicsItem *item = placed.GetItem(false, true, false);
                if (item == nullptr)
                {
                    continue;
                }

                // Each layer carries the label of the size it is, and takes that size's
                // style — the same pass the Graded canvas runs, so a size reads the same
                // way on screen and in the document. The items are fresh out of GetItem
                // and thrown away with the scene, so the pass owns nothing to restore.
                item->setData(kGradedSizeRole, layer.label);
                // Every size of a footprint carries the same index, which is what makes the
                // follow set whole: moving a piece moves its other sizes with it.
                item->setData(kExportFootprintRole, i);
                OGradedSizeTrace::ApplyToSubtree(item, styles, OGradedSizeTrace::Scope::FreshItems);

                if (!isReference)
                {
                    HidePerSizeDecor(item);
                }
                else if (IsGroupedMember(pattern, geometry.groupId, cuttingPlan))
                {
                    // A grouped member carries nothing of its own: the group hosts the
                    // single set, grainline included. Its frame is measured here, at the
                    // one moment its unplaced extent is still at hand.
                    DecorFrame &frame = groupFrames[geometry.groupId];
                    frame.placement = placementMatrix;
                    frame.rotation = placement.rotation;
                    frame.bounds = frame.bounds.united(UnplacedBounds(geometry.piece));
                    frame.footprintIndex = i;
                }

                scene->addItem(item);

                if (content != nullptr)
                {
                    itemsByLayer[l].append(item);
                    geometriesByLayer[l].append(&geometry);
                }

                if (isReference && !IsGroupedMember(pattern, geometry.groupId, cuttingPlan))
                {
                    // The decor layer holds the reference size's decor. Its records are
                    // collected here, where the decision to draw it was taken — a second
                    // pass over the scene would have to guess at both.
                    OGradedSvg::LayoutObject entry;
                    entry.name = geometry.name;
                    // The records are written in the owner's own frame and the wrapper
                    // carries them; the items below are already placed and must not be.
                    entry.origin = QPointF(0.0, 0.0);
                    for (QGraphicsItem *child : item->childItems())
                    {
                        if (IsDecorChild(child))
                        {
                            entry.decorChildren.append(child);
                        }
                    }

                    BuildPieceDecor(scene, geometry, placementMatrix, placement.rotation, i, range,
                                    pattern, doc, content != nullptr ? &entry : nullptr);

                    if (content != nullptr && (!entry.blocks.isEmpty() || !entry.annotations.isEmpty()
                                               || !entry.decorChildren.isEmpty()))
                    {
                        content->decor.append(entry);
                    }
                }
            }
        }
    }

    for (auto it = groupFrames.begin(); it != groupFrames.end(); ++it)
    {
        // The children of a group are stored relative to the top-left corner of its
        // members, and that corner is only known once every member has contributed its
        // extent — hence the second pass rather than a frame built on the fly.
        DecorFrame &frame = it.value();
        frame.origin = frame.bounds.topLeft();

        OGradedSvg::LayoutObject entry;
        entry.origin = QPointF(0.0, 0.0);
        BuildGroupDecor(scene, it.key(), frame, range, unit, pattern, doc,
                        content != nullptr ? &entry : nullptr);

        if (content != nullptr && (!entry.blocks.isEmpty() || !entry.annotations.isEmpty()
                                   || !entry.decorChildren.isEmpty()))
        {
            content->decor.append(entry);
        }
    }

    // The last thing drawn, and the only one above the decor: the cut lines are all that
    // tells one sheet from the next, and a cartouche lying across one must not hide the line
    // the sheet is cut along. Two items rather than one, and that is forced — no single item
    // can sit under the pieces and over the cartouches at the same time.
    //
    // They stand down wherever the template draws a frame, because the frame IS those lines —
    // drawn once per boundary, in the user's own pen and at the opacity she set (§11.2). Left in
    // place, a black line would cover it and the setting would look dead, which is exactly the
    // defect the answer of 17-09-2026 cured in the preview (§11.5). They stay everywhere else:
    // on every other format, and whenever there is no template at all — where they are then the
    // only thing that tells one sheet from the next.
    const bool frameDrawn = marks != nullptr && framed;
    if (artboards.valid && !frameDrawn)
    {
        scene->addItem(new ArtboardGridItem(artboards.planRect, artboards.cellWidthPx,
                                            artboards.cellHeightPx, artboards.columns,
                                            artboards.rows, sheets));
    }

    if (content == nullptr)
    {
        return;
    }

    // Hand back what was drawn, in the shape a writer needs. Assembled from the pairs kept
    // during the walk rather than re-derived: a group is one object holding the members it
    // was nested as, and only that walk knows which items those are.
    for (int l = 0; l < layers.size() && l < itemsByLayer.size(); ++l)
    {
        OExportCanvasContent::Layer entry;
        entry.label = layers.at(l).label;

        // The name the layer is to be written under, read on the key the trace styles were
        // read on — the same table, resolved once above for drawing and reused here for
        // naming. The label stays what identifies the size; only the name changes.
        const auto style = styles.constFind(entry.label);
        if (style != styles.cend())
        {
            entry.displayName = style->layerName;
        }

        QHash<QString, int> objectOfName;
        const QVector<QGraphicsItem *> &items = itemsByLayer.at(l);
        for (int k = 0; k < items.size() && k < geometriesByLayer.at(l).size(); ++k)
        {
            const OExportPieceGeometry *geometry = geometriesByLayer.at(l).at(k);
            const bool grouped = IsGroupedMember(pattern, geometry->groupId, cuttingPlan);
            const QString name = grouped ? geometry->groupName : geometry->name;

            int index = objectOfName.value(name, -1);
            if (index < 0)
            {
                OExportCanvasContent::Object object;
                object.name = name;
                object.isGroup = grouped;
                entry.objects.append(object);
                index = static_cast<int>(entry.objects.size()) - 1;
                objectOfName.insert(name, index);
            }

            if (grouped)
            {
                entry.objects[index].members.append({geometry->name, items.at(k)});
            }
            else
            {
                entry.objects[index].loneItem = items.at(k);
            }
        }

        content->layers.append(entry);
    }
}
