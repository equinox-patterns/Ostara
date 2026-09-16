/************************************************************************
 **
 **  @file   oexportpdf.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the PDF export of a generation.
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

#include "oexportpdf.h"

#include "oexportartboards.h"
#include "oexportcanvas.h"
#include "ogradedsvgwriter.h"

#include "core/oexportoutput.h"

#include "../vmisc/projectversion.h"
#include "../vpatterndb/vcontainer.h"

#include "pdf/SvgToOcgPdf.h"  // Trame — writes the optional content groups

#include <QCoreApplication>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QStringList>
#include <QTemporaryDir>
#include <QVector>

namespace
{
/// @brief The decor layer's name when the range has none of its own.
///
/// Filed under MainWindow's context, and deliberately so: that is where the graded export
/// declares this same string, and therefore where its translation lives — the French UI
/// reads "Mise en page" from there. Asked under a context of our own, the lookup finds
/// nothing and the layer comes out under the English source string, which is precisely what
/// it did. One string and one context, until the two call sites are given a shared home;
/// lupdate merges them anyway, since context, source and disambiguation are all equal.
auto DefaultDecorLayerName() -> QString
{
    return QCoreApplication::translate("MainWindow", "Layout",
                                       "Decor layout layer default name in a graded SVG export");
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdf::Write(const OExportOutput &output, const QString &pdfPath,
                       const VContainer *pattern, VAbstractPattern *doc) -> bool
{
    if (pattern == nullptr || doc == nullptr || output.layers.isEmpty() || !output.result.ok)
    {
        qWarning() << "OExportPdf: the generation cannot be reproduced";
        return false;
    }

    // Draw the generation the way the canvas draws it, and take back what was drawn. The
    // scene is a local: everything the writer needs points into it, and it dies with this
    // function, once every SVG has been written.
    QGraphicsScene scene;
    OExportCanvasContent content;
    OExportCanvas::Build(&scene, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc, &content);

    // The page is the bin when the generation has no sheets to cut — a roll, the A0 sheet,
    // anything the print model left untuiled — and that is the whole of this export, left
    // exactly as it was.
    //
    // When the run IS tiled the plan is cut into its artboards, and each artboard becomes
    // a page. The grid is the SCREEN's: the same Compute() the canvas calls, so that the
    // paper cannot disagree with what is drawn, and frameHeightMm > 0 — the print model's
    // own token — is the only thing that decides it. Nothing is asked of the user, because
    // there is nothing left to ask: she already said what she prints on.
    const OExportArtboards::Grid artboards = OExportArtboards::Compute(
        output.result.binWidthPx, output.result.usedHeightPx, output.frameHeightMm,
        output.columns);

    QVector<QRectF> pages;
    if (artboards.valid)
    {
        pages.reserve(artboards.columns * artboards.rows);
        // Row by row, left to right — the order the sheets come off the printer in and are
        // taped back together in. A sheet is never cut short, so the last row is whole and
        // whatever the nesting left unused in it is printed as blank paper; that band is
        // where a hand correction may have put a piece, and it is a real sheet now.
        for (int row = 0; row < artboards.rows; ++row)
        {
            for (int col = 0; col < artboards.columns; ++col)
            {
                pages.append(QRectF(artboards.planRect.left() + col * artboards.cellWidthPx,
                                    artboards.planRect.top() + row * artboards.cellHeightPx,
                                    artboards.cellWidthPx, artboards.cellHeightPx));
            }
        }
    }
    else
    {
        pages.append(QRectF(0.0, 0.0, output.result.binWidthPx, output.result.usedHeightPx));
    }

    // The writer takes the layers largest first and paints them in that order, so the
    // smallest — the reference, the one the decor sits on — ends up on top, both in the
    // document and in the reader's panel. The canvas walks them the other way round,
    // because it draws the reference first. Hence the walk backwards.
    QList<OGradedSvg::SizeLayer> layers;
    layers.reserve(content.layers.size());
    for (int i = static_cast<int>(content.layers.size()) - 1; i >= 0; --i)
    {
        const OExportCanvasContent::Layer &layer = content.layers.at(i);

        OGradedSvg::SizeLayer sizeLayer;
        // The name the Size Styles give the size is the one written; the axis value stands
        // in when they give none.
        sizeLayer.label = layer.displayName.isEmpty() ? layer.label : layer.displayName;

        for (const OExportCanvasContent::Object &object : layer.objects)
        {
            OGradedSvg::GradedObject entry;
            entry.name = object.name;
            entry.isGroup = object.isGroup;

            if (object.isGroup)
            {
                for (const auto &member : object.members)
                {
                    entry.members.append({member.first, member.second});
                }
            }
            else
            {
                entry.loneItem = object.loneItem;
            }

            sizeLayer.objects.append(entry);
        }

        layers.append(sizeLayer);
    }

    // A range of one size is a single size: there is nothing to switch between, and a
    // layer panel offering one entry offers no choice.
    const bool layered = output.hasRange && layers.size() > 1;

    // The decor layer's name comes from the range, and from the range as it is NOW: the
    // decor follows the pattern, exactly as the canvas reads it live. A generation carries
    // its own copy of the range, taken when the run started — and that copy can predate a
    // rename, which would have handed out the old name for ever.
    QString layoutLayerName;
    if (output.hasRange)
    {
        if (const auto *ranges = pattern->SizeRanges(); ranges != nullptr)
        {
            layoutLayerName = ranges->value(output.rangeName).exportLayoutLayerName;
        }
        if (layoutLayerName.isEmpty())
        {
            layoutLayerName = output.range.exportLayoutLayerName;
        }
    }
    if (layered && layoutLayerName.isEmpty())
    {
        layoutLayerName = DefaultDecorLayerName();
    }

    QTemporaryDir dir;
    if (!dir.isValid())
    {
        qWarning() << "OExportPdf: no temporary directory";
        return false;
    }

    // One SVG per page. The drawing is the same in all of them — Build filled the content
    // once, and nothing in it depends on the sheet — so a page is that one drawing read
    // through a different rectangle: WriteGradedSvg crops to the rect it is handed, and the
    // width and height it writes are what Trame turns into that page's /MediaBox.
    QStringList svgPaths;
    svgPaths.reserve(pages.size());
    for (int i = 0; i < pages.size(); ++i)
    {
        const QString svgPath =
            dir.filePath(QStringLiteral("generation_%1.svg").arg(i, 4, 10, QLatin1Char('0')));
        if (!OGradedSvg::WriteGradedSvg(svgPath, layers, layoutLayerName, content.decor,
                                        pages.at(i), QMarginsF(), 1.0, 1.0, QString(), QString(),
                                        layered))
        {
            qWarning() << "OExportPdf: the SVG could not be written for page" << i + 1;
            return false;
        }
        svgPaths.append(svgPath);
    }
    // Trame reads the layers back out of the SVGs and rebuilds them as optional content —
    // ONE set of layers for the whole document, not one per sheet, so switching a size off
    // hides it on every page at once. Handing it files rather than buffers is its own
    // interface, and the files are what hold the page geometry: the converter takes each
    // page's size from its own SVG, not from us.
    SvgToOcgPdf converter;
    // What the reader shows in its information panel. The title is the file's own name —
    // what this delivery is called. Author and creator both name the application, as
    // Illustrator does: the content and the file come from the same hand. The producer is
    // left empty on purpose, so the module that writes the PDF names itself.
    SvgToOcgPdf::DocumentInfo info;
    info.title = QFileInfo(pdfPath).fileName();
    info.author = QStringLiteral("Ostara %1").arg(AppDisplayVersion());
    info.creator = info.author;

    if (!converter.convert(svgPaths, pdfPath, info))
    {
        qWarning() << "OExportPdf: the PDF could not be written";
        return false;
    }

    return true;
}
