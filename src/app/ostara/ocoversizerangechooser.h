/************************************************************************
 **
 **  @file   ocoversizerangechooser.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The block that asks which sizes a cover page previews, and says what it is for.
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

#ifndef OCOVERSIZERANGECHOOSER_H
#define OCOVERSIZERANGECHOOSER_H

#include "core/ocovergradedaxis.h"
#include "core/otemplateoptioneditor.h"

class QLabel;
class QPushButton;

/**
 * @brief The one question a range page cannot be designed without, asked before anything else.
 *
 * **It stands first, and the settings that follow it wait for the answer.** Which sizes the page
 * lists decides which axis runs and which axes stand still, so until a range is chosen the rest of
 * the information has nothing to answer about — the form says so by making those settings unusable
 * rather than by hiding them, so the user sees what is coming and what is holding it up.
 *
 * The two things it says out loud — the range it currently holds, and the fact that the printed
 * page will take the range its export was run with — are the same sentence twice: this range is
 * here to draw with.
 */
class OCoverSizeRangeChooser : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OCoverSizeRangeChooser(const OTemplateOptionContext &context,
                                    QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;

private:
    void BuildUi();
    void ChooseRange();

    OTemplateOptionContext m_context;
    QPushButton *m_chooseButton{nullptr};
    QLabel *m_summary{nullptr};
    QLabel *m_note{nullptr};
    QLabel *m_noTable{nullptr};
};

#endif // OCOVERSIZERANGECHOOSER_H
