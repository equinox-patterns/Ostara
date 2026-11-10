/************************************************************************
 **
 **  @file   tst_dialoginventory.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  L'inventaire mesuré : la hauteur que chaque dialogue du dépôt réclame réellement,
 **          comparée à ce qu'un écran de 1024×768 et un de 1366×768 laissent.
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

#ifndef TST_DIALOGINVENTORY_H
#define TST_DIALOGINVENTORY_H

#include <QObject>

class TST_DialogInventory : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_DialogInventory(QObject *parent = nullptr);
    ~TST_DialogInventory() override = default;

private slots:
    void EveryDialogFitsTheScreensOfTheObjective();
};

#endif // TST_DIALOGINVENTORY_H
