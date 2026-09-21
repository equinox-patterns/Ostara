/************************************************************************
 **
 **  @file   olengthunits.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  A length written in the unit the reader of a page thinks in,
 **          inches included — and inches are written the way a seamstress
 **          reads them, as a fraction of a sixteenth.
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

#ifndef OLENGTHUNITS_H
#define OLENGTHUNITS_H

#include <QString>

#include "../vmisc/def.h"

/**
 * @brief How a length is written when the page has to state it in more than one unit at once.
 *
 * A page that says what a pattern's margins are may have to say it in centimetres *and* in inches,
 * because the two readers of that page do not measure the same way. That is a second job for a
 * length: @ref OExportUnitFormat writes one unit for a field the user is editing, and this writes
 * the same length for somebody reading a printed sheet.
 *
 * **The inch is not a decimal here, and that is the whole reason this file exists.** Nobody reads
 * `0.375"` off a pattern: they read `3/8"`, because a tape measure is graduated in halves, quarters,
 * eighths and sixteenths and nothing else. A decimal inch on a printed page is a number the reader
 * has to convert in their head before they can use it — which is exactly what the fraction spares
 * them.
 *
 * **Millimetres are the unit everything is kept in**, as they are in the print tab: a length stored
 * in whatever unit the page happened to show would change meaning the day somebody changed the page.
 */
namespace OLengthUnits
{
/// @brief How finely an inch is cut — the smallest graduation a tape measure carries.
inline constexpr int kInchDenominator = 16;

/// @brief The sign an inch carries, in place of the word other units are written with.
///
/// A fraction and a word do not sit well together — `3/8 Pouce` reads as two things — and the double
/// prime is what the mark means to anyone who works in inches. It is not translated: it is the same
/// sign in every language.
inline const QString kInchSign = QStringLiteral("\"");

/// @brief @p mm written in @p unit, with the unit's sign after it.
///
/// Decimal for millimetres and centimetres, using the print tab's own precision so that a length of
/// a page and a length of the print tab cannot come out disagreeing. Fractional for inches.
auto Format(qreal mm, const Unit &unit) -> QString;

/// @brief @p inch written as the nearest sixteenth, reduced, with the inch sign.
///
/// `0.3937` gives `3/8"`, `1.375` gives `1 3/8"`, `2.0` gives `2"`. A whole number of inches keeps no
/// denominator, because `2 0/16` is not how anyone writes it.
auto Fraction(qreal inch) -> QString;
} // namespace OLengthUnits

#endif // OLENGTHUNITS_H
