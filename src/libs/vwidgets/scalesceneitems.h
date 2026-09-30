/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   21 6, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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

#ifndef SCALESCENEITEMS_H
#define SCALESCENEITEMS_H

#include <QGraphicsLineItem>
#include <functional>

#include "../vmisc/def.h"
#include "themeDef.h"

class VScaledLine : public QGraphicsLineItem
{
public:
    explicit VScaledLine(VColorRole role, QGraphicsItem *parent = nullptr);
    VScaledLine(const QLineF &line, VColorRole role, QGraphicsItem *parent = nullptr);
    ~VScaledLine() override = default;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ScaledLine)
    };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    auto shape() const -> QPainterPath override;
    auto boundingRect() const -> QRectF override;

    auto IsBoldLine() const -> bool;
    void SetBoldLine(bool bold);
    void SetHighlighted(bool on);

    /// Marks the line as picked for the layer-add/remove workflow: drawn orange and bold, like the active point
    /// in Details mode. A state of its own, so the hover highlight cannot clear it.
    void SetLayerSelected(bool selected);
    auto IsLayerSelected() const -> bool { return m_layerSelected; }

    /// Marks the line as clicked for removal from a layer: magenta and bold, on top of SetLayerSelected. While
    /// removing, the whole layer stays orange and only the lines the user clicks turn magenta.
    void SetInLayer(bool picked);
    auto IsInLayer() const -> bool { return m_inLayer; }

    auto IsShowArrows() const -> bool;
    void SetShowArrows(bool show);

    /// Gives the line the click a tool wants to hear about: it then stands for one of the segments that tool
    /// draws — a side of a rectangle, a tracé of a dart cap, the main line of a line point — and a click has to
    /// pick that segment for the operation being created. The callback carries what the owner needs (its own id
    /// and the two endpoints); this item is not a QObject and emits nothing, and a line left without a handler
    /// keeps its old meaning: the click is whatever its parent does with it.
    void SetSegmentClickHandler(std::function<void()> handler);

    auto GetColorRole() const -> VColorRole;
    void SetColorRole(VColorRole role);

    virtual void RefreshPen();

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;

private:
    auto ScaledPenWidth() const -> qreal;
    auto ArrowsPath() const -> QPainterPath;

    /// Repaint after either layer mark changed. Shared, because the two marks write the same pen: the magenta of
    /// the removal pick, the orange of the layer pick, and the colour to give back once both are gone.
    void ApplyLayerMark();

    Q_DISABLE_COPY_MOVE(VScaledLine) // NOLINT

    bool m_isBoldLine{true};
    bool m_showArrows{false};
    /// Set when the line stands for a segment its owning tool draws; empty for every other line.
    std::function<void()> m_segmentClickHandler{};
    bool m_layerSelected{false};
    bool m_inLayer{false};
    /// Colour to restore for a CustomColor line, which has no role to recompute from. Invalid when none is saved.
    QColor m_colorBeforeLayer{};
    VColorRole m_role;
};

//---------------------------------------------------------------------------------------------------------------------
inline auto VScaledLine::GetColorRole() const -> VColorRole
{
    return m_role;
}

//---------------------------------------------------------------------------------------------------------------------
inline void VScaledLine::SetColorRole(VColorRole role)
{
    m_role = role;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VScaledLine::IsBoldLine() const -> bool
{
    return m_isBoldLine;
}

//---------------------------------------------------------------------------------------------------------------------
inline void VScaledLine::SetBoldLine(bool bold)
{
    m_isBoldLine = bold;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VScaledLine::IsShowArrows() const -> bool
{
    return m_showArrows;
}

//---------------------------------------------------------------------------------------------------------------------
inline void VScaledLine::SetShowArrows(bool show)
{
    m_showArrows = show;
}

class VScaledEllipse : public QGraphicsEllipseItem
{
public:
    explicit VScaledEllipse(VColorRole role, QGraphicsItem *parent = nullptr);
    ~VScaledEllipse() override = default;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ScaledEllipse)
    };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    auto PointMode() const -> bool;
    void SetPointMode(bool newPointMode);

    void RefreshScale();

protected:
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;

private:
    Q_DISABLE_COPY_MOVE(VScaledEllipse) // NOLINT

    bool m_pointMode{true};
    VColorRole m_role;
};

//---------------------------------------------------------------------------------------------------------------------
inline auto VScaledEllipse::PointMode() const -> bool
{
    return m_pointMode;
}

//---------------------------------------------------------------------------------------------------------------------
inline void VScaledEllipse::SetPointMode(bool newPointMode)
{
    m_pointMode = newPointMode;
}

#endif // SCALESCENEITEMS_H
