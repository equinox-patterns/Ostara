/************************************************************************
 **
 **  @file   tst_dialogfit.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  The two invariants every dialog owes the screen — never taller than the usable area,
 **          never stretched inside — verified on the policy itself, at the sizes where they break.
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

#ifndef TST_DIALOGFIT_H
#define TST_DIALOGFIT_H

#include <QObject>

class TST_DialogFit : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_DialogFit(QObject *parent = nullptr);
    ~TST_DialogFit() override = default;

private slots:
    void ATooTallDialogGetsAScrollAreaAndFits();
    void TheButtonsStayInTheFrame();
    void TheDialogIsWrappedOnlyOnce();
    void ADialogThatFitsIsLeftAlone();
    void AFullWidthRowDoesNotSwallowTheSurplus();
    void ADialogThatCannotScrollIsReported();
    void ACustomButtonRowStaysInTheFrame();
    void TheExportDialogShapeFitsOnASmallScreen();
    void ANoteFieldKeepsGrowing();
    void ATooWideDialogGetsAScrollAreaAndFits();
    void TheInventoryNamesEveryDialogItMet();
    void AContentThatGrowsAfterTheShowIsFitted();
    void AContainerHoldingATableKeepsItsRoom();
    void ALongListDoesNotGrowTheDialog();
};

#endif // TST_DIALOGFIT_H
