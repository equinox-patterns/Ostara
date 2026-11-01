/************************************************************************
 **
 **  @file   ovistoolclosedart.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The preview of the tool that closes a dart, see ovistoolclosedart.h.
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

#include "ovistoolclosedart.h"

#include <QSharedPointer>
#include <QtNumeric>

#include "../../../tools/drawTools/operation/oclosedartrule.h"
#include "../vgeometry/vpointf.h"
#include "../vexception/vexception.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolCloseDart::OVisToolCloseDart(const VContainer *data, QGraphicsItem *parent)
  : VisToolRotation(data, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolCloseDart::RefreshGeometry()
{
    // An operation can be started from a line alone: the operated-object list is empty then, but the attached
    // line still has a source side to draw.
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    if (const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);
        originObjects.isEmpty() || (GetMode() == Mode::Creation && !ObjectSelected()))
    {
        // Nothing is selected yet: only the source side of the lines is drawn.
        RefreshPreviewLines(false);
        return;
    }

    DrawPivotEcho(iPoint);

    // No angle before the pivot and the point that travels are both known. Until then the pattern is shown as
    // it is, and no ghost promises a rotation that is not decided yet.
    const qreal angle = ClosingAngle();
    const bool decided = !qIsNaN(angle);

    if (decided)
    {
        // The inherited preview turns its ghost by this angle, exactly as it does for a typed one. The angle is
        // handed over as the number it already is: SetAngle() would read it back as a formula in the user's own
        // notation, which is a round trip a number read from the pattern has no business making.
        SetRotationAngle(angle);
        CreateRotatedObjects(iPoint, iCurve, angle);

        SetToolTip(tr("Closing angle = %1°, <b>Mouse click</b> - select the Target point").arg(AngleToUser(angle)));
    }
    else
    {
        // No angle yet: the ghost is drawn where the objects already are, which is also what keeps a ghost of
        // a previous state from lingering on the scene when the dart stops being defined.
        CreateRotatedObjects(iPoint, iCurve, 0.);
    }

    RefreshPreviewLines(decided);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolCloseDart::DrawPivotEcho(int &iPoint)
{
    const quint32 pivot = OriginPointId();

    // Nothing to echo before the pivot is picked, and nothing to add when the pattern already echoes it as one
    // of the operated objects: a second copy in the same place would only cost a redraw.
    if (pivot == NULL_ID || Objects().contains(pivot))
    {
        return;
    }

    try
    {
        // Drawn by the very function that draws the operated objects, and with the same role: the pivot is not
        // a different kind of thing, it is one more point of what the reader is looking at.
        AddOriginPoint(pivot, iPoint);
    }
    catch (const VException &e)
    {
        // The pivot can be gone from the pattern. This runs on every mouse move, and an exception leaving here
        // would end the application: the echo simply skips the point.
        qCDebug(vVis, "Skipping the pivot echo: %s", e.ErrorMessage().toUtf8().constData());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolCloseDart::ClosingAngle() const -> qreal
{
    if (OriginPointId() == NULL_ID || m_movingPointId == NULL_ID)
    {
        return qQNaN(); // the dart is not drawn yet: its ghost waits for its two points
    }

    try
    {
        const QPointF pivot = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(OriginPointId()));
        const QPointF moving = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_movingPointId));

        // While the user is choosing it, the target is where the cursor is: that is what makes the ghost
        // follow the mouse. The click then lands on a real point, and the angle becomes the exact one.
        const QPointF target = m_targetPointId != NULL_ID
                                   ? static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_targetPointId))
                                   : ScenePos();

        return OCloseDartRule::ClosingAngle(pivot, moving, target, m_divisor);
    }
    catch (const VException &e)
    {
        // A point of the dart is gone. The exception must not leave here: this runs on every mouse move, and
        // one raised from the Qt event loop ends the application.
        qCDebug(vVis, "Skipping the dart preview: %s", e.ErrorMessage().toUtf8().constData());
        return qQNaN();
    }
}
