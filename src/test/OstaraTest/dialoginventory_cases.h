/************************************************************************
 **
 **  @file   dialoginventory_cases.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  La liste des dialogues à mesurer — **générée à la configuration** depuis les `.ui` du
 **          dépôt, pour qu'aucune n'échappe à l'inventaire parce qu'on a oublié de l'y inscrire.
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

#ifndef DIALOGINVENTORY_CASES_H
#define DIALOGINVENTORY_CASES_H

#include <QDialog>
#include <QVector>

#include <functional>

/**
 * @brief One dialog of the application, built from its .ui and nothing else.
 *
 * The .ui is the floor: it says how tall the dialog is before its constructor adds the rows a pattern
 * brings. That is exactly what an inventory needs — a floor that no data can change.
 */
struct DialogCase
{
    const char *name;
    std::function<void(QDialog &)> setup;
};

/// Every dialog built from a `.ui` — the list is generated at configure time.
auto AllDialogCases() -> QVector<DialogCase>;

/// Every dialog that builds its interface in code, and is therefore invisible to the `.ui` sweep.
auto AllCodeBuiltCases() -> QVector<DialogCase>;

#endif // DIALOGINVENTORY_CASES_H
