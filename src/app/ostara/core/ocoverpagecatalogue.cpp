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
    const OTemplateOptionField kFontFamily{QStringLiteral("fontFamily"),
                                           OTemplateOptionKind::FontFamily,
                                           T(QT_TRANSLATE_NOOP("OTemplateSection", "Font family"))};
    const OTemplateOptionField kFontWeight{QStringLiteral("fontWeight"),
                                           OTemplateOptionKind::FontWeight,
                                           T(QT_TRANSLATE_NOOP("OTemplateSection", "Font weight"))};
    const OTemplateOptionField kFontSize{QStringLiteral("fontSize"), OTemplateOptionKind::FontSize,
                                         T(QT_TRANSLATE_NOOP("OTemplateSection", "Font size (px)"))};
    const OTemplateOptionField kLineSpacing{QStringLiteral("lineSpacing"),
                                            OTemplateOptionKind::LineSpacing,
                                            T(QT_TRANSLATE_NOOP("OTemplateSection", "Line spacing (px)"))};

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
