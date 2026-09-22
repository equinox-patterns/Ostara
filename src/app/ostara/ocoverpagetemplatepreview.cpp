/************************************************************************
 **
 **  @file   ocoverpagetemplatepreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  Preview of a cover page — one white page, and the skeleton it is filled on.
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

#include "ocoverpagetemplatepreview.h"

#include "core/ocovergradedaxis.h"
#include "core/ocoverpagelayout.h"
#include "core/ocoverpagerender.h"
#include "core/ocoverpageresolver.h"
#include "core/ocoverpagesizes.h"
#include "core/ocoverseamallowances.h"
#include "core/ocoversymbols.h"
#include "core/oseamallowancevalues.h"

#include "vmisc/def.h"
#include "vmisc/vabstractapplication.h"
#include "vmisc/vabstractvalapplication.h"
#include "vmisc/vcommonsettings.h"
#include "vwidgets/ocartouchepainter.h"
#include "xml/vabstractpattern.h"

#include <QPainter>
#include <QPaintEvent>

namespace
{
/**
 * Mid grey. It contrasts with the white page in the light theme as well as in the dark one, which
 * a theme colour would not: the page is white in both.
 */
const QColor kBackground(0x80, 0x80, 0x80);

/** What the pane asks for, so that a preview left to size itself is not a sliver. */
constexpr int kPreferredWidth = 260;

