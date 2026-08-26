/************************************************************************
 **
 **  @file   DockWrapper.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/08/2026
 **
 **  @brief  Standard dock content wrapper — ensures consistent structure,
 **          viewport theming, and spacing across all Ostara docks.
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

#ifndef DOCKWRAPPER_H
#define DOCKWRAPPER_H

class QDockWidget;
class QWidget;

/**
 * @brief WrapDockContent wraps a content widget in the standard dock structure
 *        matching ToolOptions and BackgroundImages:
 *
 *        dock → root QWidget (top margin 8px, $DockBody gap)
 *             → QScrollArea (viewport themed $DockTitle)
 *               → scrollContents QWidget (top margin 8px, $DockTitle gap)
 *                 → QGroupBox (no title, QSS margins, #dockContentGroupBox)
 *                   → content
 *
 *        The QScrollArea viewport is a plain QWidget — the inline dot-selector
 *        .QWidget { background } works on it (SUBTILITES_CODE.md §13), unlike
 *        QAbstractItemView viewports which overpaint in paintEvent (§13c).
 *
 *        The viewport background fix is applied immediately at creation time
 *        using VTheme::ColorSheme(), without waiting for showEvent or theme
 *        change (SUBTILITES_CODE.md §13e.3).
 */
void WrapDockContent(QDockWidget *dock, QWidget *content);

/**
 * @brief WrapDockContentWidget builds the standard dock content structure
 *        (root → QScrollArea → scrollContents → QGroupBox → content) as a
 *        plain widget tree under @p parent, WITHOUT installing it on a dock.
 *        Used for QStackedWidget pages (e.g. the merged Options dock).
 */
QWidget *WrapDockContentWidget(QWidget *content, QWidget *parent);

#endif // DOCKWRAPPER_H
