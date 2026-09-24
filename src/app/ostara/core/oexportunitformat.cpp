/************************************************************************
 **
 **  @file   oexportunitformat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the print tab's length formatting.
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

#include "oexportunitformat.h"

namespace
{
/// Millimetres get one decimal rather than none: the space between two pieces is set in
/// half-millimetres as often as in whole ones, and rounding to the unit would silently
/// turn 0.5 into 1.
const QVector<Unit> kUnits{Unit::Mm, Unit::Cm, Unit::Inch};

/// A laize, in the two worlds a fabric shop lives in. Centimetres first: a metric bolt is labelled
/// 140, and that is the reading the tab opens on.
const QVector<Unit> kFabricWidthUnits{Unit::Cm, Unit::Inch};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportUnitFormat::SelectableUnits() -> const QVector<Unit> &
{
    return kUnits;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportUnitFormat::FabricWidthUnits() -> const QVector<Unit> &
{
    return kFabricWidthUnits;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportUnitFormat::Decimals(const Unit &unit) -> int
{
    switch (unit)
    {
        case Unit::Mm:
            return 1;
        case Unit::Cm:
            return 2;
        case Unit::Inch:
            return 3;
        default:
            return 2;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportUnitFormat::Format(qreal mm, const Unit &unit) -> QString
{
    const qreal value = UnitConvertor(mm, Unit::Mm, unit);
    QString text = QString::number(value, 'f', Decimals(unit));

    if (text.contains(QLatin1Char('.')))
    {
        while (text.endsWith(QLatin1Char('0')))
        {
            text.chop(1);
        }
        if (text.endsWith(QLatin1Char('.')))
        {
            text.chop(1);
        }
    }

    return text;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportUnitFormat::Parse(const QString &text, const Unit &unit, bool *ok) -> qreal
{
    QString normalized = text.trimmed();
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));

    bool parsed = false;
    const qreal value = normalized.toDouble(&parsed);

    if (ok != nullptr)
    {
        *ok = parsed;
    }
    if (!parsed)
    {
        return 0.0;
    }

    return UnitConvertor(value, unit, Unit::Mm);
}
