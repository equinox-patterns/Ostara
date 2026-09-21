/************************************************************************
 **
 **  @file   ocoverseamallowances.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  What a cover page says about a pattern's margins.
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

#include "ocoverseamallowances.h"

#include "vmisc/def.h"

#include <QCoreApplication>

namespace
{
// What the two cases are called, in the context the information's own wording lives in — the same
// context the other titles are looked up in, so they are all translated by the same hand.
constexpr const char *kIncludedWord = QT_TRANSLATE_NOOP("OTemplateSection", "Seam allowances included");
constexpr const char *kNotIncludedWord =
    QT_TRANSLATE_NOOP("OTemplateSection", "Seam allowances not included");

/// The prefix both title keys carry, so that a key in the rename map says what it belongs to.
const QString kTitleKeyPrefix = QStringLiteral("seamAllowances");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The name @p entry carries in each language of the page.
 *
 * A name written in every language the page prints is read as it stands; a name written only in
 * languages this page does not print is said in the one the library was written in first, because a
 * line with a width and no name is not a line a reader can use.
 */
auto TextsFor(const OSeamAllowanceLibrary::Entry &entry, const QStringList &languages) -> QStringList
{
    QStringList texts;
    for (const QString &locale : languages)
    {
        const QString text = entry.texts.value(locale);
        if (!text.isEmpty())
        {
            texts.append(text);
        }
    }

    if (texts.isEmpty() && !entry.texts.isEmpty())
    {
        texts.append(entry.texts.constBegin().value());
    }

    return texts;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OCoverSeamAllowances::TitleKey(const QString &seamCase) -> QString
{
    return kTitleKeyPrefix + (seamCase == kIncluded ? QStringLiteral("Included")
                                                    : QStringLiteral("NotIncluded"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverSeamAllowances::TitleWord(const QString &seamCase, const QString &locale) -> QString
{
    const char *word = seamCase == kIncluded ? kIncludedWord : kNotIncludedWord;

    if (locale.isEmpty())
    {
        return QCoreApplication::translate("OTemplateSection", word);
    }
    return TranslateToLocale(locale, "OTemplateSection", word);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverSeamAllowances::Lines(const OSeamAllowanceLibrary::Library &library,
                                 const QStringList &languages, const QVector<qreal> &values,
                                 qreal defaultMm) -> QVector<Line>
{
    QVector<Line> lines;
    const qint64 wanted = OSeamAllowanceLibrary::Steps(defaultMm);

    for (const OSeamAllowanceLibrary::Entry &entry : library.entries)
    {
        for (const qreal mm : values)
        {
            // **The default entry answers the application's own default**, and not a number written
            // in the library: what that margin is worth is a preference, and the preference is read
            // where it lives. Every other entry answers its own condition.
            const bool answers = entry.byDefault
                                     ? OSeamAllowanceLibrary::Steps(mm) == wanted
                                     : OSeamAllowanceLibrary::Matches(entry, mm);
            if (!answers)
            {
                continue;
            }

            Line line;
            line.mm = mm;
            line.texts = TextsFor(entry, languages);
            lines.append(line);

            // One entry names one width — see the header. The first width it answers is the one it
            // speaks for; a second would mean the library holds two names for the same seam.
            break;
        }
    }

    return lines;
}
