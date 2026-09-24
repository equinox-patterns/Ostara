/************************************************************************
 **
 **  @file   oexportrunqueue.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the queue of runs waiting to be generated.
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

#include "oexportrunqueue.h"

#include <QCoreApplication>

#include <utility>

//---------------------------------------------------------------------------------------------------------------------
auto OExportRunStateText(OExportRunState state) -> QString
{
    switch (state)
    {
        case OExportRunState::Running:
            return QCoreApplication::translate("OExportRunState", "Running");
        case OExportRunState::Done:
            return QCoreApplication::translate("OExportRunState", "Done");
        case OExportRunState::Failed:
            return QCoreApplication::translate("OExportRunState", "Failed");
        case OExportRunState::Pending:
        default:
            return QCoreApplication::translate("OExportRunState", "Waiting");
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRunQueue::IsEmpty() const -> bool
{
    return m_items.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRunQueue::Count() const -> int
{
    return static_cast<int>(m_items.size());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRunQueue::At(int index) const -> const OExportQueueItem &
{
    return m_items.at(index);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRunQueue::Append(OExportQueueItem item) -> int
{
    m_items.append(std::move(item));
    return static_cast<int>(m_items.size()) - 1;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRunQueue::RemoveAt(int index)
{
    if (index < 0 || index >= Count())
    {
        return;
    }

    m_items.removeAt(index);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRunQueue::Names() const -> QStringList
{
    QStringList names;
    names.reserve(static_cast<qsizetype>(m_items.size()));

    for (const OExportQueueItem &item : m_items)
    {
        names.append(item.name);
    }

    return names;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRunQueue::SetState(int index, OExportRunState state)
{
    if (index < 0 || index >= Count())
    {
        return;
    }

    m_items[index].state = state;
}
