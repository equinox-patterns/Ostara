/************************************************************************
 **
 **  @file   mainwindow_graded_sizestyle.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded mode — Size Styles: the software-level per-size trace styles
 **          (color + px dashes + optional layer name) and their application on
 **          the stacked graded scene. Read-only here: the store and the canvas
 **          pass never touch the pattern (.ostara) — styles live at software
 **          level, keyed by the measurements-table canonical path + size axis.
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
#include "dialogs/ogratedsizestylesdialog.h"
#include "mainwindow_graded_helpers.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetsizestyles.h"
#include "xml/vpattern.h"
#include "../vmisc/def.h"
#include "../vmisc/defglobal.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/osizerange.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vwidgets/vgrainlineitem.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vlayout/vgraphicsfillitem.h"

#include <QBrush>
#include <QColor>
#include <QDialog>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QMap>
#include <QPen>

namespace
{

/// The (table, axis) context a style pass needs — resolved WITHOUT parsing the
/// .omt: canonical path + declared (or active-range) size axis + in-library flag.
/// The axis VALUES are only needed by the dock/dialog (GradedSizeStyleContext).
struct GradedStyleAxis
{
    QString canonical;
    QString axisName;
    bool inLibrary = false;
    bool ready = false;
};

//---------------------------------------------------------------------------------------------------------------------
auto ResolveGradedStyleAxis(VPattern *doc, VContainer *pattern, const QString &activeRangeName)
    -> GradedStyleAxis
{
    GradedStyleAxis result;
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        return result;
    }
    const QString canonical =
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
    if (canonical.isEmpty())
    {
        return result;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const bool inLibrary = MeasurementLibraryScanner::IsInLibrary(
        settings->GetPathMeasurementLibrary(), canonical);

    // The styles axis: the table's declared size axis, falling back to the active
    // range's axis when none is declared (external table, or pre-declaration data).
    QString axis = inLibrary ? settings->GetSizeAxisForMeasurement(canonical) : QString();
    if (axis.isEmpty())
    {
        const auto *ranges = pattern != nullptr ? pattern->SizeRanges() : nullptr;
        if (ranges == nullptr || activeRangeName.isEmpty() || !ranges->contains(activeRangeName))
        {
            return result;
        }
        axis = ranges->value(activeRangeName).sizeAxis;
    }
    if (axis.isEmpty())
    {
        return result;
    }

    result.canonical = canonical;
    result.axisName = axis;
    result.inLibrary = inLibrary;
    result.ready = true;
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
// Re-style one layout item subtree (the silhouette tree returned by
// VLayoutPiece::GetItem). A size style (when present) recolors and re-dashes the
// trace items of that size; items excluded from the style (grainline, fold line,
// info blocks, annotations) keep the theme piece color — the style applies to the
// traces, never to the decor. The first override of an item stores its native pen
// (kGradedBasePenRole) so a later "no style / gate off" pass can restore it.
void StyleGradedSubtree(QGraphicsItem *item, const QColor &defaultColor, const OGradedSizeStyle *style)
{
    if (item == nullptr)
    {
        return;
    }
    // Interactive children of a graded piece/host resolve their own colors → leave
    // them untouched (and do not descend: their interior text/fills are theirs).
    const int type = item->type();
    if (type == OInfoBlockItem::Type || type == OAnnotationItem::Type || type == VGrainlineItem::Type)
    {
        return;
    }

    // Grainline / fold line are decor: never recolored by the size style. They still
    // follow the theme (default piece color) like every other trace.
    const bool decorTrace = (type == VGraphicsFillItem::Type || type == VGraphicsFoldLineItem::Type);

    if (auto *path = dynamic_cast<QGraphicsPathItem *>(item))
    {
        QPen pen = path->pen();
        if (pen.style() == Qt::NoPen)
        {
            // nothing drawn by this pen — nothing to restyle
        }
        else if (style != nullptr && style->HasColor() && !decorTrace)
        {
            // Remember the native pen (style/cap/width) before overriding it, once.
            if (!item->data(kGradedBasePenRole).isValid())
            {
                item->setData(kGradedBasePenRole, pen);
            }

            pen.setColor(style->color);
            if (style->dashes.isEmpty())
            {
                pen.setStyle(Qt::SolidLine); // colored solid line
                pen.setCapStyle(Qt::SquareCap);
            }
            else
            {
                // Dashes are stored in pt (the Illustrator unit the user types);
                // a Qt pen pattern is in units of the pen width, so a dash becomes
                // pt × (px per pt) / width — a 1 pt dash is 1.333 px at scale 1:1.
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
                pen.setCapStyle(Qt::FlatCap); // match the SVG export cap (butt)
            }
            path->setPen(pen);
        }
        else
        {
            // Default (theme) branch: restore a native pen we may have overridden, then re-tint.
            const QVariant base = item->data(kGradedBasePenRole);
            if (base.isValid())
            {
                pen = base.value<QPen>();
                item->setData(kGradedBasePenRole, QVariant()); // purge: snapshot is self-healing
            }
            pen.setColor(defaultColor);
            path->setPen(pen);
        }
    }
    else if (auto *text = dynamic_cast<QGraphicsSimpleTextItem *>(item))
    {
        // Place-label text follows the theme color; size styles never recolor text.
        text->setBrush(QBrush(defaultColor));
    }

    const auto children = item->childItems();
    for (auto *child : children)
    {
        StyleGradedSubtree(child, defaultColor, style);
    }
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::GradedSizeStyleContext(QString *canonicalPath, QString *axisName, QStringList *axisValues,
                                        bool *inLibrary) -> bool
{
    canonicalPath->clear();
    axisName->clear();
    axisValues->clear();
    *inLibrary = false;

    // Dock/dialog need the axis VALUES (rows), which require reading the .omt axes
    // — the (canonical, axis, in-library) part alone is resolved without parsing.
    const GradedStyleAxis axis = ResolveGradedStyleAxis(doc, pattern, m_activeGradedRangeName);
    if (!axis.ready)
    {
        return false;
    }

    QStringList axes;
    QMap<QString, QStringList> axisMap;
    if (!LoadGradedTableAxes(&axes, &axisMap))
    {
        return false;
    }
    const QStringList values = axisMap.value(axis.axisName);
    if (values.isEmpty())
    {
        return false;
    }

    *canonicalPath = axis.canonical;
    *axisName = axis.axisName;
    *axisValues = values;
    *inLibrary = axis.inLibrary;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ApplyGradedSizeStylesToScene()
{
    if (m_sceneGraded == nullptr || m_sceneGraded->items().isEmpty())
    {
        return;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    if (settings == nullptr)
    {
        return;
    }
    const QColor defaultColor = VSceneStylesheet::PatternPieceStyle().PieceColor();

    // "Show in canvas" gate + in-library guard: when off (dark themes) or the table
    // lives outside the library, no size style is applied — every size keeps the
    // current (default) render.
    QHash<QString, OGradedSizeStyle> styles;
    if (settings->GetGradedSizeStylesShowInCanvas())
    {
        // The canvas pass only needs the (canonical, axis) pair — never the axis
        // values, so no .omt read here (cheap on theme toggles / gate / rebuild).
        const GradedStyleAxis axis = ResolveGradedStyleAxis(doc, pattern, m_activeGradedRangeName);
        if (axis.ready && axis.inLibrary)
        {
            styles = OGradedSizeStyles::StylesForAxis(settings, axis.canonical, axis.axisName);
        }
    }

    const auto items = m_sceneGraded->items();
    for (auto *item : items)
    {
        if (GradedPieceId(item) == 0) // a silhouette root (reference or ghost layer)
        {
            continue;
        }
        // The pointer targets an entry of the LOCAL @p styles hash, which is never
        // mutated nor rebuilt during this pass — it stays valid for the recursion.
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
        StyleGradedSubtree(item, defaultColor, style);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshSizeStylesDock()
{
    if (m_dockManager == nullptr || m_dockManager->sizeStylesWidget() == nullptr)
    {
        return;
    }

    QString canonical;
    QString axis;
    QStringList values;
    bool inLibrary = false;
    if (GradedSizeStyleContext(&canonical, &axis, &values, &inLibrary))
    {
        m_dockManager->sizeStylesWidget()->SetSizeStylesContext(canonical, axis, values, inLibrary);
    }
    else
    {
        m_dockManager->sizeStylesWidget()->SetSizeStylesContext(QString(), QString(), QStringList(), false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::EditSizeStyles()
{
    QString canonical;
    QString axis;
    QStringList values;
    bool inLibrary = false;
    if (!GradedSizeStyleContext(&canonical, &axis, &values, &inLibrary) || !inLibrary)
    {
        return; // styles need a measurements table from the library (button is disabled otherwise)
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const QHash<QString, OGradedSizeStyle> existing =
        OGradedSizeStyles::StylesForAxis(settings, canonical, axis);

    OGradedSizeStylesDialog dialog(values, existing, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return; // Cancel → no write at all
    }

    OGradedSizeStyles::SaveAxisStyles(settings, canonical, axis, dialog.styles());
    ApplyGradedSizeStylesToScene();
    RefreshSizeStylesDock();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowGradedSizeStylesInCanvas(bool on)
{
    auto *settings = VAbstractApplication::VApp()->Settings();
    if (settings != nullptr)
    {
        settings->SetGradedSizeStylesShowInCanvas(on);
    }
    ApplyGradedSizeStylesToScene();
}
