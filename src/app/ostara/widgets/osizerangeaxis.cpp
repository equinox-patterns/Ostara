/************************************************************************
 **
 **  @file   osizerangeaxis.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The minimum/maximum pair of a size range, as two combos of the axis's own values.
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

#include "osizerangeaxis.h"

#include <QComboBox>

namespace OSizeRangeAxis
{
void FillMinMax(QComboBox *minCombo, QComboBox *maxCombo, const QStringList &values)
{
    if (minCombo == nullptr || maxCombo == nullptr)
    {
        return;
    }

    minCombo->clear();
    maxCombo->clear();
    minCombo->addItems(values);
    maxCombo->addItems(values);

    if (values.isEmpty())
    {
        return;
    }
    minCombo->setCurrentIndex(0);
    maxCombo->setCurrentIndex(static_cast<int>(values.size()) - 1);
}

auto IsOrdered(const QComboBox *minCombo, const QComboBox *maxCombo) -> bool
{
    if (minCombo == nullptr || maxCombo == nullptr)
    {
        return false;
    }
    const int minimum = minCombo->currentIndex();
    const int maximum = maxCombo->currentIndex();
    return minimum >= 0 && maximum >= minimum;
}
} // namespace OSizeRangeAxis
