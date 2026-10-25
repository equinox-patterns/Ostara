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

#include "ocartouchesvg.h"
#include "osvgutils.h"
#include "core/oexportrecipe.h"
#include "../vlayout/vgraphicsfillitem.h"

#include <QColor>
#include <QDomDocument>
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
// The Ostara namespace lives in the shared SVG helpers: a second document now marks its own
// attributes with it, and two spellings of one identifier is one too many.

/** @brief Value of the ostara:layout marker set on the decor layer's <g>. */
constexpr auto kOstaraLayoutMarker = "yes";

/**
 * @brief Where one owner's decor is written: the file, or the owner's own frame.
 *
 * The SAME serializer serves two destinations, and this is the only thing that differs between
 * them — so there is one implementation of "how a grainline is drawn", never two that could drift.
 *
 * @li the visible Layers, where the ink is in scene coordinates and the page transform brings it
 *     into the file;
 * @li the payload's per-size copy, where the ink is in the OWNER's frame — the same frame the
 *     piece's outline travels in — so a reader places it with its own placement and computes
 *     nothing.
 *
 * A record (cartouche, annotation) is already written in the owner's frame; only the page
 * transform is added for the file. The item ink is in scene coordinates, so the file case maps it
 * through `sceneTransform()`, the payload case through the parent's inverse — which is the same
 * matrix composed the other way, and why one formula covers both.
 */
struct InkSpace
{
    QTransform page{};      ///< scene → file; identity for the payload
    bool ownerFrame{false}; ///< true: the ink is written in its owner's frame

    /// What the owner's placement did to its ink, to be undone — see LayoutObject::ownerPlacement.
    ///
    /// ⚠️ **Not the parent item's transform.** A piece's placement is baked into the paths
    /// themselves, so its item transform is the identity and the parent's inversion would undo
    /// nothing at all: the payload came out written at the sheet position of every piece, and the
    /// reader placed it a second time.
    QTransform ownerPlacement{};

    /// Item-local → the space this ink is written in.
    ///
    /// ⚠️ **L'ordre compte, et il était à l'envers.** `QTransform::A * B` veut dire « A, puis B »
    /// (la règle que les commentaires de ce fichier rappellent trois fois) : il faut donc amener
    /// l'encre dans la scène D'ABORD, puis défaire le placement du propriétaire. Écrit dans l'autre
    /// sens, on défaisait le placement avant de l'avoir subi, et un item qui porte son **propre**
    /// transform — la légende du pli, tournée et posée le long de l'axe — ressortait en
    /// `P⁻¹ · M · P` : mesuré sur le payload de ce matin, la légende de « Dos » était écrite à
    /// **x = 4924**, à 129 cm de sa pièce, donc hors du dessin. Les items sans transform propre
    /// (le trait de pli, le droit-fil) donnaient `P⁻¹ · P = I` dans les deux sens — c'est pourquoi
    /// le défaut ne se voyait que sur la légende.
    auto ToFile(const QGraphicsItem *item) const -> QTransform
    {
        if (item == nullptr)
        {
            return ownerFrame ? ownerPlacement.inverted() : page;
        }
        const QTransform scene = item->sceneTransform();
        if (!ownerFrame)
        {
            return scene * page;
        }
        return scene * ownerPlacement.inverted();
    }

    /// An owner-frame transform (a record's placement) → the space this ink is written in.
    ///
    /// ⚠️ In the owner's frame the answer is the IDENTITY, never @p owner: that transform is the
    /// owner's own placement on the sheet (a group host carries one), and the records are already
    /// stored in the frame that placement maps FROM. Keeping it would write the cartouche at the
    /// sheet position inside a record that a reader will place again.
    auto RecordsToFile(const QTransform &owner) const -> QTransform
    {
        return ownerFrame ? QTransform() : owner * page;
    }
};

