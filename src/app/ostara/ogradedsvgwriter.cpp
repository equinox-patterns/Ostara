/************************************************************************
 **
 **  @file   ogradedsvgwriter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Implementation of the graded multi-layer SVG writer (see header).
 **          The emission mirrors the flat Details writer (OInfoBlockSvg) so a
 **          graded SVG opens identically in Illustrator/Inkscape; text is only
 **          ever emitted as editable <text> (never vectorized).
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

#include "ogradedsvgwriter.h"

#include "osvgutils.h"
#include "../vlayout/vgraphicsfillitem.h"

#include <QColor>
#include <QFile>
#include <QFont>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QHash>
#include <QMarginsF>
#include <QPainterPath>
#include <QPen>
#include <QRegularExpression>
#include <QTextStream>
#include <QTransform>
#include <QtMath>
#include <QXmlStreamAttribute>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <algorithm>

namespace OGradedSvg
{
namespace
{
constexpr qreal kDefaultStrokeWidth = 0.1;

/** @brief Backend namespace "ostara", declared on the <svg> root. Its attributes
 *  are markers for the Ostara → Rabnest pipeline only — vector editors never
 *  display them, so they can carry machine meaning without constraining the
 *  visible layer names. */
constexpr auto kOstaraNamespace = "urn:ostara";

/** @brief Value of the ostara:layout marker set on the decor layer's <g>. */
constexpr auto kOstaraLayoutMarker = "yes";

/** @brief Serialize one visible trace (path item) — mirrors the flat writer. */
void WriteTrace(QXmlStreamWriter &writer, const QTransform &toSVG, const QPen &defaultPen,
                QGraphicsItem *gitem)
{
    QGraphicsPathItem *const pathItem = OSvg::AsPathItem(gitem);
    if (pathItem == nullptr || !pathItem->isVisible())
    {
        return;
    }
    const QPainterPath scenePath = pathItem->mapToScene(pathItem->path());
    if (scenePath.isEmpty())
    {
        return;
    }
    const QPainterPath svgPath = toSVG.map(scenePath);

    QPen pen = pathItem->pen();
    if (pen.style() == Qt::NoPen)
    {
        pen = defaultPen;
    }
    if (pen.widthF() <= 0.0)
    {
        pen.setWidthF(kDefaultStrokeWidth);
    }
    const QString stroke = pen.color().name();

    QString fill = QStringLiteral("none");
    const bool grainOrFold = gitem->type() == VGraphicsFillItem::Type
                             || gitem->type() == VGraphicsFoldLineItem::Type;
    if (pathItem->brush().style() != Qt::NoBrush)
    {
        fill = pathItem->brush().color().name();
    }
    else if (grainOrFold)
    {
        fill = stroke;
    }

    writer.writeStartElement(QStringLiteral("path"));
    writer.writeAttribute(QStringLiteral("d"), OSvg::PainterPathToSvgD(svgPath));
    writer.writeAttribute(QStringLiteral("fill"), fill);
    writer.writeAttribute(QStringLiteral("stroke"), stroke);
    writer.writeAttribute(QStringLiteral("stroke-width"), QString::number(pen.widthF()));
    const QString dash = OSvg::DashArray(pen);
    if (!dash.isEmpty())
    {
        writer.writeAttribute(QStringLiteral("stroke-dasharray"), dash);
    }
    writer.writeEndElement();  // path
}

/** @brief Serialize one editable simple-text item — mirrors the flat writer. */
void WriteText(QXmlStreamWriter &writer, const QTransform &toSVG, QGraphicsSimpleTextItem *textItem)
{
    if (textItem == nullptr || !textItem->isVisible() || textItem->text().isEmpty())
    {
        return;
    }
    const QFont font = textItem->font();
    const QFontMetricsF fm(font);
    const QTransform tx = textItem->sceneTransform() * toSVG;
    writer.writeStartElement(QStringLiteral("text"));
    writer.writeAttribute(QStringLiteral("x"), QStringLiteral("0"));
    writer.writeAttribute(QStringLiteral("y"), QString::number(fm.ascent()));
    writer.writeAttribute(QStringLiteral("font-family"), font.family());
    writer.writeAttribute(QStringLiteral("font-size"),
                          QStringLiteral("%1").arg(std::max(QFontInfo(font).pixelSize(), 1)));
    if (font.italic())
    {
        writer.writeAttribute(QStringLiteral("font-style"), QStringLiteral("italic"));
    }
    writer.writeAttribute(QStringLiteral("font-weight"), QString::number(font.weight()));
    writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
    writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(tx));
    writer.writeCharacters(textItem->text());
    writer.writeEndElement();  // text
}

