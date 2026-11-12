/************************************************************************
 **
 **  @file   tst_grainlinearrows.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/11/2026
 **
 **  @brief  The twelve grainline arrow states and the six buttons the options dock offers
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

#ifndef TST_GRAINLINEARROWS_H
#define TST_GRAINLINEARROWS_H

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

class TST_GrainlineArrows : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_GrainlineArrows(QObject *parent = nullptr);

private slots:
    void TestEveryValueSurvivesADecode();
    void TestEveryCombinationIsDistinct();
    void TestOneArrowUsesTheRightEnd();
    void TestUnknownValueFallsBackToThePlainGrainline();

private:
    Q_DISABLE_COPY_MOVE(TST_GrainlineArrows) // NOLINT
};

#endif // TST_GRAINLINEARROWS_H
