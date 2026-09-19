/************************************************************************
 **
 **  @file   otemplatesection.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  One section type of a template catalogue, and its declarative option schema.
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

#ifndef OTEMPLATESECTION_H
#define OTEMPLATESECTION_H

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <functional>

class QWidget;
class OTemplateOptionEditor;
struct OTemplateOptionContext;

/**
 * @brief Kind of an option field — drives both serialisation and the generated editing control.
 *
 * **A shared vocabulary, not a cartouche's.** A catalogue draws the kinds it needs and leaves the
 * others alone; what the kinds have in common is that each one is a way of writing and editing a
 * value, and none of them belongs to a family. The ones a family never declares cost it nothing:
 * an editor builds a control per *field*, and a field that is not declared is not built.
 *
 * The editor is data-driven: it reads the fields of the selected section type and builds one
 * control per field. Adding a kind means adding an entry here, a control builder in the editor,
 * a branch in the serialisation, and one field that declares it — five places, not the two the
 * comment here used to claim.
 */
enum class OTemplateOptionKind
{
    FontFamily,   ///< QString — font family
    FontWeight,   ///< int — QFont::Weight (0 = default)
    FontSize,     ///< int — font size in px (0 = default)
    LineSpacing,  ///< int — extra vertical space in px with the next line (0 = default, negative = tighter)
    Axis,         ///< QString — size-axis name from the loaded table
    MeasureName,  ///< QString — measurement full name from the loaded table
    Separator,    ///< QString — separator between name and value ("", ":", "-", "/", "·")
    NumberStyle,  ///< QString — piece-number padding: "1" or "01"
    Logo,         ///< QString — logo file name from the logo library
    LogoSize,     ///< double — logo size in cm
    Cut,          ///< composite — declared by the cartouche: its sub-options are in the field
    Multilingual, ///< bool — write this information once per block language
    Boolean,      ///< bool — a plain yes-or-no the information decides for itself
    AxisRename,   ///< composite — declared by the cartouche: its sub-options are in the field
    TextSource,   ///< QString — where a text comes from: written here, or filled in per pattern
    Text,         ///< QString — a text written here, possibly on several lines
    SizeCase      ///< QString — which case a size information is written for: one size, or a range
};

/**
 * @brief The two places a text can come from, for a field of kind TextSource.
 *
 * The kind owns its values, as FontWeight owns its weights: what the two are is a property of the
 * question being asked, not of the family that asks it. A catalogue that wants the choice reads
 * them here, and so does anything that has to read the answer back — a preview deciding whether
 * the words are there to be drawn or still to be filled in.
 */
namespace OTemplateTextSource
{
/// The text is written with the information, and is the same wherever that information is used.
inline const QString kHere = QStringLiteral("here");
/// The text is not written here: it is filled in once per pattern, later.
inline const QString kPerPattern = QStringLiteral("perPattern");
} // namespace OTemplateTextSource

/**
 * @brief The two cases a size information can be written for, for a field of kind SizeCase.
 *
 * Same reasoning as @ref OTemplateTextSource: what the two are is a property of the question being
 * asked — *did this output come out as one size, or as a range?* — and not of the family that asks
 * it. A catalogue that wants the choice reads them here, and so does anything that has to read the
 * answer back: a resolver deciding whether the page states one size or lists the sizes it holds.
 *
 * The answer is not the user's to invent — it follows from how the output was generated — but it is
 * the user's to look at, which is why the field exists at all.
 */
namespace OTemplateSizeCase
{
/// The output carries one size, so the information states it and lists nothing.
inline const QString kSingle = QStringLiteral("single");
/// The output carries a range, so the information lists the sizes the page holds.
inline const QString kMulti = QStringLiteral("multi");
} // namespace OTemplateSizeCase

/**
 * @brief Declarative description of one option of a section type.
 */
struct OTemplateOptionField
{
    QString key;
    OTemplateOptionKind kind{OTemplateOptionKind::FontFamily};
    QString label;
    QStringList values{};     ///< candidate values for the editor (e.g. separators); empty = kind default
    QVariant defaultValue{};  ///< per-field default value; invalid = kind default