/** @brief Serialize one visible trace (path item) — mirrors the flat writer. */
void WriteTrace(QXmlStreamWriter &writer, const InkSpace &space, const QPen &defaultPen,
                QGraphicsItem *gitem, bool includeHidden = false)
{
    QGraphicsPathItem *const pathItem = OSvg::AsPathItem(gitem);
    if (pathItem == nullptr || (!includeHidden && !pathItem->isVisible()))
    {
        return;
    }
    const QPainterPath svgPath = space.ToFile(pathItem).map(pathItem->path());
    if (svgPath.isEmpty())
    {
        return;
    }

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
    // **What this path IS, not only what it looks like.** A grainline and a fold line are drawn as
    // paths like any other, so a reader that only sees the drawing cannot tell them from an outline
    // — and Rabnest has to: the grain decides which way a piece may be turned, and the fold decides
    // where it cannot be cut. Two attributes, written once, and the information stops being a
    // picture.
    if (gitem->type() == VGraphicsFillItem::Type)
    {
        writer.writeAttribute(QStringLiteral("ostara:grain"), QStringLiteral("yes"));
    }
    else if (gitem->type() == VGraphicsFoldLineItem::Type)
    {
        writer.writeAttribute(QStringLiteral("ostara:fold"), QStringLiteral("yes"));
    }
    const QString dash = OSvg::DashArray(pen);
    if (!dash.isEmpty())
    {
        writer.writeAttribute(QStringLiteral("stroke-dasharray"), dash);
    }
    writer.writeEndElement();  // path
}

/** @brief The same caption, as OUTLINES — the form a file that carries no font can still draw.
 *
 * ⚠️ **Un `<text>` ne voyage pas.** Le lecteur d'un payload n'a pas la police du patron : d'une
 * légende écrite en texte il ne reste rien, ou bien elle garde son ancre et se retrouve hors du
 * dessin. Les cartouches ont déjà tranché la question (`OCartoucheSvg`, `textAsPaths`) ; la légende
 * du pli suit la même règle et pour la même raison.
 *
 * Le tracé est construit exactement là où `WriteText` écrit son ancre — à l'origine de l'item, sur
 * la ligne de base à `ascent` — puis posé par la MÊME transformation, pour que les deux formes
 * donnent la même encre.
 */
void WriteTextAsPath(QXmlStreamWriter &writer, const InkSpace &space,
                     QGraphicsSimpleTextItem *textItem, bool includeHidden = false)
{
    if (textItem == nullptr || (!includeHidden && !textItem->isVisible())
        || textItem->text().isEmpty())
    {
        return;
    }
    const QFont font = textItem->font();
    const QFontMetricsF fm(font);
    QPainterPath glyphs;
    glyphs.addText(0.0, fm.ascent(), font, textItem->text());
    if (glyphs.isEmpty())
    {
        return;
    }
    writer.writeStartElement(QStringLiteral("path"));
    writer.writeAttribute(QStringLiteral("d"),
                          OSvg::PainterPathToSvgD(space.ToFile(textItem).map(glyphs)));
    writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
    // ⚠️ **Les deux pièges des cartouches, qui coûtent un cerne noir autour de chaque glyphe.** Un
    // tracé rempli qui suit un tracé tracé garde le stylo en main si le fichier ne le dit pas, et un
    // tracé de texte est construit en OddEven quand SVG suppose nonzero : sans la règle, les creux
    // des lettres se remplissent et les mots sortent gras et baveux.
    writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("#000"));
    writer.writeAttribute(QStringLiteral("stroke-width"), QStringLiteral(".01"));
    writer.writeAttribute(QStringLiteral("fill-rule"),
                          glyphs.fillRule() == Qt::OddEvenFill ? QStringLiteral("evenodd")
                                                              : QStringLiteral("nonzero"));
    writer.writeEndElement();  // path
}

