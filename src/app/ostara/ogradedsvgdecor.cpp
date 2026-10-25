/************************************************************************
 **
 **  @file   ogradedsvgdecor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Reference decor resolution for the graded "Layout" layer (see
 **          header). Mirrors the live Graded canvas: group members' own decor
 **          is masked in favour of the group's, and every child is placed at
 **          its RANGE position (overrides of the exported range win, otherwise
 **          the resolved design geometry/centres) — never at the static
 **          Details-mode GetItem positions.
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

#include "ogradedsvgdecor.h"
#include "ogradedgeometry.h"

#include "mainwindow_graded_helpers.h"
#include "osvgutils.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"
#include "widgets/InfoBlock/oinfoblockexport.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/oannotationdata.h"
#include "../vpatterndb/oinfoblockdata.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/othumbnaildata.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vpiecegrainline.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QList>
#include <QMap>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QtMath>

namespace OGradedDecor
{
namespace
{
/// Pixel-per-cm at the SVG 96 dpi reference (1 cm = 96/2.54 px).
constexpr qreal kPxPerCm = 96.0 / 2.54;

//---------------------------------------------------------------------------------------------------------------------
/// Rotation of an annotation: formula for a piece, literal for a group.
auto RotationDeg(const QString &formula, bool isGroup, const VContainer *pattern) -> qreal
{
    if (formula.isEmpty())
    {
        return 0.0;
    }
    if (isGroup)
    {
        bool ok = false;
        const qreal value = formula.toDouble(&ok);
        return ok ? value : 0.0;
    }
    Calculator cal;
    const qreal value = cal.EvalFormula(pattern->DataVariables(), formula);
    return (qIsNaN(value) || qIsInf(value)) ? 0.0 : value;
}

//---------------------------------------------------------------------------------------------------------------------
/// Build the grainline path item (a VGraphicsFillItem, like the canvas/holder)
/// parented to @p parentItem, anchored at @p pos (parent-local).
auto MakeGrainlineItem(QGraphicsItem *parentItem, const QPointF &pos, qreal rotationDeg,
                       qreal lengthPx, GrainlineArrowDirection arrowType) -> QGraphicsItem *
{
    const VPieceGrainline grainline(pos, lengthPx, qDegreesToRadians(rotationDeg), arrowType);
    const GrainlineShape shape = grainline.Shape();
    if (shape.isEmpty())
    {
        return nullptr;
    }
    auto *item = new VGraphicsFillItem(VLayoutPiece::GrainlinePath(shape), parentItem);
    item->SetCustomPen(true);
    QPen pen = item->pen();
    pen.setWidthF(VAbstractApplication::VApp()->Settings()->WidthHairLine());
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    item->setPen(pen);
    return item;
}

//---------------------------------------------------------------------------------------------------------------------
/// Length in px of a grainline (override length wins, else the design length).
auto GrainlineLengthPx(const VGrainlineData &geom, const OSizerange::GrainlineOverride &ov) -> qreal
{
    if (ov.valid && ov.lengthPx > 0.0)
    {
        return ov.lengthPx;
    }
    const qreal cm = geom.GetLength().isEmpty() ? 10.0 : geom.GetLength().toDouble();
    return cm * kPxPerCm;
}

//---------------------------------------------------------------------------------------------------------------------
/// Records of the info blocks of one owner (piece or group): the classic Details
/// exporter geometry, PLUS the exported range's block overrides (a move/resize done
/// in Graded) so the block sits where it is shown on the range's canvas.
auto OwnerBlocks(const OInfoBlockData &info, const OSizerange::PieceContent &pc,
                 const OSizerange::GroupContent &gc, bool isGroup,
                 const OInfoBlockEffectiveState &state, const QPointF &centerLocal)
    -> QVector<OInfoBlockSvg::Block>
{
    // ⚠️ **La MÊME résolution que le payload PDF, pas une copie.** `OGraded::ResolveInfoBlock`
    // (ogradedgeometry.cpp) est le seul endroit qui décide où va un bloc : override de plage s'il
    // existe, géométrie enregistrée sinon, et empilement sous le centre du propriétaire en dernier
    // recours. L'export SVG en avait sa propre copie, et c'est cette duplication qui faisait
    // diverger le SVG de ce que le canvas montre — le bloc, l'annotation, la miniature et le
    // droit-fil partaient du même côté, alors que le trait de pli, écrit depuis l'item, restait
    // juste. Une fonction, deux sorties : elles ne peuvent plus se contredire.
    QVector<OInfoBlockSvg::Block> out;
    const QList<int> blockIndexes = state.blockContent.keys();
    for (int blockIndex : blockIndexes)
    {
        const QVector<OCartoucheRenderLine> lines = state.blockContent.value(blockIndex).toVector();
        if (lines.isEmpty())
        {
            continue;
        }

        const QHash<int, OSizerange::BlockOverride> &ovs = isGroup ? gc.blocks : pc.blocks;
        const auto oit = ovs.constFind(blockIndex);
        const OGraded::InfoBlockPlacement placement =
            OGraded::ResolveInfoBlock(info, oit == ovs.cend() ? OSizerange::BlockOverride{}
                                                              : oit.value(),
                                      lines, blockIndex, centerLocal);

        OInfoBlockSvg::Block block;
        block.lines = lines;
        block.fontScale = placement.fontScale;
        block.rect = QRectF(placement.topLeft, placement.size);
        block.rotation = placement.rotation;
        out.append(block);
    }
    return out;
}

//---------------------------------------------------------------------------------------------------------------------
/// Records of the floating annotations of one owner: classic Details geometry,
/// PLUS the exported range's annotation overrides (guarded by the annotation-list
/// fingerprint) so a moved/rotated annotation sits where it is shown on the range.
auto OwnerAnnotations(const OAnnotationData &annData, bool isGroup, const OSizerange::PieceContent &pc,
                      const OSizerange::GroupContent &gc, const VContainer *pattern,
                      const QPointF &centerLocal) -> QVector<OInfoBlockSvg::Annotation>
{
    QVector<OInfoBlockSvg::Annotation> out;
    const QList<OAnnotation> annotations = annData.Annotations();
    out.reserve(annotations.size());
    const OAnnotationStyleResolver &resolver = VToolSeamAllowance::AnnotationStyleResolver();
    const QHash<int, OSizerange::AnnotationOverride> &ovs =
        isGroup ? gc.annotations : pc.annotations;
    int index = 0;
    for (const OAnnotation &annotation : annotations)
    {
        if (annotation.text.isEmpty())
        {
            ++index;
            continue;
        }
        OAnnotationGeom geom = annotation.geom;
        const auto oit = ovs.constFind(index);
        if (oit != ovs.cend() && oit->valid)
        {
            geom = oit->geom;  // pos + boxWidth/boxHeight + fontScale (range move/resize)
        }

        const OAnnotationRenderStyle style =
            resolver ? resolver(annotation.templateName) : OAnnotationRenderStyle();

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
        const QFontMetricsF fm(font);
        const QRectF br = fm.boundingRect(QRectF(0.0, 0.0, 0.0, 0.0), Qt::AlignLeft, annotation.text);

        const qreal fontScale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
        const qreal contentW = br.width() * fontScale;
        const qreal contentH = br.height() * fontScale;
        qreal w = geom.boxWidth;
        qreal h = geom.boxHeight;
        w = qFuzzyIsNull(w) ? contentW : qMax(w, contentW);
        h = qFuzzyIsNull(h) ? contentH : qMax(h, contentH);

        QPointF topLeft;
        if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
        {
            topLeft = centerLocal;  // never moved: auto-place, centered
        }
        else
        {
            topLeft = geom.pos;
        }
        const qreal rotation = RotationDeg(annotation.rotationFormula, isGroup, pattern);

        OInfoBlockSvg::Annotation ann;
        ann.text = annotation.text;
        ann.fontFamily = style.fontFamily;
        ann.fontFace = style.fontFace;
        ann.fontSizePx = style.fontSize;
        ann.textColor = style.textColor.isEmpty() ? QStringLiteral("#000000") : style.textColor;
        ann.backgroundColor = style.backgroundColor;
        ann.fontScale = fontScale;
        ann.rotation = rotation;
        ann.rect = QRectF(topLeft, QSizeF(w, h));
        out.append(ann);
        ++index;
    }
    return out;
}

//---------------------------------------------------------------------------------------------------------------------
/// The miniature of one owner — a lone piece's own, or the one a group carries for a member.
///
/// **The same three sources the live canvas reads**, in the same order: the drawing comes from the
/// tool at the PATTERN's reference size (never from the layer this run happens to be standing on),
/// the frame from the range when the user moved it there and from the piece's own geometry
/// otherwise, and the box an unplaced frame is measured against from the member's own rectangle in
/// the owner's frame. Where the canvas and the file read one computation, they cannot disagree.
void AppendThumbnail(QVector<OInfoBlockSvg::Thumbnail> &out, quint32 pieceId, const VPiece &piece,
                     const OThumbnailData &stored, const OSizerange::ThumbnailOverride &override,
                     const QRectF &pieceBox)
{
    if (!stored.IsActive())
    {
        return;  // nothing asked for, in Details or in the group
    }
    OThumbnailGeom geom;
    if (override.valid)
    {
        geom.pos = override.pos;
        geom.width = override.width;
        geom.height = override.height;
    }
    else
    {
        geom = stored.Geom();
    }
    OInfoBlockSvg::Thumbnail record =
        OInfoBlockSvg::BuildThumbnail(pieceId, piece, stored, geom, pieceBox,
                                      override.valid || stored.HasGeometry());
    if (record.content.isEmpty())
    {
        // The tool is gone, or the reference drawing was never captured: the canvas draws no
        // miniature either, and a block with nothing in it is not a block this file should carry.
        return;
    }
    out.append(record);
}

//---------------------------------------------------------------------------------------------------------------------
/// Reference (first size) item of a piece in a scene, or nullptr.
auto ReferenceItemOf(QGraphicsScene *scene, quint32 pieceId) -> QGraphicsItem *
{
    const auto items = scene->items();
    for (auto *item : items)
    {
        if (GradedPieceId(item) == pieceId && GradedIsReference(item))
        {
            return item;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
/// The MEMBER VIEW of a grouped member — the copy Graded stands beside its group, at the place
/// Details gives it (`kGradedMemberViewRole`, `mainwindow_graded_helpers.h:74`), or nullptr.
///
/// ⚠️ **A grouped piece has two carriers on the reference layer, and they are not at the same
/// place.** Its stack carries the piece id and stands at the group's stacking position — that is
/// what the group draws and what prints. Its member view is a second render, movable, standing in
/// the workshop row where Details puts it, and it is the one that carries the member's own
/// children. Reading the stack for a member (as `ReferenceItemOf` does) builds its decor on the
/// group and the two drawings land on top of each other: mesuré sur « Manche » et « Manche
/// poignet », superposés au groupe au lieu d'être à côté de lui.
auto MemberViewOf(QGraphicsScene *scene, quint32 pieceId) -> QGraphicsItem *
{
    const auto items = scene->items();
    for (auto *item : items)
    {
        if (item->data(kGradedMemberViewRole).toUInt() == pieceId)
        {
            return item;
        }
    }
    return nullptr;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
QList<OGradedSvg::LayoutObject> BuildReferenceDecor(QGraphicsScene *scene, const OSizerange &range,
                                                    const VContainer *pattern, VAbstractPattern *doc,
                                                    const QList<quint32> &lonePieceIds,
                                                    const QList<quint32> &groupIds)
{
    QList<OGradedSvg::LayoutObject> out;
    if (scene == nullptr || pattern == nullptr || doc == nullptr)
    {
        return out;
    }

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();

    // Map every piece to its group (>= 2 members), like the canvas.
    QHash<quint32, quint32> pieceGroupOf;
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            if (git.value().pieceIds.size() < 2)
            {
                continue;
            }
            for (quint32 memberId : git.value().pieceIds)
            {
                pieceGroupOf.insert(memberId, git.key());
            }
        }
    }

    // Mask the reference items of group members: hide their own grainline and their
    // fold/text unless the member carries the group's fold — the group shows its own decor.
    for (auto it = pieceGroupOf.cbegin(); it != pieceGroupOf.cend(); ++it)
    {
        QGraphicsItem *const ref = ReferenceItemOf(scene, it.key());
        if (ref == nullptr)
        {
            continue;
        }
        const OPieceGroup &group = groups->value(it.value());
        const bool foldCarrier = (group.m_foldLineMemberId == it.key());
        const auto children = ref->childItems();
        for (QGraphicsItem *child : children)
        {
            if (child->type() == VGraphicsFillItem::Type)
            {
                child->hide();
            }
            else if (!foldCarrier && (child->type() == VGraphicsFoldLineItem::Type
                                      || child->type() == QGraphicsSimpleTextItem::Type))
            {
                child->hide();
            }
        }
    }

    // --- Le décor d'UNE pièce, seule ou membre d'un groupe -------------------
    //
    // ⚠️ **Un membre EST une pièce, et le SVG doit le porter comme telle.** Graded ne dessine
    // que le décor du groupe sur sa couche de référence : les enfants d'un membre y sont
    // éteints par les règles du groupe — `ApplyReferenceGrainline` ne crée même pas son
    // droit-fil, `ApplyGroupFoldLine` lui retire le pli. Le SVG graded sortait donc avec
    // `<g id="Manche">` réduit à son contour, et Rabnest, qui doit pouvoir poser un membre
    // SEUL, n'avait rien à lire. On rallume ces enfants le temps de les écrire, puis on les
    // remasque : le dessin garde son apparence, il cesse seulement d'être muet.
    const auto buildOwner = [&](quint32 pieceId) -> OGradedSvg::LayoutObject
    {
        OGradedSvg::LayoutObject object;
        if (piecesMap == nullptr || !piecesMap->contains(pieceId))
        {
            return object;
        }
        // ⚠️ **La VUE du membre, jamais son stack.** Un membre de groupe a deux porteurs sur la
        // couche de référence : son stack, qui garde l'id de la pièce et qui est posé à
        // l'empilement du groupe — c'est le dessin du groupe — et sa vue, une seconde rendue
        // mobile, posée là où Details le met. C'est la vue qui porte les enfants du membre, et
        // c'est sa position que le SVG doit écrire : lire le stack superposait le membre au
        // groupe.
        QGraphicsItem *const ref = pieceGroupOf.contains(pieceId)
                                       ? MemberViewOf(scene, pieceId)
                                       : ReferenceItemOf(scene, pieceId);
        if (ref == nullptr)
        {
            return object;
        }
        const VPiece &piece = piecesMap->value(pieceId);
        const auto pit = range.pieceContent.constFind(pieceId);
        const OSizerange::PieceContent &content =
            pit == range.pieceContent.cend() ? OSizerange::PieceContent{} : pit.value();

        object.name = piece.GetName();
        object.origin = ref->mapToScene(QPointF(0.0, 0.0));

        // Hide the static Details grainline child; it is replaced below at the range position.
        const auto children = ref->childItems();
        for (QGraphicsItem *child : children)
        {
            if (child->type() == VGraphicsFillItem::Type)
            {
                child->hide();
            }
        }

        // Les enfants qu'un membre ne montre pas sur la couche de référence, rallumés le temps
        // de les écrire — le pli et sa légende, que le groupe a masqués au membre qui ne porte
        // pas son axe.
        QVector<QGraphicsItem *> dimmed;
        for (QGraphicsItem *child : ref->childItems())
        {
            if (!child->isVisible()
                && (child->type() == VGraphicsFoldLineItem::Type
                    || child->type() == QGraphicsSimpleTextItem::Type))
            {
                child->show();
                dimmed.append(child);
            }
        }

        // Grainline at its RANGE position (override | design geometry | centre).
        const VGrainlineData geom = piece.GetGrainlineGeometry();
        if (geom.IsEnabled() && geom.IsVisible())
        {
            QPointF pos = content.grainline.valid ? content.grainline.pos : geom.GetPos();
            if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
            {
                pos = ref->boundingRect().center();  // parent-local default
            }
            bool ok = false;
            const QString rotText = geom.GetRotation();
            const qreal rotation = rotText.isEmpty() ? 0.0 : rotText.toDouble(&ok);
            MakeGrainlineItem(ref, pos, ok ? rotation : 0.0, GrainlineLengthPx(geom, content.grainline),
                              geom.GetArrowType());
        }

        // Remaining visible decor children (the rebuilt grainline + fold + fold text).
        for (QGraphicsItem *child : ref->childItems())
        {
            if (child->isVisible() && OSvg::IsInfoChild(child))
            {
                object.decorChildren.append(child);
            }
        }

        // Info blocks + annotations (range overrides win over the design geometry).
        const OInfoBlockEffectiveState state =
            OInfoBlockResolver::ResolveEffectiveState(piece, doc, pattern, range.ReferenceAxes());
        const QPointF centerLocal = ref->boundingRect().center();
        object.blocks =
            OwnerBlocks(piece.GetInfoBlockData(), content, {}, false, state, centerLocal);
        object.annotations =
            OwnerAnnotations(piece.GetAnnotationData(), false, content, {}, pattern, centerLocal);
        // The piece's own miniature, in its own frame — the very frame Details stores it in, so a
        // range needs no conversion to apply it. The box an unplaced frame is measured against is
        // the piece's own rectangle, which is what the canvas measures too.
        AppendThumbnail(object.thumbnails, pieceId, piece, piece.GetThumbnailData(), content.thumbnail,
                        ref->boundingRect());

        // L'apparence du dessin est rendue intacte : ce qui a été rallumé est remasqué.
        for (QGraphicsItem *child : dimmed)
        {
            child->hide();
        }
        return object;
    };

    // --- Pièces seules --------------------------------------------------------
    for (quint32 pieceId : lonePieceIds)
    {
        if (pieceGroupOf.contains(pieceId))
        {
            continue;
        }
        const OGradedSvg::LayoutObject object = buildOwner(pieceId);
        if (!object.name.isEmpty())
        {
            out.append(object);
        }
    }

    // --- Groups --------------------------------------------------------------
    for (quint32 groupId : groupIds)
    {
        if (groups == nullptr || !groups->contains(groupId))
        {
            continue;
        }
        const OPieceGroup &group = groups->value(groupId);
        const auto git = range.groupContent.constFind(groupId);
        const OSizerange::GroupContent &content =
            git == range.groupContent.cend() ? OSizerange::GroupContent{} : git.value();

        // Union box of the reference members → group origin + union centre.
        QRectF box;
        for (quint32 memberId : group.pieceIds)
        {
            QGraphicsItem *const ref = ReferenceItemOf(scene, memberId);
            if (ref == nullptr)
            {
                continue;
            }
            const QRectF r = ref->sceneBoundingRect();
            box = box.isNull() ? r : box.united(r);
        }
        if (box.isNull())
        {
            continue;
        }

        OGradedSvg::LayoutObject object;
        object.name = group.name;
        object.origin = box.topLeft();
        const QPointF unionCenterLocal(box.width() / 2.0, box.height() / 2.0);

        // Group grainline (shared, at its range position): plain path hosted at the union origin.
        const VGrainlineData geom = group.m_glGrainline;
        if (geom.IsEnabled() && geom.IsVisible())
        {
            QPointF pos = content.grainline.valid ? content.grainline.pos : geom.GetPos();
            if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
            {
                pos = unionCenterLocal;
            }
            bool ok = false;
            const QString rotText = geom.GetRotation();
            const qreal rotation = rotText.isEmpty() ? 0.0 : rotText.toDouble(&ok);
            auto *host = new QGraphicsPathItem();
            host->setPos(box.topLeft());
            scene->addItem(host);
            QGraphicsItem *const gl =
                MakeGrainlineItem(host, pos, ok ? rotation : 0.0,
                                  GrainlineLengthPx(geom, content.grainline), geom.GetArrowType());
            if (gl != nullptr)
            {
                object.decorChildren.append(gl);
            }
        }

        // The group's fold lives on the fold-carrier member (kept visible above).
        for (quint32 memberId : group.pieceIds)
        {
            if (group.m_foldLineMemberId != memberId)
            {
                continue;
            }
            QGraphicsItem *const ref = ReferenceItemOf(scene, memberId);
            if (ref == nullptr)
            {
                continue;
            }
            const auto children = ref->childItems();
            for (QGraphicsItem *child : children)
            {
                if (child->isVisible()
                    && (child->type() == VGraphicsFoldLineItem::Type
                        || child->type() == QGraphicsSimpleTextItem::Type))
                {
                    object.decorChildren.append(child);
                }
            }
        }

        // Group info blocks + annotations (union centre, range overrides win).
        const OInfoBlockEffectiveState state =
            OInfoBlockResolver::ResolveEffectiveState(group, doc, pattern, range.ReferenceAxes());
        object.blocks =
            OwnerBlocks(group.GetInfoBlockData(), {}, content, true, state, unionCenterLocal);
        object.annotations =
            OwnerAnnotations(group.GetAnnotationData(), true, {}, content, pattern, unionCenterLocal);

        // The group's miniatures, in the group's own frame: one per member, exactly as the live
        // holder draws them. Each is measured against its own member's box — never the group's
        // union — so an unplaced frame lands on the piece it draws wherever the group sits.
        for (quint32 memberId : group.pieceIds)
        {
            QGraphicsItem *const memberItem = ReferenceItemOf(scene, memberId);
            if (memberItem == nullptr || piecesMap == nullptr || !piecesMap->contains(memberId))
            {
                continue;  // a member with no reference layer: nothing to measure a frame on
            }
            const OSizerange::ThumbnailOverride memberOverride =
                content.memberThumbnails.value(memberId);
            AppendThumbnail(object.thumbnails, memberId, piecesMap->value(memberId),
                            group.MemberThumbnail(memberId), memberOverride,
                            memberItem->sceneBoundingRect().translated(-box.x(), -box.y()));
        }
        out.append(object);

        // ⚠️ **Et CHAQUE MEMBRE, comme une pièce.** Le groupe porte son dessin commun — un
        // cartouche, un droit-fil pour ses membres — et c'est ce que l'impression montre. Mais
        // un membre affiché SEUL porte le sien, et Rabnest ne peut le poser que s'il est dans le
        // SVG. Sans ces objets, `Manche` et `Manche poignet` sortaient en contours nus.
        for (quint32 memberId : group.pieceIds)
        {
            const OGradedSvg::LayoutObject memberObject = buildOwner(memberId);
            if (!memberObject.name.isEmpty())
            {
                out.append(memberObject);
            }
        }
    }

    return out;
}
}  // namespace OGradedDecor
