/************************************************************************
 **
 **  @file   oexportcuttingguard.cpp
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

#include "oexportcuttingguard.h"

#include <algorithm>

#include "../ogradedgeometry.h"
#include "../vmisc/typedef.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingGuard::PiecesToCut(const QMap<quint32, quint32> &memberChoices, const VContainer *data)
    -> QVector<quint32>
{
    QVector<quint32> pieces;
    if (data == nullptr || data->DataPieces() == nullptr)
    {
        return pieces;
    }

    for (auto it = data->DataPieces()->constBegin(); it != data->DataPieces()->constEnd(); ++it)
    {
        const quint32 pieceId = it.key();

        // The same filter Details and Layout apply: a piece the user left out of the layout is
        // not cut.
        if (not data->IsPieceInLayout(pieceId))
        {
            continue;
        }

        const quint32 groupId = data->GetPieceGroupId(pieceId);
        if (groupId == NULL_ID)
        {
            pieces.append(pieceId); // outside a group, cut as it is
            continue;
        }

        if (memberChoices.value(groupId, NULL_ID) == pieceId)
        {
            pieces.append(pieceId); // the member the user chose for that group
        }
    }

    return pieces;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingGuard::HasKnownDirection(quint32 pieceId, const VContainer *data) -> bool
{
    if (data == nullptr || data->DataPieces() == nullptr || not data->DataPieces()->contains(pieceId))
    {
        return false;
    }

    const VPiece piece = data->GetPiece(pieceId);

    // 1. The piece's own grainline, when it is switched on. IsEnabled says the direction was
    //    given; IsVisible only says whether it is drawn, and a direction that is not shown is
    //    still a direction.
    if (piece.GetGrainlineGeometry().IsEnabled())
    {
        return true;
    }

    // 2. The piece's own fold line: its axis is the grain.
    //
    //    What says a piece HAS one is the line itself, never its type: FoldLineType's default is
    //    TwoArrowsTextAbove and not None, so "the type is not None" is true of every piece ever
    //    created, fold line or not. The canvas asks the same way — a mirror line that is null is
    //    no mirror line.
    if (not piece.SeamAllowanceMirrorLine(data).isNull())
    {
        return true;
    }

    // 3. The group. A member carries no grainline of its own — the group hosts one for all of
    //    them — so the group is read before the piece is called faulty.
    const auto *groups = data->DataPieceGroups();
    const quint32 groupId = data->GetPieceGroupId(pieceId);
    if (groups == nullptr || groupId == NULL_ID || not groups->contains(groupId))
    {
        return false;
    }

    const OPieceGroup group = groups->value(groupId);
    if (group.pieceIds.size() < OGraded::kMinGroupMembers)
    {
        // Not a group at all, so it hosts nothing: its members are read as ordinary pieces.
        return false;
    }

    const VGrainlineData &grainline = group.m_glGrainline;
    return grainline.IsEnabled() && grainline.IsVisible();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingGuard::FaultyPieces(const QMap<quint32, quint32> &memberChoices, const VContainer *data)
    -> QVector<FaultyPiece>
{
    QVector<FaultyPiece> faulty;

    for (quint32 pieceId : PiecesToCut(memberChoices, data))
    {
        if (not HasKnownDirection(pieceId, data))
        {
            const VPiece piece = data->GetPiece(pieceId);
            faulty.append(FaultyPiece{pieceId, piece.GetPieceNumber(), piece.GetName()});
        }
    }

    // In the order the user reads her pieces: the number she gives a piece in Details.
    std::stable_sort(faulty.begin(), faulty.end(),
                     [](const FaultyPiece &left, const FaultyPiece &right) { return left.number < right.number; });
    return faulty;
}
