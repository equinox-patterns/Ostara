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
#include "core/ocoverpagecatalogue.h"
#include "core/ocoverpagelayout.h"
#include "core/ocoverpageresolver.h"
#include "core/ocoverpagesizes.h"

#include "vmisc/vabstractapplication.h"
#include "vwidgets/ocartouchepainter.h"

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

void OCoverPageTemplatePreview::Refresh()
{
    update();
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

    // What one band has to draw, asked of the resolver — the one place that knows how an
    // information is drawn. It is given no pattern: the editor draws a template on its own, so
    // everything that reads the pattern resolves to nothing.
    //
    // **The sizes are worked out here, not by the resolver.** Which sizes a range holds is a
    // question about the loaded table and the styles authored for it — the resolver draws what it
    // is handed and knows nothing of either, and a pattern being printed will answer the same
    // question from the range its output was run with.
    const auto LinesOf = [this](OCoverPageTemplate::Placement placement) {
        QVector<OCartoucheRenderLine> lines;
        for (const OCoverPageTemplate::Section &section : m_template->Sections(placement))
        {
            OCoverPageResolver::PatternData data;
            data.sizes = OCoverPageSizes(m_table, OCoverGradedAxisOptions::FromOptions(section.options),
                                         VAbstractApplication::VApp()->Settings());

            QVector<OCartoucheRenderLine> drawn = OCoverPageResolver::Resolve(
                section, data, OCartouchePainter::BaseFont(), m_template->languages, m_table);
            if (drawn.isEmpty())
            {
                // Nothing to draw — a logo nobody chose, a text left to be filled in, a name the
                // pattern has not given. The page itself draws nothing there; here the information
                // stands for itself as its own name, as the Information block preview shows its
                // sections. What is being judged is the arrangement, and an information showing
                // nothing would look like one that is not there at all.
                const OTemplateSection &type = OCoverPageCatalogue::Section(section.typeId);
                const QFont font =
                    OCartouchePainter::SectionFont(section.options, OCartouchePainter::BaseFont());
                const int lineSpacing =
                    section.options.value(QStringLiteral("lineSpacing"), 0).toInt();
                drawn.append(OCartouchePainter::TextLine(
                    type.IsValid() ? type.DisplayName() : section.typeId, font, lineSpacing));
            }
            lines.append(drawn);
        }
        return lines;
    };

    const QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> bandLines = {
        {OCoverPageTemplate::Placement::Top, LinesOf(OCoverPageTemplate::Placement::Top)},
        {OCoverPageTemplate::Placement::MiddleLeft,
         LinesOf(OCoverPageTemplate::Placement::MiddleLeft)},
        {OCoverPageTemplate::Placement::MiddleRight,
         LinesOf(OCoverPageTemplate::Placement::MiddleRight)},
        {OCoverPageTemplate::Placement::Bottom, LinesOf(OCoverPageTemplate::Placement::Bottom)},
    };

    // The page is laid out at its real size, then the whole of it is brought down to what the pane
    // can show: the display scale is applied to what was placed, never laid out into. So what is
    // placed here is what the export draws, at the size it will be printed.
    const qreal contentScale = page.width() / sheet.width();
    const auto ToScreen = [&page, contentScale](const QRectF &cell) -> QRectF {
        return {page.left() + cell.left() * contentScale, page.top() + cell.top() * contentScale,
                cell.width() * contentScale, cell.height() * contentScale};
    };

    // The page's own ink for its words. **The lines carry no colour of their own — except the trace
    // samples**, which are drawn in the colour of the size they stand for: that colour is the whole
    // point of the sample, and it is borrowed and given back by the painter around each one.
    painter.setPen(QColor());
    const QMap<OCoverPageTemplate::Placement, QRectF> cells = OCoverPageLayout::Place(bandLines);
    for (auto cell = cells.constBegin(); cell != cells.constEnd(); ++cell)
    {
        const QVector<OCartoucheRenderLine> lines = bandLines.value(cell.key());
        if (!lines.isEmpty())
        {
            // The fonts are brought to the pane's scale before the lines are laid out: the whole
            // page is shown at a fraction of its size, so everything on it is drawn at that
            // fraction — the words as much as a logo, and as much as the geometry of a test
            // square. Painting the text at its print size inside a reduced page would make it as
            // much too large as the page is reduced, and a five centimetre square would be drawn
            // around a label that no longer fits in it.
            const QVector<OCartoucheRenderLine> shown =
                OCartouchePainter::ScaledLines(lines, contentScale);
            OCartouchePainter::Paint(painter, ToScreen(cell.value()), shown, contentScale);
        }
    }
}
