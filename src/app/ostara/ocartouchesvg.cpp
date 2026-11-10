/************************************************************************
 **
 **  @file   ocartouchesvg.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  Serializing a placed cartouche item, written once for every
 **          document that carries one.
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

#include "ocartouchesvg.h"

#include "core/ologorender.h"
#include "osvgutils.h"

#include <QFile>
#include <QFontInfo>
#include <QImageReader>
#include <QPainterPath>
#include <QPen>
#include <QRegularExpression>
#include <QTransform>
#include <QXmlStreamAttribute>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace
{
/**
 * @brief Writes a raster logo as one `<image>` carrying its own bytes in a data URI.
 *
 * **Base64 rather than a link into the library**, for the reason a drawing is inlined: an exported
 * plan is expected to stand on its own, and a plan that shows its logo only while the file is
 * still in `~/Ostara/Logos` is a plan somebody else cannot open.
 *
 * **The value is never wrapped.** XML turns a newline inside an attribute into a space, and a
 * space in the middle of base64 is a corrupt picture — so it goes out in one piece, however long.
 *
 * The declared type is the one Qt reads the file as, not the one its name suggests: a `.png` that
 * is really a JPEG has to go out as `image/jpeg` for a reader to draw it.
 */
void WriteEmbeddedPicture(QXmlStreamWriter &writer, const QByteArray &bytes, const QString &path,
                          const QRectF &target)
{
    const QByteArray format = QImageReader::imageFormat(path);
    const QString mime = format.isEmpty() ? QStringLiteral("image/png")
                                         : QStringLiteral("image/") + QString::fromLatin1(format);

    writer.writeStartElement(QStringLiteral("image"));
    // Declared on the element that uses it rather than on the three roots this writer serves:
    // `xlink:href` is the form every reader of an SVG still understands, Qt's own renderer
    // included (it reads that name first, and the plain `href` only after it).
    writer.writeNamespace(QStringLiteral("http://www.w3.org/1999/xlink"), QStringLiteral("xlink"));
    writer.writeAttribute(QStringLiteral("xlink:href"),
                          QStringLiteral("data:%1;base64,%2")
                              .arg(mime, QString::fromLatin1(bytes.toBase64())));
    writer.writeAttribute(QStringLiteral("x"), QString::number(target.x(), 'g', 8));
    writer.writeAttribute(QStringLiteral("y"), QString::number(target.y(), 'g', 8));
    writer.writeAttribute(QStringLiteral("width"), QString::number(target.width(), 'g', 8));
    writer.writeAttribute(QStringLiteral("height"), QString::number(target.height(), 'g', 8));
    writer.writeEndElement();  // image
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OCartoucheSvg::WriteItems(QXmlStreamWriter &writer, const QVector<OCartoucheRenderItem> &items,
                               qreal scale, bool textAsPaths)
{
    for (const OCartoucheRenderItem &el : items)
    {
        // A shape is the one item that is neither words nor an image: geometry, stroked
        // where Layout placed it, unfilled, in the black the text is written in. Written
        // here because Paint draws it — an item the painter draws and the writer drops
        // would be the preview and the file disagreeing about the same block.
        if (el.kind == OCartoucheRenderItem::Kind::Shape)
        {
            writer.writeStartElement(QStringLiteral("rect"));
            writer.writeAttribute(QStringLiteral("x"), QString::number(el.target.x()));
            writer.writeAttribute(QStringLiteral("y"), QString::number(el.target.y()));
            writer.writeAttribute(QStringLiteral("width"), QString::number(el.target.width()));
            writer.writeAttribute(QStringLiteral("height"), QString::number(el.target.height()));
            writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
            writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("black"));
            writer.writeAttribute(QStringLiteral("stroke-width"),
                                  QString::number(OCartouchePainter::kShapePenWidth
                                                  * scale));
            writer.writeEndElement();  // rect
            continue;
        }
        if (el.kind == OCartoucheRenderItem::Kind::Text)
        {
            // The marks the line stands for, before its words — written here for the reason
            // the shape above is: Paint draws them, and an item the painter draws and the
            // writer drops would be the preview and the file disagreeing about the same
            // block. Traced first, then filled, which is the order the painter draws them in.
            if (!el.marks.IsEmpty())
            {
                const QTransform place =
                    QTransform::fromTranslate(el.marksRect.left(), el.marksRect.top());
                for (const QPainterPath &path : el.marks.strokes)
                {
                    writer.writeStartElement(QStringLiteral("path"));
                    writer.writeAttribute(QStringLiteral("d"),
                                          OSvg::PainterPathToSvgD(place.map(path)));
                    writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
                    writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("black"));
                    writer.writeAttribute(QStringLiteral("stroke-width"),
                                          QString::number(OCartouchePainter::kShapePenWidth
                                                          * scale));
                    writer.writeEndElement();  // path
                }
                for (const QPainterPath &path : el.marks.fills)
                {
                    writer.writeStartElement(QStringLiteral("path"));
                    writer.writeAttribute(QStringLiteral("d"),
                                          OSvg::PainterPathToSvgD(place.map(path)));
                    writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
                    // Same pen, same trap as the words above: a filled path following a stroked one
                    // — and a mark's fills ALWAYS follow its strokes — would otherwise be outlined
                    // with whatever pen was left in hand.
                    writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("#000"));
                    writer.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral(".01"));
                    writer.writeEndElement();  // path
                }
            }

            const QFontInfo info(el.font);
            if (textAsPaths)
            {
                QPainterPath glyphs;
                glyphs.addText(el.pen, el.font, el.text);
                if (glyphs.isEmpty())
                {
                    continue;
                }
                writer.writeStartElement(QStringLiteral("path"));
                writer.writeAttribute(QStringLiteral("d"), OSvg::PainterPathToSvgD(glyphs));
                writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
                // ⚠️ **A filled path cannot say "no stroke" to this renderer.** QSvgRenderer keeps
                // the pen it last held, and reads `stroke="none"` as "no change" — measured on a
                // probe: a `fill="black" stroke="none"` path following a stroked one comes out
                // OUTLINED, and Qt expands that stroke into a FILLED BAND around the shape. On a
                // glyph that band is a black outline a point wide, which is what "bold, and not
                // sharp" is: every stem thickened, every corner smudged.
                //
                // The remedy is the scissors' own (§17.3): a stroke so thin it is nothing, in the
                // fill's own colour, so that whatever pen happens to be in hand cannot show.
                writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("#000"));
                writer.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral(".01"));
                // ⚠️ **The path's own fill rule, and never SVG's default.** Qt builds a text path
                // with Qt::OddEvenFill — measured: `addText` answers OddEven — and the counters of
                // a letter are cut by THAT rule. SVG's default is `nonzero`, so a glyph written
                // without this attribute has its counters FILLED: the words come out looking bold
                // and blotchy, and nothing in the file says why. Written from the path rather than
                // hard-coded, so a path built the other way round keeps its own rule.
                writer.writeAttribute(QStringLiteral("fill-rule"),
                                      glyphs.fillRule() == Qt::OddEvenFill
                                          ? QStringLiteral("evenodd")
                                          : QStringLiteral("nonzero"));
                writer.writeEndElement();  // path
            }
            else
            {
                writer.writeStartElement(QStringLiteral("text"));
                writer.writeAttribute(QStringLiteral("x"), QString::number(el.pen.x()));
                writer.writeAttribute(QStringLiteral("y"), QString::number(el.pen.y()));
                writer.writeAttribute(QStringLiteral("font-family"), info.family());
                writer.writeAttribute(QStringLiteral("font-size"),
                                      QString::number(std::max(info.pixelSize(), 1)));
                if (info.style() != QFont::StyleNormal)
                {
                    writer.writeAttribute(QStringLiteral("font-style"),
                                          info.style() == QFont::StyleItalic ? QStringLiteral("italic")
                                                                            : QStringLiteral("oblique"));
                }
                // Qt 6 QFont::Weight uses the CSS scale (100..900): write it
                // numerically so Medium (500) / DemiBold (600) are not lost.
                writer.writeAttribute(QStringLiteral("font-weight"), QString::number(info.weight()));
                // **The face's NAME, because a weight number names no face.** Trame reads this one
                // and draws with exactly it; the number stays for whatever reads the file next and
                // knows nothing of Ostara's own attribute.
                writer.writeAttribute(QStringLiteral("ostara:font-face"), info.styleName());
                writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
                writer.writeCharacters(el.text);
                writer.writeEndElement();  // text
            }

            // The sample the line carries, written once for both branches: it is not text,
            // so it goes out the same way whether the words were drawn or outlined.
            if (!el.trace.isNull())
            {
                OSvg::WriteLine(writer, el.trace, OCartouchePainter::TracePen(el));
            }
        }
        else
        {
            WriteLogo(writer, el);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCartoucheSvg::WriteLogo(QXmlStreamWriter &writer, const OCartoucheRenderItem &el)
{
    const QString logoPath = el.sourcePath;
    if (logoPath.isEmpty() || el.target.width() <= 0.0 || el.target.height() <= 0.0)
    {
        return;
    }

    QFile logoFile(logoPath);
    if (!logoFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    const QByteArray logoBytes = logoFile.readAll();

    // **A drawing is inlined, a picture is embedded.** An SVG carries its own geometry: copied
    // here as it is, the logo stays vector and the exported file stays independent of the library
    // it came from (Val Fork behaviour, kept). A PNG or a JPEG is bytes and nothing else — read as
    // XML it yields no `<svg>` element, and the block came out with an empty `<g>` where the logo
    // was: the logo gone from the plan, and nothing said.
    if (!OLogoRender::IsVector(logoPath))
    {
        WriteEmbeddedPicture(writer, logoBytes, logoPath, el.target);
        return;
    }

    QXmlStreamReader rd(logoBytes);
    while (!rd.atEnd() && !(rd.isStartElement() && rd.name() == QLatin1String("svg")))
    {
        rd.readNext();
    }
    qreal naturalW = el.target.width();
    qreal naturalH = el.target.height();
    if (!rd.atEnd())
    {
        const QString vb = rd.attributes().value(QLatin1String("viewBox")).toString();
        if (!vb.isEmpty())
        {
            static const QRegularExpression re(QStringLiteral("[\\s,]+"));
            const QStringList p = vb.split(re, Qt::SkipEmptyParts);
            if (p.size() >= 4)
            {
                naturalW = p[2].toDouble();
                naturalH = p[3].toDouble();
            }
        }
        else
        {
            const double w = rd.attributes().value(QLatin1String("width")).toDouble();
            const double h = rd.attributes().value(QLatin1String("height")).toDouble();
            if (w > 0.0)
            {
                naturalW = w;
            }
            if (h > 0.0)
            {
                naturalH = h;
            }
        }
    }
    const qreal sx = (naturalW > 0.0) ? el.target.width() / naturalW : 1.0;
    const qreal sy = (naturalH > 0.0) ? el.target.height() / naturalH : 1.0;

    writer.writeStartElement(QStringLiteral("g"));
    writer.writeAttribute(QStringLiteral("transform"),
                          QStringLiteral("translate(%1,%2) scale(%3,%4)")
                              .arg(el.target.x(), 0, 'g', 8)
                              .arg(el.target.y(), 0, 'g', 8)
                              .arg(sx, 0, 'g', 8)
                              .arg(sy, 0, 'g', 8));

    int svgDepth = 0;
    while (!rd.atEnd())
    {
        rd.readNext();
        if (rd.isStartElement())
        {
            ++svgDepth;
            const QString ns = rd.namespaceUri().toString();
            const QString name = rd.name().toString();
            if (ns.isEmpty() || ns == QLatin1String("http://www.w3.org/2000/svg"))
            {
                writer.writeStartElement(name);
            }
            else
            {
                writer.writeStartElement(ns, name);
            }
            for (const QXmlStreamAttribute &a : rd.attributes())
            {
                const QString ans = a.namespaceUri().toString();
                if (ans.isEmpty())
                {
                    writer.writeAttribute(a.name().toString(), a.value().toString());
                }
                else
                {
                    writer.writeAttribute(ans, a.name().toString(), a.value().toString());
                }
            }
        }
        else if (rd.isEndElement())
        {
            if (svgDepth <= 0)
            {
                break;
            }
            --svgDepth;
            writer.writeEndElement();
        }
        else if (rd.isCharacters() && !rd.isWhitespace())
        {
            writer.writeCharacters(rd.text().toString());
        }
    }
    writer.writeEndElement();  // g (logo)
}
