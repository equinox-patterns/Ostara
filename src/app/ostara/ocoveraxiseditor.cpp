/************************************************************************
 **
 **  @file   ocoveraxiseditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The axes a cover page states beside its sizes, minus the one its range grades along.
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

#include "ocoveraxiseditor.h"

#include "core/ocovergradedaxis.h"

OCoverAxesEditor::OCoverAxesEditor(const OTemplateOptionContext &context, QWidget *parent)
  : OTemplateAxisEditor(context, parent)
{
    // The base built itself once already, with nothing reserved — a box the user could tick for a
    // fraction of a second would be a box they could tick. Read again here, before the form is
    // shown, so the first thing drawn is already the right thing.
    Refresh(context);
}

void OCoverAxesEditor::Refresh(const OTemplateOptionContext &context)
{
    SetReservedAxis(OCoverGradedAxisOptions::FromOptions(context.options).axis);
    OTemplateAxisEditor::Refresh(context);
}
