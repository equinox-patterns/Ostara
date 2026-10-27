/************************************************************************
 **
 **  @file   vpattern_geometryrefresh.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Piece geometry refresh methods extracted from vpattern.cpp
 **
 **  @note   Extracted from vpattern.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 24/09/2026.
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

#include "vpattern.h"

#include "../core/vapplication.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vmaingraphicsview.h"

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::IsPieceGeometryDirty() const -> bool
{
    return m_pieceGeometryDirty;
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetPieceGeometryDirty(bool newPieceGeometryDirty)
{
    m_pieceGeometryDirty = newPieceGeometryDirty;
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::RefreshDirtyPieceGeometry(const QList<vidtype> &list)
{
    m_pieceGeometryDirty = false;

    if (list.isEmpty())
    {
        return;
    }

    // Resolve the piece tools and read the relevant settings on the GUI thread.
    QVector<VToolSeamAllowance *> pieceTools;
    pieceTools.reserve(list.size());
    for (auto pieceId : std::as_const(list))
    {
        try
        {
            if (auto *piece = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId)))
            {
                pieceTools.append(piece);
            }
        }
        catch (const VExceptionBadId &)
        {
            // do nothing
        }
    }

    if (pieceTools.isEmpty())
    {
        return;
    }

    const bool combineTogether = VAbstractValApplication::VApp()->ValentinaSettings()->IsBoundaryTogetherWithNotches();
    const bool pieceShowMainPath = VAbstractApplication::VApp()->Settings()->IsPieceShowMainPath();

    // Recompute every piece's geometry serially. The render pipeline reads shared VContainer data
    // and evaluates formulas through qmuparser, neither of which is thread-safe, so the pieces must
    // not be computed concurrently (doing so caused intermittent heap corruption on patterns with
    // many pieces). The two-phase Prepare/Apply split is kept so the compute stays cleanly separated
    // from the scene update, but both phases run on the GUI thread.
    for (auto *piece : std::as_const(pieceTools))
    {
        piece->PrepareRefreshGeometry(combineTogether, pieceShowMainPath);
    }

    // Apply the precomputed geometry to the scene.
    for (auto *piece : std::as_const(pieceTools))
    {
        piece->ApplyBatchGeometry();
    }

    emit CheckLayout();
    VMainGraphicsView::NewSceneRect(sceneDetail, VAbstractValApplication::VApp()->getSceneView());
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SetGBBackupFilePath(const QString &fileName)
{
    m_garbageCollectBackupFilePath = fileName;
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::RefreshPieceGeometry()
{
    // Whether triggered by the debounce timer or called directly for an immediate flush, make sure
    // no pending timer fire follows and triggers a redundant refresh.
    m_refreshGeometryTimer->stop();

    // Off the modeling canvas the pieces are not on screen, so the flush is deferred to the switch
    // back to it. A caller that reads the scene items immediately must ask for the flush itself
    // instead of relying on this slot: see FlushPieceGeometry().
    if (!VAbstractApplication::VApp()->IsAppInGUIMode()
        || VAbstractValApplication::VApp()->GetDrawMode() == Draw::Modeling)
    {
        FlushPieceGeometry();
    }
    else
    {
        m_pieceGeometryDirty = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::FlushPieceGeometry()
{
    m_refreshGeometryTimer->stop();

    if (updatePieces.isEmpty())
    {
        updatePieces = data->DataPieces()->keys();
    }
    RefreshDirtyPieceGeometry(updatePieces);

    updatePieces.clear();
}
