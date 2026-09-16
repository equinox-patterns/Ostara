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
#include <QPainter>
#include <QPaintEvent>
#include <QPointF>
#include <QRectF>
#include <QVector>

namespace
{
/**
 * Mid grey. It contrasts with the white sheet in the light theme as well as in the
 * dark one, which a theme colour would not: the sheet is white in both.
 */
const QColor kBackground(0x80, 0x80, 0x80);

/** Hairline, so the sheet keeps an edge whatever the background is set to later. */
const QColor kSheetEdge(0x40, 0x40, 0x40);

constexpr int kMargin = 12;

/** Three columns, two rows — the shape of a tiled plan that shows both axes. */
constexpr int kColumns = 3;
constexpr int kRows = 2;
} // namespace

OExportTemplatePreview::OExportTemplatePreview(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(200, 200);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatePreview::SetSheetRatio(qreal ratio)
{
    if (ratio <= 0.0 || qFuzzyCompare(ratio, m_sheetRatio))
    {
        return;
    }

    m_sheetRatio = ratio;
    update();
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

    // The sheets tile edge to edge, the way the plan glues its artboards: no gap, and the
    // grid is centred as a whole. Their size is the largest that fits three columns and two
    // rows at once, proportions kept — so a sheet is never left floating inside a wider cell,
    // which is what opened a hole between the columns.
    const qreal sheetWidth = qMin(available.width() / kColumns,
                                  available.height() / kRows * m_sheetRatio);
    if (sheetWidth <= 0.0)
    {
        return;
    }
    const qreal sheetHeight = sheetWidth / m_sheetRatio;

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

    // Every fill first, then every outline. Outlining a sheet before filling its neighbour
    // would have that fill cover the shared edge, and the two internal verticals would go.
    for (const QRectF &sheet : sheets)
    {
        painter.fillRect(sheet, Qt::white);
    }

    painter.setPen(kSheetEdge);
    for (const QRectF &sheet : sheets)
    {
        painter.drawRect(sheet);
    }
}
