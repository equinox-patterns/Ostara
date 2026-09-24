/************************************************************************
 **
 **  @file   othumbnailtextbuilder.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25 September, 2026
 **
 **  @brief  What the piece miniature says, in the application's own words:
 **          the seam-allowance values it is cut with, and the legend that
 **          names their unit — in the languages the pattern is written in.
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

#ifndef OTHUMBNAILTEXTBUILDER_H
#define OTHUMBNAILTEXTBUILDER_H

#include "../vpatterndb/othumbnaildata.h"

class VPiece;
class VAbstractPattern;
class VContainer;

/**
 * @brief The application's answer to @ref OThumbnailTextResolver.
 *
 * Units, decimal separator and translation all live in the application, so the library that draws
 * the miniature asks for the words instead of owning them. Nothing here reads the container: the
 * values are the ones the miniature already stores, in millimetres, and no formula is ever
 * evaluated — a seam allowance is a number the user typed.
 */
namespace OThumbnailTextBuilder
{
auto Build(const VPiece &piece, VAbstractPattern *doc, const VContainer *data) -> OThumbnailTexts;

/// @brief One value as the miniature prints it: `1`, `3/8`, or both at once as `1 (3/8)`.
///
/// The panel previews a row with this and the canvas prints with it, so the two cannot come out
/// saying different things about the same margin.
auto ValueText(qreal mm, bool centimeters, bool inches) -> QString;
} // namespace OThumbnailTextBuilder

#endif // OTHUMBNAILTEXTBUILDER_H
