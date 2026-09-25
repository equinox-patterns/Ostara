/************************************************************************
 **
 **  @file   otextwrap.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  Repli d'un texte à une largeur donnée — un seul algorithme
 **          pour toutes les surfaces qui posent du texte sur un dessin.
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
#ifndef OTEXTWRAP_H
#define OTEXTWRAP_H

#include <QString>
#include <QStringList>

#include <functional>

class QFont;
class VSvgFontEngine;

/**
 * @brief Le repli du texte, écrit une fois.
 *
 * Deux surfaces ne mesurent pas le texte de la même façon — une police Qt se mesure avec ses
 * métriques, une police de tracé avec son moteur — mais la règle de repli, elle, ne change pas.
 * L'appelant dit donc *comment* sa surface mesure, et l'algorithme reste ici, à un seul endroit.
 */
namespace OTextWrap
{
/**
 * @brief Comment une surface mesure le texte.
 *
 * Deux fonctions et non une seule, parce qu'une surface ne mesure pas toujours un caractère comme
 * elle mesure un mot : le chemin « contour de police » additionne les avances caractère par
 * caractère pour un mot, mais coupe un mot trop long en interrogeant les métriques brutes.
 *
 * @a word mesure un mot entier, @a part un caractère ou un fragment de mot coupé.
 */
struct Measure
{
    std::function<qreal(const QString &)> word{};
    std::function<qreal(const QString &)> part{};
};

/**
 * @brief Lines replie @a text à @a maxWidth, en coupant entre les mots.
 *
 * Un mot plus large que @a maxWidth est coupé, et chaque ligne est remplie aussi loin qu'elle le
 * peut avant que la suivante ne s'ouvre : le texte prend **autant de lignes qu'il en a besoin**,
 * jamais moins, et rien n'est tronqué.
 *
 * Un saut de ligne écrit dans @a text est une ligne forcée et n'est jamais recollé : c'est ainsi
 * qu'on met une langue sous une autre.
 *
 * @param text le texte à replier
 * @param maxWidth la largeur disponible, dans l'unité que rend @a measure ; <= 0 signifie
 *                 « pas de limite », et le texte tient alors sur une seule ligne
 * @param measure comment la surface mesure le texte
 * @return les lignes, dans l'ordre de lecture ; liste vide seulement si @a text est vide
 */
auto Lines(const QString &text, qreal maxWidth, const Measure &measure) -> QStringList;

/// @brief La mesure d'un texte dessiné avec @a font.
auto FontMeasure(const QFont &font) -> Measure;

/// @brief La mesure d'un texte dessiné par un moteur de police de tracé, à l'épaisseur @a penWidth.
auto EngineMeasure(const VSvgFontEngine &engine, qreal penWidth) -> Measure;

/**
 * @brief SplitByWidth coupe en au plus deux fragments un mot que @a maxWidth ne peut pas tenir.
 *
 * Rend une liste vide quand aucun point de coupure n'a été trouvé — l'appelant lit ce vide comme
 * « garde le mot entier », et non comme « ce mot disparaît ». C'est la règle du moteur d'origine,
 * conservée telle quelle : un mot court toujours plus large que la place disponible doit rester
 * lisible, quitte à déborder.
 */
auto SplitByWidth(const QString &text, qreal maxWidth, const Measure &measure) -> QStringList;
} // namespace OTextWrap

#endif // OTEXTWRAP_H
