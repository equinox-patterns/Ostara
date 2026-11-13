/************************************************************************
 **
 **  @file   tst_fullscreen.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Essai du plein écran que l'application possède, et du filet qui le rend après une dialogue.
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

#ifndef TST_FULLSCREEN_H
#define TST_FULLSCREEN_H

#include <QObject>

class TST_FullScreen : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_FullScreen(QObject *parent = nullptr);
    ~TST_FullScreen() override = default;

private slots:
    void cleanup();

    void TheNativeTakeoverFillsTheScreen();
    void TheFramelessTakeoverCoversTheScreen();
    void TheMaximizedWindowComesBackMaximized();
    void TheStateTheSystemTookAwayIsNotActive();
    void TheKeeperGivesTheScreenBackAfterTheDialog();
    void TheKeeperLeavesAWindowAloneItNeverTook();
    void TheKeeperWaitsForTheLastDialog();
    void TheKeeperWaitsUntilTheDialogReallyLeaves();
    void TheChoiceToLeaveIsNotUndoneByADialog();
    void TheFailureCasesAreHarmless();
    void AWindowAlreadyFullScreenIsNotTrapped();
};

#endif // TST_FULLSCREEN_H
