/************************************************************************
 **
 **  @file   ocheckboxpairrowproperty.h
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

#ifndef OCHECKBOXPAIRROWPROPERTY_H
#define OCHECKBOXPAIRROWPROPERTY_H

#include "../vpropertyexplorer/vproperty.h"

class QCheckBox;
class QWidget;

//! Two booleans on a single full-width line: the boxes, then their texts, kept out of the form's label column
//! (VProperty::setSpanningRow).
//!
//! The two are read together — a pleat's cap is drawn, and its sense may be reversed — and one row per box pushed the
//! second one a line lower, where it no longer read as the other half of the first.
//!
//! The value is the pair itself, one bit per box, and that is why this row is not the boolean row of
//! OCheckBoxRowProperty: the browser only submits a change it can read in the value
//! (VPropertyFormWidget::commitData), so a value carrying the first box alone would leave every click on the second
//! one without the least effect.
class OCheckBoxPairRowProperty : public VPE::VProperty
{
    Q_OBJECT // NOLINT

public:
    OCheckBoxPairRowProperty(const QString &firstText, const QString &secondText);

    /** @brief The two states the row shows, given together: before the row exists, and whenever it is reloaded. */
    void SetState(bool first, bool second);

    auto FirstChecked() const -> bool;
    auto SecondChecked() const -> bool;

    auto createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                      const QAbstractItemDelegate *delegate) -> QWidget * override;
    auto data(int column, int role) const -> QVariant override;
    auto setEditorData(QWidget *editor) -> bool override;
    auto getEditorData(const QWidget *editor) const -> QVariant override;

private:
    Q_DISABLE_COPY_MOVE(OCheckBoxPairRowProperty) // NOLINT

    /** One bit per box, in the order they are drawn. */
    static constexpr int kFirstBox = 1;
    static constexpr int kSecondBox = 2;

    /** @brief The two boxes as the value carries them: what a box shows when it is there, the stored bit otherwise. */
    auto State() const -> int;

    QString m_firstText;
    QString m_secondText;

    QWidget *m_row{nullptr};
    QCheckBox *m_first{nullptr};
    QCheckBox *m_secondBox{nullptr};
};

#endif // OCHECKBOXPAIRROWPROPERTY_H
