/************************************************************************
 **
 **  @file   mainwindow_graded_build.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: build the stacked graded scene (one layer per size of the active range).
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
#include "ui_mainwindow.h"

#include <QSet>

#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "dialogs/owaitdialog.h"
#include "mainwindow_graded_helpers.h"
#include "ogradedgeometry.h"
#include "core/ocornertrace.h"
#include "core/ogroupsuperpose.h"
#include "core/osizetable.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vexception/vexception.h"
#include "../vmisc/def.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/othumbnailitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vwidgets/vgrainlineitem.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QBrush>
#include <QEventLoop>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPen>
#include <QScopeGuard>
#include <QStringList>
#include <QTimer>
#include <QtGlobal>
#include <QtNumeric>
#include <QUndoStack>
#include <QVector>

#include <algorithm>
#include <memory>

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// .omt dimension base == 1-based index of the CSV label inside the axis values.
auto GradedBaseForLabel(const QStringList &axisValues, const QString &label) -> qreal
{
    const qsizetype idx = axisValues.indexOf(label);
    return idx >= 0 ? static_cast<qreal>(idx + 1) : 1.0;
}

/// Deterministic "silhouette content" fingerprint of a piece for the graded structure
/// signature. Covers the piece-content edits that must refresh the graded canvas (seam
/// allowance/margins + hide/show main path, mounting notches, internal paths, place labels)
/// but DELIBERATELY excludes mx/my (a plain Details move must not relocate the graded canvas)
/// and excludes info-blocks/annotations (those refresh on every apply instead).
auto GradedPieceContentDigest(const VPiece &piece) -> QString
{
    QStringList parts;
    parts.append(QStringLiteral("sa=%1%2%3")
                     .arg(piece.IsSeamAllowance() ? 1 : 0)
                     .arg(piece.IsHideMainPath() ? 1 : 0)
                     .arg(piece.IsShowFullPiece() ? 1 : 0));
    parts.append(QStringLiteral("saw=%1").arg(piece.GetFormulaSAWidth()));

    for (const CustomSARecord &record : piece.GetCustomSARecords())
    {
        parts.append(QStringLiteral("csa=%1,%2,%3,%4,%5")
                         .arg(record.startPoint)
                         .arg(record.path)
                         .arg(record.endPoint)
                         .arg(record.reverse ? 1 : 0)
                         .arg(static_cast<int>(record.includeType)));
    }

    QVector<quint32> internalPaths = piece.GetInternalPaths();
    std::sort(internalPaths.begin(), internalPaths.end());
    for (quint32 id : internalPaths)
    {
        parts.append(QStringLiteral("ip=%1").arg(id));
    }

    QVector<quint32> placeLabels = piece.GetPlaceLabels();
    std::sort(placeLabels.begin(), placeLabels.end());
    for (quint32 id : placeLabels)
    {
        parts.append(QStringLiteral("pl=%1").arg(id));
    }

    // Mounting notches ("repère de montage") live on the main-path nodes (IsPassmark).
    const QVector<VPieceNode> nodes = piece.GetPath().GetNodes();
    for (const VPieceNode &node : nodes)
    {
        if (node.IsPassmark())
        {
            parts.append(QStringLiteral("nm=%1").arg(node.GetId()));
        }
    }

    return parts.join(QLatin1Char('|'));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::BuildGradedScene()
{
    if (m_gradedBuilding)
    {
        return; // re-entrancy guard (undo/redo can re-enter)
    }
    m_gradedBuilding = true;
    const QScopeGuard buildingGuard([this]() { m_gradedBuilding = false; });

    // A deferred rebuild (undo hook) may fire after the user already left Graded — do not
    // mutate the (possibly no longer visible) graded scene in that case.
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Graded)
    {
        return;
    }

    // Entry from another mode forces a full re-capture so ANY piece-shape change (not just the
    // structural ones) is reflected; the flag is consumed by this single call.
    const bool fullCapture = m_gradedForceCapture;
    m_gradedForceCapture = false;

    if (m_sceneGraded == nullptr || doc == nullptr || m_m.isNull() || m_activeGradedRangeName.isEmpty())
    {
        return;
    }

    // Structural signature of the pattern content that feeds the graded canvas. The canvas
    // is rebuilt only when this changes (pieces/groups added or removed, active range). A
    // plain MOVE in Details only changes mx/my — the graded canvas keeps its own positions,
    // so Details and Graded never communicate through moves.
    const auto structureSignature = [this]() -> QString {
        QStringList parts;
        parts.append(m_activeGradedRangeName);
        if (const auto *pieces = pattern->DataPieces(); pieces != nullptr)
        {
            QList<quint32> ids = pieces->keys();
            std::sort(ids.begin(), ids.end());
            for (quint32 id : ids)
            {
                parts.append(QString::number(id));
                // Child digest: adding/removing a piece's grainline in Details propagates to
                // Graded (a child change is a structural event, like adding/removing a piece).
                const VPiece &piece = pieces->value(id);
                parts.append(QStringLiteral("gl%1=%2").arg(id).arg(piece.GetGrainlineGeometry().IsEnabled() ? 1 : 0));
                // Piece-content digest: seam allowance/margins + hide/show main path, mounting
                // notches, internal paths, place labels (mx/my deliberately excluded).
                parts.append(GradedPieceContentDigest(piece));
                // Forced alignment point: changing it (or undoing that change) while Graded is
                // active must re-capture — the item roles carry the point's X per size.
                parts.append(QStringLiteral("ap%1=%2").arg(id).arg(piece.GetGradedAlignPointId()));
            }
        }
        if (const auto *groups = pattern->DataPieceGroups(); groups != nullptr)
        {
            QList<quint32> ids = groups->keys();
            std::sort(ids.begin(), ids.end());
            for (quint32 groupId : ids)
            {
                parts.append(QLatin1Char('g') + QString::number(groupId));
                const OPieceGroup &group = groups->value(groupId);
                parts.append(QStringLiteral("ggl%1=%2").arg(groupId).arg(group.m_glGrainline.IsEnabled() ? 1 : 0));
                QList<quint32> members = group.pieceIds;
                std::sort(members.begin(), members.end());
                for (quint32 member : members)
                {
                    parts.append(QString::number(member));
                }
                // The group's common point and whether it drives the alignment.
                parts.append(QStringLiteral("gap%1=%2:%3:%4").arg(groupId).arg(group.alignPointSourceId)
                                 .arg(group.alignPointMemberId).arg(group.alignOnPoint ? 1 : 0));
            }
        }
        return parts.join(QLatin1Char('|'));
    };
    const QString signature = structureSignature();

    if (!fullCapture && signature == m_gradedSceneSignature && not m_sceneGraded->items().isEmpty())
    {
        ApplyGradedLayoutOffsets(); // keep the (idempotent) positions in sync with the model
        return;
    }

    // Single "please wait" feedback for the whole capture. The shared seam helper runs silently
    // in this loop (PrepareDetailsForLayout(details, false)), so no progress bar flashes here;
    // instead a static wait dialog appears only once the build has really taken the threshold.
    // Fast builds end before the dialog is ever painted — nothing flashes.
    constexpr int kGradedWaitThresholdMs = 400;
    QElapsedTimer waitTimer;
    waitTimer.start();
    std::unique_ptr<OWaitDialog> waitDialog; // RAII: closed/destroyed on every path out of this scope
    const auto ShowWaitDialogIfLong = [this, &waitTimer, &waitDialog]() {
        if (waitDialog == nullptr && waitTimer.elapsed() >= kGradedWaitThresholdMs)
        {
            waitDialog.reset(new OWaitDialog(tr("Preparing the graded scene…"), this));
            waitDialog->show();
            // The window system needs a few event turns to map and paint the dialog — a single
            // pump can leave the paint pending, notably right after the modal size-range dialog
            // has closed (first graded build).
            for (int i = 0; i < 4; ++i)
            {
                QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            }
        }
    };

    // Always drop stale silhouettes before (re)building.
    m_sceneGraded->clear();

    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // Suppress heavy doc emissions during the capture loop (RAII).
    const QSignalBlocker signalBlocker(doc);

    OSizeTable reader;
    reader.Load();
    if (not reader.HasMultiSizeTable())
    {
        return;
    }

    const QStringList axes = reader.AxisNames();
    const int gradedAxisPos = static_cast<int>(axes.indexOf(range.sizeAxis));
    if (gradedAxisPos < 0)
    {
        return;
    }

    QMap<QString, QStringList> axisValues;
    for (const QString &axis : axes)
    {
        axisValues.insert(axis, reader.AxisValues(axis));
    }

    const QStringList gradedValues = axisValues.value(range.sizeAxis);
    const int minIdx = static_cast<int>(gradedValues.indexOf(range.minValue));
    const int maxIdx = static_cast<int>(gradedValues.indexOf(range.maxValue));
    if (minIdx < 0 || maxIdx < 0 || minIdx > maxIdx)
    {
        return;
    }

    // Base value of a fixed (non graded) axis slot.
    auto FixedBase = [&axes, &axisValues, &range](int slot) -> qreal {
        const QString label = range.axisValues.value(axes.value(slot));
        return GradedBaseForLabel(axisValues.value(axes.value(slot)), label);
    };

    // Snapshot the live state.
    const qreal origA = m_currentDimensionA;
    const qreal origB = m_currentDimensionB;
    const qreal origC = m_currentDimensionC;
    const bool wasModified = doc->IsModified();
    if (m_gradation != nullptr)
    {
        m_gradation->stop();
    }

    const int layerCount = maxIdx - minIdx + 1;
    for (int idx = minIdx; idx <= maxIdx; ++idx)
    {
        qreal baseA = 1.0, baseB = 1.0, baseC = 1.0;
        const qreal gradedBase = static_cast<qreal>(idx + 1);
        if (gradedAxisPos == 0)
        {
            baseA = gradedBase;
        }
        else
        {
            baseA = FixedBase(0);
        }
        if (gradedAxisPos == 1)
        {
            baseB = gradedBase;
        }
        else
        {
            baseB = FixedBase(1);
        }
        if (gradedAxisPos == 2)
        {
            baseC = gradedBase;
        }
        else
        {
            baseC = FixedBase(2);
        }

        pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});
        m_m->StoreNames(false);
        m_m->ReadMeasurements(baseA, baseB, baseC);
        doc->LiteParseTree(Document::FullLiteParse);

        QVector<DetailForLayout> details;
        QVector<VLayoutPiece> pieces;
        try
        {
            details = SortDetailsForLayout(pattern->DataPieces(), pattern);
            pieces = PrepareDetailsForLayout(details, doc, false);
        }
        catch (const VException &e)
        {
            // A piece that cannot be prepared for a graded size must not crash the mode entry —
            // skip the remaining sizes; the restore below still runs and the partial scene is shown.
            qWarning("Graded capture failed for a size - %s", qUtf8Printable(e.ErrorMessage()));
            break;
        }

        // The stretch a group's members share is brought onto ONE line before anything is drawn: what
        // is left of the doubling is a matter of geometry, not of dashes (OGroupSuperpose).
        OGroupSuperpose::Snap(details, pieces, pattern);

        // One real piece render per (piece × size); z = N − layer → reference (min size) on
        // top. Render the piece the same way Val Fork's graded scene does (piece.GetItem):
        // main path + seam allowance + internal paths + passmarks + place labels + grainline
        // + fold line — NOT an invented silhouette.
        QVector<QGraphicsItem *> itemsOfSize;

        const int layerPos = idx - minIdx;
        const int z = layerCount - layerPos;
        const bool isReference = (layerPos == 0);

        for (int k = 0; k < pieces.size() && k < details.size(); ++k)
        {
            const quint32 pieceId = details.at(k).id;

            // Frozen base: captured the first time the piece enters a graded build. A plain
            // Details move afterwards must NOT relocate the graded canvas, even across captures.
            const QPointF livePos(pieces.at(k).GetMx(), pieces.at(k).GetMy());
            const QPointF basePos = m_gradedFrozenBases.value(pieceId, livePos);
            if (!m_gradedFrozenBases.contains(pieceId))
            {
                m_gradedFrozenBases.insert(pieceId, livePos);
            }

            // grabSurface: the contour is cut at the corners right below (OCornerTrace, for the
            // trace pattern), and a cut path has no surface left — the item would only answer on
            // its outline. The closed contour is kept as the surface to grab on.
            QGraphicsItem *item = pieces.at(k).GetItem(false, false, false, true);
            // GetItem paints every trace with a default (black) pen — apply the theme's piece
            // color, the same one Details paints piece/group silhouettes with (VColorRole::PieceColor).
            RecolorGradedItemPens(item);
            item->setPos(basePos);
            item->setData(kGradedBasePosRole, basePos); // frozen base for idempotent re-applies
            item->setData(kGradedSizeRole, gradedValues.at(idx)); // size label → per-size styling
            item->setZValue(z);
            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
            item->setFlag(QGraphicsItem::ItemIsMovable, isReference);
            item->setData(kGradedPieceIdRole, pieceId);
            item->setData(kGradedIsRefRole, isReference);

            // Forced alignment point: captured here, while this size's measurements are still in the
            // container. The alignment pass runs later, on the restored live combination, and has
            // nothing left but the item roles — hence the per-item role rather than a table.
            const qreal alignPointX = OGraded::UnitAlignPointX(pieceId, details.at(k).piece, pattern);
            if (!qIsNaN(alignPointX))
            {
                item->setData(kGradedAlignPointXRole, alignPointX);
            }
            m_sceneGraded->addItem(item);
            itemsOfSize.append(item);
        }

        // The dash pattern leaves from the places the group's members have in common — the corners of
        // the trace, and every construction point two members share — so both members' patterns leave
        // from the very same places (OCornerTrace).
        {
            QSet<QGraphicsItem *> shared;
            const auto *groups = pattern->DataPieceGroups();
            if (groups != nullptr)
            {
                for (auto group = groups->cbegin(); group != groups->cend(); ++group)
                {
                    QVector<QGraphicsPathItem *> members;
                    for (const quint32 pieceId : group.value().pieceIds)
                    {
                        for (QGraphicsItem *candidate : itemsOfSize)
                        {
                            if (GradedPieceId(candidate) == pieceId)
                            {
                                if (auto *path = dynamic_cast<QGraphicsPathItem *>(candidate); path != nullptr)
                                {
                                    members.append(path);
                                    shared.insert(candidate);
                                }
                            }
                        }
                    }
                    if (members.size() > 1)
                    {
                        OCornerTrace::RestartAtSharedStretches(members);
                    }
                }
            }
            for (QGraphicsItem *candidate : itemsOfSize)
            {
                if (!shared.contains(candidate))
                {
                    OCornerTrace::RestartAtCorners(dynamic_cast<QGraphicsPathItem *>(candidate));
                }
            }
        }

        // Let a long build surface its wait dialog between sizes (never mid-parse).
        ShowWaitDialogIfLong();
    }

    // Restore the live combination without leaving the DOM dirty.
    pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});
    m_m->StoreNames(false);
    m_m->ReadMeasurements(origA, origB, origC);
    doc->LiteParseTree(Document::FullLiteParse);
    doc->RefreshPieceGeometry();
    doc->SetModified(wasModified);

    ShowWaitDialogIfLong();

    // Alignment + per-range content offsets (recomputed from the model). A fresh capture always
    // (re)builds the full decor + children — the active-range content may differ from the last apply.
    ApplyGradedLayoutOffsets(true);

    // Reference drag bookkeeping: base positions + one-time hooks (mouse release,
    // undo stack) driving the fan-out and the rebuild after undo/redo.
    m_gradedBasePositions.clear();
    const auto allItems = m_sceneGraded->items();
    for (auto *baseItem : allItems)
    {
        if (GradedIsReference(baseItem))
        {
            m_gradedBasePositions.insert(GradedPieceId(baseItem), baseItem->pos());
        }
    }

    if (not m_gradedDragHooked)
    {
        m_gradedDragHooked = true;
        connect(ui->view, &VMainGraphicsView::MouseRelease, this, &MainWindow::GradedMouseReleased,
                Qt::UniqueConnection);

        // Details-like block selection: only the clicked info block shows its editing outline
        // + handles (the others are deactivated).
        connect(m_sceneGraded, &VMainGraphicsScene::ItemByMousePress, this, [this](QGraphicsItem *pressed) {
            auto *pressedBlock = dynamic_cast<OInfoBlockItem *>(pressed);
            auto *pressedAnnotation = dynamic_cast<OAnnotationItem *>(pressed);
            auto *pressedThumbnail = dynamic_cast<OThumbnailItem *>(pressed);
            const auto sceneItems = m_sceneGraded->items();
            for (auto *it : sceneItems)
            {
                if (auto *blk = dynamic_cast<OInfoBlockItem *>(it))
                {
                    blk->SetActive(blk == pressedBlock);
                }
                else if (auto *ann = dynamic_cast<OAnnotationItem *>(it))
                {
                    ann->SetActive(ann == pressedAnnotation);
                }
                else if (auto *thumb = dynamic_cast<OThumbnailItem *>(it))
                {
                    thumb->SetActive(thumb == pressedThumbnail);
                }
            }
        });

        auto *undoStack = VAbstractApplication::VApp()->getUndoStack();
        // Note: no Qt::UniqueConnection here — it is only legal for pointer-to-member
        // functions, not lambdas (Qt warns and drops the connection). The surrounding
        // m_gradedDragHooked guard already ensures this block runs once.
        connect(undoStack, &QUndoStack::indexChanged, this,
                [this]() {
                    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded
                        && not m_activeGradedRangeName.isEmpty())
                    {
                        // A right-click alignment edit flags a single unit: refresh the geometry
                        // of THAT unit only (RealignGradedUnit) instead of the whole canvas.
                        if (m_gradedPendingAlignUnit != 0)
                        {
                            const quint32 unitId = m_gradedPendingAlignUnit;
                            const bool isGroup = m_gradedPendingAlignIsGroup;
                            m_gradedPendingAlignUnit = 0;
                            m_gradedPendingAlignIsGroup = false;
                            QTimer::singleShot(0, this,
                                               [this, unitId, isGroup]() { RealignGradedUnit(unitId, isGroup); });
                            return;
                        }
                        // Defer out of the current event; route through BuildGradedScene so the
                        // structure/content signature decides: unchanged → apply-only offsets,
                        // changed (e.g. an undone piece-content edit) → full re-capture.
                        QTimer::singleShot(0, this, [this]() { BuildGradedScene(); });
                    }
                });

        // Right-click on a graded piece/group opens the alignment menu (mode + range guarded
        // inside the handler). The signal is emitted by the view for any non-tool item under the
        // cursor; the graded silhouette roots and group hosts are plain path items, so they fire it.
        connect(ui->view, &VMainGraphicsView::SceneContextMenuRequested, this,
                &MainWindow::GradedAlignContextMenu, Qt::UniqueConnection);
    }

    // Drop frozen bases of pieces that no longer exist (removed in Details).
    if (const auto *currentPieces = pattern->DataPieces(); currentPieces != nullptr)
    {
        for (auto it = m_gradedFrozenBases.begin(); it != m_gradedFrozenBases.end();)
        {
            if (currentPieces->contains(it.key()))
            {
                ++it;
            }
            else
            {
                it = m_gradedFrozenBases.erase(it);
            }
        }
    }

    // Trace styles by size (color + px dashes): fresh capture → re-apply on the new scene.
    ApplyGradedSizeStylesToScene();

    m_gradedSceneSignature = signature;
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Re-color one layout item subtree (the silhouette tree returned by VLayoutPiece::GetItem) with
/// the theme's piece color, mirroring what Details paints every piece trace with (VColorRole::PieceColor).
/// Interactive children (info blocks, annotations, grainline, miniature) manage their own colors
/// and are skipped.
void RecolorGradedLayoutSubtree(QGraphicsItem *item, const QColor &color)
{
    if (item == nullptr)
    {
        return;
    }
    // Interactive children of a graded piece/host resolve their own colors → leave them untouched
    // (and do not descend: their interior text/fills are theirs to color).
    if (item->type() == OInfoBlockItem::Type || item->type() == OAnnotationItem::Type
        || item->type() == VGrainlineItem::Type || item->type() == OThumbnailItem::Type)
    {
        return;
    }

    if (auto *path = dynamic_cast<QGraphicsPathItem *>(item))
    {
        QPen pen = path->pen();
        if (pen.style() != Qt::NoPen)
        {
            pen.setColor(color);
            path->setPen(pen);
        }
        // A layout fill/fold item paints its interior with the pen color (VGraphicsFillItem paint),
        // so re-tinting the pen recolors both the stroke and any filled area.
    }
    else if (auto *text = dynamic_cast<QGraphicsSimpleTextItem *>(item))
    {
        text->setBrush(QBrush(color));
    }

    const auto children = item->childItems();
    for (auto *child : children)
    {
        RecolorGradedLayoutSubtree(child, color);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void RecolorGradedItemPens(QGraphicsItem *root)
{
    if (root == nullptr)
    {
        return;
    }
    const QColor color = VSceneStylesheet::PatternPieceStyle().PieceColor();
    RecolorGradedLayoutSubtree(root, color);
}

