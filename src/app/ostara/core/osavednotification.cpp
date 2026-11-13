/************************************************************************
 **
 **  @file   osavednotification.cpp
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

#include "osavednotification.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QString>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto Title() -> QString
{
    return QCoreApplication::translate("OSavedNotification", "Save successful");
}

//---------------------------------------------------------------------------------------------------------------------
// The house's success box: QMessageBox::information, the same call the export confirmations use,
// so a save and an export answer in the same voice and with the same icon.
void Show(QWidget *parent, const QString &sentence)
{
    QMessageBox::information(parent, Title(), sentence);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OSavedNotification::Template(QWidget *parent)
{
    Show(parent, QCoreApplication::translate("OSavedNotification", "The template has been saved."));
}

//---------------------------------------------------------------------------------------------------------------------
void OSavedNotification::Measurements(QWidget *parent)
{
    Show(parent, QCoreApplication::translate("OSavedNotification", "The measurements table has been saved."));
}
