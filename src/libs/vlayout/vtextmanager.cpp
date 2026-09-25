/************************************************************************
 **
 **  @file   vpatternpiecedata.cpp
 **  @author Bojan Kverh
 **  @date   July 19, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include <QApplication>
#include <QDate>
#include <QDebug>
#include <QFileInfo>
#include <QFlags> // QFlags<Qt::Alignment>
#include <QFontMetrics>
#include <QLatin1String>
#include <QtMath>

#include "../qmuparser/qmutokenparser.h"
#include "../vmisc/svgfont/vsvgfont.h"
#include "../vmisc/svgfont/vsvgfontdatabase.h"
#include "../vmisc/svgfont/vsvgfontengine.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vtranslator.h"
#include "../vpatterndb/calculator.h"
#include "otextwrap.h"
#include "vtextmanager.h"

using namespace Qt::Literals::StringLiterals;

const quint32 TextLine::streamHeader = 0xA3881E49; // CRC-32Q string "TextLine"
const quint16 TextLine::classVersion = 1;

// Friend functions
//---------------------------------------------------------------------------------------------------------------------
auto operator<<(QDataStream &dataStream, const TextLine &data) -> QDataStream &
{
    dataStream << TextLine::streamHeader << TextLine::classVersion;

    // Added in classVersion = 1
    dataStream << data.qsText;
    dataStream << data.iFontSize;
    dataStream << data.bold;
    dataStream << data.italic;
    dataStream << data.eAlign;

    // Added in classVersion = 2

    return dataStream;
}

//---------------------------------------------------------------------------------------------------------------------
auto operator>>(QDataStream &dataStream, TextLine &data) -> QDataStream &
{
    quint32 actualStreamHeader = 0;
    dataStream >> actualStreamHeader;

    if (actualStreamHeader != TextLine::streamHeader)
    {
        QString const message = QCoreApplication::tr("TextLine prefix mismatch error: actualStreamHeader = 0x%1 and "
                                                     "streamHeader = 0x%2")
                                    .arg(actualStreamHeader, 8, 0x10, '0'_L1)
                                    .arg(TextLine::streamHeader, 8, 0x10, '0'_L1);
        throw VException(message);
    }

    quint16 actualClassVersion = 0;
    dataStream >> actualClassVersion;

    if (actualClassVersion > TextLine::classVersion)
    {
        QString const message = QCoreApplication::tr("TextLine compatibility error: actualClassVersion = %1 and "
                                                     "classVersion = %2")
                                    .arg(actualClassVersion)
                                    .arg(TextLine::classVersion);
        throw VException(message);
    }

    dataStream >> data.qsText;
    dataStream >> data.iFontSize;
    dataStream >> data.bold;
    dataStream >> data.italic;
    dataStream >> data.eAlign;

    //    if (actualClassVersion >= 2)
    //    {

    //    }

    return dataStream;
}

const quint32 VTextManager::streamHeader = 0x47E6A9EE; // CRC-32Q string "VTextManager"
const quint16 VTextManager::classVersion = 2;

// Friend functions
//---------------------------------------------------------------------------------------------------------------------
auto operator<<(QDataStream &dataStream, const VTextManager &data) -> QDataStream &
{
    dataStream << VTextManager::streamHeader << VTextManager::classVersion;

    // Added in classVersion = 1
    dataStream << data.m_font;
    dataStream << data.m_liLines;

    // Added in classVersion = 2
    dataStream << data.m_svgFontFamily;
    dataStream << data.m_svgFontPointSize;

    return dataStream;
}

//---------------------------------------------------------------------------------------------------------------------
auto operator>>(QDataStream &dataStream, VTextManager &data) -> QDataStream &
{
    quint32 actualStreamHeader = 0;
    dataStream >> actualStreamHeader;

    if (actualStreamHeader != VTextManager::streamHeader)
    {
        QString const message =
            QCoreApplication::tr("VTextManager prefix mismatch error: actualStreamHeader = 0x%1 and "
                                 "streamHeader = 0x%2")
                .arg(actualStreamHeader, 8, 0x10, '0'_L1)
                .arg(VTextManager::streamHeader, 8, 0x10, '0'_L1);
        throw VException(message);
    }

    quint16 actualClassVersion = 0;
    dataStream >> actualClassVersion;

    if (actualClassVersion > VTextManager::classVersion)
    {
        QString const message = QCoreApplication::tr("VTextManager compatibility error: actualClassVersion = %1 and "
                                                     "classVersion = %2")
                                    .arg(actualClassVersion)
                                    .arg(VTextManager::classVersion);
        throw VException(message);
    }

    dataStream >> data.m_font;
    dataStream >> data.m_liLines;

    if (actualClassVersion >= 2)
    {
        dataStream >> data.m_svgFontFamily;
        dataStream >> data.m_svgFontPointSize;
    }

    return dataStream;
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSpacing returns the vertical spacing between the lines
 * @return spacing
 */
