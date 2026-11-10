/************************************************************************
 **
 **  @file   tst_foldermove.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  Changing a library folder moves its files, and the move is all or nothing: one file that cannot
 **          arrive undoes every copy already made, and the source keeps everything. A half-moved library, with
 **          the setting already pointing at the new folder, is a state no interface can repair.
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

#ifndef TST_FOLDERMOVE_H
#define TST_FOLDERMOVE_H

#include <QObject>

class TST_FolderMove : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_FolderMove(QObject *parent = nullptr);
    ~TST_FolderMove() override = default;

private slots:
    void MovesEverythingIncludingSubfolders();
    void OneFileThatCannotArriveLeavesEverything();
    void DestinationThatCannotBeCreatedChangesNothing();
    void MissingSourceIsNotAFailure();
    void NestedFoldersAreRefused();
    void MovingAFolderOntoItselfChangesNothing();
    void AFailedMoveKeepsTheFoldersItFound();
    void ACallerListDoesNotDecideTheMove();
};

#endif // TST_FOLDERMOVE_H
