/************************************************************************
 **
 **  @file   oexportunitformat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  How a length of the print tab is shown, and how it is read back —
 **          millimetres stored, the user's unit on screen.
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

#ifndef OEXPORTUNITFORMAT_H
#define OEXPORTUNITFORMAT_H

#include <QString>
#include <QVector>

#include "../vmisc/def.h"

/**
 * @brief The display unit of the print tab, and the one conversion it implies.
 *
 * Every length of the print tab — the sheet's dimensions, the document margin, the space
 * between pieces — is stored in **millimetres**, and shown in the unit the user picked.
 * Millimetres are the one fixed point: a sheet size is a standard before it is a number
 * on screen, and storing what the user happens to be reading would mean the stored value
 * changed meaning the day she switched to inches.
 *
 * The conversion lives here rather than in each widget for the obvious reason: three
 * fields converting on their own is three chances to convert differently.
 */
namespace OExportUnitFormat
{
/// @brief The units the print tab offers, in the order its combo lists them.
auto SelectableUnits() -> const QVector<Unit> &;

/// @brief The units a laize may be entered in, in the order its combo lists them.
///
/// A second catalogue rather than a filter over the first, because the two answer different
/// questions: the print tab offers every unit a *sheet* may be described in, and a sheet can be
/// described in millimetres. Nobody buys 1400 mm of cloth. A laize is either metric or imperial —
/// the two worlds a fabric shop lives in — and the millimetre belongs to neither.
auto FabricWidthUnits() -> const QVector<Unit> &;

/// @brief How many decimals @p unit is written with before trailing zeros are dropped.
auto Decimals(const Unit &unit) -> int;

/// @brief @p mm written in @p unit, trailing zeros removed.
///
/// The stripping is what makes the field readable: a sheet is 19 cm, not 19.00 cm, and
/// "19.00" would claim a precision the standard does not have. It also keeps the values
/// the user set — 0.5 cm stays 0.5 cm — from drifting into 0.50 cm on the way back.
auto Format(qreal mm, const Unit &unit) -> QString;

/// @brief A length typed in @p unit, given back in millimetres.
///
/// Accepts both decimal separators: the keyboard offers the one the locale uses, which is
/// not the C locale's. @p ok is false when the text is not a number at all — which is not
/// the same answer as zero, since zero IS a legitimate margin.
auto Parse(const QString &text, const Unit &unit, bool *ok = nullptr) -> qreal;
} // namespace OExportUnitFormat

#endif // OEXPORTUNITFORMAT_H
