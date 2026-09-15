/************************************************************************
 **
 **  @file   oexportrunqueue.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The runs the Export mode has been asked for but which have not
 **          been generated yet — the list the entry dialog fills, and the
 **          order it fires them in.
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

#ifndef OEXPORTRUNQUEUE_H
#define OEXPORTRUNQUEUE_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "oexportoutput.h"

/**
 * @brief Where a queued run stands.
 *
 * The last two are the only ones the user sees settle: a batch walks the queue, and a line
 * the engine refused is reported on the line itself rather than by stopping the walk.
 */
enum class OExportRunState
{
    Pending, ///< added to the list, never asked for yet
    Running, ///< handed to the engine
    Done,    ///< generated and filed in the dock
    Failed   ///< the engine refused it — the reason has already been shown
};

/// @brief One line of the list: what to generate, under which name, and where it stands.
struct OExportQueueItem
{
    QString name{};
    OExportRunRequest request{};
    OExportRunState state{OExportRunState::Pending};
};

/**
 * @brief The runs waiting to be generated, in the order they were added.
 *
 * The queue holds the request and the NAME together, and that pairing is the whole point:
 * a line is named the moment it is added, not when it runs. The list shows a name before
 * anything has been nested — which is what the user is deciding on — and the suffix that
 * makes that name unique has to be settled then, or two identical lines would be born
 * carrying the same one.
 *
 * Session-scoped today: the entry dialog owns it and it dies with the dialog. It is a class
 * of its own rather than a vector buried in a widget because the day the list has to outlive
 * the dialog — the favourites — what moves is who owns this, and nothing else.
 */
class OExportRunQueue
{
public:
    auto IsEmpty() const -> bool;
    auto Count() const -> int;

    /// @brief The line at @p index. Undefined when the index is out of range.
    auto At(int index) const -> const OExportQueueItem &;

    /// @brief Append @p item at the end of the queue. @return the index it took.
    auto Append(OExportQueueItem item) -> int;

    /// @brief Drop the line at @p index. No-op when the index is out of range.
    void RemoveAt(int index);

    /// @brief The names the queued lines carry — what a new line must not collide with.
    auto Names() const -> QStringList;

    /// @brief Write where the line at @p index stands. No-op when the index is out of range.
    void SetState(int index, OExportRunState state);

private:
    QVector<OExportQueueItem> m_items{};
};

#endif // OEXPORTRUNQUEUE_H
