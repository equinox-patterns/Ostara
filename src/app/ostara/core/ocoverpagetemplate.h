/************************************************************************
 **
 **  @file   ocoverpagetemplate.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  A cover page template — the pattern's informations, laid out once.
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

#ifndef OCOVERPAGETEMPLATE_H
#define OCOVERPAGETEMPLATE_H

#include "otemplatesectionhost.h"

#include <QDomElement>
#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

class QDomDocument;

/**
 * @brief What a pattern's cover page shows, and how — written once, used by every pattern.
 *
 * **The template says WHICH information and HOW it is shown, never WHAT its value is.** The name
 * of the pattern, the sizes it comes in, the stature it is cut for: none of that is here. A
 * template is the shape a cover page takes, and the values are read from the pattern when the
 * page is drawn — which is what lets one template serve a whole library.
 *
 * **One leaf, of a size that is the feature's and not the preview's.** A cover page is 19 × 25.9 cm
 * — always, whatever the pattern. The preview shows it reduced to fit the pane, but the ratio it
 * shows it at is exact, and the export draws that same page at its real size: an information asked
 * for at two centimetres takes two centimetres on the sheet. A page drawn at another scale would
 * not be this pattern's cover page.
 *
 * ⚠️ Those two numbers are still written in the preview alone. They belong here, where both the
 * preview and the export can read them — to be moved before the export exists.
 *
 * **A default-constructed one is what a new template is.** The template shipped with the
 * installation is a *file*, written by OCoverPageTemplateStorage, and is not what this starts
 * from.
 *
 * Serialised as XML against `://schema/coverpage_template/v1.0.0.xsd`, in a `.ocpt` file under
 * ~/Ostara/CoverPageTemplate.
 */
struct OCoverPageTemplate
{
    /**
     * @brief Where an information sits on the page.
     *
     * The page is a skeleton of three stacked bands, and only the middle one is divided. That is
     * the whole of the layout, and it is deliberately the whole of it: this is a cover page, not
     * a magazine, and what it has to cover is a single block, two columns, or both with something
     * above and something below. Every arrangement a pattern needs is one of these four, empty or
     * not — a band nobody fills has no height and is not drawn.
     *
     * The widths are the skeleton's, never the user's: full width, or half of it.
     */
    enum class Placement : int
    {
        Top = 0,
        MiddleLeft = 1,
        MiddleRight = 2,
        Bottom = 3
    };

    /** @brief The four placements, in the order the page stacks them. */
    static auto Placements() -> const QList<Placement> &;

    /**
     * @brief One information the cover page shows, in the order it is laid out.
     *
     * The type id says what it is — « logo » — and the options say how: which logo, at what size.
     * The type itself is never in the file, only referenced, so a catalogue that learns a new
     * field leaves every existing template readable.
     */
    struct Section
    {
        QString typeId;
        QHash<QString, QVariant> options;
    };

    /** What the template is called. The file it is saved in carries the same name. */
    QString name;

    /**
     * The display languages of the block (ISO locale codes, in display order).
     *
     * Never empty: a file that declares none — an older one, or a hand-edited one — falls back to
     * the application's own language rather than to nothing, which is the only reading that leaves
     * the template drawable.
     */
    QStringList languages;

    /**
     * The informations, one ordered list per placement.
     *
     * Four lists rather than one list of placed sections, because the order is not shared: the
     * arrows move an information within its band, and an information that had to travel past the
     * ones of other bands to move one row would make « up » mean something different from what it
     * says. The four keys are always the four placements; a band nobody fills is an empty list,
     * which is not the same thing as a band that is missing.
     */
    QMap<Placement, QList<Section>> bands;

    /** @brief The informations of one placement, in the order they are laid out. */
    auto Sections(Placement placement) const -> const QList<Section> &;

    /** @brief The section at @p index of @p placement — an empty one, static, when there is none. */
    auto SectionAt(Placement placement, int index) const -> const Section &;

    /** @brief Appends an information of the given catalogue type to one band, at its defaults. */
    void AddSection(Placement placement, const QString &typeId);

    void RemoveSection(Placement placement, int index);
    void MoveUp(Placement placement, int index);
    void MoveDown(Placement placement, int index);

    /** @brief This template as an XML element, ready to be appended to a document. */
    auto ToXml(QDomDocument &doc) const -> QDomElement;

    /**
     * @brief Reads a template back off an XML element, and says whether it was one.
     *
     * Every attribute is read over a default-constructed template, so a file written by an older
     * build — or a hand-edited one missing a setting — opens with that setting at its default
     * rather than at nothing. Returns false, and changes nothing, when the element is not a
     * template element at all.
     */
    auto LoadFromXml(const QDomElement &root) -> bool;
};

/**
 * @brief One band of a cover page, presented as a template the shared section editor can edit.
 *
 * The section editor addresses a template as one flat, ordered list of sections. A cover page has
 * four such lists, and which one is being edited depends on what the user selected — so the band
 * is handed over as a template of its own, and the editor never learns that bands exist.
 *
 * **This is the adapter, and it is deliberately the covering page's.** The shared editor is not
 * bent to fit one family: a family that stores its sections differently is the one that adapts.
 *
 * It holds a pointer, not a copy: what the editor writes goes straight into the template, and no
 * copy has to be reconciled afterwards.
 */
class OCoverPageBandHost : public OTemplateSectionHost
{
public:
    OCoverPageBandHost(OCoverPageTemplate *tpl, OCoverPageTemplate::Placement placement);

    /** @brief Points the adapter at another band of the same template. */
    void SetPlacement(OCoverPageTemplate::Placement placement);

    auto Languages() const -> QStringList override;
    auto SectionCount() const -> int override;
    auto SectionTypeId(int index) const -> QString override;
    auto SectionType(const QString &typeId) const -> const OTemplateSection & override;
    auto SectionOptions(int index) const -> QHash<QString, QVariant> override;
    void SetSectionOption(int index, const QString &key, const QVariant &value) override;

private:
    OCoverPageTemplate *m_template{nullptr};
    OCoverPageTemplate::Placement m_placement{OCoverPageTemplate::Placement::Top};
};

#endif // OCOVERPAGETEMPLATE_H
