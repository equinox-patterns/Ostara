/************************************************************************
 **
 **  @file   dialogseamallowance_helpers.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Shared helpers for DialogSeamAllowance.
 **  @note   Extracted from dialogseamallowance.cpp by Sophie Denys
 **          <contact@sophiedenys.com> as part of Ostara refactoring.
 **
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef DIALOGSEAMALLOWANCE_HELPERS_H
#define DIALOGSEAMALLOWANCE_HELPERS_H

#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QStringList>

#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"

enum TabOrder { Paths = 0, Pins = 1, Labels = 2, Grainline = 3, Passmarks = 4, PlaceLabels = 5, FoldLine = 6, Count = 7 };

enum class MainPathContextMenuOption : int
{
    NoSelection,
    Reverse,
    NonePassmark,
    OneLine,
    TwoLines,
    ThreeLines,
    TMark,
    ExternalVMark,
    InternalVMark,
    UMark,
    BoxMark,
    CheckMark,
    Uniqueness,
    TurnPoint,
    Excluded,
    DuplicateNode,
    Delete,
    LAST_ONE_DO_NOT_USE
};

//---------------------------------------------------------------------------------------------------------------------
inline void EnableDefButton(QPushButton *defButton, const QString &formula)
{
    SCASSERT(defButton != nullptr)

    if (formula != currentSeamAllowance)
    {
        defButton->setEnabled(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
inline auto GetFormulaFromUser(QPlainTextEdit *textEdit) -> QString
{
    SCASSERT(textEdit != nullptr)
    return VTranslateVars::TryFormulaFromUser(textEdit->toPlainText(),
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
inline void InitComboBoxFormats(QComboBox *box, const QStringList &items, const QString &currentFormat)
{
    SCASSERT(box != nullptr)

    const QSignalBlocker blocker(box);
    box->addItems(items);
    int const index = box->findText(currentFormat);
    if (index != -1)
    {
        box->setCurrentIndex(index);
    }
    else
    {
        box->setCurrentIndex(0);
    }
}

#endif // DIALOGSEAMALLOWANCE_HELPERS_H
