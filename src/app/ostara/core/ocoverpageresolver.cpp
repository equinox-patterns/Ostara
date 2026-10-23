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
#include "olengthunits.h"
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

/// @brief One width stated in every unit the user asked for, held apart by what they chose.
///
/// **With the air a written line needs.** A separator between two values is read at a glance, and
/// `1 cm/3/8"` reads as one token where `1 cm / 3/8"` reads as two. The user picks the sign; how it
/// is spaced is not a setting, because there is only one answer that can be read.
auto StatedApart(const QStringList &units, const QString &separator) -> QString
{
    if (units.isEmpty())
    {
        return QString();
    }
    if (separator.isEmpty())
    {
        return units.join(QLatin1Char(' '));
    }
    return units.join(QStringLiteral(" ") + separator + QStringLiteral(" "));
}

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
        // Nothing to draw while the editor is drawing the template - no pattern is loaded, so the
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

        // **The stroke is drawn inside the square, not across its edge.** A square test is a
        // reference someone holds a ruler against, and a ruler meets the OUTSIDE of the line:
        // drawn centred on its path, the printed square came out at the side asked for **plus one
        // pen** — 0.26 mm of paper, since a unit of the file is a dot at 96 dpi. Half a percent of
        // a five centimetre square, for a tool whose whole job is to show whether the printer held
        // its scale. The path is therefore pulled in by half a pen on every side, which puts the
        // ink's outer edge exactly on the side asked for. The inner edge is then half a pen short
        // of it, and that is the trade a stroke forces: both edges cannot be the size.
        const qreal stroke = OCartouchePainter::kShapePenWidth;
        for (const TestSquare &square : squares)
        {
            const QRectF outline(frame.center().x() - square.side / 2.0 + stroke / 2.0,
                                 frame.center().y() - square.side / 2.0 + stroke / 2.0,
                                 square.side - stroke, square.side - stroke);
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

    if (section.typeId == QLatin1String("symbols"))
    {
        // What stands between the languages of the title and of each legend line. One word, and the
        // same one the sizes' title is joined by: a page that separated the same two things
        // differently in two of its informations would be two formats.
        const QString languageSeparator =
            section.options.value(OTemplateSeparatorOptions::kLanguageSeparator).toString();

        QVector<OCartoucheRenderLine> lines;

        // The title, renamed or not. **A legend is the one information whose own word may have to say
        // what this pattern means by it**: "Symbols" is what the application calls it, and a pattern
        // that reads those very marks differently says so here.
        const QVariantMap renames = section.options.value(OTemplateAxisOptions::kRenames).toMap();
        const auto TitleFor = [&renames](const QString &locale) -> QString
        {
            // Keyed by language, and read exactly as an axis alias is: an empty one means the word was
            // not changed, so the application's own stands.
            const QString written = renames.value(OCoverSymbols::kTitleKey)
                                        .toMap()
                                        .value(locale)
                                        .toMap()
                                        .value(QStringLiteral("name"))
                                        .toString();
            return written.isEmpty() ? OCoverSymbols::TitleWord(locale) : written;
        };

        QStringList title;
        if (languages.isEmpty())
        {
            title.append(TitleFor(QString()));
        }
        for (const QString &locale : languages)
        {
            title.append(TitleFor(locale));
        }
        if (!title.isEmpty())
        {
            // **One line, however many languages**: a title is a word, and the same word repeated down
            // the page would read as so many titles. What holds them apart is the separator the user
            // chose — and with « None » it is the space that was always there.
            lines.append(OCartouchePainter::TextLines(
                OTemplateSeparatorOptions::Join(title, languageSeparator), font, lineSpacing));
        }

        // Under it, one line per symbol, in the library's order. **The marks stand before the words on
        // the same line**, which is what a legend is: the shape, then what it means.
        //
        // The meanings are held apart by a separator of their own: a title and a line of the legend
        // are two different sentences, and a page that joined them the same way would be saying the
        // two are the same kind of thing.
        const QString symbolsSeparator =
            section.options.value(QStringLiteral("symbolsLanguageSeparator")).toString();
        const QFont symbolsFont =
            OCartouchePainter::SectionFont(section.options, baseFont, QStringLiteral("symbols"));
        const int symbolsSpacing = section.options.value(QStringLiteral("symbolsLineSpacing"), 0).toInt();

        for (const OCoverSymbols::Line &symbol : data.symbols)
        {
            lines.append(OCartouchePainter::TextLineWithSymbols(
                OTemplateSeparatorOptions::Join(symbol.texts, symbolsSeparator), symbolsFont,
                symbolsSpacing, symbol.marks));
        }

        return lines;
    }

    if (section.typeId == QLatin1String("seamAllowances"))
    {
        // **Nothing is drawn until the question is answered.** The two cases read differently, and
        // an information that guessed which one was meant would be printing a sentence its author
        // never chose — so it draws nothing at all, as it does for a logo nobody picked.
        const QString seamCase = section.options.value(OCoverSeamAllowances::kCaseKey).toString();
        if (seamCase.isEmpty())
        {
            return {};
        }

        // What stands between the languages of the heading and of each margin's name. The shared
        // separator for the heading, and one of the information's own for the names: a heading and a
        // line of margins are two different sentences.
        const QString languageSeparator =
            section.options.value(OTemplateSeparatorOptions::kLanguageSeparator).toString();

        QVector<OCartoucheRenderLine> lines;

        // The heading, renamed or not — and **the two cases are titled separately**, because a page
        // that says the allowances are included says something else than one that says they are not.
        const QVariantMap renames = section.options.value(OTemplateAxisOptions::kRenames).toMap();
        const QString titleKey = OCoverSeamAllowances::TitleKey(seamCase);
        const auto TitleFor = [&renames, &titleKey, &seamCase](const QString &locale) -> QString
        {
            // Keyed by language, and read exactly as an axis alias is: an empty one means the words
            // were not changed, so the application's own stand.
            const QString written = renames.value(titleKey)
                                        .toMap()
                                        .value(locale)
                                        .toMap()
                                        .value(QStringLiteral("name"))
                                        .toString();
            return written.isEmpty() ? OCoverSeamAllowances::TitleWord(seamCase, locale) : written;
        };

        QStringList title;
        if (languages.isEmpty())
        {
            title.append(TitleFor(QString()));
        }
        for (const QString &locale : languages)
        {
            title.append(TitleFor(locale));
        }

        if (!title.isEmpty())
        {
            // **One line per language when the user asked for it.** These headings are sentences —
            // « Seam allowances not included » — and two of them held on one line by a separator is
            // a line a reader has to take apart to find where one language stops.
            if (section.options.value(OTemplateAxisOptions::kOneLine, false).toBool())
            {
                for (const QString &word : title)
                {
                    lines.append(OCartouchePainter::TextLine(word, font, lineSpacing));
                }
            }
            else
            {
                lines.append(OCartouchePainter::TextLines(
                    OTemplateSeparatorOptions::Join(title, languageSeparator), font, lineSpacing));
            }
        }

        // **Under it, the margins — and only in the case that has any.** A page stating that the
        // allowances are not part of the drawings has said everything it has to say; the widths it
        // would name are precisely what the reader is being told not to look for.
        if (seamCase == OCoverSeamAllowances::kIncluded)
        {
            const QFont valuesFont = OCartouchePainter::SectionFont(section.options, baseFont,
                                                                    QStringLiteral("seamValues"));
            const int valuesSpacing =
                section.options.value(QStringLiteral("seamValuesLineSpacing"), 0).toInt();
            const QString namesSeparator =
                section.options.value(OCoverSeamAllowances::kValuesLanguageSeparatorKey).toString();
            const QString valueSeparator =
                section.options.value(OTemplateSeparatorOptions::kSeparator).toString();
            const QStringList units =
                section.options.value(OCoverSeamAllowances::kUnitsKey).toStringList();
            const QString unitSeparator =
                section.options.value(OCoverSeamAllowances::kUnitSeparatorKey).toString();

            for (const OCoverSeamAllowances::Line &margin : data.seamAllowances)
            {
                // **The same width, said in every unit the user asked for** — « 1 cm / 3/8" » — which
                // is what lets one page be read by two people who measure differently.
                QStringList stated;
                for (const QString &unit : units)
                {
                    stated.append(OLengthUnits::Format(margin.mm, StrToUnits(unit)));
                }

                lines.append(OCartouchePainter::TextLine(
                    OTemplateSeparatorOptions::Compose(
                        OTemplateSeparatorOptions::Join(margin.texts, namesSeparator),
                        StatedApart(stated, unitSeparator), valueSeparator),
                    valuesFont, valuesSpacing));
            }
        }

        return lines;
    }

    if (section.typeId == QLatin1String("space"))
    {
        // **A page has nothing to say here, and that is the information.** An empty space is room
        // the user asked for between two things, and what it resolves to is a line that draws
        // nothing and takes exactly that room — see OCartouchePainter::SpaceLine. It is a line and
        // not a margin because the room has to be counted by everything that measures a band, and
        // because it moves with the page when the page is drawn at another size.
        //
        // A height of nothing is no line at all: a space left at zero would otherwise be a gap
        // nobody chose, and the preview would have nothing to show for the section.
        const qreal height = section.options.value(QStringLiteral("spaceHeight"), 0.0).toDouble();
        if (height <= 0.0)
        {
            return {};
        }

        return {OCartouchePainter::SpaceLine(height)};
    }

    if (section.typeId == QLatin1String("sizes"))
    {
        const QString sizeCase =
            section.options.value(QStringLiteral("sizeCase"), OTemplateSizeCase::kSingle).toString();
        // Which of the two the page is: one that states a size, or one that lists the sizes it
        // holds. The answer decides the title, whether the sizes are listed, and where the axes
        // that do not run take their value.
        //
        // **The run answers at export, the designer only in the preview.** The case is a fact about
        // the output (`OTemplateSizeCase` says so itself), so a page designed for ranges printed
        // from a single-size run cannot go on listing sizes the paper does not carry.
        const bool listsRange = data.listsRange.value_or(sizeCase != OTemplateSizeCase::kSingle);
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
        // **A page printed from a single size can only be about one value per axis**, and that
        // value is the one the run was asked for — see `PatternData::axisValues`.
        for (const QString &axis : axes)
        {
            if (!table->AxisNames().contains(axis))
            {
                continue; // an axis the current table does not declare: dead data, no line
            }

            // **An axis stands where the RUN put it, and the table answers only when nobody did.**
            //
            // A range says which sizes run; it says nothing about the axes that do not, and those
            // are frozen where the run pinned them. A single-size run pins all of them. `axisValues`
            // is that answer, handed over by whoever ran the nesting — see the nomenclature spec §6,
            // and PatternData::axisValues.
            //
            // Falling back to the table's own reference value is the preview's case, and the only
            // honest answer left: a page showing nothing where an axis stands would be showing less
            // than the table knows.
            const QString original = data.axisValues.value(axis, table->AnchorValue(axis));
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
