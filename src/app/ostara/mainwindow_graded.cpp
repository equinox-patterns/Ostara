/************************************************************************
 **
 **  @file   mainwindow_graded.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Graded mode entry: mode button handling and size-range flow.
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

#include "core/savegradedrangecontent.h"
#include "dialogs/ogradedrangedialog.h"
#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "widgets/InfoBlock/OPieceCartoucheTable.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "widgets/ODockManager.h"
#include "widgets/TopBar.h"
#include "xml/vpattern.h"
#include "xml/vpattern_graded.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vexception/vexception.h"
#include "../vmisc/def.h"
#include "../vpatterndb/calculator.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/ocartouchepainter.h"
#include "../vwidgets/vgrainlineitem.h"
#include "../vwidgets/vpiecegrainline.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QDomDocument>
#include <QGraphicsItem>
#include <QHash>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QScopeGuard>
#include <QStringList>
#include <QTimer>
#include <QtGlobal>
#include <QtMath>
#include <QUndoStack>
#include <QVector>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionGraded(bool checked)
{
    if (not checked)
    {
        ui->actionGraded->setChecked(true);
        return;
    }

    ArrowTool(true);

    const Draw prevMode = VAbstractValApplication::VApp()->GetDrawMode();

    ui->actionDraw->setChecked(false);
    ui->actionDetails->setChecked(false);
    ui->actionGraded->setChecked(true);
    ui->actionLayout->setChecked(false);

    // Guard: Graded needs at least one piece (created in Details) and a loaded
    // measurements table. The multi-size requirement is checked by the entry
    // flow (EnterGradedMode).
    if (not VAbstractValApplication::VApp()->getOpeningPattern() &&
        (pattern->DataPieces()->isEmpty() || m_m.isNull()))
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("You can't use Graded mode yet. Please, create at least one "
                                    "workpiece and load a measurements table."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        ui->actionGraded->setChecked(false);
        prevMode == Draw::Calculation ? ui->actionDraw->setChecked(true)
                                      : ui->actionDetails->setChecked(true);
        return;
    }

    // Switch to Graded first (empty canvas, graded chrome), so the entry dialog opens
    // ABOVE the empty graded canvas — not from the mode the user clicked in.
    qCDebug(vMainWindow, "Show graded scene");
    m_dockManager->saveDockStateForMode(prevMode);

    if (not switchScene(Draw::Graded))
    {
        return;
    }

    SetEnableWidgets(true);
    m_dockManager->applyModeDocks(Draw::Graded);
    m_topBar->SetGradedChrome(true);

    // Entry flow: create or reuse the size range for the active measurements table.
    // Cancel (or no usable table) → back to the previous mode.
    if (not EnterGradedMode())
    {
        prevMode == Draw::Calculation ? ActionDraw(true) : ActionDetails(true);
        return;
    }

    m_gradedForceCapture = true; // entering from another mode → always refresh the silhouettes
    BuildGradedScene();
    // Frame the whole graded arrangement on entry (same as the "Zoom fit best" action).
    ui->view->ZoomFitBest();

    m_statusLabel->setText(QString());
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::EnterGradedMode()
{
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        return false;
    }

    const QString canonical =
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());

    OPieceCartoucheTable reader;
    reader.Load();
    if (not reader.HasMultiSizeTable() || reader.AxisNames().isEmpty())
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("Graded mode needs a multi-size measurements table (.omt)."),
                                 QMessageBox::Ok);
        return false;
    }

    const QStringList axes = reader.AxisNames();
    QMap<QString, QStringList> axisValues;
    for (const QString &axis : axes)
    {
        axisValues.insert(axis, reader.AxisValues(axis));
    }

    // Reuse an existing range for this table — only if still coherent with the current
    // table (graded axis + frozen values of every OTHER axis must still exist).
    const auto *existing = pattern->SizeRanges();
    QString staleRangeToDrop; // dropped only on the accept path (never on Cancel)
    for (auto it = existing->cbegin(); it != existing->cend(); ++it)
    {
        if (it->measurement != canonical)
        {
            continue;
        }

        const QStringList gradedValues = axisValues.value(it->sizeAxis);
        const int lo = static_cast<int>(gradedValues.indexOf(it->minValue));
        const int hi = static_cast<int>(gradedValues.indexOf(it->maxValue));

        bool frozenValuesOk = true;
        for (auto av = it->axisValues.cbegin(); av != it->axisValues.cend(); ++av)
        {
            if (!axes.contains(av.key()) || !axisValues.value(av.key()).contains(av.value()))
            {
                frozenValuesOk = false;
                break;
            }
        }

        if (axes.contains(it->sizeAxis) && lo >= 0 && hi >= 0 && lo <= hi && frozenValuesOk)
        {
            m_activeGradedRangeName = it.key();
            return true;
        }

        // Stale range (table changed): defer the removal until the user accepts a new one —
        // a Cancel must NOT lose the stored range (undo-free destructive edit otherwise).
        staleRangeToDrop = it.key();
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("The stored size range no longer matches the loaded "
                                "measurements table. It was removed — please, create a new one."),
                             QMessageBox::Ok);
        break;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const QString libPath = settings->GetPathMeasurementLibrary();
    const bool inLibrary = MeasurementLibraryScanner::IsInLibrary(libPath, canonical);
    const QString declared = inLibrary ? settings->GetSizeAxisForMeasurement(canonical) : QString();

    OGradedRangeDialog::Config config;
    config.canonicalPath = canonical;
    config.axisNames = axes;
    config.axisValues = axisValues;
    config.axisStepNeeded = declared.isEmpty();
    config.declaredAxis = declared;
    QStringList takenNames = existing->keys();
    takenNames.removeAll(staleRangeToDrop); // the stale range's own name may be reused
    config.takenNames = takenNames;

    OGradedRangeDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    // Persist the axis declaration (software store) when the table lives in the library.
    if (config.axisStepNeeded && inLibrary)
    {
        settings->SetSizeAxisForMeasurement(canonical, dialog.sizeAxis());
    }

    OSizerange range = dialog.range();
    if (not range.IsValid())
    {
        return false;
    }

    QHash<QString, OSizerange> ranges = *pattern->SizeRanges();
    ranges.remove(staleRangeToDrop); // deferred stale drop — only now that a new range is accepted
    if (ranges.contains(range.name))
    {
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("A size range with this name already exists."), QMessageBox::Ok);
        return false;
    }
    ranges.insert(range.name, range);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    m_activeGradedRangeName = range.name;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// .omt dimension base == 1-based index of the CSV label inside the axis values.
auto GradedBaseForLabel(const QStringList &axisValues, const QString &label) -> qreal
{
    const qsizetype idx = axisValues.indexOf(label);
    return idx >= 0 ? static_cast<qreal>(idx + 1) : 1.0;
}

/// Item roles tagging every graded item so alignment / offsets / drag can recover the
/// owning piece id, whether the item is the reference (layer 0) of its piece, and the
/// position base (mx/my) the item was created at — used to make re-applies idempotent.
constexpr int kGradedPieceIdRole      = Qt::UserRole + 900;
constexpr int kGradedIsRefRole        = Qt::UserRole + 901;
constexpr int kGradedBasePosRole      = Qt::UserRole + 902;
constexpr int kGradedHostGroupIdRole  = Qt::UserRole + 903; // invisible group host (holds group children)
constexpr int kGradedGrainlineOwnerRole = Qt::UserRole + 904; // interactive grainline marker (owner id)
constexpr int kGradedInfoBlockRole    = Qt::UserRole + 905; // interactive info-block marker (owner id)
constexpr int kGradedAnnotationRole   = Qt::UserRole + 906; // interactive annotation marker (owner id)

/// Piece id carried by an item (or 0 if the item isn't a graded piece item).
auto GradedPieceId(const QGraphicsItem *item) -> quint32
{
    return item != nullptr && !item->data(kGradedPieceIdRole).isNull()
               ? item->data(kGradedPieceIdRole).toUInt()
               : 0;
}

auto GradedIsReference(const QGraphicsItem *item) -> bool
{
    return item != nullptr && item->data(kGradedIsRefRole).toBool();
}

/// Deterministic "silhouette content" fingerprint of a piece for the graded structure
/// signature. Covers the piece-content edits that must refresh the graded canvas (seam
/// allowance/margins + hide/show main path, mounting notches, internal paths, place labels)
/// but DELIBERATELY excludes mx/my (a plain Details move must not relocate the graded canvas)
/// and excludes info-blocks/annotations (those refresh on every apply instead).
auto GradedPieceContentDigest(const VPiece &piece) -> QString
{
    QStringList parts;
    parts.append(QStringLiteral("sa=%1%2%3%4%5")
                     .arg(piece.IsSeamAllowance() ? 1 : 0)
                     .arg(piece.IsSeamAllowanceBuiltIn() ? 1 : 0)
                     .arg(piece.IsHideMainPath() ? 1 : 0)
                     .arg(piece.IsShowFullPiece() ? 1 : 0)
                     .arg(piece.IsTrueZeroWidth() ? 1 : 0));
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

/// Deterministic fingerprint of an annotation list (order + template + text). Used to detect
/// a list change in Details (delete/reorder) so stale positional overrides are not mis-applied.
auto GradedAnnotationsFingerprint(const QList<OAnnotation> &annotations) -> QString
{
    QStringList parts;
    parts.reserve(annotations.size());
    for (const OAnnotation &annotation : annotations)
    {
        parts.append(annotation.templateName + QLatin1Char('#') + annotation.text.trimmed());
    }
    return parts.join(QLatin1Char('|'));
}

/// A grouped member masks its own decor: grainline hidden everywhere, fold line kept only on
/// the member that carries the group's fold (mirror of Details + the SVG export path).
void HideGroupedReferenceMemberDecor(QGraphicsItem *memberItem, quint32 pieceId, const OPieceGroup &group)
{
    const bool foldCarrier = (group.m_foldLineMemberId == pieceId);
    const QList<QGraphicsItem *> children = memberItem->childItems();
    for (QGraphicsItem *child : children)
    {
        if (child->type() == VGraphicsFillItem::Type)
        {
            child->hide(); // member's grainline → the group host shows the shared one
        }
        else if (!foldCarrier && (child->type() == VGraphicsFoldLineItem::Type
                                  || child->type() == QGraphicsSimpleTextItem::Type))
        {
            child->hide(); // fold mark/text, unless this member is the group fold carrier
        }
    }
}

/// Rebuild the per-group reference decoration (run once positions are final): mask the grouped
/// members' own grainline/fold and host the GROUP's shared grainline on an invisible root whose
/// anchor is the union of the graded reference members — like the Details group holder, but
/// anchored to the graded items (mirror of the SVG export, mainwindowsnogui.cpp:657-747).
void RefreshGradedReferenceGroupDecor(VContainer *pattern, QGraphicsScene *scene)
{
    if (pattern == nullptr || scene == nullptr)
    {
        return;
    }

    // Drop previous hosts (and, with them, their interactive children) — they are re-created
    // from scratch now that positions are final. Collect the hosts to delete FIRST, then
    // delete, so the code never dereferences an already-freed child inside the snapshot
    // (ordering-independent, robust to future reparenting).
    const auto stale = scene->items();
    QList<QGraphicsItem *> hostsToDelete;
    for (auto *item : stale)
    {
        if (!item->data(kGradedHostGroupIdRole).isNull())
        {
            hostsToDelete.append(item);
        }
    }
    for (auto *host : hostsToDelete)
    {
        scene->removeItem(host);
        delete host;
    }
    const auto items = scene->items();

    // 1) Mask the decor that must NOT appear per piece:
    //    - ghost layers (any piece): no grainline/fold — only the reference size carries decor;
    //    - reference grouped members: own grainline hidden, own fold kept only on the fold
    //      carrier (the group host shows the shared grainline instead, like Details).
    QHash<quint32, QRectF> groupBounds;
    for (auto *item : items)
    {
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0)
        {
            continue;
        }

        if (not GradedIsReference(item))
        {
            // Ghost layer: the sizes under the reference carry no per-piece decor.
            const QList<QGraphicsItem *> children = item->childItems();
            for (QGraphicsItem *child : children)
            {
                if (child->type() == VGraphicsFillItem::Type
                    || child->type() == VGraphicsFoldLineItem::Type
                    || child->type() == QGraphicsSimpleTextItem::Type)
                {
                    child->hide();
                }
            }
            continue;
        }

        // Reference layer.
        const quint32 gid = pattern->GetPieceGroupId(pieceId);
        if (gid == NULL_ID)
        {
            continue; // a lone piece keeps its own grainline/fold on the reference
        }
        const OPieceGroup &group = pattern->DataPieceGroups()->value(gid);
        HideGroupedReferenceMemberDecor(item, pieceId, group);

        const QRectF r = item->sceneBoundingRect();
        QRectF &b = groupBounds[gid];
        b = b.isNull() ? r : b.united(r);
    }
    if (groupBounds.isEmpty())
    {
        return; // no group on the reference → nothing to host
    }

    // Highest layer z, so the host draws above the reference pieces without intercepting clicks.
    qreal topZ = 0.0;
    for (auto *item : items)
    {
        if (GradedPieceId(item) != 0)
        {
            topZ = qMax(topZ, item->zValue());
        }
    }

    // 2) per-group invisible host sized to the union box: it gives the interactive group
    //    grainline (built by RefreshGradedReferenceGrainlines) a valid local frame to clamp
    //    into, without intercepting any mouse event itself.
    for (auto it = groupBounds.cbegin(); it != groupBounds.cend(); ++it)
    {
        const QRectF &box = it.value();
        auto *host = new QGraphicsPathItem();
        host->setPos(box.topLeft());
        QPainterPath hostPath;
        hostPath.addRect(QRectF(QPointF(0.0, 0.0), box.size()));
        host->setPath(hostPath);
        host->setPen(Qt::NoPen);
        host->setBrush(Qt::NoBrush);
        host->setZValue(topZ + 1.0);
        host->setFlag(QGraphicsItem::ItemIsSelectable, false);
        host->setFlag(QGraphicsItem::ItemIsMovable, false);
        host->setAcceptedMouseButtons(Qt::NoButton);
        host->setData(kGradedHostGroupIdRole, it.key());
        scene->addItem(host);
    }
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

    OPieceCartoucheTable reader;
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
            pieces = PrepareDetailsForLayout(details);
        }
        catch (const VException &e)
        {
            // A piece that cannot be prepared for a graded size must not crash the mode entry —
            // skip the remaining sizes; the restore below still runs and the partial scene is shown.
            qWarning("Graded capture failed for a size — %s", qUtf8Printable(e.ErrorMessage()));
            break;
        }

        // One real piece render per (piece × size); z = N − layer → reference (min size) on
        // top. Render the piece the same way Val Fork's graded scene does (piece.GetItem):
        // main path + seam allowance + internal paths + passmarks + place labels + grainline
        // + fold line — NOT an invented silhouette.
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

            QGraphicsItem *item = pieces.at(k).GetItem(false, false, false);
            item->setPos(basePos);
            item->setData(kGradedBasePosRole, basePos); // frozen base for idempotent re-applies
            item->setZValue(z);
            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
            item->setFlag(QGraphicsItem::ItemIsMovable, isReference);
            item->setData(kGradedPieceIdRole, pieceId);
            item->setData(kGradedIsRefRole, isReference);
            m_sceneGraded->addItem(item);
        }
    }

    // Restore the live combination without leaving the DOM dirty.
    pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});
    m_m->StoreNames(false);
    m_m->ReadMeasurements(origA, origB, origC);
    doc->LiteParseTree(Document::FullLiteParse);
    doc->RefreshPieceGeometry();
    doc->SetModified(wasModified);

    // Alignment + per-range content offsets (recomputed from the model).
    ApplyGradedLayoutOffsets();

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
                        // Defer out of the current event; route through BuildGradedScene so the
                        // structure/content signature decides: unchanged → apply-only offsets,
                        // changed (e.g. an undone piece-content edit) → full re-capture.
                        QTimer::singleShot(0, this, [this]() { BuildGradedScene(); });
                    }
                });
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

    m_gradedSceneSignature = signature;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ApplyGradedLayoutOffsets()
{
    if (m_sceneGraded == nullptr || pattern == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }

    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // Idempotent re-apply: alignment + stored offsets below are computed from the item's
    // frozen base (mx/my). Reset every item to that base first, so a re-apply (drag end,
    // undo/redo, rebuild) never stacks offsets on top of already-offset positions.
    const auto resetItems = m_sceneGraded->items();
    for (auto *item : resetItems)
    {
        if (GradedPieceId(item) != 0 && !item->data(kGradedBasePosRole).isNull())
        {
            item->setPos(item->data(kGradedBasePosRole).toPointF());
        }
    }

    // Re-align the size layers of a piece in SCENE coordinates, as a delta added to the
    // item's mx/my base (Val Fork BuildGradedDetailsScene) — never re-base to the local
    // origin, which would discard the piece's Details placement. Then add the per-range
    // content offsets. The model is the single source of truth.
    const auto AnchorPoint = [](const QRectF &rect, int h, int v) -> QPointF {
        const qreal hFrac = h == 1 ? 0.0 : (h == 3 ? 1.0 : 0.5);
        const qreal vFrac = v == 1 ? 0.0 : (v == 3 ? 1.0 : 0.5);
        return QPointF(rect.left() + rect.width() * hFrac, rect.top() + rect.height() * vFrac);
    };

    QHash<quint32, QList<QGraphicsItem *>> byPiece;
    const auto allItems = m_sceneGraded->items();
    for (auto *baseItem : allItems)
    {
        if (GradedPieceId(baseItem) != 0)
        {
            byPiece[GradedPieceId(baseItem)].append(baseItem);
        }
    }

    // Group membership: a member piece is never aligned/offset as an individual — the group
    // is the unit (same rule as Details). Ostara groups have no "no-align" sentinel
    // (alignH/V default 2 = center), so every group with ≥ 2 members is aligned as a unit.
    QHash<quint32, quint32> pieceGroupOf; // member piece id -> group id
    const auto *groups = pattern->DataPieceGroups();
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            if (git.value().pieceIds.size() < 2)
            {
                continue;
            }
            for (quint32 memberId : git.value().pieceIds)
            {
                pieceGroupOf.insert(memberId, git.key());
            }
        }
    }

    // Pass 1 — align each NON-grouped piece's size layers (additive delta, mx/my preserved).
    const auto *piecesMap = pattern->DataPieces();
    for (auto it = byPiece.cbegin(); it != byPiece.cend(); ++it)
    {
        if (pieceGroupOf.contains(it.key()))
        {
            continue; // group members are aligned as their group (pass 2)
        }

        const QList<QGraphicsItem *> &items = it.value();
        if (items.size() < 2)
        {
            continue; // a single layer keeps its Details (mx/my) position
        }

        int alignH = 2;
        int alignV = 2;
        if (piecesMap != nullptr && piecesMap->contains(it.key()))
        {
            const VPiece &piece = piecesMap->value(it.key());
            alignH = piece.GetGradedAlignH();
            alignV = piece.GetGradedAlignV();
        }

        // Capture the scene rects BEFORE any move (they include the mx/my base), then shift
        // each layer by the anchor delta only — position additive, never absolute re-base.
        QVector<QRectF> rects;
        rects.reserve(items.size());
        QRectF unionRect;
        for (const auto *item : items)
        {
            const QRectF r = item->sceneBoundingRect();
            rects.append(r);
            unionRect = unionRect.isNull() ? r : unionRect.united(r);
        }

        const QPointF target = AnchorPoint(unionRect, alignH, alignV);
        for (int i = 0; i < items.size(); ++i)
        {
            const QPointF delta = target - AnchorPoint(rects.at(i), alignH, alignV);
            if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
            {
                items.at(i)->setPos(items.at(i)->pos() + delta);
            }
        }
    }

    // Pass 2 — align each GROUP as a rigid unit (mirror of Val Fork BuildGradedDetailsScene's
    // group pass): per layer the union of the members, cross-layer overall union, then shift
    // every member of a layer by the same anchor delta.
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            const OPieceGroup &group = git.value();
            if (group.pieceIds.size() < 2)
            {
                continue;
            }

            QMap<qreal, QList<QGraphicsItem *>> itemsByLayer; // z -> items
            QMap<qreal, QRectF> rectByLayer;                  // z -> pre-move member union
            int totalItems = 0;
            for (quint32 memberId : group.pieceIds)
            {
                const auto found = byPiece.constFind(memberId);
                if (found == byPiece.cend())
                {
                    continue;
                }
                for (auto *item : *found)
                {
                    const qreal z = item->zValue();
                    itemsByLayer[z].append(item);
                    ++totalItems;
                    const QRectF r = item->sceneBoundingRect();
                    QRectF &lr = rectByLayer[z];
                    lr = lr.isNull() ? r : lr.united(r);
                }
            }
            if (totalItems < 2)
            {
                continue;
            }

            QRectF overallRect;
            for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
            {
                overallRect = overallRect.isNull() ? it.value() : overallRect.united(it.value());
            }

            for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
            {
                const QPointF delta = AnchorPoint(overallRect, group.alignH, group.alignV)
                                      - AnchorPoint(it.value(), group.alignH, group.alignV);
                if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y()))
                {
                    continue;
                }
                for (auto *item : itemsByLayer[it.key()])
                {
                    item->setPos(item->pos() + delta);
                }
            }
        }
    }

    // Per-range content offsets (drag deltas persisted in the size range). A group member
    // only ever moves through its group's offset — never through a per-piece one.
    for (auto pc = range.pieceContent.cbegin(); pc != range.pieceContent.cend(); ++pc)
    {
        if (pieceGroupOf.contains(pc.key()))
        {
            continue;
        }
        const auto found = byPiece.constFind(pc.key());
        if (found == byPiece.cend())
        {
            continue;
        }
        const QPointF delta(pc.value().dx, pc.value().dy);
        for (auto *item : *found)
        {
            item->setPos(item->pos() + delta);
        }
    }

    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            const auto content = range.groupContent.constFind(git.key());
            if (content == range.groupContent.cend())
            {
                continue;
            }
            const QPointF delta(content->dx, content->dy);
            for (quint32 memberId : git.value().pieceIds)
            {
                const auto found = byPiece.constFind(memberId);
                if (found == byPiece.cend())
                {
                    continue;
                }
                for (auto *item : *found)
                {
                    item->setPos(item->pos() + delta);
                }
            }
        }
    }

    // Refresh the reference drag baselines.
    m_gradedBasePositions.clear();
    for (auto *baseItem : allItems)
    {
        if (GradedIsReference(baseItem))
        {
            m_gradedBasePositions.insert(GradedPieceId(baseItem), baseItem->pos());
        }
    }

    // Positions are final: mask grouped members' own grainline/fold and (re)anchor the group
    // grainline host (Details rule — a group shows its own children instead of the members').
    RefreshGradedReferenceGroupDecor(pattern, m_sceneGraded);

    // Interactive grainlines, info blocks then annotations on the reference layer — Details-like.
    RefreshGradedReferenceGrainlines();
    RefreshGradedReferenceInfoBlocks();
    RefreshGradedReferenceAnnotations();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedGrainline(quint32 ownerId, bool isGroup, const OSizerange::GrainlineOverride &grainline)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    if (isGroup)
    {
        newRange.groupContent[ownerId].grainline = grainline;
    }
    else
    {
        newRange.pieceContent[ownerId].grainline = grainline;
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceGrainlines()
{
    if (m_sceneGraded == nullptr || pattern == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // 1) Drop previous interactive grainline items (children of reference pieces / group hosts).
    auto items = m_sceneGraded->items();
    for (auto *owner : items)
    {
        if (GradedPieceId(owner) == 0 && owner->data(kGradedHostGroupIdRole).isNull())
        {
            continue;
        }
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (!child->data(kGradedGrainlineOwnerRole).isNull())
            {
                delete child;
            }
        }
    }
    items = m_sceneGraded->items(); // fresh — the deleted children above must not be re-walked

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();
    const auto rotationDeg = [](const QString &formula) -> qreal {
        bool ok = false;
        const qreal value = formula.toDouble(&ok);
        return ok ? value : 0.0;
    };

    // 2) Lone (non-grouped) reference pieces with an enabled grainline → interactive item.
    for (auto *item : items)
    {
        if (!GradedIsReference(item))
        {
            continue;
        }
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0 || piecesMap == nullptr || !piecesMap->contains(pieceId))
        {
            continue;
        }
        if (pattern->GetPieceGroupId(pieceId) != NULL_ID)
        {
            continue; // grouped → the group hosts the (single) grainline
        }

        const VPiece &piece = piecesMap->value(pieceId);
        const VGrainlineData geom = piece.GetGrainlineGeometry();
        if (!geom.IsEnabled() || !geom.IsVisible())
        {
            continue;
        }

        const auto pit = range.pieceContent.constFind(pieceId);
        const bool ov = (pit != range.pieceContent.cend() && pit->grainline.valid);
        QPointF pos = ov ? pit->grainline.pos : geom.GetPos();
        if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
        {
            pos = item->boundingRect().center(); // parent-local default
        }
        const qreal rot = rotationDeg(geom.GetRotation());
        const qreal baseLenPx =
            ToPixel((geom.GetLength().isEmpty() ? 10.0 : geom.GetLength().toDouble()),
                    *pattern->GetPatternUnit());
        const qreal lenPx = (ov && pit->grainline.lengthPx > 0.0) ? pit->grainline.lengthPx : baseLenPx;

        // Hide the static grainline GetItem drew — the interactive item replaces it.
        const auto children = item->childItems();
        for (auto *child : children)
        {
            if (child->type() == VGraphicsFillItem::Type)
            {
                child->hide();
            }
        }

        auto *gl = new VGrainlineItem(VColorRole::PieceColor, item);
        gl->setData(kGradedGrainlineOwnerRole, pieceId);
        gl->setFlag(QGraphicsItem::ItemIsMovable, true);
        gl->SetMoveType(VPieceItem::MoveTypes(VPieceItem::IsMovable | VPieceItem::IsResizable));
        gl->UpdateGeometry(pos, rot, lenPx, geom.GetArrowType());

        connect(gl, &VGrainlineItem::SignalMoved, this, [this, pieceId](const QPointF &p) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR; // carry over any existing length
            newR.pieceContent[pieceId].grainline.valid = true;
            newR.pieceContent[pieceId].grainline.pos = p;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
        connect(gl, &VGrainlineItem::SignalResized, this, [this, gl, pieceId](qreal lengthPx) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR; // carry over any existing position
            newR.pieceContent[pieceId].grainline.valid = true;
            newR.pieceContent[pieceId].grainline.pos = gl->pos();
            newR.pieceContent[pieceId].grainline.lengthPx = lengthPx;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
    }

    // 3) Groups: interactive grainline on the invisible host (its bounding rect = union box).
    for (auto *item : items)
    {
        if (item->data(kGradedHostGroupIdRole).isNull() || groups == nullptr)
        {
            continue;
        }
        const quint32 groupId = item->data(kGradedHostGroupIdRole).toUInt();
        if (!groups->contains(groupId))
        {
            continue;
        }
        const OPieceGroup &group = groups->value(groupId);
        const VGrainlineData geom = group.m_glGrainline;
        if (!geom.IsEnabled() || !geom.IsVisible())
        {
            continue;
        }

        const auto git = range.groupContent.constFind(groupId);
        const bool ov = (git != range.groupContent.cend() && git->grainline.valid);
        QPointF pos = ov ? git->grainline.pos : geom.GetPos();
        if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
        {
            pos = item->boundingRect().center(); // host-local default (union centre)
        }
        const qreal rot = rotationDeg(geom.GetRotation());
        const qreal baseLenPx =
            ToPixel((geom.GetLength().isEmpty() ? 10.0 : geom.GetLength().toDouble()),
                    *pattern->GetPatternUnit());
        const qreal lenPx = (ov && git->grainline.lengthPx > 0.0) ? git->grainline.lengthPx : baseLenPx;

        auto *gl = new VGrainlineItem(VColorRole::PieceColor, item);
        gl->setData(kGradedGrainlineOwnerRole, groupId);
        gl->setFlag(QGraphicsItem::ItemIsMovable, true);
        gl->SetMoveType(VPieceItem::MoveTypes(VPieceItem::IsMovable | VPieceItem::IsResizable));
        gl->UpdateGeometry(pos, rot, lenPx, geom.GetArrowType());

        connect(gl, &VGrainlineItem::SignalMoved, this, [this, groupId](const QPointF &p) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR;
            newR.groupContent[groupId].grainline.valid = true;
            newR.groupContent[groupId].grainline.pos = p;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
        connect(gl, &VGrainlineItem::SignalResized, this, [this, gl, groupId](qreal lengthPx) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR;
            newR.groupContent[groupId].grainline.valid = true;
            newR.groupContent[groupId].grainline.pos = gl->pos();
            newR.groupContent[groupId].grainline.lengthPx = lengthPx;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedInfoBlock(quint32 ownerId, bool isGroup, int block, const OInfoBlockGeom &geom)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    OSizerange::BlockOverride override;
    override.valid = true;
    override.geom = geom;
    if (isGroup)
    {
        newRange.groupContent[ownerId].blocks.insert(block, override);
    }
    else
    {
        newRange.pieceContent[ownerId].blocks.insert(block, override);
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceInfoBlocks()
{
    if (m_sceneGraded == nullptr || pattern == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // Drop previous interactive block items, then re-fetch (the old list must not be reused).
    auto items = m_sceneGraded->items();
    for (auto *owner : items)
    {
        if (GradedPieceId(owner) == 0 && owner->data(kGradedHostGroupIdRole).isNull())
        {
            continue;
        }
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (!child->data(kGradedInfoBlockRole).isNull())
            {
                delete child;
            }
        }
    }
    items = m_sceneGraded->items();

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();

    // Shared builder: instantiate one OInfoBlockItem per active block of an owner, from the
    // Details base geometry or the per-range override, and wire move/resize to the range.
    constexpr qreal kBlockStackingGap = 12.0;
    auto BuildOwnerBlocks = [this, &range](QGraphicsItem *owner, quint32 ownerId, bool isGroup,
                                           const OInfoBlockData &baseData,
                                           const OInfoBlockEffectiveState &state) {
        const QPointF center = owner->boundingRect().center();
        const QList<int> blockNumbers = state.blockContent.keys();
        for (int block : blockNumbers)
        {
            const auto lines = state.blockContent.value(block);
            if (lines.isEmpty())
            {
                continue;
            }

            OSizerange::BlockOverride override;
            if (isGroup)
            {
                const auto it = range.groupContent.constFind(ownerId);
                if (it != range.groupContent.cend())
                {
                    override = it->blocks.value(block);
                }
            }
            else
            {
                const auto it = range.pieceContent.constFind(ownerId);
                if (it != range.pieceContent.cend())
                {
                    override = it->blocks.value(block);
                }
            }

            OInfoBlockGeom geom = baseData.BlockGeom(block);
            if (override.valid)
            {
                geom.pos = override.geom.pos;
                if (override.geom.fontScale > 0.0)
                {
                    geom.fontScale = override.geom.fontScale;
                }
            }
            const QSizeF natural = OCartouchePainter::NaturalSize(lines);
            const qreal scale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
            geom.width = scale * natural.width();
            geom.height = scale * natural.height();
            if (!override.valid && !baseData.HasStoredGeometry(block))
            {
                geom.pos = center + QPointF(0.0, (block - 1) * (geom.height + kBlockStackingGap));
            }

            auto *blk = new OInfoBlockItem(block, owner);
            blk->setData(kGradedInfoBlockRole, ownerId);
            blk->SetBlockGeom(geom);
            blk->SetContent(lines, geom.fontScale);
            blk->show();

            connect(blk, &OInfoBlockItem::Moved, this,
                    [this, ownerId, isGroup, block, baseData](int, const QPointF &p) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        qreal fontScale = baseData.BlockGeom(block).fontScale;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::BlockOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).blocks.value(block)
                                    : cur.pieceContent.value(ownerId).blocks.value(block);
                        if (ov.valid && ov.geom.fontScale > 0.0)
                        {
                            fontScale = ov.geom.fontScale;
                        }
                        if (fontScale <= 0.0)
                        {
                            fontScale = 1.0;
                        }
                        OInfoBlockGeom g;
                        g.pos = p;
                        g.fontScale = fontScale;
                        StoreGradedInfoBlock(ownerId, isGroup, block, g);
                    });
            connect(blk, &OInfoBlockItem::Resized, this,
                    [this, blk, ownerId, isGroup, block, baseData](int, qreal newW, qreal newH, qreal origW,
                                                                  qreal) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        qreal fontScale = baseData.BlockGeom(block).fontScale;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::BlockOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).blocks.value(block)
                                    : cur.pieceContent.value(ownerId).blocks.value(block);
                        if (ov.valid && ov.geom.fontScale > 0.0)
                        {
                            fontScale = ov.geom.fontScale;
                        }
                        if (fontScale <= 0.0)
                        {
                            fontScale = 1.0;
                        }
                        if (origW > 0.0)
                        {
                            fontScale *= newW / origW;
                        }
                        OInfoBlockGeom g;
                        g.pos = blk->pos();
                        g.width = newW;
                        g.height = newH;
                        g.fontScale = fontScale;
                        StoreGradedInfoBlock(ownerId, isGroup, block, g);
                    });
        }
    };

    // Lone (non-grouped) reference pieces.
    for (auto *item : items)
    {
        if (!GradedIsReference(item))
        {
            continue;
        }
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0 || piecesMap == nullptr || !piecesMap->contains(pieceId))
        {
            continue;
        }
        if (pattern->GetPieceGroupId(pieceId) != NULL_ID)
        {
            continue; // grouped → the group hosts the (single) set of blocks
        }
        const VPiece &piece = piecesMap->value(pieceId);
        const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(piece, doc, pattern);
        if (!state.active)
        {
            continue;
        }
        BuildOwnerBlocks(item, pieceId, false, piece.GetInfoBlockData(), state);
    }

    // Groups.
    for (auto *item : items)
    {
        if (item->data(kGradedHostGroupIdRole).isNull() || groups == nullptr)
        {
            continue;
        }
        const quint32 groupId = item->data(kGradedHostGroupIdRole).toUInt();
        if (!groups->contains(groupId))
        {
            continue;
        }
        const OPieceGroup &group = groups->value(groupId);
        const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(group, doc, pattern);
        if (!state.active)
        {
            continue;
        }
        BuildOwnerBlocks(item, groupId, true, group.GetInfoBlockData(), state);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedAnnotation(quint32 ownerId, bool isGroup, int index, const OAnnotationGeom &geom)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    OSizerange::AnnotationOverride override;
    override.valid = true;
    override.geom = geom;

    // Record which annotation list these positional overrides were made against, so a later
    // Details delete/reorder can be detected and the stale overrides dropped.
    QList<OAnnotation> annotationList;
    if (isGroup)
    {
        const auto *groups = pattern->DataPieceGroups();
        if (groups != nullptr && groups->contains(ownerId))
        {
            annotationList = groups->value(ownerId).GetAnnotationData().Annotations();
        }
    }
    else
    {
        const auto *pieces = pattern->DataPieces();
        if (pieces != nullptr && pieces->contains(ownerId))
        {
            annotationList = pieces->value(ownerId).GetAnnotationData().Annotations();
        }
    }
    const QString fp = GradedAnnotationsFingerprint(annotationList);

    if (isGroup)
    {
        newRange.groupContent[ownerId].annotations.insert(index, override);
        newRange.groupContent[ownerId].annotationsFp = fp;
    }
    else
    {
        newRange.pieceContent[ownerId].annotations.insert(index, override);
        newRange.pieceContent[ownerId].annotationsFp = fp;
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceAnnotations()
{
    if (m_sceneGraded == nullptr || pattern == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // Drop previous interactive annotation items, then re-fetch the scene list.
    auto items = m_sceneGraded->items();
    for (auto *owner : items)
    {
        if (GradedPieceId(owner) == 0 && owner->data(kGradedHostGroupIdRole).isNull())
        {
            continue;
        }
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (!child->data(kGradedAnnotationRole).isNull())
            {
                delete child;
            }
        }
    }
    items = m_sceneGraded->items();

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();
    const OAnnotationStyleResolver &styleResolver = VToolSeamAllowance::AnnotationStyleResolver();

    // Rotation: fixed from the base annotation — evaluated like Details (piece formula via the
    // calculator over the pattern variables; group = literal degrees). Never stored/modified.
    auto RotationDeg = [this](const QString &formula, bool isGroup) -> qreal {
        if (isGroup)
        {
            bool ok = false;
            const qreal value = formula.toDouble(&ok);
            return ok ? value : 0.0;
        }
        if (formula.isEmpty())
        {
            return 0.0;
        }
        try
        {
            Calculator cal;
            const qreal value = cal.EvalFormula(pattern->DataVariables(), formula);
            return (qIsNaN(value) || qIsInf(value)) ? 0.0 : value;
        }
        catch (...)
        {
            return 0.0; // malformed rotation formula → fall back like Details (invalid → 0°)
        }
    };

    auto BuildOwnerAnnotations = [this, &range, &styleResolver, &RotationDeg](
                                     QGraphicsItem *owner, quint32 ownerId, bool isGroup,
                                     const QList<OAnnotation> &annotations) {
        const QPointF center = owner->boundingRect().center();
        const QString storedFp = isGroup ? range.groupContent.value(ownerId).annotationsFp
                                         : range.pieceContent.value(ownerId).annotationsFp;
        // If the annotation list changed in Details since the overrides were stored, drop them
        // (reset to Details positions) instead of mis-applying them by stale index.
        const bool overridesStale =
            !storedFp.isEmpty() && storedFp != GradedAnnotationsFingerprint(annotations);

        for (int i = 0; i < annotations.size(); ++i)
        {
            const OAnnotation &annotation = annotations.at(i);
            if (annotation.text.trimmed().isEmpty())
            {
                continue; // Details hides empty-text annotations
            }

            OSizerange::AnnotationOverride override;
            if (!overridesStale)
            {
                if (isGroup)
                {
                    const auto it = range.groupContent.constFind(ownerId);
                    if (it != range.groupContent.cend())
                    {
                        override = it->annotations.value(i);
                    }
                }
                else
                {
                    const auto it = range.pieceContent.constFind(ownerId);
                    if (it != range.pieceContent.cend())
                    {
                        override = it->annotations.value(i);
                    }
                }
            }

            OAnnotationGeom geom = annotation.geom;
            if (override.valid)
            {
                geom = override.geom;
            }
            if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
            {
                geom.pos = center; // "never moved" sentinel → owner centre (Details rule)
            }

            OAnnotationRenderStyle style = styleResolver ? styleResolver(annotation.templateName)
                                                         : OAnnotationRenderStyle();
            const qreal rotationDeg = RotationDeg(annotation.rotationFormula, isGroup);

            auto *item = new OAnnotationItem(i, owner);
            item->setData(kGradedAnnotationRole, ownerId);
            item->SetIndex(i);
            item->SetContent(annotation.text, style);
            item->SetAnnotationGeom(geom, rotationDeg);
            item->show();

            connect(item, &OAnnotationItem::Moved, this,
                    [this, ownerId, isGroup, i, annotations](int, const QPointF &p) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        const OAnnotationGeom baseGeom = annotations.value(i).geom;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::AnnotationOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).annotations.value(i)
                                    : cur.pieceContent.value(ownerId).annotations.value(i);
                        OAnnotationGeom g = ov.valid ? ov.geom : baseGeom;
                        g.pos = p;
                        StoreGradedAnnotation(ownerId, isGroup, i, g);
                    });
            connect(item, &OAnnotationItem::Resized, this,
                    [this, item, ownerId, isGroup, i, annotations](int, qreal newW, qreal newH, qreal origW,
                                                                  qreal origH) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        const OAnnotationGeom baseGeom = annotations.value(i).geom;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::AnnotationOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).annotations.value(i)
                                    : cur.pieceContent.value(ownerId).annotations.value(i);
                        OAnnotationGeom g = ov.valid ? ov.geom : baseGeom;
                        g.pos = item->pos();
                        g.boxWidth = newW;
                        g.boxHeight = newH;
                        // fontScale only on a proportional (corner) scale — Details rule.
                        const qreal fontScale = (ov.valid && ov.geom.fontScale > 0.0) ? ov.geom.fontScale
                                                                                      : baseGeom.fontScale;
                        g.fontScale = fontScale > 0.0 ? fontScale : 1.0;
                        if (origW > 0.0 && qFuzzyCompare(newW / origW, newH / origH))
                        {
                            g.fontScale *= newW / origW; // proportional (corner) scale only
                        }
                        StoreGradedAnnotation(ownerId, isGroup, i, g);
                    });
        }
    };

    // Lone (non-grouped) reference pieces.
    for (auto *item : items)
    {
        if (!GradedIsReference(item))
        {
            continue;
        }
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0 || piecesMap == nullptr || !piecesMap->contains(pieceId))
        {
            continue;
        }
        if (pattern->GetPieceGroupId(pieceId) != NULL_ID)
        {
            continue; // grouped → the group hosts the (single) set of annotations
        }
        const VPiece &piece = piecesMap->value(pieceId);
        BuildOwnerAnnotations(item, pieceId, false, piece.GetAnnotationData().Annotations());
    }

    // Groups.
    for (auto *item : items)
    {
        if (item->data(kGradedHostGroupIdRole).isNull() || groups == nullptr)
        {
            continue;
        }
        const quint32 groupId = item->data(kGradedHostGroupIdRole).toUInt();
        if (!groups->contains(groupId))
        {
            continue;
        }
        const OPieceGroup &group = groups->value(groupId);
        BuildOwnerAnnotations(item, groupId, true, group.GetAnnotationData().Annotations());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::GradedMouseReleased()
{
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Graded || doc == nullptr
        || m_activeGradedRangeName.isEmpty() || m_sceneGraded == nullptr)
    {
        return;
    }

    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }

    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    bool changed = false;

    const auto items = m_sceneGraded->items();
    const auto *groups = pattern->DataPieceGroups();
    const auto GroupOfPiece = [groups](quint32 pieceId) -> quint32 {
        if (groups == nullptr)
        {
            return 0;
        }
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            if (git.value().pieceIds.contains(pieceId))
            {
                return git.key();
            }
        }
        return 0;
    };

    for (auto *baseItem : items)
    {
        if (!GradedIsReference(baseItem))
        {
            continue;
        }

        const quint32 pieceId = GradedPieceId(baseItem);
        const auto baseIt = m_gradedBasePositions.constFind(pieceId);
        if (baseIt == m_gradedBasePositions.cend())
        {
            continue;
        }

        const QPointF delta = baseItem->pos() - baseIt.value();
        if (qAbs(delta.x()) < 0.5 && qAbs(delta.y()) < 0.5)
        {
            continue;
        }

        // The dragged piece belongs to a group → move the whole group as one rigid unit:
        // fan the same delta to every member × every layer, persisted as a group offset
        // (a member never drags alone, like in Details).
        if (const quint32 groupId = GroupOfPiece(pieceId); groupId != 0)
        {
            const QList<quint32> memberIds = groups->value(groupId).pieceIds;
            OSizerange::GroupContent &content = newRange.groupContent[groupId];
            content.dx += delta.x();
            content.dy += delta.y();

            for (auto *otherItem : items)
            {
                if (otherItem != baseItem)
                {
                    const quint32 otherId = GradedPieceId(otherItem);
                    if (otherId != 0 && memberIds.contains(otherId))
                    {
                        otherItem->setPos(otherItem->pos() + delta);
                    }
                }
            }
            // Refresh the drag base of every reference member of the group.
            for (auto *refItem : items)
            {
                if (GradedIsReference(refItem))
                {
                    const quint32 refId = GradedPieceId(refItem);
                    if (refId != 0 && memberIds.contains(refId))
                    {
                        m_gradedBasePositions.insert(refId, refItem->pos());
                    }
                }
            }
            changed = true;
            continue;
        }

        // Lone piece: existing per-piece path.
        OSizerange::PieceContent &content = newRange.pieceContent[pieceId];
        content.dx += delta.x();
        content.dy += delta.y();
        changed = true;
        m_gradedBasePositions.insert(pieceId, baseItem->pos());

        // Fan the offset out to every other layer (size) of the same piece.
        for (auto *otherItem : items)
        {
            if (otherItem != baseItem && GradedPieceId(otherItem) == pieceId)
            {
                otherItem->setPos(otherItem->pos() + delta);
            }
        }
    }

    if (changed)
    {
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
    }
}
