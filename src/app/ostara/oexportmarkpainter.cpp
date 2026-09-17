/************************************************************************
 **
 **  @file   oexportmarkpainter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  Draws the assembly marks of a template — frame, scissors, marks and pagination.
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

#include "oexportmarkpainter.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QSvgRenderer>
#include <QtMath>

namespace
{
/** Points to millimetres, the conversion every length of the decor goes through. */
constexpr qreal kMmPerPt = 25.4 / 72.0;

/**
 * Pixels to millimetres, for the one length of the decor given in pixels rather than in points
 * or in millimetres. A pixel is an absolute length like the others — 96 of them make an inch —
 * so the page number keeps its size from one format to the next, as the frame and the marks do.
 */
constexpr qreal kMmPerPx = 25.4 / 96.0;

/**
 * The scissors are drawn at the size their files declare — 7.468 mm across, 13.333 mm along.
 * They are not scaled to the sheet: a cut mark is the same mark on a small artboard and on a
 * large one (§3.5). The two numbers are the sides the two files declare — the long one along
 * the line, the short one across it — and they are read off the files, not rounded to
 * something prettier.
 */
constexpr qreal kScissorsAcrossMm = 7.468;
constexpr qreal kScissorsAlongMm = 13.333;

/**
 * How far along its own edge a scissors is set back from the corner: a quarter of that edge's
 * length. Not on the corner itself — the pagination and the remaining marks will come there.
 */
constexpr qreal kScissorsOffsetRatio = 0.25;

/**
 * The stand-in outline drawn where the decor is not: one tenth of black, present enough to tell
 * one sheet from the next and too faint to be mistaken for something the template draws.
 */
constexpr float kSheetGuideOpacity = 0.10F;

/**
 * The number of one sheet, from its place in the tiling — and the only thing of the decor that
 * is read off the sheet rather than off the plan.
 *
 * Reading order for the number alone: left to right, then top to bottom, so a three-column plan
 * reads 1 2 3 across its first row and 4 5 6 across its second. In the two-axis form the letter
 * is the column and the number is the row, so that same plan reads A1 B1 C1 and A2 B2 C2 — the
 * two axes, as the mark table is read.
 *
 * The count of columns comes in as an argument and is not a constant of this file: the preview
 * draws three of them, the canvas as many as the run was nested on, and the number alone is the
 * one thing that reads the difference.
 */
QString SheetNumberText(int columns, OExportPaginationForm form, int row, int col)
{
    if (form == OExportPaginationForm::NumberAndLetter)
    {
        // The letter is written from the column index, as the mark table's columns are read:
        // 'A' is the first column, 'B' the second. Past twenty-six columns there is no letter
        // left — which is why letters alone are not offered as a form, and why a run is capped
        // at that count where its columns are entered (kPaginationMaxColumns). Reaching here
        // with more of them is not something the interface can ask for — see §3.4.3.
        const char letter = static_cast<char>('A' + col);
        return QLatin1Char(letter) + QString::number(row + 1);
    }

    return QString::number(row * columns + col + 1);
}
} // namespace

OExportMarkPainter::OExportMarkPainter()
{
    m_scissorsVertical = std::make_unique<QSvgRenderer>(
        QStringLiteral(":/export/icon_scissors_vertical.svg"));
    m_scissorsHorizontal = std::make_unique<QSvgRenderer>(
        QStringLiteral(":/export/icon_scissors_horizontal.svg"));

    for (const OExportMarkShapeModel &model : OExportMarkShapes())
    {
        m_shapeRenderers.push_back(std::make_unique<QSvgRenderer>(
            QStringLiteral(":/export/%1").arg(QLatin1String(model.icon))));
    }
}

//---------------------------------------------------------------------------------------------------------------------

