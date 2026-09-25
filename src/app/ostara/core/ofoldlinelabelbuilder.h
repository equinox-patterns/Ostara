/************************************************************************
 **
 **  @file   ofoldlinelabelbuilder.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  La légende générique d'une foldline, dans les langues du
 **          cartouche de la pièce.
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
#ifndef OFOLDLINELABELBUILDER_H
#define OFOLDLINELABELBUILDER_H

#include <QString>

class VPiece;
class VAbstractPattern;

/**
 * @brief Ce que l'application sait dire et que la bibliothèque de dessin ne sait pas : la légende
 * générique d'une foldline, dans les langues du cartouche de la pièce.
 *
 * Une foldline qui n'a pas de texte à elle porte un texte générique. Ce texte se dit dans chaque
 * langue du cartouche, et c'est une connaissance de l'application : le cartouche, ses langues et
 * leur traduction n'appartiennent pas à la bibliothèque qui dessine la ligne. C'est pourquoi celle-ci
 * demande le texte au lieu de le composer — le même partage que pour la légende d'une miniature.
 *
 * Ce que l'utilisateur a écrit lui-même n'arrive jamais ici : un texte à soi est le texte, et la
 * bibliothèque ne le remplace pas.
 */
namespace OFoldLineLabelBuilder
{
/// @brief La légende générique de @a piece, une langue après l'autre, tenues par le même séparateur
/// que celui dont le cartouche sépare les langues d'un mot — une foldline qui porte plusieurs
/// langues se lit comme un mot du cartouche, et non comme une phrase par langue.
///
/// Rend une chaîne vide quand il n'y a rien à dire : le texte par défaut s'applique alors.
auto Caption(const VPiece &piece, VAbstractPattern *doc) -> QString;
} // namespace OFoldLineLabelBuilder

#endif // OFOLDLINELABELBUILDER_H
