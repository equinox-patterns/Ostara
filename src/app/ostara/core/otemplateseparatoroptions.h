/************************************************************************
 **
 **  @file   otemplateseparatoroptions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   19/09/2026
 **
 **  @brief  What a template writes between two things on one line.
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

#ifndef OTEMPLATESEPARATOROPTIONS_H
#define OTEMPLATESEPARATOROPTIONS_H

#include <QLatin1Char>
#include <QString>
#include <QStringList>

/**
 * @brief What stands between two things written on one line, and how that line is composed.
 *
 * **One vocabulary, two families.** A cartouche states the sizes of the piece it decorates, a cover
 * page the sizes of the sheet it heads; both write an axis beside its value, and a page may write
 * the same word in several languages on a single line. The question *what stands between them* is
 * the same question for both, so its answers live here rather than in either catalogue — a list
 * copied from one family into the other agrees with it only on the day it is copied.
 *
 * **No separator is not nothing.** It is one space: « Stature 168 ». A separator that is there takes
 * a space on each of its sides: « Stature : 168 ». That is what the cartouche has always written,
 * and Compose() is from now on the only place it is written.
 *
 * The lists below are the values a radio row offers, in the order it offers them. The empty string
 * always comes first: it is the « None » entry, and the default of every separator unless the field
 * that declares it says otherwise.
 */
namespace OTemplateSeparatorOptions
{
/// The option holding the separator between a name — an axis, a measurement — and its value.
inline const QString kSeparator = QStringLiteral("separator");

/// The option holding the separator between the languages an information's title is written in.
inline const QString kLanguageSeparator = QStringLiteral("languageSeparator");

/// Every separator any information may hold, « None » first: what a separator field that names no
/// list of its own is allowed to carry, and what a value read back from a file is checked against.
inline auto AllSeparators() -> QStringList
{
    return {QString(), QStringLiteral(":"), QStringLiteral("-"), QStringLiteral("/"),
            QStringLiteral("·")};
}

/// The separators an axis may stand behind its value. **No « - »**: it is reserved for joining the
/// languages of one line — « Stature : 168 - Height : 168 » — so a line that used it twice could not
/// be read back into the two things it was made of.
inline auto AxisValueSeparators() -> QStringList
{
    return {QString(), QStringLiteral(":"), QStringLiteral("/"), QStringLiteral("·")};
}

/**
 * @brief The token standing for « the first language bare, the others in brackets ».
 *
 * **A word rather than a bracket**, because it is not punctuation: it says where each language goes,
 * and a bracket stored in the field would be read back as a bracket the user wanted printed.
 */
inline const QString kParenthesesSeparator = QStringLiteral("parens");

/// The separators between the languages of an information's title. Fewer than the others: a title
/// is one word per language and nothing around it, so punctuation is all it has to carry.
inline auto LanguageSeparators() -> QStringList
{
    return {QString(), QStringLiteral("/"), kParenthesesSeparator};
}

/// What holds apart the languages of one axis when they share a line. **Not offered as a separator
/// anywhere**, and that is the whole point of it: this one is not the user's to change, and a line
/// that carried it twice could not be told from one that carried it once between its languages.
inline const QString kLanguageJoin = QStringLiteral(" - ");

/**
 * @brief « name [separator] value » — the one place the shape of such a line is decided.
 *
 * An empty value writes the name alone: there is nothing to stand apart from. An empty separator
 * still separates, with the space that was always there.
 */
inline auto Compose(const QString &name, const QString &value, const QString &separator) -> QString
{
    if (value.isEmpty())
    {
        return name;
    }
    if (separator.isEmpty())
    {
        return name + QLatin1Char(' ') + value;
    }
    return name + QLatin1Char(' ') + separator + QLatin1Char(' ') + value;
}

/// The same shape, folded over as many parts as there are — what joins the languages of one line.
inline auto Join(const QStringList &parts, const QString &separator) -> QString
{
    if (parts.isEmpty())
    {
        return QString();
    }

    if (separator == kParenthesesSeparator)
    {
        // **The languages stand one after another and only the first is bare**: « Tailles (Sizes) ».
        // The page is read in its first language, and what follows is there for whoever reads the
        // other one — so it is put aside rather than made equal to what it translates. Three
        // languages are « Tailles (Sizes, Grössen) » and not « Tailles (Sizes) (Grössen) »: what is
        // bracketed is the aside, not each language of it.
        //
        // This is a rule about the *order* of the parts, and it lives here for that reason: Compose
        // only ever sees two of them and cannot tell which of the two it was handed is the first.
        QString joined = parts.constFirst();
        if (parts.size() > 1)
        {
            joined += QStringLiteral(" (") + parts.mid(1).join(QStringLiteral(", "))
                      + QLatin1Char(')');
        }
        return joined;
    }

    QString joined = parts.constFirst();
    for (int i = 1; i < parts.size(); ++i)
    {
        joined = Compose(joined, parts.at(i), separator);
    }
    return joined;
}
} // namespace OTemplateSeparatorOptions

#endif // OTEMPLATESEPARATOROPTIONS_H
