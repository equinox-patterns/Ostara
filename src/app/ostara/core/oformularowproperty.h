/************************************************************************
 **
 **  @file   oformularowproperty.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  a formula row drawn on a single line, label beside the field
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

#ifndef OFORMULAROWPROPERTY_H
#define OFORMULAROWPROPERTY_H

#include "vformulaproperty.h"

class QLineEdit;

//! A formula row drawn on one line, its label beside the field, kept out of the form's label column
//! (VProperty::setSpanningRow).
//!
//! VFormulaProperty draws a bold label on its own line followed by an indented "Formula" row, and its label
//! widens the label column of the whole form: every shorter row then sees its field pushed to the right
//! (Close dart, where "Closing divisor:" faces "Notes:"). The formula itself is inherited whole — value,
//! notation, refusal revert and fx dialog — only the drawing changes.
class OFormulaRowProperty : public VFormulaProperty
{
    Q_OBJECT // NOLINT

public:
    explicit OFormulaRowProperty(const QString &name);

    //! The field of this row *is* the formula: the "Formula" child of VFormulaProperty is not drawn.
    auto getRowCount() const -> vpesizetype override;

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                      const QAbstractItemDelegate *delegate) -> QWidget * override;
    auto setEditorData(QWidget *editor) -> bool override;
    auto getEditorData(const QWidget *editor) const -> QVariant override;

    //! Puts a value the tool kept — it refused the one just typed — back into the field, over the text the
    //! user is still writing in it. A refusal has to win, unlike the periodic refresh of the dock.
    void RevertTo(const VFormula &formula);

private:
    //! Tells the form the field was edited, so that the dock hands the value over to the tool.
    void Commit();
    void OpenFormulaDialog();

    QWidget *m_row{nullptr};
    QLineEdit *m_field{nullptr};
};

#endif // OFORMULAROWPROPERTY_H
