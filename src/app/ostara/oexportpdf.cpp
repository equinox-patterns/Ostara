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

#include "oexportcanvas.h"
#include "ogradedsvgwriter.h"

#include "core/oexportoutput.h"

#include "../vmisc/projectversion.h"
#include "../vpatterndb/vcontainer.h"

#include "pdf/SvgToOcgPdf.h"  // Trame — writes the optional content groups

#include <QCoreApplication>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QTemporaryDir>

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
    // function, once the SVG has been written.
    QGraphicsScene scene;
    OExportCanvasContent content;
    OExportCanvas::Build(&scene, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc, &content);

    // The page is the bin — the width the print model imposed, the height the nesting used.
    //
    // And on a tiled run this is deliberately NOT what the canvas shows: the screen lays the
    // result on the sheets it will be cut into, while one page here still holds the whole
    // plan. The two do not contradict each other — cutting the plan into sheets is precisely
    // what a tiled export has yet to learn, and until it does, a page that IS a sheet would
    // be a page with five sixth of the plan missing. The canvas is handed no grid here (the
    // parameter keeps its default), so nothing of it reaches this file or the SVG.
    const QRectF imageRect(0.0, 0.0, output.result.binWidthPx, output.result.usedHeightPx);

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

    const QString svgPath = dir.filePath(QStringLiteral("generation.svg"));
    if (!OGradedSvg::WriteGradedSvg(svgPath, layers, layoutLayerName, content.decor, imageRect,
                                    QMarginsF(), 1.0, 1.0, QString(), QString(), layered))
    {
        qWarning() << "OExportPdf: the SVG could not be written";
        return false;
    }
    // Trame reads the layers back out of the SVG and rebuilds them as optional content.
    // Handing it the file rather than a buffer is its own interface, and the file is what
    // holds the page geometry — the converter takes the page from the SVG, not from us.
    SvgToOcgPdf converter;
    // What the reader shows in its information panel. The title is the file's own name —
    // what this delivery is called. Author and creator both name the application, as
    // Illustrator does: the content and the file come from the same hand. The producer is
    // left empty on purpose, so the module that writes the PDF names itself.
    SvgToOcgPdf::DocumentInfo info;
    info.title = QFileInfo(pdfPath).fileName();
    info.author = QStringLiteral("Ostara %1").arg(AppDisplayVersion());
    info.creator = info.author;

    if (!converter.convert(svgPath, pdfPath, info))
    {
        qWarning() << "OExportPdf: the PDF could not be written";
        return false;
    }

    return true;
}
