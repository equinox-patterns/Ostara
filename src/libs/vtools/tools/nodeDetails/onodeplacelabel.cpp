/************************************************************************
 **
 **  @file   onodeplacelabel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Clickable place-label point of a Details piece — implementation
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

#include "onodeplacelabel.h"

#include "../vgeometry/vplacelabelitem.h"
#include "../vwidgets/vgraphicssimpletextitem.h"

//---------------------------------------------------------------------------------------------------------------------
ONodePlaceLabel::ONodePlaceLabel(QGraphicsItem *parent)
  : VScenePoint(VColorRole::PiecePointColor, parent)
{
    // Details-mode piece points are outline-only at rest; the fill appears on hover/active.
    SetPointFillEnabled(false);

    // VScenePoint defaults to the Draw-mode DraftLabel* roles — switch to the piece roles, the
    // same recolouring VNodePoint applies for the piece's own node points.
    SetLabelTextColorRole(VColorRole::PieceNodeLabelColor);
    SetLabelTextHoverColorRole(VColorRole::PieceNodeLabelHoverColor);
    SetLabelLineColorRole(VColorRole::PieceNodeLabelLineColor);
    m_namePoint->SetShowParentTooltip(false);

    // Standard selectable-point semantics: a click selects the handle (Qt single-selection);
    // picking any other item or the empty canvas clears it.
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setCursor(Qt::ArrowCursor);
}

//---------------------------------------------------------------------------------------------------------------------
auto ONodePlaceLabel::itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant
{
    // Mirror the Qt selection state onto VScenePoint's active pen, so a selected label handle is
    // drawn exactly like an active point (orange) and reverts as soon as Qt deselects it.
    if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        SetActivePoint(isSelected());
    }
    return VScenePoint::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
void ONodePlaceLabel::UpdateFromLabel(quint32 labelId, const VPlaceLabelItem &label)
{
    m_id = labelId;
    setToolTip(label.name());

    // VScenePoint::RefreshPointGeometry sets the scene position from the label coordinates, the
    // point-name text at mx/my and re-evaluates label visibility (zoom/settings) + leader line.
    RefreshPointGeometry(label);
}
