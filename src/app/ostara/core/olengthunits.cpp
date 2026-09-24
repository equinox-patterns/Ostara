/************************************************************************
 **
 **  @file   olengthunits.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  A length written in the unit the reader of a page thinks in.
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

#include "olengthunits.h"

#include "oexportunitformat.h"

#include <QString>

#include <cmath>

namespace
{
/// The millimetres in an inch — the exact definition, and the only conversion this file needs.
constexpr qreal kMmPerInch = 25.4;

/// The millimetres in a yard — exact too: 36 inches by definition.
constexpr qreal kMmPerYard = 914.4;

/// @p graduations of @p denominator, written as a mixed number and reduced.
///
/// `6/16` gives `3/8`, `16/16` gives `1`, `22/8` gives `2 3/4`. Both callers round their own way
/// before they get here — the inch to the nearest graduation, the yard up to the next one — and that
/// is deliberate, so the rounding stays **out** of this function: it is the one thing the two must
/// not share. What they do share is the writing, and that is all this is.
auto MixedNumber(int graduations, int denominator) -> QString
{
    const int whole = graduations / denominator;
    int numerator = graduations % denominator;

    if (numerator == 0)
    {
        // A whole number keeps no denominator: 2", never 2 0/16".
        return QString::number(whole);
    }

    // Halved until it will not halve again, so the reader gets 3/8 and not 6/16.
    while (numerator % 2 == 0)
    {
        numerator /= 2;
        denominator /= 2;
    }

    // Above the unit the fraction is carried beside the whole ones — 1 3/8 — which is how a tape
    // measure is read: past the last whole mark, then on by graduations.
    const QString fraction =
        QString::number(numerator) + QLatin1Char('/') + QString::number(denominator);
    return whole > 0 ? QString::number(whole) + QLatin1Char(' ') + fraction : fraction;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLengthUnits::Fraction(qreal inch) -> QString
{
    // **Rounded first, then reduced.** Rounding to the nearest graduation is what turns a converted
    // length back into something a tape measure can be read against: a centimetre is 0.3937 of an
    // inch, which is no graduation at all, and 3/8" is the graduation beside it.
    return MixedNumber(qRound(inch * kInchDenominator), kInchDenominator);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLengthUnits::Yards(qreal mm) -> QString
{
    // **Rounded up, and that is the opposite of the inch above — on purpose.** An inch is a
    // measurement, so it takes the nearest graduation. A yardage is a purchase: rounding 1.81 down to
    // 1 3/4 would send the reader to the shop for fabric the plan does not fit in, which is the one
    // error nothing downstream can repair. See the header for why the two are not harmonised.
    const qreal yards = mm / kMmPerYard;
    const int eighths = static_cast<int>(std::ceil(yards * kYardDenominator));
    return MixedNumber(eighths, kYardDenominator) + QLatin1Char(' ') + kYardSign;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLengthUnits::Format(qreal mm, const Unit &unit) -> QString
{
    if (unit == Unit::Inch)
    {
        return Fraction(mm / kMmPerInch) + kInchSign;
    }

    // **The print tab's own precision, borrowed rather than written again.** A length shown on a page
    // and the same length shown in the print tab are the same length; a second table of decimals is
    // how the two would come to disagree.
    return OExportUnitFormat::Format(mm, unit) + QLatin1Char(' ') + UnitsToStr(unit, true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLengthUnits::Value(qreal mm, const Unit &unit) -> QString
{
    if (unit == Unit::Inch)
    {
        // Bare as well: the inch sign belongs to the unit, and here the unit is named elsewhere. The
        // fraction itself is the same one @ref Format writes, reduced the same way.
        return Fraction(mm / kMmPerInch);
    }

    return OExportUnitFormat::Format(mm, unit);
}
