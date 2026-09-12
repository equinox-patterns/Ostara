/************************************************************************
 **
 **  @file   opiecegroupholderitem.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Canvas holder that renders a piece group's common children
 **          (grainline, cartouche blocks, annotations) as one overlay.
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

#ifndef OPIECEGROUPHOLDERITEM_H
#define OPIECEGROUPHOLDERITEM_H

#include <QGraphicsObject>
#include <QPainterPath>
#include <QVector>

#include <functional>

#include "../vpatterndb/opiecegroup.h"

class OAnnotationItem;
class OInfoBlockItem;
class QGraphicsSceneContextMenuEvent;
class VAbstractPattern;
class VContainer;
class VGrainlineItem;

/**
 * @brief The OPieceGroupHolderItem class — renders a group's common children on
 *        the Details canvas. Read-only (no drag/resize editing yet): members are
 *        masked by their own tools, this overlay shows what the group owns.
 */
class OPieceGroupHolderItem final : public QGraphicsObject
{
    Q_OBJECT // NOLINT

public:
    explicit OPieceGroupHolderItem(quint32 groupId, QGraphicsItem *parent = nullptr);
    ~OPieceGroupHolderItem() override;

    auto type() const -> int override { return UserType + 90; }
    void Refresh(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data);
    /** @brief Toggle mouse interaction on the holder and its children. During the Link tool the
     *  holders stay visible (their children must not vanish) but become click-transparent so the
     *  clicks reach the member pieces beneath (which the workflow rejects, being already grouped). */
    void SetInteractive(bool interactive);
    /** @brief Mark the group selected on the canvas — draws the selection block (dashed rect +
     *  handles), like a selected piece. */
    void SetSelected(bool selected);

signals:
    void Clicked(quint32 groupId);
    /** @brief Right-click "Unlink all": dissolve the group. MainWindow runs the undo command
     *  (the push removes this very holder, so it must not run inside this event handler). */
    void UnlinkAllRequested(quint32 groupId);

public slots:
    /** @brief Activate the clicked child (outline + resize handles), deactivate the rest
     *  — mirror of VToolSeamAllowance::ResetChildren. */
    void ResetChildren(QGraphicsItem *pItem);

protected:
    auto boundingRect() const -> QRectF override;
    auto shape() const -> QPainterPath override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    void RefreshGrainline(const OPieceGroup &group, VContainer *data);
    void RefreshBlocks(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data);
    void RefreshAnnotations(const OPieceGroup &group);
    void CommitGroup(std::function<void(OPieceGroup &)> func);

    void OnGrainlineMoved(const QPointF &pos);
    void OnGrainlineResized(qreal length);
    void OnGrainlineRotated(qreal rot, const QPointF &pos);
    void OnBlockMoved(int blockIndex, const QPointF &pos);
    void OnBlockResized(int blockIndex, qreal newW, qreal newH, qreal origW, qreal origH);
    void OnAnnotationMoved(int index, const QPointF &pos);
    void OnAnnotationResized(int index, qreal newW, qreal newH, qreal origW, qreal origH);

    Q_DISABLE_COPY_MOVE(OPieceGroupHolderItem) // NOLINT

    quint32 m_groupId{NULL_ID};
    bool m_selected{false};
    /** @brief Drag state: the holder moves the whole group (Val Fork master-piece behaviour). */
    QPointF m_pressScenePos;
    bool m_dragging{false};
    bool m_movedSincePress{false};
    /** @brief Union of the members' scene bounding rects — the holder's clickable frame
     *  (Val Fork VPieceGroupHolderItem). In scene coordinates. */
    QRectF m_groupBounds;
    QRectF m_childRefRect; // Rect the group's children anchor against, in the holder's local frame
    VGrainlineItem *m_grainlineItem{nullptr};
    QVector<OInfoBlockItem *> m_blockItems;
    QVector<OAnnotationItem *> m_annotationItems;
    VAbstractPattern *m_doc{nullptr};
    VContainer *m_data{nullptr};
};

#endif // OPIECEGROUPHOLDERITEM_H
