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
#include "mainwindow_graded_helpers.h"
#include "ogradedgeometry.h"
#include "ogradedsizetrace.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"
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
    ArtboardGridItem(const QRectF &planRect, qreal cellWidthPx, qreal cellHeightPx, int columns, int rows)
      : m_planRect(planRect), m_cellWidthPx(cellWidthPx), m_cellHeightPx(cellHeightPx),
        m_columns(columns), m_rows(rows)
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
    }

private:
    /// The bin's own pen, so a cut line and the plan's border read as one drawing.
    static constexpr qreal kPenWidth = 1.0;

    QRectF m_planRect;
    qreal m_cellWidthPx{0.0};
    qreal m_cellHeightPx{0.0};
    int m_columns{0};
    int m_rows{0};
};

/// The paper of a tiled run: the whole sheet area, white, and nothing drawn on it.
///
/// The pen is dropped on purpose. A QGraphicsRectItem carries a black one-pixel pen by
/// default, and ArtboardGridItem already draws the plan's outer edge — the two would land on
/// the same line, one drawn twice.
///
/// The rect is the plan's and nothing else: a carrier the scene can see but the eye cannot is
/// what feeds VisibleItemsBoundingRect, and a rect wider than the paper would leave the plan
/// pinned to one edge of the viewport when the result is framed. MakeDecorHost tells the same
/// story from the other end.
void MakeArtboardPaper(QGraphicsScene *scene, const QRectF &planRect)
{
    auto *paper = new QGraphicsRectItem(planRect);
    paper->setPen(Qt::NoPen);
    paper->setBrush(QBrush(Qt::white));
    paper->setFlag(QGraphicsItem::ItemIsSelectable, false);
    paper->setFlag(QGraphicsItem::ItemIsMovable, false);
    paper->setAcceptedMouseButtons(Qt::NoButton);
    paper->setZValue(kPaperZ);
    scene->addItem(paper);
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
auto IsGroupedMember(const VContainer *pattern, quint32 groupId) -> bool
{
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
                             const OSizerange *range, const Unit &unit, const VContainer *pattern)
{
    VPieceGrainline &grainline = placed.GetGrainline();

    if (IsGroupedMember(pattern, geometry.groupId))
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
    scene->addItem(host);
    return host;
}

//---------------------------------------------------------------------------------------------------------------------
/// The mode shows the decor, it never edits it. Both item classes draw their eight resize
/// handles under `m_active` alone — SetReadOnly only drops the movable flag — so an item
/// left active would still paint them and answer the mouse. Manual correction is a subject
/// of its own, with its own spec.
template <typename DecorItem> void MakeDecorReadOnly(DecorItem *item)
{
    item->SetActive(false);
    item->SetReadOnly(true);
    item->setAcceptedMouseButtons(Qt::NoButton);
    item->show();
}

//---------------------------------------------------------------------------------------------------------------------
/// Add an owner's information blocks and annotations under @p host, in @p host's frame.
///
/// The owner is the group when the piece belongs to one — a grouped member carries none of
/// these, exactly as it carries no grainline of its own. Details, the Graded canvas and its
/// SVG export all draw that same line.
///
/// @param centerLocal the point a child that was never placed falls back on, in that frame
auto BuildOwnerDecor(QGraphicsItem *host, const IPieceInfo &owner, bool isGroup, quint32 ownerId,
                     const QPointF &centerLocal, const OSizerange *range, const VContainer *pattern,
                     VAbstractPattern *doc, OGradedSvg::LayoutObject *decor = nullptr)
{
    if (pattern == nullptr)
    {
        return;
    }

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

            auto *item = new OInfoBlockItem(block, host);
            item->SetBlockGeom(geom);
            item->SetContent(lines, placement.fontScale);
            MakeDecorReadOnly(item);

            if (decor != nullptr)
            {
                // The same four values the item was just given, in the record form the SVG
                // writer takes. Read off `placement` rather than off the item: the item
                // resolves its own layout, and asking it back would be asking a drawing
                // for the numbers it was drawn from.
                OInfoBlockSvg::Block record;
                record.lines = lines;
                record.fontScale = placement.fontScale;
                record.rect = QRectF(placement.topLeft, placement.size);
                record.rotation = placement.rotation;
                decor->blocks.append(record);
            }
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

        auto *item = new OAnnotationItem(i, host);
        item->SetIndex(i);
        // SetContent before SetAnnotationGeom: an auto-sized box takes its size from the
        // natural size SetContent measured. Details and Graded both call them in this order.
        item->SetContent(annotation.text, style);
        item->SetAnnotationGeom(geom, rotation);
        MakeDecorReadOnly(item);

        if (decor != nullptr)
        {
            // An auto-sized box takes its size from the natural size of its text — the
            // same arithmetic OGradedDecor does for the graded SVG. It is written twice
            // because the two live in different translation units; if they ever disagree,
            // a cartouche would sit differently on screen and on paper, which is the one
            // thing this whole path exists to prevent.
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

            OInfoBlockSvg::Annotation record;
            record.text = annotation.text;
            record.fontFamily = style.fontFamily;
            record.fontWeight = style.fontWeight;
            record.fontSizePx = style.fontSize;
            record.textColor = style.textColor.isEmpty() ? QStringLiteral("#000000") : style.textColor;
            record.backgroundColor = style.backgroundColor;
            record.fontScale = fontScale;
            record.rotation = rotation;
            record.rect = QRectF(geom.pos, QSizeF(w, h));
            decor->annotations.append(record);
        }
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
                     const QTransform &placement, qreal rotation, const OSizerange *range,
                     const VContainer *pattern, VAbstractPattern *doc,
                     OGradedSvg::LayoutObject *decor = nullptr)
{
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

    DecorFrame frame;
    frame.placement = placement;
    frame.rotation = rotation;
    frame.origin = QPointF(0.0, 0.0);
    frame.bounds = UnplacedBounds(geometry.piece);

    auto *host = MakeDecorHost(scene, frame);
    if (decor != nullptr)
    {
        // The carrier's own transform, which is what maps this owner's frame into the bin.
        // The records below are written in that frame; the items the carrier holds are not,
        // they are already placed — hence two wrappers in the SVG.
        decor->transform = host->sceneTransform();
    }
    BuildOwnerDecor(host, piece.value(), false, geometry.id, frame.bounds.center() - frame.origin, range,
                    pattern, doc, decor);

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

    BuildOwnerDecor(host, group.value(), true, groupId, centerLocal, range, pattern, doc, decor);

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
void OExportCanvas::Build(QGraphicsScene *scene,
                          const QVector<OExportSizeLayer> &layers,
                          const OExportNestingResult &result,
                          const OSizerange *range,
                          const VContainer *pattern,
                          VAbstractPattern *doc,
                          OExportCanvasContent *content,
                          const OExportArtboards::Grid &artboards)
{
    if (scene == nullptr || !result.ok)
    {
        return;
    }

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
    if (artboards.valid)
    {
        MakeArtboardPaper(scene, artboards.planRect);
    }
    else
    {
        auto *bin = new QGraphicsRectItem(0, 0, result.binWidthPx, result.usedHeightPx);
        bin->setPen(QPen(Qt::black, 1));
        bin->setBrush(QBrush(Qt::white));
        bin->setZValue(kBinZ);
        scene->addItem(bin);
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

                // The engine places a footprint by the min corner of its unrotated polygon,
                // so the piece has to be brought back to that origin before being moved —
                // exactly what the nested SVG writes as translate(x, y) rotate(r)
                // translate(-ox, -oy). The piece's alignment shift belongs to that origin,
                // not after the rotation: it is part of where the piece sits in the
                // document, and the engine measured the footprint with it already applied.
                QTransform placementMatrix;
                placementMatrix.translate(placement.x, placement.y);
                placementMatrix.rotate(placement.rotation);
                placementMatrix.translate(-placement.originX + geometry.alignDelta.x(),
                                          -placement.originY + geometry.alignDelta.y());

                // The placement is baked into the piece's own matrix rather than into a
                // transform on the item: an item transform would drag the bounding rect,
                // the selection and the rubber band along with it.
                VLayoutPiece placed = geometry.piece;
                placed.SetMatrix(placementMatrix);

                // The decor is set on the piece before GetItem runs, because GetItem is
                // what builds it — and it is set on the reference layer only.
                if (isReference)
                {
                    ApplyReferenceGrainline(placed, geometry, range, unit, pattern);
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
                OGradedSizeTrace::ApplyToSubtree(item, styles, OGradedSizeTrace::Scope::FreshItems);

                if (!isReference)
                {
                    HidePerSizeDecor(item);
                }
                else if (IsGroupedMember(pattern, geometry.groupId))
                {
                    // A grouped member carries nothing of its own: the group hosts the
                    // single set, grainline included. Its frame is measured here, at the
                    // one moment its unplaced extent is still at hand.
                    DecorFrame &frame = groupFrames[geometry.groupId];
                    frame.placement = placementMatrix;
                    frame.rotation = placement.rotation;
                    frame.bounds = frame.bounds.united(UnplacedBounds(geometry.piece));
                }

                scene->addItem(item);

                if (content != nullptr)
                {
                    itemsByLayer[l].append(item);
                    geometriesByLayer[l].append(&geometry);
                }

                if (isReference && !IsGroupedMember(pattern, geometry.groupId))
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

                    BuildPieceDecor(scene, geometry, placementMatrix, placement.rotation, range, pattern,
                                    doc, content != nullptr ? &entry : nullptr);

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
    if (artboards.valid)
    {
        scene->addItem(new ArtboardGridItem(artboards.planRect, artboards.cellWidthPx,
                                            artboards.cellHeightPx, artboards.columns,
                                            artboards.rows));
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
            const bool grouped = IsGroupedMember(pattern, geometry->groupId);
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
