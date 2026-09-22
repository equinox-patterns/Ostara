/************************************************************************
 **
 **  @file   oexportcoverpage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  Export mode — where the cover page stands on a given format.
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

#include "oexportcoverpage.h"

#include "core/ocovergradedaxis.h"
#include "core/ocoverpagelayout.h"
#include "core/ocoverpageresolver.h"
#include "core/ocoverpagesizes.h"
#include "core/ocoverpagetemplatestorage.h"
#include "core/ocoverseamallowances.h"
#include "core/ocoversymbols.h"
#include "core/oexportoutput.h"
#include "core/oseamallowancelibrary.h"
#include "core/oseamallowancevalues.h"
#include "core/osizetable.h"
#include "core/osymbollibrary.h"
#include "dialogs/oexportprintmodel.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vwidgets/ocartouchepainter.h"
#include "xml/vabstractpattern.h"

#include <QSizeF>

//---------------------------------------------------------------------------------------------------------------------
auto OExportCoverPage::SeatOf(const OExportPrintModel *model) -> Seat
{
    // A null model is a token the table has since lost — a generation kept across a table
    // that shrank. It draws nothing rather than guessing a seat it cannot check.
    return model == nullptr ? Seat::None : model->coverSeat;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCoverPage::BlockPx(Seat seat, qreal marginPx) -> QRectF
{
    if (seat != Seat::InSheet)
    {
        return {};
    }

    // The page's own rectangle, off the edges by the DOCUMENT's margin — the figure the user set,
    // and not a second margin invented for the block. See the note in the header: the block was an
    // A4's rectangle at the absolute corner first, and it gave the nesting away for nothing.
    //
    // At the plan's top-left corner: y grows downward, so `marginPx` from the top is the margin.
    const QSizeF page = OCoverPageLayout::PageSize();
    return {marginPx, marginPx, page.width(), page.height()};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCoverPage::PaperPx(Seat seat, qreal marginPx, const QRectF &planRect,
                               qreal sheetWidthPx, qreal sheetHeightPx) -> QRectF
{
    switch (seat)
    {
        case Seat::InSheet:
            return BlockPx(seat, marginPx).translated(planRect.topLeft());

        case Seat::ExtraSheet:
            // Above row 1, column 1 — one sheet of the grid's own size. Negative Y is above
            // the plan, and that is all it is: `planRect` — the fence a hand may not cross —
            // is deliberately NOT grown upward with it, or a piece could be dragged onto the
            // cover page.
            return {planRect.left(), planRect.top() - sheetHeightPx,
                    sheetWidthPx, sheetHeightPx};

        case Seat::None:
            break;
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCoverPage::PagePx(Seat seat, qreal marginPx, const QRectF &planRect,
                              qreal sheetWidthPx, qreal sheetHeightPx) -> QRectF
{
    const QRectF paper = PaperPx(seat, marginPx, planRect, sheetWidthPx, sheetHeightPx);
    if (paper.isEmpty())
    {
        return {};
    }

    // The page's own size, and never a share of the paper: the cover page is 19 × 25.9 cm at
    // 100 %, which is what makes the canvas evidence about the paper rather than a picture of
    // it. A paper too small would let it overflow, centred — honest, and no format does it.
    const QSizeF page = OCoverPageLayout::PageSize();

    return {paper.left() + (paper.width() - page.width()) / 2.0,
            paper.top() + (paper.height() - page.height()) / 2.0,
            page.width(), page.height()};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCoverPage::Of(const OExportOutput &output, VAbstractPattern *doc) -> Drawing
{
    Drawing drawing;

    // The margin travels with the page, because a reserved block is placed with it — see the note
    // on Drawing::marginPx. Read from the RUN, like everything else here.
    drawing.marginPx = ToPixel(output.marginMm, Unit::Mm);

    // The seat comes from the FORMAT, and an empty name means no cover page at all. The two are the
    // same answer to the same question, and both leave every consumer with nothing to draw.
    drawing.seat = SeatOf(FindPrintModel(output.printModelToken));
    if (drawing.seat == Seat::None || output.coverPageTemplateName.isEmpty())
    {
        drawing.seat = Seat::None;
        return drawing;
    }

    // **By NAME, resolved afresh on every rebuild** — the marks' own rule and the marks' own reason:
    // a template is a file the user edits, and a copy taken at nesting time would go on drawing what
    // the file said that day. A name that resolves to nothing draws nothing (see
    // OCoverPageTemplateStorage::Resolve, which deliberately has no fallback).
    OCoverPageTemplate tpl;
    if (!OCoverPageTemplateStorage::Resolve(output.coverPageTemplateName, &tpl))
    {
        return drawing;
    }

    // The three stores a page can read, loaded for the length of this call. **Deliberately not
    // kept**: the plan on screen is rebuilt on demand, and a legend the user has since edited must
    // be the one drawn — which is the same reason the assembly marks are re-read every time.
    OSymbolLibrary::Library symbols = OSymbolLibrary::Load();
    OSeamAllowanceLibrary::Library seams = OSeamAllowanceLibrary::Load();
    OSizeTable table;
    table.Load();

    const VCoverPageData page = doc == nullptr ? VCoverPageData() : doc->GetCoverPage();

    const auto DataOf = [&](const OCoverPageTemplate::Section &section)
        -> OCoverPageResolver::PatternData {
        OCoverPageResolver::PatternData data;

        // ── What the page announces comes from the RUN (§6) ──────────────────
        //
        // **Which sizes are listed is the run's answer**, not the template's demonstration range.
        // The four values that say WHICH sizes are the generation's; the words and the sample
        // length stay the designer's and are read from the section's own options.
        OCoverGradedAxis axis = OCoverGradedAxisOptions::FromOptions(section.options);
        if (output.hasRange)
        {
            axis.measurement = output.range.measurement;
            axis.axis = output.range.sizeAxis;
            axis.minValue = output.range.minValue;
            axis.maxValue = output.range.maxValue;
        }
        data.sizes = OCoverPageSizes(&table, axis, VAbstractApplication::VApp()->Settings());

        // **Where each axis stands, said by the RUN.** A range says which sizes run and says nothing
        // about the axes that do not: those are frozen, and the run recorded their values — as it
        // recorded the one size a single-size run was asked for. Empty falls back to the table, which
        // is the preview's case and the only honest answer left.
        data.axisValues = output.hasRange ? output.range.axisValues : output.pinnedAxes;

        // What belongs to the DOCUMENT, and is re-read on every rebuild: the pattern's own name, the
        // text left to be filled in, the legend it chose, the margins it is cut with.
        data.name = doc == nullptr ? QString() : doc->GetPatternName();
        data.freeText = page.freeText;
        data.symbols = OCoverSymbols::Lines(symbols, tpl.languages, page.symbols);

        const QVector<qreal> widths = OSeamAllowanceValues::Collect(doc);
        if (!widths.isEmpty())
        {
            // The default margin is the application's, kept in the unit the application works in and
            // brought to millimetres here — the one unit a cover page states a length in.
            auto *settings = VAbstractApplication::VApp()->Settings();
            const qreal defaultMm = UnitConvertor(settings->GetDefaultSeamAllowance(),
                                                  StrToUnits(settings->GetUnit()), Unit::Mm);
            data.seamAllowances =
                OCoverSeamAllowances::Lines(seams, tpl.languages, widths, defaultMm,
                                            page.marginNames);
        }

        return data;
    };

    // **substituteEmpty false: this is the page the paper will carry.** An information that resolves
    // to nothing draws nothing — no stand-in for a value that is not there, which is the preview's
    // privilege and nobody else's (§6).
    drawing.bands = OCoverPageRender::Bands(tpl, DataOf, OCartouchePainter::BaseFont(), &table,
                                            /*substituteEmpty*/ false);
    return drawing;
}
