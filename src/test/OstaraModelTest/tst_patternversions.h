/************************************************************************
 **
 **  @file   tst_patternversions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  The versions of the pattern, as the dock and the information
 **          block read them.
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

#ifndef TST_PATTERN_VERSIONS_H
#define TST_PATTERN_VERSIONS_H

#include <QObject>

class TST_PatternVersions : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_PatternVersions(QObject *parent = nullptr);
    ~TST_PatternVersions() override = default;

private slots:
    // A group carries no version of its own: it shows the versions of its members, all of them and in
    // the pattern's own order — « All versions + Version A », never « All versions » alone.
    void GroupShowsItsMembersVersions();

private:
    Q_DISABLE_COPY_MOVE(TST_PatternVersions) // NOLINT
};

#endif // TST_PATTERN_VERSIONS_H
