/************************************************************************
 **
 **  @file   mainwindow_graded_helpers.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: item roles and predicates shared by the Graded-mode TUs.
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

#ifndef MAINWINDOW_GRADED_HELPERS_H
#define MAINWINDOW_GRADED_HELPERS_H

#include <QGraphicsItem>
#include <QVariant>
#include <QtGlobal>

class QGraphicsScene;
class VContainer;

/// Item roles tagging every graded item so alignment / offsets / drag can recover the owning
/// piece id, whether the item is the reference (layer 0) of its piece, and the position base
/// (mx/my) the item was created at — used to make re-applies idempotent. Shared across the
/// Graded-mode translation units (mainwindow_graded_*.cpp). The per-child-type markers
/// (interactive grainline / info-block / annotation) stay file-local to their own TU.
constexpr int kGradedPieceIdRole     = Qt::UserRole + 900;
constexpr int kGradedIsRefRole       = Qt::UserRole + 901;
constexpr int kGradedBasePosRole     = Qt::UserRole + 902;
constexpr int kGradedHostGroupIdRole = Qt::UserRole + 903; // invisible group host (holds group children)

/// Piece id carried by an item (or 0 if the item isn't a graded piece item).
inline auto GradedPieceId(const QGraphicsItem *item) -> quint32
{
    if (item == nullptr)
    {
        return 0;
    }
    const QVariant id = item->data(kGradedPieceIdRole); // data() is virtual — call it once
    return id.isNull() ? 0 : id.toUInt();
}

/// True when the item is the reference (topmost, first size) of its graded piece.
inline auto GradedIsReference(const QGraphicsItem *item) -> bool
{
    return item != nullptr && item->data(kGradedIsRefRole).toBool();
}

/// Rebuild the per-group reference decoration (defined in mainwindow_graded_group.cpp): mask the
/// grouped members' own grainline/fold and (re)anchor each group's invisible host onto the union
/// of its graded reference members. Positions must be final when called.
void RefreshGradedReferenceGroupDecor(VContainer *pattern, QGraphicsScene *scene);

#endif // MAINWINDOW_GRADED_HELPERS_H
