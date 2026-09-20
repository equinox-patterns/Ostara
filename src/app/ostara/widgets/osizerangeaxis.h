/************************************************************************
 **
 **  @file   osizerangeaxis.h
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

#ifndef OSIZERANGEAXIS_H
#define OSIZERANGEAXIS_H

#include <QStringList>
#include <QtGlobal>

class QComboBox;

/**
 * @brief The two ends of a size range, and the one rule that holds them together.
 *
 * **A range is chosen, never typed.** The table declares an axis's values and their order, so the
 * ends of a range are two of them — « from 36 to 44 » is a choice among the sizes the table has,
 * not a pair of numbers that might name a size nobody makes. The counters offer exactly those
 * values, in the table's own order, and these two functions are what puts them there and says
 * whether the answer is a range at all.
 *
 * They are shared because two dialogs ask the same question of the same table: the Graded mode's
 * size ranges, and the range a cover page previews. Two implementations of « when are the two ends
 * in order » would be two answers to one question.
 */
namespace OSizeRangeAxis
{
/**
 * @brief Fill the two counters from one axis's values: the first and the last are the range.
 *
 * A whole axis is the range a user who chooses nothing gets, and it is the only default that is
 * never wrong: an empty or reversed opening would be a range nobody asked for.
 */
void FillMinMax(QComboBox *minCombo, QComboBox *maxCombo, const QStringList &values);

/**
 * @brief True when the two counters name a range rather than a hole.
 *
 * Read off the positions, not the values: the axis lists its sizes in the order it makes them, so
 * a minimum that stands after the maximum is a range that would come out empty.
 */
Q_REQUIRED_RESULT auto IsOrdered(const QComboBox *minCombo, const QComboBox *maxCombo) -> bool;
} // namespace OSizeRangeAxis

#endif // OSIZERANGEAXIS_H
