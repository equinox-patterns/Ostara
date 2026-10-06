/************************************************************************
 **
 **  @file   ogroupboxproperty.h
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

#ifndef OGROUPBOXPROPERTY_H
#define OGROUPBOXPROPERTY_H

#include "../vpropertyexplorer/plugins/vboolproperty.h"

class QGroupBox;

//! A group of rows drawn as a titled frame — a QGroupBox, the frame a dialog groups its options in — with its
//! children as the rows inside it (VProperty::setGroupBox).
//!
//! The dock's own groups are collapsible headers, which read as switches to fold and unfold rather than as the
//! groups of a dialog; this one is the frame itself, with the title on its border.
//!
//! A **checkable** frame is the group's value: the title carries the box, exactly like the checkable group of a
//! dialog, and what it means (show the whole figure, for instance) is what its owner reads and writes.
class OGroupBoxProperty : public VPE::VBoolProperty
{
    Q_OBJECT // NOLINT

public:
    OGroupBoxProperty(const QString &title, bool checkable);

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                      const QAbstractItemDelegate *delegate) -> QWidget * override;
    auto data(int column, int role) const -> QVariant override;
    auto setEditorData(QWidget *editor) -> bool override;
    auto getEditorData(const QWidget *editor) const -> QVariant override;

private:
    Q_DISABLE_COPY_MOVE(OGroupBoxProperty) // NOLINT

    bool m_checkable{false};
    QGroupBox *m_box{nullptr};
};

#endif // OGROUPBOXPROPERTY_H
