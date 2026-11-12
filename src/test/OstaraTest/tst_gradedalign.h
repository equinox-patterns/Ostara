/************************************************************************
 **
 **  @file   tst_gradedalign.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/11/2026
 **
 **  @brief  L'écart d'alignement gradué, axe par axe : ancres, point forcé, et l'un sans l'autre.
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

#ifndef TST_GRADEDALIGN_H
#define TST_GRADEDALIGN_H

#include <QObject>

class TST_GradedAlign : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_GradedAlign(QObject *parent = nullptr);
    ~TST_GradedAlign() override = default;

private slots:
    void AnchorsDriveBothAxes();
    void ForcedPointDrivesHorizontalOnly();
    void ForcedPointDrivesVerticalOnly();
    void BothAxesFollowTheirOwnPoint();
    void MissingLayerPointFallsBackOnItsAnchor();
};

#endif // TST_GRADEDALIGN_H
