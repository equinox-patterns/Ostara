/************************************************************************
 **
 **  @file   ovistoolmovebatch.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The preview of a move by batch: the sources, and the ghost of every copy the chain will make.
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

#include "ovistoolmovebatch.h"

#include <QSharedPointer>

#include "../../../tools/drawTools/operation/omovebatchtransform.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "visoperation.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolMoveBatch::OVisToolMoveBatch(const VContainer *data, QGraphicsItem *parent)
  : VisOperation(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor2);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolMoveBatch::SetMoves(const QVector<quint32> &sourceIds, const QVector<OMoveBatchMove> &moves)
{
    m_sourceIds = sourceIds;
    m_moves = moves;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolMoveBatch::RefreshGeometry()
{
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);
    if (originObjects.isEmpty())
    {
        RefreshPreviewLines(false);
        return;
    }

    if (m_moves.isEmpty())
    {
        // The chain is not picked yet: only the source side is drawn.
        RefreshPreviewLines(false);
        return;
    }

    QVector<SourceItem> source;
    source.reserve(m_sourceIds.size());
    for (const quint32 id : std::as_const(m_sourceIds))
    {
        source.append(SourceItem{.id = id});
    }

    QVector<qreal> lengths;
    QVector<qreal> angles;
    QVector<qreal> rotationAngles;
    lengths.reserve(m_moves.size());
    angles.reserve(m_moves.size());
    rotationAngles.reserve(m_moves.size());
    for (const auto &move : std::as_const(m_moves))
    {
        lengths.append(FindLengthFromUser(move.length, GetData()->DataVariables()));
        angles.append(FindValFromUser(move.angle, GetData()->DataVariables()));
        rotationAngles.append(FindValFromUser(move.rotationAngle, GetData()->DataVariables()));
    }

    const OMoveBatchTransform transform(source, m_moves, lengths, angles, rotationAngles, GetData());
    CreateTransformedObjects(iPoint, iCurve, transform);

    // The moved images of the line endpoints are already drawn as points; the source lines stay as reference.
    RefreshPreviewLines(false);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolMoveBatch::CreateTransformedObjects(int &iPoint, int &iCurve, const OMoveBatchTransform &transform)
{
    for (const quint32 id : Objects())
    {
        const QSharedPointer<VGObject> obj = GetData()->GetGObject(id);

        // This check helps to find missed objects in the switch
        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (obj->getType())
        {
            case GOType::Point:
            {
                const QSharedPointer<VPointF> p = GetData()->GeometricObject<VPointF>(id);
                ++iPoint;
                VScaledEllipse *point = GetPoint(static_cast<quint32>(iPoint), VColorRole::VisSupportColor);
                DrawPoint(point, static_cast<QPointF>(transform.Image(*p, QString())));
                break;
            }
            case GOType::Arc:
                iCurve = AddTransformedCurve<VArc>(id, iCurve, transform);
                break;
            case GOType::EllipticalArc:
                iCurve = AddTransformedCurve<VEllipticalArc>(id, iCurve, transform);
                break;
            case GOType::Spline:
                iCurve = AddTransformedCurve<VSpline>(id, iCurve, transform);
                break;
            case GOType::SplinePath:
                iCurve = AddTransformedCurve<VSplinePath>(id, iCurve, transform);
                break;
            case GOType::CubicBezier:
                iCurve = AddTransformedCurve<VCubicBezier>(id, iCurve, transform);
                break;
            case GOType::CubicBezierPath:
                iCurve = AddTransformedCurve<VCubicBezierPath>(id, iCurve, transform);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                qCDebug(vVis, "Object %u cannot be moved by batch, skipped", id);
                break;
        }

        QT_WARNING_POP
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Item>
auto OVisToolMoveBatch::AddTransformedCurve(quint32 id, int i, const OMoveBatchTransform &transform) -> int
{
    const QSharedPointer<Item> curve = GetData()->template GeometricObject<Item>(id);

    ++i;
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::VisSupportColor);
    const Item transformed = transform.Image(*curve, QString());
    DrawPath(path, transformed.GetPath(), transformed.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    return i;
}
