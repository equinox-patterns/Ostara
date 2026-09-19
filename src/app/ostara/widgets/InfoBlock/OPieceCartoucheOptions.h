/************************************************************************
 **
 **  @file   OPieceCartoucheOptions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  The option vocabulary of the piece-cartouche catalogue.
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

#ifndef OPIECECARTOUCHEOPTIONS_H
#define OPIECECARTOUCHEOPTIONS_H

#include "core/otemplateaxisoptions.h"

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

/**
 * @brief The option keys, and the value shapes, that only the piece cartouche uses.
 *
 * The mechanism — what a section is, what a field is, how a default is answered — is shared and
 * lives in `core/otemplatesection.h`. What is left in this header belongs to this family alone:
 * a cartouche cuts fabric, thermofusible and lining, numbers its pieces, and renames a size axis
 * in each of the languages it is written in. A cover page will do none of that.
 */

/**
 * @brief Option keys of the Cut section — one section covers the three fabrics
 * (fabric / interfacing / lining); each filled tab becomes one label line.
 */
namespace OPieceCartoucheCut
{
inline const QString kFabricQty = QStringLiteral("fabricQty");
inline const QString kFabricOnFold = QStringLiteral("fabricOnFold");
inline const QString kInterfacingQty = QStringLiteral("interfacingQty");
inline const QString kInterfacingOnFold = QStringLiteral("interfacingOnFold");
inline const QString kLiningQty = QStringLiteral("liningQty");
inline const QString kLiningOnFold = QStringLiteral("liningOnFold");

inline const QStringList kQtyKeys = {kFabricQty, kInterfacingQty, kLiningQty};
inline const QStringList kFoldKeys = {kFabricOnFold, kInterfacingOnFold, kLiningOnFold};
inline const QStringList kAllKeys = {kFabricQty, kFabricOnFold, kInterfacingQty, kInterfacingOnFold,
                                     kLiningQty, kLiningOnFold};
}  // namespace OPieceCartoucheCut

/**
 * @brief Option keys of the axis-rename composite: per-language display aliases
 * of an axis and its values (never "alias" in the UI).
 *
 * The keys themselves moved to the shared vocabulary — a cover page renames the axes it shows the
 * same way, and the two editors are meant to be the same editor. What is left here is the name
 * this family knew them by.
 */
namespace OPieceCartoucheAxisRename
{
/// The shared key of the alias map. See `core/otemplateaxisoptions.h`.
inline const QString kRenames = OTemplateAxisOptions::kRenames;
/// The shared key of the "one line per language" flag.
inline const QString kOneLine = OTemplateAxisOptions::kOneLine;
}  // namespace OPieceCartoucheAxisRename

/**
 * @brief Option of the piece-number section: how the number and its total are
 * shown ("1 of 10" or "01 of 10"). Single digits only — 10 is never "010".
 */
namespace OPieceCartoucheNumber
{
/// QString — kPlain or kPadded.
inline const QString kStyle = QStringLiteral("numberStyle");
/// Show a single digit as is: 1, 2, … 9.
inline const QString kPlain = QStringLiteral("1");
/// Pad a single digit with a leading zero: 01, 02, … 09.
inline const QString kPadded = QStringLiteral("01");
}  // namespace OPieceCartoucheNumber

/// Format a piece number for display: kPadded pads single digits, anything else
/// (and any number of two digits or more) is left as is.
Q_REQUIRED_RESULT auto FormatPieceNumber(quint32 number, const QString &style) -> QString;

#endif // OPIECECARTOUCHEOPTIONS_H
