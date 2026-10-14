/************************************************************************
 **
 **  @file   oexportcoverpagesvg.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  Export mode — the cover page as an SVG document of its own.
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

#include "oexportcoverpagesvg.h"

#include "core/ocoverpagelayout.h"
#include "ocartouchesvg.h"
#include "osvgutils.h"

#include "../vmisc/defglobal.h"

#include <QFile>
#include <QXmlStreamWriter>

//---------------------------------------------------------------------------------------------------------------------
auto OExportCoverPageSvg::Write(const QString &fileName, const OExportCoverPage::Drawing &drawing,
                                const QRectF &pageRect) -> bool
{
    // Nothing to draw, and nothing to declare: a page whose bands are empty is not a page, and the
    // caller asks the seat before it lays anything down. Writing it would put an empty document in
    // the file and a page in the reader's panel.
    if (drawing.IsEmpty() || pageRect.width() <= 0.0 || pageRect.height() <= 0.0)
    {
        return false;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(false);
    writer.writeStartDocument();

    writer.writeStartElement(QStringLiteral("svg"));
    writer.writeDefaultNamespace(QStringLiteral("http://www.w3.org/2000/svg"));
    // Ostara's own attributes are marked, and the prefix has to be declared here for the document
    // to be XML at all: `ostara:font-face` is what tells Trame which FACE each line is set in.
    writer.writeNamespace(QLatin1String(OSvg::kOstaraNamespace), QStringLiteral("ostara"));

    {
        // The page's own size, in millimetres, and the viewBox in print pixels: the two figures are
        // the same length said twice, which is what makes a document at print size rather than a
        // document at a scale. The convention is the SVG writers' own — 96 px to the inch, the
        // resolution the whole export works in.
        constexpr qreal dpm = PrintDPI / 25.4;
        writer.writeAttribute(QStringLiteral("width"),
                              QStringLiteral("%1mm").arg(pageRect.width() / dpm, 0, 'f', 4));
        writer.writeAttribute(QStringLiteral("height"),
                              QStringLiteral("%1mm").arg(pageRect.height() / dpm, 0, 'f', 4));
    }
    writer.writeAttribute(QStringLiteral("viewBox"),
                          QStringLiteral("%1 %2 %3 %4")
                              .arg(pageRect.x())
                              .arg(pageRect.y())
                              .arg(pageRect.width())
                              .arg(pageRect.height()));

    // The bands, placed by the page's own layout and drawn at print size — a scale of 1 is the
    // printed page, and the page here IS the printed page. The same two calls the canvas paints
    // through, so what is written and what was shown cannot come apart.
    const QMap<OCoverPageTemplate::Placement, QRectF> cells =
        OCoverPageLayout::Place(drawing.bands);
    for (auto cell = cells.constBegin(); cell != cells.constEnd(); ++cell)
    {
        const QVector<OCartoucheRenderLine> lines = drawing.bands.value(cell.key());
        if (lines.isEmpty())
        {
            continue;
        }

        // ⚠️ The layout measures a band from the PAGE's top-left; this document is laid out in the
        // PLAN's coordinates, and the page's own place is added here — the one translation between
        // the two frames, and the reason PagePx is handed over beside the bands rather than
        // recomputed by each caller.
        const QRectF target(pageRect.left() + cell.value().left(),
                            pageRect.top() + cell.value().top(), cell.value().width(),
                            cell.value().height());

        // ⚠️ **The words stay words, and the FACE travels with them.**
        //
        // Outlined, the page carried whole letterforms and no face at all: faithful by construction
        // and impossible to edit — and it made the cover page the only document in the file drawn by
        // another route than the rest, which is exactly what is refused here: *what the app shows is
        // what the PDF prints.* The layout layer's own text is words; the page must read like it.
        //
        // What a `<text>` needs to be faithful is the FACE and not a weight NUMBER: `ostara:font-face`
        // carries the style name the application drew with — see OCartoucheSvg::WriteItems — and
        // Trame sets exactly that one. Measured: a line naming « SemiBold » comes out 348,4 wide
        // where the same words without the name come out 325,9. The face itself, and not a guess.
        OCartoucheSvg::WriteItems(writer, OCartouchePainter::Layout(target, lines, 1.0), 1.0,
                                  /*textAsPaths*/ false);
    }

    writer.writeEndElement();  // svg
    writer.writeEndDocument();
    file.close();

    return true;
}
