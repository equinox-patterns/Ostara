/************************************************************************
 **
 **  @file   otemplateoptioneditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The control a composite option field builds for itself.
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

#ifndef OTEMPLATEOPTIONEDITOR_H
#define OTEMPLATEOPTIONEDITOR_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include <functional>

class OSizeTable;

/**
 * @brief What a composite's control is given: what it reads, and how it writes back.
 *
 * The values are handed over as a map rather than asked for one at a time: a composite keeps
 * several of them under keys of its own choosing, and a control that had to ask which would need
 * to know the section as well as the editor does.
 */
struct OTemplateOptionContext
{
    /// The section's options as they stand — the composite's own keys are among them.
    QHash<QString, QVariant> options;

    /// Which information is being edited, for a composite that is not the same on all of them.
    QString sectionTypeId;

    /// What the field is called on screen, for a control that carries its own title.
    QString label;

    /// The languages the template is written in, in display order.
    QStringList languages;

    /// The sizes of the pattern, when a table is loaded — null when there is none.
    const OSizeTable *table{nullptr};

    /// Writes one value of the section back. Empty while a control is only being refreshed.
    std::function<void(const QString &key, const QVariant &value)> apply;
};

/**
 * @brief The control a composite option field builds for itself.
 *
 * **This is what keeps a family's composites out of the shared editor.** A composite is not a
 * value: it is several, under keys only the catalogue that declared it knows, drawn by a control
 * only that family has. So the field carries the factory, and the shared editor builds what it is
 * handed, puts it where the field says, and asks it to refresh when something else has changed the
 * options under it — not knowing, at any point, that a cut or an axis rename exists.
 */
class OTemplateOptionEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OTemplateOptionEditor(QWidget *parent = nullptr);
    ~OTemplateOptionEditor() override;

    OTemplateOptionEditor(const OTemplateOptionEditor &) = delete;
    auto operator=(const OTemplateOptionEditor &) -> OTemplateOptionEditor & = delete;

    /// Re-reads the section's options. Called when something outside this control changed them.
    virtual void Refresh(const OTemplateOptionContext &context) = 0;
};

#endif // OTEMPLATEOPTIONEDITOR_H
