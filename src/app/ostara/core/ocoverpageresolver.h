/************************************************************************
 **
 **  @file   ocoverpageresolver.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  One information of a cover page, turned into the lines the page draws.
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

#ifndef OCOVERPAGERESOLVER_H
#define OCOVERPAGERESOLVER_H

#include "ocoverpagesizes.h"
#include "ocoverpagetemplate.h"
#include "ocoverseamallowances.h"
#include "ocoversymbols.h"

#include "vwidgets/ocartouchepainter.h"

#include <QFont>
#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include <optional>

/**
 * @brief What a cover page draws, for one information.
 *
 * **This is the only place that knows how an information is drawn.** The catalogue says which
 * fields an information has and what they are called; this says what they come to on the page —
 * a logo becomes an image line, a text becomes as many lines as it has breaks, and an information
 * whose value is not there yet becomes nothing at all. Adding an information means adding a branch
 * here, and both the preview and the PDF export read it: neither of them names an information, so
 * neither can drift from the other.
 *
 * It is the counterpart of `OInfoBlockResolver`, which does this for a cartouche. Two families, two
 * resolvers — what they resolve is what differs, and each keeps its own.
 *
 * @note **Nothing is not "draw nothing".** The preview stands an information's own name in the
 * place of one that resolves to nothing, so that the user can see it is there and waiting for a
 * value; the page itself draws nothing there. Which of the two is happening is the preview's
 * decision, not this one's — see the spec §6.
 */
class OSizeTable;

namespace OCoverPageResolver
{
/**
 * @brief What the pattern gives the page.
 *
 * A cover page is written once and filled from the pattern it is exported with — except in the
 * editor, which draws a template on its own, with no pattern loaded at all. That is what an empty
 * one of these stands for: every information that reads the pattern resolves to nothing, and the
 * preview shows its name in its place.
 */
struct PatternData
{
    /// The value every axis stands at for the run being drawn, by axis name — **the run's own
    /// answer, and the reason this field exists** (nomenclature spec §6).
    ///
    /// Empty is the preview's case, and a name the map does not hold falls back to the table's
    /// reference value: a page may ask about an axis the run never pinned, and the table's own
    /// answer is then the only honest one.
    QMap<QString, QString> axisValues;

    /// Whether this page states one size or lists a range — **the run's answer when it has one**.
    ///
    /// Empty is the preview's case: there, the page shows the case the designer is working on, which
    /// is the section's own `sizeCase` option. At export the answer is not his to give — it follows
    /// from how the output was generated (see `OTemplateSizeCase`), and a page designed for ranges
    /// printed from a single-size run would list sizes the paper does not carry.
    std::optional<bool> listsRange;

    /// The name of the pattern, as the pattern toolbar holds it.
    QString name;

    /// The free text filled in for this pattern — what a template that leaves it to be filled asks
    /// the output dialog for, and what does not exist yet.
    QString freeText;

    /**
     * @brief The sizes the page lists when it states a range, in the table's own order.
     *
     * **Handed over rather than looked up.** Which sizes a range holds is a question about a table,
     * a range and the styles authored for it — three things a page is not — so whoever holds them
     * answers it and the page draws the answer. Empty when the page states one size, or when no
     * range has been chosen yet, which is a page with no sizes to list.
     */
    QVector<OCoverSizeLine> sizes;

    /**
     * @brief The legend this page shows, in the library's own order.
     *
     * **Handed over rather than looked up**, like the sizes above: which symbols are permanent, and
     * what each is made of, are questions about a library a page knows nothing about — so whoever
     * holds it answers and the page draws the answer. Empty when there is nothing to explain.
     */
    QVector<OCoverSymbols::Line> symbols;

    /**
     * @brief The margins the pattern is cut with, in the library's own order.
     *
     * **Handed over rather than looked up**, like the sizes and the legend: reading a pattern's
     * pieces, and deciding which name answers which width, are questions about a pattern and a
     * library — neither of which a page holds. Empty when the page states that the allowances are
     * not included, and when the pattern carries none.
     */
    QVector<OCoverSeamAllowances::Line> seamAllowances;
};

/**
 * @brief The lines one information draws, in order, at print size.
 *
 * @param baseFont the font the page is set in, which a section's own typography starts from —
 *                 a fact about where the page is drawn, which is why the caller passes it.
 * @param languages the display languages of the template, in the order the language row holds
 *                 them. An information whose words are the app's own — the name a test square
 *                 writes in its middle — is set once per language; one that carries its own
 *                 written text is set once. The list is never empty, and the page shows what it
 *                 holds in that order.
 * @param table the sizes of the pattern, or null when there is none. An information that states a
 *              size cannot state one without it: it draws nothing rather than a guess, the same
 *              way it draws nothing for a logo nobody chose.
 */
auto Resolve(const OCoverPageTemplate::Section &section, const PatternData &data,
             const QFont &baseFont, const QStringList &languages, const OSizeTable *table)
    -> QVector<OCartoucheRenderLine>;
} // namespace OCoverPageResolver

#endif // OCOVERPAGERESOLVER_H
