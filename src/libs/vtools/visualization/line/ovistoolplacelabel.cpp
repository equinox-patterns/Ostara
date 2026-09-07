/************************************************************************
 **
 **  @file   ovistoolplacelabel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Overlay magenta (VisSupportColor) des points retenus pendant la
 **          collecte multi-points de l'outil Place Label (mode Draw).
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

#include "ovistoolplacelabel.h"

#include <QPen>
#include <QtAlgorithms>

#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolPlaceLabel::OVisToolPlaceLabel(const VContainer *data, QGraphicsItem *parent)
  : VisLine(data, parent)
{
    setZValue(2); // Show on top of the real points
    setPen(QPen(Qt::NoPen));
}

//---------------------------------------------------------------------------------------------------------------------
OVisToolPlaceLabel::~OVisToolPlaceLabel()
{
    qDeleteAll(m_markers);
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
void OVisToolPlaceLabel::VisualMode(quint32 id)
{
    Q_UNUSED(id)
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPlaceLabel::SetPointIds(const QVector<quint32> &ids)
{
    m_pointIds = ids;

    qDeleteAll(m_markers);
    m_markers.clear();

    StartVisualMode(); // RefreshGeometry() + AddOnScene() (ajoute la vis à la scène si besoin)
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPlaceLabel::RefreshGeometry()
{
    const qsizetype count = m_pointIds.size();
    for (qsizetype i = 0; i < count; ++i)
    {
        const QSharedPointer<VPointF> p = GetData()->GeometricObject<VPointF>(m_pointIds.at(i));
        VScaledEllipse *const marker = GetPointItem(m_markers, quint32(i), VColorRole::VisSupportColor, this);
        DrawPoint(marker, static_cast<QPointF>(*p));
    }
}