    /**
     * For a composite field: the sub-options it expands into, and their defaults.
     *
     * **This is what keeps a family's composites out of the shared code.** A composite does not
     * store one value under its own key — it stores several under keys of its own choosing, and
     * only the catalogue that declares it knows which. Saying so here rather than switching on the
     * kind means the shared section expands a composite because it was told to, not because it
     * recognises one.
     *
     * Empty for a plain field, which contributes its own key.
     */
    QHash<QString, QVariant> compositeDefaults{};

    /**
     * Show this field only while another field of the same section holds one of these values.
     *
     * Empty means always shown. Saying it here rather than switching on the kind keeps the shared
     * editor from having to recognise the one kind that needs it: it hides a row because it was
     * told to, exactly as it expands a composite because it was told to. A field that is hidden
     * keeps its value — the user who comes back to it finds what was written.
     */
    QString visibleWhenKey{};
    QStringList visibleWhenValues{};

    /**
     * The line this field shares with its neighbours, and the words at its head.
     *
     * An information whose typeface is three settings does not want three rows of one control
     * each: the fields that name the same line are laid out on it, in the order they are declared,
     * and the first of them may give the line its words. **An empty key means a row of its own,
     * under its own label** — what every field did before this existed, and what a field that is
     * not part of a group keeps doing.
     *
     * Nothing is said about how many fields a line holds or which kinds they are: a line of one is
     * the field's own row, and what stands on a line is what the catalogue put there. Like
     * @ref visibleWhenKey, the shared editor obeys a declaration rather than recognising a kind —
     * it puts a font and a size together because it was told to, not because it knows a typeface.
     */
    QString rowKey{};

    /**
     * The words at the head of that line, read from the first field that opens it.
     *
     * Empty leaves the line unlabelled, which is what a group of controls that name themselves
     * wants: a caption beside a font, a weight and a size would be spelling out what the reader
     * already sees. A field that joined a line without words carries its own label as its tooltip,
     * so what a caption gave permanently is still one hover away.
     */
    QString rowLabel{};

    /**
     * For a composite field: builds the control that edits it.
     *
     * A composite does not store one value under one key — it stores several, and draws them its
     * own way. Only the family that declared it can draw it, so the family provides the factory
     * here and the shared editor builds what it is handed. A composite without one draws nothing:
     * a field whose values several keys hold is one only its own family knows how to show.
     */
    std::function<OTemplateOptionEditor *(QWidget *parent, const OTemplateOptionContext &context)>
        compositeEditor{};

    /**
     * A field that is a block rather than a value takes a row of its own.
     *
     * In the form, a control stands under a caption in a column; a table of aliases or a switch
     * that says how the whole information is written would read as one more value with a label
     * beside it, which is what it is not.
     */
    bool onOwnRow{false};
};

/**
 * @brief One section type of a catalogue.
 *
 * A catalogue entry carries its identity (id, display name, description) and its declarative
 * option schema. A template stores section *instances*, which reference a type by id and hold
 * concrete option values — the entry itself is never in a file.
 */
class OTemplateSection
{
public:
    OTemplateSection() = default;
    OTemplateSection(const QString &id, const QString &sourceName, const QString &displayName,
                     const QString &description, const QList<OTemplateOptionField> &fields);

    /// True when the section carries a non-empty id (i.e. it exists in the catalogue).
    auto IsValid() const -> bool;

    auto Id() const -> QString;
    auto DisplayName() const -> QString;

    /// The untranslated (English) name — lets a preview render the block in another display
    /// language through the app's own translation catalogues.
    auto SourceName() const -> QString;

    auto Description() const -> QString;
    auto Fields() const -> const QList<OTemplateOptionField> &;

    /// True when the option schema contains the given key.
    auto HasField(const QString &key) const -> bool;

    /// Default value for one option key (per its kind).
    auto DefaultValue(const QString &key) const -> QVariant;

    /// Map of every option key → its default value, composites expanded.
    auto DefaultOptions() const -> QHash<QString, QVariant>;

private:
    QString m_id;
    QString m_sourceName;
    QString m_displayName;
    QString m_description;
    QList<OTemplateOptionField> m_fields;
};

#endif // OTEMPLATESECTION_H
