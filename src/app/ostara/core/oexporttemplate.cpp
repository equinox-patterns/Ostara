/************************************************************************
 **
 **  @file   oexporttemplate.cpp
 **
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  Reading and writing a template as XML.
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

#include "oexporttemplate.h"

#include "../vmisc/def.h"

#include <QDomDocument>
#include <QStringList>

namespace
{
const QString kTagRoot = QStringLiteral("assemblyMarkTemplate");
const QString kAttrVersion = QStringLiteral("version");
const QString kFormatVersion = QStringLiteral("1.0");

const QString kTagName = QStringLiteral("name");
const QString kTagFrame = QStringLiteral("frame");
const QString kTagMarks = QStringLiteral("marks");
const QString kTagPagination = QStringLiteral("pagination");

const QString kAttrWidthPt = QStringLiteral("widthPt");
const QString kAttrDashed = QStringLiteral("dashed");
const QString kAttrDashes = QStringLiteral("dashes");
const QString kAttrOpacity = QStringLiteral("opacity");
const QString kAttrScissorsHorizontal = QStringLiteral("scissorsHorizontal");
const QString kAttrScissorsVertical = QStringLiteral("scissorsVertical");
const QString kAttrScissorsOpacity = QStringLiteral("scissorsOpacity");

const QString kAttrShape = QStringLiteral("shape");
const QString kAttrSizeMm = QStringLiteral("sizeMm");
const QString kAttrSizeUnit = QStringLiteral("sizeUnit");
const QString kAttrCorner = QStringLiteral("corner");
const QString kAttrVerticalCentre = QStringLiteral("verticalCentre");
const QString kAttrHorizontalCentre = QStringLiteral("horizontalCentre");

const QString kAttrForm = QStringLiteral("form");
const QString kTagCentre = QStringLiteral("centre");
const QString kTagCorner = QStringLiteral("corner");
const QString kTagVerticalCentre = QStringLiteral("verticalCentre");
const QString kTagHorizontalCentre = QStringLiteral("horizontalCentre");
const QString kAttrEnabled = QStringLiteral("enabled");
const QString kAttrFontFamily = QStringLiteral("fontFamily");
const QString kAttrFontWeight = QStringLiteral("fontWeight");
const QString kAttrFontSize = QStringLiteral("fontSize");

//---------------------------------------------------------------------------------------------------------------------
// Reading. Each of these takes the value the caller already holds and gives it back untouched when the attribute is
// absent or unreadable — which is what makes a file written by an older build, or edited by hand, open on its defaults
// rather than on nothing.

auto ReadText(const QDomElement &element, const QString &name, const QString &fallback) -> QString
{
    return element.hasAttribute(name) ? element.attribute(name) : fallback;
}

auto ReadFlag(const QDomElement &element, const QString &name, bool fallback) -> bool
{
    if (!element.hasAttribute(name))
    {
        return fallback;
    }

    const QString text = element.attribute(name);
    return text == QLatin1String("true") || text == QLatin1String("1");
}

auto ReadInt(const QDomElement &element, const QString &name, int fallback) -> int
{
    bool ok = false;
    const int value = element.attribute(name).toInt(&ok);
    return ok ? value : fallback;
}

auto ReadReal(const QDomElement &element, const QString &name, qreal fallback) -> qreal
{
    bool ok = false;
    const qreal value = element.attribute(name).toDouble(&ok);
    return ok ? value : fallback;
}

//---------------------------------------------------------------------------------------------------------------------
// The two enums that travel as tokens rather than as numbers. A token survives an enum gaining a value in the middle;
// an index does not. Both tables are read in their own order, which is the order their enums declare.

auto ShapeToken(OExportMarkShape shape) -> QString
{
    const QVector<OExportMarkShapeModel> &shapes = OExportMarkShapes();
    const int index = static_cast<int>(shape);
    if (index < 0 || index >= shapes.size())
    {
        return QLatin1String(shapes.constFirst().token);
    }
    return QLatin1String(shapes.at(index).token);
}

auto ShapeFromToken(const QString &token, OExportMarkShape fallback) -> OExportMarkShape
{
    const QVector<OExportMarkShapeModel> &shapes = OExportMarkShapes();
    for (int index = 0; index < shapes.size(); ++index)
    {
        if (token == QLatin1String(shapes.at(index).token))
        {
            return static_cast<OExportMarkShape>(index);
        }
    }
    return fallback;
}

auto FormToken(OExportPaginationForm form) -> QString
{
    const QVector<OExportPaginationFormModel> &forms = OExportPaginationForms();
    const int index = static_cast<int>(form);
    if (index < 0 || index >= forms.size())
    {
        return QLatin1String(forms.constFirst().token);
    }
    return QLatin1String(forms.at(index).token);
}

auto FormFromToken(const QString &token, OExportPaginationForm fallback) -> OExportPaginationForm
{
    const QVector<OExportPaginationFormModel> &forms = OExportPaginationForms();
    for (int index = 0; index < forms.size(); ++index)
    {
        if (token == QLatin1String(forms.at(index).token))
        {
            return static_cast<OExportPaginationForm>(index);
        }
    }
    return fallback;
}

//---------------------------------------------------------------------------------------------------------------------
// The text style, written and read in one place so that the four placements cannot drift apart.

void WriteTextStyle(QDomElement &element, const OExportPaginationTextStyle &style)
{
    // An empty family is not written at all: it means the default face, and writing "" would say
    // the same thing in a way a reader has to know about.
    if (!style.fontFamily.isEmpty())
    {
        element.setAttribute(kAttrFontFamily, style.fontFamily);
    }
    element.setAttribute(kAttrFontWeight, QString::number(style.weight));
    element.setAttribute(kAttrFontSize, QString::number(style.sizePx));
    element.setAttribute(kAttrOpacity, QString::number(style.opacity));
}

void ReadTextStyle(const QDomElement &element, OExportPaginationTextStyle &style)
{
    style.fontFamily = ReadText(element, kAttrFontFamily, style.fontFamily);
    style.weight = ReadInt(element, kAttrFontWeight, style.weight);
    style.sizePx = ReadInt(element, kAttrFontSize, style.sizePx);
    style.opacity = ReadInt(element, kAttrOpacity, style.opacity);
}

void WritePlacement(QDomDocument &doc, QDomElement &parent, const QString &tag, bool enabled,
                    const OExportPaginationTextStyle &style)
{
    QDomElement element = doc.createElement(tag);
    element.setAttribute(kAttrEnabled, enabled);
    WriteTextStyle(element, style);
    parent.appendChild(element);
}

void ReadPlacement(const QDomElement &parent, const QString &tag, bool &enabled,
                   OExportPaginationTextStyle &style)
{
    const QDomElement element = parent.firstChildElement(tag);
    if (element.isNull())
    {
        return; // absent: the placement keeps the default it was given
    }

    enabled = ReadFlag(element, kAttrEnabled, enabled);
    ReadTextStyle(element, style);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplate::ToXml(QDomDocument &doc) const -> QDomElement
{
    QDomElement root = doc.createElement(kTagRoot);
    root.setAttribute(kAttrVersion, kFormatVersion);

    QDomElement nameElement = doc.createElement(kTagName);
    nameElement.appendChild(doc.createTextNode(name));
    root.appendChild(nameElement);

    QDomElement frameElement = doc.createElement(kTagFrame);
    frameElement.setAttribute(kAttrWidthPt, QString::number(frame.widthPt));
    frameElement.setAttribute(kAttrDashed, frame.dashed);
    if (!frame.dashes.isEmpty())
    {
        QStringList pattern;
        pattern.reserve(frame.dashes.size());
        for (qreal value : frame.dashes)
        {
            pattern.append(QString::number(value));
        }
        frameElement.setAttribute(kAttrDashes, pattern.join(QLatin1Char(' ')));
    }
    frameElement.setAttribute(kAttrOpacity, QString::number(frame.frameOpacity));
    frameElement.setAttribute(kAttrScissorsHorizontal, frame.scissorsHorizontal);
    frameElement.setAttribute(kAttrScissorsVertical, frame.scissorsVertical);
    frameElement.setAttribute(kAttrScissorsOpacity, QString::number(frame.scissorsOpacity));
    root.appendChild(frameElement);

    QDomElement marksElement = doc.createElement(kTagMarks);
    marksElement.setAttribute(kAttrShape, ShapeToken(marks.shape));
    marksElement.setAttribute(kAttrSizeMm, QString::number(marks.sizeMm));
    // The stored length never leaves millimetres; this is only the unit the field shows it in.
    marksElement.setAttribute(kAttrSizeUnit, UnitsToStr(marks.sizeUnit));
    marksElement.setAttribute(kAttrOpacity, QString::number(marks.opacity));
    marksElement.setAttribute(kAttrCorner, marks.onCorner);
    marksElement.setAttribute(kAttrVerticalCentre, marks.onVerticalCentre);
    marksElement.setAttribute(kAttrHorizontalCentre, marks.onHorizontalCentre);
    root.appendChild(marksElement);

    QDomElement paginationElement = doc.createElement(kTagPagination);
    paginationElement.setAttribute(kAttrForm, FormToken(pagination.form));
    WritePlacement(doc, paginationElement, kTagCentre, pagination.centre.enabled,
                   pagination.centre.style);
    WritePlacement(doc, paginationElement, kTagCorner, pagination.corner.enabled,
                   pagination.corner.style);
    WritePlacement(doc, paginationElement, kTagVerticalCentre, pagination.verticalCentre.enabled,
                   pagination.verticalCentre.style);
    WritePlacement(doc, paginationElement, kTagHorizontalCentre, pagination.horizontalCentre.enabled,
                   pagination.horizontalCentre.style);
    root.appendChild(paginationElement);

    return root;
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplate::LoadFromXml(const QDomElement &root) -> bool
{
    if (root.tagName() != kTagRoot)
    {
        return false;
    }

    // Read over a fresh template rather than over whatever was here: every setting the file does
    // not carry takes its default, and nothing of a previously loaded template survives.
    const OExportTemplate defaults;
    *this = defaults;

    name = root.firstChildElement(kTagName).text();

    const QDomElement frameElement = root.firstChildElement(kTagFrame);
    if (!frameElement.isNull())
    {
        frame.widthPt = ReadReal(frameElement, kAttrWidthPt, frame.widthPt);
        frame.dashed = ReadFlag(frameElement, kAttrDashed, frame.dashed);
        frame.frameOpacity = ReadInt(frameElement, kAttrOpacity, frame.frameOpacity);
        frame.scissorsHorizontal =
            ReadFlag(frameElement, kAttrScissorsHorizontal, frame.scissorsHorizontal);
        frame.scissorsVertical =
            ReadFlag(frameElement, kAttrScissorsVertical, frame.scissorsVertical);
        frame.scissorsOpacity =
            ReadInt(frameElement, kAttrScissorsOpacity, frame.scissorsOpacity);

        if (frameElement.hasAttribute(kAttrDashes))
        {
            QVector<qreal> pattern;
            const QStringList parts =
                frameElement.attribute(kAttrDashes).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (const QString &part : parts)
            {
                bool ok = false;
                const qreal value = part.toDouble(&ok);
                if (ok)
                {
                    pattern.append(value);
                }
            }
            if (!pattern.isEmpty())
            {
                frame.dashes = pattern;
            }
        }
    }

    const QDomElement marksElement = root.firstChildElement(kTagMarks);
    if (!marksElement.isNull())
    {
        marks.shape = ShapeFromToken(ReadText(marksElement, kAttrShape, ShapeToken(marks.shape)),
                                     marks.shape);
        marks.sizeMm = ReadReal(marksElement, kAttrSizeMm, marks.sizeMm);
        marks.sizeUnit = StrToUnits(ReadText(marksElement, kAttrSizeUnit,
                                             UnitsToStr(marks.sizeUnit)));
        marks.opacity = ReadInt(marksElement, kAttrOpacity, marks.opacity);
        marks.onCorner = ReadFlag(marksElement, kAttrCorner, marks.onCorner);
        marks.onVerticalCentre =
            ReadFlag(marksElement, kAttrVerticalCentre, marks.onVerticalCentre);
        marks.onHorizontalCentre =
            ReadFlag(marksElement, kAttrHorizontalCentre, marks.onHorizontalCentre);
    }

    const QDomElement paginationElement = root.firstChildElement(kTagPagination);
    if (!paginationElement.isNull())
    {
        pagination.form = FormFromToken(
            ReadText(paginationElement, kAttrForm, FormToken(pagination.form)), pagination.form);
        ReadPlacement(paginationElement, kTagCentre, pagination.centre.enabled,
                      pagination.centre.style);
        ReadPlacement(paginationElement, kTagCorner, pagination.corner.enabled,
                      pagination.corner.style);
        ReadPlacement(paginationElement, kTagVerticalCentre, pagination.verticalCentre.enabled,
                      pagination.verticalCentre.style);
        ReadPlacement(paginationElement, kTagHorizontalCentre, pagination.horizontalCentre.enabled,
                      pagination.horizontalCentre.style);
    }

    return true;
}
