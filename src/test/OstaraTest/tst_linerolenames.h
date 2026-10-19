/************************************************************************
 **
 **  @file   tst_linerolenames.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  A line name lives only as long as the two points it joins: `<lineNames>` is a sibling of the blocks,
 **          so nothing rewrites it when a point or a whole piece leaves. This is the container's side of that
 **          rule — the names whose pair is gone are dropped, and the name of a line that is gone stops counting
 **          as taken.
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

#ifndef TST_LINEROLENAMES_H
#define TST_LINEROLENAMES_H

#include <QObject>

class TST_LineRoleNames : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_LineRoleNames(QObject *parent = nullptr);
    ~TST_LineRoleNames() override = default;

private slots:
    void OrphanNameIsDroppedAndTheNameIsFreed();
    void NameNoLineBearsIsDropped();
    void UnreadablePairIsKept();
    void NameOfAnotherLineIsTaken();
};

#endif // TST_LINEROLENAMES_H
