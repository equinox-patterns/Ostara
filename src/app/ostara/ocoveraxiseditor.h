/************************************************************************
 **
 **  @file   ocoveraxiseditor.h
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

#ifndef OCOVERAXISEDITOR_H
#define OCOVERAXISEDITOR_H

#include "widgets/Template/otemplateaxiseditor.h"

/**
 * @brief The shared axes control, told which axis the page's range already speaks for.
 *
 * **The one thing a cover page knows that a cartouche does not.** The shared control offers every
 * axis the table declares, because offering them is all it can do; a cover page listing a range has
 * one axis it must not offer — the one the range grades along, which the page already states one
 * line per size. The difference is a single answer read from the section's own options, so it is a
 * subclass rather than a second control: everything else is the same question about the same table.
 *
 * It reads the answer on every @ref Refresh, which is what keeps it right when the user picks a
 * range on a different axis — the case a value captured once would get wrong.
 */
class OCoverAxesEditor : public OTemplateAxisEditor
{
    Q_OBJECT

public:
    explicit OCoverAxesEditor(const OTemplateOptionContext &context, QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;
};

#endif // OCOVERAXISEDITOR_H
