/************************************************************************
 **
 **  @file   otemplatesectioneditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Data-driven edit zone for one template section.
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

#ifndef OTEMPLATESECTIONEDITOR_H
#define OTEMPLATESECTIONEDITOR_H

#include "core/otemplatesection.h"
#include "core/otemplatesectionhost.h"

#include <QFormLayout>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

class OSizeTable;
class OTemplateOptionEditor;
struct OTemplateOptionContext;

/**
 * @brief Edit zone for the currently selected template section.
 *
 * The SOLID core: the widget does not know the section types — it asks the family for the
 * catalogue entry of the section it is editing, then draws one control per field that entry
 * declares (Open/Closed: a new section type does not change this widget). It also shows the
 * entry's description.
 *
 * A family answers through OTemplateSectionHost, so the same information can mean different
 * things in two families without this widget ever having to choose between them.
 */
class OTemplateSectionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OTemplateSectionEditor(QWidget *parent = nullptr);

    /// Edit section `index` of `tpl` (lifetime owned by the caller). -1 clears.
    /// `table` gives read access to the loaded .omt for the table-driven controls.
    void SetSection(OTemplateSectionHost *tpl, int index, const OSizeTable *table);

    /// Re-translate the dynamic content (called on QEvent::LanguageChange).
    void RetranslateUI();

signals:
    /// Fired when any option changes (the editor marks the template modified).
    void OptionsChanged();

private:
    void Rebuild();

    /// Tells every composite control that the options under it have changed.
    void RefreshCompositeEditors();

    /// What a composite control is given: the options, the languages, the table, and how to write.
    auto OptionContext() -> OTemplateOptionContext;

    /// Shows or hides the fields whose schema makes them depend on another field's value.
    void UpdateFieldVisibility();

    /// One field the schema shows conditionally, and the control that has to go with it.
    struct ConditionalField
    {
        QWidget *control{nullptr};
        QString whenKey;
        QStringList whenValues;
    };

    QVBoxLayout *m_root{nullptr};
    QWidget *m_content{nullptr};
    QFormLayout *m_form{nullptr};
    QList<ConditionalField> m_conditionals;
    OTemplateSectionHost *m_template{nullptr};
    const OSizeTable *m_table{nullptr};
    int m_index{-1};

    /**
     * The controls the section's composite fields built for themselves — one per composite, in the
     * order they were declared. This editor does not know what any of them is; it only knows that
     * they may need to be told when the options change under them.
     */
    QList<OTemplateOptionEditor *> m_composites;
};

#endif // OTEMPLATESECTIONEDITOR_H
