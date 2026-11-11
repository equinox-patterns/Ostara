/************************************************************************
 **
 **  @file   tst_splineportions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  Les morceaux d'un chemin coupe a ses propres noeuds : combien, lesquels, et ce qu'ils
 **          deviennent quand une borne ne designe plus rien.
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

#ifndef TST_SPLINEPORTIONS_H
#define TST_SPLINEPORTIONS_H

#include <QObject>

class TST_SplinePortions : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_SplinePortions(QObject *parent = nullptr);
    ~TST_SplinePortions() override = default;

private slots:
    void NoCutIsOneRun();
    void ThreeCutsMakeFourRuns();
    void EndsAreNeverCut();
    void BoxOrderDoesNotChangeTheRuns();
    void RunsRoundTripThroughTheFile();
    void RunKeysNameBothEnds();
    void LegacyIdsAreStillRead();
};

#endif // TST_SPLINEPORTIONS_H