auto VTextManager::GetSpacing() const -> int
{
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFont set the text base font
 * @param font text base font
 */
void VTextManager::SetFont(const QFont &font)
{
    m_font = font;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetFont returns the text base font
 * @return text base font
 */
auto VTextManager::GetFont() const -> const QFont &
{
    return m_font;
}

//---------------------------------------------------------------------------------------------------------------------
void VTextManager::SetSVGFontFamily(const QString &fontFamily)
{
    m_svgFontFamily = fontFamily;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::GetSVGFontFamily() const -> QString
{
    return m_svgFontFamily;
}

//---------------------------------------------------------------------------------------------------------------------
void VTextManager::SetSVGFontPointSize(int pointSize)
{
    m_svgFontPointSize = pointSize < VCommonSettings::MinPieceLabelFontPointSize()
                             ? VCommonSettings::MinPieceLabelFontPointSize()
                             : pointSize;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::GetSVGFontPointSize() const -> int
{
    return m_svgFontPointSize;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFontSize sets the font size
 * @param iFS font size in pixels
 */
void VTextManager::SetFontSize(int iFS)
{
    iFS < VCommonSettings::MinPieceLabelFontPointSize()
        ? m_font.setPointSize(qMax(VCommonSettings::MinPieceLabelFontPointSize(), 1))
        : m_font.setPointSize(qMax(iFS, 1));
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::GetAllSourceLines() const -> QVector<TextLine>
{
    return m_liLines;
}

//---------------------------------------------------------------------------------------------------------------------
void VTextManager::SetAllSourceLines(const QVector<TextLine> &lines)
{
    m_liLines = lines;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VTextManager::GetSourceLinesCount returns the number of input text lines
 * @return number of text lines that were added to the list by calling AddLine
 */
auto VTextManager::GetSourceLinesCount() const -> vsizetype
{
    return m_liLines.count();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VTextManager::GetSourceLine returns the reference to i-th text line
 * @param i index of the requested line
 * @return reference to the requested TextLine object
 */
auto VTextManager::GetSourceLine(vsizetype i) const -> const TextLine &
{
    Q_ASSERT(i >= 0);
    Q_ASSERT(i < m_liLines.count());
    return m_liLines.at(i);
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::GetLabelSourceLines(int width, const QFont &font) const -> QVector<TextLine>
{
    QVector<TextLine> lines;
    lines.reserve(m_liLines.size());
    QFont fnt = font;
    int const fSize = m_font.pointSize();

    for (const auto &tl : m_liLines)
    {
        fnt.setPointSize(qMax(fSize + tl.iFontSize, 1));
        if (!VAbstractApplication::VApp()->Settings()->GetSingleStrokeOutlineFont())
        {
            fnt.setBold(tl.bold);
        }
        fnt.setItalic(tl.italic);

        QString const qsText = tl.qsText;
        if (HorizontalAdvance(qsText, fnt) > width)
        {
            const QStringList brokeLines = BreakTextIntoLines(qsText, fnt, width);
            for (const auto &lineText : brokeLines)
            {
                TextLine line = tl;
                line.qsText = lineText;
                lines.append(line);
            }
        }
        else
        {
            lines.append(tl);
        }
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::GetLabelSourceLines(int width, const VSvgFont &font, qreal penWidth) const -> QVector<TextLine>
{
    if (!font.IsValid())
    {
        return m_liLines;
    }

    VSvgFontDatabase  const*db = VAbstractApplication::VApp()->SVGFontDatabase();
    QVector<TextLine> lines;
    lines.reserve(m_liLines.size());
    int const fSize = m_font.pointSize();

    for (const auto &tl : m_liLines)
    {
        VSvgFont lineFont = font;
        lineFont.SetPointSize(fSize + tl.iFontSize);
        lineFont.SetBold(tl.bold);
        lineFont.SetItalic(tl.italic);

        VSvgFontEngine const engine = db->FontEngine(lineFont);

        VSvgFont const svgFont = engine.Font();
        if (!svgFont.IsValid())
        {
            lines.append(tl);
            continue;
        }

        QString const qsText = tl.qsText;
        if (engine.TextWidth(qsText, penWidth) > width)
        {
            const QStringList brokeLines = BreakTextIntoLines(qsText, svgFont, width, penWidth);
            for (const auto &lineText : brokeLines)
            {
                TextLine line = tl;
                line.qsText = lineText;
                lines.append(line);
            }
        }
        else
        {
            lines.append(tl);
        }
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::BreakTextIntoLines(const QString &text, const QFont &font, int maxWidth) -> QStringList
{
    // Le repli lui-même vit dans OTextWrap. Ce qui reste ici est la seule chose que ce moteur
    // ajoute : la façon dont l'application mesure un texte, qui dépend du réglage « police
    // filaire ». Un mot se mesure avec cette convention, un caractère avec les métriques brutes —
    // exactement comme avant l'extraction.
    QFontMetrics const metrics(font);
    OTextWrap::Measure measure;
    measure.word = [&font](const QString &value) -> qreal { return HorizontalAdvance(value, font); };
    measure.part = [metrics](const QString &value) -> qreal { return metrics.horizontalAdvance(value); };
    return OTextWrap::Lines(text, maxWidth, measure);
}

//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::BreakTextIntoLines(const QString &text, const VSvgFont &font, int maxWidth, qreal penWidth) const
    -> QStringList
{
    VSvgFontDatabase  const*db = VAbstractApplication::VApp()->SVGFontDatabase();

    VSvgFontEngine const engine = db->FontEngine(font);

    VSvgFont const svgFont = engine.Font();
    if (!svgFont.IsValid())
    {
        return {text};
    }

    // Le repli vit dans OTextWrap ; ici on ne dit que comment un moteur de police de tracé mesure.
    return OTextWrap::Lines(text, maxWidth, OTextWrap::EngineMeasure(engine, penWidth));
}


//---------------------------------------------------------------------------------------------------------------------
auto VTextManager::HorizontalAdvance(const QString &text, const QFont &font) -> int
{
    const VCommonSettings *settings = VAbstractApplication::VApp()->Settings();
    qreal const penWidth = VAbstractApplication::VApp()->Settings()->WidthHairLine();

    QFontMetrics const fm(font);
    if (settings->GetSingleStrokeOutlineFont())
    {
        int w = 0;
        for (auto c : std::as_const(text))
        {
            w += fm.horizontalAdvance(c) + qRound(penWidth / 2.0);
        }

        return w;
    }
    return fm.horizontalAdvance(text);
}
