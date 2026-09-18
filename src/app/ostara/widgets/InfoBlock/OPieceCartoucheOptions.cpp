/************************************************************************
 **
 **  @file   OPieceCartoucheOptions.cpp
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

#include "OPieceCartoucheOptions.h"

auto ParseAxisRenames(const QVariant &var) -> QMap<QString, OAxisRename>
{
    QMap<QString, OAxisRename> result;
    const QVariantMap map = var.toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        const QVariantMap entry = it.value().toMap();
        OAxisRename rename;
        rename.name = entry.value(QStringLiteral("name")).toString();
        const QVariantList list = entry.value(QStringLiteral("values")).toList();
        rename.values.reserve(list.size());
        for (const QVariant &value : list)
        {
            rename.values.append(value.toString());
        }
        result.insert(it.key(), rename);
    }
    return result;
}

auto MakeAxisRenames(const QMap<QString, OAxisRename> &renames) -> QVariant
{
    QVariantMap map;
    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it)
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), it.value().name);
        QVariantList list;
        list.reserve(it.value().values.size());
        for (const QString &value : it.value().values)
        {
            list.append(value);
        }
        entry.insert(QStringLiteral("values"), list);
        map.insert(it.key(), entry);
    }
    return map;
}

auto FormatPieceNumber(quint32 number, const QString &style) -> QString
{
    // Padding is for single digits only: 1 becomes "01", but 10 stays "10" and 100 stays "100".
    if (style != OPieceCartoucheNumber::kPadded)
    {
        return QString::number(number);
    }
    return QString::number(number).rightJustified(2, QLatin1Char('0'));
}
