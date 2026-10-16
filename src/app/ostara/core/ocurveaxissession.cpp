/************************************************************************
 **
 **  @file   ocurveaxissession.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The gesture that gives a curve handle its angle from an axis: two points, then a direction.
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

#include "ocurveaxissession.h"

#include <QSharedPointer>

#include "../dialogs/ochooseaxisdirectiondialog.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolabstractcurve.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"

//---------------------------------------------------------------------------------------------------------------------
OCurveAxisSession::OCurveAxisSession(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
OCurveAxisSession::~OCurveAxisSession()
{
    // A session cut short — the dock gone, the window closed — must not leave the curve showing a state the
    // file does not hold.
    if (IsRunning())
    {
        Cancel();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::Start(VToolAbstractCurve *tool, int slot, const QString &slotName)
{
    if (IsRunning() || tool == nullptr || slot < 0 || slot >= tool->AxisSlotCount())
    {
        return;
    }

    m_tool = tool;
    m_slot = slot;
    m_slotName = slotName;
    m_points.clear();

    // The direction the handle already has is the one the window opens on: changing only the direction is then
    // a matter of picking the axis again and pressing the same button twice more.
    m_sense = tool->AxisSlot(slot).IsValid() ? tool->AxisSlot(slot).Sense() : CurveAxisSense::Line;

    emit ShowStatusTip(tr("Click the first point of the axis (Esc to cancel)"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxisSession::IsRunning() const -> bool
{
    return not m_tool.isNull() && m_slot >= 0;
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::OnPointClicked(quint32 pointId)
{
    if (not IsRunning())
    {
        return;
    }

    if (not HasPoint(pointId))
    {
        emit ShowStatusTip(tr("Click a point of the pattern (Esc to cancel)"));
        return;
    }

    if (m_points.contains(pointId))
    {
        // The two points of an axis are two distinct points: a second click on the same one changes nothing.
        emit ShowStatusTip(tr("Click another point than the first one (Esc to cancel)"));
        return;
    }

    m_points.append(pointId);
    emit HighlightPoint(pointId, true);

    if (m_points.size() < 2)
    {
        emit ShowStatusTip(tr("Click the second point of the axis (Esc to cancel)"));
        return;
    }

    OpenDirectionWindow();
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::Cancel()
{
    if (not IsRunning())
    {
        return;
    }

    // Nothing was written: putting back what the session started from is all it takes, and it is a no-op when
    // the window never opened.
    m_tool->RevertAxis();
    End();
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::OpenDirectionWindow()
{
    if (not IsRunning())
    {
        return;
    }

    const auto Apply = [this](CurveAxisSense sense)
    {
        m_sense = sense;
        if (IsRunning())
        {
            Show(OCurveAxis(m_points.at(0), m_points.at(1), m_sense));

            // The preview rebuilds the curve, and rebuilding it puts the visualization's magenta discs back on
            // top of the points just taken: Visualization::DrawPoint ends on setVisible(true), so hiding them
            // once is not enough — every direction shown makes them reappear. The marks are therefore asserted
            // again after each preview, which is also what keeps both points orange while the window is open.
            ReassertHighlights();
        }
    };

    // The curve takes the direction before the window is even up: the starting point of the gesture is what the
    // user is about to look at, not the state it had before.
    Apply(m_sense);

    OChooseAxisDirectionDialog::Config config;
    config.intro = AxisIntro(m_slotName);
    config.acceptText = tr("Apply");
    config.selected = m_sense;

    OChooseAxisDirectionDialog dialog(config, VAbstractValApplication::VApp()->getMainWindow());
    connect(&dialog, &OChooseAxisDirectionDialog::SenseChanged, this, Apply);

    if (dialog.exec() == QDialog::Accepted)
    {
        // The window announces every direction it shows, so the one it closes on is already on screen —
        // applying it again would redraw the curve for nothing. Only an acceptance that never went through a
        // click (no change of direction at all) still needs the final word.
        if (dialog.ChosenSense() != m_sense)
        {
            Apply(dialog.ChosenSense());
        }
        m_tool->CommitAxis();
    }
    else
    {
        m_tool->RevertAxis();
    }

    End();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Lays the mark of the taken points again, without forgetting the ones already marked.
 *
 *  Idempotent on purpose: it is called after every preview, and each call must leave both points — not only the
 *  last one — wearing the orange of the picking. */
void OCurveAxisSession::ReassertHighlights()
{
    for (const quint32 pointId : m_points)
    {
        emit HighlightPoint(pointId, true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::Show(const OCurveAxis &axis)
{
    if (IsRunning())
    {
        m_tool->PreviewAxis(m_slot, axis);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// The sentence deliberately does NOT name the two points the axis joins: the user has just clicked them, they
// are marked on the canvas, and their labels are pattern vocabulary she does not need to read back.
auto OCurveAxisSession::AxisIntro(const QString &slotName) const -> QString
{
    if (slotName.isEmpty())
    {
        return tr("The angle follows the axis.");
    }

    return tr("%1 — the angle follows the axis.").arg(slotName);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxisSession::HasPoint(quint32 pointId) const -> bool
{
    if (m_tool.isNull() || pointId == NULL_ID)
    {
        return false;
    }

    try
    {
        // Every point of the pattern is a candidate, the ones of the curve first: an axis is often one of the
        // two points it joins, and just as often a line of the pattern somewhere else.
        return not m_tool->getData()->GeometricObject<VPointF>(pointId).isNull();
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::ReleaseHighlights()
{
    for (const quint32 pointId : std::as_const(m_points))
    {
        emit HighlightPoint(pointId, false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxisSession::End()
{
    ReleaseHighlights();

    m_points.clear();
    m_tool.clear();
    m_slot = -1;
    m_slotName.clear();
    m_sense = CurveAxisSense::Line;

    emit ShowStatusTip(QString());
    emit Finished();
}
