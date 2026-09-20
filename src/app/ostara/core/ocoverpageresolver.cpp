/************************************************************************
 **
 **  @file   ocoverpageresolver.cpp
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

#include "ocovergradedaxis.h"
#include "ocoverpageresolver.h"

#include "ocoverpagecatalogue.h"
#include "ologolibrary.h"
#include "ologorender.h"
#include "osizetable.h"
#include "otemplateaxisoptions.h"
#include "otemplateseparatoroptions.h"

#include "vmisc/def.h"

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------

namespace
{
/// The side of each square. They are not translated: a unit is not a word.
constexpr double kSquareFiveCm = 5.0;
constexpr double kSquareThreeIn = 3.0;

/// How far the size is written from the square's corner, in centimetres — clear of the line, so
/// the two do not touch and the value stays readable.
constexpr double kCornerInsetCm = 0.3;

/// One square of the block: its side at print size, and the value written at its corner.
struct TestSquare
{
    qreal side{0.0};  ///< in print pixels.
    QString value;    ///< what is written at its top-left corner.
};

/**
 * @brief The squares the user switched on, the smaller first.
 *
 * **The size is the whole point of this information**, so it is computed with the very conversions
 * the page and its bands are placed with: a square drawn from `CmToPixel(5)` agrees with the sheet
 * it is printed on, which is what makes it a test rather than a decoration. Three inches come out
 * at exactly 288 px, with no rounding left to be wrong about.
 */
auto TestSquares(const QHash<QString, QVariant> &options) -> QVector<TestSquare>
{
    const auto isOn = [&options](const QString &key) { return options.value(key, false).toBool(); };

    QVector<TestSquare> squares;
    if (isOn(QStringLiteral("square5cm")))
    {
        squares.append({CmToPixel(kSquareFiveCm), QStringLiteral("5cm")});
    }
    if (isOn(QStringLiteral("square3in")))
    {
        squares.append({InchToPixel(kSquareThreeIn), QStringLiteral("3\"")});
    }
    return squares;
}

/// The word the single-size case writes. Not the information's own name — the menu says
/// « Size(s) - .omt » and the page says « Size » — so the two are separate strings and this one,
/// being a word, is translated.
constexpr const char *kSizeWord = QT_TRANSLATE_NOOP("OTemplateSection", "Size");
// The same word, said of a list rather than of one size. A page that states a range is a page
// about sizes, and its title says so — the plural is not decoration, it is what tells the reader
// at a glance which of the two pages they are holding.
constexpr const char *kSizesWord = QT_TRANSLATE_NOOP("OTemplateSection", "Sizes");

/// The languages a page is written in. Never empty: a template always holds one, and a section
/// resolved outside any language still has to write its words in the source.
auto RenderLanguages(const QStringList &languages) -> QStringList
{
    return languages.isEmpty() ? QStringList{QString()} : languages;
}

/// What one display language says about one axis — the shape the sizes information stores.
///
/// **The axis is the outer key, and the language the inner one**, which is the shape the shared
/// control writes: an alias belongs to one axis, and the same word can be the name of no two of
/// them. Reading the two the other way round finds nothing at all — silently, since a lookup that
/// misses looks exactly like a user who renamed nothing.
auto AxisRenames(const QVariantMap &renames, const QString &locale, const QString &axis)
    -> QVariantMap
{
    return renames.value(axis).toMap().value(locale).toMap();
}

/// The name the axis is written under, or an empty string when it keeps its own.
auto AxisAlias(const QVariantMap &renames, const QString &locale, const QString &axis) -> QString
{
    return AxisRenames(renames, locale, axis).value(QStringLiteral("name")).toString();
}

