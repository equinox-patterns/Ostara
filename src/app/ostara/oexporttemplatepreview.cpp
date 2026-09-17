/************************************************************************
 **
 **  @file   oexporttemplatepreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Preview of a tiled plan — six white artboards on a contrasting background.
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

#include "oexporttemplatepreview.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPaintEvent>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QSvgRenderer>
#include <QVector>
#include <QtMath>

namespace
{
/**
 * Mid grey. It contrasts with the white sheet in the light theme as well as in the
 * dark one, which a theme colour would not: the sheet is white in both.
 */
const QColor kBackground(0x80, 0x80, 0x80);

constexpr int kMargin = 12;

/** Three columns, two rows — the shape of a tiled plan that shows both axes. */
constexpr int kColumns = 3;
constexpr int kRows = 2;

/** What the pane asks for at 1:1, when it is left to size itself. */
constexpr int kPreferredSide = 200;

} // namespace

// The decor itself — the frame, the scissors, the marks and the pagination — is drawn by
// OExportMarkPainter, which the Export canvas uses as well. This widget decides only what a
// preview adds on top of it: how big the sheets are drawn, and what stands in for a frame that
// the format does not carry.
OExportTemplatePreview::OExportTemplatePreview(QWidget *parent) : QWidget(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetSheetSizeMm(const QSizeF &sizeMm)
{
    if (sizeMm.width() <= 0.0 || sizeMm.height() <= 0.0 || sizeMm == m_sheetSizeMm)
    {
        return;
    }

    m_sheetSizeMm = sizeMm;

    // The grid the widget asks to be is measured in the sheet, so a new sheet is a new size.
    updateGeometry();
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetFrameSettings(const OExportFrameSettings &frame)
{
    m_decor.SetFrameSettings(frame);
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetMarkSettings(const OExportMarkSettings &marks)
{
    m_decor.SetMarkSettings(marks);
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetPaginationSettings(const OExportPaginationSettings &pagination)
{
    m_decor.SetPaginationSettings(pagination);
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetSheetFramed(bool framed)
{
    if (framed == m_sheetFramed)
    {
        return;
    }

    m_sheetFramed = framed;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetZoomFactor(qreal factor)
{
    if (factor < 1.0)
    {
        factor = 1.0;
    }
    if (qFuzzyCompare(factor, m_zoom))
    {
        return;
    }

    m_zoom = factor;

    // The size the widget asks for changes with the zoom, and with it whether the pane around
    // it has anything to scroll.
    updateGeometry();
    update();
}

//---------------------------------------------------------------------------------------------------------------------

QSize OExportTemplatePreview::GridSizePx() const
{
    const bool frozen = m_zoom > 1.0 && m_frozenPxPerMm > 0.0;
    const qreal pxPerMm = frozen ? m_frozenPxPerMm * m_zoom : m_frozenPxPerMm;
    if (pxPerMm <= 0.0)
    {
        return {};
    }

    return QSize(qCeil(m_sheetSizeMm.width() * kColumns * pxPerMm) + 2 * kMargin,
                 qCeil(m_sheetSizeMm.height() * kRows * pxPerMm) + 2 * kMargin);
}

//---------------------------------------------------------------------------------------------------------------------

QSize OExportTemplatePreview::minimumSizeHint() const
{
    // At 1:1 there is nothing to scroll: the grid is fitted to whatever room it is given, so
    // the widget asks only for a workable minimum. Above it the grid has a real size, and the
    // scroll area needs to be told — this is how it learns it.
    if (m_zoom <= 1.0)
    {
        return QSize(kPreferredSide, kPreferredSide);
    }

    const QSize grid = GridSizePx();
    return grid.isEmpty() ? QSize(kPreferredSide, kPreferredSide) : grid;
}

//---------------------------------------------------------------------------------------------------------------------

QSize OExportTemplatePreview::sizeHint() const
{
    return minimumSizeHint();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), kBackground);

    const QRectF available = QRectF(rect()).adjusted(kMargin, kMargin, -kMargin, -kMargin);
    if (available.width() <= 0.0 || available.height() <= 0.0)
    {
        return;
    }

    const qreal ratio = m_sheetSizeMm.width() / m_sheetSizeMm.height();

    qreal pxPerMm = 0.0;
    if (m_zoom > 1.0 && m_frozenPxPerMm > 0.0)
    {
        // Zoomed: the scale was settled before the widget grew, and it is kept. Re-fitting it
        // here would measure the zoomed grid and divide the zoom straight back out of itself.
        pxPerMm = m_frozenPxPerMm * m_zoom;
    }
    else
    {
        // The sheets tile edge to edge, the way the plan glues its artboards: no gap, and the
        // grid is centred as a whole. Their size is the largest that fits three columns and two
        // rows at once, proportions kept — so a sheet is never left floating inside a wider
        // cell, which is what opened a hole between the columns.
        const qreal sheetWidth =
            qMin(available.width() / kColumns, available.height() / kRows * ratio);
        if (sheetWidth <= 0.0)
        {
            return;
        }

        pxPerMm = sheetWidth / m_sheetSizeMm.width();
        m_frozenPxPerMm = pxPerMm;
    }

    // One scale for the whole pane, and it is the only one: the sheet is drawn with it, and so
    // is the frame, and the marks, and the pagination. Nothing that the template draws may be
    // sized as a fraction of the sheet — a mark is the same size on a 15 cm artboard and on a
    // 50 cm one. At this scale that mark therefore covers less of the big sheet than of the
    // small one, which is not a defect of the preview: it is the truth it is there to show.
    const qreal sheetWidth = m_sheetSizeMm.width() * pxPerMm;
    const qreal sheetHeight = m_sheetSizeMm.height() * pxPerMm;

    const qreal gridWidth = sheetWidth * kColumns;
    const qreal gridHeight = sheetHeight * kRows;
    const QPointF origin(available.center().x() - gridWidth / 2.0,
                         available.center().y() - gridHeight / 2.0);

    QVector<QRectF> sheets;
    sheets.reserve(kColumns * kRows);
    for (int row = 0; row < kRows; ++row)
    {
        for (int col = 0; col < kColumns; ++col)
        {
            sheets.append(QRectF(origin.x() + col * sheetWidth,
                                 origin.y() + row * sheetHeight,
                                 sheetWidth,
                                 sheetHeight));
        }
    }

    for (const QRectF &sheet : sheets)
    {
        painter.fillRect(sheet, Qt::white);
    }

    // The sheets are filled and nothing else. An outline used to be drawn around each of them,
    // to tell one from the next — and it made the preview lie twice: the export draws no such
    // line, and a hairline of its own sat exactly where the template's frame goes, so lowering
    // the frame's opacity changed nothing on screen. What separates the sheets here is what
    // separates them on paper: the frame the template draws, and nothing else. At zero opacity
    // there is no line left, which is the truth of the setting, not a defect of the preview.
    // The decor is drawn by the one renderer the Export canvas draws with too, so that what is
    // previewed here and what the plan shows there cannot drift apart. The grid is the only thing
    // this pane brings of its own: three columns and two rows, the shape that shows both axes.
    OExportMarkGrid grid;
    grid.columns = kColumns;
    grid.rows = kRows;
    grid.sheetWidthPx = sheetWidth;
    grid.sheetHeightPx = sheetHeight;
    grid.pxPerMm = pxPerMm;

    // The faint stand-in outline is asked for here, and nowhere else. This pane has no cut lines
    // of its own: without it, six white rectangles would read as one white block. The canvas has
    // them, so it asks for none — and both stand them down where the frame is drawn, for the same
    // reason: a second line under the user's own would fight the opacity they set.
    m_decor.Paint(painter, origin, grid, m_sheetFramed, true);
}

//---------------------------------------------------------------------------------------------------------------------

