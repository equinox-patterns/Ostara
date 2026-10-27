/************************************************************************
 **
 **  @file   otooltip.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/10/2026
 **
 **  @brief  The hover tooltip of a Draw-mode object: what it may name, and how it reads a length.
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
#ifndef OTOOLTIP_H
#define OTOOLTIP_H

#include <QString>
#include <QStringList>

#include "../vtheme/themeDef.h"

class VGObject;

/** @brief The three rules of the hover tooltip, in one place.
 *
 *  **A point name is never written** — neither alone (`A1`) nor inside an object name (`Ligne_A_B`). A
 *  distance that only a point name could label is a distance the user has no use for; the tooltip drops it.
 *
 *  **A named object writes its alias only** — `Ligne_…`, `Arc_…`, `ElArc_…`, `Courbe_…` — and writes
 *  nothing at all when the user has not named it. A technical name is the reader's route, not a label: the
 *  canvas must never show `Ligne_A_B` or `Spl_A_B`. Curves are read as `Courbe_` whatever head the file
 *  carries (`Spl_`, `SplPath_`).
 *
 *  **A length that belongs to a coloured stroke wears that stroke's colour**, so the number can be matched
 *  to the line on the canvas: green (`VisSupportColor2`) is the first part, red (`VisSupportColor4`) the
 *  remainder, magenta (`VisSupportColor`) the support, blue (`VisSupportColor3`) the fourth part of the two
 *  intersection tools. The colour comes from the theme, never from a literal.
 */
namespace OToolTip
{
/// @brief The alias a named object shows — `Courbe_dos`, `Ligne_emmanchure` — or an empty string when the
/// user has not named it. @param object a curve, an arc, an elliptical arc (a point never carries one).
auto DisplayName(const VGObject &object) -> QString;

/// @brief Same, from a `VGObject::ObjectName()` result (`Spl_dos (Spl_A_B)` → `Courbe_dos`). The technical
/// name alone (`Spl_A_B`) answers an empty string: there is no alias in it to show.
auto DisplayName(const QString &objectName) -> QString;

/** @brief The alias a named LINE shows — `Ligne_emmanchure` — or an empty string when it has none.
 *
 * A line is not a `VGObject`: its alias is the role name the container keeps under the pair of its points
 * (`VContainer::LineRoleName`, `VToolLine::GetLineRoleName`). @param roleName that suffix, empty when the
 * line is unnamed.
 */
auto LineName(const QString &roleName) -> QString;

/// @brief A length as the tooltip writes it: pixels turned into the pattern's unit, `12,3 cm`.
/// @param pixels the model's own value, what every geometric object answers.
auto Measure(qreal pixels) -> QString;

/// @brief An angle as the tooltip writes it: the model's degrees, `45°`.
auto Degrees(qreal angle) -> QString;

/// @brief A table of rows, or an empty string when no row has anything to say — which clears the tooltip
/// instead of leaving an empty frame on the canvas. @param rows the `<tr>` fragments, in reading order.
auto Table(const QStringList &rows) -> QString;

/// @brief One `<b>label:</b> value` row.
auto Row(const QString &label, const QString &value) -> QString;

/// @brief A row carrying an object's alias as its title; empty when the object has none.
auto NameRow(const VGObject &object) -> QString;

/// @brief A title row carrying a name the caller already built — a line's, which has no `VGObject` behind it.
auto NameRow(const QString &name) -> QString;

/** @brief A row whose value belongs to a coloured stroke: a square in the stroke's own colour leads it.
 *
 * The square is the "pastille" the options dock already uses on the name of that same stroke, so the user
 * reads the tooltip and the canvas with one code. @param label what the value is; @param value the number
 * and its unit; @param role the colour role the visualization gave that stroke.
 */
auto StrokeRow(const QString &label, const QString &value, VColorRole role) -> QString;

/** @brief A row naming a stroke the preview paints in `role`'s colour, or nothing when it has no alias.
 *
 * The strokes of a cut or an intersection are named by the user, not by their points: an unnamed half says
 * nothing here, and the canvas already shows it.
 */
auto StrokeNameRow(const QString &label, const VGObject &object, VColorRole role) -> QString;

/// @brief The horizontal rule the operations use to separate their own parameters.
auto Separator() -> QString;
} // namespace OToolTip

#endif // OTOOLTIP_H
