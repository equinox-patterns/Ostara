/************************************************************************
 **
 **  @file   oformularowproperty.cpp
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

#include "oformularowproperty.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QScopedPointer>
#include <QStyleOptionViewItem>
#include <QToolButton>

#include "vtheme.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vformula.h"
#include "../vpropertyexplorer/vproperty.h"
#include "../vpropertyexplorer/vproperty_p.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"

//---------------------------------------------------------------------------------------------------------------------
OFormulaRowProperty::OFormulaRowProperty(const QString &name)
  : VFormulaProperty(name)
{
    setSpanningRow(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OFormulaRowProperty::getRowCount() const -> vpesizetype
{
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFormulaRowProperty::createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                                       const QAbstractItemDelegate *delegate) -> QWidget *
{
    Q_UNUSED(options)
    Q_UNUSED(delegate)

    m_row = new QWidget(parent);
    auto *layout = new QHBoxLayout(m_row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);

    m_field = new QLineEdit(m_row);
    m_field->setLocale(parent->locale());
    m_field->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_field->setText(GetFormula().GetFormula(FormulaType::ToUser));
    connect(m_field, &QLineEdit::editingFinished, this, [this]() { Commit(); });

    auto *fxButton = new QToolButton(m_row);
    fxButton->setText(QStringLiteral("..."));
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    fxButton->setIcon(QIcon(QStringLiteral(":/icons/%1/function_%1.svg")
                                .arg(isDark ? QStringLiteral("dark") : QStringLiteral("light"))));
    fxButton->setIconSize(QSize(16, 16));
    fxButton->setFixedSize(24, 24);
    connect(fxButton, &QToolButton::clicked, this, [this]() { OpenFormulaDialog(); });

    layout->addWidget(m_field, 1);
    layout->addWidget(fxButton);

    // SetFormula() writes into the row through this pointer the value the tool refused.
    vproperty_d_ptr->editor = m_row;
    return m_row;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFormulaRowProperty::setEditorData(QWidget *editor) -> bool
{
    if (m_field == nullptr || editor != m_row)
    {
        return false;
    }

    // The dock refreshes its rows between two keystrokes: never write over the field being typed in. A
    // value the tool refused comes back through RevertTo(), which is not bound by that rule.
    if (!m_field->hasFocus())
    {
        m_field->setText(GetFormula().GetFormula(FormulaType::ToUser));
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFormulaRowProperty::getEditorData(const QWidget *editor) const -> QVariant
{
    if (m_field == nullptr || editor != m_row)
    {
        return {};
    }

    // The field is read as it was written: the form compares this with the value the property holds, and
    // that difference is what makes the dock hand the edit over to the tool.
    VFormula formula = GetFormula();
    formula.SetFormula(m_field->text(), FormulaType::FromUser);
    formula.Eval();

    QVariant value;
    value.setValue(formula);
    return value;
}

//---------------------------------------------------------------------------------------------------------------------
void OFormulaRowProperty::RevertTo(const VFormula &formula)
{
    SetFormula(formula); // the row goes back to what the tool holds

    if (m_field != nullptr)
    {
        m_field->setText(formula.GetFormula(FormulaType::ToUser));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OFormulaRowProperty::Commit()
{
    if (m_row != nullptr)
    {
        QCoreApplication::postEvent(m_row, new VPE::UserChangeEvent());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OFormulaRowProperty::OpenFormulaDialog()
{
    VFormula formula = GetFormula();

    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(
        formula.getData(), formula.getToolId(), VAbstractValApplication::VApp()->getMainWindow()));
    dialog->setCheckZero(formula.getCheckZero());
    dialog->setPostfix(formula.getPostfix());
    dialog->SetFormula(formula.GetFormula(FormulaType::FromUser));

    if (dialog->exec() != QDialog::Accepted)
    {
        return;
    }

    // Same conversion as the fx button of VFormulaPropertyEditor. The result goes into the field and not
    // into the property: it is the difference between the two that carries the edit to the tool.
    formula.SetFormula(dialog->GetFormula(), FormulaType::ToUser);
    formula.Eval();

    if (m_field != nullptr)
    {
        m_field->setText(formula.GetFormula(FormulaType::ToUser));
    }
    Commit();
}
