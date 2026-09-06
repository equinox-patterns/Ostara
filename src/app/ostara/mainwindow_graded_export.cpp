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

#include "core/ogratedsizestyles.h"
#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "mainwindow_graded_helpers.h"
#include "ogradedsvgdecor.h"
#include "ogradedgeometry.h"
#include "ogradedsvgwriter.h"
#include "osvgutils.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vexception/vexception.h"
#include "../vmisc/def.h"
#include "../vmisc/defglobal.h"
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
#include <QGraphicsPathItem>
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

#include <algorithm>

namespace
{
/// .omt dimension base == 1-based index of the CSV label inside the axis values.
auto GradedBaseForLabel(const QStringList &axisValues, const QString &label) -> qreal
{
    const qsizetype idx = axisValues.indexOf(label);
    return idx >= 0 ? static_cast<qreal>(idx + 1) : 1.0;
}

/// Re-style one trace subtree with a size style (mirror of the canvas
/// StyleGradedSubtree): color + pt→px dashes on the non-decor traces only.
void StyleTraceSubtree(QGraphicsItem *root, const OGradedSizeStyle *style)
{
    if (root == nullptr)
    {
        return;
    }
    if (auto *path = dynamic_cast<QGraphicsPathItem *>(root))
    {
        QPen pen = path->pen();
        const int type = root->type();
        const bool decorTrace = type == VGraphicsFillItem::Type
                                || type == VGraphicsFoldLineItem::Type;
        if (pen.style() != Qt::NoPen && style != nullptr && style->HasColor() && !decorTrace)
        {
            pen.setColor(style->color);
            if (style->dashes.isEmpty())
            {
                pen.setStyle(Qt::SolidLine);
                pen.setCapStyle(Qt::SquareCap);
            }
            else
            {
                const qreal width = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
                const qreal ptToPx = PrintDPI / 72.0;
                QVector<qreal> pattern;
                pattern.reserve(static_cast<int>(style->dashes.size()));
                for (qreal value : style->dashes)
                {
                    pattern.append(value * ptToPx / width);
                }
                pen.setStyle(Qt::CustomDashLine);
                pen.setDashPattern(pattern);
                pen.setCapStyle(Qt::FlatCap);
            }
            path->setPen(pen);
        }
    }
    const auto children = root->childItems();
    for (auto *child : children)
    {
        StyleTraceSubtree(child, style);
    }
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportGradedRange(const QString &rangeName)
{
    if (doc == nullptr || pattern == nullptr || m_m.isNull())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(rangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(rangeName);

    QStringList axes;
    QMap<QString, QStringList> axisValues;
    if (!LoadGradedTableAxes(&axes, &axisValues))
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("This size range needs its measurements table loaded."),
                                 QMessageBox::Ok);
        return;
    }

    const QStringList gradedValues = axisValues.value(range.sizeAxis);
    const int minIdx = static_cast<int>(gradedValues.indexOf(range.minValue));
    const int maxIdx = static_cast<int>(gradedValues.indexOf(range.maxValue));
    if (minIdx < 0 || maxIdx < 0 || minIdx > maxIdx)
    {
        return;
    }

    // --- Destination (range settings, else software defaults). -----------------
    auto *settings = VAbstractApplication::VApp()->Settings();

    // Per-size styles of the Size Styles dock (table canonical path + size axis).
    // Applied regardless of the "Show in canvas" gate (the gate is canvas-only).
    const QString canonical =
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
    QHash<QString, OGradedSizeStyle> styles;
    if (!canonical.isEmpty())
    {
        styles = OGradedSizeStyles::StylesForAxis(settings, canonical, range.sizeAxis);
    }

    QString outDir = range.exportPath;
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

    QString baseName = range.exportFileName;
    if (baseName.isEmpty())
    {
        QStringList parts;
        const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
        if (!patternPath.isEmpty())
        {
            parts.append(QFileInfo(patternPath).completeBaseName());
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

    const qreal xScale = range.exportXScale > 0.0 ? range.exportXScale : 1.0;
    const qreal yScale = range.exportYScale > 0.0 ? range.exportYScale : 1.0;
    const QString layoutLayerName = range.exportLayoutLayerName.isEmpty()
                                        ? tr("Layout")
                                        : range.exportLayoutLayerName;

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
        return;
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
            pieces = PrepareDetailsForLayout(details, false);
        }
        catch (const VException &e)
        {
            error = e.ErrorMessage();
            ok = false;
            break;
        }

        const int layerPos = idx - minIdx;
        const int z = layerCount - layerPos;
        const bool isReference = (layerPos == 0);

        for (int k = 0; k < pieces.size() && k < details.size(); ++k)
        {
            const quint32 pieceId = details.at(k).id;
            const QPointF livePos(pieces.at(k).GetMx(), pieces.at(k).GetMy());
            const QPointF basePos = frozenBases.value(pieceId, livePos);
            if (!frozenBases.contains(pieceId))
            {
                frozenBases.insert(pieceId, livePos);
            }

            QGraphicsItem *item = pieces.at(k).GetItem(false, false, false);
            RecolorGradedItemPens(item);
            item->setPos(basePos);
            item->setData(kGradedBasePosRole, basePos);
            item->setData(kGradedSizeRole, gradedValues.at(idx));
            item->setZValue(z);
            item->setData(kGradedPieceIdRole, pieceId);
            item->setData(kGradedIsRefRole, isReference);
            scene.addItem(item);
            itemsByPiece[pieceId].append(item);
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
        QMessageBox::warning(this, tr("Graded mode"),
                             error.isEmpty() ? tr("The graded export failed.") : error,
                             QMessageBox::Ok);
        return;
    }

    // Alignment + per-range content offsets (same as the live canvas).
    const QList<QGraphicsItem *> allItems = scene.items();
    OGraded::ApplyOffsets(allItems, range, pattern);

    // Per-size trace styles (color + pt→px dashes), mirroring the canvas pass
    // (StyleGradedSubtree): decor traces (grain/fold) are never recolored.
    for (auto *item : allItems)
    {
        if (GradedPieceId(item) == 0)
        {
            continue;
        }
        const OGradedSizeStyle *style = nullptr;
        if (!styles.isEmpty())
        {
            const QString sizeValue = item->data(kGradedSizeRole).toString();
            const auto it = styles.constFind(sizeValue);
            if (it != styles.cend() && it->HasColor())
            {
                style = &it.value();
            }
        }
        StyleTraceSubtree(item, style);
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
        QMessageBox::warning(this, tr("Graded mode"), tr("The SVG file could not be written."),
                             QMessageBox::Ok);
    }
}