/// The aliases of the axis's values, indexed by their place in the table.
auto ValueAliases(const QVariantMap &renames, const QString &locale, const QString &axis)
    -> QVariantList
{
    return AxisRenames(renames, locale, axis).value(QStringLiteral("values")).toList();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageResolver::Resolve(const OCoverPageTemplate::Section &section, const PatternData &data,
                                 const QFont &baseFont, const QStringList &languages,
                                 const OSizeTable *table) -> QVector<OCartoucheRenderLine>
{
    // An information no catalogue declares is one that cannot be shown, let alone drawn: it was
    // written by a build that knew it, or by hand.
    const OTemplateSection &type = OCoverPageCatalogue::Section(section.typeId);
    if (!type.IsValid())
    {
        return {};
    }

    const QFont font = OCartouchePainter::SectionFont(section.options, baseFont);
    const int lineSpacing = section.options.value(QStringLiteral("lineSpacing"), 0).toInt();

    if (section.typeId == QLatin1String("patternName"))
    {
        // Nothing to draw while the editor is drawing the template — no pattern is loaded, so the
        // name the pattern would give is not there to be written.
        return data.name.isEmpty() ? QVector<OCartoucheRenderLine>()
                                   : OCartouchePainter::TextLines(data.name, font, lineSpacing);
    }

    if (section.typeId == QLatin1String("logo"))
    {
        const QString stored = section.options.value(QStringLiteral("logo")).toString();
        const double sizeCm = section.options.value(QStringLiteral("logoSize"), 2.0).toDouble();

        const OLogoRender::Drawing drawing = OLogoRender::Draw(stored, sizeCm);
        if (drawing.pixmap.isNull())
        {
            // Nothing chosen yet, or a file that has gone since. Both come to the same page: one
            // with no logo on it.
            return {};
        }

        // The path travels with the line: whoever draws the page at print resolution rasterises
        // the logo itself rather than scaling what was drawn for a screen.
        return {OCartouchePainter::LogoLine(drawing.pixmap, drawing.heightPx, lineSpacing,
                                            OLogoLibrary::Directory() + QLatin1Char('/') + stored)};
    }

    if (section.typeId == QLatin1String("freeText"))
    {
        // Two sources, and the template says which: the words written in it, or the ones the
        // pattern's output carries. Nothing is drawn while the second has not been filled in.
        const bool writtenHere = section.options.value(QStringLiteral("textSource")).toString()
                                 == OTemplateTextSource::kHere;
        const QString text = writtenHere ? section.options.value(QStringLiteral("text")).toString()
                                         : data.freeText;
        if (text.isEmpty())
        {
            return {};
        }

        return OCartouchePainter::TextLines(text, font, lineSpacing);
    }

    if (section.typeId == QLatin1String("testSquare"))
    {
        const QVector<TestSquare> squares = TestSquares(section.options);
        if (squares.isEmpty())
        {
            // Neither square was switched on. There is nothing to measure, so there is nothing to
            // draw — and the preview stands the information's own name in its place.
            return {};
        }

        // Both squares share one centre, the smaller inside the larger: one ruler laid across the
        // big one checks the small one in the same go, which is what having two is for.
        qreal side = 0.0;
        for (const TestSquare &square : squares)
        {
            side = (std::max)(side, square.side);
        }
        const QRectF frame(0.0, 0.0, side, side);

        const QFont cornerFont =
            OCartouchePainter::SectionFont(section.options, baseFont, QStringLiteral("corner"));
        const int cornerSpacing =
            section.options.value(QStringLiteral("cornerLineSpacing"), 0).toInt();

        QVector<QRectF> outlines;
        QVector<OCartoucheShapeLabel> labels;
        const qreal inset = CmToPixel(kCornerInsetCm);
        for (const TestSquare &square : squares)
        {
            const QRectF outline(frame.center().x() - square.side / 2.0,
                                 frame.center().y() - square.side / 2.0, square.side, square.side);
            outlines.append(outline);

            // The value stands just inside the square's own top-left corner, written at the size
            // the square is — that is what there is to read against a ruler. Its box is kept
            // within the square, so a long value cannot make the block measure wider than the
            // geometry that gives it its meaning.
            OCartoucheShapeLabel value;
            value.box = QRectF(outline.left() + inset, outline.top() + inset,
                               outline.width() - inset, outline.height() - inset);
            value.align = Qt::AlignLeft | Qt::AlignTop;
            value.text = square.value;
            value.font = cornerFont;
            value.lineSpacing = cornerSpacing;
            labels.append(value);
        }

        // The wording in the middle is the information's own name — the same string the menu shows,
        // so the two cannot come to say different things. It is set once per display language, in
        // the order the template lists them: a cover page is printed once and read in as many
        // languages as the user asked for. A language the app has not been translated into falls
        // back to the source, so a page is never left with a line missing.
        const QByteArray source = type.SourceName().toUtf8();
        QStringList wording;
        if (languages.isEmpty())
        {
            wording.append(TranslateToLocale(QString(), "OTemplateSection", source.constData()));
        }
        for (const QString &locale : languages)
        {
            wording.append(TranslateToLocale(locale, "OTemplateSection", source.constData()));
        }

        if (!wording.isEmpty())
        {
            OCartoucheShapeLabel center;
            center.box = frame;
            center.align = Qt::AlignCenter;
            // One break, one line: TextLines splits what is written here into the lines the page
            // stacks, and that is what makes one language one line.
            center.text = wording.join(QLatin1Char('\n'));
            center.font =
                OCartouchePainter::SectionFont(section.options, baseFont, QStringLiteral("center"));
            center.lineSpacing =
                section.options.value(QStringLiteral("centerLineSpacing"), 0).toInt();
            labels.append(center);
        }

        return {OCartouchePainter::ShapeLine(outlines, labels, lineSpacing)};
    }

    if (section.typeId == QLatin1String("sizes"))
    {
        const QString sizeCase =
            section.options.value(QStringLiteral("sizeCase"), OTemplateSizeCase::kSingle).toString();
        // Which of the two the page is: one that states a size, or one that lists the sizes it
        // holds. The answer decides the title, whether the sizes are listed, and where the axes
        // that do not run take their value.
        const bool listsRange = sizeCase != OTemplateSizeCase::kSingle;
        if (table == nullptr)
        {
            return {}; // no .omt: an information about sizes has no sizes to state
        }

        const QStringList axes = section.options.value(OTemplateAxisOptions::kAxes).toStringList();
        const QVariantMap renames = section.options.value(OTemplateAxisOptions::kRenames).toMap();
        const bool oneLine = section.options.value(OTemplateAxisOptions::kOneLine, false).toBool();

        // What stands between the languages of a line, and between an axis and its value. Two
        // questions, and the same vocabulary answers both here and in a cartouche: a page and a
        // cartouche that separated the same two things differently would be two formats.
        const QString languageSeparator =
            section.options.value(OTemplateSeparatorOptions::kLanguageSeparator).toString();
        const QString separator =
            section.options.value(OTemplateSeparatorOptions::kSeparator).toString();

        // The axes are set in their own type — the same for all of them, and not the one the title
        // is set in. Reading it by prefix is what keeps the two from coming apart.
        const QFont axisFont =
            OCartouchePainter::SectionFont(section.options, baseFont, QStringLiteral("axis"));
        const int axisSpacing = section.options.value(QStringLiteral("axisLineSpacing"), 0).toInt();

        QVector<OCartoucheRenderLine> lines;

        // The title. One size, so the information states it and lists nothing under that word but
        // the axes it was asked for. It is translated like the wording in the middle of a test
        // square, and written once per display language — a page is printed once and read in as
        // many languages as the user asked for.
        QStringList title;
        if (languages.isEmpty())
        {
            title.append(TranslateToLocale(QString(), "OTemplateSection",
                                          listsRange ? kSizesWord : kSizeWord));
        }
        for (const QString &locale : languages)
        {
            title.append(TranslateToLocale(locale, "OTemplateSection",
                                          listsRange ? kSizesWord : kSizeWord));
        }
        if (!title.isEmpty())
        {
            // **One line, however many languages**: a title is a word, and the same word repeated
            // down the page would read as so many titles. What holds them apart is the separator
            // the user chose — and with « None » it is the space that was always there.
            lines.append(OCartouchePainter::TextLines(
                OTemplateSeparatorOptions::Join(title, languageSeparator), font, lineSpacing));
        }

        // Under it, one line per axis the user asked for, in the order the table declares them.
        // **A page printed from a single size can only be about one value per axis**, so that is
        // what is written: the table's reference value, the size the pattern's measurements are
        // written against. The run that prints the page knows the size it was asked for, and will
        // hand it over the day a cover page is printed from a run at all.
        for (const QString &axis : axes)
        {
            if (!table->AxisNames().contains(axis))
            {
                continue; // an axis the current table does not declare: dead data, no line
            }

            // **An axis the page does not grade stands where the table says it stands.** A range
            // says which sizes run; it says nothing about the axes that do not, and the table's own
            // reference value is the answer a page states when nothing tells it otherwise — the
            // same value a page stating a single size states.
            const QString original = table->AnchorValue(axis);
            if (original.isEmpty())
            {
                continue;
            }
            const int index = static_cast<int>(table->AxisValues(axis).indexOf(original));

            QStringList languageLines;
            for (const QString &locale : RenderLanguages(languages))
            {
                const QString alias = AxisAlias(renames, locale, axis);
                const QString shownAxis = alias.isEmpty() ? axis : alias;

                QString shownValue = original;
                const QVariantList aliases = ValueAliases(renames, locale, axis);
                if (index >= 0 && index < aliases.size() && !aliases.at(index).toString().isEmpty())
                {
                    shownValue = aliases.at(index).toString();
                }
                languageLines.append(
                    OTemplateSeparatorOptions::Compose(shownAxis, shownValue, separator));
            }

            if (oneLine)
            {
                for (const QString &text : languageLines)
                {
                    lines.append(OCartouchePainter::TextLines(text, axisFont, axisSpacing));
                }
            }
            else
            {
                // The languages of one axis on a single line, held apart by the reserved « - » — the
                // one character the axis separator is never offered, so that this line can still be
                // read back into the two things it was made of.
                lines.append(OCartouchePainter::TextLine(
                    languageLines.join(OTemplateSeparatorOptions::kLanguageJoin), axisFont, axisSpacing));
            }
        }

        // **The sizes the range holds come last, under the axes that stand still.** The page reads
        // top to bottom as the range is described: what it is written for, then those axes of the
        // body that do not move, then the sizes it comes in — the list the page exists for, and the
        // one a reader looks for after knowing what is being listed.
        //
        // One line per size, each with a sample of its own trace. The word is the one the page
        // shows — the table's, or the alias the user wrote for it — and the sample is the ink and
        // the dash pitch that size is drawn with, over the length the user asked for.
        //
        // A page with no range chosen yet lists nothing, and says so by drawing nothing there: the
        // editor's block is what asks the user for the range, and a page cannot ask.
        if (listsRange)
        {
            const QFont sizesFont =
                OCartouchePainter::SectionFont(section.options, baseFont, QStringLiteral("sizes"));
            const int sizesSpacing =
                section.options.value(QStringLiteral("sizesLineSpacing"), 0).toInt();
            const qreal sampleLength = CmToPixel(
                OCoverGradedAxisOptions::FromOptions(section.options).traceLengthCm);

            for (const OCoverSizeLine &size : data.sizes)
            {
                OCartoucheTraceSample sample;
                sample.enabled = true;
                sample.lengthPx = sampleLength;
                sample.color = size.color;
                sample.dashes = size.dashes;
                sample.penWidth = OCoverSizeLine::kTracePenWidthPx;
                lines.append(OCartouchePainter::TextLineWithTrace(size.shown, sizesFont, sizesSpacing,
                                                                  sample));
            }
        }

        return lines;
    }

    // An information the catalogue declares and nothing here draws — the ones reading a size table,
    // a measurement or a piece, none of which a cover page has yet.
    return {};
}
