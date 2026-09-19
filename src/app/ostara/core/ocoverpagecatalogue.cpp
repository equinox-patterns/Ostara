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

#include "core/otemplateseparatoroptions.h"
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
        return new OTemplateAxisEditor(context, parent);
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
    const OTemplateOptionField kTitleLineSpacing = kLineSpacing;
    // How the languages of that title stand together. The title is one word written once per
    // language and it stays on one line — what holds the words apart is this, and with « None » it
    // is the space that was already there.
    const OTemplateOptionField kTitleSeparator{
        OTemplateSeparatorOptions::kLanguageSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Title separator")),
        OTemplateSeparatorOptions::LanguageSeparators(), QString()};
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
    const OTemplateOptionField kAxisLineSpacing{
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
    const OTemplateOptionField kAxisSeparator{
        OTemplateSeparatorOptions::kSeparator, OTemplateOptionKind::Separator,
        T(QT_TRANSLATE_NOOP("OTemplateSection", "Separator between axe and value")),
        OTemplateSeparatorOptions::AxisValueSeparators(), QString()};

    // **The order the fields are declared is the order they are shown, blocks included**: what the
    // information is written for, then how the title is set and how its languages are held apart,
    // then the axes, then how they are set and how they stand to their values. Each head sits right
    // above what it sets, which is the whole reason a block goes in the form at its place rather
    // than under everything.
    const QList<OTemplateOptionField> sizeFields = {
        kSizeCase,      kTitleFontFamily, kTitleFontWeight, kTitleFontSize, kTitleLineSpacing,
        kTitleSeparator, kSizeAxes,       kAxisFontFamily,  kAxisFontWeight, kAxisFontSize,
        kAxisLineSpacing, kAxisSeparator};

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
                                      "A text of your own — written here, so that every cover page "
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
