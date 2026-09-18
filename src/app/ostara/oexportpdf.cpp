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
#include "oexportsheets.h"
#include "oexportunderlay.h"
#include "ogradedsvgwriter.h"

#include "core/oexportoutput.h"
#include "core/oexporttemplate.h"

#include "../vmisc/projectversion.h"
#include "../vpatterndb/vcontainer.h"

#include "pdf/SvgToOcgPdf.h"  // Trame — writes the optional content groups

#include <QCoreApplication>
#include <QFileInfo>
#include <QGraphicsScene>
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
                       const VContainer *pattern, VAbstractPattern *doc,
                       const OExportTemplate *marks, bool framed, bool addLayoutLayer) -> bool
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

    // Which of those sheets is worth a page, read from the placements the run produced and any
    // hand has moved since — the generation's own, never a copy of them. The canvas asks the same
    // question of the same two things, so the paper on screen and the paper in the file are one
    // decision: a sheet a hand has emptied is not written, and a piece put back on a dropped one
    // brings its page back with it.
    const OExportSheets::Set sheets = OExportSheets::Compute(artboards, output.result.placements);

    QVector<QRectF> pages;
    if (artboards.valid)
    {
        pages.reserve(sheets.count);
        // Row by row, left to right — the order the sheets come off the printer in and are taped
        // back together in. A sheet with nothing on it is passed over: paper, ink and a hand to
        // tape it are what a page costs, and the nesting's short last row would otherwise be paid
        // for in full. What decides is the content and never the grid — a blank sheet that a hand
        // has since put a piece on is a real sheet again, and is written as one.
        for (int row = 0; row < artboards.rows; ++row)
        {
            for (int col = 0; col < artboards.columns; ++col)
            {
                if (!sheets.IsPresent(row, col))
                {
                    continue;
                }

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

    // ONE SVG, over the whole plan — not one per sheet. The drawing below is the same
    // everywhere, and a sheet has become a matter for the PDF: the plan is written once
    // into a Form XObject and every page references it, offset so that its own artboard
    // lands on the paper. Writing it per sheet was writing the same drawing N times, and
    // rendering it per sheet was rendering it N times.
    //
    // The rectangle is the plan's own, and the plan starts at the origin: that is what
    // keeps the writer's transform the identity and leaves the sheet offset entirely to
    // the PDF. A page is the plan read through a rectangle — and the writer does not crop
    // to what it is handed, it only sizes the root and shifts the content, so the plan
    // being whole here is exactly what each page needs.
    const QRectF imageRect = artboards.valid ? artboards.planRect : pages.first();

    const QString svgPath = dir.filePath(QStringLiteral("generation.svg"));
    if (!OGradedSvg::WriteGradedSvg(svgPath, layers, layoutLayerName, content.decor, imageRect,
                                    QMarginsF(), 1.0, 1.0, QString(), QString(), layered))
    {
        qWarning() << "OExportPdf: the SVG could not be written";
        return false;
    }

    // The sheet's own frame, its numbering and its registration marks go in as a SECOND
    // document, not into the one above. Trame draws that one on every page, under the layers
    // and — the whole point — OUTSIDE every optional content group, so that nothing in the
    // reader's layer panel can switch the paper's own frame off. A sheet whose marks can be
    // turned off is a sheet that can be assembled wrongly, and it is the silent kind of wrong:
    // the print looks right.
    //
    // It is written over the whole plan, in the plan's own frame, and each page shows of it
    // exactly what it shows of the plan — the same crop, by the same translation, so no page
    // identity has to cross into Trame. Drawn sheet by sheet it would be wrong: a seam belongs
    // to the two sheets that share it and must carry ONE line from the plan's anchor, or its
    // dashes restart at every edge and the join shows in print.
    //
    // Only where there is something to draw: a tiled run whose template resolved. A roll and
    // the A0 sheet have no sheets to cut and no frame to carry, and their export is left
    // exactly as it was — no second document, nothing added to the file.
    QString underlayPath;
    if (artboards.valid && marks != nullptr)
    {
        underlayPath = dir.filePath(QStringLiteral("underlay.svg"));
        if (!OExportUnderlay::Write(underlayPath, svgPath, *marks, artboards, sheets, framed))
        {
            qWarning() << "OExportPdf: the assembly marks could not be written";
            return false;
        }
    }

    // Trame reads the layers back out of the SVG and rebuilds them as optional content —
    // ONE set of layers for the whole document, not one per sheet, so switching a size off
    // hides it on every page at once. What it takes from us besides the file is the page
    // list: the artboards, in the order the sheets come off the printer. A page's size is
    // ours to state now rather than the SVG's to imply, and that is what lets one drawing
    // serve all of them.
    SvgToOcgPdf converter;
    // What the reader shows in its information panel. The title is the file's own name —
    // what this delivery is called. Author and creator both name the application, as
    // Illustrator does: the content and the file come from the same hand. The producer is
    // left empty on purpose, so the module that writes the PDF names itself.
    SvgToOcgPdf::DocumentInfo info;
    info.title = QFileInfo(pdfPath).fileName();
    info.author = QStringLiteral("Ostara %1").arg(AppDisplayVersion());
    info.creator = info.author;

    // The underlay is the fifth argument, and an empty one says there is none: Trame then
    // writes the page stream it has always written, byte for byte the same. The sixth says
    // whether the layout is offered as a layer — the one thing the dialog asks about, and it
    // opens on no.
    if (!converter.convert(svgPath, pages, pdfPath, info, underlayPath, addLayoutLayer))
    {
        qWarning() << "OExportPdf: the PDF could not be written";
        return false;
    }

    return true;
}
