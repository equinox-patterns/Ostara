/************************************************************************
 **
 **  @file   mainwindow_thumbnail.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Piece miniatures at the reference size of the pattern.
 **
 **          A piece's geometry is memoized when the pattern is parsed, so the
 **          drawing of the reference size cannot be recomputed on demand: the
 **          document has to stand on that size, be parsed, and be put back. This
 **          is the same round trip the Graded mode and the exports already do,
 **          and it is why it happens on a discrete event (coalesced by a timer)
 **          rather than on every repaint.
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

#include "mainwindow.h"

#include "core/odetailspropertybrowser.h"
#include "widgets/ODockManager.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/tools/othumbnailcache.h"
#include "../vtools/tools/vtoolseamallowance.h"

#include <QScopeGuard>
#include <QTimer>

namespace
{
/**
 * @brief The base to freeze an axis on.
 *
 * DimensionABase() is 0 when the table carries no anchor for that axis. Falling back to the active
 * value keeps the miniature on a size somebody actually chose, instead of measuring the piece at a
 * base of zero.
 */
auto ResolveReferenceBase(qreal anchorBase, qreal activeBase) -> qreal
{
    return anchorBase > 0.0 ? anchorBase : activeBase;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ScheduleThumbnailSnapshots()
{
    if (m_thumbnailSnapshots == nullptr)
    {
        return;  // created by the constructor; nothing to arm otherwise
    }
    // The flag must be right before anything repaints: a block that would draw the live geometry
    // while a reference drawing is on its way is a block that changes size under the user's eyes.
    UpdateThumbnailReferenceFlag();
    // A burst of edits (typing, dragging a node) must collapse into one round trip.
    m_thumbnailSnapshots->start();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateThumbnailReferenceFlag()
{
    bool frozen = false;
    if (doc != nullptr && not m_m.isNull() && not m_m->Dimensions().isEmpty())
    {
        const qreal refA = ResolveReferenceBase(m_m->DimensionABase(), m_currentDimensionA);
        const qreal refB = ResolveReferenceBase(m_m->DimensionBBase(), m_currentDimensionB);
        const qreal refC = ResolveReferenceBase(m_m->DimensionCBase(), m_currentDimensionC);
        frozen = not (qFuzzyCompare(refA, m_currentDimensionA) && qFuzzyCompare(refB, m_currentDimensionB)
                      && qFuzzyCompare(refC, m_currentDimensionC));
    }
    OThumbnailCache::SetFrozenReference(frozen);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ApplyReferenceSize(qreal baseA, qreal baseB, qreal baseC)
{
    if (m_gradation != nullptr)
    {
        m_gradation->stop();  // a pending size switch would fight this one
    }
    pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});
    m_m->StoreNames(false);
    m_m->ReadMeasurements(baseA, baseB, baseC);
    doc->LiteParseTree(Document::FullLiteParse);
    StoreDimensions();
    // The capture that follows reads the scene items, so they must already carry the reference
    // geometry. The mode-guarded refresh would only mark them dirty when the app is off the modeling
    // canvas — exactly the state a file open leaves behind — and the capture would then glue the
    // reference internal paths to the active outline, a drawing that belongs to no size at all.
    doc->FlushPieceGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshThumbnailSnapshots()
{
    if (doc == nullptr || m_m.isNull())
    {
        return;
    }

    // This parses the document twice; nothing it does may start another round.
    if (m_thumbnailSnapshotBusy)
    {
        return;
    }
    m_thumbnailSnapshotBusy = true;
    const auto busyGuard = qScopeGuard([this]() { m_thumbnailSnapshotBusy = false; });

    const QHash<quint32, VPiece> *pieces = std::as_const(*pattern).DataPieces();
    if (pieces == nullptr || pieces->isEmpty())
    {
        OThumbnailCache::Clear();
        return;
    }

    // Nothing to freeze as long as no piece shows a miniature.
    bool anyThumbnail = false;
    for (auto it = pieces->constBegin(); it != pieces->constEnd(); ++it)
    {
        if (it.value().GetThumbnailData().IsActive())
        {
            anyThumbnail = true;
            break;
        }
    }
    if (not anyThumbnail)
    {
        OThumbnailCache::Clear();
        return;
    }

    // Without axes (a single-size table) there is only one size: the live drawing is already it.
    if (m_m->Dimensions().isEmpty())
    {
        OThumbnailCache::Clear();
        return;
    }

    // The reference size of an axis is its anchor — the same value the cartouche shows for it.
    const qreal refA = ResolveReferenceBase(m_m->DimensionABase(), m_currentDimensionA);
    const qreal refB = ResolveReferenceBase(m_m->DimensionBBase(), m_currentDimensionB);
    const qreal refC = ResolveReferenceBase(m_m->DimensionCBase(), m_currentDimensionC);

    if (qFuzzyCompare(refA, m_currentDimensionA) && qFuzzyCompare(refB, m_currentDimensionB)
        && qFuzzyCompare(refC, m_currentDimensionC))
    {
        // Already standing on the reference size: the tools carry the right geometry.
        OThumbnailCache::Clear();
        return;
    }

    const qreal originalA = m_currentDimensionA;
    const qreal originalB = m_currentDimensionB;
    const qreal originalC = m_currentDimensionC;
    const bool wasModified = doc->IsModified();

    // 1. Stand on the reference size and let every piece be rebuilt there.
    ApplyReferenceSize(refA, refB, refC);

    // 2. Remember every miniature drawing — the parse above recreated all the tools, so this has to
    //    happen now, while the reference geometry is the one in memory.
    OThumbnailCache::Clear();
    const QHash<quint32, VPiece> *referencePieces = std::as_const(*pattern).DataPieces();
    if (referencePieces != nullptr)
    {
        for (auto it = referencePieces->constBegin(); it != referencePieces->constEnd(); ++it)
        {
            if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(it.key()));
                tool != nullptr)
            {
                tool->CaptureThumbnailContent();
            }
        }
    }

    // 3. Put the document back exactly where the user left it — including its "modified" flag: a
    //    thumbnail must never make the pattern look edited.
    ApplyReferenceSize(originalA, originalB, originalC);
    doc->SetModified(wasModified);

    // 4. Those two parses destroyed the tools the dock was holding and repopulated the blocks with
    //    the cached drawings (UpdateThumbnail reads the cache). Repopulate the dock's piece page.
    if (m_dockManager != nullptr)
    {
        m_dockManager->detailsOptions()->RefreshPieceAfterParse();
    }
}
