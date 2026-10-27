/************************************************************************
 **
 **  @file   vgrainlineitem.cpp
 **  @author Bojan Kverh
 **  @date   September 10, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 19/10/2026.
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

#include <QDebug>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtMath>
#include <cmath>

#include "../vmisc/def.h"
#include "../vmisc/literals.h"
#include "vscenestylesheet.h"
#include "../vmisc/vabstractapplication.h"
#include "global.h"
#include "vgrainlineitem.h"
#include "vpiecegrainline.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
#include "../vmisc/backport/qpainterstateguard.h"
#else
#include <QPainterStateGuard>
#endif

constexpr int rectWidth = 30;
constexpr int resizeRectSize = 10;
constexpr double kResizeHitMargin = 10.0;  // invisible grab zone above/below each bar
constexpr int rotateCircR = 7;
constexpr int activeZ = 10;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::VGrainlineItem constructor
 * @param pParent pointer to the parent item
 */
VGrainlineItem::VGrainlineItem(VColorRole role, QGraphicsItem *pParent)
  : VPieceItem(pParent),
    m_penWidth(VAbstractApplication::VApp()->Settings()->WidthMainLine()),
    m_role(role)
{
    setAcceptHoverEvents(true);
    m_inactiveZ = 5;
    Reset();
    UpdateRectangle();
}

