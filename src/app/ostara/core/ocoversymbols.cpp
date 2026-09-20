/************************************************************************
 **
 **  @file   ocoversymbols.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The symbols a cover page's legend is made of — the marks, and
 **          the words, ready to be put into lines.
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

#include "ocoversymbols.h"

#include "osymbolrender.h"

#include "vmisc/def.h"

#include <QCoreApplication>

namespace
{
// The word the application calls a legend by, in the context its own wording lives in — the same
// context the sizes' title is looked up in, so the two are translated by the same hand.
constexpr const char *kSymbolsWord = QT_TRANSLATE_NOOP("OTemplateSection", "Symbols");
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OCoverSymbols::TitleWord(const QString &locale) -> QString
{
    if (locale.isEmpty())
    {
        return QCoreApplication::translate("OTemplateSection", kSymbolsWord);
    }
    return TranslateToLocale(locale, "OTemplateSection", kSymbolsWord);
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverSymbols::Lines(const OSymbolLibrary::Library &library, const QStringList &languages)
    -> QVector<Line>
{
    QVector<Line> lines;

    for (const OSymbolLibrary::Entry &entry : library.entries)
    {
        // **The permanent ones only** — see the header. A symbol with no mark is nothing a legend
        // could show, and one written that way is left out rather than drawn as a bare word.
        if (!entry.permanent || entry.shapes.isEmpty())
        {
            continue;
        }

        Line line;
        line.marks = OSymbolRender::Compose(entry.shapes);

        for (const QString &locale : languages)
        {
            const QString text = entry.texts.value(locale);
            if (!text.isEmpty())
            {
                line.texts.append(text);
            }
        }

        if (line.texts.isEmpty())
        {
            // **A symbol written only in a language this page does not print comes out as a row of
            // marks with nothing beside it** — and a legend that cannot say what a mark means is not
            // a legend. The library's own first language says it, and a reader who cannot read that
            // is better served than by a blank.
            const QString said = entry.texts.value(library.languages.value(0));
            if (!said.isEmpty())
            {
                line.texts.append(said);
            }
        }

        lines.append(line);
    }

    return lines;
}
