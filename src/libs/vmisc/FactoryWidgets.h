/************************************************************************
 **
 **  @file   FactoryWidgets.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   03/08/2026
 **
 **  @brief  Factory functions for Ostara widgets — each function creates
 **          a pre-configured Qt widget ready for use in the UI.
 **
 **          Usage:
 **            auto *combo = MakeComboBox(this);
 **            combo->addItems({"Option A", "Option B"});
 **
 **          The factory encapsulates configuration that would otherwise
 **          need to be repeated at every call site (macOS popup fix, etc.).
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

#ifndef FACTORYWIDGETS_H
#define FACTORYWIDGETS_H

#include <QComboBox>
#include <QListView>

/// Applies the Qt-styled QListView dropdown to an existing QComboBox (e.g. one
/// created by Qt Designer's setupUi).  The native macOS popup ignores QSS
/// entirely (hover, selection colours, borders) — this ensures the global
/// stylesheet is respected.
inline void ApplyComboBoxView(QComboBox *combo)
{
    auto *view = new QListView();
    combo->setView(view);
    // Keep the dropdown icons the same size as the closed-field icons (e.g. the
    // line-style swatches, which are 80x14 in the dialogs' .ui).
    view->setIconSize(combo->iconSize());
}

/// Creates a QComboBox whose dropdown uses a Qt-styled QListView instead of
/// the native macOS popup.  The native popup ignores QSS entirely (hover,
/// selection colours, borders) — this factory ensures the global stylesheet
/// is respected.
inline auto MakeComboBox(QWidget *parent = nullptr) -> QComboBox *
{
    auto *combo = new QComboBox(parent);
    ApplyComboBoxView(combo);
    return combo;
}

#endif // FACTORYWIDGETS_H
