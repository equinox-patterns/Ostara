/************************************************************************
 **
 **  @file   ocheckboxrowproperty.cpp
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

#include "ocheckboxrowproperty.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QSizePolicy>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "../vmisc/compatibility.h"
#include "../vpropertyexplorer/vproperty.h"
#include "../vpropertyexplorer/vproperty_p.h"

//---------------------------------------------------------------------------------------------------------------------
OCheckBoxRowProperty::OCheckBoxRowProperty(const QString &text)
  : VPE::VBoolProperty(QString()),
    m_text(text)
{
    // The row carries its own text: it must not land in the form's label column.
    setSpanningRow(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxRowProperty::createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                                        const QAbstractItemDelegate *delegate) -> QWidget *
{
    Q_UNUSED(options)
    Q_UNUSED(delegate)

    // QFrame, not a bare QWidget: a plain QWidget paints no background, so the dock viewport's own
    // colour shows through the row and draws a patch behind the box (SubtiliteCode 02, §14). QFrame with
    // NoFrame stays invisible here, exactly like the row buildEditor wraps around a spanning property.
    auto *frame = new QFrame(parent);
    frame->setFrameStyle(QFrame::NoFrame);
    m_row = frame;
    m_row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *layout = new QHBoxLayout(m_row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    m_box = new QCheckBox(m_row);
    m_box->setCheckState(vproperty_d_ptr->VariantValue.toBool() ? Qt::Checked : Qt::Unchecked);
    connect(m_box,
            CHECKBOX_STATE_CHANGED,
            this,
            [this]() -> void
            {
                QCoreApplication::postEvent(vproperty_d_ptr->editor, new VPE::UserChangeEvent());
            });

    // Word wrap so a long explanation cannot widen the dock nor force a horizontal scrollbar.
    auto *label = new QLabel(m_text, m_row);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    layout->addWidget(m_box);
    layout->addWidget(label, 1);

    vproperty_d_ptr->editor = m_row;
    return m_row;
}

//---------------------------------------------------------------------------------------------------------------------
// Same contract as VBoolProperty::data, minus its unchecked qobject_cast on the editor: the editor is the row
// here, not the box.
auto OCheckBoxRowProperty::data(int column, int role) const -> QVariant
{
    if (column == DPC_Data)
    {
        if (Qt::DisplayRole == role)
        {
            const bool checked = m_box != nullptr ? m_box->checkState() == Qt::Checked
                                                  : vproperty_d_ptr->VariantValue.toBool();
            return checked ? QVariant(Qt::Checked) : QVariant(Qt::Unchecked);
        }

        if (Qt::EditRole == role)
        {
            return vproperty_d_ptr->VariantValue;
        }
    }

    return VPE::VProperty::data(column, role);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxRowProperty::setEditorData(QWidget *editor) -> bool
{
    if (m_box == nullptr || editor != m_row)
    {
        return false;
    }

    const QSignalBlocker blocker(m_box);
    m_box->setCheckState(vproperty_d_ptr->VariantValue.toBool() ? Qt::Checked : Qt::Unchecked);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxRowProperty::getEditorData(const QWidget *editor) const -> QVariant
{
    if (m_box == nullptr || editor != m_row)
    {
        return {};
    }

    return m_box->checkState() == Qt::Checked ? QVariant(Qt::Checked) : QVariant(Qt::Unchecked);
}
