/************************************************************************
 **
 **  @file   tst_pleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  Le pli : ce que la regle deplace, ou elle pose la ligne, et ce qu'elle refuse.
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

#ifndef TST_PLEAT_H
#define TST_PLEAT_H

#include <QObject>

class TST_Pleat : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_Pleat(QObject *parent = nullptr);
    ~TST_Pleat() override = default;

private slots:
    void ObjectInTheRegionTravels();
    void ObjectOutOfTheRegionStays();
    void DepartureLineStandsOnItself();
    void SecondLineIsOneStepOn();
    void PleatCountAddsTheFolds();
    void EveryRefusalSaysWhatToDo();
};

#endif // TST_PLEAT_H
