/************************************************************************
 **
 **  @file   vformulaproperty.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   28 8, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#include <QtDebug>

#include "vformulaproperty.h"
#include "vformulapropertyeditor.h"

#include <QApplication>
#include <QCoreApplication>
#include <QTimer>

#include "../vmisc/vabstractapplication.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vpropertyexplorer/plugins/vstringproperty.h"
#include "../vpropertyexplorer/vproperty_p.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "vformulapropertyeditor.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The container of the pattern itself, or a null pointer when no pattern is open.
 *
 *  A property's formula comes with the container its tool holds, and that is a *copy*. A parse clears the variables
 *  it rebuilds — the lines among them — and clearing one detaches the pattern's table from every copy: a copy goes
 *  on answering for what was never cleared, a measurement, and answers nothing for a line. A formula judged against
 *  it then names nothing, and nothing can be asked about what it names. The pattern's own container is what has to
 *  be read; this is a copy of it, taken for the length of one judgement and thrown away with it. */
auto PatternData() -> QScopedPointer<VContainer>
{
    VAbstractPattern *const doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return QScopedPointer<VContainer>();
    }

    return QScopedPointer<VContainer>(new VContainer(doc->GetCompleteData()));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VFormulaProperty::VFormulaProperty(const QString &name)
  : VProperty(name,
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              static_cast<QMetaType::Type>(VFormula::FormulaTypeId()))
#else
              static_cast<QVariant::Type>(VFormula::FormulaTypeId()))
#endif
{
    vproperty_d_ptr->type = VPE::Property::Complex;

    auto *tmpFormula = new VPE::VStringProperty(tr("Formula"));
    addChild(tmpFormula);
    tmpFormula->setUpdateBehaviour(true, false);
    tmpFormula->setOsSeparator(VAbstractApplication::VApp()->Settings()->GetOsSeparator());

    // This row *is* the formula the user reads, and the field is the one place a write does not reach: a formula
    // whose object has just been named is written into the property, its child and the tool, and the field would
    // keep the old name until the panel is rebuilt. The form shows this row back its value once committed.
    QMap<QString, QVariant> settings = tmpFormula->getSettings();
    settings.insert(QStringLiteral("RefreshEditor"), true);
    tmpFormula->setSettings(settings);

    // Cannot use virtual function setValue in constructor
    SetFormula(QVariant(0).value<VFormula>());
}

//---------------------------------------------------------------------------------------------------------------------
//! Get the data how it should be displayed
auto VFormulaProperty::data(int column, int role) const -> QVariant
{
    if (column == DPC_Data && (Qt::DisplayRole == role || Qt::EditRole == role))
    {
        return getValue();
    }

    return VProperty::data(column, role);
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaProperty::flags(int column) const -> Qt::ItemFlags
{
    if (column == DPC_Name || column == DPC_Data)
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    return Qt::NoItemFlags;
}

//---------------------------------------------------------------------------------------------------------------------
//! Returns an editor widget, or NULL if it doesn't supply one
auto VFormulaProperty::createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                                    const QAbstractItemDelegate *delegate) -> QWidget *
{
    Q_UNUSED(options)
    Q_UNUSED(delegate)

    auto formula = VProperty::vproperty_d_ptr->VariantValue.value<VFormula>();
    auto *tmpEditor = new VFormulaPropertyEditor(parent);
    tmpEditor->setLocale(parent->locale());
    tmpEditor->SetFormula(formula);
    VProperty::vproperty_d_ptr->editor = tmpEditor;
    return VProperty::vproperty_d_ptr->editor;
}

