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
class QPainterPath;
class QPen;
class QTransform;

namespace OSvg
{
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

/** @brief Safe QGraphicsPathItem cast (handles VGraphicsFillItem/VGraphicsFoldLineItem). */
QGraphicsPathItem *AsPathItem(QGraphicsItem *item);

/** @brief Whether a piece child is "info" (grain/fold/text/pattern) vs geometry. */
bool IsInfoChild(const QGraphicsItem *child);
}  // namespace OSvg

#endif  // OSVGUTILS_H