//---------------------------------------------------------------------------------------------------------------------
auto VGrainlineItem::shape() const -> QPainterPath
{
    // The whole block (m_polyBound) is the click target so a click anywhere in the
    // rectangle the editing outline will show activates the grainline, not just the
    // thin line itself. The pieces must be **united**, not merely added: `addPolygon`
    // stacks subpaths, and Qt's fill rule then counts the overlap twice — under
    // odd-even (the default) the line and its arrows become a hole, under winding
    // they cancel. Either way the one place a user aims at fell through to the piece
    // underneath, which is exactly what made a grainline hard to grab.
    QPainterPath bound;
    bound.addPolygon(m_polyBound);
    QPainterPath path = MainShape().united(bound);
    if (m_eMode != mNormal)
    {
        QPainterPath hit0;
        hit0.addPolygon(m_resizeHit[0]);
        QPainterPath hit1;
        hit1.addPolygon(m_resizeHit[1]);
        path = path.united(hit0).united(hit1);
    }
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::paint paints the item content
 * @param pP pointer to the painter object
 * @param pOption not used
 * @param pWidget not used
 */
void VGrainlineItem::paint(QPainter *pP, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget)
{
    Q_UNUSED(pOption)
    Q_UNUSED(pWidget)
    QPainterStateGuard guard(pP);
    QColor clr = m_color;

    if (m_role != VColorRole::CustomColor)
    {
        clr = VSceneStylesheet::Color(m_role);
    }

    const qreal width = ScaleWidth(VAbstractApplication::VApp()->Settings()->WidthHairLine(), SceneScale(scene()));
    pP->setPen(QPen(clr, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    pP->setRenderHints(QPainter::Antialiasing);

    VPieceGrainline const grainline(QPointF(), m_dLength, m_dRotation, m_eArrowType);
    // main line
    const QLineF mainLine = grainline.GetMainLine();
    pP->drawLine(mainLine.p1(), mainLine.p2());

    pP->setBrush(clr);

    if (grainline.IsArrowUpEnabled())
    {
        pP->drawPolygon(grainline.ArrowUp());
    }

    if (grainline.IsArrowDownEnabled())
    {
        pP->drawPolygon(grainline.ArrowDown());
    }

    if (grainline.IsFourWays())
    {
        // secondary line
        const QLineF secondaryLine = grainline.SecondaryLine();
        pP->drawLine(secondaryLine.p1(), secondaryLine.p2());

        if (grainline.IsArrowLeftEnabled())
        {
            pP->drawPolygon(grainline.ArrowLeft());
        }

        if (grainline.IsArrowRightEnabled())
        {
            pP->drawPolygon(grainline.ArrowRight());
        }
    }

    if (m_eMode != mNormal)
    {
        pP->setPen(QPen(clr, 2, Qt::DashLine));
        pP->setBrush(Qt::NoBrush);
        // bounding polygon
        pP->drawPolygon(m_polyBound);
        pP->setPen(QPen(clr, 3));

        if (m_eMode != mRotate)
        {
            pP->setBrush(clr);
            UpdatePolyResize();
            pP->drawPolygon(m_polyResize[0]);
            pP->drawPolygon(m_polyResize[1]);
        }

        pP->setBrush(Qt::NoBrush);
        if (m_eMode == mResize)
        {
            pP->setPen(clr);
            pP->drawLine(m_polyBound.at(0), m_polyBound.at(2));
            pP->drawLine(m_polyBound.at(1), m_polyBound.at(3));
        }

        if (m_eMode == mRotate)
        {
            QPointF const ptC = (m_polyBound.at(0) + m_polyBound.at(2)) / 2;
            qreal const dRad = rotateCircR;
            pP->setBrush(clr);
            pP->drawEllipse(ptC, dRad, dRad);

            pP->setBrush(Qt::NoBrush);
            guard.save();
            pP->translate(ptC);
            pP->rotate(qRadiansToDegrees(-m_dRotation));
            int const iX = qRound(m_dLength / 2 - 0.5 * dRad);
            int const iY =
                grainline.IsFourWays() ? qRound((m_dLength / 2) - 0.8 * dRad) : qRound(rectWidth - 0.5 * dRad);
            int const iR = qRound(dRad * 3);
            pP->drawArc(iX - iR, iY - iR, iR, iR, 0 * 16, -90 * 16);
            pP->drawArc(-iX, iY - iR, iR, iR, 270 * 16, -90 * 16);
            pP->drawArc(-iX, -iY, iR, iR, 180 * 16, -90 * 16);
            pP->drawArc(iX - iR, -iY, iR, iR, 90 * 16, -90 * 16);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::UpdateGeometry updates the item with grainline parameters
 * @param ptPos position of one grainline's end
 * @param dRotation rotation of the grainline in [degrees]
 * @param dLength length of the grainline in user's units
 */
void VGrainlineItem::UpdateGeometry(const QPointF &ptPos, qreal dRotation, qreal dLength, GrainlineArrowDirection eAT)
{
    m_dRotation = qDegreesToRadians(dRotation);
    m_dLength = dLength;
    m_eArrowType = eAT;

    VPieceGrainline const grainline(ptPos, m_dLength, m_dRotation, m_eArrowType);
    qreal dX;
    qreal dY;
    QPointF pt = ptPos;
    if (not grainline.IsContained(parentItem()->boundingRect(), dX, dY))
    {
        pt.setX(pt.x() + dX);
        pt.setY(pt.y() + dY);
    }
    setPos(pt);

    UpdateRectangle();
    Update();
}

//---------------------------------------------------------------------------------------------------------------------
auto VGrainlineItem::Grainline() const -> VPieceGrainline
{
    return {QLineF(m_ptStart, m_ptFinish), m_eArrowType};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::mousePressEvent handles left button mouse press events
 * @param pME pointer to QGraphicsSceneMouseEvent object
 */
void VGrainlineItem::mousePressEvent(QGraphicsSceneMouseEvent *pME)
{
    if (pME->button() == Qt::LeftButton && pME->type() != QEvent::GraphicsSceneMouseDoubleClick &&
        (flags() & QGraphicsItem::ItemIsMovable))
    {
        if (m_moveType == NotMovable)
        {
            pME->ignore();
            return;
        }

        pME->accept();

        m_ptStartPos = pos();
        m_ptStartMove = pME->scenePos();
        m_dStartLength = m_dLength;
        m_dStartRotation = m_dRotation;
        m_dAngle = GetAngle(mapToParent(pME->pos()));
        m_ptRotCenter = m_ptCenter;

        if ((m_moveType & AllModifications) == AllModifications)
        {
            AllUserModifications(pME->pos());
            setZValue(activeZ);
            Update();
        }
        else if (m_moveType & IsRotatable)
        {
            if (m_moveType & IsResizable)
            {
                AllUserModifications(pME->pos());
            }
            else if (m_moveType & IsMovable)
            {
                UserRotateAndMove();
            }
            else
            {
                m_eMode = mRotate;
                SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
            }
            setZValue(activeZ);
            Update();
        }
        else if (m_moveType & IsResizable)
        {
            if (m_moveType & IsRotatable)
            {
                AllUserModifications(pME->pos());
            }
            else if (m_moveType & IsMovable)
            {
                UserMoveAndResize(pME->pos());
            }
            setZValue(activeZ);
            Update();
        }
        else if (m_moveType & IsMovable)
        {
            if (m_moveType & IsRotatable)
            {
                UserRotateAndMove();
            }
            else if (m_moveType & IsResizable)
            {
                UserMoveAndResize(pME->pos());
            }
            else
            {
                m_eMode = mMove;
                SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
            }

            setZValue(activeZ);
            Update();
        }
        else
        {
            pME->ignore();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::mouseMoveEvent handles mouse move events, making sure that the item is moved properly
 * @param pME pointer to QGraphicsSceneMouseEvent object
 */
void VGrainlineItem::mouseMoveEvent(QGraphicsSceneMouseEvent *pME)
{
    QPointF const ptDiff = pME->scenePos() - m_ptStartMove;
    qreal dX;
    qreal dY;
    if (m_eMode == mMove && m_moveType & IsMovable)
    {
        QPointF pt = m_ptStartPos + ptDiff;
        VPieceGrainline const grainline(pt, m_dLength, m_dRotation, m_eArrowType);
        if (not grainline.IsContained(parentItem()->boundingRect(), dX, dY))
        {
            pt.setX(pt.x() + dX);
            pt.setY(pt.y() + dY);
        }
        setPos(pt);
        Update();
    }
    else if (m_eMode == mResize && m_moveType & IsResizable)
    {
        qreal dLen = qSqrt(ptDiff.x() * ptDiff.x() + ptDiff.y() * ptDiff.y());
        qreal const dAng = qAtan2(-ptDiff.y(), ptDiff.x());
        // The sign depends on which bar was grabbed: the start bar grows the line
        // backward (finish stays), the finish bar grows it forward (start stays).
        dLen = (m_resizeFromEnd ? 1 : -1) * dLen * qCos(dAng - m_dRotation);
        qreal const dPrevLen = m_dLength;
        // try with new length
        if (not(m_moveType & IsMovable))
        {
            dLen *= 2;
        }

        if (m_dStartLength + dLen < ToPixel(5, Unit::Mm))
        {
            return;
        }

        m_dLength = m_dStartLength + dLen;

        QPointF pos;

        if (m_resizeFromEnd && (m_moveType & IsMovable))
        {
            // Grow from the finish end: the start end (the item position) stays fixed.
            pos = m_ptStartPos;
        }
        else if (m_moveType & IsMovable)
        {
            QLineF grainline(this->pos().x(), this->pos().y(), this->pos().x() + dPrevLen, this->pos().y());
            grainline.setAngle(qRadiansToDegrees(m_dRotation));
            grainline = QLineF(grainline.p2(), grainline.p1());
            grainline.setLength(m_dLength);
            pos = grainline.p2();
        }
        else
        {
            QLineF grainline(m_ptCenter.x(), m_ptCenter.y(), m_ptCenter.x() + m_dLength / 2.0, m_ptCenter.y());

            grainline.setAngle(qRadiansToDegrees(m_dRotation));
            grainline = QLineF(grainline.p2(), grainline.p1());
            grainline.setLength(m_dLength);

            pos = grainline.p2();
        }

        qreal dX;
        qreal dY;
        VPieceGrainline const grainline(pos, m_dLength, m_dRotation, m_eArrowType);
        if (not grainline.IsContained(parentItem()->boundingRect(), dX, dY))
        {
            m_dLength = dPrevLen;
        }
        else
        {
            setPos(pos);
        }

        UpdateRectangle();
        Update();
    }
    else if (m_eMode == mRotate && m_moveType & IsRotatable)
    {
        // prevent strange angle changes due to singularities
        qreal const dLen = qSqrt(ptDiff.x() * ptDiff.x() + ptDiff.y() * ptDiff.y());
        if (dLen < 2)
        {
            return;
        }

        if (fabs(m_dAngle) < 0.01)
        {
            m_dAngle = GetAngle(mapToParent(pME->pos()));
            return;
        }

        qreal const dAng = GetAngle(mapToParent(pME->pos())) - m_dAngle;
        QPointF const ptNewPos = Rotate(m_ptStartPos, m_ptRotCenter, dAng);
        VPieceGrainline const grainline(ptNewPos, m_dLength, m_dStartRotation + dAng, m_eArrowType);
        if (grainline.IsContained(parentItem()->boundingRect(), dX, dY))
        {
            setPos(ptNewPos);
            m_dRotation = m_dStartRotation + dAng;
            UpdateRectangle();
            Update();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::mouseReleaseEvent handles mouse release events and emits the proper signal if the item was
 * moved
 * @param pME pointer to QGraphicsSceneMouseEvent object
 */
void VGrainlineItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *pME)
{
    if (pME->button() != Qt::LeftButton)
    {
        VPieceItem::mouseReleaseEvent(pME);
        return;
    }

    if ((m_eMode == mMove || m_eMode == mRotate || m_eMode == mResize) && (flags() & QGraphicsItem::ItemIsMovable))
    {
        SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);
    }

    QPointF const ptDiff = pME->scenePos() - m_ptStartMove;
    qreal const dLen = qSqrt(ptDiff.x() * ptDiff.x() + ptDiff.y() * ptDiff.y());
    bool const bShort = (dLen < 2);

    if (m_eMode == mMove || m_eMode == mResize)
    {
        if (bShort)
        {
            if (m_bReleased && m_moveType & IsRotatable)
            {
                m_eMode = mRotate;
                Update();
            }
        }
        else
        {
            if (m_eMode == mMove && m_moveType & IsMovable)
            {
                emit SignalMoved(pos());
            }
            else if (m_moveType & IsResizable)
            {
                emit SignalResized(m_dLength);
            }
            Update();
        }
    }
    else
    {
        if (bShort)
        {
            m_eMode = mMove;
        }
        else if (m_moveType & IsRotatable)
        {
            emit SignalRotated(m_dRotation, m_ptStart);
        }
        Update();
    }
    m_bReleased = true;
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::hoverEnterEvent(QGraphicsSceneHoverEvent *pME)
{
    if (flags() & QGraphicsItem::ItemIsMovable)
    {
        SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);
        m_penWidth = VAbstractApplication::VApp()->Settings()->WidthMainLine() + 1;
    }
    VPieceItem::hoverEnterEvent(pME);
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::hoverMoveEvent(QGraphicsSceneHoverEvent *pME)
{
    if (flags() & QGraphicsItem::ItemIsMovable)
    {
        // The full-width resize bars (with their enlarged hit zones) must show the
        // resize cursor on hover, not only while dragging.
        if (m_eMode != mNormal && (m_resizeHit[0].containsPoint(pME->pos(), Qt::OddEvenFill)
                                   || m_resizeHit[1].containsPoint(pME->pos(), Qt::OddEvenFill)))
        {
            setCursor(Qt::SizeVerCursor);
        }
        else
        {
            SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);
        }
    }
    QGraphicsItem::hoverMoveEvent(pME);
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *pME)
{
    if (flags() & QGraphicsItem::ItemIsMovable)
    {
        m_penWidth = VAbstractApplication::VApp()->Settings()->WidthMainLine();
    }
    VPieceItem::hoverLeaveEvent(pME);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::UpdateBox updates the item
 */
void VGrainlineItem::Update()
{
    update(m_rectBoundingBox);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::UpdateRectangle updates the polygon for the box around active item
 * and the bounding rectangle
 */
void VGrainlineItem::UpdateRectangle()
{
    VPieceGrainline const grainline(QPointF(), m_dLength, m_dRotation, m_eArrowType);
    const QLineF mainLine = grainline.GetMainLine();

    m_ptStart = mapToParent(mainLine.p1());
    m_ptFinish = mapToParent(mainLine.p2());
    m_ptCenter = (m_ptStart + m_ptFinish) / 2;

    m_polyBound.clear();

    if (grainline.IsFourWays())
    {
        m_polyBound << QPointF(mainLine.p1().x() + (m_dLength / 2) * cos(m_dRotation + M_PI / 2),
                               mainLine.p1().y() - (m_dLength / 2) * sin(m_dRotation + M_PI / 2));
        m_polyBound << QPointF(mainLine.p1().x() + (m_dLength / 2) * cos(m_dRotation - M_PI / 2),
                               mainLine.p1().y() - (m_dLength / 2) * sin(m_dRotation - M_PI / 2));
        m_polyBound << QPointF(mainLine.p2().x() + (m_dLength / 2) * cos(m_dRotation - M_PI / 2),
                               mainLine.p2().y() - (m_dLength / 2) * sin(m_dRotation - M_PI / 2));
        m_polyBound << QPointF(mainLine.p2().x() + (m_dLength / 2) * cos(m_dRotation + M_PI / 2),
                               mainLine.p2().y() - (m_dLength / 2) * sin(m_dRotation + M_PI / 2));

        const QLineF secondaryLine = grainline.SecondaryLine();
        m_ptSecondaryStart = mapToParent(secondaryLine.p1());
        m_ptSecondaryFinish = mapToParent(secondaryLine.p2());
    }
    else
    {
        m_polyBound << QPointF(mainLine.p1().x() + rectWidth * cos(m_dRotation + M_PI / 2),
                               mainLine.p1().y() - rectWidth * sin(m_dRotation + M_PI / 2));
        m_polyBound << QPointF(mainLine.p1().x() + rectWidth * cos(m_dRotation - M_PI / 2),
                               mainLine.p1().y() - rectWidth * sin(m_dRotation - M_PI / 2));
        m_polyBound << QPointF(mainLine.p2().x() + rectWidth * cos(m_dRotation - M_PI / 2),
                               mainLine.p2().y() - rectWidth * sin(m_dRotation - M_PI / 2));
        m_polyBound << QPointF(mainLine.p2().x() + rectWidth * cos(m_dRotation + M_PI / 2),
                               mainLine.p2().y() - rectWidth * sin(m_dRotation + M_PI / 2));
    }
    // prepareGeometryChange() must run BEFORE the bounding rect changes: Qt un-indexes the item
    // with the rectangle it has at that moment, so calling it afterwards left the scene's index
    // describing the previous rectangle — and a click inside the new one could miss the grainline.
    prepareGeometryChange();
    m_rectBoundingBox = m_polyBound.boundingRect().adjusted(-2, -2, 2, 2);
    setTransformOriginPoint(m_rectBoundingBox.center());

    UpdatePolyResize();
}

//---------------------------------------------------------------------------------------------------------------------
auto VGrainlineItem::GetAngle(const QPointF &pt) const -> double
{
    return -VPieceItem::GetAngle(pt);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::Rotate rotates point pt around ptCenter by angle dAng [rad]
 * and returns the resulting point
 * @param pt point to rotate
 * @param ptCenter center of rotation
 * @param dAng angle of rotation
 * @return point, which is a result of rotating pt around ptCenter by angle dAng
 */
auto VGrainlineItem::Rotate(const QPointF &pt, const QPointF &ptCenter, qreal dAng) -> QPointF
{
    QPointF const ptRel = pt - ptCenter;
    QPointF ptFinal;
    ptFinal.setX(ptRel.x() * qCos(dAng) + ptRel.y() * qSin(dAng));
    ptFinal.setY(-ptRel.x() * qSin(dAng) + ptRel.y() * qCos(dAng));
    ptFinal += ptCenter;
    return ptFinal;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VGrainlineItem::GetInsideCorner calculates a point inside the bounding polygon,
 * dDist away of i-th point in each direction
 * @param i index of corner
 * @param dDist distance
 * @return resulting point
 */
auto VGrainlineItem::GetInsideCorner(int i, qreal dDist) const -> QPointF
{
    QPointF pt1 = m_polyBound.at((i + 1) % m_polyBound.count()) - m_polyBound.at(i);
    QPointF pt2 = m_polyBound.at((i + m_polyBound.count() - 1) % m_polyBound.count()) - m_polyBound.at(i);

    pt1 = dDist * pt1 / qSqrt(pt1.x() * pt1.x() + pt1.y() * pt1.y());
    pt2 = dDist * pt2 / qSqrt(pt2.x() * pt2.x() + pt2.y() * pt2.y());

    return m_polyBound.at(i) + pt1 + pt2;
}

//---------------------------------------------------------------------------------------------------------------------
auto VGrainlineItem::MainShape() const -> QPainterPath
{
    VPieceGrainline const grainline(QPointF(), m_dLength, m_dRotation, m_eArrowType);
    const QLineF mainLine = grainline.GetMainLine();
    QPainterPath linePath;
    linePath.moveTo(mainLine.p1());
    linePath.lineTo(mainLine.p2());
    linePath.closeSubpath();

    QPainterPathStroker stroker;
    stroker.setWidth(m_penWidth);
    QPainterPath path;
    path.addPath((stroker.createStroke(linePath) + linePath).simplified());
    path.closeSubpath();

    if (grainline.IsArrowUpEnabled())
    {
        QPainterPath polyPath;
        polyPath.addPolygon(grainline.ArrowUp());

        path.addPath((stroker.createStroke(polyPath) + polyPath).simplified());
        path.closeSubpath();
    }

    if (grainline.IsArrowDownEnabled())
    {
        QPainterPath polyPath;
        polyPath.addPolygon(grainline.ArrowDown());

        path.addPath((stroker.createStroke(polyPath) + polyPath).simplified());
        path.closeSubpath();
    }

    if (grainline.IsFourWays())
    {
        const QLineF secondaryLine = grainline.SecondaryLine();
        QPainterPath secondaryLinePath;
        secondaryLinePath.moveTo(secondaryLine.p1());
        secondaryLinePath.lineTo(secondaryLine.p2());
        secondaryLinePath.closeSubpath();

        path.addPath((stroker.createStroke(secondaryLinePath) + secondaryLinePath).simplified());
        path.closeSubpath();

        if (grainline.IsArrowLeftEnabled())
        {
            QPainterPath polyPath;
            polyPath.addPolygon(grainline.ArrowLeft());

            path.addPath((stroker.createStroke(polyPath) + polyPath).simplified());
            path.closeSubpath();
        }

        if (grainline.IsArrowRightEnabled())
        {
            QPainterPath polyPath;
            polyPath.addPolygon(grainline.ArrowRight());

            path.addPath((stroker.createStroke(polyPath) + polyPath).simplified());
            path.closeSubpath();
        }
    }
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::AllUserModifications(const QPointF &pos)
{
    if (m_eMode != mRotate)
    {
        UserMoveAndResize(pos);
    }
    else
    {
        SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::UserRotateAndMove()
{
    if (m_eMode != mRotate)
    {
        m_eMode = mMove;
    }
    SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::UserMoveAndResize(const QPointF &pos)
{
    if (m_resizeHit[0].containsPoint(pos, Qt::OddEvenFill))
    {
        m_eMode = mResize;
        m_resizeFromEnd = false;  // grow from the start end
        setCursor(Qt::SizeVerCursor);
    }
    else if (m_resizeHit[1].containsPoint(pos, Qt::OddEvenFill))
    {
        m_eMode = mResize;
        m_resizeFromEnd = true;  // grow from the finish end
        setCursor(Qt::SizeVerCursor);
    }
    else
    {
        m_eMode = mMove; // block later if need
        SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VGrainlineItem::UpdatePolyResize()
{
    // Two full-width bars, one at each end, spanning the grainline's width and
    // extending resizeRectSize along the line. Each has an invisible, slightly
    // taller hit zone (kResizeHitMargin above and below) that also grabs the
    // resize. The along direction matches the grainline's drawing
    // (p2 = p1 + length * (cos, -sin)).
    const QPointF dir(cos(m_dRotation), -sin(m_dRotation));
    const double dSize = resizeRectSize;
    const double dHit = dSize + 2.0 * kResizeHitMargin;

    // Bar 0 — start end (p1): visible bar + enlarged hit zone.
    m_polyResize[0].clear();
    m_polyResize[0] << m_polyBound.at(1) << m_polyBound.at(0) << m_polyBound.at(0) + dSize * dir
                    << m_polyBound.at(1) + dSize * dir;
    m_resizeHit[0].clear();
    m_resizeHit[0] << m_polyBound.at(1) - kResizeHitMargin * dir
                   << m_polyBound.at(0) - kResizeHitMargin * dir
                   << m_polyBound.at(0) + dHit * dir << m_polyBound.at(1) + dHit * dir;

    // Bar 1 — finish end (p2): visible bar + enlarged hit zone.
    m_polyResize[1].clear();
    m_polyResize[1] << m_polyBound.at(2) << m_polyBound.at(3) << m_polyBound.at(3) - dSize * dir
                    << m_polyBound.at(2) - dSize * dir;
    m_resizeHit[1].clear();
    m_resizeHit[1] << m_polyBound.at(2) + kResizeHitMargin * dir
                   << m_polyBound.at(3) + kResizeHitMargin * dir
                   << m_polyBound.at(3) - dHit * dir << m_polyBound.at(2) - dHit * dir;
}
