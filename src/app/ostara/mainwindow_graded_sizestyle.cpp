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
#include "ogradedsizetrace.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetsizestyles.h"
#include "xml/vpattern.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QColor>
#include <QDialog>
#include <QMap>

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
    const OGradedSizeStyles::StyleAxis axis =
        OGradedSizeStyles::ResolveStyleAxis(doc, pattern, m_activeGradedRangeName);
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
        const OGradedSizeStyles::StyleAxis axis =
        OGradedSizeStyles::ResolveStyleAxis(doc, pattern, m_activeGradedRangeName);
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
        // The canvas outlives this pass — it replays on a theme change, on the dialog's OK
        // and on the "Show in canvas" toggle — so the pass owns the default rendering: it
        // re-tints what no style covers and restores the native pen it overrode.
        OGradedSizeTrace::ApplyToSubtree(item, styles, OGradedSizeTrace::Scope::LiveScene, defaultColor);
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
