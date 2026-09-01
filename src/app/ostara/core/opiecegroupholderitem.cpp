/************************************************************************
 **
 **  @file   opiecegroupholderitem.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Canvas holder that renders a piece group's common children.
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

#include "opiecegroupholderitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QUndoStack>

#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/movepiece.h"
#include "../vtools/undocommands/savepiecegroupoptions.h"
#include "../vwidgets/vgrainlineitem.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto GroupCenter(const OPieceGroup &group) -> QPointF
{
    for (quint32 pieceId : group.pieceIds)
    {
        try
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
            if (tool != nullptr)
            {
                return tool->sceneBoundingRect().center();
            }
        }
        catch (const VExceptionBadId &)
        {
            continue;  // stale member id (hand-edited file) — skip it
        }
    }
    return QPointF();
}

//---------------------------------------------------------------------------------------------------------------------
auto EvaluateRotation(const QString &formula) -> qreal
{
    if (formula.isEmpty())
    {
        return 0.0;
    }
    bool ok = false;
    const qreal value = formula.toDouble(&ok);
    return ok ? value : 0.0;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPieceGroupHolderItem::OPieceGroupHolderItem(quint32 groupId, QGraphicsItem *parent)
  : QGraphicsObject(parent), m_groupId(groupId)
{
    setZValue(50);
    setAcceptedMouseButtons(Qt::LeftButton);

    m_grainlineItem = new VGrainlineItem(VColorRole::PieceColor, this);
    // Make the group's grainline directly manipulable on the canvas, like a piece's
    // (the piece sets ItemIsMovable via EnableToolMove; the holder has no such signal).
    m_grainlineItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    m_grainlineItem->SetMoveType(VPieceItem::AllModifications);
    connect(m_grainlineItem, &VGrainlineItem::SignalMoved, this, &OPieceGroupHolderItem::OnGrainlineMoved);
    connect(m_grainlineItem, &VGrainlineItem::SignalResized, this, &OPieceGroupHolderItem::OnGrainlineResized);
    connect(m_grainlineItem, &VGrainlineItem::SignalRotated, this, &OPieceGroupHolderItem::OnGrainlineRotated);

    for (int block = 1; block <= 11; ++block)
    {
        auto *blockItem = new OInfoBlockItem(block, this);
        connect(blockItem, &OInfoBlockItem::Moved, this, &OPieceGroupHolderItem::OnBlockMoved);
        connect(blockItem, &OInfoBlockItem::Resized, this, &OPieceGroupHolderItem::OnBlockResized);
        blockItem->hide();
        m_blockItems.append(blockItem);
    }
}

//---------------------------------------------------------------------------------------------------------------------
OPieceGroupHolderItem::~OPieceGroupHolderItem() = default;

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::Refresh(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data)
{
    m_doc = doc;
    m_data = data;

    // The holder sits at the top-left of the union of the members' bounds (Val Fork
    // VPieceGroupHolderItem): children are laid out in LOCAL coordinates so they move
    // together with the group, and the frame is the group's clickable area.
    QRectF bounds;
    for (quint32 pieceId : group.pieceIds)
    {
        try
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
            if (tool != nullptr)
            {
                bounds = bounds.united(tool->sceneBoundingRect());
            }
        }
        catch (const VExceptionBadId &)
        {
            continue;  // stale member id (hand-edited file) — skip it
        }
    }
    m_groupBounds = bounds;
    setPos(bounds.topLeft());

    RefreshGrainline(group, data);
    RefreshBlocks(group, doc, data);
    RefreshAnnotations(group);
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::RefreshGrainline(const OPieceGroup &group, VContainer *data)
{
    const VGrainlineData &gl = group.m_glGrainline;
    if (!gl.IsEnabled() || !gl.IsVisible())
    {
        m_grainlineItem->hide();
        return;
    }

    QPointF pos = gl.GetPos();  // stored positions are relative to the group origin
    if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
    {
        pos = GroupCenter(group) - m_groupBounds.topLeft();  // local default (follows the group)
    }
    const qreal rotation = EvaluateRotation(gl.GetRotation());
    const qreal lengthCm = gl.GetLength().isEmpty() ? 10.0 : gl.GetLength().toDouble();
    const qreal lengthPx = ToPixel(lengthCm, *data->GetPatternUnit());
    m_grainlineItem->UpdateGeometry(pos, rotation, lengthPx, gl.GetArrowType());
    m_grainlineItem->show();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::RefreshBlocks(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data)
{
    const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(group, doc, data);
    const QPointF center = GroupCenter(group) - m_groupBounds.topLeft();  // local

    for (int block = 1; block <= 11; ++block)
    {
        OInfoBlockItem *blockItem = m_blockItems.at(block - 1);
        const bool exists = state.active && state.blockContent.contains(block);
        if (!exists)
        {
            blockItem->SetActive(false);
            blockItem->hide();
            continue;
        }

        OInfoBlockGeom geom = group.GetInfoBlockData().BlockGeom(block);
        const QVector<OCartoucheRenderLine> lines = state.blockContent.value(block).toVector();
        const QSizeF natural = OCartouchePainter::NaturalSize(lines);
        const qreal scale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
        geom.width = scale * natural.width();
        geom.height = scale * natural.height();
        if (!group.GetInfoBlockData().HasStoredGeometry(block))
        {
            geom.pos = center + QPointF(0.0, (block - 1) * (geom.height + 12.0));
        }

        blockItem->SetBlockGeom(geom);
        blockItem->SetContent(lines, geom.fontScale);
        blockItem->show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::RefreshAnnotations(const OPieceGroup &group)
{
    const QList<OAnnotation> annotations = group.GetAnnotationData().Annotations();
    const QPointF center = GroupCenter(group) - m_groupBounds.topLeft();  // local

    // Grow the pool on demand.
    while (m_annotationItems.size() < annotations.size())
    {
        const int index = static_cast<int>(m_annotationItems.size());
        auto *item = new OAnnotationItem(index, this);
        connect(item, &OAnnotationItem::Moved, this, &OPieceGroupHolderItem::OnAnnotationMoved);
        connect(item, &OAnnotationItem::Resized, this, &OPieceGroupHolderItem::OnAnnotationResized);
        item->hide();
        m_annotationItems.append(item);
    }

    for (int i = 0; i < m_annotationItems.size(); ++i)
    {
        OAnnotationItem *item = m_annotationItems.at(i);
        const bool exists = i < annotations.size() && !annotations.at(i).text.isEmpty();
        if (!exists)
        {
            item->SetActive(false);
            item->hide();
            continue;
        }

        const OAnnotation &annotation = annotations.at(i);
        item->SetIndex(i);
        const OAnnotationRenderStyle style = VToolSeamAllowance::AnnotationStyleResolver()
                                                 ? VToolSeamAllowance::AnnotationStyleResolver()(annotation.templateName)
                                                 : OAnnotationRenderStyle();
        item->SetContent(annotation.text, style);
        OAnnotationGeom geom = annotation.geom;
        if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
        {
            geom.pos = center;
        }
        item->SetAnnotationGeom(geom, EvaluateRotation(annotation.rotationFormula));
        item->show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
QRectF OPieceGroupHolderItem::boundingRect() const
{
    if (m_groupBounds.isNull())
    {
        return childrenBoundingRect();
    }
    // Local coordinates: the holder sits at the group's top-left.
    return childrenBoundingRect().united(m_groupBounds.translated(-m_groupBounds.topLeft()));
}

//---------------------------------------------------------------------------------------------------------------------
QPainterPath OPieceGroupHolderItem::shape() const
{
    if (m_groupBounds.isNull())
    {
        return QGraphicsObject::shape();
    }
    // The whole group frame is clickable, not just the children (Val Fork).
    QPainterPath path;
    path.addRect(m_groupBounds.translated(-m_groupBounds.topLeft()));
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
    // Nothing to paint: the children render the group's content.
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_pressScenePos = event->scenePos();
    m_dragging = true;
    m_movedSincePress = false;
    emit Clicked(m_groupId);
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (not m_dragging)
    {
        event->ignore();
        return;
    }

    const QPointF delta = event->scenePos() - m_pressScenePos;
    if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y()))
    {
        event->accept();
        return;
    }

    // Grouped into a single undo step ("Move piece group"), like dragging a member piece.
    if (not m_movedSincePress)
    {
        VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("Move piece group"));
        m_movedSincePress = true;
    }

    if (const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
        groups != nullptr && groups->contains(m_groupId))
    {
        const OPieceGroup &group = groups->value(m_groupId);
        for (quint32 pid : group.pieceIds)
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
            if (tool != nullptr)
            {
                VAbstractApplication::VApp()->getUndoStack()->push(
                    new MovePiece(m_doc, tool->pos().x() + delta.x(), tool->pos().y() + delta.y(), pid,
                                  scene()));
            }
        }
    }

    // Live preview: indexChanged is suppressed while the undo macro is open, so the usual
    // RefreshGroupHolders → Refresh repaint does not run during the drag. Move the holder with
    // the group so its local-coordinate children follow in real time (like a piece).
    setPos(pos() + delta);

    m_pressScenePos = event->scenePos();
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_movedSincePress)
    {
        VAbstractApplication::VApp()->getUndoStack()->endMacro();
        m_movedSincePress = false;
    }
    m_dragging = false;
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::CommitGroup(std::function<void(OPieceGroup &)> func)
{
    if (m_doc == nullptr || m_data == nullptr)
    {
        return;
    }
    auto *groups = m_data->DataPieceGroups();
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return;
    }
    OPieceGroup oldGroup = groups->value(m_groupId);
    OPieceGroup newGroup = oldGroup;
    func(newGroup);
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SavePieceGroupOptions(oldGroup, newGroup, m_doc, m_data, m_groupId));
    Refresh(newGroup, m_doc, m_data);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::ResetChildren(QGraphicsItem *pItem)
{
    OInfoBlockItem *pBlock = nullptr;
    for (OInfoBlockItem *blockItem : m_blockItems)
    {
        if (blockItem == pItem)
        {
            pBlock = blockItem;
            break;
        }
    }
    OAnnotationItem *pAnnotation = nullptr;
    for (OAnnotationItem *annotationItem : m_annotationItems)
    {
        if (annotationItem == pItem)
        {
            pAnnotation = annotationItem;
            break;
        }
    }
    for (OInfoBlockItem *blockItem : m_blockItems)
    {
        blockItem->SetActive(blockItem == pBlock);
    }
    for (OAnnotationItem *annotationItem : m_annotationItems)
    {
        annotationItem->SetActive(annotationItem == pAnnotation);
    }
    if (pBlock != nullptr || pAnnotation != nullptr)
    {
        emit Clicked(m_groupId);  // selecting a group child selects the group (like a piece)
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnGrainlineMoved(const QPointF &pos)
{
    CommitGroup([pos](OPieceGroup &group) { group.m_glGrainline.SetPos(pos); });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnGrainlineResized(qreal length)
{
    const qreal lengthCm = FromPixel(length, *m_data->GetPatternUnit());
    CommitGroup([this, lengthCm](OPieceGroup &group)
    {
        group.m_glGrainline.SetPos(m_grainlineItem->pos());
        group.m_glGrainline.SetLength(QString::number(lengthCm));
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnGrainlineRotated(qreal rot, const QPointF &pos)
{
    // The canvas emits radians; the format (and RefreshGrainline) expect degrees —
    // mirror VToolSeamAllowance::SaveRotateGrainline.
    CommitGroup([rot, pos](OPieceGroup &group)
    {
        group.m_glGrainline.SetPos(pos);
        group.m_glGrainline.SetRotation(QString::number(qRadiansToDegrees(rot)));
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnBlockMoved(int blockIndex, const QPointF &pos)
{
    CommitGroup([this, blockIndex, pos](OPieceGroup &group)
    {
        if (!group.m_infoBlocks.IsSet())
        {
            const OInfoBlockEffectiveState st = OInfoBlockResolver::ResolveEffectiveState(group, m_doc, m_data);
            group.m_infoBlocks.SetActive(st.active);
            group.m_infoBlocks.SetTemplate(st.templateName);
            group.m_infoBlocks.SetDispatch(st.dispatch);
            group.m_infoBlocks.SetSet(true);
        }
        OInfoBlockGeom geom = group.m_infoBlocks.BlockGeom(blockIndex);
        geom.pos = pos;
        group.m_infoBlocks.SetBlockGeom(blockIndex, geom);
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnBlockResized(int blockIndex, qreal newW, qreal newH, qreal origW, qreal origH)
{
    Q_UNUSED(origH)
    CommitGroup([this, blockIndex, newW, newH, origW](OPieceGroup &group)
    {
        if (!group.m_infoBlocks.IsSet())
        {
            const OInfoBlockEffectiveState st = OInfoBlockResolver::ResolveEffectiveState(group, m_doc, m_data);
            group.m_infoBlocks.SetActive(st.active);
            group.m_infoBlocks.SetTemplate(st.templateName);
            group.m_infoBlocks.SetDispatch(st.dispatch);
            group.m_infoBlocks.SetSet(true);
        }
        OInfoBlockGeom geom = group.m_infoBlocks.BlockGeom(blockIndex);
        geom.pos = m_blockItems.at(blockIndex - 1)->pos();
        geom.width = newW;
        geom.height = newH;
        // Proportional resize: the content scales with the box.
        if (origW > 0.0)
        {
            geom.fontScale *= (newW / origW);
        }
        group.m_infoBlocks.SetBlockGeom(blockIndex, geom);
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnAnnotationMoved(int index, const QPointF &pos)
{
    CommitGroup([index, pos](OPieceGroup &group)
    {
        QList<OAnnotation> annotations = group.m_annotations.Annotations();
        if (index < 0 || index >= annotations.size())
        {
            return;
        }
        annotations[index].geom.pos = pos;
        group.m_annotations.SetAnnotations(annotations);
        group.m_annotations.SetSet(true);
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupHolderItem::OnAnnotationResized(int index, qreal newW, qreal newH, qreal origW, qreal origH)
{
    Q_UNUSED(origW)
    Q_UNUSED(origH)
    CommitGroup([this, index, newW, newH](OPieceGroup &group)
    {
        QList<OAnnotation> annotations = group.m_annotations.Annotations();
        if (index < 0 || index >= annotations.size())
        {
            return;
        }
        annotations[index].geom.pos = m_annotationItems.at(index)->pos();
        annotations[index].geom.boxWidth = newW;
        annotations[index].geom.boxHeight = newH;
        group.m_annotations.SetAnnotations(annotations);
        group.m_annotations.SetSet(true);
    });
}
