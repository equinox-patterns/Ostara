/************************************************************************
 **
 **  @file   osymbolrender.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  A mark drawn on its own — the one place a symbol becomes ink.
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

#ifndef OSYMBOLRENDER_H
#define OSYMBOLRENDER_H

#include <QColor>
#include <QPainterPath>
#include <QRectF>
#include <QSizeF>
#include <QVector>

#include "osymbolshapes.h"
#include "vwidgets/ocartouchepainter.h"

class QPainter;

/**
 * @brief Drawing a mark outside a piece — the geometry every symbol is shown with.
 *
 * **A mark is not redrawn here, it is asked for.** Every shape comes from the very code the canvas
 * draws with: a notch from @ref VPassmark, a grain line from @ref VPieceGrainline, a fold line from
 * @ref VFoldLine, a place label from @ref VAbstractPiece. A symbol and the mark it stands for are
 * therefore the same drawing, and can never drift apart — which is the whole point of a legend.
 *
 * **What a mark has no say in, this file decides**: a mark usually takes its size from the piece it
 * sits on, and a symbol has no piece. @ref Build draws every mark at the size below, and @ref Draw
 * fits that into whatever room the caller gives it.
 */
namespace OSymbolRender
{
/**
 * @brief A shape as it is drawn — in the two ways a mark is drawn.
 *
 * A mark is not one kind of ink: a grain line is a line **and** two arrowheads, and the arrowheads
 * are filled polygons while the line is a stroke. Keeping the two apart is what lets a caller draw
 * a symbol the way the canvas does, rather than approximating it with an outline.
 */
struct Drawing
{
    QVector<QPainterPath> strokes; ///< traced with the pen
    QVector<QPainterPath> fills;   ///< filled with the ink
};

/** @brief The shape at its own size, in its own coordinates, origin near its middle. */
auto Build(const OSymbolShapes::Shape &shape) -> Drawing;

/**
 * @brief The marks of one legend line, side by side, ready to be drawn.
 *
 * **A symbol may stand for several marks, and a legend shows them as one thing**: three notches named
 * "notches" are a row of marks with one word after them, not three lines. They follow one another at
 * the size each will print at — what makes a legend worth reading is that its marks can be held
 * against the pattern — and the row is centred on its own middle, so marks of different heights read
 * as one row rather than as a ragged top.
 *
 * The block's origin is its top-left corner, and it is handed to the page's painter as it is
 * (@ref OCartouchePainter::TextLineWithSymbols draws it before the words of the line).
 */
auto Compose(const QVector<OSymbolShapes::Shape> &shapes) -> OCartoucheSymbolMarks;

/**
 * @brief The room @ref Build takes, in print px.
 *
 * A caller that wants a symbol to keep its proportions against its neighbours asks for this: how
 * much room a mark needs is a property of the mark, and a rectangle picked for it would say the
 * grain line and the buttonhole are the same size.
 */
auto NaturalSize(const OSymbolShapes::Shape &shape) -> QSizeF;

/**
 * @brief Draw @p shape inside @p rect, centred and keeping its proportions.
 *
 * **A symbol has a size of its own** (@ref NaturalSize), and this draws it in whatever room the
 * caller gives it: a caller showing a legend hands it a rectangle the size of the mark itself, so
 * that what is seen is what will print, and one laying out a page works out its rectangles first.
 * The mark is never distorted — a line stays a line — so the rectangle is filled on the side that
 * binds.
 *
 * @param penWidth the line's width, in the units of @p rect. A symbol is drawn at the line width of
 *                 the surface it lands on, so a caller that scales its surface scales this too.
 */
void Draw(QPainter &painter, const QRectF &rect, const OSymbolShapes::Shape &shape, const QColor &color,
          qreal penWidth = 1.0);
} // namespace OSymbolRender

#endif // OSYMBOLRENDER_H
