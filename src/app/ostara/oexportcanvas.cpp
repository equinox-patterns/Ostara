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
#include <QGraphicsPathItem>
#include <QPainterPath>

#include <algorithm>

#include "core/ogratedsizestyles.h"
#include "core/oexporttemplate.h"
#include "mainwindow_graded_helpers.h"
#include "oexportmarkpainter.h"
#include "ogradedgeometry.h"
#include "ogradedsizetrace.h"
#include "oexportcoverpage.h"
#include "oexportgeometry.h"
#include "oexportink.h"
#include "oexportfootprint.h"
#include "oexportmatpacking.h"
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
#include "../vpatterndb/ochildanchor.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/othumbnaildata.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockcontent.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/othumbnailitem.h"
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
/// the run, the document and the template - the canvas places and paints, and looking a template up
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
        // Decided on 18-09-2026: a logo of two centimetres is two centimetres on a page
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
void MakePaper(QGraphicsScene *scene, const QRectF &rect, const QPointF &origin = {})
{
    auto *paper = new QGraphicsRectItem(rect);
    paper->setPen(Qt::NoPen);
    paper->setBrush(QBrush(Qt::white));
    paper->setFlag(QGraphicsItem::ItemIsSelectable, false);
    paper->setFlag(QGraphicsItem::ItemIsMovable, false);
    paper->setAcceptedMouseButtons(Qt::NoButton);
    paper->setZValue(kPaperZ);
    // La feuille se pose où le plan se pose : un second plan de la paire a son propre papier.
    paper->setPos(origin);
    scene->addItem(paper);
}

void MakeArtboardPaper(QGraphicsScene *scene, const OExportArtboards::Grid &artboards,
                       const OExportSheets::Set &sheets, const QPointF &origin = {})
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
                                    artboards.cellWidthPx, artboards.cellHeightPx),
                      origin);
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
/// The range's frame for the miniature of one piece, or an invalid one.
///
/// Same rule as the two above, with one difference: a miniature belongs to a PIECE, never to a group —
/// so the group's overrides are keyed by the MEMBER they lay out, not by the group. A group carries one
/// per member, and each is looked up on the piece it draws.
auto ThumbnailOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup, quint32 pieceId)
    -> OSizerange::ThumbnailOverride
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
        const auto ov = it.value().memberThumbnails.constFind(pieceId);
        return ov == it.value().memberThumbnails.cend() ? OSizerange::ThumbnailOverride{} : ov.value();
    }

    const auto it = range->pieceContent.constFind(ownerId);
    return it == range->pieceContent.cend() ? OSizerange::ThumbnailOverride{} : it.value().thumbnail;
}

