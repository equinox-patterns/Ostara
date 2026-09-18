/************************************************************************
 **
 **  @file   oexportunderlay.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  Export mode — write a template's assembly marks as the document the PDF
 **          draws under the plan, on every one of its pages.
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

#include "oexportunderlay.h"

#include "core/oexporttemplate.h"
#include "oexportmarkpainter.h"

#include "../vmisc/def.h"
#include "../vmisc/defglobal.h"

#include <QDomDocument>
#include <QFile>
#include <QPainter>
#include <QSize>
#include <QSvgGenerator>

namespace
{
/**
 * @brief Gives @p svgPath the page of @p planSvgPath — its width, its height and its viewBox.
 *
 * The generator is not allowed to write those three, and the reason is not tidiness. It measures
 * in WHOLE PIXELS — `QSvgGenerator::setSize` takes a `QSize` — while the plan's rectangle is
 * fractional: a sheet of 259 mm is 978,89 px, and the millimetres the generator prints back are
 * rounded with it. Trame reads both documents' pages and refuses the underlay outright unless
 * the two agree EXACTLY, so a rounded millimetre is not a slightly-off drawing: it is a refused
 * one. And a refused one is not a degraded export either — it is a `qWarning`, which in Ostara
 * opens a modal box. The user would be told her export failed by a dialog she cannot act on.
 *
 * So the figures are copied from the plan's own root instead. The equality then holds by
 * construction, rather than by two formattings of the same number happening to agree — which is
 * also why nothing here reformats them: they are moved across as the plan wrote them.
 *
 * The plan's root is the one document that has them: it is written before this, and Trame reads
 * its page the same way it will read ours. Everything but these three attributes is left alone,
 * and the root is the only part Trame discards — it re-roots every top-level child on the plan's
 * own page — so this is the whole of what the underlay's root is for.
 */
auto AdoptPlanPage(const QString &svgPath, const QString &planSvgPath) -> bool
{
    const auto read = [](const QString &path, QDomDocument &document) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "OExportUnderlay: cannot read" << path;
            return false;
        }
        const QDomDocument::ParseResult result = document.setContent(&file);
        file.close();

        if (!result)
        {
            qWarning() << "OExportUnderlay: XML parse error at" << result.errorLine << ":"
                       << result.errorMessage;
            return false;
        }
        return true;
    };

    QDomDocument plan;
    if (!read(planSvgPath, plan))
    {
        return false;
    }
    QDomDocument underlay;
    if (!read(svgPath, underlay))
    {
        return false;
    }

    const QDomElement planRoot = plan.documentElement();
    QDomElement root = underlay.documentElement();
    for (const char *name : {"width", "height", "viewBox"})
    {
        const QString attribute = QString::fromLatin1(name);
        root.setAttribute(attribute, planRoot.attribute(attribute));
    }

    QFile file(svgPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning() << "OExportUnderlay: cannot write" << svgPath;
        return false;
    }
    const QByteArray out = underlay.toByteArray(-1);
    const bool written = file.write(out) == out.size();
    file.close();

    if (!written)
    {
        qWarning() << "OExportUnderlay: the document was not written whole";
    }
    return written;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
bool OExportUnderlay::Write(const QString &svgPath, const QString &planSvgPath,
                            const OExportTemplate &marks, const OExportArtboards::Grid &artboards,
                            const OExportSheets::Set &sheets, bool framed)
{
    // Built here exactly as the canvas builds it — same fields, same sources — so that the two
    // cannot place a mark differently. It is what tells the painter which sheets are printed,
    // and the marks follow the paper: a junction where fewer than two sheets meet carries
    // nothing, and a sheet that is not printed takes its scissors and its number with it.
    OExportMarkGrid grid;
    grid.columns = artboards.columns;
    grid.rows = artboards.rows;
    grid.sheetWidthPx = artboards.cellWidthPx;
    grid.sheetHeightPx = artboards.cellHeightPx;
    grid.pxPerMm = ToPixel(1.0, Unit::Mm);
    grid.sheets = sheets;

    // The template is handed over whole, and the painter decides what its values mean — a zero
    // opacity, a mark that is off, a form of numbering. It is the same object the preview and
    // the canvas use, which is what keeps the three from drifting apart.
    OExportMarkPainter decor;
    decor.SetFrameSettings(marks.frame);
    decor.SetMarkSettings(marks.marks);
    decor.SetPaginationSettings(marks.pagination);

    QSvgGenerator generator;
    generator.setFileName(svgPath);
    generator.setResolution(static_cast<int>(PrintDPI));
    generator.setSize(QSize(qRound(artboards.planRect.width()),
                            qRound(artboards.planRect.height())));
    generator.setViewBox(artboards.planRect);

    {
        QPainter painter(&generator);
        if (!painter.isActive())
        {
            qWarning() << "OExportUnderlay: the painter did not start on" << svgPath;
            return false;
        }

        // The plan's own frame, and the whole plan at once. A seam between two sheets is ONE
        // line belonging to both, drawn from the plan's anchor: drawn sheet by sheet it would
        // restart its dash pattern at every edge, and the join would show in print. The painter
        // still receives the plan's origin, because that is what it locates the frame and the
        // tiling against.
        //
        // The translate is what puts the drawing in the document's own coordinates rather than
        // in the plan's. It is the identity today — the tiling is built from the origin, so the
        // two are the same point — and it is written rather than left out so that the drawing
        // stays the plan's whatever the origin becomes, instead of following it in silence.
        painter.translate(-artboards.planRect.topLeft());

        // sheetGuide: no, and for the canvas's reason rather than the preview's — a page IS a
        // sheet here, so the edge of the paper is the edge of the sheet, and there is nothing
        // to stand in for.
        decor.Paint(painter, artboards.planRect.topLeft(), grid, framed, false);
    }

    return AdoptPlanPage(svgPath, planSvgPath);
}
