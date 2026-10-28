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

#include "ocartouchesvg.h"
#include "osvgutils.h"

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

#include "../ifc/xml/vabstractpattern.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vpatterndb/othumbnaildata.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/othumbnailitem.h"
#include "../vtools/tools/vtoolseamallowance.h"

#include "vmisc/vabstractapplication.h"
#include "vmisc/vcommonsettings.h"

#include <QtGlobal>

#include <algorithm>

namespace
{
constexpr qreal kDefaultStrokeWidth = 0.1;


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
auto MiniatureStroke() -> qreal
{
    return qBound(kDefaultStrokeWidth, VAbstractApplication::VApp()->Settings()->WidthHairLine(), 1.0);
}

//---------------------------------------------------------------------------------------------------------------------
auto BuildThumbnail(quint32 pieceId, const VPiece &piece, const OThumbnailData &data,
                    const OThumbnailGeom &geom, const QRectF &pieceBox, bool geomChosen) -> Thumbnail
{
    Thumbnail record;

    // The drawing of the REFERENCE size, from wherever it comes: the cache when the active size is
    // another one, the live geometry when the two are the same. Asking the tool is also what makes
    // the export agree with the canvas by construction — an empty answer means the canvas shows
    // nothing either, so omitting the block is not a loss.
    //
    // **`getTool()` is not a guard: it throws** when the tool is gone (the family of bugs.md 22-08
    // and 27-08). An export must not die on a piece whose tool is missing.
    VToolSeamAllowance *tool = nullptr;
    try
    {
        tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
    }
    catch (const VException &)
    {
        tool = nullptr;
    }
    if (tool == nullptr)
    {
        return record;  // no canvas at all (batch export): there is nothing "what you see" could mean
    }
    const QPainterPath content = tool->ThumbnailReferenceContent();
    if (content.isEmpty())
    {
        return record;  // the reference drawing is still on its way, and the canvas shows none of it
    }

    const OThumbnailGeom resolved = OThumbnailItem::ResolveBlockGeom(geom, pieceBox, geomChosen);
    const QRectF block(0.0, 0.0, resolved.width, resolved.height);
    const QRectF contentBounds = content.boundingRect();
    const QFont font = OThumbnailItem::ValuesFont();
    const QFont legendFont = OThumbnailItem::LegendFont();
    const VToolSeamAllowance::ThumbnailWording wording = tool->ResolveThumbnailWording(piece, data);

    record.content = content;
    record.contentBounds = contentBounds;
    record.rect = QRectF(resolved.pos, QSizeF(resolved.width, resolved.height));
    record.font = font;
    record.legendFont = legendFont;
    for (int w = 0; w < wording.labels.size(); ++w)
    {
        const OThumbnailItem::LabelView &label = wording.labels.at(w);
        if (label.text.isEmpty())
        {
            continue;  // an empty word is not a word, and holds no place in the row
        }
        const QPointF inBlock = label.placed
                                    ? OThumbnailItem::FittedWordOrigin(contentBounds, block,
                                                                       label.offset)
                                    : OThumbnailItem::DefaultWordOrigin(wording.labels, w, block,
                                                                        font);
        record.words.append({label.text, inBlock, false});
    }
    if (!wording.legend.isEmpty())
    {
        const QPointF inBlock =
            data.HasLegendOffset()
                ? OThumbnailItem::FittedWordOrigin(contentBounds, block, data.LegendOffset())
                : OThumbnailItem::DefaultLegendOrigin(wording.legend, block, legendFont);
        record.words.append({wording.legend, inBlock, true});
    }
    return record;
}

//---------------------------------------------------------------------------------------------------------------------
bool WriteSvg(const QString &fileName, const QList<QGraphicsItem *> &pieces,
              const QList<QVector<Block>> &blocksByPiece,
              const QList<QVector<Annotation>> &annotationsByPiece,
              const QList<QVector<Thumbnail>> &thumbnailsByPiece, const QStringList &pieceNames,
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
    // Ostara's own attribute prefix, declared here because the info blocks' text goes out through
    // OCartoucheSvg::WriteItems, which marks every line with `ostara:font-face`. A prefix with no
    // declaration is not a document a namespace-aware parser will read at all: Illustrator refuses
    // the file, and so do Inkscape and xmllint. QXmlStreamReader, which reads these files back with
    // namespace processing off, never complained — it asks for the literal qualified name — so the
    // fault only ever showed on the outside.
    writer.writeNamespace(QLatin1String(OSvg::kOstaraNamespace), QStringLiteral("ostara"));
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
            writer.writeAttribute(QStringLiteral("d"), OSvg::PainterPathToSvgD(svgPath));
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

            // content -> scene -> file, and the three steps are written in the order they APPLY —
            // leftmost first, because Qt's (A*B) applies A then B. A content point c therefore
            // reaches origin + rect.topLeft + R·c, exactly where the canvas item draws it
            // (setPos(pos) + setRotation(-rotation) + setRect(0,0,w,h)), and the page transform is
            // applied to that SCENE point last. A naive "translate then rotate" would rotate in the
            // last step and carry the translation with it (the box flies to the pattern origin);
            // folding toSVG in first — which is what this line used to do — reads a block-local
            // point as if it were a scene point, so the record's offset passes through none of the
            // page's own scale and turning. The error is (R−I)·m + (1−σ)·t, where m is the page
            // offset and t the record's scene position: tens of millimetres on a page whose margins
            // and rotations make those non-zero, and exactly zero at rotation 0 and a 100 % scale —
            // which is how it went unnoticed. The miniature below has always composed this way.
            QTransform rot;
            rot.rotate(-block.rotation);
            QTransform tr;
            tr.translate(origin.x() + block.rect.x(), origin.y() + block.rect.y());
            const QTransform toFile = (rot * tr) * toSVG;

            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("transform"), MatrixAttr(toFile));

