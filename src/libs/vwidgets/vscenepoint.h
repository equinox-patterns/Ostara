/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   14 6, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 16/10/2026.
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

#ifndef VSCENEPOINT_H
#define VSCENEPOINT_H

#include <QGraphicsEllipseItem>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "themeDef.h"

class VGraphicsSimpleTextItem;
class VPointF;
class VScaledLine;

class VScenePoint : public QGraphicsEllipseItem
{
public:
    explicit VScenePoint(VColorRole role, QGraphicsItem *parent = nullptr);
    ~VScenePoint() override = default;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ScenePoint)
    };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    virtual void RefreshPointGeometry(const VPointF &point);
    virtual void RefreshScale();
    auto boundingRect() const -> QRectF override;
    auto shape() const -> QPainterPath override;

    auto GetColorRole() const -> VColorRole;
    void SetColorRole(VColorRole role);

    auto GetLabelLineColorRole() const -> VColorRole;
    void SetLabelLineColorRole(VColorRole role);

    auto GetLabelTextColorRole() const -> VColorRole;
    void SetLabelTextColorRole(VColorRole role);

    auto GetLabelTextHoverColorRole() const -> VColorRole;
    void SetLabelTextHoverColorRole(VColorRole role);

    void RefreshLine();

    virtual void SetActivePoint(bool active);

    /** @brief Set the transient multi-selection state (e.g. passmark points clicked while
     *  adding/removing). Rendered magenta, distinct from the persistent active (orange).
     */
    void SetTemporarySelected(bool selected);

    /** @brief Set the "chosen by a picking workflow" state — an object clicked while adding it to a layer, or
     *  one of the two points of a curve-handle axis.
     *
     *  Rendered orange and bold, like the active point. A state of its own: SetActivePoint is driven by the
     *  property browser, which would clear the mark every time the current item changes — and a click on the
     *  canvas goes through it too (the Dependencies dock shows the object, the Options dock follows), so a mark
     *  laid with SetActivePoint lasts only until the next click.
     */
    void SetLayerSelected(bool selected);

    /** @brief Set the "picked for removal" state — an object the user has clicked to take out of a layer.
     *
     *  Rendered magenta and bold, on top of SetLayerSelected: while removing, every candidate stays orange and
     *  the ones clicked turn magenta, so the user sees what the coming Enter will take away.
     */
    void SetInLayer(bool picked);

    /** @brief Toggle whether the point disc is filled at rest.
     *
     *  Details-mode piece points (VNodePoint) are outline-only when idle; hover/active
     *  states keep their fill. Draw-mode points (VSimplePoint) stay filled.
     */
    void SetPointFillEnabled(bool fill);

protected:
    /** @brief namePoint point label. */
    VGraphicsSimpleTextItem *m_namePoint; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    /** @brief lineName line what we see if label moved too far away from point. */
    VScaledLine *m_lineName; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    bool m_onlyPoint{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_isHovered{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_selected{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_temporarySelected{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_layerSelected{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_inLayer{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_alwaysHovered{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_fillPoints{true}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
    bool m_showLabel{false};  // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    bool m_selectedFromChild{false}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;

    void SetOnlyPoint(bool value);
    auto IsOnlyPoint() const -> bool;

    void RefreshHoverPen();

private:
    Q_DISABLE_COPY_MOVE(VScenePoint) // NOLINT

    void ScaleMainPenWidth(qreal scale);

    VColorRole m_role;
};

//---------------------------------------------------------------------------------------------------------------------
inline auto VScenePoint::GetColorRole() const -> VColorRole
{
    return m_role;
}

//---------------------------------------------------------------------------------------------------------------------
inline void VScenePoint::SetColorRole(VColorRole role)
{
    m_role = role;
}

//---------------------------------------------------------------------------------------------------------------------
inline void VScenePoint::SetPointFillEnabled(bool fill)
{
    m_fillPoints = fill;
}

#endif // VSCENEPOINT_H