//---------------------------------------------------------------------------------------------------------------------
//! Sets the property's data to the editor (returns false, if the standard delegate should do that)
auto VFormulaProperty::setEditorData(QWidget *editor) -> bool
{
    if (auto *tmpWidget = qobject_cast<VFormulaPropertyEditor *>(editor); tmpWidget != nullptr)
    {
        auto formula = VProperty::vproperty_d_ptr->VariantValue.value<VFormula>();
        tmpWidget->SetFormula(formula);
    }
    else
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
//! Gets the data from the widget
auto VFormulaProperty::getEditorData(const QWidget *editor) const -> QVariant
{

    if (const auto *tmpWidget = qobject_cast<const VFormulaPropertyEditor *>(editor); tmpWidget != nullptr)
    {
        QVariant value;
        value.setValue(tmpWidget->GetFormula());
        return value;
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaProperty::bottomWidget() const -> QWidget *
{
    if (const auto *editor = qobject_cast<const VFormulaPropertyEditor *>(VProperty::vproperty_d_ptr->editor);
        editor != nullptr)
    {
        return editor->presetRow();
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaProperty::type() const -> QString
{
    return QStringLiteral("formula");
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaProperty::clone(bool include_children, VProperty *container) const -> VPE::VProperty *
{
    if (!container)
    {
        container = new VFormulaProperty(getName());

        if (!include_children)
        {
            const QList<VProperty *> tmpChildren = container->getChildren();
            for (auto *tmpChild : tmpChildren)
            {
                container->removeChild(tmpChild);
                delete tmpChild;
            }
        }
    }

    return VProperty::clone(false, container); // Child
}

//---------------------------------------------------------------------------------------------------------------------
void VFormulaProperty::setValue(const QVariant &value)
{
    auto tmpFormula = value.value<VFormula>();
    SetFormula(tmpFormula);
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaProperty::getValue() const -> QVariant
{
    VFormula const tmpFormula = GetFormula();
    QVariant value;
    value.setValue(tmpFormula);
    return value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaProperty::GetFormula() const -> VFormula
{
    return VProperty::vproperty_d_ptr->VariantValue.value<VFormula>();
}

//---------------------------------------------------------------------------------------------------------------------
void VFormulaProperty::SetFormula(const VFormula &formula)
{
    if (vproperty_d_ptr->Children.count() < 1)
    {
        return;
    }

    QVariant value;
    value.setValue(formula);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    value.convert(QMetaType(VFormula::FormulaTypeId()));
#else
    value.convert(VFormula::FormulaTypeId());
#endif
    VProperty::vproperty_d_ptr->VariantValue = value;

    QVariant tmpFormula(formula.GetFormula());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    tmpFormula.convert(QMetaType(QMetaType::QString));
#else
    tmpFormula.convert(QVariant::String);
#endif

    VProperty::vproperty_d_ptr->Children.at(0)->setValue(tmpFormula);

    if (VProperty::vproperty_d_ptr->editor != nullptr)
    {
        setEditorData(VProperty::vproperty_d_ptr->editor); // NOLINT(clang-analyzer-optin.cplusplus.VirtualCall)
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VFormulaProperty::ValueChildChanged(const QVariant &value, int typeForParent)
{
    Q_UNUSED(typeForParent)
    VFormula newFormula = GetFormula();


    newFormula.SetFormula(value.toString(), FormulaType::FromUser);

    // The *names* are judged against the pattern's own container, not the copy the tool holds: a line cleared and
    // rebuilt by the last parse is in one and not in the other. The *value*, though, is computed against the
    // container the formula came with, and only against that one: the values a tool lends — `CurrentLength`, the
    // length being cut, the seam allowance being drawn — live in the tool's container and nowhere else. Judging
    // against the pattern is what lets a line be recognised; computing against it is what would make
    // `CurrentLength/2` unreadable, and the field look read-only.
    const QScopedPointer<VContainer> patternData = PatternData();
    const VContainer *const judgement = patternData.isNull() ? newFormula.getData() : patternData.data();

    // The field writes in whatever notation the formula is *displayed* in, the container answers to the
    // calculator's own, and the two have to meet before anything can be judged: a name is internal. `InternalFormula`
    // is the one place that knows both, so a name typed the way it is read is understood here exactly as it is in
    // the assistant.
    const QString internal = InternalFormula(value.toString(), judgement);

    // A formula still written with the handle an object had before somebody named it is not refused for that: the
    // name is known, so it is put right here, and what the tool receives is the name the object has now.
    newFormula.SetFormula(VContainer::FormulaWithoutStaleHandles(judgement, internal), FormulaType::FromSystem);
    newFormula.Eval();


    // A formula written here names an object that has no name: refused, and the property keeps the formula the
    // tool holds — the same answer it gives a formula the parser cannot read, which `Eval` has just marked.
    // The field is left showing what was typed, so the sentence below it can be read.
    // Asked of the pattern's container, the one that knows the lines: the copy the tool holds answers for the
    // values it lends and for nothing else, which is exactly why the same formula was named in one place and
    // not in the other.
    if (not VContainer::UnnamedObjectInFormula(judgement, newFormula.GetFormula(FormulaType::FromSystem)).isEmpty())
    {
        // Kept in the log only. It is a warning no longer: the names are asked for below, so there is nothing for
        // the user to read here — and a `qWarning` in this application is not a line of text, it is a modal box
        // (the message handler turns warnings into dialogs). Two dialogs for one gesture, one of them useless.
        qDebug() << "Formula refused, names to be asked for:" << newFormula.Reason();


        // Refused, but not a dead end: the objects the formula uses can be given a name, and once they have one the
        // formula the user wrote is written where they meant it to go. Asked *after* this returns, never from here:
        // a modal opened inside the form's commit starts a nested event loop over a widget half-way through
        // committing — which is exactly what crashed, `VPropertyFormWidget::commitData` -> `childChanged` ->
        // `QDialog::exec` -> `QWindow::handle()` on a window already being torn down.
        // One gesture, one question: the same formula reaches this function from the field, from the form and from
        // the row, and each commit refuses it. The first asks; the others go straight to the write below, because
        // the write — and the event that carries it to the tool — is what refreshes the field. Skipping them
        // altogether, as an outright lock did, leaves the old name on screen until the panel is reopened.
        const bool askForNames = not m_askingForNames;
        if (askForNames)
        {
            m_askingForNames = true;
        }

        const QString typed = value.toString();
        QTimer::singleShot(0, this,
                           [this, typed, askForNames]()
                           {
                               NameObjectsAndCommit(typed, askForNames);
                               if (askForNames)
                               {
                                   m_askingForNames = false;
                               }
                           });

                return;
    }

    SetFormula(newFormula);
}

//---------------------------------------------------------------------------------------------------------------------
void VFormulaProperty::NameObjectsAndCommit(const QString &typed, bool askForNames)
{

    VFormula formula = GetFormula();
    const VContainer *const data = formula.getData();
    if (data == nullptr)
    {
        return;
    }

    // Same container as the commit that asked for these names: the pattern's own, where a line cleared and rebuilt
    // by the last parse really is. The copy the tool holds would answer nothing about it.
    const QScopedPointer<VContainer> patternData = PatternData();
    const VContainer *const judgement = patternData.isNull() ? data : patternData.data();

    // What was typed, in the internal notation the container answers to: the names asked for here are names, and a
    // name is internal. Same reading as the commit above, so both agree on what the formula says.
    QString internal = InternalFormula(typed, judgement);

    // Nothing is written before the last answer, and cancelling writes nothing at all: the pattern is untouched and
    // the field stays on the formula the tool holds. Asked once per gesture — the later commits of the same edit go
    // straight to the write.
    if (askForNames
        && not NameUnnamedObjects(VAbstractValApplication::VApp()->getMainWindow(), judgement, internal))
    {
        return;
    }

    // What the fx button does with the formula it is given: the difference between the field and what the tool
    // holds is what carries the edit to the tool — and what refreshes the field, which is what the user watches.
    formula.SetFormula(internal, FormulaType::FromSystem);
    formula.Eval();

    SetFormula(formula);

    if (vproperty_d_ptr->editor != nullptr)
    {
        QCoreApplication::postEvent(vproperty_d_ptr->editor, new VPE::UserChangeEvent());
    }

    // The field has kept the focus the whole time — the dialogue was opened on top of it — and the panel only reads
    // a field back when it lets the focus go: the form commits on focus-out. The focus is therefore given up here,
    // exactly as clicking beside the field does it, which is the gesture the user should not have to make to see
    // the name they have just given.
    if (QWidget *const focused = QApplication::focusWidget(); focused != nullptr)
    {
        focused->clearFocus();
    }
}
