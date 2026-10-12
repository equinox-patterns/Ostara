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

#include <QtDebug>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QScopedPointer>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <QToolButton>

#include "vtheme.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vpropertyexplorer/vproperty.h"
#include "../vpropertyexplorer/vproperty_p.h"
#include "../vtools/dialogs/dialogtoolbox.h"
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

    // Same two roles as the main formula field: the *names* are judged against the pattern's own container, where a
    // line cleared and rebuilt by the last parse really is, and the *value* is computed against the container the
    // formula came with — the only one that knows the values a tool lends (`CurrentLength`, the seam allowance).
    const QScopedPointer<VContainer> patternData = PatternData();
    const VContainer *const judgement = patternData.isNull() ? formula.getData() : patternData.data();

    // A formula still written with the handle an object had before somebody named it is not refused for that: the
    // name is known, so it is put right here, and what the tool receives is the name the object has now.
    formula.SetFormula(
        VContainer::FormulaWithoutStaleHandles(judgement, formula.GetFormula(FormulaType::FromSystem)),
        FormulaType::FromSystem);
    formula.Eval();

    // Refused when it names an object nobody named: the value handed back is the one the tool holds, so the
    // dock has nothing to commit and the tool keeps its formula. The caller shows `Reason()`.
    // Asked of the pattern's container, the one that knows the lines.
    if (not VContainer::UnnamedObjectInFormula(judgement, formula.GetFormula(FormulaType::FromSystem)).isEmpty())
    {
        // Kept in the log only: the names are asked for below, and a `qWarning` in this application is not a line of
        // text but a modal box (the message handler turns warnings into dialogs) — two dialogs for one gesture.
        qDebug() << "Formula refused, names to be asked for:" << formula.Reason();

        // Refused, but not a dead end: the objects can be given a name, and once they have one the formula the
        // user wrote is committed where they meant it to go. Asked after this returns — a modal opened from inside
        // the form's commit is what crashed, `VPropertyFormWidget::commitData` -> `childChanged` -> `QDialog::exec`
        // -> `QWindow::handle()` on a window already being torn down.
        const QString typed = m_field->text();
        if (m_askingForNames)
        {
            return QVariant::fromValue(GetFormula()); // one question is already pending for this gesture
        }
        m_askingForNames = true;

        // The row is asked for a value by a form that reads it as const, and naming an object writes: the pointer
        // is only read again by the event loop, once the commit is over.
        auto *self = const_cast<OFormulaRowProperty *>(this);
        QTimer::singleShot(0, self,
                           [self, typed]()
                           {
                               self->NameObjectsAndCommit(typed);
                               self->m_askingForNames = false;
                           });
        return QVariant::fromValue(GetFormula());
    }

    QVariant value;
    value.setValue(formula);
    return value;
}

//---------------------------------------------------------------------------------------------------------------------
void OFormulaRowProperty::NameObjectsAndCommit(const QString &typed)
{
    VFormula formula = GetFormula();
    const VContainer *const data = formula.getData();
    if (data == nullptr || m_field == nullptr)
    {
        return;
    }

    // The names are read and asked against the pattern's own container, as the commit above judges them.
    const QScopedPointer<VContainer> patternData = PatternData();
    const VContainer *const judgement = patternData.isNull() ? data : patternData.data();

    // What was typed, in the internal notation the container answers to: the names asked for here are names, and a
    // name is internal. Same reading as the commit above, so both agree on what the formula says.
    QString internal = InternalFormula(typed, judgement);

    // Nothing is written before the last answer, and cancelling writes nothing at all: the pattern is untouched and
    // the field stays on the formula the tool holds.
    if (not NameUnnamedObjects(VAbstractValApplication::VApp()->getMainWindow(), judgement, internal))
    {
        return;
    }

    formula.SetFormula(internal, FormulaType::FromSystem);
    formula.Eval();
    SetFormula(formula);
    m_field->setText(formula.GetFormula(FormulaType::ToUser));

    // The field has kept the focus the whole time — the dialogue was opened on top of it — and the row is only read
    // back when it lets the focus go. Given up here, exactly as clicking beside the field does it.
    m_field->clearFocus();

    // The field no longer holds what the tool holds, and that difference is what carries the edit over to it.
    Commit();
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
