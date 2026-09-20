/************************************************************************
 **
 **  @file   ocoverpagelayout.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The page a cover page is written on, and where the bands stand on it.
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

#ifndef OCOVERPAGELAYOUT_H
#define OCOVERPAGELAYOUT_H

#include "ocoverpagetemplate.h"

#include "vwidgets/ocartouchepainter.h"

#include <QMap>
#include <QRectF>
#include <QSizeF>
#include <QVector>

/**
 * @brief The page, and the skeleton the informations stand on.
 *
 * **The page is the feature's, not the preview's.** A cover page is 19 × 25.9 cm, always. The
 * preview shows it reduced to fit its pane; the PDF export draws it at its real size. Both read
 * the size from here, and everything on the page is placed here too, in print units — so an
 * information asked for in centimetres takes that many centimetres on the sheet, and the drawing
 * that comes out of the export is the drawing the user was shown, at the size it will be printed.
 *
 * That is also why **nothing here knows about a screen**: the measurements are printer pixels, and
 * a display scales the whole result at the end rather than being laid out into.
 */
namespace OCoverPageLayout
{
/**
 * The sheet a cover page is written on — these two, whatever the pattern.
 *
 * The size is not a property of the preview: a page laid out at another size would not be this
 * pattern's cover page, and an information measured in centimetres would be printed at the wrong
 * one.
 */
constexpr qreal kPageWidthCm = 19.0;
constexpr qreal kPageHeightCm = 25.9;

/** @brief The page at print size, in printer pixels — what those two centimetres come to. */
auto PageSize() -> QSizeF;

/**
 * @brief How wide an information may be on this band, in printer pixels.
 *
 * **The page's own rule, said once.** A band's content takes a share of the band, and the middle
 * band is two columns of half a page each — so a middle information has half the width of a top
 * one, and a text that fits in the one runs off the other. A caller that folds a text to fit needs
 * this number, and it must be the very one @ref Place cuts its rectangles with: two answers to one
 * question is a text folded to a width the page does not have.
 */
Q_REQUIRED_RESULT auto ContentWidth(OCoverPageTemplate::Placement placement) -> qreal;

/**
 * @brief Where each band's content is drawn, at print size, in printer pixels.
 *
 * The three stacked bands — the top one, the middle one's two columns, the bottom one — of which
 * the middle is as tall as its tallest column. They are stacked as one block and the block is
 * centred, so a template that fills only the middle band sits in the middle of the page.
 *
 * **The widths are the skeleton's, never the user's**: full width, or half of it. And the content
 * of every band takes at most 95 % of its own — the two columns of the middle would otherwise meet
 * at the seam and read as one line rather than two, and all four share one margin, a band touching
 * the edge while a column does not looking like a mistake rather than a choice.
 *
 * A band that holds nothing has no height and gets no rectangle: nothing is drawn there, and what
 * is below it moves up rather than leaving a gap where a band would have been.
 */
auto Place(const QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> &bandLines)
    -> QMap<OCoverPageTemplate::Placement, QRectF>;
} // namespace OCoverPageLayout

#endif // OCOVERPAGELAYOUT_H
