/************************************************************************
 **
 **  @file   oinfoblockexport.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   03 September, 2026
 **
 **  @brief  Flat Details SVG writer with faithful information blocks.
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

#include "oinfoblockexport.h"

#include <QColor>
#include <QDebug>
#include <QFile>
#include <QFont>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QXmlStreamAttribute>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "../vlayout/vgraphicsfillitem.h"

#include <QtGlobal>

#include <algorithm>

namespace
{
constexpr qreal kDefaultStrokeWidth = 0.1;

//---------------------------------------------------------------------------------------------------------------------
QString PainterPathToSvgD(const QPainterPath &path)
{
    QString d;
    QTextStream stream(&d);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(3);

    for (int i = 0; i < path.elementCount(); ++i)
    {
        const QPainterPath::Element e = path.elementAt(i);
        switch (e.type)
        {
            case QPainterPath::MoveToElement:
                stream << "M " << e.x << ' ' << e.y << ' ';
                break;
            case QPainterPath::LineToElement:
                stream << "L " << e.x << ' ' << e.y << ' ';
                break;
            case QPainterPath::CurveToElement:
                stream << "C " << e.x << ' ' << e.y << ' ';
                break;
            case QPainterPath::CurveToDataElement:
                stream << e.x << ' ' << e.y << ' ';
                break;
            default:
                break;
        }
    }
    return d.trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
QString SanitizeSvgId(const QString &name)
{
    QString id;
    id.reserve(name.size());
    for (const QChar ch : name)
    {
        id += (ch.isLetterOrNumber() || ch == u'_' || ch == u'-') ? ch : u'_';
    }
    if (!id.isEmpty() && id.front().isDigit())
    {
        id.prepend(u'_');
    }
    if (id.isEmpty())
    {
        id = QStringLiteral("piece");
    }
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
QString DashArray(const QPen &pen)
{
    if (pen.style() == Qt::SolidLine)
    {
        return {};
    }
    const qreal w = std::max(pen.widthF(), static_cast<qreal>(1.0));
    switch (pen.style())
    {
        case Qt::DashLine:
            return QStringLiteral("%1 %2").arg(w * 4.0).arg(w * 2.0);
        case Qt::DotLine:
            return QStringLiteral("%1 %2").arg(w).arg(w * 2.0);
        case Qt::DashDotLine:
            return QStringLiteral("%1 %2 %3 %2").arg(w * 4.0).arg(w * 2.0).arg(w);
        case Qt::DashDotDotLine:
            return QStringLiteral("%1 %2 %3 %2 %3 %2").arg(w * 4.0).arg(w * 2.0).arg(w);
        default:
            return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
QString MatrixAttr(const QTransform &m)
{
    return QStringLiteral("matrix(%1,%2,%3,%4,%5,%6)")
        .arg(m.m11(), 0, 'g', 8)
        .arg(m.m12(), 0, 'g', 8)
        .arg(m.m21(), 0, 'g', 8)
        .arg(m.m22(), 0, 'g', 8)
        .arg(m.dx(), 0, 'g', 8)
        .arg(m.dy(), 0, 'g', 8);
}

//---------------------------------------------------------------------------------------------------------------------
// The two custom piece items override type() (so qgraphicsitem_cast fails on
// them) but both are QGraphicsPathItem subclasses.
QGraphicsPathItem *AsPathItem(QGraphicsItem *item)
{
    const int t = item->type();
    if (t == VGraphicsFillItem::Type || t == VGraphicsFoldLineItem::Type)
    {
        return static_cast<QGraphicsPathItem *>(item);
    }
    return qgraphicsitem_cast<QGraphicsPathItem *>(item);
}
}  // namespace

namespace OInfoBlockSvg
{
//---------------------------------------------------------------------------------------------------------------------
bool WriteSvg(const QString &fileName, const QList<QGraphicsItem *> &pieces,
              const QList<QVector<Block>> &blocksByPiece,
              const QList<QVector<Annotation>> &annotationsByPiece, const QStringList &pieceNames,
              const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
              const QString &title, const QString &description, bool textAsPaths,
              bool showGrainline, const QHash<int, QVector<int>> &groupMembers)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Can't open file for SVG export" << fileName;
        return false;
    }

    // Same geometry as VLayoutExporter::ExportToSVG: imageRect (paper) scaled,
    // shifted by the margins, offset by the scene origin.
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
    writer.writeNamespace(QStringLiteral("http://www.inkscape.org/namespaces/inkscape"), QStringLiteral("inkscape"));
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

    // --- Geometry + blocks, one <g> per piece --------------------------------
    QHash<QString, int> usedIds;
    QSet<int> memberSet;
    for (auto it = groupMembers.cbegin(); it != groupMembers.cend(); ++it)
    {
        for (int mi : it.value())
        {
            memberSet.insert(mi);
        }
    }
    for (int pi = 0; pi < pieces.size(); ++pi)
    {
        QGraphicsItem *const pieceItem = pieces.at(pi);
        if (pieceItem == nullptr || !pieceItem->isVisible())
        {
            continue;
        }
        if (memberSet.contains(pi))
        {
            continue;  // a grouped piece is emitted inside its parent group
        }

        QString const baseId = pieceNames.value(pi);
        QString id = SanitizeSvgId(baseId);
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
        writer.writeStartElement(QStringLiteral("g"));
        writer.writeAttribute(QStringLiteral("id"), id);

        // Geometry: main contour (root) then its children.
        auto writePathItem = [&](QGraphicsItem *gitem)
        {
            QGraphicsPathItem *const pathItem = AsPathItem(gitem);
            if (pathItem == nullptr || !pathItem->isVisible())
            {
                return;
            }
            if (!showGrainline && gitem->type() == VGraphicsFillItem::Type)
            {
                return;  // grainline hidden (mirrors PrepareGrainlineForExport)
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
            writer.writeAttribute(QStringLiteral("d"), PainterPathToSvgD(svgPath));
            writer.writeAttribute(QStringLiteral("fill"), fill);
            writer.writeAttribute(QStringLiteral("stroke"), stroke);
            writer.writeAttribute(QStringLiteral("stroke-width"), QString::number(pen.widthF()));
            const QString dash = DashArray(pen);
            if (!dash.isEmpty())
            {
                writer.writeAttribute(QStringLiteral("stroke-dasharray"), dash);
            }
            writer.writeEndElement();  // path
        };

        auto writeTextItem = [&](QGraphicsSimpleTextItem *textItem)
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
            // Qt 6 QFont::Weight uses the CSS scale (100..900).
            writer.writeAttribute(QStringLiteral("font-weight"), QString::number(font.weight()));
            writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
            writer.writeAttribute(QStringLiteral("transform"), MatrixAttr(tx));
            writer.writeCharacters(textItem->text());
            writer.writeEndElement();  // text
        };

        auto isInfoChild = [](QGraphicsItem *c)
        {
            if (c == nullptr || !c->isVisible())
            {
                return false;
            }
            const int t = c->type();
            if (t == QGraphicsSimpleTextItem::Type || t == VGraphicsFillItem::Type
                || t == VGraphicsFoldLineItem::Type)
            {
                return true;
            }
            QGraphicsPathItem *const p = AsPathItem(c);
            return p != nullptr && p->brush().style() != Qt::NoBrush;
        };

        // A group nests its member pieces' geometry (contours + internal paths);
        // the members' own children (grain/fold/blocks/annotations) are masked on
        // the canvas, so geometry only, inside their own <g>.
        if (const QVector<int> members = groupMembers.value(pi); !members.isEmpty())
        {
            for (int mi : members)
            {
                QGraphicsItem *const memberItem = pieces.value(mi);
                if (memberItem == nullptr)
                {
                    continue;
                }
                QString mid = SanitizeSvgId(pieceNames.value(mi));
                if (usedIds.contains(mid))
                {
                    const int count = usedIds.value(mid) + 1;
                    usedIds.insert(mid, count);
                    mid += u'_' + QString::number(count);
                }
                else
                {
                    usedIds.insert(mid, 0);
                }
                writer.writeStartElement(QStringLiteral("g"));
                writer.writeAttribute(QStringLiteral("id"), mid);
                writePathItem(memberItem);  // main contour
                const QList<QGraphicsItem *> mChildren = memberItem->childItems();
                for (QGraphicsItem *child : mChildren)
                {
                    if (!isInfoChild(child))
                    {
                        writePathItem(child);
                    }
                }
                writer.writeEndElement();  // g (member piece)
            }
        }
        else
        {
            writePathItem(pieceItem);  // main contour
            const QList<QGraphicsItem *> children = pieceItem->childItems();
            for (QGraphicsItem *child : children)
            {
                if (!isInfoChild(child))
                {
                    writePathItem(child);
                }
            }
        }

        // Children (grainline, fold line, blocks, annotations) go into a
        // dedicated "<name>_information" group.
        writer.writeStartElement(QStringLiteral("g"));
        writer.writeAttribute(QStringLiteral("id"), id + QStringLiteral("_information"));
        const QList<QGraphicsItem *> children = pieceItem->childItems();
        for (QGraphicsItem *child : children)
        {
            if (isInfoChild(child))
            {
                if (child->type() == QGraphicsSimpleTextItem::Type)
                {
                    writeTextItem(static_cast<QGraphicsSimpleTextItem *>(child));
                }
                else
                {
                    writePathItem(child);
                }
            }
        }

        // --- Information blocks of this piece --------------------------------
        const QPointF origin = pieceItem->mapToScene(QPointF(0.0, 0.0));
        const QVector<Block> &blocks = blocksByPiece.value(pi);
        for (const Block &block : blocks)
        {
            if (block.lines.isEmpty() || block.rect.width() <= 0.0 || block.rect.height() <= 0.0)
            {
                continue;
            }

            // Layout in the block content space, exactly like the canvas paint.
            QVector<OCartoucheRenderLine> scaledLines =
                OCartouchePainter::ScaledLines(block.lines, block.fontScale);
            const QRectF contentRect(0.0, 0.0, block.rect.width(), block.rect.height());
            const QVector<OCartoucheRenderItem> laid =
                OCartouchePainter::Layout(contentRect, scaledLines, block.fontScale);

            // content -> scene -> file.
            // Qt combines transforms in row-vector order: (A*B) applies A then B,
            // so a naive "translate then rotate" rotates in the LAST step and the
            // translation gets rotated too (the box flies to the pattern origin).
            // To map a content point c to  origin + rect.topLeft + R·c  (rotation
            // about the box top-left), compose the rotation FIRST, then translate.
            QTransform rot;
            rot.rotate(-block.rotation);
            QTransform tr;
            tr.translate(origin.x() + block.rect.x(), origin.y() + block.rect.y());
            const QTransform toFile = toSVG * (rot * tr);

            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("transform"), MatrixAttr(toFile));

            for (const OCartoucheRenderItem &el : laid)
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
                                                          * block.fontScale));
                    writer.writeEndElement();  // rect
                    continue;
                }
                if (el.kind == OCartoucheRenderItem::Kind::Text)
                {
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
                        writer.writeAttribute(QStringLiteral("d"), PainterPathToSvgD(glyphs));
                        writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
                        writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("none"));
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
                        writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
                        writer.writeCharacters(el.text);
                        writer.writeEndElement();  // text
                    }
                }
                else
                {
                    // Logo: inline the original user SVG (Val Fork behaviour).
                    const QString logoPath = el.sourcePath;
                    if (logoPath.isEmpty() || el.target.width() <= 0.0 || el.target.height() <= 0.0)
                    {
                        continue;
                    }
                    QFile logoFile(logoPath);
                    if (!logoFile.open(QIODevice::ReadOnly))
                    {
                        continue;
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
                    writer.writeAttribute(
                        QStringLiteral("transform"),
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
            }
            writer.writeEndElement();  // g (block)
        }

        // --- Floating annotations of this piece ------------------------------
        // Same origin/matrix convention as the blocks: the rect is piece-local,
        // its top-left already resolved by the caller.
        auto writeAnnotation = [&](const Annotation &annotation)
        {
            if (annotation.text.isEmpty() || annotation.rect.width() <= 0.0
                || annotation.rect.height() <= 0.0)
            {
                return;
            }

            // content -> scene -> file (same row-vector composition as the blocks:
            // rotation FIRST, then the translation).
            QTransform rot;
            rot.rotate(-annotation.rotation);
            QTransform tr;
            tr.translate(origin.x() + annotation.rect.x(), origin.y() + annotation.rect.y());
            const QTransform toFile = toSVG * (rot * tr);

            // Rebuild the render font exactly like the canvas item: base pixel
            // = style size, or the resolved default, then × fontScale.
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
            const int basePixel = annotation.fontSizePx > 0 ? annotation.fontSizePx
                                                            : QFontInfo(font).pixelSize();
            font.setPixelSize(qRound(basePixel * annotation.fontScale));
            const QFontInfo resolvedFont(font);

            const QColor resolvedTextColor(annotation.textColor);
            const QString fill = resolvedTextColor.isValid() ? resolvedTextColor.name()
                                                             : QStringLiteral("#000000");

            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("transform"), MatrixAttr(toFile));

            const QColor resolvedBackground(annotation.backgroundColor);
            if (!annotation.backgroundColor.isEmpty() && resolvedBackground.isValid())
            {
                writer.writeStartElement(QStringLiteral("rect"));
                writer.writeAttribute(QStringLiteral("width"),
                                      QString::number(annotation.rect.width()));
                writer.writeAttribute(QStringLiteral("height"),
                                      QString::number(annotation.rect.height()));
                writer.writeAttribute(QStringLiteral("fill"), resolvedBackground.name());
                writer.writeEndElement();  // rect
            }

            // Qt drawText(rect, Qt::AlignCenter): each line centered horizontally
            // in the box, the whole block centered vertically (lineCount lines of
            // fm.height(), first baseline as below).
            const QStringList lines = annotation.text.split(u'\n');
            const QFontMetricsF fm(font);
            const qreal lineHeight = fm.height();
            const qreal firstBaseline = (annotation.rect.height() - static_cast<qreal>(lines.size()) * lineHeight)
                                            / 2.0
                                        + fm.ascent();

            for (int li = 0; li < lines.size(); ++li)
            {
                const QString &line = lines.at(li);
                const qreal x = (annotation.rect.width() - fm.horizontalAdvance(line)) / 2.0;
                const qreal y = firstBaseline + li * lineHeight;

                if (textAsPaths)
                {
                    QPainterPath glyphs;
                    glyphs.addText(QPointF(x, y), font, line);
                    if (glyphs.isEmpty())
                    {
                        continue;
                    }
                    writer.writeStartElement(QStringLiteral("path"));
                    writer.writeAttribute(QStringLiteral("d"), PainterPathToSvgD(glyphs));
                    writer.writeAttribute(QStringLiteral("fill"), fill);
                    writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("none"));
                    writer.writeEndElement();  // path
                }
                else
                {
                    writer.writeStartElement(QStringLiteral("text"));
                    writer.writeAttribute(QStringLiteral("x"), QString::number(x));
                    writer.writeAttribute(QStringLiteral("y"), QString::number(y));
                    writer.writeAttribute(QStringLiteral("font-family"), resolvedFont.family());
                    writer.writeAttribute(QStringLiteral("font-size"),
                                          QString::number(std::max(resolvedFont.pixelSize(), 1)));
                    // Qt 6 QFont::Weight uses the CSS scale (100..900): write it
                    // numerically so Medium (500) / DemiBold (600) are not lost.
                    writer.writeAttribute(QStringLiteral("font-weight"),
                                          QString::number(resolvedFont.weight()));
                    writer.writeAttribute(QStringLiteral("fill"), fill);
                    writer.writeCharacters(line);
                    writer.writeEndElement();  // text
                }
            }
            writer.writeEndElement();  // g (annotation)
        };

        const QVector<Annotation> &annotations = annotationsByPiece.value(pi);
        for (const Annotation &annotation : annotations)
        {
            writeAnnotation(annotation);
        }
        writer.writeEndElement();  // g (information)
        writer.writeEndElement();  // g (piece)
    }

    writer.writeEndElement();  // svg
    writer.writeEndDocument();
    return true;
}
}  // namespace OInfoBlockSvg
