/************************************************************************
 **
 **  @file   ocoverpagecatalogue.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The catalogue of what a cover page can show.
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

#include "ocoverpagecatalogue.h"

#include "core/ocovergradedaxis.h"
#include "core/otemplateaxisoptions.h"
#include "core/otemplateseparatoroptions.h"
#include "ocoveraxiseditor.h"
#include "ocovergradedaxiseditor.h"
#include "ocoverseamallowances.h"
#include "ocoverseamtitleeditor.h"
#include "ocoversizerangechooser.h"
#include "ocoversymbolstitleeditor.h"
#include "vmisc/def.h"
#include "widgets/Template/otemplateaxiseditor.h"

#include <QCoreApplication>

namespace
{
// The same translation context the shared section types use: a label like « Logo » means the
// same thing on a cover page as in a cartouche, and translating it twice would be translating
// it twice.
auto T(const char *source) -> QString
{
    return QCoreApplication::translate("OTemplateSection", source);
}

auto MakeSection(const char *id, const char *sourceName, const char *descriptionSource,
                 const QList<OTemplateOptionField> &fields) -> OTemplateSection
{
    return OTemplateSection(QString::fromUtf8(id), QString::fromUtf8(sourceName), T(sourceName),
                            T(descriptionSource), fields);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageCatalogue::Build() -> QList<OTemplateSection>
{
    OTemplateOptionField kFontFamily{QStringLiteral("fontFamily"), OTemplateOptionKind::FontFamily,
                                     T(QT_TRANSLATE_NOOP("OTemplateSection", "Font family"))};
    OTemplateOptionField kFontWeight{QStringLiteral("fontWeight"), OTemplateOptionKind::FontWeight,
                                     T(QT_TRANSLATE_NOOP("OTemplateSection", "Font weight"))};
    OTemplateOptionField kFontSize{QStringLiteral("fontSize"), OTemplateOptionKind::FontSize,
                                   T(QT_TRANSLATE_NOOP("OTemplateSection", "Font size (px)"))};
    const OTemplateOptionField kLineSpacing{QStringLiteral("lineSpacing"),
                                            OTemplateOptionKind::LineSpacing,
                                            T(QT_TRANSLATE_NOOP("OTemplateSection", "Line spacing (px)"))};

    // The typeface is one question asked three ways, so its three answers stand on one line — and
    // that line says nothing at its head: the three controls are the words, and « Font family »
    // written beside them would be naming what the reader already sees. The line spacing is not one
    // of them. It is how the lines sit, not what they are set in, so it keeps a row of its own with
    // the words that tell it apart from a size.
    const QString fontRow = QStringLiteral("font");
    kFontFamily.rowKey = fontRow;
    kFontWeight.rowKey = fontRow;
    kFontSize.rowKey = fontRow;

    // The typography every written information is styled with, kept in one list: an information
    // that says something is written states the same four things about it.
    const QList<OTemplateOptionField> textFields = {kFontFamily, kFontWeight, kFontSize, kLineSpacing};

    const OTemplateOptionField kLogo{QStringLiteral("logo"), OTemplateOptionKind::Logo,
                                     T(QT_TRANSLATE_NOOP("OTemplateSection", "Logo"))};
    const OTemplateOptionField kLogoSize{QStringLiteral("logoSize"), OTemplateOptionKind::LogoSize,
                                         T(QT_TRANSLATE_NOOP("OTemplateSection", "Logo size (cm)"))};

    // A free text is one information because only the source of the words differs: where it takes
    // its place on the page, and how it is set, are the same either way. So the user says which
    // source, and the field that holds the words is shown for the one that writes them here.
    OTemplateOptionField kFreeTextSource{QStringLiteral("textSource"), OTemplateOptionKind::TextSource,
                                         T(QT_TRANSLATE_NOOP("OTemplateSection", "Text"))};
    kFreeTextSource.defaultValue = OTemplateTextSource::kHere;

    OTemplateOptionField kFreeText{QStringLiteral("text"), OTemplateOptionKind::Text,
                                   T(QT_TRANSLATE_NOOP("OTemplateSection", "Your text"))};
    kFreeText.visibleWhenKey = kFreeTextSource.key;
    kFreeText.visibleWhenValues = {OTemplateTextSource::kHere};

    QList<OTemplateOptionField> freeTextFields{kFreeTextSource, kFreeText};
    freeTextFields.append(textFields);

    // A test square asks two questions and sets two things in type: the size written at each
    // square's corner, and the wording in the middle. Two groups, so two sets of keys — and the
    // keys carry the prefix the resolver reads them back by, which is what keeps a group and its
    // font from coming apart.
    OTemplateOptionField kSquare5cm{QStringLiteral("square5cm"), OTemplateOptionKind::Boolean,
                                    T(QT_TRANSLATE_NOOP("OTemplateSection", "5 cm square"))};
    kSquare5cm.defaultValue = true;
    OTemplateOptionField kSquare3in{QStringLiteral("square3in"), OTemplateOptionKind::Boolean,
                                    T(QT_TRANSLATE_NOOP("OTemplateSection", "3\" square"))};
    kSquare3in.defaultValue = true;

    // The words a test square writes in its middle are the information's own name — one source
    // string, so the menu and the page cannot come to say different things.
    const char *const kSquareTestSource = QT_TRANSLATE_NOOP("OTemplateSection", "Square test");

    OTemplateOptionField kCornerFontFamily{
        QStringLiteral("cornerFontFamily"), OTemplateOptionKind::FontFamily,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Corner font family"))};
    OTemplateOptionField kCornerFontWeight{
        QStringLiteral("cornerFontWeight"), OTemplateOptionKind::FontWeight,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Corner font weight"))};
    OTemplateOptionField kCornerFontSize{
        QStringLiteral("cornerFontSize"), OTemplateOptionKind::FontSize,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Corner font size (px)"))};
    const OTemplateOptionField kCornerLineSpacing{
        QStringLiteral("cornerLineSpacing"), OTemplateOptionKind::LineSpacing,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Corner line spacing (px)"))};

    // Each group is a line, headed by the thing it sets: the two sizes at the corners, and the
    // name in the middle. Without that head the two lines would be six identical controls.
    const QString cornerRow = QStringLiteral("corner");
    kCornerFontFamily.rowKey = cornerRow;
    kCornerFontFamily.rowLabel = QStringLiteral("5cm/3\"");
    kCornerFontWeight.rowKey = cornerRow;
    kCornerFontSize.rowKey = cornerRow;

    OTemplateOptionField kCenterFontFamily{
        QStringLiteral("centerFontFamily"), OTemplateOptionKind::FontFamily,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Center font family"))};
    OTemplateOptionField kCenterFontWeight{
        QStringLiteral("centerFontWeight"), OTemplateOptionKind::FontWeight,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Center font weight"))};
    OTemplateOptionField kCenterFontSize{
        QStringLiteral("centerFontSize"), OTemplateOptionKind::FontSize,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Center font size (px)"))};
    const OTemplateOptionField kCenterLineSpacing{
        QStringLiteral("centerLineSpacing"), OTemplateOptionKind::LineSpacing,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Center line spacing (px)"))};

    const QString centerRow = QStringLiteral("center");
    kCenterFontFamily.rowKey = centerRow;
    kCenterFontFamily.rowLabel = T(kSquareTestSource);
    kCenterFontWeight.rowKey = centerRow;
    kCenterFontSize.rowKey = centerRow;

    // What the printed sheet holds, so that whoever reads it knows which trace belongs to which
    // size. A .omt declares three axes at most — its own schema says so, `sizeGroup maxOccurs="3"`
    // in `ifc/schema/omt/v0.1.0.xsd` — and an output is either one size of that table or a range of
    // them: one information written two ways. Which of the two is not the user's to invent, it
    // follows from how the output was generated; the field is here so that both can be seen, and
    // each set in its own right.
    const OTemplateOptionField kSizeCase{QStringLiteral("sizeCase"), OTemplateOptionKind::SizeCase,
                                         T(QT_TRANSLATE_NOOP("OTemplateSection", "Written for"))};

    // Which axes the page states, and what each of them is called in each language. **The same
    // question a cartouche asks of the same table**, so the same control: the shared composite, and
    // the keys it writes are the shared ones. Neither family has its own idea of what an axis is.
    OTemplateOptionField kSizeAxes{QStringLiteral("axes"), OTemplateOptionKind::AxisRename,
                                   T(QT_TRANSLATE_NOOP("OTemplateSection", "Axes"))};
    kSizeAxes.compositeDefaults = {{OTemplateAxisOptions::kAxes, QStringList{}},
                                   {OTemplateAxisOptions::kRenames, QVariantMap{}},
                                   {OTemplateAxisOptions::kOneLine, false}};
    kSizeAxes.compositeEditor = [](QWidget *parent, const OTemplateOptionContext &context) {
        return new OCoverAxesEditor(context, parent);
    };
    // A block rather than a value: it takes a row of its own, under its own title.
    kSizeAxes.onOwnRow = true;

    // **The two typographies of this information are headed, and their heads are not optional.**
    // Each is three controls sitting on one line, and a line of identical controls with nothing at
    // its head is a line nobody can tell from the other one — the two differ only in what they set.
    //
    // The heads are put on copies rather than on the shared fields: `kFontFamily` and its two
    // neighbours belong to every information that writes words, and a « Title » set on them would
    // appear on the pattern name's line and the free text's as well.
    OTemplateOptionField kTitleFontFamily = kFontFamily;
    kTitleFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Title"));
    OTemplateOptionField kTitleFontWeight = kFontWeight;
    OTemplateOptionField kTitleFontSize = kFontSize;
    OTemplateOptionField kTitleLineSpacing = kLineSpacing;
    // How the languages of that title stand together. The title is one word written once per
    // language and it stays on one line — what holds the words apart is this, and with « None » it
    // is the space that was already there.
    OTemplateOptionField kTitleSeparator{
        OTemplateSeparatorOptions::kLanguageSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Title separator")),
        OTemplateSeparatorOptions::LanguageSeparators(), {}, QString()};
    //
    // The axes are set in a type of their own, the same for all of them — a second set of the same
    // three questions, which is why their keys carry a prefix: `SectionFont` reads them back by it,
    // and that is what keeps a group and its type from coming apart.
    OTemplateOptionField kAxisFontFamily{
        QStringLiteral("axisFontFamily"), OTemplateOptionKind::FontFamily,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Axis font family"))};
    OTemplateOptionField kAxisFontWeight{
        QStringLiteral("axisFontWeight"), OTemplateOptionKind::FontWeight,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Axis font weight"))};
    OTemplateOptionField kAxisFontSize{
        QStringLiteral("axisFontSize"), OTemplateOptionKind::FontSize,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Axis font size (px)"))};
    OTemplateOptionField kAxisLineSpacing{
        QStringLiteral("axisLineSpacing"), OTemplateOptionKind::LineSpacing,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Axis line spacing (px)"))};

    const QString axisRow = QStringLiteral("axisType");
    kAxisFontFamily.rowKey = axisRow;
    // What this line sets, said out loud: the line above it sets the title, this one sets the axes,
    // and without a head the two are three identical controls each.
    kAxisFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Axis value"));
    kAxisFontWeight.rowKey = axisRow;
    kAxisFontSize.rowKey = axisRow;

    // What stands between an axis and its value. **The very same list the cartouche offers for the
    // very same two things**, and taken from the same place rather than written again here: two
    // families that separated an axis from its value differently would be two formats.
    OTemplateOptionField kAxisSeparator{
        OTemplateSeparatorOptions::kSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Separator between axe and value")),
        OTemplateSeparatorOptions::AxisValueSeparators(), {}, QString()};

    // A line between two groups of settings, and nothing else: it asks nothing and keeps nothing.
    // Two of them, because a range is written in three parts — the title, the axes that stand
    // still, and the axis that runs — and the third part is not there at all when the page states
    // one size, which is what its own `visibleWhenKey` says below.
    const OTemplateOptionField kTitleRule{QStringLiteral("titleRule"), OTemplateOptionKind::Rule,
                                          QString()};
    OTemplateOptionField kAxesRule{QStringLiteral("axesRule"), OTemplateOptionKind::Rule,
                                   QString()};

    // **The legend.** Its title is the one a user may rename, and for a reason none of the other
    // informations has: what a mark means is a matter of the pattern, not of the application —
    // "Symbols" is what Ostara calls a legend, and a pattern that reads those very marks differently
    // says so here. It is renamed per language like the marks are, which is why the control is the
    // shared rename editor used with a single thing to rename.
    OTemplateOptionField kSymbolsTitleFontFamily = kFontFamily;
    kSymbolsTitleFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Title"));
    OTemplateOptionField kSymbolsTitleFontWeight = kFontWeight;
    OTemplateOptionField kSymbolsTitleFontSize = kFontSize;
    OTemplateOptionField kSymbolsTitleLineSpacing = kLineSpacing;

    // **The field carries the name of the map it fills, and not a name of its own.** A composite
    // stores nothing under its field's key — it stores under the keys it declares — and the section
    // writer decides what to write by looking that key up. A field named anything else is a field
    // the writer never sees, and the renames it holds are never saved. The cartouche names its own
    // rename field the same way, for the same reason.
    OTemplateOptionField kSymbolsTitleRename{
        OTemplateAxisOptions::kRenames, OTemplateOptionKind::AxisRename,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Rename the title"))};
    // **Only the renames themselves.** The shared writer also knows a list of things and a
    // one-line flag; neither means anything here — there is one title, and it is one line whatever
    // happens — and a page that wrote them would be saying it had chosen something it has not.
    kSymbolsTitleRename.compositeDefaults = {{OTemplateAxisOptions::kRenames, QVariantMap{}}};
    kSymbolsTitleRename.compositeEditor = [](QWidget *parent, const OTemplateOptionContext &context) {
        return new OCoverSymbolsTitleEditor(context, parent);
    };
    kSymbolsTitleRename.onOwnRow = true;

    OTemplateOptionField kSymbolsTitleSeparator{
        OTemplateSeparatorOptions::kLanguageSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Title separator")),
        OTemplateSeparatorOptions::LanguageSeparators(), {}, QString()};

    const OTemplateOptionField kSymbolsRule{QStringLiteral("symbolsRule"), OTemplateOptionKind::Rule,
                                            QString()};

    // How the meanings themselves are set — a second typography, and prefixed like the sizes' axes
    // one so that `SectionFont` reads each group back by its own name.
    OTemplateOptionField kSymbolsFontFamily{
        QStringLiteral("symbolsFontFamily"), OTemplateOptionKind::FontFamily,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Symbol font family"))};
    OTemplateOptionField kSymbolsFontWeight{
        QStringLiteral("symbolsFontWeight"), OTemplateOptionKind::FontWeight,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Symbol font weight"))};
    OTemplateOptionField kSymbolsFontSize{
        QStringLiteral("symbolsFontSize"), OTemplateOptionKind::FontSize,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Symbol font size (px)"))};
    OTemplateOptionField kSymbolsLineSpacing{
        QStringLiteral("symbolsLineSpacing"), OTemplateOptionKind::LineSpacing,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Symbol line spacing (px)"))};

    const QString symbolsRow = QStringLiteral("symbolsType");
    kSymbolsFontFamily.rowKey = symbolsRow;
    kSymbolsFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Symbol text"));
    kSymbolsFontWeight.rowKey = symbolsRow;
    kSymbolsFontSize.rowKey = symbolsRow;

    // **A third list, and the third of its kind.** A legend's lines each carry one meaning written in
    // every language of the page, and this is what holds those apart — the title's own separator
    // would not do, since a title and a line of the legend are two different sentences.
    OTemplateOptionField kSymbolsSeparator{
        QStringLiteral("symbolsLanguageSeparator"), OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Symbol separator")),
        OTemplateSeparatorOptions::LanguageSeparators(), {}, QString()};
    // **The question the whole range page waits on, asked first.** Which sizes the page lists is
    // what decides which axis runs and which stand still, so it stands directly under « written
    // for » and everything that depends on it is unusable until it is answered.
    //
    // Its key is none of the block's own, and that is deliberate: the graded axis below already
    // carries the range's first key, and two blocks writing the same file attribute would write it
    // twice.
    OTemplateOptionField kSizeRangeChooser{QStringLiteral("sizeRangeChooser"),
                                           OTemplateOptionKind::SizeRange,
                                           T(QT_TRANSLATE_NOOP("OTemplateSection", "Size range"))};
    kSizeRangeChooser.compositeEditor = [](QWidget *parent, const OTemplateOptionContext &context) {
        return new OCoverSizeRangeChooser(context, parent);
    };
    kSizeRangeChooser.onOwnRow = true;
    kSizeRangeChooser.visibleWhenKey = QStringLiteral("sizeCase");
    kSizeRangeChooser.visibleWhenValues = {OTemplateSizeCase::kMulti};

    /**
     * **Whether the rest of a range page can be answered at all.**
     *
     * A page that lists sizes cannot say anything about its axes or its typography until it knows
     * which axis runs — there is no list to set, and no way to tell a graded axis from a standing
     * one. A page that states a single size has no such question and is answered from the start,
     * which is why this is a predicate over the whole section rather than a key to compare: the
     * condition is a disjunction, and a declaration that cannot say « or » would have the shared
     * editor guessing.
     */
    const auto readyWhenSized = [](const QHash<QString, QVariant> &options) {
        const QString sizeCase =
            options.value(QStringLiteral("sizeCase"), OTemplateSizeCase::kSingle).toString();
        return sizeCase != OTemplateSizeCase::kMulti
               || !OCoverGradedAxisOptions::FromOptions(options).measurement.isEmpty();
    };

    // **The sizes the page lists, and what each of them is called.** A block rather than a value,
    // like the axes above: it holds a range, a name per size, and the length of the sample drawn
    // for each. Its own control, declared here, so that nothing in the shared editor has to know
    // what a graded axis is.
    //
    // **Its key is the block's own first key**, exactly as the axes block's key is `axes`: the
    // section writer reads a field's key to decide whether the field has anything to write, and a
    // block whose key is none of its own would be a block the file never carries.
    OTemplateOptionField kGradedAxis{OCoverGradedAxisOptions::kRange, OTemplateOptionKind::SizeRange,
                                     T(QT_TRANSLATE_NOOP("OTemplateSection", "Graded axis"))};
    kGradedAxis.compositeDefaults = OCoverGradedAxisOptions::DefaultOptions();
    kGradedAxis.compositeEditor = [](QWidget *parent, const OTemplateOptionContext &context) {
        return new OCoverGradedAxisEditor(context, parent);
    };
    kGradedAxis.onOwnRow = true;

    // The graded axis is set in a type of its own, like the axes and the title before it: a list of
    // sizes is not the same text as a value standing beside an axis name, and the user who wants
    // them set differently must be able to say so. The line spacing is part of that type because
    // **it moves the words and the sample together** — what it spaces is the line, and the line is
    // both of them.
    OTemplateOptionField kSizesFontFamily{
        QStringLiteral("sizesFontFamily"), OTemplateOptionKind::FontFamily,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Sizes font family"))};
    OTemplateOptionField kSizesFontWeight{
        QStringLiteral("sizesFontWeight"), OTemplateOptionKind::FontWeight,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Sizes font weight"))};
    OTemplateOptionField kSizesFontSize{
        QStringLiteral("sizesFontSize"), OTemplateOptionKind::FontSize,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Sizes font size (px)"))};
    OTemplateOptionField kSizesLineSpacing{
        QStringLiteral("sizesLineSpacing"), OTemplateOptionKind::LineSpacing,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Sizes line spacing (px)"))};

    const QString sizesRow = QStringLiteral("sizesType");
    kSizesFontFamily.rowKey = sizesRow;
    kSizesFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Size value"));
    kSizesFontWeight.rowKey = sizesRow;
    kSizesFontSize.rowKey = sizesRow;

    // The third part is written only for a range: a page stating one size has no axis that runs.
    for (OTemplateOptionField *field : {&kAxesRule, &kGradedAxis, &kSizesFontFamily, &kSizesFontWeight,
                                        &kSizesFontSize, &kSizesLineSpacing})
    {
        field->visibleWhenKey = QStringLiteral("sizeCase");
        field->visibleWhenValues = {OTemplateSizeCase::kMulti};
    }

    // **Everything below the range waits for it.** The title's type, the axes and their names, and
    // how the sizes themselves are set: all of it is about a list of sizes, and until the page
    // knows which list, there is nothing to answer. They are made unusable rather than hidden, so
    // the user sees what the information is made of and what is still holding it up.
    for (OTemplateOptionField *field :
         {&kTitleFontFamily, &kTitleFontWeight, &kTitleFontSize, &kTitleLineSpacing,
          &kTitleSeparator, &kSizeAxes, &kAxisFontFamily, &kAxisFontWeight, &kAxisFontSize,
          &kAxisLineSpacing, &kAxisSeparator, &kSizesFontFamily, &kSizesFontWeight, &kSizesFontSize,
          &kSizesLineSpacing})
    {
        field->enabledWhen = readyWhenSized;
    }

    // **The order the fields are declared is the order they are shown, blocks included**: what the
    // information is written for, then the range everything else depends on, then how the title is
    // set and how its languages are held apart, then the axes, then how they are set and how they
    // stand to their values, and last the axis that runs. Each head sits right above what it sets,
    // which is the whole reason a block goes in the form at its place rather than under everything.
    // The two rules part the three groups, so that the graded axis below them is read as its own
    // set of settings rather than as more of the axes' own.
    const QList<OTemplateOptionField> sizeFields = {
        kSizeCase,         kSizeRangeChooser, kTitleFontFamily,  kTitleFontWeight, kTitleFontSize,
        kTitleLineSpacing, kTitleSeparator,   kTitleRule,        kSizeAxes,        kAxisFontFamily,
        kAxisFontWeight,   kAxisFontSize,     kAxisLineSpacing,  kAxisSeparator,   kAxesRule,
        kGradedAxis,       kSizesFontFamily,  kSizesFontWeight,  kSizesFontSize,   kSizesLineSpacing};

    // The legend's own order: how its title is set and how its languages stand together, then what
    // the title says — a name is chosen once the thing it names has been decided — then, under a
    // rule, how the marks' own meanings are set and how theirs stand together.
    const QList<OTemplateOptionField> symbolsFields = {
        kSymbolsTitleFontFamily, kSymbolsTitleFontWeight, kSymbolsTitleFontSize,
        kSymbolsTitleLineSpacing, kSymbolsTitleSeparator, kSymbolsTitleRename, kSymbolsRule,
        kSymbolsFontFamily, kSymbolsFontWeight, kSymbolsFontSize, kSymbolsLineSpacing,
        kSymbolsSeparator};

    // ═══ Seam allowances ═══
    //
    // **A question the whole information waits on.** A pattern is drawn with its seam allowances or
    // drawn without them, and which one it is decides what the reader has to be told: without them
    // the page says so and stops, with them it has to name every margin the pattern is cut with.
    // The answer is asked first and nothing else can be touched until it is given — see the
    // enabledWhen below, which is what makes that true rather than merely intended.
    OTemplateOptionField kSeamCase{OCoverSeamAllowances::kCaseKey, OTemplateOptionKind::Choice,
                                   T(QT_TRANSLATE_NOOP("OTemplateSection", "Seam allowances"))};
    // The labels are the words a reader of the page meets, and the values are what the file keeps:
    // an answer stored as a translated sentence would change meaning with the language of whoever
    // opened the template next.
    kSeamCase.values = {OCoverSeamAllowances::kIncluded, OCoverSeamAllowances::kNotIncluded};
    kSeamCase.valueLabels = {T(QT_TRANSLATE_NOOP("OTemplateSection", "Included in the drawings")),
                             T(QT_TRANSLATE_NOOP("OTemplateSection", "Not included"))};

    // The heading, which both cases print — each with words of its own, and the two are renamed
    // independently: a page that says « Marges comprises » says nothing about what it will say when
    // they are not, and the two titles are long enough that each of them is worth its own one-line
    // answer.
    OTemplateOptionField kSeamTitleFontFamily = kFontFamily;
    kSeamTitleFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Title"));
    OTemplateOptionField kSeamTitleFontWeight = kFontWeight;
    OTemplateOptionField kSeamTitleFontSize = kFontSize;
    OTemplateOptionField kSeamTitleLineSpacing = kLineSpacing;

    const QString seamTitleRow = QStringLiteral("seamTitle");
    kSeamTitleFontFamily.rowKey = seamTitleRow;
    kSeamTitleFontWeight.rowKey = seamTitleRow;
    kSeamTitleFontSize.rowKey = seamTitleRow;

    // The field carries the name of the map it fills — see the legend's rename above for why.
    OTemplateOptionField kSeamTitleRename{
        OTemplateAxisOptions::kRenames, OTemplateOptionKind::AxisRename,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Rename the title"))};
    // Both renames live in that one map, under a key per case, and the one-line flag is shared: the
    // two titles are the same heading set the same way, and only their words differ.
    kSeamTitleRename.compositeDefaults = {{OTemplateAxisOptions::kRenames, QVariantMap{}},
                                          {OTemplateAxisOptions::kOneLine, false}};
    kSeamTitleRename.compositeEditor = [](QWidget *parent, const OTemplateOptionContext &context) {
        return new OCoverSeamTitleEditor(context, parent);
    };
    kSeamTitleRename.onOwnRow = true;

    OTemplateOptionField kSeamTitleSeparator{
        OTemplateSeparatorOptions::kLanguageSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Title separator")),
        OTemplateSeparatorOptions::LanguageSeparators(), {}, QString()};

    OTemplateOptionField kSeamRule{QStringLiteral("seamRule"), OTemplateOptionKind::Rule, QString()};

    // Under the rule, the margins themselves. **A second typography**, prefixed so that SectionFont
    // reads the group back by its own name and cannot take the title's.
    OTemplateOptionField kSeamValuesFontFamily{
        QStringLiteral("seamValuesFontFamily"), OTemplateOptionKind::FontFamily,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Margin font family"))};
    OTemplateOptionField kSeamValuesFontWeight{
        QStringLiteral("seamValuesFontWeight"), OTemplateOptionKind::FontWeight,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Margin font weight"))};
    OTemplateOptionField kSeamValuesFontSize{
        QStringLiteral("seamValuesFontSize"), OTemplateOptionKind::FontSize,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Margin font size (px)"))};
    OTemplateOptionField kSeamValuesLineSpacing{
        QStringLiteral("seamValuesLineSpacing"), OTemplateOptionKind::LineSpacing,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Margin line spacing (px)"))};

    const QString seamValuesRow = QStringLiteral("seamValues");
    kSeamValuesFontFamily.rowKey = seamValuesRow;
    kSeamValuesFontFamily.rowLabel = T(QT_TRANSLATE_NOOP("OTemplateSection", "Margin lines"));
    kSeamValuesFontWeight.rowKey = seamValuesRow;
    kSeamValuesFontSize.rowKey = seamValuesRow;

    // **Two separators again, and for the same reason as the legend's**: a name written in every
    // language of the page, and — inside that line — a width written in every unit the user asked
    // for. The first holds languages apart, the second holds units apart, and they are not the same
    // question.
    OTemplateOptionField kSeamValuesLanguageSeparator{
        OCoverSeamAllowances::kValuesLanguageSeparatorKey, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Name separator")),
        OTemplateSeparatorOptions::LanguageSeparators(), {}, QString()};

    // What stands between a margin's name and its width — « Couture : 1 cm ».
    OTemplateOptionField kSeamValueSeparator{
        OTemplateSeparatorOptions::kSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Separator between name and value")),
        OTemplateSeparatorOptions::AllSeparators(), {}, QStringLiteral(":")};

    // **The width is stated in the units the reader measures in**, and the page may have to state it
    // in more than one: a pattern is cut by people who work in centimetres and by people who work in
    // inches, and a sheet that only spoke one of the two would be read with a conversion in mind.
    OTemplateOptionField kSeamUnits{OCoverSeamAllowances::kUnitsKey, OTemplateOptionKind::CheckGroup,
                                    T(QT_TRANSLATE_NOOP("OTemplateSection", "Units"))};
    // The stored values are the application's own names for the units, so that what a page wrote is
    // read by the same table that names a unit everywhere else.
    kSeamUnits.values = {UnitsToStr(Unit::Mm, false), UnitsToStr(Unit::Cm, false),
                         UnitsToStr(Unit::Inch, false)};
    kSeamUnits.defaultValue = QStringList{UnitsToStr(Unit::Cm, false)};

    OTemplateOptionField kSeamUnitSeparator{
        OCoverSeamAllowances::kUnitSeparatorKey, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Separator between units")),
        {QStringLiteral("/"), QStringLiteral("-"), QStringLiteral("·")}, {}, QStringLiteral("/")};

    // **The rest of the section waits for the answer, and part of it waits for one of the two.**
    // Everything below the question is unusable until it is answered — shown but greyed, so that the
    // user sees what the information is made of and what is holding it back. The margins themselves
    // are not even that in the other case: a page that says the allowances are not included has no
    // widths to name, and offering the fields would be offering a setting with nothing to set.
    const auto answered = [](const QHash<QString, QVariant> &options) {
        return !options.value(OCoverSeamAllowances::kCaseKey).toString().isEmpty();
    };
    const QList<OTemplateOptionField *> seamFieldsWaiting = {
        &kSeamTitleFontFamily, &kSeamTitleFontWeight, &kSeamTitleFontSize, &kSeamTitleLineSpacing,
        &kSeamTitleRename,     &kSeamTitleSeparator,   &kSeamRule,
        &kSeamValuesFontFamily, &kSeamValuesFontWeight, &kSeamValuesFontSize,
        &kSeamValuesLineSpacing, &kSeamValuesLanguageSeparator, &kSeamValueSeparator, &kSeamUnits,
        &kSeamUnitSeparator};
    for (OTemplateOptionField *field : seamFieldsWaiting)
    {
        field->enabledWhen = answered;
    }
    // The first field of each row carries the conditions for the whole row — a field that joins a
    // line already open is not registered — so the two rows say « only when included » where they
    // begin.
    for (OTemplateOptionField *field : {&kSeamValuesFontFamily, &kSeamValuesLanguageSeparator,
                                        &kSeamValueSeparator, &kSeamUnits, &kSeamUnitSeparator})
    {
        field->visibleWhenKey = OCoverSeamAllowances::kCaseKey;
        field->visibleWhenValues = {OCoverSeamAllowances::kIncluded};
    }

    const QList<OTemplateOptionField> seamFields = {
        kSeamCase,           kSeamTitleFontFamily,        kSeamTitleFontWeight,
        kSeamTitleFontSize,  kSeamTitleLineSpacing,       kSeamTitleRename,
        kSeamTitleSeparator, kSeamRule,                   kSeamValuesFontFamily,
        kSeamValuesFontWeight, kSeamValuesFontSize,       kSeamValuesLineSpacing,
        kSeamValuesLanguageSeparator, kSeamValueSeparator, kSeamUnits,
        kSeamUnitSeparator};

    // **A distance, and nothing else.** A page is not only made of what it says: a heading is set
    // apart from what follows it, a block is brought down the sheet, and neither of those is a
    // word. It is not a line spacing either — that one belongs to the line above and moves with its
    // font's height, where what is wanted here is a distance the user chose.
    OTemplateOptionField kSpaceHeight{QStringLiteral("spaceHeight"), OTemplateOptionKind::Pixels,
                                      T(QT_TRANSLATE_NOOP("OTemplateSection", "Height"))};
    kSpaceHeight.defaultValue = 25;

    const QList<OTemplateOptionField> spaceFields = {kSpaceHeight};

    return {
        // The pattern's own name, read from the pattern — nothing for the user to fill in, and
        // nothing here to hold it: the template says that the name is shown and how, and the
        // value comes from the file when the page is drawn.
        MakeSection("patternName",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Pattern name"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "The name of the pattern (set in the pattern toolbar)."),
                    textFields),
        MakeSection("logo",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Logo"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "A logo from your library (add your SVG logos in the Logo page)."),
                    {kLogo, kLogoSize, kLineSpacing}),
        // The same split as the cartouche's free text — the template says that the text is shown
        // and how, and the user writes what it says. But a cover page serves a whole library of
        // patterns, so a mention that never changes would have to be retyped for each of them:
        // hence the two sources, and the user's own answer to which one this is.
        MakeSection("freeText",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Free text"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "A text of your own - written here, so that every cover page "
                                      "made with this template says it, or filled in for each "
                                      "pattern."),
                    freeTextFields),
        // The square a needleworker measures against a ruler. It is the one information of the page
        // that is not made of words: what it says is its own size, and it is printed at that size
        // or the page is wrong. Which is why it is drawn rather than written — see the resolver.
        MakeSection("testSquare", kSquareTestSource,
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "Two squares of a known size, to check that the page is "
                                      "printed at its true size. The wording in the middle is the "
                                      "name of this information, written once per display "
                                      "language."),
                    {kSquare5cm, kSquare3in, kCornerFontFamily, kCornerFontWeight, kCornerFontSize,
                     kCornerLineSpacing, kCenterFontFamily, kCenterFontWeight, kCenterFontSize,
                     kCenterLineSpacing}),
        // What the sheet holds, so that its reader knows which trace is which. Specific to a
        // pattern carrying a .omt: a cover page cannot state sizes it has not been given. The
        // content adapts on its own — one size is stated, a range is listed — which is what the
        // description says, because a user who opens this information has to know that before
        // wondering why it looks empty.
        MakeSection("sizes",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Size(s) - .omt"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "The sizes the printed sheet holds, so that its reader knows "
                                      "which trace is which. The content adapts by itself: an output "
                                      "generated from a single size states it, an output generated "
                                      "from a size range lists them. "
                                      "For the preview to show anything, the pattern must have a "
                                      ".omt size table loaded."),
                    sizeFields),
        // The legend: what the marks on the sheet mean. **The library decides what is here** — its
        // permanent symbols, and the ones the pattern asked for, in the order they were put in — so
        // this information holds no list of its own, and the description says as much, since a user
        // who opens it must know where a symbol is added, and where a waiting one is asked for,
        // before wondering why this is empty.
        MakeSection("symbols",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Symbols"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "What the marks on the pattern mean: one line per symbol your "
                                      "library holds as permanent, followed by the ones this pattern "
                                      "chose - in their own order. Symbols themselves are written and "
                                      "ordered in the Symbols page; the ones that wait to be chosen "
                                      "are picked per pattern, in the Cover page dock."),
                    symbolsFields),
        // The margins the pattern is cut with. **The library decides what is here** — the names on
        // its entries — and the pattern decides which of them the page prints, so the description
        // says where a name is written, since a user who opens this must know that before wondering
        // why it is empty.
        MakeSection("seamAllowances",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Seam allowances"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "Whether the seam allowances are part of the drawings and, "
                                      "when they are, the name of each margin the pattern is cut "
                                      "with: one line per width the pattern actually carries. "
                                      "The names are written in the Seam allowances page, and a "
                                      "pattern must be open for the preview to show them."),
                    seamFields),
        // **Room, and nothing in it.** The one thing a page holds that says nothing: what the user
        // is arranging with it is the page itself, and the height is the only thing there is to set.
        MakeSection("space",
                    QT_TRANSLATE_NOOP("OTemplateSection", "Space"),
                    QT_TRANSLATE_NOOP("OTemplateSection",
                                      "An empty space, to set the page out: give it the height it "
                                      "should take, and nothing is drawn in it."),
                    spaceFields),
    };
}

//---------------------------------------------------------------------------------------------------------------------

namespace
{
bool g_catalogueBuilt = false;
}

auto OCoverPageCatalogue::Sections() -> const QList<OTemplateSection> &
{
    static QList<OTemplateSection> sections;
    if (!g_catalogueBuilt)
    {
        sections = Build();
        g_catalogueBuilt = true;
    }
    return sections;
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageCatalogue::InvalidateCache()
{
    g_catalogueBuilt = false;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageCatalogue::Section(const QString &id) -> const OTemplateSection &
{
    for (const OTemplateSection &section : Sections())
    {
        if (section.Id() == id)
        {
            return section;
        }
    }
    static const OTemplateSection kEmpty;
    return kEmpty;
}
