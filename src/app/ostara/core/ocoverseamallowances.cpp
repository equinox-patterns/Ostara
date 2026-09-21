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

#include <algorithm>

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
 * @brief Whether @p entry has something to say about @p mm.
 *
 * **The rule, in one place**, because three questions are asked of it — which lines a page prints
 * (@ref Lines), which widths are ambiguous (@ref Ambiguous), and what the library calls a width
 * (@ref LibraryNameFor) — and a rule answered two ways would be a page that contradicted itself.
 *
 * @p defaultKey is the application's own default margin, already brought to the step comparisons
 * are made on: the entry marked as the default answers that, and not a number written in the
 * library, because what that margin is worth is a preference and the preference is read where it
 * lives.
 */
auto Answers(const OSeamAllowanceLibrary::Entry &entry, qreal mm, qint64 defaultKey) -> bool
{
    return entry.byDefault ? OSeamAllowanceLibrary::Steps(mm) == defaultKey
                           : OSeamAllowanceLibrary::Matches(entry, mm);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The name @p entry carries in each language of the page.
 *
 * A name written in every language the page prints is read as it stands; a name written only in
 * languages this page does not print is said in the one the library was written in first, because a
 * line with a width and no name is not a line a reader can use.
 */
auto TextsFor(const OSeamAllowanceLibrary::Entry &entry, const QStringList &languages,
              const QVector<VCoverPageMarginName> &patternNames, qreal mm) -> QStringList
{
    QStringList texts;
    for (const QString &locale : languages)
    {
        // **The pattern's own name comes first.** A name the user gave this width on this pattern is
        // what this pattern calls it, and the library — which cannot tell two widths of one entry
        // apart — is only consulted for the widths the pattern left alone.
        const QString given = CoverPageMarginNameFor(patternNames, mm, locale);
        const QString text = !given.isEmpty() ? given : entry.texts.value(locale);
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
                                 qreal defaultMm, const QVector<VCoverPageMarginName> &patternNames)
    -> QVector<Line>
{
    QVector<Line> lines;
    const qint64 wanted = OSeamAllowanceLibrary::Steps(defaultMm);

    for (const OSeamAllowanceLibrary::Entry &entry : library.entries)
    {
        for (const qreal mm : values)
        {
            if (!Answers(entry, mm, wanted))
            {
                continue;
            }

            Line line;
            line.mm = mm;
            line.texts = TextsFor(entry, languages, patternNames, mm);
            lines.append(line);

            // **And it answers as many widths as the pattern holds.** A rule written as "from 2 cm up
            // is a hem" is one name for two different margins, and a page that printed only one of
            // them would be hiding a width the reader is measuring against. Each is stated; it is
            // the pattern's own names that tell them apart, and they are read above.
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverSeamAllowances::Ambiguous(const OSeamAllowanceLibrary::Library &library,
                                     const QVector<qreal> &values, qreal defaultMm) -> QVector<qreal>
{
    const qint64 wanted = OSeamAllowanceLibrary::Steps(defaultMm);
    QVector<qreal> ambiguous;

    for (const OSeamAllowanceLibrary::Entry &entry : library.entries)
    {
        QVector<qreal> answered;
        for (const qreal mm : values)
        {
            if (Answers(entry, mm, wanted))
            {
                answered.append(mm);
            }
        }

        // **One width is not a question.** An entry that answers a single width of this pattern names
        // it, and there is nothing for anyone to decide; it is when one rule has to speak for two
        // widths that a page shows the same word twice and only the person who drew the pattern can
        // say which line is which.
        if (answered.size() < 2)
        {
            continue;
        }

        for (const qreal mm : answered)
        {
            if (!ambiguous.contains(mm))
            {
                ambiguous.append(mm);
            }
        }
    }

    // Smallest first, as the widths themselves are: what is shown is the pattern's margins, and a
    // list ordered by which library rule happened to catch them would be ordered by something the
    // reader cannot see.
    std::sort(ambiguous.begin(), ambiguous.end());
    return ambiguous;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverSeamAllowances::LibraryNameFor(const OSeamAllowanceLibrary::Library &library, qreal mm,
                                          qreal defaultMm, const QString &locale) -> QString
{
    const qint64 wanted = OSeamAllowanceLibrary::Steps(defaultMm);
    for (const OSeamAllowanceLibrary::Entry &entry : library.entries)
    {
        if (!Answers(entry, mm, wanted))
        {
            continue;
        }

        // The library's own first language says it when the one asked for is not written, exactly as
        // the lines do: a reader who cannot read that is better served than by a blank.
        if (!entry.texts.isEmpty())
        {
            return entry.texts.value(locale, entry.texts.constBegin().value());
        }
        return {};
    }
    return {};
}
