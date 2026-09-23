/************************************************************************
 **
 **  @file   oexportcuttingguard.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — what a cutting plan refuses to run without.
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

#ifndef OEXPORTCUTTINGGUARD_H
#define OEXPORTCUTTINGGUARD_H

#include <QMap>
#include <QString>
#include <QVector>
#include <QtGlobal>

class VContainer;

/**
 * @brief What a cutting plan needs before it may run — and which pieces do not have it.
 *
 * A cutting plan lays pieces on fabric, and a piece of fabric has a grain. A piece with no known
 * direction cannot be laid on it: the nesting would have to invent one, and a plan built on an
 * invented direction looks exactly like a plan built on a real one. So the plan refuses to start.
 *
 * Two things name a direction, and they are equivalent: an **enabled grainline**, or a **fold
 * line**. A member of a piece group carries neither of its own — the group hosts one grainline
 * for all of them, and the group's fold line lives on the single member designated to draw it —
 * so the group is read before a piece is called faulty.
 *
 * This file holds no widget and no scene on purpose: it is a business rule, and a rule that can
 * be wrong has to be readable on its own.
 */
namespace OExportCuttingGuard
{
/// One piece a cutting plan cannot place. It carries what the user calls it: piece names are not
/// unique, the number is.
struct FaultyPiece
{
    quint32 pieceId{0};
    uint number{0};
    QString name{};
};

/**
 * @brief The pieces a cutting plan will cut, in the order the pattern numbers them.
 *
 * A group is DECOMPOSED for a cutting plan: it serves printing — two pieces filling one shape on
 * a sheet — and has no meaning on fabric, where a piece is cut once. So a group contributes the
 * single member the user chose, and nothing else. Every piece outside a group is cut as it is.
 *
 * A piece the user left out of the layout is not cut. A group of fewer than two surviving members
 * is not a group at all, and its members are read as ordinary pieces.
 */
auto PiecesToCut(const QMap<quint32, quint32> &memberChoices, const VContainer *data) -> QVector<quint32>;

/// @brief True when the piece has a known direction: its own enabled grainline, its own fold line, or its group's grainline.
auto HasKnownDirection(quint32 pieceId, const VContainer *data) -> bool;

/// @brief The pieces of @ref PiecesToCut whose direction is NOT known. Empty means the plan may run.
auto FaultyPieces(const QMap<quint32, quint32> &memberChoices, const VContainer *data) -> QVector<FaultyPiece>;
} // namespace OExportCuttingGuard

#endif // OEXPORTCUTTINGGUARD_H
