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
 *
 * **The settings stand in a frame titled with the information's own name**, which is what tells
 * the user what they are editing: a form with no frame around it reads as the continuation of the
 * list above it, whatever the colour behind it.
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

    /**
     * @brief Tells every composite control that the options under it have changed.
     *
     * **A host asks for this when what a control shows depends on what another one wrote** — the
     * axes a cover page offers depend on the axis its range grades along. It rebuilds those
     * controls, so it is not for every keystroke: the editor calls it only when the answer to one
     * of those cross-questions has actually changed.
     */
    void RefreshCompositeEditors();

    /**
     * @brief The options of the information being edited, as they stand.
     *
     * So a host can tell whether one of them moved without keeping a copy of its own — a copy that
     * would be stale the moment the user typed.
     */
    Q_REQUIRED_RESULT auto CurrentOptions() const -> QHash<QString, QVariant>;

signals:
    /// Fired when any option changes (the editor marks the template modified).
    void OptionsChanged();

private:
    void Rebuild();

    /**
     * @brief Puts a field's control where the field says it goes.
     *
     * On the line it shares with its neighbours when it names one, and on a row of its own under
     * its own label otherwise. The first field to name a line opens it and gives it its words.
     */
    void PlaceControl(const OTemplateOptionField &field, QWidget *control);

    /// Notes a control the section's own options make usable, or unusable, as a whole.
    void RegisterEnabler(const OTemplateOptionField &field, QWidget *control);

    /**
     * @brief Notes a control that has to be shown or hidden with another field's value.
     *
     * @param ownRow true when the control fills its row whole — a block, or a field that takes the
     *        row without a caption. The form then has no field to hide, so the control is hidden
     *        itself.
     */
    void RegisterConditional(const OTemplateOptionField &field, QWidget *control, bool ownRow);

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
        /// The control fills its row whole, so there is no field role to hide — hide the control.
        bool ownRow{false};
    };

    QVBoxLayout *m_root{nullptr};
    QWidget *m_content{nullptr};
    QFormLayout *m_form{nullptr};
    QList<ConditionalField> m_conditionals;

    /// One field the section's own options make usable or not, and the control that follows.
    struct EnabledField
    {
        QWidget *control{nullptr};
        std::function<bool(const QHash<QString, QVariant> &)> when;
    };
    QList<EnabledField> m_enabled;

    /// The lines the fields share, by the key those fields declare. Rebuilt with the form.
    QHash<QString, QHBoxLayout *> m_rows;

    /// The weight controls, by the key they declare — « fontWeight », « cornerFontWeight ». They
    /// are held so that a family change can refill the list of faces that belongs to it: the faces
    /// are the family's, and a list built once would go on offering the wrong font's.
    QHash<QString, QComboBox *> m_weightCombos;

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
