/************************************************************************
 **
 **  @file   ooutputsstale.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Implementation of the Export-mode staleness warning.
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

#include "ooutputsstale.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QPixmap>
#include <QStyle>
#include <QWidget>

#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpropertyexplorer/checkablemessagebox.h"

//---------------------------------------------------------------------------------------------------------------------
auto OOutputsStale::ShouldWarn() -> bool
{
    return VAbstractApplication::VApp()->Settings()->GetAskRegenerateOutputs();
}

//---------------------------------------------------------------------------------------------------------------------
void OOutputsStale::Show(QWidget *parent)
{
    // The instance API, not the doNotAskAgain* helpers: those write to their own QSettings
    // key, which the preferences' warning reset does not reach. Going through
    // VCommonSettings keeps this warning resettable like the two it sits next to.
    Utils::CheckableMessageBox box(parent);
    box.setIconPixmap(QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(32, 32));
    box.setWindowTitle(tr("Export mode"));
    box.setText(tr("You have made changes. Your outputs must be regenerated."));
    box.setCheckBoxVisible(true);
    // The box carries its own label ("Do not ask again"), which does not say the same
    // thing: here the user is not answering a question, she is asking to be spared a
    // message she has understood.
    box.setCheckBoxText(tr("Do not show this message again"));
    box.setChecked(false); // unticked by default — the message is meant to be read once
    box.setStandardButtons(QDialogButtonBox::Ok);
    box.setDefaultButton(QDialogButtonBox::Ok);
    box.exec();

    if (box.isChecked())
    {
        VAbstractApplication::VApp()->Settings()->SetAskRegenerateOutputs(false);
    }
}
