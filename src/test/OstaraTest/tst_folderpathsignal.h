/************************************************************************
 **
 **  @file   tst_folderpathsignal.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  Every library folder setter announces the move, once, with the settings key it wrote. The windows
 **          read these keys at the moment they are used, so a folder that moves without a word keeps being
 **          answered from the previous path until the reader happens to touch something else.
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

#ifndef TST_FOLDERPATHSIGNAL_H
#define TST_FOLDERPATHSIGNAL_H

#include <QObject>

class TST_FolderPathSignal : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_FolderPathSignal(QObject *parent = nullptr);
    ~TST_FolderPathSignal() override = default;

private slots:
    void EverySetterAnnouncesItsFolder();
    void WritingTheSameFolderTwiceSaysItOnce();
};

#endif // TST_FOLDERPATHSIGNAL_H
