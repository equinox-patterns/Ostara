/************************************************************************
 **
 **  @file   vcoverpagedata.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  What a cover page reads from the pattern rather than from its own template.
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

#ifndef VCOVERPAGEDATA_H
#define VCOVERPAGEDATA_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

/**
 * @brief What the cover page reads from the pattern rather than from its own template.
 *
 * **A template says what to show and how, never what it shows.** Everything that differs from one
 * pattern to the next has to come from the pattern, and this is where the three things that do are
 * kept: the free text a template leaves to be filled in, the symbols chosen from the library for
 * this pattern, and the names this pattern gives to its own seam allowances.
 *
 * **None of it reaches the container.** It is read from the document when a page is drawn, the way
 * the final measurements are, so it costs nothing to a pattern that was never exported and nothing
 * to an edit that never touched a cover page.
 *
 * **Kept in a header of its own** so that the pieces that read it — the resolver, the seam allowance
 * lines — can be handed it without being handed the whole pattern, and the DOM that carries it.
 */

/**
 * One seam allowance of the pattern, named by the pattern itself rather than by the library.
 *
 * **The width is the identity, and it is the pattern's own.** A library entry holds no identity of
 * its own — only its place in a file the user may reorder — so a name kept against one would break
 * the first time the library moved. A width does not: 2 cm is 2 cm whatever the library does.
 */
struct VCoverPageMarginName
{
    qreal mm{0.0};
    QMap<QString, QString> texts{}; ///< one name per locale, keyed by the locale's code
};

/** @brief The three things a pattern contributes to its own cover page. */
struct VCoverPageData
{
    /**
     * The text a template left to be filled in, **in one language and not one per language** — which
     * is what the template's own text is when it is written there: a free text is the author's note
     * about this pattern, and it says what they wrote, in the words they wrote it in.
     */
    QString freeText{};

    QStringList symbols{}; ///< the chosen library entries, by their stable id
    QVector<VCoverPageMarginName> marginNames{};
};

/**
 * The step a margin width is kept and compared on: hundredths of a millimetre, which is what the two
 * decimals of the file hold. A width written and read back is therefore the same width, and the
 * 2 cm a page found on a pattern is the 2 cm the pattern was given a name for.
 */
constexpr qint64 kCoverPageWidthStep = 100;

/** @brief @p mm as the whole hundredths of a millimetre a name is kept against. */
inline auto CoverPageWidthKey(qreal mm) -> qint64
{
    return qRound64(mm * kCoverPageWidthStep);
}

/**
 * @brief The name @p names give to a margin of @p mm in @p locale, or nothing when they give none.
 *
 * **Widths are compared as whole hundredths**, never as reals, so no float equality is ever asked —
 * and asking here rather than at each call site keeps that step in one place.
 */
inline auto CoverPageMarginNameFor(const QVector<VCoverPageMarginName> &names, qreal mm,
                                   const QString &locale) -> QString
{
    const qint64 wanted = CoverPageWidthKey(mm);
    for (const VCoverPageMarginName &margin : names)
    {
        if (CoverPageWidthKey(margin.mm) == wanted)
        {
            return margin.texts.value(locale);
        }
    }
    return {};
}

/// @brief The same question asked of a whole @ref VCoverPageData.
inline auto CoverPageMarginNameFor(const VCoverPageData &data, qreal mm, const QString &locale) -> QString
{
    return CoverPageMarginNameFor(data.marginNames, mm, locale);
}

#endif // VCOVERPAGEDATA_H
