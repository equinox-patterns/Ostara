/************************************************************************
 **
 **  @file   otemplatesectionhost.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  What a section editor needs of the template it edits — and nothing more.
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

#ifndef OTEMPLATESECTIONHOST_H
#define OTEMPLATESECTIONHOST_H

#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

class OTemplateSection;

/**
 * @brief What a section editor needs of the template it edits, and nothing more.
 *
 * The editor builds one control per declared field and writes back what the user changes. It has
 * no business knowing what a template *is*: a cartouche of pieces and a pattern's cover page keep
 * their sections in different objects, with different files and different folders, and neither
 * should have to be the other for one editor to serve both.
 *
 * So it depends on this instead — the three questions it actually asks. Whatever can answer them
 * can be edited by it, and a third family costs three methods rather than a second editor. This is
 * the whole reason the editor is data-driven: the schema says what to draw, and this says where to
 * put it.
 *
 * **It is deliberately narrow.** Every method added here is a demand made of every family at once,
 * including the ones that will never use it — so it carries the section list and nothing that only
 * some templates have.
 */
class OTemplateSectionHost
{
public:
    OTemplateSectionHost() = default;
    virtual ~OTemplateSectionHost() = default;

    OTemplateSectionHost(const OTemplateSectionHost &) = default;
    auto operator=(const OTemplateSectionHost &) -> OTemplateSectionHost & = default;

    /**
     * @brief The languages the template is written in, in display order — never empty.
     *
     * A section that renames something per language builds one tab per entry here, so the editor
     * has to know them. Every family has them: what a cover page says, it says in some language,
     * and so does a cartouche.
     */
    virtual auto Languages() const -> QStringList = 0;

    /** @brief How many sections the template carries. */
    virtual auto SectionCount() const -> int = 0;

    /** @brief The type id of the section at @p index — empty when there is none there. */
    virtual auto SectionTypeId(int index) const -> QString = 0;

    /**
     * @brief The catalogue entry behind a type id — the schema of the section being edited.
     *
     * The editor draws one control per declared field and shows the entry's description, so it
     * needs the entry itself and not only its id. **Asking the family for it is the point**: two
     * catalogues may both hold an information called "free text" and mean different things by it,
     * and the one that describes the section in hand is the one from the catalogue that declared
     * it. An unknown id answers an empty entry rather than a reference to nothing.
     */
    virtual auto SectionType(const QString &typeId) const -> const OTemplateSection & = 0;

    /**
     * @brief The options the section at @p index currently holds — empty when there is none there.
     *
     * Read whole rather than one key at a time: the editor fills a form from them, and a form's
     * worth of keys asked for one by one would be the same map taken apart and put back together.
     */
    virtual auto SectionOptions(int index) const -> QHash<QString, QVariant> = 0;

    /** @brief Writes one option of one section. An index or a key that is not there is ignored. */
    virtual void SetSectionOption(int index, const QString &key, const QVariant &value) = 0;
};

#endif // OTEMPLATESECTIONHOST_H
