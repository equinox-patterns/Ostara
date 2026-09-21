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

namespace
{
/// The millimetres in an inch — the exact definition, and the only conversion this file needs.
constexpr qreal kMmPerInch = 25.4;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLengthUnits::Fraction(qreal inch) -> QString
{
    // **Rounded first, then reduced.** Rounding to the nearest graduation is what turns a converted
    // length back into something a tape measure can be read against: a centimetre is 0.3937 of an
    // inch, which is no graduation at all, and 3/8" is the graduation beside it.
    const int sixteenths = qRound(inch * kInchDenominator);
    const int whole = sixteenths / kInchDenominator;
    int numerator = sixteenths % kInchDenominator;
    int denominator = kInchDenominator;

    if (numerator == 0)
    {
        // A whole number of inches keeps no denominator: 2", never 2 0/16".
        return QString::number(whole);
    }

    // Halved until it will not halve again, so the reader gets 3/8 and not 6/16.
    while (numerator % 2 == 0)
    {
        numerator /= 2;
        denominator /= 2;
    }

    // Above an inch the fraction is carried beside the whole inches — 1 3/8" — which is how a tape
    // measure is read: past the last whole mark, then on by graduations.
    const QString fraction =
        QString::number(numerator) + QLatin1Char('/') + QString::number(denominator);
    return whole > 0 ? QString::number(whole) + QLatin1Char(' ') + fraction : fraction;
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
