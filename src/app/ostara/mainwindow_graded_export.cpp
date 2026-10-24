/************************************************************************
 **
 **  @file   mainwindow_graded_export.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded SVG export of a size range (Size Ranges dock, file-svg row
 **          button). Headless per-size recapture into a private scene (the live
 **          graded canvas is never touched), reusable alignment/offsets
 **          (OGraded::ApplyOffsets), then the multi-layer writer (OGradedSvg).
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

#include <QSet>

#include "core/ocornertrace.h"
#include "core/ogroupsuperpose.h"
#include "core/ogradedsizeranges.h"
#include "core/ogratedsizestyles.h"
#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "mainwindow_graded_helpers.h"
#include "ogradedsvgdecor.h"
#include "ogradedgeometry.h"
#include "ogradedsizetrace.h"
#include "ogradedsvgwriter.h"
#include "osvgutils.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vexception/vexception.h"
#include "../vmisc/def.h"
#include "../vmisc/typedef.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QDir>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QHash>
#include <QMap>
#include <QMarginsF>
#include <QMessageBox>
#include <QPointF>
#include <QScopeGuard>
#include <QSignalBlocker>
#include <QVector>
#include <QtGlobal>
#include <QtNumeric>

#include <algorithm>

namespace
{
/// .omt dimension base == 1-based index of the CSV label inside the axis values.
auto GradedBaseForLabel(const QStringList &axisValues, const QString &label) -> qreal
{
    const qsizetype idx = axisValues.indexOf(label);
    return idx >= 0 ? static_cast<qreal>(idx + 1) : 1.0;
}

}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportGradedRange(const QString &rangeName)
{
    ExportGradedRangeTo(rangeName, QString(), true, nullptr, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
QString MainWindow::GradedPatternNameToken() const
{
    if (doc != nullptr)
    {
        const QString name = doc->GetPatternName().trimmed();
        if (!name.isEmpty())
        {
            return name;
        }
    }
    const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
    return patternPath.isEmpty() ? QString() : QFileInfo(patternPath).completeBaseName();
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::ExportGradedRangeTo(const QString &rangeName, const QString &forcedDir,
                                     const bool showDialogs, QString *writtenFilePath,
                                     QString *errorOut)
{
    if (doc == nullptr || pattern == nullptr || m_m.isNull())
    {
        return false;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(rangeName))
    {
        return false;
    }
    const OSizerange &range = ranges->value(rangeName);

    QStringList axes;
    QMap<QString, QStringList> axisValues;
    if (!LoadGradedTableAxes(&axes, &axisValues))
    {
        if (showDialogs)
        {
            QMessageBox::information(this, tr("Graded mode"),
                                     tr("This size range needs its measurements table loaded."),
                                     QMessageBox::Ok);
        }
        return false;
    }

    const QStringList gradedValues = axisValues.value(range.sizeAxis);
    const int minIdx = static_cast<int>(gradedValues.indexOf(range.minValue));
    const int maxIdx = static_cast<int>(gradedValues.indexOf(range.maxValue));
    if (minIdx < 0 || maxIdx < 0 || minIdx > maxIdx)
    {
        return false;
    }

    // --- Destination (range settings, else software defaults). -----------------
    auto *settings = VAbstractApplication::VApp()->Settings();

    // Per-size styles of the Size Styles dock, read on the very key the canvas reads them
    // on — same resolution, same library guard. What is exported is what is shown: a table
    // outside the library is styled nothing on screen, and nothing here either. The
    // "Show in canvas" gate stays unread: it is a canvas preference, not a decision about
    // the file.
    QHash<QString, OGradedSizeStyle> styles;
    const OGradedSizeStyles::StyleAxis axis =
        OGradedSizeStyles::ResolveStyleAxis(doc, pattern, range.name);
    if (axis.ready && axis.inLibrary)
    {
        styles = OGradedSizeStyles::StylesForAxis(settings, axis.canonical, axis.axisName);
    }

    QString outDir = !forcedDir.isEmpty() ? forcedDir : range.exportPath;
    if (outDir.isEmpty())
    {
        outDir = settings->GetPathLayoutExport();
    }
    QDir dir(outDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        if (errorOut != nullptr)
        {
            *errorOut = tr("Cannot create the export folder.");
        }
        if (showDialogs)
        {
            QMessageBox::warning(this, tr("Graded mode"), tr("Cannot create the export folder."),
                                 QMessageBox::Ok);
        }
        return false;
    }

    QString baseName = range.exportFileName;
    if (baseName.isEmpty())
    {
        QStringList parts;
        // Pattern-name field first, pattern FILE base name as fallback — same
        // token the dialog pre-fills, so the preview matches the written file.
        const QString nameToken = GradedPatternNameToken();
        if (!nameToken.isEmpty())
        {
            parts.append(nameToken);
        }
        parts.append(range.minValue);
        parts.append(range.maxValue);
        for (const QString &axis : range.axisValues.keys())
        {
            parts.append(range.axisValues.value(axis));
        }
        baseName = parts.join(u'_');
    }
    if (QFileInfo(baseName).suffix().compare(QStringLiteral("svg"), Qt::CaseInsensitive) != 0)
    {
        baseName += QStringLiteral(".svg");
    }
    const QString filePath = dir.filePath(baseName);

    // Ask before overwriting an existing export (the recompute below is expensive).
    if (QFileInfo::exists(filePath))
    {
        const auto answer = QMessageBox::question(
            this, tr("Graded mode"),
            tr("The file \"%1\" already exists in the export folder.\n"
               "Do you want to replace it?")
                .arg(baseName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return false; // user declined to replace this export — not an error
        }
    }

    const qreal xScale = range.exportXScale > 0.0 ? range.exportXScale : 1.0;
    const qreal yScale = range.exportYScale > 0.0 ? range.exportYScale : 1.0;
    const QString layoutLayerName = range.exportLayoutLayerName.isEmpty()
                                        ? tr("Layout",
                                              "Decor layout layer default name in a graded SVG export")
                                        : range.exportLayoutLayerName;

    // The miniatures the decor layer carries are drawn at the PATTERN's reference size — the very
    // drawing Details and the live canvas show — while the walk below leaves the document on the
    // last size it takes. Freeze them here, before anything is disturbed, and NOT when the layer is
    // written: asked later, the live fallback would hand this export a size of its own range and the
    // layer would show that one instead. A no-op when no piece carries a miniature, and the same
    // call the Details export makes — so both documents are guaranteed the same drawings.
    RefreshThumbnailSnapshots(/*freeze*/ true);

    // --- Headless per-size capture into a private scene (canvas untouched). ----
    QGraphicsScene scene;
    const QSignalBlocker signalBlocker(doc);
    const qreal origA = m_currentDimensionA;
    const qreal origB = m_currentDimensionB;
    const qreal origC = m_currentDimensionC;
    const bool wasModified = doc->IsModified();

    QHash<quint32, QList<QGraphicsItem *>> itemsByPiece;
    QStringList ascLabels;
    // Reuse the canvas frozen bases when exporting the ACTIVE range so the SVG is
    // pixel-identical to what is displayed; otherwise freeze at the reference size.
    QHash<quint32, QPointF> frozenBases;
    if (rangeName == m_activeGradedRangeName)
    {
        frozenBases = m_gradedFrozenBases;
    }

    QString error;
    bool ok = true;

    const auto restoreState = [&]() {
        pattern->ClearVariables({VarType::Measurement, VarType::MeasurementSeparator});
        m_m->StoreNames(false);
        m_m->ReadMeasurements(origA, origB, origC);
        doc->LiteParseTree(Document::FullLiteParse);
        doc->RefreshPieceGeometry();
        doc->SetModified(wasModified);
    };
    const QScopeGuard restoreGuard([&restoreState]() { restoreState(); });

    const int gradedAxisPos = static_cast<int>(axes.indexOf(range.sizeAxis));
    if (gradedAxisPos < 0)
    {
        return false;
    }
    const auto fixedBase = [&axes, &axisValues, &range](int slot) -> qreal {
        const QString label = range.axisValues.value(axes.value(slot));
        return GradedBaseForLabel(axisValues.value(axes.value(slot)), label);
    };

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
            baseA = fixedBase(0);
        }
        if (gradedAxisPos == 1)
        {
            baseB = gradedBase;
        }
        else
        {
            baseB = fixedBase(1);
        }
        if (gradedAxisPos == 2)
        {
            baseC = gradedBase;
        }
        else
        {
            baseC = fixedBase(2);
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
            error = e.ErrorMessage();
            ok = false;
            break;
        }

        // The very rule the canvas applies, on the pieces themselves (OGroupSuperpose).
        OGroupSuperpose::Snap(details, pieces, pattern);


        const int layerPos = idx - minIdx;
        const int z = layerCount - layerPos;
        const bool isReference = (layerPos == 0);

        QVector<QGraphicsItem *> itemsOfSize;

        for (int k = 0; k < pieces.size() && k < details.size(); ++k)
        {
            const quint32 pieceId = details.at(k).id;
            // Same rule as the live canvas: a group member stands at its group's stack position, the
            // group's own geometry, never at its workshop position (see BuildGradedScene).
            const QPointF livePos(pieces.at(k).GetMx(), pieces.at(k).GetMy());
            const quint32 groupId = pattern->GetPieceGroupId(pieceId);
            QPointF basePos;
            if (groupId != NULL_ID && pattern->DataPieceGroups() != nullptr)
            {
                basePos = pattern->DataPieceGroups()->value(groupId).MemberStackPosition(pieceId, livePos);
            }
            else
            {
                basePos = frozenBases.value(pieceId, livePos);
                if (!frozenBases.contains(pieceId))
                {
                    frozenBases.insert(pieceId, livePos);
                }
            }

            QGraphicsItem *item = pieces.at(k).GetItem(false, false, false);
            RecolorGradedItemPens(item);
            item->setPos(basePos);
            item->setData(kGradedBasePosRole, basePos);
            item->setData(kGradedSizeRole, gradedValues.at(idx));
            item->setZValue(z);
            item->setData(kGradedPieceIdRole, pieceId);
            item->setData(kGradedIsRefRole, isReference);

            // Forced alignment point: captured while this size's measurements are still loaded,
            // exactly like the live canvas build — otherwise the exported SVG would drift from it.
            const qreal alignPointX = OGraded::UnitAlignPointX(pieceId, details.at(k).piece, pattern);
            if (!qIsNaN(alignPointX))
            {
                item->setData(kGradedAlignPointXRole, alignPointX);
            }
            scene.addItem(item);
            itemsByPiece[pieceId].append(item);
            itemsOfSize.append(item);
        }

        // The very rule the canvas applies: the members of a group leave their dash pattern from the
        // places they have in common — the corners of the trace and every construction point they
        // share — so a size is drawn the same way on screen and here (OCornerTrace).
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


        if (ok)
        {
            ascLabels.append(gradedValues.at(idx));
        }
        else
        {
            break;
        }
    }

    if (!ok || ascLabels.isEmpty())
    {
        if (errorOut != nullptr)
        {
            *errorOut = error.isEmpty() ? tr("The graded export failed.") : error;
        }
        if (showDialogs)
        {
            QMessageBox::warning(this, tr("Graded mode"),
                                 error.isEmpty() ? tr("The graded export failed.") : error,
                                 QMessageBox::Ok);
        }
        return false;
    }

    // Alignment + per-range content offsets (same as the live canvas).
    const QList<QGraphicsItem *> allItems = scene.items();
    OGraded::ApplyOffsets(allItems, range, pattern);

    // Per-size trace styles (color + pt→px dashes) — the very pass the live canvas runs,
    // so a size is painted the same way on screen and here. These items are thrown away
    // with the private scene they were captured in: the pass owns no default rendering,
    // it only overrides what a style covers, and it recolors no text.
    for (auto *item : allItems)
    {
        if (GradedPieceId(item) == 0)
        {
            continue;
        }
        OGradedSizeTrace::ApplyToSubtree(item, styles, OGradedSizeTrace::Scope::FreshItems);
    }

    // Normalize the scene to the origin like the classic Details exporter (its caller
    // translates the ROOT items by the content top-left — children follow their parent).
    // Keeps the raw (potentially large) scene offset out of the block/annotation
    // transforms — a rotated record would otherwise amplify that offset and fly far.
    {
        const QRectF rawBounds = scene.itemsBoundingRect();
        const QPointF delta(-rawBounds.x(), -rawBounds.y());
        if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
        {
            const auto items = scene.items();
            for (QGraphicsItem *item : items)
            {
                if (item->parentItem() != nullptr)
                {
                    continue;  // only top-level roots move — children follow their parent
                }
                item->setPos(item->pos() + delta);
            }
        }
    }

    // --- Assemble the writer inputs (deterministic order by name). ------------
    const auto *piecesMap = pattern->DataPieces();

    struct GroupEntry
    {
        quint32 id;
        QString name;
        QVector<quint32> members;  // ordered, present pieces
    };
    QVector<GroupEntry> groups;
    const auto *allGroups = pattern->DataPieceGroups();
    if (allGroups != nullptr)
    {
        for (auto git = allGroups->cbegin(); git != allGroups->cend(); ++git)
        {
            if (git.value().pieceIds.size() < 2)
            {
                continue;
            }
            GroupEntry entry;
            entry.id = git.key();
            entry.name = git.value().name;
            for (quint32 mid : git.value().pieceIds)
            {
                if (itemsByPiece.contains(mid))
                {
                    entry.members.append(mid);
                }
            }
            if (entry.members.size() >= 2)
            {
                groups.append(entry);
            }
        }
    }
    std::sort(groups.begin(), groups.end(),
              [](const GroupEntry &l, const GroupEntry &r) { return l.name < r.name; });

    const int nLayers = static_cast<int>(ascLabels.size());
    QVector<OGradedSvg::SizeLayer> layersAsc;
    layersAsc.reserve(nLayers);
    for (int si = 0; si < nLayers; ++si)
    {
        OGradedSvg::SizeLayer layer;
        // Layer display name = Size Styles override (layerName) when set, else the axis value.
        layer.label = ascLabels.at(si);
        if (!styles.isEmpty())
        {
            const auto styleIt = styles.constFind(ascLabels.at(si));
            if (styleIt != styles.cend() && !styleIt->layerName.isEmpty())
            {
                layer.label = styleIt->layerName;
            }
        }
        // Deterministic top-level object order: lone pieces sorted by name, then groups.
        for (auto it = itemsByPiece.cbegin(); it != itemsByPiece.cend(); ++it)
        {
            if (pattern->GetPieceGroupId(it.key()) != NULL_ID)
            {
                continue;
            }
            OGradedSvg::GradedObject object;
            object.name = piecesMap->value(it.key()).GetName();
            object.isGroup = false;
            object.loneItem = it.value().value(si);
            layer.objects.append(object);
        }
        // Sort lone objects by name (stable, keep order by construction).
        std::stable_sort(layer.objects.begin(), layer.objects.end(),
                         [](const OGradedSvg::GradedObject &l, const OGradedSvg::GradedObject &r) {
                             return l.name < r.name;
                         });
        for (const GroupEntry &group : groups)
        {
            OGradedSvg::GradedObject object;
            object.name = group.name;
            object.isGroup = true;
            for (quint32 mid : group.members)
            {
                const QList<QGraphicsItem *> &its = itemsByPiece.value(mid);
                if (si < its.size())
                {
                    object.members.append(
                        {piecesMap->value(mid).GetName(), its.at(si)});
                }
            }
            layer.objects.append(object);
        }
        layersAsc.append(layer);
    }

    // Writer consumes grand → petit (smallest/reference becomes topmost).
    QList<OGradedSvg::SizeLayer> layers;
    for (int si = nLayers - 1; si >= 0; --si)
    {
        layers.append(layersAsc.at(si));
    }

    // --- Layout layer decor (reference size) ----------------------------------
    // Reproduces the live Graded canvas: group members' own decor is masked in
    // favour of the group's, and every child is placed at its RANGE position.
    QList<OGradedSvg::LayoutObject> layoutObjects;
    {
        QVector<quint32> loneIds;
        for (auto it = itemsByPiece.cbegin(); it != itemsByPiece.cend(); ++it)
        {
            if (pattern->GetPieceGroupId(it.key()) == NULL_ID)
            {
                loneIds.append(it.key());
            }
        }
        std::sort(loneIds.begin(), loneIds.end(), [piecesMap](quint32 a, quint32 b) {
            return piecesMap->value(a).GetName() < piecesMap->value(b).GetName();
        });
        QVector<quint32> groupIdOrder;
        groupIdOrder.reserve(static_cast<int>(groups.size()));
        for (const GroupEntry &group : groups)
        {
            groupIdOrder.append(group.id);
        }
        layoutObjects =
            OGradedDecor::BuildReferenceDecor(&scene, range, pattern, doc, loneIds, groupIdOrder);
    }

    const QRectF imageRect = scene.itemsBoundingRect();
    constexpr qreal oneCmPx = 96.0 / 25.4;
    const QMarginsF margins(oneCmPx, oneCmPx, oneCmPx, oneCmPx);

    const bool written = OGradedSvg::WriteGradedSvg(filePath, layers, layoutLayerName,
                                                    layoutObjects, imageRect, margins, xScale,
                                                    yScale, QString(), QString());
    if (!written)
    {
        if (errorOut != nullptr)
        {
            *errorOut = tr("The SVG file could not be written.");
        }
        if (showDialogs)
        {
            QMessageBox::warning(this, tr("Graded mode"), tr("The SVG file could not be written."),
                                 QMessageBox::Ok);
        }
        return false;
    }

    if (writtenFilePath != nullptr)
    {
        *writtenFilePath = filePath;
    }
    if (showDialogs)
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("The file \"%1\" was successfully exported to:\n%2")
                                     .arg(baseName, QFileInfo(filePath).absolutePath()),
                                 QMessageBox::Ok);
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportAllGradedRanges(const QString &dirPath)
{
    if (doc == nullptr || pattern == nullptr || m_m.isNull())
    {
        return;
    }
    auto *settings = VAbstractApplication::VApp()->Settings();

    // Destination: the folder picked in the dock (fallback: software default).
    QString outDir = dirPath.trimmed();
    if (outDir.isEmpty())
    {
        outDir = settings->GetPathLayoutExport();
    }
    QDir dir(outDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
    {
        QMessageBox::warning(this, tr("Graded mode"), tr("Cannot create the export folder."),
                             QMessageBox::Ok);
        return;
    }

    // The dock's own filter: what it does not list, this does not export. A range built
    // against another table — or against another axis of the same table — is not what the
    // user is working on, and its styles live under a key no screen reads.
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
    QStringList names;
    if (const auto *ranges = pattern->SizeRanges(); ranges != nullptr)
    {
        for (auto it = ranges->cbegin(); it != ranges->cend(); ++it)
        {
            if (OGradedSizeRanges::Matches(it.value(), context))
            {
                names.append(it.key());
            }
        }
    }
    names.sort(); // same alphabetical order as the dock rows — one layered SVG per range

    if (names.isEmpty())
    {
        QMessageBox::information(this, tr("Graded mode"), tr("No size range to export."),
                                 QMessageBox::Ok);
        return;
    }

    QStringList writtenFiles; // file names actually exported
    QStringList failedRanges; // ranges that errored
    int skipped = 0;          // overwrite declined by the user

    for (const QString &rangeName : names)
    {
        QString writtenPath;
        QString error;
        // Batch mode: no per-range dialog — the overwrite-ask still shows per file
        // (inside ExportGradedRangeTo), results are reported once in the recap.
        if (ExportGradedRangeTo(rangeName, outDir, false, &writtenPath, &error))
        {
            writtenFiles.append(QFileInfo(writtenPath).fileName());
        }
        else if (error.isEmpty())
        {
            ++skipped; // user answered "No" to the replace question
        }
        else
        {
            failedRanges.append(rangeName);
        }
    }

    // --- Recap (single dialog for the whole batch) ---------------------------
    QStringList lines;
    if (!writtenFiles.isEmpty())
    {
        lines << tr("The following files were exported to:") << dir.absolutePath();
        for (const QString &file : writtenFiles)
        {
            lines << QStringLiteral("• ") + file;
        }
    }
    if (skipped > 0)
    {
        lines << tr("%1 file(s) were not exported (replace declined).").arg(skipped);
    }
    if (!failedRanges.isEmpty())
    {
        lines << tr("The following size ranges could not be exported: %1.")
                     .arg(failedRanges.join(QStringLiteral(", ")));
    }
    if (lines.isEmpty())
    {
        return;
    }

    const QString text = lines.join(QLatin1Char('\n'));
    if (writtenFiles.isEmpty() && !failedRanges.isEmpty())
    {
        QMessageBox::warning(this, tr("Graded mode"), text, QMessageBox::Ok);
    }
    else
    {
        QMessageBox::information(this, tr("Graded mode"), text, QMessageBox::Ok);
    }
}
