/************************************************************************
 **
 **  @file   oexportalignment.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — line the sizes of a piece up before they are unioned
 **          into one footprint.
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

#ifndef OEXPORTALIGNMENT_H
#define OEXPORTALIGNMENT_H

#include <QVector>

class VContainer;
struct OExportSizeLayer;

/// @brief Lines the sizes of a piece up, so that unioning them means something.
///
/// A footprint is the union of every size of a piece. That union only makes sense if
/// the sizes sit on top of each other — and the model does not place them that way:
/// each size keeps its own coordinates, and a piece pinned at its fold and a piece
/// growing from its centre do not drift alike. Graded lines them up before drawing;
/// so does this, and for the same reason, using the same two passes and the same
/// anchoring rule (`OGraded::AnchorPoint`).
///
/// The per-piece and per-group content offsets of a range are deliberately NOT
/// applied: they move every size of a piece by the same delta, which leaves the
/// footprint's shape untouched — and its shape is all nesting looks at.
class OExportAlignment
{
public:
    /// @brief Shift every piece of @p layers so that the sizes of one piece line up.
    ///
    /// The shift is written into each piece as `alignDelta`, in pixels, and is applied
    /// where the geometry is consumed: the engine and the canvas both add it. Pieces
    /// of a group move as one rigid block, anchored on the group's own alignment.
    ///
    /// @param centreEveryPiece ignore every piece's own alignment rule and centre it.
    ///
    /// **Ce que le projecteur demande, et lui seul.** Une pièce au pli est rangée sur le côté
    /// qu'elle occupe dans le patron — à gauche ou à droite — parce que c'est ce qu'on voit quand on
    /// la trace pliée. Dépliée, la même règle la fait pendre d'un côté : les tailles s'empilent
    /// alors décalées, l'empreinte s'allonge, et la pièce projetée n'est pas centrée sur elle-même.
    /// Centrer toutes les pièces est la règle d'un plan qu'on projette — il n'y a pas de pli à
    /// border, donc pas de côté à privilégier.
    ///
    /// ⚠️ Un point d'alignement forcé prend le dessus sur la règle horizontale : centrer sans
    /// l'ignorer laisserait la pièce exactement là où elle est. Il est donc ignoré ici aussi.
    static void Apply(QVector<OExportSizeLayer> &layers, const VContainer *container,
                      bool centreEveryPiece = false);
};

#endif // OEXPORTALIGNMENT_H
