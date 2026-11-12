/************************************************************************
 **
 **  @file   opiececontentdigest.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12 November, 2026
 **
 **  @brief  What a piece DRAWS, in one string.
 **
 **          Not everything a piece carries is visible in its outline: a grainline, an annotation or
 **          a cartouche move without changing it, while a seam allowance, a notch or an internal path
 **          do. That difference is what tells a canvas it can keep its drawing, and what tells a
 **          miniature it has nothing new to capture — either way, a pass that would have cost a full
 **          re-parse of the document.
 **
 **          One definition, two readers: the Graded canvas (which rebuilds its scene only when the
 **          content behind it changed) and the miniature scheduling (which used to re-parse the whole
 **          pattern after every edit, a grainline nudge included).
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

#ifndef OPIECECONTENTDIGEST_H
#define OPIECECONTENTDIGEST_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "../../../libs/vpatterndb/vpiece.h"
#include "../../../libs/vpatterndb/vpiecenode.h"

namespace OPieceContentDigest
{
/**
 * @brief The geometry a piece draws, as one comparable string.
 *
 * Deliberately NOT the whole piece: the name, the grainline, the cartouche, the annotations, the
 * miniature frame and the markers are left out — they change what is written beside the piece, never
 * the shape it draws.
 */
inline auto Of(const VPiece &piece) -> QString
{
    QStringList parts;
    // A piece shown mirrored draws the same shape the other way round. The canvas needs to know (its
    // scene is rebuilt from this), the miniature's drawing too — it captures the mirrored outline.
    parts.append(QStringLiteral("sa=%1%2%3%4")
                     .arg(piece.IsSeamAllowance() ? 1 : 0)
                     .arg(piece.IsHideMainPath() ? 1 : 0)
                     .arg(piece.IsShowFullPiece() ? 1 : 0)
                     .arg(piece.IsForceFlipping() ? 1 : 0));
    parts.append(QStringLiteral("saw=%1").arg(piece.GetFormulaSAWidth()));

    for (const CustomSARecord &record : piece.GetCustomSARecords())
    {
        parts.append(QStringLiteral("csa=%1,%2,%3,%4,%5")
                         .arg(record.startPoint)
                         .arg(record.path)
                         .arg(record.endPoint)
                         .arg(record.reverse ? 1 : 0)
                         .arg(static_cast<int>(record.includeType)));
    }

    QVector<quint32> internalPaths = piece.GetInternalPaths();
    std::sort(internalPaths.begin(), internalPaths.end());
    for (quint32 id : internalPaths)
    {
        parts.append(QStringLiteral("ip=%1").arg(id));
    }

    QVector<quint32> placeLabels = piece.GetPlaceLabels();
    std::sort(placeLabels.begin(), placeLabels.end());
    for (quint32 id : placeLabels)
    {
        parts.append(QStringLiteral("pl=%1").arg(id));
    }

    // Mounting notches ("repère de montage") live on the main-path nodes (IsPassmark).
    const QVector<VPieceNode> nodes = piece.GetPath().GetNodes();
    for (const VPieceNode &node : nodes)
    {
        if (node.IsPassmark())
        {
            parts.append(QStringLiteral("nm=%1").arg(node.GetId()));
        }
    }

    return parts.join(QLatin1Char('|'));
}
} // namespace OPieceContentDigest

#endif // OPIECECONTENTDIGEST_H
