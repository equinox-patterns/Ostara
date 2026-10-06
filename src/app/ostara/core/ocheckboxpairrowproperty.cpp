/************************************************************************
 **
 **  @file   ocheckboxpairrowproperty.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/10/2026
 **
 **  @brief  two boolean boxes drawn on one full-width line, their texts beside them
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

#include "ocheckboxpairrowproperty.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QMetaType>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyleOptionViewItem>
#include <QVariant>
#include <QWidget>

#include "../vmisc/compatibility.h"
#include "../vpropertyexplorer/vproperty.h"
#include "../vpropertyexplorer/vproperty_p.h"

//---------------------------------------------------------------------------------------------------------------------
OCheckBoxPairRowProperty::OCheckBoxPairRowProperty(const QString &firstText, const QString &secondText)
  : VPE::VProperty(QString(), QMetaType::Int),
    m_firstText(firstText),
    m_secondText(secondText)
{
    // The row carries its own texts: they must not land in the form's label column.
    setSpanningRow(true);

    // No box drawn yet: the value is what says which of them will be ticked.
    VPE::VProperty::setValue(0);
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckBoxPairRowProperty::SetState(bool first, bool second)
{
    // setValue keeps the boxes in step with the value, and does nothing at all before the row exists.
    VPE::VProperty::setValue((first ? kFirstBox : 0) | (second ? kSecondBox : 0));
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxPairRowProperty::State() const -> int
{
    const int stored = vproperty_d_ptr->VariantValue.toInt();

    int state = 0;

    if (m_first != nullptr ? m_first->checkState() == Qt::Checked : (stored & kFirstBox) != 0)
    {
        state |= kFirstBox;
    }

    if (m_secondBox != nullptr ? m_secondBox->checkState() == Qt::Checked : (stored & kSecondBox) != 0)
    {
        state |= kSecondBox;
    }

    return state;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxPairRowProperty::FirstChecked() const -> bool
{
    return (State() & kFirstBox) != 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxPairRowProperty::SecondChecked() const -> bool
{
    return (State() & kSecondBox) != 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxPairRowProperty::createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                                            const QAbstractItemDelegate *delegate) -> QWidget *
{
    Q_UNUSED(options)
    Q_UNUSED(delegate)

    // QFrame, not a bare QWidget: a plain QWidget paints no background, so the dock viewport's own colour shows
    // through the row and draws a patch behind the boxes (SubtiliteCode 02, §14). QFrame with NoFrame stays
    // invisible here, exactly like the row buildEditor wraps around a spanning property.
    auto *frame = new QFrame(parent);
    frame->setFrameStyle(QFrame::NoFrame);
    m_row = frame;
    m_row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto *layout = new QHBoxLayout(m_row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_first = new QCheckBox(m_firstText, m_row);
    m_secondBox = new QCheckBox(m_secondText, m_row);

    // The boxes take their state before they are wired: giving it to them is what makes a checked box differ from the
    // unchecked box it is built as, and a box reporting that change before the row has become the property's editor
    // would ask for a change event on nothing (QCoreApplication::postEvent warns about the null receiver).
    const int state = vproperty_d_ptr->VariantValue.toInt();
    m_first->setCheckState((state & kFirstBox) != 0 ? Qt::Checked : Qt::Unchecked);
    m_secondBox->setCheckState((state & kSecondBox) != 0 ? Qt::Checked : Qt::Unchecked);

    const auto watch = [this](QCheckBox *item) -> void
    {
        connect(item,
                CHECKBOX_STATE_CHANGED,
                this,
                [this]() -> void
                {
                    QCoreApplication::postEvent(vproperty_d_ptr->editor, new VPE::UserChangeEvent());
                });
    };

    watch(m_first);
    watch(m_secondBox);

    layout->addWidget(m_first);
    layout->addWidget(m_secondBox);

    // The two boxes stay side by side on the left: what is left of the line belongs to no box, and a pair of check
    // boxes left to share it would drift apart as the dock grows.
    layout->addStretch(1);

    vproperty_d_ptr->editor = m_row;
    return m_row;
}

//---------------------------------------------------------------------------------------------------------------------
// Same contract as VBoolProperty::data, minus its unchecked qobject_cast on the editor: the editor is the row here,
// not one box, and the value is the pair of boxes, not a single boolean.
auto OCheckBoxPairRowProperty::data(int column, int role) const -> QVariant
{
    if (column == DPC_Data)
    {
        if (Qt::DisplayRole == role)
        {
            return State();
        }

        if (Qt::EditRole == role)
        {
            return vproperty_d_ptr->VariantValue;
        }
    }

    return VPE::VProperty::data(column, role);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxPairRowProperty::setEditorData(QWidget *editor) -> bool
{
    if (m_first == nullptr || editor != m_row)
    {
        return false;
    }

    const int state = vproperty_d_ptr->VariantValue.toInt();

    const QSignalBlocker first(m_first);
    m_first->setCheckState((state & kFirstBox) != 0 ? Qt::Checked : Qt::Unchecked);

    if (m_secondBox != nullptr)
    {
        const QSignalBlocker second(m_secondBox);
        m_secondBox->setCheckState((state & kSecondBox) != 0 ? Qt::Checked : Qt::Unchecked);
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckBoxPairRowProperty::getEditorData(const QWidget *editor) const -> QVariant
{
    if (m_first == nullptr || editor != m_row)
    {
        return {};
    }

    return State();
}
