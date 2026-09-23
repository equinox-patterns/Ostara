/************************************************************************
 **
 **  @file   oexportplanimagewriter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Implementation of the plan image writer.
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

#include "oexportplanimagewriter.h"

#include "../vmisc/defglobal.h"

#include <QBuffer>
#include <QByteArray>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QGraphicsScene>
#include <QImage>
#include <QImageWriter>
#include <QPainter>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSvgGenerator>
#include <QVector>

#include <QtGlobal>

#include <functional>

namespace
{
using OExportPlanImage::Plan;
using OExportPlanImage::Target;

/// What both syntaxes ask of their painter before anything is drawn.
///
/// The font is not a detail: OExportMarkPainter reads the family off the painter when the
/// template names none, so a generator's own default would answer where the screen's font is
/// the answer — the same plan drawn with two different sets of numbers.
void PreparePainter(QPainter &painter, QGraphicsScene *scene)
{
    painter.setRenderHint(QPainter::Antialiasing, true);

    // The plan is written smaller than it is drawn, and the cartouches carry raster logos.
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    painter.setFont(scene->font());
}

/// @brief Take every clip out of the document — they clip nothing, and they cost everything.
///
/// QGraphicsScene::render clips the painter to the rectangle it renders, and QSvgGenerator turns
/// every clip into a <clipPath> and points nearly every <g> at it. Two sources feed them here:
/// the scene's own clip, which is the plan's rectangle — the very rectangle the root <svg>
/// already clips to, since it is the viewBox — and the cartouches, which clip themselves to
/// their own boxes.
///
/// ⚠️ MEASURED, and the whole reason this function exists: Illustrator drops the opacity of a
/// document that carries a clip-path. The SAME plan, the same text, the same opacities — with a
/// clip the text comes out solid black, without one it is correctly faded. Qt's own reader and
/// WebKit honour both, so the file is faithful in those and was not in the one the user has.
///
/// ⚠️ And removing them is free, MEASURED as well: the plan rendered before and after differs by
/// ZERO pixels out of 1 570 800. The cartouches' own clips clip nothing — their content already
/// fits the box they draw it in.
///
/// A document that cannot be read back comes back untouched. An export must not fail over a
/// nicety, and what QSvgGenerator wrote is already a valid file.
auto StripClips(const QByteArray &svg) -> QByteArray
{
    QDomDocument document;
    const QDomDocument::ParseResult parsed = document.setContent(svg);
    if (!parsed)
    {
        return svg;
    }

    QVector<QDomElement> definitions;
    const QDomNodeList found = document.elementsByTagName(QStringLiteral("clipPath"));
    for (int i = 0; i < found.count(); ++i)
    {
        definitions.append(found.at(i).toElement());
    }

    if (definitions.isEmpty())
    {
        return svg;
    }

    // And the definitions follow: a <clipPath> nothing points at is a definition of nothing.
    // The references go first, by a walk rather than a lookup by tag name: what matters is the
    // attribute, not the element that happens to carry it. QSvgGenerator writes it on <g> — this
    // does not have to be true for the removal to be right.
    std::function<void(QDomElement)> strip = [&strip](QDomElement element)
    {
        element.removeAttribute(QStringLiteral("clip-path"));

        for (QDomElement child = element.firstChildElement(); !child.isNull();
             child = child.nextSiblingElement())
        {
            strip(child);
        }
    };
    strip(document.documentElement());

    for (const QDomElement &definition : definitions)
    {
        definition.parentNode().removeChild(definition);
    }

    return document.toByteArray(-1);
}

/// @brief Move the opacity of every text off the text, and onto a group around it.
///
/// ⚠️ MEASURED on a real plan, opened in Illustrator: a group carrying `opacity` IS honoured —
/// the scissors and the assembly marks are drawn that way and they fade correctly — while a
/// <text> carrying only `fill-opacity` is not. Illustrator's own model puts transparency at the
/// level of the OBJECT, and a text object imported from SVG cannot carry it in its fill.
///
/// So the text is given what the shapes have: a group of its own, carrying the very figure its
/// fill had, and an opaque fill. The transform is EXACT rather than approximate — a reader
/// multiplied the group's opacity by the text's fill-opacity, and it now multiplies the same two
/// numbers in the other order. Nothing else is touched: the wrapper holds the text alone, so a
/// shape that happened to share the generated group is not dimmed by a figure meant for a text.
///
/// ⚠️ It cannot be done where the figure comes from. `ApplyTextStyle` puts it in the ink colour,
/// and Trame reads a text's opacity from `fill-opacity` ON THE ELEMENT when it re-emits the PDF's
/// decor (SvgToOcgPdf.cpp:1251). Moving it there would print the PDF's page numbers solid — a
/// validated feature. It is moved here, after generation, and only in this file.
auto LowerTextOpacity(const QByteArray &svg) -> QByteArray
{
    QDomDocument document;
    const QDomDocument::ParseResult parsed = document.setContent(svg);
    if (!parsed)
    {
        return svg;
    }

    QVector<QDomElement> texts;
    const QDomNodeList found = document.elementsByTagName(QStringLiteral("text"));
    for (int i = 0; i < found.count(); ++i)
    {
        texts.append(found.at(i).toElement());
    }

    for (QDomElement text : texts) // by value: QDomElement is a handle, and setAttribute is not const
    {
        const QString figure = text.attribute(QStringLiteral("fill-opacity"));

        bool ok = false;
        const qreal opacity = figure.toDouble(&ok);
        if (!ok || opacity <= 0.0 || opacity >= 1.0)
        {
            continue; // a text drawn at full strength needs no wrapper, and gets none
        }

        QDomNode parent = text.parentNode();
        if (parent.isNull())
        {
            continue;
        }

        QDomElement wrapper = document.createElement(QStringLiteral("g"));
        wrapper.setAttribute(QStringLiteral("opacity"), figure); // the figure itself, not a rewrite
        text.setAttribute(QStringLiteral("fill-opacity"), QStringLiteral("1"));

        parent.replaceChild(wrapper, text); // the wrapper takes the text's place...
        wrapper.appendChild(text);          // ...and the text moves into it
    }

    return document.toByteArray(-1);
}

/// @brief Write @p plan as an SVG whose page measures the size that was asked for.
///
/// The scale is carried by the pair size/viewBox, and the painter stays at 1. That is the road
/// OExportUnderlay::Write takes, and the reason is the same: the generator declares its page in
/// millimetres from `size.width() * 25.4 / resolution`, so the two figures together say how big
/// the file is and how much drawing goes into it.
///
/// ⚠️ Scaling the painter as well would write the plan at 1/96 of the size asked for, and
/// nothing would say so. Neither may take both roads.
///
/// ⚠️ No translate(-planRect.topLeft()) either, unlike the underlay. The viewBox is written
/// verbatim and is never re-applied as a transformation — it already places the origin. The
/// plan's rectangle starts at (0, 0), so the translation would be the identity anyway and only
/// would sit there waiting to be wrong.
auto WriteSvg(const QString &path, QGraphicsScene *scene, const Plan &plan, const Target &target,
              const QString &title) -> bool
{
    // ⚠️ Never setFileName: the generator would open the file itself and report a failure with
    // a qWarning — which in Ostara opens a modal box before the caller can say what happened.
    // Buffered, the write is ours, and its failure is one boolean.
    QByteArray buffer;
    QBuffer device(&buffer);
    if (!device.open(QIODevice::WriteOnly))
    {
        return false;
    }

    // ⚠️ SVG 1.1, and NOT the SvgTiny12 Qt defaults to — this line is what makes the file
    // faithful anywhere. The profile is not a nicety: SVG Tiny 1.2 restricts the `opacity`
    // property to the `image` element alone and says it "must be treated as an unsupported
    // value" everywhere else (§11.12). A reader that honours the declared profile therefore
    // drops EVERY group opacity, which is exactly what the assembly marks and the frame are
    // made of — Illustrator does, a browser does not, and the file has no business choosing
    // between them. 1.1 is the profile every authoring tool is built around.
    //
    // It gives back the clips as well: Tiny 1.2 has none, so none were ever written, and the
    // setClipPath DrawMarks puts before its marks was silently discarded.
    QSvgGenerator generator(QSvgGenerator::SvgVersion::Svg11);
    generator.setOutputDevice(&device);
    generator.setResolution(qRound(PrintDPI));
    generator.setSize(target.sizePx);
    generator.setViewBox(plan.rectPx);
    if (!title.isEmpty())
    {
        generator.setTitle(title);
    }

    {
        QPainter painter(&generator);
        if (!painter.isActive())
        {
            return false;
        }

        PreparePainter(painter, scene);

        // Target and source are the same rectangle: with IgnoreAspectRatio the painter's
        // transform is then a translation of the source's own origin, and the drawing comes out
        // in the scene's pixels — which is exactly what the viewBox declares.
        scene->render(&painter, plan.rectPx, plan.rectPx, Qt::IgnoreAspectRatio);
    } // ~QPainter — it is the destructor that flushes the generator

    device.close();

    // The generator's output is not quite what is written. Two passes, each answering a
    // MEASURED defect of the format as other software reads it — see both functions.
    const QByteArray withoutClips = StripClips(buffer);
    const QByteArray document = LowerTextOpacity(withoutClips);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    if (file.write(document) != document.size())
    {
        return false;
    }

    return file.commit();
}

/// @brief Write @p plan as a PNG of exactly the pixels that were counted.
///
/// Here the scale IS the painter's: target and source differ, so render() lays the plan's
/// rectangle onto the image's own pixels. The SVG takes the other road; neither takes both.
auto WritePng(const QString &path, QGraphicsScene *scene, const Plan &plan, const Target &target)
    -> bool
{
    QImage image(target.sizePx, QImage::Format_ARGB32);
    if (image.isNull())
    {
        return false;
    }

    image.fill(Qt::white);

    // ⚠️ Written into the file, and not left to Qt's default — which is the SCREEN's resolution
    // (QImageData initialises it from qt_defaultDpis()). The user asked for a figure, and a
    // raster that did not carry it would be placed three or four times too wide by whatever
    // lays it out. This is a deliberate departure from the house habit: nowhere else in Ostara
    // sets these, because nowhere else is a resolution asked for.
    const int dotsPerMeter = qRound(target.dpi / 25.4 * 1000.0);
    image.setDotsPerMeterX(dotsPerMeter);
    image.setDotsPerMeterY(dotsPerMeter);

    {
        QPainter painter(&image);
        PreparePainter(painter, scene);

        scene->render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(target.sizePx)), plan.rectPx,
                      Qt::IgnoreAspectRatio);
    } // ~QPainter — the image is not readable until the painting is over

    // QImage::save() would open the file itself and warn its own way on failure; a modal box is
    // the caller's to raise, once, with the generation's name in it.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }

    QImageWriter writer(&file, QByteArrayLiteral("png"));
    if (!writer.write(image))
    {
        return false;
    }

    return file.commit();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlanImageWriter::Write(const QString &path, QGraphicsScene *scene, const Plan &plan,
                                   const Target &target, Format format, const QString &title) -> bool
{
    if (scene == nullptr || path.isEmpty() || !target.IsValid() || plan.rectPx.isEmpty())
    {
        return false;
    }

    return (format == Format::Png) ? WritePng(path, scene, plan, target)
                                   : WriteSvg(path, scene, plan, target, title);
}