//---------------------------------------------------------------------------------------------------------------------
/// Whether a piece belongs to a group that is nested as one rigid block. Below two
/// members a group is not a block — the canvas, the export and the container all draw
/// that line in the same place (OGraded::kMinGroupMembers).
auto IsGroupedMember(const VContainer *pattern, quint32 groupId, OExportFormat::Kind format) -> bool
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
    if (OExportFormat::IsCutting(format))
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
                             OExportFormat::Kind format, bool asLonePiece = false)
{
    VPieceGrainline &grainline = placed.GetGrainline();

    if (!asLonePiece && IsGroupedMember(pattern, geometry.groupId, format))
    {
        // A grouped member carries no grainline of its own: the group hosts one for all
        // of them, exactly as Details and Graded do it.
        //
        // ⚠️ **Sauf pour le payload.** Rabnest doit pouvoir afficher un membre SEUL, et il lui faut
        // alors la ligne de ce membre : `asLonePiece` demande la sienne, et cette copie-là ne va pas
        // au plan — elle est masquée aussitôt construite. Ce que le groupe dessine n'est pas touché.
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

    OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, geometry.id, false);
    // Le repli sur le groupe, pour la même raison qu'à `ApplyReferenceGrainline`.
    if (!ov.valid && geometry.groupId != 0)
    {
        ov = GrainlineOverrideOf(range, geometry.groupId, true);
    }
    // ⚠️ **Le membre d'un groupe prend le droit-fil du GROUPE quand il n'a pas le sien.**
    // En Graded on sélectionne le groupe, et c'est lui qui porte la ligne de ses membres ; la place
    // posée à la main est donc rangée dans le contenu du groupe, pas dans celui de la pièce. Sans ce
    // repli, un membre retombait sur la position du MODÈLE — celle d'avant Graded — et le payload
    // écrivait une ligne que l'écran n'a jamais montrée.
    if (!ov.valid && geometry.groupId != 0)
    {
        ov = GrainlineOverrideOf(range, geometry.groupId, true);
    }
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
/// Give a grouped member the fold line the GROUP carries — that is, only the member it chose.
///
/// **A group draws one fold line, and it is the chosen member's.** Details says it with the stack
/// (`OPieceGroupHolderItem::RefreshMemberShapes`: the mirror line, its mark and its captions are
/// taken from the one member `m_foldLineMemberId` names), Graded says it on its reference layer, and
/// a plotter does not care because it decomposes the groups. The export canvas drew each member's
/// own — so a printed group showed as many fold lines as it had members carrying one, and the choice
/// the group exists for disappeared on paper.
///
/// ⚠️ **A cutting plan and a plotter are NOT concerned.** Both cut pieces one by one, so the member
/// being cut keeps its own fold line exactly as its own piece draws it. The caller poses the rule
/// once, above, and it is the same one the grainline obeys.
///
/// ⚠️ **La ligne miroir, elle, n'est PAS touchée.** Elle ne dessine pas le pli : elle dessine l'axe
/// d'une pièce dépliée, et c'est elle qui fabrique aussi les place-labels en miroir
/// (`VLayoutPiece::GetItem`) — l'effacer retirerait les étiquettes du membre. Ce que le groupe ne
/// prend pas à un membre qu'il n'a pas choisi, c'est son PLI : sa ligne, ses flèches, son texte.
void ApplyGroupFoldLine(VLayoutPiece &placed, const OExportPieceGeometry &geometry,
                        const VContainer *pattern)
{
    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (groups == nullptr || geometry.groupId == NULL_ID)
    {
        return;
    }

    const auto group = groups->constFind(geometry.groupId);
    if (group == groups->cend() || group.value().m_foldLineMemberId == geometry.id)
    {
        return; // the group's own choice — its line stays where the member draws it
    }

    placed.SetFoldLineType(FoldLineType::None);
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

    /// One box per member of a GROUP, by piece id, in the frame the group's children are stored in —
    /// which takes its origin off @c bounds, hence the translation done once the walk is over.
    ///
    /// It exists for the miniatures alone, and for the reason the Graded canvas measures them the same
    /// way: **a miniature nobody ever placed lands on the PIECE IT DRAWS**, never on the union of the
    /// group. Empty for a lone piece, which is its own box (@c bounds).
    QHash<quint32, QRectF> memberBoxes{};

    /// Le pli du groupe — porté par UN membre (`m_foldLineMemberId`), et ramassé ici.
    ///
    /// ⚠️ **Le groupe n'a aucune géométrie de pli : il désigne un membre, et c'est la pièce de ce
    /// membre qui porte le trait.** Sur le canvas, ce trait est un enfant de l'item du porteur, donc il
    /// se voit — mais la couche de décor du PDF ne sérialise que les enfants des PORTEURS DE DÉCOR, et
    /// le porteur est le groupe : le trait n'était écrit nulle part, et le PDF d'un groupe n'avait pas
    /// de pli alors que le canvas en montrait un. Mesuré sur un vrai run.
    ///
    /// Ils sont écrits dans le décor du groupe (`InkSpace::ToFile` ramène chaque item dans le repère
    /// du propriétaire à partir de sa transformation de scène), donc rien n'est reparenté.
    QVector<QGraphicsItem *> foldChildren{};
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
/// One owner's miniatures, RESOLVED — the record form both the writers and the canvas consume.
///
/// A miniature is a child of the piece like its cartouche, and it is resolved from the very three
/// sources the Details and Graded canvases read, in the very order they read them: the owner's own
/// miniature data — a group carries one per MEMBER, never one of its own — the range's frame when it
/// has one, and the box of the piece it DRAWS, which is what an unplaced frame is measured on.
///
/// The record itself is the shared builder's (@ref OInfoBlockSvg::BuildThumbnail), so this mode, the
/// flat sheet and the graded sheet cannot come to three answers about the same miniature. It comes
/// back with empty content when the tool is gone or the reference drawing was never captured, and the
/// canvas draws nothing then either — so dropping it here is not a loss.
///
/// ⚠️ **Un plan de coupe n'en porte aucune** (décidé). Ce n'est pas un détail de
/// dessin : la règle est posée **ici**, à la résolution, pour qu'aucun record n'existe et qu'aucun des
/// trois chemins ne puisse en dessiner un par mégarde — le champ est vide, pas caché.
///
/// @param boxes the box each piece is measured on when nobody ever placed its frame, by piece id, in
///              the owner's own frame. A piece absent from it is skipped: there is nothing to measure
///              an unplaced frame on, and a miniature without a frame is not one.
auto ResolveOwnerMiniatures(const IPieceInfo &owner, bool isGroup, quint32 ownerId,
                            const OSizerange *range, const QHash<quint32, QRectF> &boxes,
                            const VContainer *pattern, OExportFormat::Kind format)
    -> QVector<OInfoBlockSvg::Thumbnail>
{
    QVector<OInfoBlockSvg::Thumbnail> out;

    if (not OExportFormat::CarriesDecor(format) || pattern == nullptr)
    {
        return out;
    }

    const auto *pieces = pattern->DataPieces();
    if (pieces == nullptr)
    {
        return out;
    }

    // Who carries what: a lone piece carries its own miniature, a group one per member. The downcast
    // is what tells the two apart — `IPieceInfo` has no per-member accessor, and a group is the only
    // owner that needs one. It fails safe: an owner that is not the group it claims to be carries
    // nothing rather than somebody else's drawing.
    const auto *group = isGroup ? dynamic_cast<const OPieceGroup *>(&owner) : nullptr;
    if (isGroup && group == nullptr)
    {
        return out;
    }

    QVector<quint32> pieceIds;
    if (isGroup)
    {
        pieceIds = group->pieceIds;
    }
    else
    {
        pieceIds.append(ownerId);
    }

    for (const quint32 pieceId : pieceIds)
    {
        const auto piece = pieces->constFind(pieceId);
        const auto box = boxes.constFind(pieceId);
        if (piece == pieces->cend() || box == boxes.cend())
        {
            continue; // a member the document no longer holds, or one nothing measured
        }

        const OThumbnailData stored = isGroup ? group->MemberThumbnail(pieceId)
                                              : owner.GetThumbnailData();
        if (!stored.IsActive())
        {
            continue; // nothing asked for, in Details or in the group
        }

        const OSizerange::ThumbnailOverride ov = ThumbnailOverrideOf(range, ownerId, isGroup, pieceId);

        OThumbnailGeom geom;
        if (ov.valid)
        {
            geom.pos = ov.pos;
            geom.width = ov.width;
            geom.height = ov.height;
        }
        else
        {
            geom = stored.Geom();
        }

        OInfoBlockSvg::Thumbnail record =
            OInfoBlockSvg::BuildThumbnail(pieceId, piece.value(), stored, geom, box.value(),
                                          ov.valid || stored.HasGeometry());
        if (record.content.isEmpty())
        {
            continue;
        }
        out.append(record);
    }

    return out;
}

//---------------------------------------------------------------------------------------------------------------------
/// Add an owner's information blocks, annotations and miniatures, in @p host's frame.
///
/// The owner is the group when the piece belongs to one — a grouped member carries none of
/// these, exactly as it carries no grainline of its own. Details, the Graded canvas and its
/// SVG export all draw that same line.
///
/// @param centerLocal the point a child that was never placed falls back on, in that frame
/// @param mirrorX the vertical line the children are mirrored about, or NaN when the owner is not
///                reflected — see @ref DecorFrame::mirrorX
/// @param miniatureBoxes the box each piece is measured on when nobody ever placed its miniature's
///                frame, by piece id, in that same frame — see @ref ResolveOwnerMiniatures
/// @param format what the run is — a cutting plan carries no miniature at all, and it is
///               @ref OExportFormat::CarriesDecor that says so
/// @param sizeAxes the size this resolution describes — one value per axis, keyed the way the
///                 measurement table names its axes. Empty means the table's reference size,
///                 which is what every live canvas wants; the export hands over the run's own.
auto ResolveOwnerDecor(const IPieceInfo &owner, bool isGroup, quint32 ownerId,
                       const QPointF &centerLocal, qreal mirrorX, const OSizerange *range,
                       const VContainer *pattern, VAbstractPattern *doc,
                       const QHash<quint32, QRectF> &miniatureBoxes, OExportFormat::Kind format,
                       const QMap<QString, QString> &sizeAxes,
                       const OInfoBlockResolver::CutOverride *cut = nullptr)
    -> OExportPieceDecor
{
    OExportPieceDecor decor;
    if (pattern == nullptr)
    {
        return decor;
    }

    decor.resolved = true;

    // The third child, resolved in the same breath as the two below and from the same three sources
    // the canvases read — see @ref ResolveOwnerMiniatures, which is also where a cutting plan is told
    // it carries none.
    decor.thumbnails = ResolveOwnerMiniatures(owner, isGroup, ownerId, range, miniatureBoxes, pattern,
                                              format);

    // A cutting plan carries no block and no annotation either (decided), and the rule is
    // posed here, at resolution, exactly like the miniature's just above: both fields stay empty,
    // they are never hidden, so none of the three paths — canvas, image, PDF — can draw one by
    // mistake, and there is no guard to forget in a fourth. The fold-line caption obeys the same
    // decision but does not live in this record: @ref OExportCanvas::Build turns it off on the copy
    // of the piece that GetItem is about to draw.
    if (not OExportFormat::CarriesDecor(format))
    {
        return decor;
    }

    // A block exists when the template dispatches a section to it and that section has
    // content: the resolver leaves nothing else in blockContent. Without a document there
    // is no template to resolve, so the blocks are skipped and the annotations are not.
    if (doc != nullptr)
    {
        // ⚠️ `cut` est le seul endroit où un format impose un contenu : le projecteur montre une
        // pièce une fois par coupe, donc chaque exemplaire dit « une fois » — voir CutOverride.
        const OInfoBlockEffectiveState state =
            OInfoBlockResolver::ResolveEffectiveState(owner, doc, pattern, sizeAxes, cut);
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
        if (!style.fontFace.isEmpty())
        {
            font.setStyleName(style.fontFace);
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
        record.fontFace = style.fontFace;
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
        style.fontFace = record.fontFace;
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

    for (const OInfoBlockSvg::Thumbnail &record : decor.thumbnails)
    {
        // ⚠️ No mirror here, unlike the two loops above, and it is structural rather than an omission:
        // a miniature is resolved for a PRINT run alone (see @ref ResolveOwnerMiniatures), and a print
        // run never reflects a piece — the three places that set @c mirrorX (the folded twin,
        // `Force flipping`, the symmetrical copy) all sit behind the cutting-plan branch of
        // OExportGeometry. Were that to change, this is the line that would have to learn to flip the
        // drawing and not only the box.
        OThumbnailGeom geom;
        geom.pos = record.rect.topLeft();
        geom.width = record.rect.width();
        geom.height = record.rect.height();

        auto *item = new OThumbnailItem(host);
        item->SetContent(record.content, record.contentBounds);
        item->SetBlockGeom(geom);

        // ⚠️ The words are given PLACES, not offsets: the record's are already through the fit, and
        // putting them through it a second time is what a `SetLabels` call would do — see
        // OThumbnailItem::SetPlacedWords. All of its words are handed over together, the legend among
        // them, and each carries which of the two texts it is: they are set in their own fonts.
        QVector<OThumbnailItem::PlacedWord> words;
        words.reserve(record.words.size());
        for (const OInfoBlockSvg::ThumbnailWord &word : record.words)
        {
            words.append({word.text, word.origin, word.legend});
        }
        item->SetPlacedWords(words);

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
                     const QHash<quint32, QRectF> &miniatureBoxes, OExportFormat::Kind format,
                     const QMap<QString, QString> &sizeAxes = {},
                     OGradedSvg::LayoutObject *decor = nullptr) -> void
{
    const OExportPieceDecor records = ResolveOwnerDecor(owner, isGroup, ownerId, centerLocal, mirrorX,
                                                        range, pattern, doc, miniatureBoxes,
                                                        format, sizeAxes);
    DrawOwnerDecor(host, records);

    if (decor != nullptr)
    {
        decor->blocks = records.blocks;
        decor->annotations = records.annotations;
        decor->thumbnails = records.thumbnails;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// Whether an item is decor rather than geometry of the piece it hangs from.
///
/// The very filter the SVG writer routes on: what it can write as a trace or as text.
/// A cartouche and an annotation are neither, and travel as records instead.
///
/// ⚠️ **A plain `QGraphicsPathItem` is GEOMETRY, and it used to be taken for decor.** A piece builds
/// its internal paths, its place labels and its mirror line as bare path items beside its contour
/// (`VLayoutPiece::GetItem`), so this filter — which accepted every path — carried them out of the
/// piece and into the decor. The result was visible in the file: with every size layer switched off,
/// size 32's internal paths and place labels were still there, in the common trunk, where they belong
/// to no size and to no piece. What is decor here is what a piece does not DRAW with: its grain line,
/// its fold mark and the fold's caption.
auto IsDecorChild(QGraphicsItem *child, bool includeHidden = false) -> bool
{
    if (child == nullptr || (!includeHidden && !child->isVisible()))
    {
        return false;
    }

    const int type = child->type();
    return type == VGraphicsFillItem::Type      // the grain line
           || type == VGraphicsFoldLineItem::Type  // the fold mark
           || type == QGraphicsSimpleTextItem::Type;  // the fold's caption
}

//---------------------------------------------------------------------------------------------------------------------
/// The decor of one isolated piece, on the reference layer.
///
/// A piece's children are stored in the document's own coordinates, so its carrier takes
/// the placement as-is and its local origin is the document origin. A piece of a group of
/// one keeps its own decor: below kMinGroupMembers a group is not a block.
///
/// @param hostOut receives the carrier the children hang under, when the caller has to hide it —
///        the member of a group keeps its own decor FOR THE PAYLOAD only (Rabnest may show it
///        alone), and that ink must not be drawn on a plan the group already decorates.
void BuildPieceDecor(QGraphicsScene *scene, const OExportPieceGeometry &geometry,
                     const QTransform &placement, qreal rotation, int footprintIndex,
                     const OSizerange *range, const VContainer *pattern, VAbstractPattern *doc,
                     OExportFormat::Kind format, OGradedSvg::LayoutObject *decor = nullptr,
                     QGraphicsItem **hostOut = nullptr)
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
            decor->thumbnails = geometry.decor.thumbnails;
        }
    }
    else
    {
        // A lone piece is its own box: the miniature nobody placed lands on it, not on a frame of
        // its own that nothing ever measured.
        BuildOwnerDecor(host, pieces->constFind(geometry.id).value(), false, geometry.id,
                        frame.bounds.center() - frame.origin, frame.mirrorX, range, pattern, doc,
                        {{geometry.id, frame.bounds}}, format, {}, decor);
    }

    if (host->childItems().isEmpty())
    {
        delete host;
    }
    else if (hostOut != nullptr)
    {
        *hostOut = host;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// Les enfants qu'un membre de groupe porte POUR LUI, pour le payload seulement.
///
/// Un plan d'impression dessine le groupe : c'est le groupe qui porte la grainline, le cartouche, les
/// annotations et les miniatures de ses membres. Mais Rabnest doit pouvoir n'afficher **qu'un
/// membre** — et il lui faut alors les enfants de ce membre, pas ceux du groupe. Ostara les écrit donc
/// dans le payload, à côté du `<decor>` du groupe : le lecteur choisit, le plan ne bouge pas.
///
/// ⚠️ **Rien de ce qui est construit ici n'est visible.** La pièce et son porteur sont masqués
/// aussitôt : le plan continue de montrer le groupe, et seul le sérialiseur lit cette encre. C'est le
/// même geste que le plan de coupe, où le membre retenu porte son décor parce qu'il est seul.
void CollectMemberOwnDecor(QGraphicsScene *scene, const OExportPieceGeometry &geometry,
                           const QTransform &placement, qreal rotation, int footprintIndex,
                           const OSizerange *range, const Unit &unit, const VContainer *pattern,
                           VAbstractPattern *doc, const QVector<QGraphicsItem *> &foldChildren,
                           OGradedSvg::LayoutObject *entry)
{
    // Sa pièce, avec SA ligne : `asLonePiece` demande celle du membre là où le plan l'éteint.
    VLayoutPiece placed = geometry.piece;
    placed.SetMatrix(geometry.piece.GetMatrix() * placement);

    // ⚠️ **Le droit-fil se RALLUME avant d'être posé, pas après.** `ApplyReferenceGrainline` rend la
    // main sans rien faire quand la ligne est masquée (« rien n'est dessiné pour cette pièce ») —
    // et un membre de groupe l'a masquée, c'est la règle du groupe. Appelée avant, elle ne posait
    // donc jamais la ligne GELÉE, et l'item était construit avec celle du MODÈLE : mesuré sur
    // « Manche » à la taille 32, 0.796 de la pièce au lieu de 0.857 — alors que l'attribut
    // `grainLine` du même enregistrement, lui, valait bien 0.857.
    if (placed.GetGrainline().IsEnabled())
    {
        placed.GetGrainline().SetVisible(true);
    }
    placed.SetShowMirrorLine(true);

    // `Print` : ce bloc montre un membre SEUL, avec TOUT ce qu'il porte — le format n'a rien à y
    // éteindre, et c'est le sens de « asLonePiece » depuis toujours.
    ApplyReferenceGrainline(placed, geometry, range, unit, pattern, OExportFormat::Kind::Print,
                            /*asLonePiece=*/true);

    // ── Un membre montré SEUL montre TOUT ce qu'il a ────────────────────────────────────────────
    //
    // Deux drapeaux de DESSIN du plan ne valent pas pour ce bloc-là :
    //
    // 1. une grainline **cachée** en Details (elle existe, on ne l'affiche pas sur le patron) — un
    //    lecteur qui n'affiche que ce membre n'a rien d'autre à montrer, et c'est cette ligne-là qui
    //    dit le sens ;
    // 2. le pli d'une pièce dépliée sans son axe (`IsShowFullPiece() && !IsShowMirrorLine()`) — la
    //    ligne existe et c'est elle qui dit le pli.
    //
    // Aucun des deux ne touche le plan : cette copie est masquée aussitôt construite.
    if (placed.GetGrainline().IsEnabled())
    {
        placed.GetGrainline().SetVisible(true);
    }
    placed.SetShowMirrorLine(true);

    QGraphicsItem *item = placed.GetItem(false, true, false);
    if (item == nullptr)
    {
        return;
    }

    entry->name = geometry.name;
    entry->ownerId = geometry.id;
    // The ink is already placed (the matrix is baked into the piece), so this is what brings it back
    // into the frame `outline` travels in — exactly what the lone-piece entry does.
    entry->ownerPlacement = placement;
    entry->origin = QPointF(0.0, 0.0);
    bool hasOwnFold = false;
    for (QGraphicsItem *child : item->childItems())
    {
        if (IsDecorChild(child))
        {
            entry->decorChildren.append(child);
            hasOwnFold = hasOwnFold || child->type() == VGraphicsFoldLineItem::Type;
        }
    }

    // ── Le pli, repris de la copie VISIBLE quand le membre n'en produit pas ─────────────────────
    //
    // Le porteur du pli du groupe le doit à SON item ; les autres n'en ont aucun (le plan l'éteint
    // chez eux, et c'est la règle du groupe). On ne reprend donc les items du groupe que si la copie
    // n'a rien donné — sinon le trait serait écrit DEUX fois, à l'identique : mesuré sur « Dos x1
    // pli », deux `path fold` aux mêmes coordonnées.
    if (!hasOwnFold)
    {
        entry->decorChildren += foldChildren;
    }

    // ── Et on retire le CONTENU, pas la visibilité ───────────────────────────────────────────────
    //
    // ⚠️ **Le sérialiseur saute un item dont `isVisible()` est faux — et l'enfant d'un parent masqué
    // n'est jamais visible.** Masquer la pièce, comme le faisait la première version, effaçait donc
    // justement les enfants qu'on venait de ramasser : le bloc du membre sortait vide (mesuré sur un
    // vrai payload, pli et grainline absents). On les garde donc VISIBLES et on leur retire leur
    // contenu : `ItemHasNoContents` ne peint rien, mais l'item existe et se sérialise. Aucune encre
    // n'entre dans le plan.
    const std::function<void(QGraphicsItem *)> mute = [&](QGraphicsItem *it)
    {
        it->setFlag(QGraphicsItem::ItemHasNoContents, true);
        it->setAcceptedMouseButtons(Qt::NoButton);
        it->setFlag(QGraphicsItem::ItemIsSelectable, false);
        const auto kids = it->childItems();
        for (QGraphicsItem *child : kids)
        {
            mute(child);
        }
    };
    mute(item);
    scene->addItem(item);

    QGraphicsItem *host = nullptr;
    BuildPieceDecor(scene, geometry, placement, rotation, footprintIndex, range, pattern, doc,
                    OExportFormat::Kind::Print, entry, &host);
    if (host != nullptr)
    {
        host->setVisible(false);
    }
}

/// A group has no item of its own in this scene: the nesting places its members. Graded
/// builds the same invisible carrier — its origin is the top-left corner of the members,
/// which is the frame the group stores all of its children in.

/// A group has no item of its own in this scene: the nesting places its members. Graded
/// builds the same invisible carrier — its origin is the top-left corner of the members,
/// which is the frame the group stores all of its children in.
void BuildGroupDecor(QGraphicsScene *scene, quint32 groupId, const DecorFrame &frame,
                     const OExportGroupDecor *frozen, const OSizerange *range, const Unit &unit,
                     const VContainer *pattern, VAbstractPattern *doc, OExportFormat::Kind format,
                     OGradedSvg::LayoutObject *decor = nullptr)
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
        decor->ownerId = groupId;
        decor->isGroup = true;
        decor->transform = host->sceneTransform();
        // A group's children ARE placed by their host's transform, unlike a piece's.
        decor->ownerPlacement = host->sceneTransform();
    }

    const VGrainlineData &geom = group.value().m_glGrainline;
    if (geom.IsEnabled() && geom.IsVisible())
    {
        // ⚠️ **The frozen placement wins.** It was resolved at generation, while the document was still
        // on the reference layer's size; the live branch below reads the document, so it would draw the
        // group's grainline at whatever size the document is on NOW. That is exactly the defect found
        // by changing the Details size and coming back to the plan.
        if (frozen != nullptr && frozen->grainline.resolved)
        {
            const OGraded::GrainlinePlacement placement{frozen->grainline.pos,
                                                        frozen->grainline.lengthPx};
            OGraded::MakeGrainlineItem(host, geom, placement, frozen->grainline.rotationDegrees);
        }
        else
        {
            const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, groupId, true);
            const OGraded::GrainlinePlacement placement =
                OGraded::ResolveGrainline(geom, ov, true, pattern, unit, centerLocal);
            const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), true, pattern);

            OGraded::MakeGrainlineItem(host, geom, placement, rotation);
        }
    }

    // No mirror for a group: nothing in this path reflects one — the reflection is a property of a
    // PIECE (the folded twin, `Force flipping`, the symmetrical copy), and a grouped member carries
    // no decor of its own. The group's carrier is also built in another frame, where the axis above
    // would not mean anything.
    if (frozen != nullptr && frozen->owner.resolved)
    {
        // The group's cartouche, annotations and miniatures, frozen with the run like a piece's — see
        // @ref OExportGroupDecor. The carrier differs; the resolution is the same one.
        DrawOwnerDecor(host, frozen->owner);

        if (decor != nullptr)
        {
            decor->blocks = frozen->owner.blocks;
            decor->annotations = frozen->owner.annotations;
            decor->thumbnails = frozen->owner.thumbnails;
        }
    }
    else
    {
        // The members' boxes brought into the frame the group's children are written in — the origin
        // is the union's top-left corner, which the walk has just settled. A miniature nobody placed
        // is measured on the member it draws, never on the union: the same rule the Graded canvas and
        // the graded export apply.
        QHash<quint32, QRectF> memberBoxes;
        for (auto it = frame.memberBoxes.cbegin(); it != frame.memberBoxes.cend(); ++it)
        {
            memberBoxes.insert(it.key(), it.value().translated(-frame.origin.x(), -frame.origin.y()));
        }

        BuildOwnerDecor(host, group.value(), true, groupId, centerLocal, qQNaN(), range, pattern, doc,
                        memberBoxes, format, {}, decor);
    }

    if (decor != nullptr)
    {
        // The grainline only — the carrier's other children are the cartouche, the annotations and
        // the miniatures, which travel as records, in their owner's frame.
        for (QGraphicsItem *child : host->childItems())
        {
            if (IsDecorChild(child))
            {
                decor->decorChildren.append(child);
            }
        }

        // ── Et le pli, qui appartient au PORTEUR et non au groupe ────────────────────────────────
        //
        // Le groupe n'a aucune géométrie de pli : il désigne un membre, et le trait est un enfant de
        // l'item de ce membre. Sans cette ligne, le canvas montrait le pli du groupe et le PDF n'en
        // avait aucun — la couche de décor ne sérialise que les enfants des porteurs, et le porteur
        // est le groupe, qui n'en a pas. `InkSpace::ToFile` ramène chaque item de la scène dans le
        // repère du propriétaire, donc celui du membre arrive à sa place dans le repère du groupe.
        decor->decorChildren += frame.foldChildren;
    }

    if (host->childItems().isEmpty())
    {
        delete host;
    }
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto ResolvePieceDecor(const OExportPieceGeometry &geometry, const OSizerange *range,
                       const VContainer *pattern, VAbstractPattern *doc, OExportFormat::Kind format,
                       const QMap<QString, QString> &sizeAxes) -> OExportPieceDecor
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

    // ── Le projecteur, et son « Cut 1 » ─────────────────────────────────────────────────────────
    //
    // La quantité écrite sur un plan projeté n'est pas celle de la pièce : c'est **toujours une**,
    // parce que chaque exemplaire est montré séparément — une pièce coupée deux fois apparaît deux
    // fois, et chacune dit « Couper 1 ». La quantité réelle continue de voyager dans la charge utile.
    // `on fold` reste, lui : c'est une instruction de placement, pas une quantité, et une pièce au
    // pli est dépliée puis montrée une fois.
    //
    // ⚠️ Le pli se lit sur LA GÉOMÉTRIE, jamais sur la case du cartouche : c'est la foldline qui
    // décide de la pose, et c'est elle qui doit se lire ici.
    OInfoBlockResolver::CutOverride cut;
    const OInfoBlockResolver::CutOverride *cutOverride = nullptr;
    if (OExportFormat::IsProjector(format))
    {
        cut.quantity = 1;
        cut.onFold = geometry.onFold;
        cutOverride = &cut;
    }

    return ResolveOwnerDecor(piece.value(), false, geometry.id, centerLocal, geometry.mirrorX, range,
                             pattern, doc, {{geometry.id, UnplacedBounds(geometry.piece)}},
                             format, sizeAxes, cutOverride);
}

//---------------------------------------------------------------------------------------------------------------------
auto ResolvePieceGrainline(const OExportPieceGeometry &geometry, const OSizerange *range,
                           const Unit &unit, const VContainer *pattern, OExportFormat::Kind format,
                           const QRectF &referenceSeat) -> OExportPieceGrainline
{
    OExportPieceGrainline frozen;

    // Le format ne décide plus rien ici : toutes les pièces gelent leur ligne, membres compris —
    // c'est le DESSIN qui l'éteint sur une copie posée (voir ApplyReferenceGrainline), pas le record.
    Q_UNUSED(format)

    // ── Le membre d'un groupe : sa ligne est gelée ELLE AUSSI ────────────────────────────────────
    //
    // Le plan ne la dessine pas — c'est le groupe qui porte sa grainline, et `ApplyReferenceGrainline`
    // l'éteint sur la copie posée. Mais le payload doit pouvoir la donner à un lecteur qui n'affiche
    // QUE ce membre : Rabnest reçoit donc, dans le `<piece>` du membre, ses enfants à lui. Une ligne
    // gelée n'est qu'un enregistrement ; rien ne la dessine ici, et le dessin du groupe ne bouge pas.

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

    OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, geometry.id, false);

    // ── La place Graded, reportée sur CETTE taille ───────────────────────────────────────────
    //
    // The override is a point of the DOCUMENT, arranged in Graded where every size is drawn one on
    // top of the other — one place, valid for all of them. A plan draws ONE size, on its own
    // geometry, with no such alignment: the same point lands elsewhere, and the grain lines come out
    // scattered. The seat is therefore carried over the way Graded carries it — the same relative
    // position inside the piece, measured against the range's reference size, which is where the
    // decor is resolved everywhere else in the application.
    //
    // Without a reference seat the raw point stands: that is the old behaviour, and it is exactly
    // right for a run on the reference size itself.
    if (ov.valid && !referenceSeat.isNull())
    {
        const QRectF now = UnplacedBounds(geometry.piece);
        if (!now.isNull() && !qFuzzyIsNull(referenceSeat.width())
            && !qFuzzyIsNull(referenceSeat.height()))
        {
            const qreal u = (ov.pos.x() - referenceSeat.left()) / referenceSeat.width();
            const qreal v = (ov.pos.y() - referenceSeat.top()) / referenceSeat.height();
            ov.pos = QPointF(now.left() + u * now.width(), now.top() + v * now.height());
        }
    }
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
auto ResolveGroupDecor(const QVector<OExportPieceGeometry> &members, quint32 groupId,
                       const OSizerange *range, const Unit &unit, const VContainer *pattern,
                       VAbstractPattern *doc, OExportFormat::Kind format,
                       const QMap<QString, QString> &sizeAxes) -> OExportGroupDecor
{
    OExportGroupDecor frozen;

    // A cutting plan DECOMPOSES the groups — the kept member carries its own decor and the group draws
    // none. The very test the drawing side makes, asked once, here: nothing to freeze where nothing is
    // drawn.
    if (!IsGroupedMember(pattern, groupId, format))
    {
        return frozen;
    }

    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (groups == nullptr)
    {
        return frozen;
    }

    const auto group = groups->constFind(groupId);
    if (group == groups->cend())
    {
        return frozen;
    }

    // The group's frame, measured exactly as the drawing measures it: the union of its members'
    // unplaced extents, and the centre of that union is what a child that was never placed falls back
    // on. ⚠️ Taken on the FROZEN geometry, so generation and drawing agree about it — and the drawing
    // measures the same union over the same members of the same layer.
    QRectF bounds;
    for (const OExportPieceGeometry &member : members)
    {
        bounds = bounds.united(UnplacedBounds(member.piece));
    }
    const QPointF centerLocal = bounds.center() - bounds.topLeft();

    // ⚠️ **Ce que le document porte n'est PAS la position de la taille du run** — il faut la recalculer.
    //
    // Les enfants d'une PIÈCE sont re-matérialisés à chaque mise à la taille : `VPattern::Parse` les
    // parcourt. Ceux d'un GROUPE, non — le parse ne connaît que `DataPieces()`, et le seul écrivain de
    // leurs positions est `OPieceGroupHolderItem::Refresh`, atteint uniquement par `CheckLayout`, qui ne
    // part jamais en mode Export. La position stockée est donc celle de la dernière fois où l'on a
    // REGARDÉ le groupe dans la scène Details, et l'export la relisait telle quelle.
    //
    // ⚠️ **Et l'origine du détenteur se déduit, elle ne se devine pas.** `RefRect(groupe, data, origine)`
    // translate chaque membre par son `(mx, my)` avant d'unionner, puis retranche `origine`. Le
    // détenteur prend pour origine le coin de l'union **dans les coordonnées de scène** — donc dans cet
    // espace DÉJÀ TRANSLATÉ, qui est exactement ce que rend `RefRect(..., QPointF(0, 0))`. Prendre le
    // coin de l'union des `UnplacedBounds`, qui ne porte pas la translation, plaçait les enfants aux
    // coordonnées de la scène Details — la tentative qui les a sortis du canvas.
    OPieceGroup sized = group.value();
    const QRectF pointsBox = OChildAnchor::RefRect(sized, pattern, QPointF(0.0, 0.0));
    const QRectF ref = OChildAnchor::RefRect(sized, pattern, pointsBox.topLeft());
    OChildAnchor::Materialize(sized, ref);

    // The boxes a miniature nobody placed is measured on, in the frame the group's children are
    // written in: its own member's extent, brought back by the union's corner — never the union, which
    // is what the group's cartouche is centred on. The drawing side builds them the same way, off the
    // same frozen geometry, so screen and paper cannot measure two different boxes.
    QHash<quint32, QRectF> memberBoxes;
    for (const OExportPieceGeometry &member : members)
    {
        memberBoxes.insert(member.id,
                           UnplacedBounds(member.piece).translated(-bounds.topLeft().x(),
                                                                   -bounds.topLeft().y()));
    }

    frozen.owner = ResolveOwnerDecor(sized, true, groupId, centerLocal, qQNaN(), range, pattern, doc,
                                     memberBoxes, format, sizeAxes);

    const VGrainlineData &geom = sized.m_glGrainline;
    if (geom.IsEnabled() && geom.IsVisible())
    {
        const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, groupId, true);
        const OGraded::GrainlinePlacement placement =
            OGraded::ResolveGrainline(geom, ov, true, pattern, unit, centerLocal);
        const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), true, pattern);

        frozen.grainline.resolved = true;
        frozen.grainline.pos = placement.pos;
        frozen.grainline.lengthPx = placement.lengthPx;
        frozen.grainline.rotationDegrees = rotation;
    }

    frozen.resolved = true;
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
                          const OExportRunFlags &run,
                          const OExportPlanSeat &seat)
{
    if (scene == nullptr || !result.ok)
    {
        return;
    }

    // Lu une fois : le reste de cette fonction demande la même chose à quatre endroits, et quatre
    // lectures d'un format sont quatre occasions d'en lire un autre.
    const OExportFormat::Kind format = run.format;

    // ── Où ce plan se pose ────────────────────────────────────────────────────────────────────────
    //
    // Tout ce qui suit est écrit dans le repère du PLAN — le bac part de (0,0) — et c'est cette
    // translation qui l'amène dans la scène. Un run ordinaire a l'origine pour repère : rien ne bouge.
    // Une PAIRE en dessine deux, et c'est le second qui se décale.
    //
    // ⚠️ Elle est POST-multipliée à la matrice de placement et non posée en transformation d'item :
    // un item transformé traînerait sa bounding rect et sa sélection, et la correction — qui lit des
    // transformations de scène — verrait autre chose que ce que le plan mesure.
    const QTransform originMatrix = QTransform::fromTranslate(seat.origin.x(), seat.origin.y());

    // ── Les BANDES du plan plié, et le vide qui les sépare à l'écran ─────────────────────────────
    //
    // ⚠️ **Une bande = un canvas, et la séparation ne vit QUE ici.** Le run additionne des hauteurs,
    // il ne connaît aucun vide : le métrage, `usedHeightPx` et les fichiers DXF/HPGL ne voient rien
    // de ce qui suit. Ce sont deux choses que le dessin décide seul, et il les décide ensemble :
    //
    //   1. chaque bande est un canvas fermé, à ses propres dimensions ;
    //   2. les bandes sont écartées d'un vide lisible, et **leurs pièces suivent leur bande**.
    //
    // ⚠️ **Décaler le cadre sans décaler ses pièces ne veut rien dire** : une pièce sortirait de son
    // canvas. Le décalage est donc une propriété de la BANDE, appliquée au cadre comme aux pièces —
    // d'où ces deux variables déclarées ici, avant les pièces, qui sont dessinées plus bas.
    QVector<QRectF> bandRects;
    QVector<int> placementBand;   ///< la bande de CHAQUE placement, dans l'ordre du run
    qreal bandGapPx = 0.0;    ///< le pas d'une bande à la suivante : le vide, et le titre dedans
    qreal bandLeadPx = 0.0;   ///< la place du titre au-dessus de la PREMIÈRE bande
    // ⚠️ **La bande d'une pièce ne se devine PAS depuis sa géométrie.** `OExportPlacement::y` est
    // le point de placement, pas le haut de la pièce, et `originY`/`extentY` sont mesurés avant
    // rotation : une pièce taillée dans le BIAIS a une boîte tournée qui n'a plus aucun rapport
    // avec eux, et elle était classée dans une autre bande — donc dessinée hors de son canvas.
    // C'est la nidification qui sait à quelle zone elle a confié chaque pièce ; `placementBand` ne
    // fait que lire ce qu'elle a rangé, dans l'ordre où elle l'a rangé.


    // Le premier plan d'un run — et le seul, dans tous les cas sauf la paire — vide la scène. Le
    // second s'ajoute au premier : deux appels, une seule scène.
    if (seat.clearScene)
    {
        scene->clear();
    }

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

    // ⚠️ **L'écart d'alignement de la RÉFÉRENCE, par pièce.** Le bloc (cartouche, annotations,
    // miniatures) est posé une fois pour la plage, dans le repère de référence ; pour le poser sur
    // une autre taille il faut l'y ramener, c'est-à-dire l'écarter de `A_ref − A_taille`. Mesuré sur
    // « Devant x1 pli » : +529,147 ; 76,246 remet la 46 exactement sur le pli de sa propre pièce.
    // Les traits géométriques (droit-fil, pli, légende), eux, n'ont pas besoin de cet écart : ce sont
    // les enfants de la pièce de cette taille, ils sont déjà dans son repère.
    QHash<quint32, QPointF> referenceAlignDelta;
    if (!layers.isEmpty())
    {
        for (const OExportPieceGeometry &geometry : layers.first().pieces)
        {
            referenceAlignDelta.insert(geometry.id, geometry.alignDelta);
        }
    }

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
        MakeArtboardPaper(scene, artboards, sheets, seat.origin);

        // The cover page's own sheet, for the three formats that give it one. **Added here and not
        // through OExportSheets**: it is not a cell of the grid, so the plan's ranks stay 1..n, its
        // page count never sees it, and the arbitration that compares printed sheets cannot be moved
        // by it — which is exactly what the A4/Letter formats need, their nesting being untouched.
        if (coverSeat == OExportCoverPage::Seat::ExtraSheet)
        {
            MakePaper(scene, OExportCoverPage::PaperPx(coverSeat, coverMarginPx, artboards.planRect,
                                                       artboards.cellWidthPx,
                                                       artboards.cellHeightPx),
                      seat.origin);
        }
    }
    else
    {
        // ⚠️ **Pas de bac autour d'un plan à plusieurs bandes.** Ce rectangle enfermerait les vides
        // qui les séparent et redirait, en un seul trait, le contraire de ce que les canvas
        // montrent : chaque bande a le sien, et c'est lui qui porte le fond blanc.
        if (result.foldedZones.size() < 2)
        {
            auto *bin = new QGraphicsRectItem(0, 0, result.binWidthPx, result.usedHeightPx);
            bin->setPen(QPen(Qt::black, 1));
            bin->setBrush(QBrush(Qt::white));
            bin->setZValue(kBinZ);
            bin->setPos(seat.origin);
            scene->addItem(bin);
        }

        // ── La grille des tapis d'un plotter ──────────────────────────────────────────────────────
        //
        // Un tapis est une CELLULE DURE : le logiciel du plotter refuse une forme qui la déborde, et
        // c'est pourquoi l'aperçu la montre — chaque tapis est un chargement de machine, et leur
        // nombre est ce que l'onglet cherche à faire descendre.
        //
        // ⚠️ **Un bord de tapis n'est plus une ligne de coupe.** Il l'était tant que chaque pièce
        // gardait son bloc de tapis et que ses morceaux tombaient sur les bords ; les morceaux se
        // rangent maintenant là où ils tiennent, donc les deux se croisent. Sans conséquence : la
        // découpe est dans la géométrie du morceau, jamais dans le cadre qui l'entoure.
        //
        // Le Z du dessus est celui de la grille d'artboards : les deux disent la même chose, l'endroit
        // où le papier ou le tapis s'arrête, et aucun plan de découpe n'a les deux à la fois.
        //
        // ⚠️ **Affichage seulement** — le fichier est écrit en parcourant le contenu, jamais la scène.
        if (format == OExportFormat::Kind::Plotter && run.matCount > 0 && run.matWidthMm > 0.0
            && run.matHeightMm > 0.0)
        {
            const qreal matWidthPx = ToPixel(run.matWidthMm, Unit::Mm);
            const qreal matHeightPx = ToPixel(run.matHeightMm, Unit::Mm);
            const QSize arrangement = OExportMatPacking::Arrangement(run.matCount);

            QPen matPen(Qt::gray, 1, Qt::DashLine);

            // Un trait par tapis, et **seulement les tapis ouverts** : la dernière rangée d'un
            // rangement compact reste incomplète, et tracer la grille jusqu'au bord du canevas
            // annoncerait un tapis que personne ne chargera.
            for (int index = 0; index < run.matCount; ++index)
            {
                const QRectF tile = OExportMatPacking::TileRect(index, arrangement, matWidthPx,
                                                                matHeightPx);

                auto *item = new QGraphicsRectItem(tile);
                item->setPen(matPen);
                item->setBrush(Qt::NoBrush);
                item->setZValue(kGridZ);
                item->setPos(seat.origin);
                scene->addItem(item);
            }
        }

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
        // unique a une zone — la colonne — dont le pli est **à gauche, et toujours** : le candidat à
        // un bord l'écrit en dur (`PlanFoldedFabric`), et ses pièces au pli se collent à cette paroi.
        // ⚠️ Ce commentaire a affirmé le contraire — « c'est la pièce au pli qui en décide » — et
        // c'était faux : **aucune variabilité de côté n'existe dans le code pour ce cas.** Un plan à
        // deux bords, lui, en a trois, et leurs côtés sont fixés par zone.
        //
        // Et les rectangles ne se dessinent **qu'à partir de deux** : une seule zone EST la
        // colonne, et le trait du bac la dessine déjà. **Noir à 75 %, tirets 20 / 10** (choisi le
        // 25-09) : le pointillé noir à 50 % ne se lisait pas sur le bac blanc, et c'est ce trait qui
        // dit où s'arrête chaque zone. Purement visuel : la ligne du milieu d'un plan à deux bords
        // est celle où les deux lisières se rejoignent, et aucune pièce ne la traverse — non parce
        // que ce trait l'interdit, mais parce que chaque rectangle est un bac à part et que le
        // moteur ne pose jamais hors de son bac.
        //
        // ⚠️ **Les longueurs sont des PIXELS** : `setDashPattern` les prend en multiples de la
        // largeur de plume, qui vaut 1 ici — 20 px de tiret, 10 px d'espace. Et le bout est PLAT,
        // sans quoi un tiret de 20 px en mesurerait 21 et l'espace 9 : c'est la règle que `WriteLine`
        // s'impose déjà, « butt, like the pen the screen draws with ». Vérifié sur banc :
        // `QSvgGenerator` écrit `stroke-dasharray="20,10"` et `stroke-opacity="0.74902"`.
        //
        // Tout part dans l'écran, dans le PNG et dans le SVG : l'export d'image rend la scène. Le
        // plan de coupe n'a pas de PDF — il n'y a donc rien d'autre à nourrir.
        QPen zonePen(QColor(0, 0, 0, 191), 1); // 191 / 255 : le noir à 75 %
        zonePen.setCapStyle(Qt::FlatCap);
        zonePen.setDashPattern({20.0, 10.0});

        // ── Chaque BANDE est son propre CANVAS, et les canvas sont écartés ─────────────────────
        //
        // ⚠️ **Une bande = un canvas.** Ce n'est pas décoratif : c'est la seule façon de lire d'un
        // coup d'œil que le plan vaut la SOMME des hauteurs et non celle d'un bac unique. Le run,
        // lui, ne connaît aucun vide — `usedHeightPx`, le métrage et les fichiers DXF/HPGL sont
        // ceux du plan continu, et rien ici ne les touche.
        //
        // ⚠️ **On regroupe par HAUT de zone.** L/R partagent le même haut : ils sont une bande, donc
        // un canvas. Le pli simple en est une autre, la bande mixte une troisième.
        for (const OExportFoldedZone &zone : result.foldedZones)
        {
            bool merged = false;
            for (QRectF &band : bandRects)
            {
                if (qAbs(band.top() - zone.rect.top()) < 0.5)
                {
                    band = band.united(zone.rect);
                    merged = true;
                    break;
                }
            }
            if (!merged)
            {
                bandRects.append(zone.rect);
            }
        }
        std::sort(bandRects.begin(), bandRects.end(),
                  [](const QRectF &left, const QRectF &right) { return left.top() < right.top(); });

        const qreal cmPerPx = 1.0 / ToPixel(1.0, Unit::Cm);

        // Le titre se dimensionne sur le PLAN, et le vide se dimensionne sur le TITRE : il doit tenir
        // dehors, au-dessus du cadre. C'est cet ordre-là, et c'est pour ça que les deux se calculent
        // après les bandes et avant de les dessiner.
        //
        // ⚠️ **En pixels de SCÈNE, jamais en points.** Un `QGraphicsSimpleTextItem` sans police prend
        // la taille par défaut, rendue à l'échelle de l'écran : sur un plan de deux mètres, le titre
        // faisait trois pixels. Proportionné à la largeur du canvas, il grossit avec le plan.
        qreal widestBand = 0.0;
        for (const QRectF &band : bandRects)
        {
            widestBand = std::max(widestBand, band.width());
        }
        const int labelPx = qMax(12, qRound(widestBand * 0.025));
        // ⚠️ **Aucune place réservée au-dessus de la PREMIÈRE bande — `Canvas = Tissu`.** Le cadre de
        // la première bande commence exactement au bord du tissu, à l'origine du bac. Réserver une
        // hauteur pour son titre décalait ce cadre et faisait grandir tout le dessin : c'est ce que
        // Sophie a vu. Ce n'était pas une séparation, c'était un agrandissement.
        bandLeadPx = 0.0;
        bandGapPx = labelPx * 1.9;    // un cadre, puis son titre, puis le cadre suivant

        // La bande de chaque placement, lue dans l'ordre où le run les a rangés : chaque zone dit
        // combien elle en porte, et la liste des zones suit exactement la liste des placements.
        // Aucune géométrie là-dedans, donc rien que la rotation puisse fausser.
        placementBand.clear();
        placementBand.reserve(result.placements.size());
        for (const OExportFoldedZone &zone : result.foldedZones)
        {
            int band = 0;
            for (int i = 0; i < bandRects.size(); ++i)
            {
                if (qAbs(bandRects.at(i).top() - zone.rect.top()) < 0.5)
                {
                    band = i;
                    break;
                }
            }
            for (int k = 0; k < zone.placementCount; ++k)
            {
                placementBand.append(band);
            }
        }
        // Un compte qui ne retombe pas sur le nombre de placements ne déplace RIEN : mieux vaut
        // une séparation absente qu'une pièce posée dans une bande qui n'est pas la sienne.
        if (placementBand.size() != result.placements.size())
        {
            placementBand.clear();
        }

        for (int index = 0; index < bandRects.size(); ++index)
        {
            const qreal shiftY = bandLeadPx + static_cast<qreal>(index) * bandGapPx;
            const QRectF canvas = bandRects.at(index).translated(0.0, shiftY);

            // Le canvas : un cadre FERMÉ, avec son propre fond blanc. Les bandes ne se touchent plus.
            auto *frame = new QGraphicsRectItem(canvas);
            frame->setPen(QPen(Qt::black, 1));
            frame->setBrush(QBrush(Qt::white));
            frame->setZValue(kBinZ);
            frame->setPos(seat.origin);
            scene->addItem(frame);

            // ── Le titre, PAR SECTION ────────────────────────────────────────────────────────────
            //
            // ⚠️ **Une bande à deux plis a DEUX titres.** Un seul titre pour les deux était
            // mensonger : sur une laize de 140, L/R annonçait « 70 × 54,8 » alors que chaque moitié
            // fait 35 — la largeur affichée ne correspondait à aucune section. Chaque section dit
            // maintenant SA largeur et SA longueur, et chacune se pose au-dessus de sa moitié :
            // **L collé à gauche, R collé à droite**.
            int zoneCount = 0;
            for (const OExportFoldedZone &zone : result.foldedZones)
            {
                if (qAbs(zone.rect.top() - bandRects.at(index).top()) < 0.5)
                {
                    ++zoneCount;
                }
            }

            // Le titre est AU-DESSUS du cadre, et sans fond : pas de rectangle derrière lui, pas de
            // place réservée dans le tissu. Le canvas reste exactement le rectangle de tissu.
            QFont labelFont;
            labelFont.setPixelSize(labelPx);

            for (const OExportFoldedZone &zone : result.foldedZones)
            {
                if (qAbs(zone.rect.top() - bandRects.at(index).top()) >= 0.5)
                {
                    continue;
                }

                const QRectF section = zone.rect.translated(0.0, shiftY);
                auto *label = new QGraphicsSimpleTextItem(
                    QStringLiteral("%1 × %2 cm")
                        .arg(section.width() * cmPerPx, 0, 'f', 1)
                        .arg(section.height() * cmPerPx, 0, 'f', 1));
                label->setFont(labelFont);
                label->setBrush(Qt::black);
                label->setZValue(100.0);   // au-dessus des pièces : un titre se lit

                // ── La flèche de pli, et le bloc qu'elle forme avec le texte ─────────────────────
                //
                // ⚠️ **Un U couché, une SEULE pointe.** Deux traits parallèles qui s'écartent du
                // pli, reliés par un demi-tour posé SUR le bord du pli — le cul —, et la pointe au
                // bout du trait du HAUT, tournée vers l'intérieur : le tissu part, se replie au pli,
                // et revient. Deux pointes liraient « ça va des deux côtés », ce qui n'est pas ce
                // qu'on dessine en couture.
                //
                // ⚠️ **Le bloc est ancré CÔTÉ PLI** : pli à gauche, le U ouvre la marche et le texte
                // le suit ; pli à droite, le texte d'abord et le U ferme la marche, cul au bord.
                const qreal textTop = canvas.top() - labelPx * 1.15;
                const qreal textWidth = label->boundingRect().width();
                const qreal arrowWidth = labelPx * 3.0;   // la largeur se voit : c'est le sens
                const qreal arrowHeight = labelPx * 0.9;
                const qreal gap = labelPx * 0.35;
                const qreal margin = labelPx * 0.2;

                const bool foldOnLeft = zone.foldOnTheLeft;
                qreal arrowLeft = 0.0;
                qreal textLeft = 0.0;
                if (foldOnLeft)
                {
                    arrowLeft = section.left() + margin;
                    textLeft = arrowLeft + arrowWidth + gap;
                }
                else
                {
                    arrowLeft = section.right() - margin - arrowWidth;
                    textLeft = arrowLeft - gap - textWidth;
                }

                label->setPos(seat.origin + QPointF(textLeft, textTop));
                scene->addItem(label);

                // Le U : deux traits parallèles, reliés par le cul posé sur le bord du pli.
                const qreal centreY = textTop + label->boundingRect().height() * 0.5;
                const qreal upperY = centreY - arrowHeight * 0.5;
                const qreal lowerY = centreY + arrowHeight * 0.5;
                const qreal bendX = foldOnLeft ? arrowLeft : arrowLeft + arrowWidth;
                const qreal tipX = foldOnLeft ? arrowLeft + arrowWidth : arrowLeft;
                QPen arrowPen(Qt::black, qMax(1.0, labelPx * 0.09));
                arrowPen.setCapStyle(Qt::FlatCap);

                QPainterPath uTurn;
                uTurn.moveTo(tipX, lowerY);
                uTurn.lineTo(bendX, lowerY);
                uTurn.lineTo(bendX, upperY);
                uTurn.lineTo(tipX, upperY);

                auto *arrow = new QGraphicsPathItem(uTurn);
                arrow->setPen(arrowPen);
                arrow->setBrush(Qt::NoBrush);
                arrow->setZValue(100.0);
                arrow->setPos(seat.origin);
                scene->addItem(arrow);

                // La pointe, au bout du trait du HAUT, tournée vers l'intérieur.
                const qreal direction = foldOnLeft ? 1.0 : -1.0;
                const qreal headLength = labelPx * 0.55;
                const qreal headHalf = labelPx * 0.32;
                QPainterPath head;
                head.moveTo(tipX, upperY);
                head.lineTo(tipX - direction * headLength, upperY - headHalf);
                head.lineTo(tipX - direction * headLength, upperY + headHalf);
                head.closeSubpath();

                auto *arrowHead = new QGraphicsPathItem(head);
                arrowHead->setPen(arrowPen);
                arrowHead->setBrush(QBrush(Qt::black));
                arrowHead->setZValue(100.0);
                arrowHead->setPos(seat.origin);
                scene->addItem(arrowHead);
            }

            // Les zones de la bande, DÉCALÉES avec elle : le trait de pli est une ligne physique, il
            // suit sa bande. Leur contour pointillé ne se dessine qu'à partir de deux — une bande
            // d'une seule zone EST le canvas, son cadre le dit déjà.
            for (const OExportFoldedZone &zone : result.foldedZones)
            {
                if (qAbs(zone.rect.top() - bandRects.at(index).top()) >= 0.5)
                {
                    continue;
                }

                const QRectF outline = zone.rect.translated(0.0, shiftY);
                if (zoneCount > 1)
                {
                    auto *rect = new QGraphicsRectItem(outline);
                    rect->setPen(zonePen);
                    rect->setBrush(Qt::NoBrush);
                    rect->setZValue(kBinZ);
                    rect->setPos(seat.origin);
                    scene->addItem(rect);
                }

                if (!zone.folded)
                {
                    continue;   // une bande plate n'a pas de pli à tracer
                }

                const qreal foldX = zone.foldOnTheLeft ? outline.left() : outline.right();
                auto *fold = new QGraphicsLineItem(foldX, outline.top(), foldX, outline.bottom());
                fold->setPen(QPen(Qt::black, 1, Qt::DashDotLine));
                fold->setZValue(kBinZ);
                fold->setPos(seat.origin);
                scene->addItem(fold);
            }
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

        scene->addItem(new AssemblyMarksItem(*marks, artboards.planRect.topLeft() + seat.origin, grid,
                                             framed));
    }

    // The page itself, over the marks and under the pieces — see kCoverZ for why that band and not
    // another. A roll takes the same path as a sheet: its "plan rect" is the empty one, whose
    // top-left IS the bin's origin, so the block lands at the corner either way.
    if (cover != nullptr && !cover->IsEmpty())
    {
        scene->addItem(new CoverPageItem(
            OExportCoverPage::PagePx(coverSeat, coverMarginPx, artboards.planRect,
                                     artboards.cellWidthPx, artboards.cellHeightPx).translated(seat.origin),
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
                // ⚠️ **Une pièce suit SA bande.** Les canvas sont écartés à l'écran (voir plus
                // haut), et une pièce laissée à sa place vraie sortirait du canvas qui la porte.
                // Le décalage est donc inséré ICI, une fois, dans la matrice de placement : il
                // entraîne le contour, le décor, la grainline et les étiquettes d'un seul coup,
                // parce qu'ils sont tous construits à partir de cette matrice.
                //
                // ⚠️ **Affichage seulement.** Ce décalage ne remonte nulle part : ni dans
                // `result.placements`, ni dans la hauteur, ni dans le métrage, ni dans les fichiers.
                // `i` est l'indice du placement dans le run : c'est lui qui porte la bande, pas la
                // géométrie de la pièce (voir `placementBand` plus haut).
                const QTransform bandMatrix = QTransform::fromTranslate(
                    0.0, bandLeadPx + static_cast<qreal>(placementBand.value(i, 0)) * bandGapPx);
                const QTransform placementMatrix =
                    OExportFootprint::PieceMatrix(placement, geometry.alignDelta) * bandMatrix
                    * originMatrix;

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

                // "Cut on fold" is a caption, and a cutting plan shows none: the mark — the line and
                // its two arrows — stays, the text leaves. Asked of the piece here, on the copy
                // GetItem is about to draw, so the canvas, the image and the PDF lose it together.
                if (format == OExportFormat::Kind::CuttingPlan)
                {
                    placed.SetShowFoldLineLabel(false);
                }

                // The decor is set on the piece before GetItem runs, because GetItem is
                // what builds it — and it is set on the reference layer only.
                if (isReference)
                {
                    ApplyReferenceGrainline(placed, geometry, range, unit, pattern, format);

                    // Et le pli, qui est le décor du GROUPE : seul le membre qu'il désigne le porte.
                    // Ni un plan de coupe ni un plotter ne sont concernés — ils décomposent les
                    // groupes, donc chacun de leurs membres garde le sien.
                    if (format == OExportFormat::Kind::Print)
                    {
                        ApplyGroupFoldLine(placed, geometry, pattern);
                    }
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
                // ── Le tracé : celui du format, ou celui des tailles ──────────────────────────────
                //
                // Un projecteur a **une seule encre** : noir, plein, 7 / 5,25 pt, la même pour
                // toutes les tailles — c'est ce qui rend ses calques interchangeables. Les styles de
                // taille de Graded s'arrêtent donc ici : ils donneraient à chaque taille sa couleur
                // et ses tirets, et deux tailles ne se superposeraient plus de la même façon.
                //
                // ⚠️ Posée sur les ITEMS, par le canvas : le fichier n'a aucune règle de trait à lui
                // et écrit ce qu'il voit, comme pour tous les autres formats. L'écran et le papier
                // ne peuvent donc pas diverger sur l'épaisseur d'un trait.
                if (format == OExportFormat::Kind::Projector)
                {
                    OExportInk::ApplyToSubtree(item);
                }
                else
                {
                    OGradedSizeTrace::ApplyToSubtree(item, styles, OGradedSizeTrace::Scope::FreshItems);
                }

                if (!isReference)
                {
                    HidePerSizeDecor(item);
                }
                else if (IsGroupedMember(pattern, geometry.groupId, format))
                {
                    // A grouped member carries nothing of its own: the group hosts the
                    // single set, grainline included. Its frame is measured here, at the
                    // one moment its unplaced extent is still at hand.
                    DecorFrame &frame = groupFrames[geometry.groupId];
                    frame.placement = placementMatrix;
                    frame.rotation = placement.rotation;
                    frame.bounds = frame.bounds.united(UnplacedBounds(geometry.piece));
                    // The member's own box, kept beside the union: a miniature is measured on the
                    // piece it draws, never on the group. Brought into the group's frame at the end
                    // of the walk, when that frame's origin is known.
                    frame.memberBoxes.insert(geometry.id, UnplacedBounds(geometry.piece));
                    frame.footprintIndex = i;

                    // ── Le pli du groupe : c'est CE membre qui le porte ───────────────────────────
                    //
                    // Le groupe désigne le porteur (`m_foldLineMemberId`) et n'a aucune géométrie de
                    // pli. Son trait est un enfant de l'item du porteur : il se voit au canvas, mais
                    // il n'était écrit nulle part, donc le PDF sortait sans pli là où l'écran en
                    // montrait un. On les ramasse ici, pour les joindre au décor du groupe.
                    //
                    // ⚠️ Un seul membre les donne, et c'est le porteur : c'est la règle du groupe
                    // (`ApplyGroupFoldLine` a déjà éteint celle des autres), et la doubler ici ferait
                    // deux traits.
                    bool carriesFold = false;
                    if (content != nullptr && format != OExportFormat::Kind::Plotter)
                    {
                        const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
                        if (groups != nullptr)
                        {
                            const auto group = groups->constFind(geometry.groupId);
                            carriesFold = group != groups->cend()
                                          && group.value().m_foldLineMemberId == geometry.id;
                        }
                        if (carriesFold)
                        {
                            for (QGraphicsItem *child : item->childItems())
                            {
                                if (IsDecorChild(child))
                                {
                                    frame.foldChildren.append(child);
                                }
                            }
                        }
                    }

                    // ── Les enfants du membre, pour le payload ──────────────────────────────────
                    //
                    // ⚠️ **UNE valeur, pas une par taille.** Les enfants d'un membre n'ont pas de clé
                    // de taille : `pieceContent[pieceId]` porte une position par plage, exprimée dans
                    // le repère local de la pièce, et c'est le déplacement de la vue à chaque taille
                    // (`mx/my + layerDelta + dx/dy`) qui la porte. Autrement dit `alignDelta` fait le
                    // travail côté lecteur.
                    //
                    // ⚠️ **Et ils sont construits ici, sur la couche de référence, avec `asLonePiece`.**
                    // L'item de la couche a été vidé par les règles de dessin du groupe (droit-fil
                    // éteint, pli retiré au membre qui n'est pas porteur) : lire dedans ne donnerait
                    // rien, et le lire par couche décalait le droit-fil une seconde fois — mesuré sur
                    // « Manche » et « Manche poignet ».
                    if (content != nullptr && format != OExportFormat::Kind::Plotter)
                    {
                        OGradedSvg::LayoutObject member;
                        member.layerIndex = 0;
                        CollectMemberOwnDecor(scene, geometry, placementMatrix, placement.rotation, i,
                                              range, unit, pattern, doc,
                                              carriesFold ? frame.foldChildren
                                                          : QVector<QGraphicsItem *>(),
                                              &member);
                        if (!member.decorChildren.isEmpty() || !member.blocks.isEmpty()
                            || !member.annotations.isEmpty() || !member.thumbnails.isEmpty())
                        {
                            content->payloadDecor.append(member);
                            // ⚠️ **Et RIEN dans le DESSIN.** Les enfants d'un membre ne sont pas
                            // une donnée du document : c'est le groupe qui se dessine, et les
                            // membres n'en portent une copie que pour que le payload puisse les
                            // montrer SEULS. Les mettre dans `content.decor` — la liste dont la
                            // couche visible est écrite — les faisait apparaître PAR-DESSUS le
                            // groupe sur le PDF : constaté, et c'était faux.
                            //
                            // Le commentaire d'origine, conservé pour mémoire :
                            // Les membres existent dans chaque calque de taille, mais vides : un
                            // `<g id="Manche">` avec un seul `path`, son contour. Leurs enfants ne
                            // peuvent pas y venir du dessin, parce que les règles du groupe les y
                            // éteignent — `ApplyReferenceGrainline` ne crée même pas l'item du
                            // droit-fil d'un membre, et `ApplyGroupFoldLine` retire le pli au membre
                            // qui n'est pas porteur. `CollectMemberOwnDecor`, juste au-dessus, les
                            // reconstruit depuis la copie de la pièce à cette taille ; il restait à
                            // les **écrire**, et c'est ce qui manquait : le SVG graded est ce que
                            // Rabnest doit lire, pas une seconde description du même dessin.
                            //
                            // Sur la couche de référence seulement : c'est celle qui porte les
                            // enfants, comme pour une pièce seule (`isReference`, plus bas).
                            // (aucun ajout à `content.decor` : voir ci-dessus.)
                        }
                    }

                    // ── Les enfants du membre viennent du MÊME chemin que ceux d'une pièce ─────
                    //
                    // ⚠️ **Ce bloc a été retiré, et c'est le correctif.** Il construisait les enfants
                    // d'un membre par `CollectMemberOwnDecor`, qui les lit dans le DOCUMENT au moment
                    // de la génération — donc à la taille où le document a été remis, une seule fois,
                    // et la même position était recopiée dans les huit tailles. Mesuré sur « Manche »
                    // et « Manche poignet » : des pas de 16, 22, 101, 30, 31, 110, 39 px entre tailles
                    // successives, au lieu du pas constant d'une position calculée par taille.
                    //
                    // Un membre est une pièce : ses enfants sont `geometry.decor`, résolu par couche
                    // comme ceux de toutes les autres — c'est la branche `else if`, plus bas. Le
                    // `<group>` garde les siens, et Rabnest choisit.
                }

                scene->addItem(item);

                if (content != nullptr)
                {
                    itemsByLayer[l].append(item);
                    geometriesByLayer[l].append(&geometry);
                }

                if (isReference && !IsGroupedMember(pattern, geometry.groupId, format))
                {
                    // The decor layer holds the reference size's decor. Its records are
                    // collected here, where the decision to draw it was taken — a second
                    // pass over the scene would have to guess at both.
                    OGradedSvg::LayoutObject entry;
                    entry.name = geometry.name;
                    entry.ownerId = geometry.id;
                    entry.layerIndex = l;
                    // The ink is already placed (the matrix is baked into the piece), so this is
                    // what brings it back into the frame `outline` travels in.
                    entry.ownerPlacement = placementMatrix;
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
                                    pattern, doc, format, content != nullptr ? &entry : nullptr);

                    if (content != nullptr && (!entry.blocks.isEmpty() || !entry.annotations.isEmpty()
                                               || !entry.thumbnails.isEmpty()
                                               || !entry.decorChildren.isEmpty()))
                    {
                        content->decor.append(entry);
                    }
                }
                else if (content != nullptr && format != OExportFormat::Kind::Plotter)
                {
                    // ── Le décor de CETTE couche, pour le payload seul ─────────────────────────
                    //
                    // ⚠️ **Y compris la couche de référence pour un membre de groupe.** Il n'entre
                    // pas dans la branche qui précède — le groupe dessine, pas lui — et il restait
                    // donc sans aucune entrée : sa `<piece>` de référence sortait avec un décor VIDE.
                    // Mesuré : longueur 0 sur « Dos », « Manche », « Manche poignet » et « Test ».
                    //
                    // Graded ne projette pas les enfants d'une taille sur une autre : il construit
                    // ceux de chaque pièce à la taille de sa couche, puis masque ceux qui ne sont pas
                    // la référence (`HidePerSizeDecor`, juste avant). Les items sont donc là, à leur
                    // taille, et il ne reste qu'à les enregistrer — rien n'est dessiné ici : la couche
                    // de décor visible reste celle de la référence, et c'est ce que l'écran montre.
                    //
                    // Les records viennent de `geometry.decor`, gelé pour CETTE taille par
                    // `freezePieceDecor`. Sans eux un membre seul n'aurait que son droit-fil.
                    //
                    // ⚠️ **Et les MEMBRES DE GROUPE passent par ici aussi.** Ils empruntaient
                    // `CollectMemberOwnDecor`, qui construit les enfants depuis le DOCUMENT : les huit
                    // tailles en recevaient donc une copie identique (mesuré : « Dos », « Manche » et
                    // « Manche poignet » à une seule variante, pli dérivant de 282 à 86 px). Un membre
                    // est une pièce comme une autre pour ses traits — c'est son item de couche qui les
                    // porte, à sa taille.
                    OGradedSvg::LayoutObject entry;
                    entry.name = geometry.name;
                    entry.ownerId = geometry.id;
                    entry.layerIndex = l;
                    entry.ownerPlacement = placementMatrix;
                    entry.origin = QPointF(0.0, 0.0);
                    // ⚠️ **Les enfants MASQUÉS comptent, et c'est même le cas courant ici.**
                    // `HidePerSizeDecor` vient de cacher le droit-fil, le pli et sa légende de cette
                    // couche : c'est une décision de DESSIN (une seule taille porte le décor visible),
                    // pas de contenu — et le payload, lui, doit porter les enfants de cette taille.
                    // Sans ce drapeau, chaque taille non-référence sortait sans pli ni légende.
                    // ⚠️ **Sur la couche de référence, l'item de la pièce a été vidé de deux de ses
                    // enfants par des règles de DESSIN** : `ApplyReferenceGrainline` éteint le
                    // droit-fil d'un membre de groupe (`SetEnabled(false)`, puis `GetItem` ne le crée
                    // pas), et `ApplyGroupFoldLine` retire le pli à tout membre qui n'est pas le
                    // porteur désigné. Le payload, lui, doit porter les enfants de la pièce — ce
                    // qu'elle a, pas ce que le dessin a gardé. On lui donne donc sa propre source :
                    // une copie de la pièce, à cette taille, dont les enfants ne sont pas éteints,
                    // ajoutée à la scène masquée pour que ses items vivent jusqu'à l'écriture.
                    QGraphicsItem *source = item;
                    if (isReference && IsGroupedMember(pattern, geometry.groupId, format)
                        && content != nullptr)
                    {
                        // ⚠️ **Aucune substitution de ligne ici.** `geometry.piece` porte la ligne
                        // de la pièce À CETTE TAILLE (matérialisée depuis son ancre à chaque parse) :
                        // c'est elle qu'il faut, pas l'enregistrement gelé, qui est une autre valeur.
                        VLayoutPiece lone = geometry.piece;
                        lone.SetMatrix(placed.GetMatrix());
                        QGraphicsItem *host = lone.GetItem(false, true, false);
                        if (host != nullptr)
                        {
                            host->setVisible(false);
                            scene->addItem(host);
                            source = host;
                        }
                    }

                    for (QGraphicsItem *child : source->childItems())
                    {
                        if (IsDecorChild(child, true))
                        {
                            entry.decorChildren.append(child);
                        }
                    }
                    entry.blocks = geometry.decor.blocks;
                    entry.annotations = geometry.decor.annotations;
                    entry.thumbnails = geometry.decor.thumbnails;
                    // ⚠️ **Aucun décalage ici, et c'est le correctif.** Graded pose le bloc à sa
                    // position LOCALE ; c'est le déplacement de la vue d'une taille à l'autre qui
                    // l'emporte. Le décor étant résolu par taille, le bloc de chaque taille est déjà à
                    // sa place : le décaler de `A_ref − A_taille` le déplaçait de 219 px sur
                    // « Manche » et de 529 px sur « Devant » à la 46 — c'est le décalage constaté,
                    // et il n'avait pas lieu d'être.

                    if (!entry.blocks.isEmpty() || !entry.annotations.isEmpty()
                        || !entry.thumbnails.isEmpty() || !entry.decorChildren.isEmpty())
                    {
                        // ⚠️ **`payloadDecor`, jamais `decor`.** `content.decor` est la liste dont la
                        // couche de décor DESSINÉE est écrite : y mettre les tailles non-référence y
                        // ferait imprimer un cartouche et des annotations par taille, alors que leur
                        // encre d'items est masquée deux lignes plus haut. Rabnest lit les deux
                        // listes fondues, le papier n'en reçoit qu'une.
                        content->payloadDecor.append(entry);
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

        // The group's decor, frozen with the run — see @ref OExportGroupDecor. Absent on a cutting
        // plan, which decomposes the groups, and on any group the run did not resolve.
        const auto frozenEntry = result.groupDecor.constFind(it.key());
        const OExportGroupDecor *frozen =
            frozenEntry == result.groupDecor.cend() ? nullptr : &frozenEntry.value();

        OGradedSvg::LayoutObject entry;
        entry.origin = QPointF(0.0, 0.0);
        BuildGroupDecor(scene, it.key(), frame, frozen, range, unit, pattern, doc, format,
                        content != nullptr ? &entry : nullptr);

        if (content != nullptr && (!entry.blocks.isEmpty() || !entry.annotations.isEmpty()
                                   || !entry.thumbnails.isEmpty()
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
        auto *grid = new ArtboardGridItem(artboards.planRect, artboards.cellWidthPx,
                                          artboards.cellHeightPx, artboards.columns,
                                          artboards.rows, sheets);
        grid->setPos(seat.origin);
        scene->addItem(grid);
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
            const bool grouped = IsGroupedMember(pattern, geometry->groupId, format);
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
