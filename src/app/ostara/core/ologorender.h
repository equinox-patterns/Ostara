/************************************************************************
 **
 **  @file   ologorender.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  A logo turned into something a painter can draw — the one place it is done.
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

#ifndef OLOGORENDER_H
#define OLOGORENDER_H

#include <QPixmap>
#include <QString>

/**
 * @brief Turning a logo file into a pixmap, at the size its template asked for.
 *
 * **It was written three times before it was written once.** Drawing a logo means the same six
 * steps everywhere — read the name out of the options, build the path, load the SVG, refuse it
 * when it does not load, turn centimetres into pixels, keep the file's proportions — and two
 * copies of those steps had already drifted apart before a third was about to be made. A
 * preview and a resolver disagreeing about how big a logo is would be a preview that lies.
 *
 * **The size is in centimetres and the answer is in pixels**, because the template stores what a
 * user typed and a painter draws what a screen has. The conversion is the application's printing
 * resolution, so the logo is the size it will be on paper however big the preview is drawn — what
 * a preview does with that is its own business, and it is why a caller that scales passes the
 * scale to the painter rather than to this.
 */
namespace OLogoRender
{
/** @brief A logo drawn, and the height it was drawn at. */
struct Drawing
{
    QPixmap pixmap;      ///< null when there is nothing to draw — a missing or unreadable file
    double heightPx{0.0}; ///< the height the painter is told the line is, in pixels
};

/** @brief How tall a logo of @p sizeCm is drawn, in pixels. */
auto HeightFor(double sizeCm) -> double;

/**
 * @brief Draws @p fileName, from the logo library, @p sizeCm centimetres tall.
 *
 * The proportions of the file are kept: the height is what was asked for, and the width follows.
 * A file that cannot be read gives back an empty drawing rather than a placeholder — nothing is
 * drawn, which is what a cover page with no logo on it should look like.
 */
auto Draw(const QString &fileName, double sizeCm) -> Drawing;
} // namespace OLogoRender

#endif // OLOGORENDER_H
