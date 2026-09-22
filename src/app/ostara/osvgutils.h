/************************************************************************
 **
 **  @file   osvgutils.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Shared low-level SVG emission primitives used by the Ostara SVG
 **          exporters (manual QXmlStreamWriter writers). Kept apart from the
 **          flat Details writer (OInfoBlockSvg) so a new graded writer can
 **          reuse them without touching the stable mono-size writer.
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

#ifndef OSVGUTILS_H
#define OSVGUTILS_H

#include <QtGlobal>

class QGraphicsItem;
class QGraphicsPathItem;
class QLineF;
class QPainterPath;
class QPen;
class QTransform;
class QXmlStreamWriter;

namespace OSvg
{
/// @brief The namespace Ostara marks its own attributes with — `ostara:layout`, `ostara:font-face`.
///
/// Declared on the root of every document that carries one: a prefix with no declaration is not
/// XML a parser will read, and the reading side asks for the literal qualified name.
constexpr auto kOstaraNamespace = "urn:ostara";

/** @brief Serialize a QPainterPath into an SVG "d" attribute (M/L/C). */
QString PainterPathToSvgD(const QPainterPath &path);

/** @brief Make an SVG id from an arbitrary display name (XML-friendly). */
QString SanitizeSvgId(const QString &name);

/** @brief Serialize a QTransform into an SVG "matrix(...)" attribute. */
QString MatrixAttr(const QTransform &matrix);

/**
 * @brief SVG stroke-dasharray for a pen. Handles the named Qt styles AND
 *        Qt::CustomDashLine (pattern × pen width → px), which is what the
 *        Graded size styles use (dashes stored in pt, converted to px).
 */
QString DashArray(const QPen &pen);

/**
 * @brief Write a straight segment as an SVG `<line>`, with the pen's ink, width and dashes.
 *
 * The coordinates are the caller's, already in the space of the group it writes into. The dashes go
 * out through @ref DashArray, so a pen built anywhere in the application serializes the same way —
 * and a solid line writes no `stroke-dasharray` at all, which is how SVG says solid.
 *
 * **One writer, because two exports draw the same segment**: the cartouche's blocks and the graded
 * sheet both walk the same placed items, and a size's trace sample has to look the same on both.
 */
void WriteLine(QXmlStreamWriter &writer, const QLineF &segment, const QPen &pen);

/** @brief Safe QGraphicsPathItem cast (handles VGraphicsFillItem/VGraphicsFoldLineItem). */
QGraphicsPathItem *AsPathItem(QGraphicsItem *item);

/** @brief Whether a piece child is "info" (grain/fold/text/pattern) vs geometry. */
bool IsInfoChild(const QGraphicsItem *child);
}  // namespace OSvg

#endif  // OSVGUTILS_H
