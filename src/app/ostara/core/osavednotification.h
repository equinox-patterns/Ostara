/************************************************************************
 **
 **  @file   osavednotification.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  La boîte qui dit qu'un enregistrement a eu lieu, la même dans tous les éditeurs.
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

#ifndef OSAVEDNOTIFICATION_H
#define OSAVEDNOTIFICATION_H

class QWidget;

/**
 * @brief What every editor says after it wrote its file.
 *
 * A save that says nothing looks exactly like a save that failed: the user presses Save, and then
 * has to go and look in the folder to know. Every editor that writes a file asks this module, so
 * the icon, the title and the single button are the same everywhere; only the sentence is its own,
 * because only the editor knows what it wrote.
 *
 * The box is modal on purpose. The user asked for a file to be written, this is the answer to that
 * question, and it carries one button — one click, and nothing to decide.
 */
namespace OSavedNotification
{
/** @brief « The template has been saved. » — after a template editor wrote its file. */
void Template(QWidget *parent);

/** @brief « The measurements table has been saved. » — after the measurements editor wrote its file. */
void Measurements(QWidget *parent);
} // namespace OSavedNotification

#endif // OSAVEDNOTIFICATION_H