/** @brief Serialize one editable simple-text item — mirrors the flat writer. */
void WriteText(QXmlStreamWriter &writer, const InkSpace &space, QGraphicsSimpleTextItem *textItem,
               bool includeHidden = false)
{
    if (textItem == nullptr || (!includeHidden && !textItem->isVisible())
        || textItem->text().isEmpty())
    {
        return;
    }
    const QFont font = textItem->font();
    const QFontMetricsF fm(font);
    const QTransform tx = space.ToFile(textItem);
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
    // The RESOLVED weight, never the requested one: an unset weight is asked for as nought, and a
    // nought written here reads back as the nearest face Qt can find to nothing at all — measured,
    // « ExtraLight » where the application draws « Regular ». The other two writers of this file
    // have always read it back from `QFontInfo`, and this one did not.
    const QFontInfo resolved(font);
    writer.writeAttribute(QStringLiteral("font-weight"),
                          QString::number(resolved.weight()));
    // **The face's NAME, because a weight number names no face** — see WriteBlock, which has carried
    // it since the 22-09 morning. Without it, Trame re-resolves from the number and can land on
    // another face than the application's, in silence.
    writer.writeAttribute(QStringLiteral("ostara:font-face"), resolved.styleName());
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
            // The marks the line stands for, before its words — written here for the reason the
            // shape above is: Paint draws them, so the file carries them. Traced first, then filled,
            // which is the order the painter draws them in.
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
                                                          * block.fontScale));
                    writer.writeEndElement();  // path
                }
                for (const QPainterPath &path : el.marks.fills)
                {
                    writer.writeStartElement(QStringLiteral("path"));
                    writer.writeAttribute(QStringLiteral("d"),
                                          OSvg::PainterPathToSvgD(place.map(path)));
                    writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
                    writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("none"));
                    writer.writeEndElement();  // path
                }
            }

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
            // **The face's NAME, because a weight number names no face** — « Light » (300) draws
            // « ExtraLight », « Black » (900) draws « ExtraBold », measured on the fonts installed
            // here. Trame reads this attribute and sets the face itself.
            writer.writeAttribute(QStringLiteral("ostara:font-face"), info.styleName());
            writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("black"));
            writer.writeCharacters(el.text);
            writer.writeEndElement();  // text

            // The sample the line carries, right after the words it follows and inside the same
            // group, so it inherits the block's transform and lands where the screen draws it.
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
    if (!annotation.fontFace.isEmpty())
    {
        font.setStyleName(annotation.fontFace);
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

        // ⚠️ **En CONTOURS, jamais en texte.** C'est la règle que les légendes de pli suivent déjà
        // (`textAsPaths`), et la raison est la même : le lecteur du payload n'a pas les polices du
        // patron. Écrite en `<text>`, l'annotation était rendue par la police de substitution du
        // lecteur — métriques différentes, texte qui débordait de sa case, taille fausse. Un chemin
        // ne dépend d'aucune police : il se dessine à l'identique partout.
        QPainterPath linePath;
        linePath.addText(QPointF(x, y), font, line);
        const QString shape = OSvg::PainterPathToSvgD(linePath);
        if (shape.isEmpty())
        {
            continue;
        }
        writer.writeStartElement(QStringLiteral("path"));
        writer.writeAttribute(QStringLiteral("d"), shape);
        writer.writeAttribute(QStringLiteral("fill"), fill);
        writer.writeEndElement();  // path (the line, as outlines)
    }
    writer.writeEndElement();  // g (annotation)
}

/** @brief Emit one miniature: the piece's drawing at the reference size, and the words it carries.
 *
 * The frame goes in by hand exactly as a block's does — the caller has placed the block in its
 * owner's frame, and this writer only adds the translation to that top-left. No `toSVG` here for the
 * same reason WriteBlock takes none: the page offset belongs to the wrapper the record is written
 * into, applied AFTER the owner's placement, where in the chain it belongs.
 *
 * ⚠️ **The fit is applied FIRST, and the factors below are written in the order they apply — left
 * first**: Qt's `A * B` means "A, then B" (the rule the block comment above spells out). Read the
 * other way round, the drawing is fitted LAST and the miniature lands near the pattern origin.
 */
