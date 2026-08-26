/************************************************************************
 **
 **  @file   OPieceCartoucheTemplate.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Cartouche template model — ordered section list + per-section options.
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

#ifndef OPIECECARTOUCHETEMPLATE_H
#define OPIECECARTOUCHETEMPLATE_H

#include "OPieceCartoucheSection.h"

#include <QDomElement>
#include <QList>
#include <QString>
#include <QStringList>

class QDomDocument;

/**
 * @brief Cartouche template — a named, ordered list of section instances.
 *
 * The template is stored globally in ~/Ostara/CartoucheTemplates as a `.oct`
 * file. It carries no geometry: the pattern only stores the template reference,
 * the per-piece dispatch and the free text (spec §4.1 / §4.5).
 */
class OPieceCartoucheTemplate
{
public:
    /// One section *instance* of a template — a type id plus concrete options.
    struct Section
    {
        QString typeId;
        QHash<QString, QVariant> options;
    };

    OPieceCartoucheTemplate() = default;
    explicit OPieceCartoucheTemplate(const QString &name);

    auto Name() const -> QString;
    void SetName(const QString &name);

    /// The block display languages (ISO locale codes, display order).
    auto Languages() const -> QStringList;
    void SetLanguages(const QStringList &languages);
    void AddLanguage(const QString &code);
    void RemoveLanguage(const QString &code);
    auto ContainsLanguage(const QString &code) const -> bool;

    auto SectionCount() const -> int;
    auto SectionTypeId(int index) const -> QString;
    auto SectionAt(int index) const -> const Section &;
    void SetSectionOption(int index, const QString &key, const QVariant &value);

    /// Append a new section of the given catalogue type, initialized with its defaults.
    void AddSection(const QString &typeId);
    void RemoveSection(int index);
    void MoveUp(int index);
    void MoveDown(int index);
    void Clear();

    auto IsEmpty() const -> bool;

    /// Serialize to a `<cartoucheTemplate>` element inside `doc` (does not attach it).
    auto ToXml(QDomDocument &doc) const -> QDomElement;

    /// Parse a `<cartoucheTemplate>` root. Returns false when the root is not a template.
    auto LoadFromXml(const QDomElement &root) -> bool;

private:
    QString m_name;
    QStringList m_languages;
    QList<Section> m_sections;
};

#endif // OPIECECARTOUCHETEMPLATE_H