/** @brief Inline the original user SVG of a block logo (Val Fork behaviour). */
void WriteLogo(QXmlStreamWriter &writer, const OCartoucheRenderItem &el)
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
    const QByteArray logoSvgBytes = logoFile.readAll();

    QXmlStreamReader rd(logoSvgBytes);
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

/** @brief Emit one information block (editable text only). */
void WriteBlock(QXmlStreamWriter &writer, const QPointF &origin,
                const OInfoBlockSvg::Block &block)
{
    if (block.lines.isEmpty() || block.rect.width() <= 0.0 || block.rect.height() <= 0.0)
    {
        return;
    }

    QVector<OCartoucheRenderLine> scaledLines =
        OCartouchePainter::ScaledLines(block.lines, block.fontScale);
    const QRectF contentRect(0.0, 0.0, block.rect.width(), block.rect.height());
    const QVector<OCartoucheRenderItem> laid =
        OCartouchePainter::Layout(contentRect, scaledLines, block.fontScale);

    QTransform rot;
    rot.rotate(-block.rotation);
    QTransform tr;
    tr.translate(origin.x() + block.rect.x(), origin.y() + block.rect.y());
    // No toSVG here. The page's own offset is applied by the wrapper this record is written
    // into, AFTER the owner's placement — where in the chain it belongs. Folded in here it
    // would be applied BEFORE the rotation, so on any page but the first the sheet offset
    // would be turned by the owner's angle and carry the cartouche off the sheet. Invisible
    // while the page was the whole plan and toSVG was the identity; it took a tiled export
    // to show it.
    const QTransform toFile = rot * tr;

    writer.writeStartElement(QStringLiteral("g"));
    writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(toFile));

    for (const OCartoucheRenderItem &el : laid)
    {
        // The shape, as in the piece writer: geometry stroked where Layout placed it, unfilled.
        // Paint draws it, so the file carries it — the two renderings of one block are the same
        // block.
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
                                                  * block.fontScale));
            writer.writeEndElement();  // rect
            continue;
        }
        if (el.kind == OCartoucheRenderItem::Kind::Text)
        {
            const QFontInfo info(el.font);
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
            writer.writeAttribute(QStringLiteral("font-weight"), QString::number(info.weight()));
            writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
            writer.writeCharacters(el.text);
            writer.writeEndElement();  // text
        }
        else
        {
            WriteLogo(writer, el);
        }
    }
    writer.writeEndElement();  // g (block)
}