            // The items go to the shared serializer, so this file and the cover page draw
            // a block the same way — one definition of what an item becomes as SVG.
            OCartoucheSvg::WriteItems(writer, laid, block.fontScale, textAsPaths);
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

            // content -> scene -> file (the same composition as the blocks, and for the same
            // reason: rotation FIRST, then the translation, and the page transform on the scene
            // point LAST — see the note above).
            QTransform rot;
            rot.rotate(-annotation.rotation);
            QTransform tr;
            tr.translate(origin.x() + annotation.rect.x(), origin.y() + annotation.rect.y());
            const QTransform toFile = (rot * tr) * toSVG;

            // Rebuild the render font exactly like the canvas item: base pixel
            // = style size, or the resolved default, then × fontScale.
            QFont font;
            if (!annotation.fontFamily.isEmpty())
            {
                font.setFamily(annotation.fontFamily);
            }
            if (!annotation.fontFace.isEmpty())
            {
                font.setStyleName(annotation.fontFace);
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
                    writer.writeAttribute(QStringLiteral("d"), OSvg::PainterPathToSvgD(glyphs));
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

        // --- Miniatures ------------------------------------------------------
        // The canvas paint has two layers — the drawing, then the words — and so has this: the same
        // geometry, the same words at the same places, minus everything that belongs to the screen
        // (selection handles, the palette, the view zoom).
        const QVector<Thumbnail> &thumbnails = thumbnailsByPiece.value(pi);
        for (const Thumbnail &thumb : thumbnails)
        {
            if (thumb.content.isEmpty() || thumb.contentBounds.isEmpty() || thumb.rect.width() <= 0.0
                || thumb.rect.height() <= 0.0)
            {
                continue;
            }

            // The drawing goes out as one absolute path, like every other path in this document:
            // the piece's coordinates are fitted into the block, the block is put where it belongs
            // in the piece, and the file transform does the rest. No group has to carry it, and the
            // stroke keeps the width the other paths are given.
            //
            // ⚠️ **The factors are written in the order they APPLY — left first**: Qt's `A * B`
            // means "A, then B" (the same rule the block comment above spells out). Read the other
            // way round — "the right-hand factor is applied first", which is how the usual matrix
            // product reads — this line fits the drawing LAST, and the whole miniature lands near
            // the pattern origin: the fit is then applied to already-placed coordinates, so it
            // scales the block's position by the fit scale instead of scaling the drawing.
            const QRectF block(0.0, 0.0, thumb.rect.width(), thumb.rect.height());
            QTransform tr;
            tr.translate(origin.x() + thumb.rect.x(), origin.y() + thumb.rect.y());
            const QPainterPath svgPath =
                (OCartouchePainter::FitTransform(thumb.contentBounds, block) * tr * toSVG)
                    .map(thumb.content);

            writer.writeStartElement(QStringLiteral("path"));
            writer.writeAttribute(QStringLiteral("d"), OSvg::PainterPathToSvgD(svgPath));
            writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
            writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("black"));
            writer.writeAttribute(QStringLiteral("stroke-width"), QString::number(MiniatureStroke()));
            writer.writeEndElement();  // path (miniature drawing)

            if (thumb.words.isEmpty())
            {
                continue;
            }

            // The words carry their own transform, as a block's do: the caller has placed them in
            // the block's frame, and everything above that is the block's position in the file —
            // same application order as the drawing's transform above.
            writer.writeStartElement(QStringLiteral("g"));
            writer.writeAttribute(QStringLiteral("transform"), MatrixAttr(tr * toSVG));

            QVector<OCartoucheRenderItem> wordItems;
            wordItems.reserve(thumb.words.size());
            for (const ThumbnailWord &word : thumb.words)
            {
                if (word.text.isEmpty())
                {
                    continue;
                }
                // **The two texts are set apart.** A record carries the legend among the values —
                // the writers place words, they never label them — and this is where the two fonts
                // are told apart again: each word is measured and drawn in the font its own text is
                // set in, or the caption would come out as loud as the numbers it captions.
                const QFont &wordFont = word.legend ? thumb.legendFont : thumb.font;
                const QFontMetricsF metrics(wordFont);
                // **One line per language, as on the canvas.** A legend written in two languages is
                // one string with a line break in it, and SVG has no line break: a newline inside
                // `<text>` collapses into a space, so the two languages came out end to end on a
                // single line — far wider than the block they belong to. The pitch is the font's
                // own `height()`, the same one the item measures its box with (and the same one the
                // annotations above are laid out with).
                const QStringList lines = word.text.split(QLatin1Char('\n'));
                // **Every line centred in the word's box**, as the canvas draws it — asked of the
                // item, so the file cannot drift from the screen on a point nobody would notice
                // until a legend in two languages printed crooked.
                const QVector<QPointF> origins =
                    OThumbnailItem::CenteredLineOrigins(word.text, word.origin, wordFont);
                for (qsizetype i = 0; i < lines.size(); ++i)
                {
                    if (!lines.at(i).isEmpty())
                    {
                        OCartoucheRenderItem item;
                        item.kind = OCartoucheRenderItem::Kind::Text;
                        item.text = lines.at(i);
                        item.font = wordFont;
                        // The anchor is the top-left of the line's cell; a <text> wants its baseline,
                        // which is one ascent lower.
                        item.pen = origins.at(i) + QPointF(0.0, metrics.ascent());
                        wordItems.append(item);
                    }
                }
            }
            // The items go to the shared serializer, so a miniature's wording is written — as text
            // or as glyph outlines — exactly like a block's.
            OCartoucheSvg::WriteItems(writer, wordItems, 1.0, textAsPaths);
            writer.writeEndElement();  // g (miniature words)
        }

        writer.writeEndElement();  // g (information)
        writer.writeEndElement();  // g (piece)
    }

    writer.writeEndElement();  // svg
    writer.writeEndDocument();
    return true;
}
}  // namespace OInfoBlockSvg
