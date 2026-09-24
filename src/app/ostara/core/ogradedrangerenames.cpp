/************************************************************************
 **
 **  @file   ogradedrangerenames.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the graded size-range rename journal.
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

#include "ogradedrangerenames.h"

#include <QHash>
#include <QSet>

namespace
{

/// The journal itself. A function-local static, so that it is built on first use and never
/// before main() has a say — the same reason the settings keys are Q_GLOBAL_STATIC.
auto Journal() -> QHash<QString, QString> &
{
    static QHash<QString, QString> journal;
    return journal;
}

}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeRenames::Remember(const QString &oldName, const QString &newName)
{
    if (oldName.isEmpty() || newName.isEmpty() || oldName == newName)
    {
        return;
    }
    Journal().insert(oldName, newName);
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeRenames::Resolve(const QString &name) -> QString
{
    QString current = name;
    QSet<QString> visited;

    while (!current.isEmpty() && !visited.contains(current))
    {
        const auto it = Journal().constFind(current);
        if (it == Journal().cend())
        {
            break;
        }
        visited.insert(current);
        current = it.value();
    }

    return current;
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeRenames::Forget()
{
    Journal().clear();
}
