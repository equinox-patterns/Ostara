/************************************************************************
 **
 **  @file   vformulaproperty.h
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

#ifndef VFORMULAPROPERTY_H
#define VFORMULAPROPERTY_H

#include <QScopedPointer>

#include "../vpropertyexplorer/vproperty.h"

class VContainer;

/** @brief The container of the pattern itself, or a null pointer when no pattern is open.
 *
 *  A property's formula comes with the container its tool holds, and that is a *copy*: a parse clears the variables
 *  it rebuilds — the lines among them — and clearing one detaches the pattern's table from every copy. A name is
 *  judged against the pattern's own container; a value is computed against the one the formula came with, which is
 *  the only one that knows what a tool lends. */
auto PatternData() -> QScopedPointer<VContainer>;

class VFormula;

class VFormulaProperty : public VPE::VProperty
{
    Q_OBJECT // NOLINT
public:
    explicit VFormulaProperty(const QString &name);

    //! Get the data how it should be displayed
    auto data (int column = DPC_Name, int role = Qt::DisplayRole) const -> QVariant override;

    //! Returns item flags
    auto flags(int column = DPC_Name) const -> Qt::ItemFlags override;

    //! Returns an editor widget, or NULL if it doesn't supply one
    //! \param parent The widget to which the editor will be added as a child
    //! \options Render options
    //! \delegate A pointer to the QAbstractItemDelegate requesting the editor. This can be used to connect signals and
    //! slots.
    auto createEditor(QWidget* parent, const QStyleOptionViewItem& options,
                      const QAbstractItemDelegate* delegate) -> QWidget* override;

    //! Sets the property's data to the editor (returns false, if the standard delegate should do that)
    auto setEditorData(QWidget* editor) -> bool override;

    //! Gets the data from the widget
    auto getEditorData(const QWidget* editor) const -> QVariant override;

    //! Returns the preset-angle buttons row rendered below this property's children.
    auto bottomWidget() const -> QWidget* override;

    //! Returns a string containing the type of the property
    auto type() const -> QString override;

    //! Clones this property
    //! \param include_children Indicates whether to also clone the children
    //! \param container If a property is being passed here, no new VProperty is being created but instead it is tried
    //! to fill all the data into container. This can also be used when subclassing this function.
    //! \return Returns the newly created property (or container, if it was not NULL)
    Q_REQUIRED_RESULT auto clone(bool include_children = true,
                                 VProperty* container = nullptr) const -> VProperty* override;

    //! Sets the value of the property
    void setValue(const QVariant& value) override;

    //! Returns the value of the property as a QVariant
    auto getValue() const -> QVariant override;

    //! Returns the formula
    auto GetFormula() const -> VFormula;

    //! Sets the formula
    void SetFormula(const VFormula &formula);

public slots:
    void ValueChildChanged(const QVariant &value, int typeForParent) override;

private:
    /** @brief True while a question about the names a formula is missing is pending or being asked.
     *
     *  One gesture produces several commits — the field, the form, the row — and each of them refuses the same
     *  formula: without this, the question is asked once per commit, and the dialogues open on top of one another. */
    bool m_askingForNames{false};

    /** @brief Write @a typed into the tool, asking for a name first when @a askForNames.
     *
     *  The write is what refreshes the field, so it happens for every commit; the question, for the first of them
     *  only. Called after the form's commit has returned, never from inside it: see ValueChildChanged. */
    void NameObjectsAndCommit(const QString &typed, bool askForNames);
};

#endif // VFORMULAPROPERTY_H
