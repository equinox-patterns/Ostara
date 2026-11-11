/************************************************************************
 **
 **  @file   tst_rollpages.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  La découpe d'un rouleau en pages : la longueur demandée, le plafond du PDF, les
 **          marges latérales, et la règle qui ne se négocie pas — aucune pièce à cheval.
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

#ifndef TST_ROLLPAGES_H
#define TST_ROLLPAGES_H

#include <QObject>

class TST_RollPages : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_RollPages(QObject *parent = nullptr);
    ~TST_RollPages() override = default;

private slots:
    void FitsOnOnePage();
    void TrimmedWidth();
    void SplitAtMaxLength();
    void SplitAndTrim();
    void PieceLongerThanMax();
    void CompactsALargeGap();
    void ThreePagesWhenNeeded();
    void CoverPageIsNotCut();
};

#endif // TST_ROLLPAGES_H