void WriteMiniature(QXmlStreamWriter &writer, const QPointF &origin,
                    const OInfoBlockSvg::Thumbnail &thumb)
{
    if (thumb.content.isEmpty() || thumb.contentBounds.isEmpty() || thumb.rect.width() <= 0.0
        || thumb.rect.height() <= 0.0)
    {
        return;
    }

    // The drawing goes out as one absolute path, like every other path in this document: the piece's
    // coordinates are fitted into the block, the block is put where it belongs in its owner, and the
    // wrapper does the rest. No group has to carry it, and the stroke keeps the width the other
    // paths of this document are given.
    const QRectF block(0.0, 0.0, thumb.rect.width(), thumb.rect.height());
    QTransform tr;
    tr.translate(origin.x() + thumb.rect.x(), origin.y() + thumb.rect.y());
    const QPainterPath svgPath =
        (OCartouchePainter::FitTransform(thumb.contentBounds, block) * tr).map(thumb.content);

    writer.writeStartElement(QStringLiteral("path"));
    writer.writeAttribute(QStringLiteral("d"), OSvg::PainterPathToSvgD(svgPath));
    writer.writeAttribute(QStringLiteral("fill"), QStringLiteral("none"));
    writer.writeAttribute(QStringLiteral("stroke"), QStringLiteral("black"));
    writer.writeAttribute(QStringLiteral("stroke-width"),
                          QString::number(OInfoBlockSvg::MiniatureStroke()));
    writer.writeEndElement();  // path (miniature drawing)

    if (thumb.words.isEmpty())
    {
        return;
    }

    // The words are already placed in the block's frame — the item answers where a word goes, and
    // both canvases and both writers ask it the same question. They take the block's own placement
    // as their transform, in the same application order as the drawing above.
    writer.writeStartElement(QStringLiteral("g"));
    writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(tr));

    QVector<OCartoucheRenderItem> wordItems;
    wordItems.reserve(thumb.words.size());
    for (const OInfoBlockSvg::ThumbnailWord &word : thumb.words)
    {
        if (word.text.isEmpty())
        {
            continue;
        }
        // **The two texts are set apart**, exactly as OInfoBlockSvg::WriteSvg writes the same
        // miniature: a record carries the legend among the values — the writers place words, they
        // never label them — so each word is measured and drawn in the font its own text is set in.
        const QFont &wordFont = word.legend ? thumb.legendFont : thumb.font;
        const QFontMetricsF metrics(wordFont);
        // **One line per language, as on the canvas.** A legend written in two languages is one
        // string with a line break in it, and SVG has no line break: a newline inside `<text>`
        // collapses into a space, so the two languages came out end to end on a single line — far
        // wider than the block they belong to. The pitch is the font's own `height()`, the same one
        // the item measures its box with.
        const QStringList lines = word.text.split(QLatin1Char('\n'));
        qreal baseline = word.origin.y() + metrics.ascent();
        for (const QString &line : lines)
        {
            if (!line.isEmpty())
            {
                OCartoucheRenderItem item;
                item.kind = OCartoucheRenderItem::Kind::Text;
                item.text = line;
                item.font = wordFont;
                // The anchor is the top-left of the word's box, all that the screen ever needs; a
                // `<text>` wants its baseline, which is one ascent lower.
                item.pen = QPointF(word.origin.x(), baseline);
                wordItems.append(item);
            }
            baseline += metrics.height();
        }
    }
    // The items go to the shared serializer, so a miniature's wording is written exactly like a
    // block's elsewhere in this document — and, since this document's text stays editable, as words.
    OCartoucheSvg::WriteItems(writer, wordItems, 1.0, /*textAsPaths*/ false);
    writer.writeEndElement();  // g (miniature words)
}
}  // namespace

/**
 * @brief The body of one owner's carrier: its two families of ink, written in @p space.
 *
 * Kept out of the layer loop because it has TWO callers — the visible decor layer and the
 * payload's per-size copy — and because the two must never be two implementations.
 */