/** @brief Emit one floating annotation (editable text only). */
void WriteAnnotation(QXmlStreamWriter &writer, const QPointF &origin,
                     const OInfoBlockSvg::Annotation &annotation)
{
    if (annotation.text.isEmpty() || annotation.rect.width() <= 0.0 || annotation.rect.height() <= 0.0)
    {
        return;
    }

    QTransform rot;
    rot.rotate(-annotation.rotation);
    QTransform tr;
    tr.translate(origin.x() + annotation.rect.x(), origin.y() + annotation.rect.y());
    const QTransform toFile = rot * tr;

    QFont font;
    if (!annotation.fontFamily.isEmpty())
    {
        font.setFamily(annotation.fontFamily);
    }
    if (annotation.fontWeight != 0)
    {
        font.setWeight(static_cast<QFont::Weight>(annotation.fontWeight));
    }
    if (annotation.fontSizePx > 0)
    {
        font.setPixelSize(annotation.fontSizePx);
    }
    const int basePixel =
        annotation.fontSizePx > 0 ? annotation.fontSizePx : QFontInfo(font).pixelSize();
    font.setPixelSize(qRound(basePixel * annotation.fontScale));
    const QFontInfo resolvedFont(font);

    const QColor resolvedTextColor(annotation.textColor);
    const QString fill = resolvedTextColor.isValid() ? resolvedTextColor.name()
                                                     : QStringLiteral("#000000");

    writer.writeStartElement(QStringLiteral("g"));
    writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(toFile));

    const QColor resolvedBackground(annotation.backgroundColor);
    if (!annotation.backgroundColor.isEmpty() && resolvedBackground.isValid())
    {
        writer.writeStartElement(QStringLiteral("rect"));
        writer.writeAttribute(QStringLiteral("width"), QString::number(annotation.rect.width()));
        writer.writeAttribute(QStringLiteral("height"), QString::number(annotation.rect.height()));
        writer.writeAttribute(QStringLiteral("fill"), resolvedBackground.name());
        writer.writeEndElement();  // rect
    }

    const QStringList lines = annotation.text.split(u'\n');
    const QFontMetricsF fm(font);
    const qreal lineHeight = fm.height();
    const qreal firstBaseline =
        (annotation.rect.height() - static_cast<qreal>(lines.size()) * lineHeight) / 2.0
        + fm.ascent();

    for (int li = 0; li < lines.size(); ++li)
    {
        const QString &line = lines.at(li);
        const qreal x = (annotation.rect.width() - fm.horizontalAdvance(line)) / 2.0;
        const qreal y = firstBaseline + li * lineHeight;

        writer.writeStartElement(QStringLiteral("text"));
        writer.writeAttribute(QStringLiteral("x"), QString::number(x));
        writer.writeAttribute(QStringLiteral("y"), QString::number(y));
        writer.writeAttribute(QStringLiteral("font-family"), resolvedFont.family());
        writer.writeAttribute(QStringLiteral("font-size"),
                              QString::number(std::max(resolvedFont.pixelSize(), 1)));
        writer.writeAttribute(QStringLiteral("font-weight"), QString::number(resolvedFont.weight()));
        writer.writeAttribute(QStringLiteral("fill"), fill);
        writer.writeCharacters(line);
        writer.writeEndElement();  // text
    }
    writer.writeEndElement();  // g (annotation)
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
bool WriteGradedSvg(const QString &fileName, const QList<SizeLayer> &sizeLayers,
                    const QString &layoutLayerName, const QList<LayoutObject> &layoutObjects,
                    const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
                    const QString &title, const QString &description, bool layered)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Can't open file for SVG export" << fileName;
        return false;
    }

    const qreal offsetX = imageRect.x();
    const qreal offsetY = imageRect.y();
    const qreal totalWidth = imageRect.width() * xScale + margins.left() + margins.right();
    const qreal totalHeight = imageRect.height() * yScale + margins.top() + margins.bottom();
    const QTransform toSVG(xScale, 0, 0, yScale, margins.left() - offsetX * xScale,
                           margins.top() - offsetY * yScale);
    const QPen defaultPen(Qt::black, kDefaultStrokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("svg"));
    writer.writeDefaultNamespace(QStringLiteral("http://www.w3.org/2000/svg"));
    writer.writeNamespace(QStringLiteral("http://ns.adobe.com/AdobeIllustrator/10.0/"), QStringLiteral("i"));
    writer.writeNamespace(QStringLiteral("http://www.inkscape.org/namespaces/inkscape"),
                          QStringLiteral("inkscape"));
    writer.writeNamespace(QLatin1String(kOstaraNamespace), QStringLiteral("ostara"));
    writer.writeAttribute(QStringLiteral("version"), QStringLiteral("1.1"));
    {
        constexpr qreal dpm = 96.0 / 25.4;
        writer.writeAttribute(QStringLiteral("width"),
                              QStringLiteral("%1mm").arg(totalWidth / dpm, 0, 'f', 4));
        writer.writeAttribute(QStringLiteral("height"),
                              QStringLiteral("%1mm").arg(totalHeight / dpm, 0, 'f', 4));
    }
    writer.writeAttribute(QStringLiteral("viewBox"),
                          QStringLiteral("0 0 %1 %2").arg(totalWidth).arg(totalHeight));
    if (!title.isEmpty())
    {
        writer.writeTextElement(QStringLiteral("title"), title);
    }
    if (!description.isEmpty())
    {
        writer.writeTextElement(QStringLiteral("desc"), description);
    }

    // One unique-id table per layer: ids may repeat across size layers (each size shows
    // the same piece names) — Illustrator/Inkscape accept that, and a global counter
    // would wrongly rename "Piece 1" to "Piece 1_1" in layer 40.
    const auto emitPieceGeometry = [&](QGraphicsItem *root) {
        WriteTrace(writer, toSVG, defaultPen, root);
        const QList<QGraphicsItem *> children = root->childItems();
        for (QGraphicsItem *child : children)
        {
            if (!OSvg::IsInfoChild(child))
            {
                WriteTrace(writer, toSVG, defaultPen, child);
            }
        }
    };

    for (const SizeLayer &layer : sizeLayers)
    {
        QHash<QString, int> usedIds;
        const auto uniqueId = [&usedIds](const QString &base) {
            QString id = OSvg::SanitizeSvgId(base);
            if (usedIds.contains(id))
            {
                const int count = usedIds.value(id) + 1;
                usedIds.insert(id, count);
                id += u'_' + QString::number(count);
            }
            else
            {
                usedIds.insert(id, 0);
            }
            return id;
        };

        // A layered document announces each size as a layer of its own. A flat one, which
        // exists for the single-size run, writes its objects straight under the root:
        // there is nothing to switch between, so a layer would be an entry with no choice.
        if (layered)
        {
            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("id"), OSvg::SanitizeSvgId(layer.label));
            writer.writeAttribute(QStringLiteral("i:layer"), QStringLiteral("yes"));
            writer.writeAttribute(QStringLiteral("inkscape:label"), layer.label);
            writer.writeAttribute(QStringLiteral("inkscape:groupmode"), QStringLiteral("layer"));
            writer.writeTextElement(QStringLiteral("title"), layer.label);
        }

        for (const GradedObject &object : layer.objects)
        {
            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("id"), uniqueId(object.name));
            if (!object.transform.isIdentity())
            {
                writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(object.transform));
            }
            if (object.isGroup)
            {
                for (const GradedMember &member : object.members)
                {
                    if (member.item == nullptr)
                    {
                        continue;
                    }
                    writer.writeStartElement(QStringLiteral("g"));
                    writer.writeAttribute(QStringLiteral("id"), uniqueId(member.name));
                    emitPieceGeometry(member.item);
                    writer.writeEndElement();  // g (member)
                }
            }
            else if (object.loneItem != nullptr)
            {
                emitPieceGeometry(object.loneItem);
            }
            writer.writeEndElement();  // g (object)
        }
        if (layered)
        {
            writer.writeEndElement();  // g (size layer)
        }
    }

    // Layout layer — shared decor of the reference size, written last (topmost). A flat
    // document has no layer to put it in, but it still carries its cartouche.
    if (!layoutLayerName.isEmpty() || !layered)
    {
        QHash<QString, int> usedIds;
        const auto uniqueId = [&usedIds](const QString &base) {
            QString id = OSvg::SanitizeSvgId(base);
            if (usedIds.contains(id))
            {
                const int count = usedIds.value(id) + 1;
                usedIds.insert(id, count);
                id += u'_' + QString::number(count);
            }
            else
            {
                usedIds.insert(id, 0);
            }
            return id;
        };

        if (layered)
        {
            writer.writeStartElement(QStringLiteral("g"));
            // The layer keeps its user-chosen name in the id, inkscape:label and
            // <title>, so every editor shows it. Rabnest identifies this layer as
            // the decor layer via the ostara:layout backend marker below, which no
            // editor displays.
            writer.writeAttribute(QStringLiteral("id"), OSvg::SanitizeSvgId(layoutLayerName));
            writer.writeAttribute(QStringLiteral("i:layer"), QStringLiteral("yes"));
            writer.writeAttribute(QStringLiteral("inkscape:label"), layoutLayerName);
            writer.writeAttribute(QStringLiteral("inkscape:groupmode"), QStringLiteral("layer"));
            writer.writeAttribute(QStringLiteral("ostara:layout"), QLatin1String(kOstaraLayoutMarker));
            writer.writeTextElement(QStringLiteral("title"), layoutLayerName);
        }

        for (const LayoutObject &object : layoutObjects)
        {
            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("id"), uniqueId(object.name));
            // Mark every decor object wrapper as a layer (like Val Fork did) so
            // Rabnest's NestedSvgWriter moves it together with the piece — its
            // filter only transforms <g i:layer="yes"> children of this layer.
            writer.writeAttribute(QStringLiteral("i:layer"), QStringLiteral("yes"));

            // Two wrappers, because the two halves of the decor do not live in the same
            // frame. The items were placed in the scene and already carry their owner's
            // placement; the records were not, they are written in their owner's own
            // frame. The transform below therefore goes on an inner wrapper — put on the
            // outer one it would move the items a second time.
            for (QGraphicsItem *child : object.decorChildren)
            {
                if (child->type() == QGraphicsSimpleTextItem::Type)
                {
                    WriteText(writer, toSVG, static_cast<QGraphicsSimpleTextItem *>(child));
                }
                else
                {
                    WriteTrace(writer, toSVG, defaultPen, child);
                }
            }

            // The records take the page offset and the owner's placement together, in that
            // order: the owner's frame first, the page's own translation last. The decor
            // children above need no wrapper — their scene coordinates are already mapped
            // through toSVG one by one, so this is the only place the two meet.
            const QTransform recordTx = object.transform * toSVG;
            const bool placed = !recordTx.isIdentity();
            if (placed)
            {
                writer.writeStartElement(QStringLiteral("g"));
                writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(recordTx));
            }
            for (const OInfoBlockSvg::Block &block : object.blocks)
            {
                WriteBlock(writer, object.origin, block);
            }
            for (const OInfoBlockSvg::Annotation &annotation : object.annotations)
            {
                WriteAnnotation(writer, object.origin, annotation);
            }
            if (placed)
            {
                writer.writeEndElement();  // g (records, in their owner's frame)
            }
            writer.writeEndElement();  // g (layout object)
        }
        if (layered)
        {
            writer.writeEndElement();  // g (layout layer)
        }
    }

    writer.writeEndElement();  // svg
    writer.writeEndDocument();
    return true;
}
}  // namespace OGradedSvg
