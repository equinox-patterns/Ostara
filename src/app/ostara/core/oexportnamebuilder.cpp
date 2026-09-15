/************************************************************************
 **
 **  @file   oexportnamebuilder.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the generation name builder.
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

#include "oexportnamebuilder.h"

namespace
{
/// The separator between the parts. A hyphen rather than a space: the range name
/// already carries spaces ("Femme 36-44"), and a space separator would make the
/// name unreadable the moment one appears.
const QLatin1Char kSeparator('-');
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportNameBuilder::Base(const QString &rangeName, const QStringList &bases, const QString &token) -> QString
{
    QStringList parts;

    if (!rangeName.isEmpty())
    {
        parts.append(rangeName);
    }
    else
    {
        for (const QString &base : bases)
        {
            // An empty base is an axis the table does not carry. Skipped, not joined:
            // an empty field would read as a value that is missing rather than absent.
            if (!base.trimmed().isEmpty())
            {
                parts.append(base.trimmed());
            }
        }
    }

    if (!token.isEmpty())
    {
        parts.append(token);
    }

    return parts.join(kSeparator);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportNameBuilder::Unique(const QString &base, const QStringList &taken) -> QString
{
    if (!taken.contains(base))
    {
        return base;
    }

    // First free suffix wins. The scan restarts from 1 on every call rather than
    // remembering where it stopped: a name freed by a deletion has to become
    // available again, and nothing here outlives the call.
    for (int suffix = 1;; ++suffix)
    {
        const QString candidate = base + QStringLiteral(" (%1)").arg(suffix);
        if (!taken.contains(candidate))
        {
            return candidate;
        }
    }
}