void WriteOwnerBody(QXmlStreamWriter &writer, const LayoutObject &object, const InkSpace &space,
                    const QPen &defaultPen, const QTransform &foldProjection = QTransform(),
                    bool textAsPaths = false, bool includeHidden = false)
{
        // Two wrappers, because the two halves of the decor do not live in the same
        // frame. The items were placed in the scene and already carry their owner's
        // placement; the records were not, they are written in their owner's own
        // frame. The transform below therefore goes on an inner wrapper — put on the
        // outer one it would move the items a second time.
        for (QGraphicsItem *child : object.decorChildren)
        {
            // ⚠️ **The fold mark has its own seat, and its caption travels with it.** The mark runs
            // along the edge that goes on the cloth's fold, and that edge moves with the size while
            // nothing else does; the payload therefore projects it on the fold's own axis, and the
            // rest of the decor on the piece's centre.
            //
            // ⚠️ **And the caption IS part of the mark.** The only `QGraphicsSimpleTextItem` a piece
            // ever puts in its decor is the fold's caption — it is the only place one is created, in
            // `VLayoutPiece::CreateLabel` (the mark's text branch) and in
            // `VToolSeamAllowance::ApplyFoldLineLabel` (the live tool). Left out of this seat, the
            // caption kept the centre's projection and slid off the drawing as the size grew.
            const int childType = child->type();
            const bool isFold = childType == VGraphicsFoldLineItem::Type
                                || childType == QGraphicsSimpleTextItem::Type;
            const bool shifted = isFold && !foldProjection.isIdentity();
            if (shifted)
            {
                writer.writeStartElement(QStringLiteral("g"));
                writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(foldProjection));
            }

            if (childType == QGraphicsSimpleTextItem::Type)
            {
                auto *text = static_cast<QGraphicsSimpleTextItem *>(child);
                // The payload carries outlines, the visible layer keeps its editable text: a reader
                // of the payload has none of the pattern's fonts.
                if (textAsPaths)
                {
                    WriteTextAsPath(writer, space, text, includeHidden);
                }
                else
                {
                    WriteText(writer, space, text, includeHidden);
                }
            }
            else
            {
                WriteTrace(writer, space, defaultPen, child, includeHidden);
            }

            if (shifted)
            {
                writer.writeEndElement();  // g (the fold's own seat)
            }
        }

        // The records take the page offset and the owner's placement together, in that
        // order: the owner's frame first, the page's own translation last. The decor
        // children above need no wrapper — their scene coordinates are already mapped
        // through the space one by one, so this is the only place the two meet.
        const QTransform recordTx = space.RecordsToFile(object.transform);
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
        // The miniatures close the records, after the two families of text. They are a child of
        // the piece like the others: one per owner — a group carries one per member — written
        // once, in the reference layer, whichever sizes the run covers.
        for (const OInfoBlockSvg::Thumbnail &thumbnail : object.thumbnails)
        {
            WriteMiniature(writer, object.origin, thumbnail);
        }
        if (placed)
        {
            writer.writeEndElement();  // g (records, in their owner's frame)
        }
}

