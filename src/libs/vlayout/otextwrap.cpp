/************************************************************************
 **
 **  @file   otextwrap.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  Repli d'un texte à une largeur donnée — un seul algorithme
 **          pour toutes les surfaces qui posent du texte sur un dessin.
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
#include "otextwrap.h"

#include <QFont>
#include <QFontMetrics>
#include <QMutableListIterator>

#include "../vmisc/svgfont/vsvgfontengine.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// Le corps du repli : les mots d'un paragraphe, empilés tant qu'ils tiennent dans la largeur.
auto WrapParagraph(const QString &text, qreal maxWidth, const OTextWrap::Measure &measure) -> QStringList
{
    QStringList words = text.split(QLatin1Char(' '));

    QString currentLine;
    qreal currentLineWidth = 0;
    const qreal spaceWidth = measure.part(QStringLiteral(" "));
    // Tolérance d'origine : un mot qui dépasse la fin de ligne de moins d'un tiers de la largeur
    // ouvre la ligne suivante au lieu d'être serré dessus.
    const qreal tolerance = 0.3;

    QStringList lines;
    lines.reserve(words.size());
    QMutableListIterator iterator(words);

    auto AppendWord = [&currentLine, &currentLineWidth](const QString &word, qreal totalWidth)
    {
        if (!currentLine.isEmpty())
        {
            currentLine += QLatin1Char(' ');
        }
        currentLine += word;
        currentLineWidth = totalWidth;
    };

    while (iterator.hasNext())
    {
        const QString &word = iterator.next();
        const qreal wordWidth = measure.word(word);
        const qreal totalWidth = !currentLine.isEmpty() ? currentLineWidth + spaceWidth + wordWidth : wordWidth;

        if (totalWidth <= maxWidth)
        {
            AppendWord(word, totalWidth);
        }
        else if ((maxWidth - currentLineWidth) <= qRound(maxWidth * tolerance) && maxWidth >= wordWidth)
        {
            // Le mot ouvre la ligne suivante : la ligne courante est close.
            lines.append(currentLine);
            currentLine = word;
            currentLineWidth = wordWidth;
        }
        else
        {
            // Le mot ne tient pas. On ferme d'abord la ligne s'il n'y reste même pas la place d'un
            // caractère, sans quoi la coupure ci-dessous n'aurait aucune largeur où s'appliquer.
            if (currentLineWidth + spaceWidth + measure.word(QString(word.at(0))) > maxWidth)
            {
                lines.append(currentLine);
                currentLine.clear();
                currentLineWidth = 0;
            }

            const qreal subWordWidth = !currentLine.isEmpty() ? maxWidth - (currentLineWidth + spaceWidth) : maxWidth;
            const QStringList subWords = OTextWrap::SplitByWidth(word, subWordWidth, measure);

            if (subWords.isEmpty() || subWords.size() > 2)
            {
                // Aucun point de coupure : le mot reste entier et déborde. Un mot illisible serait
                // pire qu'un mot qui dépasse.
                AppendWord(word, totalWidth);
            }
            else
            {
                const qreal width = measure.word(subWords.constFirst());
                const qreal tWidth = !currentLine.isEmpty() ? currentLineWidth + spaceWidth + width : width;
                AppendWord(subWords.constFirst(), tWidth);
                lines.append(currentLine);

                if (subWords.size() == 2)
                {
                    currentLine.clear();
                    currentLineWidth = 0;

                    // Le reste du mot est repris au tour suivant, comme un mot à part entière.
                    iterator.insert(subWords.constLast());
                    iterator.previous();
                }
            }
        }
    }

    if (!currentLine.isEmpty())
    {
        lines.append(currentLine);
    }

    return lines;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OTextWrap::Lines(const QString &text, qreal maxWidth, const Measure &measure) -> QStringList
{
    QStringList lines;
    if (text.isEmpty())
    {
        return lines;
    }

    const QStringList paragraphs = text.split(QLatin1Char('\n'));
    lines.reserve(paragraphs.size());

    for (const QString &paragraph : paragraphs)
    {
        if (paragraph.isEmpty())
        {
            // Un saut de ligne écrit reste un saut de ligne : deux langues séparées par un blanc ne
            // doivent pas se retrouver recollées sur la même ligne.
            lines.append(QString());
            continue;
        }

        if (maxWidth <= 0)
        {
            // Aucune largeur à respecter : le paragraphe tient tel quel, et on ne découpe rien.
            lines.append(paragraph);
            continue;
        }

        lines.append(WrapParagraph(paragraph, maxWidth, measure));
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto OTextWrap::FontMeasure(const QFont &font) -> Measure
{
    QFontMetrics const metrics(font);
    auto advance = [metrics](const QString &text) -> qreal { return metrics.horizontalAdvance(text); };
    return Measure{advance, advance};
}

//---------------------------------------------------------------------------------------------------------------------
auto OTextWrap::EngineMeasure(const VSvgFontEngine &engine, qreal penWidth) -> Measure
{
    auto advance = [engine, penWidth](const QString &text) -> qreal { return engine.TextWidth(text, penWidth); };
    return Measure{advance, advance};
}

//---------------------------------------------------------------------------------------------------------------------
auto OTextWrap::SplitByWidth(const QString &text, qreal maxWidth, const Measure &measure) -> QStringList
{
    if (measure.part(text) <= maxWidth)
    {
        return {text};
    }

    QStringList substrings;
    substrings.reserve(2);

    const auto textLength = static_cast<int>(text.length());
    qreal lineWidth = 0;

    for (int endIndex = 0; endIndex < textLength; ++endIndex)
    {
        const qreal charWidth = measure.part(QString(text.at(endIndex)));

        if (lineWidth + charWidth > maxWidth)
        {
            if (endIndex > 0)
            {
                substrings.append(text.mid(0, endIndex));
            }

            if (endIndex < textLength)
            {
                substrings.append(text.mid(endIndex));
            }

            break;
        }

        lineWidth += charWidth;
    }

    return substrings;
}
