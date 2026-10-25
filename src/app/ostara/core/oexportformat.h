/************************************************************************
 **
 **  @file   oexportformat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Export mode — what a generation IS, in one value.
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

#ifndef OEXPORTFORMAT_H
#define OEXPORTFORMAT_H

#include <QString>

/**
 * @brief What a generation IS — the one value its behaviour keys on.
 *
 * The mode used to answer this with booleans: `cuttingPlan` on one side, `plotterPlan` on the other,
 * and "neither" meaning print. That was already a lie one boolean at a time — `cuttingPlan` decided a
 * dozen unrelated things, which is written in the code itself — and a third format made it untenable:
 * three booleans have eight states for four formats, and every reader would have to know which
 * combination is impossible.
 *
 * So the format is ONE value, and every site asks it the question it actually has. A reader that wants
 * "does this run read the Cut quantities?" asks @ref IsCutting; one that wants "is the decor part of
 * this plan?" asks @ref CarriesDecor. Nobody re-derives a format from a pair of flags.
 *
 * ⚠️ **This is not a parameter of the run.** The gaps, the page, the laize are settings; this is the
 * nature of the thing being produced, and it is what the tabs of the dialog set when they build their
 * request. Two formats never apply to one generation.
 */
namespace OExportFormat
{
enum class Kind
{
    /// Du papier : des feuilles d'artboards, une silhouette par feuille, aucun droit-fil imposé.
    Print,

    /// Du tissu, pour couper : unités, miroir, dépliage, et **aucun décor**.
    CuttingPlan,

    /// Un plan pour un plotter domestique : des tapis, des morceaux, et le décor du modèle.
    Plotter,

    /// Une projection à l'échelle 1:1 : du tissu, comme un plan de coupe — mais **tout le décor**,
    /// une seule page, et des calques de calibration que rien n'imprime.
    Projector,
};

/// @brief La génération est faite de tissu : elle lit les quantités, déplie, et retourne ce qu'on lui
/// demande de retourner.
///
/// C'est la question que posent les six comportements autrefois rangés derrière `cuttingPlan` — voir
/// `OExportGeometry::SetFormat`. Le projecteur y répond **oui** : c'est un plan de tissu, il n'est
/// simplement pas un plan de coupe.
constexpr auto IsCutting(Kind kind) -> bool
{
    return kind == Kind::CuttingPlan || kind == Kind::Projector;
}

/// @brief Le décor du patron — cartouche, annotations, miniature — fait partie du fichier.
///
/// Un plan de coupe n'en porte aucun : il dit quoi tailler, pas à qui. **Et lui seul** : un plotter
/// découpe le patron tel que le modèle le dessine, et un projecteur projette le patron, cartouche
/// compris.
constexpr auto CarriesDecor(Kind kind) -> bool
{
    return kind != Kind::CuttingPlan;
}

/// @brief Le projecteur — une projection à l'échelle 1:1, et rien d'autre.
///
/// La question se pose là où le format n'est ni de l'impression ni de la coupe : **les groupes
/// n'existent pas pour lui**. Chacun de ses membres est une pièce classique — sa propre empreinte, son
/// propre décor, son propre cartouche — alors qu'un plan de coupe garde le membre choisi et qu'un
/// plotter décompose par ses morceaux de tapis.
///
/// C'est aussi le seul format qui tienne sur une page, à l'échelle 1:1, sans tuilage : une projection
/// n'a ni feuille ni tuile, elle a une page tenue sous le plafond d'Acrobat (`Projector.md` §3).
constexpr auto IsProjector(Kind kind) -> bool
{
    return kind == Kind::Projector;
}

/// @brief Le nom du format dans un fichier de favoris. **Jamais affiché** : c'est une clé.
auto Token(Kind kind) -> QString;

/// @brief Le format que ce jeton nomme, ou @p fallback quand le jeton est vide ou inconnu.
///
/// Le repli n'est pas une politesse : un fichier de favoris écrit par une version antérieure ne porte
/// aucun jeton, et le lire comme une impression serait perdre le plan de coupe qu'il enregistrait.
/// L'appelant qui lit un vieux record passe donc les deux anciens drapeaux à @ref FromLegacyFlags.
auto FromToken(const QString &token, Kind fallback = Kind::Print) -> Kind;

/// @brief Le format d'un enregistrement écrit avant que le format ne soit une valeur.
///
/// `cuttingPlan` et `plotterPlan` étaient deux booléens, et leur lecture d'alors donnait la priorité
/// au plotter. C'est celle qu'on reprend ici, à l'identique, pour qu'un vieux favori se rouvre sur le
/// même plan qu'avant.
auto FromLegacyFlags(bool cuttingPlan, bool plotterPlan) -> Kind;
} // namespace OExportFormat

#endif // OEXPORTFORMAT_H
