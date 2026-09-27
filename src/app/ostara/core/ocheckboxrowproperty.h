/************************************************************************
 **
 **  @file   ocheckboxrowproperty.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  a boolean row drawn on one full-width line, its text beside the box
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

#ifndef OCHECKBOXROWPROPERTY_H
#define OCHECKBOXROWPROPERTY_H

#include "../vpropertyexplorer/plugins/vboolproperty.h"

class QCheckBox;
class QLabel;

//! A boolean row drawn on a single full-width line: the box, then its text, kept out of the form's label
//! column (VProperty::setSpanningRow).
//!
//! VBoolProperty draws a bare QCheckBox and leaves its label to the form's label column. A label long enough
//! to explain a behaviour then widens that column for the whole form, and every shorter row sees its field
//! pushed to the right and squeezed — the cut tools' "Show the two segments instead of the curve" did exactly
//! that to Alias, Length and Notes. Here the text follows the box on the same line and wraps when the dock is
//! narrow, so it can neither widen the label column nor force a horizontal scrollbar. The boolean itself is
//! inherited whole: value, change event and revert.
class OCheckBoxRowProperty : public VPE::VBoolProperty
{
    Q_OBJECT // NOLINT

public:
    explicit OCheckBoxRowProperty(const QString &text);

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                      const QAbstractItemDelegate *delegate) -> QWidget * override;
    auto data(int column, int role) const -> QVariant override;
    auto setEditorData(QWidget *editor) -> bool override;
    auto getEditorData(const QWidget *editor) const -> QVariant override;

private:
    Q_DISABLE_COPY_MOVE(OCheckBoxRowProperty) // NOLINT

    QString m_text;

    QWidget *m_row{nullptr};
    QCheckBox *m_box{nullptr};
};

#endif // OCHECKBOXROWPROPERTY_H