//---------------------------------------------------------------------------------------------------------------------
bool WriteGradedSvg(const QString &fileName, const QList<SizeLayer> &sizeLayers,
                    const QString &layoutLayerName, const QList<LayoutObject> &layoutObjects,
                    const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
                    const QString &title, const QString &description, bool layered,
                    const QDomElement &metadata)
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

    // The visible layers write their ink in SCENE coordinates, through the page transform — the one
    // thing that differs from the payload's copy, which is written in its owner's frame.
    InkSpace sceneSpace;
    sceneSpace.page = toSVG;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("svg"));
    writer.writeDefaultNamespace(QStringLiteral("http://www.w3.org/2000/svg"));
    writer.writeNamespace(QStringLiteral("http://ns.adobe.com/AdobeIllustrator/10.0/"), QStringLiteral("i"));
    writer.writeNamespace(QStringLiteral("http://www.inkscape.org/namespaces/inkscape"),
                          QStringLiteral("inkscape"));
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
    // What the file was made with, for whoever rebuilds the paper instead of reading it. Written
    // before anything is drawn, because it is metadata and not content: a reader that skips it
    // skips it whole, and the drawing below is exactly what it would have been without it.
    if (!metadata.isNull())
    {
        OExportRecipe::Write(writer, metadata);
    }

    // One unique-id table per layer: ids may repeat across size layers (each size shows
    // the same piece names) — Illustrator/Inkscape accept that, and a global counter
    // would wrongly rename "Piece 1" to "Piece 1_1" in layer 40.
    const auto emitPieceGeometry = [&](QGraphicsItem *root) {
        WriteTrace(writer, sceneSpace, defaultPen, root);
        const QList<QGraphicsItem *> children = root->childItems();
        for (QGraphicsItem *child : children)
        {
            if (!OSvg::IsInfoChild(child))
            {
                WriteTrace(writer, sceneSpace, defaultPen, child);
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
            //
            // ⚠️ **Only when there IS a layout layer.** The mark is read by a filter that
            // walks the layout layer's children; a flat document has no such layer, and the
            // mark there says something else entirely: the PDF writer takes every
            // `i:layer="yes"` at the top of the file for an optional content group, so the
            // decor objects became the only "layers" — and everything written OUTSIDE one,
            // the pieces and their cartouche, was dropped. A flat document has to be flat
            // all the way down: no layer group, no layer mark, and the PDF writer then takes
            // its plain branch and draws the whole file (see SvgToOcgPdf, "no layer group").
            if (layered)
            {
                writer.writeAttribute(QStringLiteral("i:layer"), QStringLiteral("yes"));
            }

            WriteOwnerBody(writer, object, sceneSpace, defaultPen);
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

//---------------------------------------------------------------------------------------------------------------------
auto OwnerDecorFor(QDomDocument &doc, const LayoutObject &object, const QTransform &projection,
                  const QTransform &foldProjection) -> QDomElement
{
    // The same serializer as the visible layer, in the other frame — that is the whole point of
    // InkSpace, and the reason this function exists instead of a second implementation.
    QByteArray buffer;
    QXmlStreamWriter writer(&buffer);

    InkSpace space;
    space.ownerFrame = true;
    space.ownerPlacement = object.ownerPlacement;

    writer.writeStartElement(QStringLiteral("decor"));
    // One wrapper, and only when there is something to say: the projection of the reference's seat
    // onto this size's piece. Wrapping the whole body — ink, records and captions together — is what
    // keeps them one drawing; transforming them one by one is how they come apart.
    const bool projected = !projection.isIdentity();
    if (projected)
    {
        writer.writeStartElement(QStringLiteral("g"));
        writer.writeAttribute(QStringLiteral("transform"), OSvg::MatrixAttr(projection));
    }

    // ⚠️ **Le pli ne suit pas l'assise du centre, et le groupe extérieur s'applique à lui aussi.**
    // Le trait de pli doit garder son x sur l'axe du pli de CETTE taille — un axe qui ne bouge pas
    // d'une taille à l'autre — tandis que le reste du décor suit le centre de la pièce, qui s'en va
    // vers la droite à chaque taille qui grandit. Écrites l'une dans l'autre, les deux translations
    // s'additionnaient : mesuré sur le payload de « Dos », le trait dérivait de +7,26 px à la 34
    // jusqu'à +106,62 px (2,82 cm) à la 46, et de +151,18 px (4,00 cm) sur « Ourlet dos » — avec
    // zéro à la taille de référence, où la projection du centre est l'identité, ce qui faisait croire
    // que le pli était juste. L'assise du pli est donc écrite dans le repère d'AVANT la projection :
    // `foldProjection · projection⁻¹`. Composée avec le groupe extérieur (SVG applique le transform
    // de l'enfant d'abord), elle redonne exactement `foldProjection`, et le trait reste sur son axe.
    QTransform foldSeat = foldProjection;
    if (projected && !foldProjection.isIdentity())
    {
        bool invertible = false;
        const QTransform back = projection.inverted(&invertible);
        if (invertible)
        {
            foldSeat = foldProjection * back;
        }
    }

    WriteOwnerBody(writer, object, space,
                   QPen(Qt::black, kDefaultStrokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
                   foldSeat,
                   // Le payload écrit les légendes en CONTOURS : son lecteur n'a pas les polices du
                   // patron. La couche visible, elle, garde son texte éditable.
                   true,
                   // ⚠️ **Et il écrit les enfants MASQUÉS.** Cacher le droit-fil et le pli des
                   // tailles autres que la référence est une décision de DESSIN — un seul décor sur
                   // la feuille — et non de contenu : le payload porte le décor de chaque taille, et
                   // celui de la 46 est précisément composé d'enfants que la couche visible cache.
                   // Sans ce drapeau, ils arrivaient vides et le plan perdait ses plis.
                   true);
    if (projected)
    {
        writer.writeEndElement();  // g (projection)
    }
    writer.writeEndElement();

    QDomDocument fragment;
    if (!fragment.setContent(buffer))
    {
        return {};
    }
    return doc.importNode(fragment.documentElement(), true).toElement();
}
}  // namespace OGradedSvg
