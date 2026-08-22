/************************************************************************
 **
 **  @file   osamarginhighlights.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  Persistent SA margin highlight children for a piece tool.
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

#include "osamarginhighlights.h"

#include "osamarginpalette.h"

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPen>
#include <QSet>

#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vlayout/vlayoutpoint.h"

namespace
{
constexpr int kHighlightPenWidth = 2;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OSAMarginHighlights::OSAMarginHighlights(QGraphicsItem *parent)
  : m_parent(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
OSAMarginHighlights::~OSAMarginHighlights()
{
    Clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OSAMarginHighlights::Clear()
{
    for (auto *item : m_items)
    {
        delete item;
    }
    m_items.clear();
    m_colorIndices.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OSAMarginHighlights::UpdateColors()
{
    for (int i = 0; i < m_items.size() && i < m_colorIndices.size(); ++i)
    {
        m_items.at(i)->setPen(QPen(OSAMarginPalette::Color(m_colorIndices.at(i)), kHighlightPenWidth));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OSAMarginHighlights::Update(const VPiece &piece, const VContainer *data)
{
    Clear();

    if (!m_parent || !data)
    {
        return;
    }

    const QVector<bool> shows = piece.GetSAMarginGroupShowStates();
    const QVector<QVector<quint32>> groups = piece.GetSAMarginGroupNodeIds();
    const QVector<int> colors = piece.GetSAMarginGroupColorIndices();
    const VPiecePath &piecePath = piece.GetPath();

    // Collect NodePoint indices once (invariant across groups).
    QVector<int> pointIndices;
    for (int i = 0; i < piecePath.CountNodes(); ++i)
    {
        if (piecePath.at(i).GetTypeTool() == Tool::NodePoint)
        {
            pointIndices.append(i);
        }
    }

    if (pointIndices.size() < 2)
    {
        return;
    }

    for (int gi = 0; gi < groups.size() && gi < shows.size() && gi < colors.size(); ++gi)
    {
        if (!shows.at(gi))
        {
            continue;
        }

        const QVector<quint32> &nodeIds = groups.at(gi);
        if (nodeIds.size() < 2)
        {
            continue;
        }

        const int colorIdx = colors.at(gi);

        // Build a set of group node IDs for O(1) lookup.
        const QSet<quint32> groupSet(nodeIds.begin(), nodeIds.end());

        // Find start/end in the circular list of NodePoints.
        int startIdx = -1;
        int endIdx = -1;

        for (int pi = 0; pi < pointIndices.size(); ++pi)
        {
            const int idx = pointIndices[pi];
            if (!groupSet.contains(piecePath.at(idx).GetId()))
            {
                continue;
            }

            const int prevPi = (pi == 0) ? static_cast<int>(pointIndices.size()) - 1 : pi - 1;
            if (!groupSet.contains(piecePath.at(pointIndices[prevPi]).GetId()))
            {
                startIdx = idx;
            }

            const int nextPi = (pi == pointIndices.size() - 1) ? 0 : pi + 1;
            if (!groupSet.contains(piecePath.at(pointIndices[nextPi]).GetId()))
            {
                endIdx = idx;
            }
        }

        // Full contour: all NodePoints are in the group.
        if (startIdx < 0 && endIdx < 0)
        {
            startIdx = 0;
            endIdx = static_cast<int>(piecePath.CountNodes()) - 1;
        }
        else if (startIdx < 0 || endIdx < 0)
        {
            continue;
        }

        // Extract the segment nodes in clockwise order (including curve nodes).
        QVector<VPieceNode> segmentNodes;
        if (startIdx <= endIdx)
        {
            segmentNodes.reserve(endIdx - startIdx + 1);
            for (int i = startIdx; i <= endIdx; ++i)
            {
                segmentNodes.append(piecePath.at(i));
            }
        }
        else
        {
            segmentNodes.reserve(piecePath.CountNodes() - startIdx + endIdx + 1);
            for (int i = startIdx; i < piecePath.CountNodes(); ++i)
            {
                segmentNodes.append(piecePath.at(i));
            }
            for (int i = 0; i <= endIdx; ++i)
            {
                segmentNodes.append(piecePath.at(i));
            }
        }

        const QVector<VLayoutPoint> segLayoutPoints =
            VPiecePath::NodesToPoints(data, segmentNodes, piecePath.GetName());
        QVector<QPointF> segPoints;
        CastTo(segLayoutPoints, segPoints);
        if (segPoints.size() < 2)
        {
            continue;
        }

        QPainterPath segPath = VPiecePath::MakePainterPath(segPoints);

        auto *highlightItem = new QGraphicsPathItem(segPath, m_parent);
        highlightItem->setPen(QPen(OSAMarginPalette::Color(colorIdx), kHighlightPenWidth));
        highlightItem->setBrush(QBrush(Qt::NoBrush));
        highlightItem->setZValue(100);
        m_items.append(highlightItem);
        m_colorIndices.append(colorIdx);
    }
}