/** The grey kept around the page, so that its edges are read against the background. */
constexpr qreal kMargin = 12.0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OCoverPageTemplatePreview::OCoverPageTemplatePreview(QWidget *parent) : QWidget(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::SetTemplate(const OCoverPageTemplate *tpl)
{
    m_template = tpl;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::SetTable(const OSizeTable *table)
{
    m_table = table;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::SetSymbols(const OSymbolLibrary::Library *symbols)
{
    m_symbols = symbols;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::SetSeamAllowances(const OSeamAllowanceLibrary::Library *seamAllowances)
{
    m_seamAllowances = seamAllowances;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::Refresh()
{
    FollowPattern();
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::FollowPattern()
{
    if (m_following != nullptr || VAbstractApplication::VApp() == nullptr)
    {
        return;
    }

    VAbstractPattern *doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        // **No pattern yet, and this is not a failure**: the editor is opened beside whatever is
        // open, and may be opened with nothing. The next Refresh tries again, so a pattern opened
        // afterwards is followed from that moment on.
        return;
    }

    // **The page follows the pattern, wherever it is changed from.** The Cover page dock writes
    // what a pattern contributes to its own cover page, and the name is written from the pattern's
    // toolbar — both while this window may be open beside them. Without this, a page would show
    // the pattern as it stood when the template was opened, and a field filled in the dock would
    // look as though it had done nothing.
    //
    // **The document is one object, emptied and refilled rather than replaced**, so a single
    // connection holds for as long as the application runs and there is nothing to undo here.
    m_following = doc;
    connect(doc, &VAbstractPattern::patternChanged, this, [this](bool) { update(); });
    connect(doc, &VAbstractPattern::FullUpdateFromFile, this, [this]() { update(); });
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplatePreview::minimumSizeHint() const -> QSize
{
    return {kPreferredWidth, kPreferredWidth};
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatePreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), kBackground);

    const qreal availableWidth = width() - 2 * kMargin;
    const qreal availableHeight = height() - 2 * kMargin;
    if (availableWidth <= 0 || availableHeight <= 0)
    {
        return;
    }

    // The page is shown whole or not at all, at the one ratio that says it is a page: the pane is
    // filled on the side that binds and the other follows. **Its size comes from the layout**, which
    // is where the export reads it too — a preview that decided the page for itself would be
    // showing a page nobody prints.
    const QSizeF sheet = OCoverPageLayout::PageSize();
    const qreal sheetRatio = sheet.height() / sheet.width();
    qreal pageWidth = availableWidth;
    qreal pageHeight = pageWidth * sheetRatio;
    if (pageHeight > availableHeight)
    {
        pageHeight = availableHeight;
        pageWidth = pageHeight / sheetRatio;
    }

    const QRectF page(rect().center().x() - pageWidth / 2.0, rect().center().y() - pageHeight / 2.0,
                      pageWidth, pageHeight);

    painter.fillRect(page, Qt::white);

    if (m_template == nullptr)
    {
        return;
    }

    // **The pattern the application has open**, read once for the whole page rather than once per
    // band: four bands ask the same four questions, and a document read between two of them would
    // not be the same document. There is nothing to read when none is open — a page looked at
    // before a pattern was chosen — and everything below then stays empty.
    const VAbstractPattern *pattern = nullptr;
    if (VAbstractApplication::VApp() != nullptr)
    {
        pattern = VAbstractValApplication::VApp()->getCurrentDocument();
    }
    const VCoverPageData coverPage = pattern == nullptr ? VCoverPageData() : pattern->GetCoverPage();

    // What one band reads, gathered here and nowhere else — the resolver is never given the pattern
    // nor a library, so everything a page reads from them is turned into values and handed over.
    // **The export canvas answers the same callable from the RUN it is drawing** (spec §6); this one
    // answers it from the model and the pattern, which is all a preview has.
    const auto DataOf = [this, pattern, &coverPage](const OCoverPageTemplate::Section &section)
        -> OCoverPageResolver::PatternData {
        OCoverPageResolver::PatternData data;

        // **The sizes are worked out here, not by the resolver.** Which sizes a range holds is a
        // question about the loaded table and the styles authored for it — the resolver draws what
        // it is handed and knows nothing of either.
        data.sizes = OCoverPageSizes(m_table, OCoverGradedAxisOptions::FromOptions(section.options),
                                     VAbstractApplication::VApp()->Settings());

        // The name the pattern carries, and the text a template left to be filled in for it —
        // the two things the pattern toolbar and the Cover page dock write. See VCoverPageData.
        data.name = pattern == nullptr ? QString() : pattern->GetPatternName();
        data.freeText = coverPage.freeText;

        // The legend's lines are worked out here for the same reason the sizes are: which symbols
        // a page shows is a question about a library and about which of them this pattern chose —
        // two things a page is not. See OCoverSymbols.
        data.symbols = m_symbols == nullptr
                           ? QVector<OCoverSymbols::Line>()
                           : OCoverSymbols::Lines(*m_symbols, m_template->languages,
                                                  coverPage.symbols);

        // The margins the pattern is cut with, worked out here for the same reason again: the
        // widths belong to the pattern and the names to the library, and a page holds neither.
        if (m_seamAllowances != nullptr)
        {
            const QVector<qreal> widths = OSeamAllowanceValues::Collect(pattern);
            if (!widths.isEmpty())
            {
                // **The default margin is the application's, read where it is set.** It is kept
                // in the unit the application works in, so it is brought to millimetres here —
                // the one unit every cover page states a length in.
                auto *settings = VAbstractApplication::VApp()->Settings();
                const qreal defaultMm =
                    UnitConvertor(settings->GetDefaultSeamAllowance(),
                                  StrToUnits(settings->GetUnit()), Unit::Mm);
                data.seamAllowances = OCoverSeamAllowances::Lines(
                    *m_seamAllowances, m_template->languages, widths, defaultMm,
                    coverPage.marginNames);
            }
        }

        return data;
    };

    // **substituteEmpty true, and that is this widget's whole privilege**: standing an information
    // in as its own name is what lets the user see it is there and waiting. The export canvas draws
    // the page the paper will carry and passes false — see the nomenclature spec §6.
    const auto bands = OCoverPageRender::Bands(*m_template, DataOf, OCartouchePainter::BaseFont(),
                                               m_table, /*substituteEmpty*/ true);

    // The page is laid out at its real size, then the whole of it is brought down to what the pane
    // can show: the display scale is applied to what was placed, never laid out into. So what is
    // placed here is what the export draws, at the size it will be printed.
    OCoverPageRender::Paint(painter, page, bands, page.width() / sheet.width());
}
