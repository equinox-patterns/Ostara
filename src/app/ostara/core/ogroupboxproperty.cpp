/************************************************************************
 **
 **  @file   ogroupboxproperty.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/10/2026
 **
 **  @brief  a group of rows drawn the way a dialog groups its options: a titled QGroupBox
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

#include "ogroupboxproperty.h"

#include <QCoreApplication>
#include <QGroupBox>
#include <QSignalBlocker>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "../vpropertyexplorer/vproperty.h"
#include "../vpropertyexplorer/vproperty_p.h"

//---------------------------------------------------------------------------------------------------------------------
OGroupBoxProperty::OGroupBoxProperty(const QString &title, bool checkable)
  : VPE::VBoolProperty(title),
    m_checkable(checkable)
{
    // The framework draws the frame, and the rows the owner added as children go inside it.
    setGroupBox(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OGroupBoxProperty::createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                                     const QAbstractItemDelegate *delegate) -> QWidget *
{
    Q_UNUSED(options)
    Q_UNUSED(delegate)

    m_box = new QGroupBox(getName(), parent);
    m_box->setCheckable(m_checkable);
    m_box->setChecked(m_checkable && vproperty_d_ptr->VariantValue.toBool());

    if (m_checkable)
    {
        // The frame's own box is the group's value: toggling it asks the dock for the change, exactly as a row
        // would (see OCheckBoxRowProperty). It is wired last, once the box carries its state: reporting that state
        // before the frame has become the property's editor would post a change event on nothing.
        connect(m_box,
                &QGroupBox::toggled,
                this,
                [this]() -> void
                {
                    QCoreApplication::postEvent(vproperty_d_ptr->editor, new VPE::UserChangeEvent());
                });
    }

    vproperty_d_ptr->editor = m_box;
    return m_box;
}

//---------------------------------------------------------------------------------------------------------------------
// Same contract as VBoolProperty::data, minus its unchecked qobject_cast on the editor: the editor is the frame
// here, not a check box.
auto OGroupBoxProperty::data(int column, int role) const -> QVariant
{
    if (column == DPC_Data)
    {
        if (Qt::DisplayRole == role)
        {
            const bool checked = m_box != nullptr ? m_box->isChecked() : vproperty_d_ptr->VariantValue.toBool();
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
auto OGroupBoxProperty::setEditorData(QWidget *editor) -> bool
{
    if (m_box == nullptr || editor != m_box)
    {
        return false;
    }

    const QSignalBlocker blocker(m_box);
    m_box->setChecked(m_checkable && vproperty_d_ptr->VariantValue.toBool());
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGroupBoxProperty::getEditorData(const QWidget *editor) const -> QVariant
{
    if (m_box == nullptr || editor != m_box)
    {
        return {};
    }

    return m_box->isChecked() ? QVariant(Qt::Checked) : QVariant(Qt::Unchecked);
}