OExportMarkPainter::~OExportMarkPainter() = default;

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::SetFrameSettings(const OExportFrameSettings &frame)
{
    m_frame = frame;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::SetMarkSettings(const OExportMarkSettings &marks)
{
    m_marks = marks;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::SetPaginationSettings(const OExportPaginationSettings &pagination)
{
    m_pagination = pagination;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::Paint(QPainter &painter, const QPointF &origin,
                               const OExportMarkGrid &grid, bool framed, bool sheetGuide) const
{
    if (!grid.IsValid())
    {
        return;
    }

    // The frame is drawn as the boundaries themselves, once each — see DrawFrame. The scissors
    // follow it, sheet by sheet: they are cut marks, and there is one pair per sheet.
    if (framed)
    {
        DrawFrame(painter, origin, grid);

        for (int row = 0; row < grid.rows; ++row)
        {
            for (int col = 0; col < grid.columns; ++col)
            {
                DrawScissors(painter,
                             QRectF(origin.x() + col * grid.sheetWidthPx,
                                    origin.y() + row * grid.sheetHeightPx, grid.sheetWidthPx,
                                    grid.sheetHeightPx),
                             grid);
            }
        }
    }
    else if (sheetGuide)
    {
        // The decor belongs to « Tiled Framed » alone — on every other tiled model the frame is
        // ignored, and there is no cut to mark inside the sheet. The sheets still have to be told
        // apart on screen, so a faint outline stands in for it.
        //
        // Never on the framed model: there the decor is the user's, and a second line under it
        // would fight the opacity they set — which is exactly what made the opacity look dead.
        QColor guide(Qt::black);
        guide.setAlphaF(kSheetGuideOpacity);
        painter.setPen(QPen(guide, 1.0));
        painter.setBrush(Qt::NoBrush);
        DrawGridLines(painter, origin, grid);
    }

    // Drawn whatever the format, as the marks below are: a sheet of A4 is glued to its neighbours
    // exactly as a sheet of « Tiled Framed » is, and has just as much need to say which one it is.
    DrawCentreNumber(painter, origin, grid);
    DrawCornerNumbers(painter, origin, grid);
    DrawEdgeNumbers(painter, origin, grid, true);
    DrawEdgeNumbers(painter, origin, grid, false);

    // Drawn whatever the format. The frame is the one thing that has nowhere to go on a model
    // whose artboard is the paper's own edge; a registration mark, itself, is just as useful
    // there — the sheets are glued the same way.
    DrawMarks(painter, origin, grid);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawFrame(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid) const
{
    const qreal penWidthPx = m_frame.widthPt * kMmPerPt * grid.pxPerMm;

    QColor line(Qt::black);
    line.setAlphaF(static_cast<float>(m_frame.frameOpacity) / 100.0F);

    QPen pen(line, penWidthPx);
    pen.setJoinStyle(Qt::MiterJoin);
    if (m_frame.dashed && !m_frame.dashes.isEmpty())
    {
        // The size styles' conversion, to the letter (ogradedsizetrace.cpp:98-108): a length in
        // pt becomes px through `PrintDPI / 72`, and a Qt pattern counts in multiples of the pen
        // width, so it is divided by that width. The cap goes flat so that the dashes end where
        // they say they do — Qt's default stretches each of them by half a width, and the SVG
        // export, which caps at butt, would not agree.
        const qreal pxPerPt = kMmPerPt * grid.pxPerMm;
        const qreal width = penWidthPx > 0.0 ? penWidthPx : 1.0;
        QVector<qreal> pattern;
        pattern.reserve(m_frame.dashes.size());
        for (qreal value : m_frame.dashes)
        {
            pattern.append(value * pxPerPt / width);
        }
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern(pattern);
        pen.setCapStyle(Qt::FlatCap);
    }
    else
    {
        pen.setStyle(Qt::SolidLine);
        pen.setCapStyle(Qt::SquareCap);
    }
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // The frame is drawn as the grid's own lines, not as six rectangles.
    //
    // Two defects came out of drawing a rectangle per sheet. A rectangle inset by half a line
    // width put sheet A's right edge half a width on one side of the boundary and sheet B's left
    // edge half a width on the other, so where one line belongs the eye saw two, side by side.
    // And a rectangle starts its dash pattern at its own top-left corner and runs clockwise, so
    // A's right edge began at the top going down while B's left edge began at the bottom going
    // up: the same seam, two different phases, and the dashes never met.
    //
    // A boundary between two sheets belongs to both. Drawn once, centred on the boundary, there
    // is nothing left to disagree about. Every vertical line runs downwards from the top of the
    // grid and every horizontal one rightwards from its left edge, so the whole plan shares one
    // anchor and the pattern carries across every seam.
    //
    // The export must keep that rule: on paper each sheet draws its four borders in its own
    // page, so each border is drawn from the plan's anchor — and in the same direction — rather
    // than from the sheet's own corner.
    DrawGridLines(painter, origin, grid);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawMarks(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid) const
{
    const int index = static_cast<int>(m_marks.shape);
    if (index < 0 || static_cast<std::size_t>(index) >= m_shapeRenderers.size())
    {
        return;
    }

    QSvgRenderer *shape = m_shapeRenderers.at(static_cast<std::size_t>(index)).get();
    const qreal side = m_marks.sizeMm * grid.pxPerMm;
    if (shape == nullptr || !shape->isValid() || side <= 0.0)
    {
        return;
    }

    // The anchors, in plan coordinates — never in sheet ones. A mark's centre lies on the
    // boundary: at a corner two boundaries cross, so the mark belongs a quarter to each of the
    // four sheets that meet there; at the middle of an edge one boundary crosses it, and the
    // two halves go to the two sheets either side.
    //
    // Every seam of the tiling gets one, not one per row of sheets: each pair of neighbours has
    // its own place to register.
    QVector<QPointF> anchors;

    if (m_marks.onCorner)
    {
        // Every corner of the tiling where sheets meet — four of them round an inside corner,
        // two along the plan's own edge, where the mark is cut down to a half.
        //
        // The four corners of the plan itself are passed over: a sheet alone in a corner has no
        // neighbour to register with, and nothing of the mark would land on paper.
        for (int col = 0; col <= grid.columns; ++col)
        {
            for (int row = 0; row <= grid.rows; ++row)
            {
                if ((col == 0 || col == grid.columns) && (row == 0 || row == grid.rows))
                {
                    continue;
                }

                anchors.append(
                    QPointF(origin.x() + col * grid.sheetWidthPx, origin.y() + row * grid.sheetHeightPx));
            }
        }
    }

    if (m_marks.onVerticalCentre)
    {
        for (int col = 1; col < grid.columns; ++col)
        {
            for (int row = 0; row < grid.rows; ++row)
            {
                anchors.append(QPointF(origin.x() + col * grid.sheetWidthPx,
                                       origin.y() + (row + 0.5) * grid.sheetHeightPx));
            }
        }
    }

    if (m_marks.onHorizontalCentre)
    {
        for (int row = 1; row < grid.rows; ++row)
        {
            for (int col = 0; col < grid.columns; ++col)
            {
                anchors.append(QPointF(origin.x() + (col + 0.5) * grid.sheetWidthPx,
                                       origin.y() + row * grid.sheetHeightPx));
            }
        }
    }

    // What falls outside the plan falls into nothing. There is no sheet out there to carry it,
    // so it is not drawn here — and the export writes nothing of it either, since a page ends at
    // its own edge. Only the marks set on the plan's outer edge are cut down by this; everywhere
    // else the whole mark lands on a sheet or on two.
    const QRectF planRect(origin.x(), origin.y(), grid.sheetWidthPx * grid.columns, grid.sheetHeightPx * grid.rows);

    painter.save();
    painter.setClipRect(planRect);
    painter.setOpacity(m_marks.opacity / 100.0);
    for (const QPointF &anchor : anchors)
    {
        shape->render(&painter,
                      QRectF(anchor.x() - side / 2.0, anchor.y() - side / 2.0, side, side));
    }
    painter.restore();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawGridLines(QPainter &painter, const QPointF &origin,
                                       const OExportMarkGrid &grid) const
{
    const qreal gridWidth = grid.sheetWidthPx * grid.columns;
    const qreal gridHeight = grid.sheetHeightPx * grid.rows;

    for (int col = 0; col <= grid.columns; ++col)
    {
        const qreal x = origin.x() + col * grid.sheetWidthPx;
        painter.drawLine(QPointF(x, origin.y()), QPointF(x, origin.y() + gridHeight));
    }

    for (int row = 0; row <= grid.rows; ++row)
    {
        const qreal y = origin.y() + row * grid.sheetHeightPx;
        painter.drawLine(QPointF(origin.x(), y), QPointF(origin.x() + gridWidth, y));
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawScissors(QPainter &painter, const QRectF &sheet, const OExportMarkGrid &grid) const
{
    if (!m_frame.scissorsHorizontal && !m_frame.scissorsVertical)
    {
        return;
    }

    painter.setOpacity(m_frame.scissorsOpacity / 100.0);

    // Each scissors lies along the line it marks and lies FLAT against it — the line is tangent
    // to the icon, not through it — with the whole of the drawing inside the sheet. That is the
    // rule the tile factory follows, and it is the only one that survives the page: a mark half
    // outside the artboard is half outside the page, and the PDF would cut it away.
    //
    // Neither sits on the corner itself: the pagination and the other marks will want that room.
    // Each is set back along its own edge by a quarter of that edge's length — for the default
    // « Tiled Framed », 19/4 = 4.75 cm on the top edge, 25.9/4 = 6.475 cm on the right one.
    const qreal across = kScissorsAcrossMm * grid.pxPerMm;
    const qreal along = kScissorsAlongMm * grid.pxPerMm;

    if (m_frame.scissorsHorizontal && m_scissorsHorizontal->isValid())
    {
        // Top-right corner: its flat upper edge on the sheet's top line, the body hanging down
        // into the sheet. No mirroring — the file is drawn for exactly this.
        const qreal offset = sheet.width() * kScissorsOffsetRatio;
        const QRectF rect(sheet.right() - offset - along, sheet.top(), along, across);
        m_scissorsHorizontal->render(&painter, rect);
    }

    if (m_frame.scissorsVertical && m_scissorsVertical->isValid())
    {
        // Bottom-right corner: its lower end set back along the right edge, flat against it.
        // No mirroring — the file is drawn for exactly this.
        const qreal offset = sheet.height() * kScissorsOffsetRatio;
        const QRectF rect(sheet.right() - across, sheet.bottom() - offset - along, across, along);
        m_scissorsVertical->render(&painter, rect);
    }

    painter.setOpacity(1.0);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::ApplyTextStyle(QPainter &painter,
                                         const OExportPaginationTextStyle &style, const OExportMarkGrid &grid) const
{
    // The absolute-size rule, applied to the one length of the decor given in pixels. The frame
    // converts its points to millimetres and then to the pane's scale; a pixel does the same, 96
    // of them to the inch. What the field says is therefore what the paper gets, on a small
    // artboard as on a large one.
    QFont font = painter.font();
    if (!style.fontFamily.isEmpty())
    {
        font.setFamily(style.fontFamily);
    }
    font.setWeight(static_cast<QFont::Weight>(style.weight));
    font.setPixelSize(qMax(1, qRound(style.sizePx * kMmPerPx * grid.pxPerMm)));

    QColor ink(Qt::black);
    ink.setAlphaF(static_cast<float>(style.opacity) / 100.0F);

    painter.setFont(font);
    painter.setPen(ink);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawCentreNumber(QPainter &painter, const QPointF &origin,
                                          const OExportMarkGrid &grid) const
{
    if (!m_pagination.centre.enabled)
    {
        return;
    }

    painter.save();
    ApplyTextStyle(painter, m_pagination.centre.style, grid);

    // A whole number per sheet, and each sheet its own: the number is what tells one printed
    // sheet from another once they are on the table, so it cannot be drawn once for the plan the
    // way the frame and the marks are. Nothing is clipped here — the centre of a sheet is the one
    // place of the decor that can never fall outside it.
    for (int row = 0; row < grid.rows; ++row)
    {
        for (int col = 0; col < grid.columns; ++col)
        {
            const QRectF sheet(origin.x() + col * grid.sheetWidthPx, origin.y() + row * grid.sheetHeightPx,
                               grid.sheetWidthPx, grid.sheetHeightPx);
            painter.drawText(sheet, Qt::AlignCenter, SheetNumberText(grid.columns, m_pagination.form, row, col));
        }
    }

    painter.restore();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawCornerNumbers(QPainter &painter, const QPointF &origin,
                                           const OExportMarkGrid &grid) const
{
    if (!m_pagination.corner.enabled)
    {
        return;
    }

    const qreal inset = kPaginationCornerInsetMm * grid.pxPerMm;

    painter.save();
    ApplyTextStyle(painter, m_pagination.corner.style, grid);
    const QFontMetricsF metrics(painter.font());

    for (int row = 0; row < grid.rows; ++row)
    {
        for (int col = 0; col < grid.columns; ++col)
        {
            const QRectF sheet(origin.x() + col * grid.sheetWidthPx, origin.y() + row * grid.sheetHeightPx,
                               grid.sheetWidthPx, grid.sheetHeightPx);

            // The sheet's four corners, told apart by which side of the sheet each lies on.
            for (int vertical = 0; vertical < 2; ++vertical)   // 0 = the top corner, 1 = the bottom
            {
                for (int horizontal = 0; horizontal < 2; ++horizontal) // 0 = left, 1 = right
                {
                    const bool right = horizontal == 1;
                    const bool bottom = vertical == 1;

                    const int anchorCol = col + (right ? 1 : 0);
                    const int anchorRow = row + (bottom ? 1 : 0);

                    // A corner of the plan is no junction: the sheet there meets nobody, and the
                    // two other sheets of its four are not sheets at all.
                    if ((anchorCol == 0 || anchorCol == grid.columns)
                        && (anchorRow == 0 || anchorRow == grid.rows))
                    {
                        continue;
                    }

                    // The junction takes the number of the sheet above and to its left — among
                    // the sheets present, the first in reading order. On the plan's top edge there
                    // is no sheet above, and on its left edge none to the left, so there the sheet
                    // itself is that one.
                    const int numberRow = anchorRow == 0 ? 0 : anchorRow - 1;
                    const int numberCol = anchorCol == 0 ? 0 : anchorCol - 1;
                    const QString label = SheetNumberText(grid.columns, m_pagination.form, numberRow, numberCol);

                    // Placed by its painted box and not by its baseline: what has to sit five
                    // millimetres from the edge is the ink, not the line the ink hangs from.
                    const QRectF box = metrics.boundingRect(label);
                    const qreal x =
                        right ? sheet.right() - inset - box.width() : sheet.left() + inset;
                    const qreal y =
                        bottom ? sheet.bottom() - inset - box.height() : sheet.top() + inset;

                    painter.drawText(QPointF(x - box.left(), y - box.top()), label);
                }
            }
        }
    }

    painter.restore();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportMarkPainter::DrawEdgeNumbers(QPainter &painter, const QPointF &origin,
                                          const OExportMarkGrid &grid, bool vertical) const
{
    const OExportPaginationEdgeNumber &placement =
        vertical ? m_pagination.verticalCentre : m_pagination.horizontalCentre;
    if (!placement.enabled)
    {
        return;
    }

    // A vertical seam runs down the plan and is told apart by its column; a horizontal one runs
    // across it and is told apart by its row. Which seams are walked, which sheets meet on them
    // and which way « across » points all follow from that one word.
    const int seamCount = vertical ? grid.columns : grid.rows;
    const int laneCount = vertical ? grid.rows : grid.columns;

    painter.save();
    ApplyTextStyle(painter, placement.style, grid);
    const QFontMetricsF metrics(painter.font());
    const qreal margin = kPaginationCornerInsetMm * grid.pxPerMm;

    // The ends of the tiling are passed over, as the marks pass them over: a seam lying on the
    // plan's own edge has one sheet and not two, so there is nobody there to be matched with.
    for (int seam = 1; seam < seamCount; ++seam)
    {
        for (int lane = 0; lane < laneCount; ++lane)
        {
            // The number of the sheet above and to the left of the junction — the corners' rule,
            // read on a seam. On a seam that is always the sheet on the first side, which keeps
            // the two readings of the plan in agreement where a corner and a seam meet.
            const QString label = vertical ? SheetNumberText(grid.columns, m_pagination.form, lane, seam - 1)
                                           : SheetNumberText(grid.columns, m_pagination.form, seam - 1, lane);
            const QRectF box = metrics.boundingRect(label);

            // How far from the seam the number sits, measured across it. A quarter of the mark's
            // side is the centre of the half of the mark that belongs to the sheet, which is the
            // idea: the number is written inside the mark rather than beside it.
            //
            // The two rails then bound it — the ink no closer to the seam than the printer's
            // margin, and never past the middle of the sheet. Between them the mark decides, and
            // a mark small enough or large enough simply hands the decision to a rail instead.
            const qreal across = vertical ? box.width() : box.height();
            const qreal halfSheet = (vertical ? grid.sheetWidthPx : grid.sheetHeightPx) / 2.0;
            const qreal wanted = m_marks.sizeMm * grid.pxPerMm / 4.0;
            const qreal railNear = margin + across / 2.0;
            const qreal railFar = halfSheet - across / 2.0;
            const qreal offset =
                railFar > railNear ? qBound(railNear, wanted, railFar) : railNear;

            for (int side = 0; side < 2; ++side)
            {
                const bool second = side == 1;

                const int sheetCol = vertical ? (second ? seam : seam - 1) : lane;
                const int sheetRow = vertical ? lane : (second ? seam : seam - 1);
                const QRectF sheet(origin.x() + sheetCol * grid.sheetWidthPx,
                                   origin.y() + sheetRow * grid.sheetHeightPx, grid.sheetWidthPx, grid.sheetHeightPx);

                // The seam's own line, and the middle of the sheet's edge along it. Only the
                // crossing distance is ever in question — the middle of an edge is as far from
                // the borders as a point of the decor gets.
                QPointF centre = vertical ? QPointF(second ? sheet.left() : sheet.right(),
                                                    sheet.center().y())
                                          : QPointF(sheet.center().x(),
                                                    second ? sheet.top() : sheet.bottom());
                if (vertical)
                {
                    centre.rx() += second ? offset : -offset;
                }
                else
                {
                    centre.ry() += second ? offset : -offset;
                }

                // Placed by its painted box, so that what is centred in the mark is the ink and
                // not the line the ink hangs from.
                painter.drawText(QPointF(centre.x() - box.width() / 2.0 - box.left(),
                                         centre.y() - box.height() / 2.0 - box.top()),
                                 label);
            }
        }
    }

    painter.restore();
}
