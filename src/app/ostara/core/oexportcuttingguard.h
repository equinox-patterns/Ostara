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
 * And a plan is made of one cloth: a piece whose **cut** is empty — the quantity of that cloth in the
 * Cut section of its cartouche — is not cut at all, and simply takes no part in that plan: it is not
 * a piece in fault, it is a piece that is not there. See @ref CutQuantity and @ref Material.
 *
 * This file holds no widget and no scene on purpose: it is a business rule, and a rule that can
 * be wrong has to be readable on its own.
 */
namespace OExportCuttingGuard
{
/// @brief Which cloth a plan is cut from — **the one thing that tells the two plans of a pair apart.**
///
/// A piece carries a cut quantity per material: the three tabs of the Cut section, Fabric /
/// Interfacing / Lining. A plan is laid for ONE of them, and a run may ask for two — the fabric and
/// the lining — which are then two plans over two different piece sets, each cut as many times as its
/// own quantity says.
///
/// **Interfacing is deliberately absent** : elle n'a pas de plan de coupe (décidé) — un entoilage se
/// taille dans les chutes, il ne s'achète pas à la laize. Le jour où elle en demanderait un, c'est
/// une valeur de plus ici et rien d'autre : tout ce fichier passe par cette énumération.
enum class Material
{
    Fabric,
    Lining,
};

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
 *
 * A piece with no cut — for the material asked for — is not cut either, see @ref CutQuantity. It is
 * left out HERE, which is what keeps it out of the direction guard as well: a piece the plan will not
 * lay out has no direction to be blamed for.
 */
auto PiecesToCut(const QMap<quint32, quint32> &memberChoices, const VContainer *data,
                 Material material = Material::Fabric) -> QVector<quint32>;

/**
 * @brief How many times a plan cuts this piece in @p material : its own cut quantity for that cloth,
 * or the group's when the piece leaves its own empty.
 *
 * **0 means there is no cut.** A piece nobody ever filled in reads 0, and 0 is not "cut once" — it is
 * "not cut at all", the value an untouched piece has carried since the cartouche section exists. A
 * piece may perfectly well be cut in the lining and not in the fabric, or the other way round: the
 * two quantities are read independently, and neither is a fallback for the other.
 */
auto CutQuantity(quint32 pieceId, const VContainer *data, Material material = Material::Fabric) -> int;

/// @brief True when the piece has a known direction: its own enabled grainline, its own fold line, or its group's grainline.
auto HasKnownDirection(quint32 pieceId, const VContainer *data) -> bool;

/**
 * @brief The pieces of the run's plans — one per material of @p materials — whose direction is NOT
 * known. Empty means every plan may run.
 *
 * The plans are unioned: a piece cut in both cloths is ONE piece with ONE direction, so it is named
 * once. A piece cut in the lining alone is named too, and it has to be — the lining plan would refuse
 * to lay it out for the very same reason.
 */
auto FaultyPieces(const QMap<quint32, quint32> &memberChoices, const VContainer *data,
                  const QVector<Material> &materials = {Material::Fabric}) -> QVector<FaultyPiece>;
} // namespace OExportCuttingGuard

#endif // OEXPORTCUTTINGGUARD_H
