/************************************************************************
 **
 **  @file   oexportnesting.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — feed the Trame nesting engine with the pattern's
 **          own geometry and bring back the placements.
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

#include "oexportnesting.h"

#include "oexportartboards.h"
#include "oexportgeometry.h"
#include "oexportsheets.h"

#include "../vlayout/vlayoutpoint.h"

#include "model/Piece.h"
#include "nesting/Bin.h"
#include "nesting/Engine.h"
#include "nesting/Footprint.h"
#include "nesting/TiledEngine.h"

#include "../vmisc/def.h"

#include <QObject>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace
{
/// The engine works in fixed-point: one pixel is kTrameScale units. Every
/// coordinate handed to it, and every coordinate it returns, uses that scale.
constexpr double kTrameScale = 1e6;

/// Le centième de pixel d'indulgence des comparaisons de largeur : une somme juste à l'arrondi
/// près ne doit pas fermer la porte au double bord, ni faire échouer un déplacement qui tient.
constexpr double kWideningPx = 0.01;

/// @param offset the piece's alignment shift, in pixels — the same one the canvas
///               applies, so the bin and the screen show the same geometry.
auto ToPath64(const QVector<VLayoutPoint> &points, const QPointF &offset) -> Clipper2Lib::Path64
{
    Clipper2Lib::Path64 path;
    path.reserve(static_cast<size_t>(points.size()));

    for (const VLayoutPoint &point : points)
    {
        path.push_back({static_cast<int64_t>(std::llround((point.x() + offset.x()) * kTrameScale)),
                        static_cast<int64_t>(std::llround((point.y() + offset.y()) * kTrameScale))});
    }

    return path;
}

/// Every piece placed. The engine marks an unplaced one with a negative
/// rotation and reports nothing else — no status, no exception.
auto AllPlaced(const std::vector<Placement> &placements) -> bool
{
    for (const Placement &p : placements)
    {
        if (p.rotation < 0.0)
        {
            return false;
        }
    }
    return true;
}

/// The engine's raw placements, turned into what the mode consumes.
///
/// The footprint's own frame: the min corner gives the origin, and the max
/// corner gives the extent. Both come from the polygon the engine actually
/// placed — never from the layers, which would be a second union and not
/// necessarily the same one.
auto ToExportPlacements(const std::vector<Placement> &placements,
                        const std::vector<Clipper2Lib::Path64> &polygons)
    -> QVector<OExportPlacement>
{
    QVector<OExportPlacement> out;
    out.reserve(static_cast<int>(placements.size()));

    for (size_t i = 0; i < placements.size(); ++i)
    {
        int64_t xMin = polygons[i].front().x;
        int64_t yMin = polygons[i].front().y;
        int64_t xMax = xMin;
        int64_t yMax = yMin;
        for (const auto &point : polygons[i])
        {
            xMin = std::min(xMin, point.x);
            yMin = std::min(yMin, point.y);
            xMax = std::max(xMax, point.x);
            yMax = std::max(yMax, point.y);
        }

        OExportPlacement placement;
        placement.x = placements[i].x;
        placement.y = placements[i].y;
        placement.rotation = placements[i].rotation;
        placement.originX = static_cast<double>(xMin) / kTrameScale;
        placement.originY = static_cast<double>(yMin) / kTrameScale;
        placement.extentX = static_cast<double>(xMax - xMin) / kTrameScale;
        placement.extentY = static_cast<double>(yMax - yMin) / kTrameScale;

        // The outline, brought into the frame the extent describes: origin at local (0, 0) and
        // pixels. It is what a hand's limits are measured against — see the note on the field.
        placement.outline.reserve(static_cast<int>(polygons[i].size()));
        for (const auto &point : polygons[i])
        {
            placement.outline.append(QPointF(static_cast<double>(point.x - xMin) / kTrameScale,
                                             static_cast<double>(point.y - yMin) / kTrameScale));
        }

        out.append(placement);
    }

    return out;
}

/// How many sheets a plan prints.
///
/// The rule belongs to OSTARA, not to Trame: `Bin.h` says the grid is not part
/// of the bin model, and the count is read off the placements by the one
/// function that counts them. Trame is asked for two plans and told nothing
/// about pages — it is here, and only here, that they are compared.
auto PrintedSheets(const QVector<OExportPlacement> &placements,
                   double usedHeightPx, const OExportNestingParams &params) -> int
{
    const double mmPerPx = 1.0 / ToPixel(1.0, Unit::Mm);

    const OExportArtboards::Grid grid =
        OExportArtboards::Compute(params.binWidthPx, usedHeightPx,
                                  params.artboardHeightPx * mmPerPx,
                                  params.artboardCount);

    if (!grid.valid)
    {
        return (std::numeric_limits<int>::max)();
    }

    // The cover page's reserved block is handed over like the grid itself, and it is the SAME block
    // for both candidate plans: it is therefore a constant in the comparison below, and the engine's
    // choice can only move through the placements — which is what it is there for.
    return OExportSheets::Compute(grid, placements, params.coverZonePx).count;
}

// ─────────────────────────────────────────────────────────────────────────────
// Le tissu plié à deux bords — trois rectangles, et deux pliages en concurrence
// ─────────────────────────────────────────────────────────────────────────────

/// Les trois rectangles d'un tissu plié à deux bords.
///
/// Les deux bandes du haut portent chacune un pli sur son bord EXTÉRIEUR — c'est ce que fait un
/// tissu dont les deux lisières sont repliées vers l'intérieur — et la ligne qui les sépare est
/// celle où les deux lisières se rejoignent. Le rectangle du bas est la colonne entière, avec un
/// seul pli : c'est là que va ce qui ne rentre dans aucune bande.
enum ZoneSeat
{
    kLeftZone = 0,
    kRightZone = 1,
    kBelowZone = 2,
};

/// Ce qu'une pièce dit au découpage : si elle est au pli, ses angles, et sa largeur.
struct PieceTalk
{
    bool onFold{false};
    std::vector<double> angles{}; ///< les angles de sa direction — jamais vide sur un plan de coupe
    double foldAngle{0.0};        ///< sa rotation au pli, dans son bon sens

    /// Le côté que sa rotation au pli appelle : vrai quand il faut un pli sur le bord DROIT.
    /// **C'est une donnée du patron**, pas un choix : la pièce est dessinée pour un pli, et la
    /// retourner la met à l'envers — sur un tissu à motif, c'est un plan faux.
    bool foldOnTheRight{false};

    double widthPx{0.0}; ///< sa largeur la plus étroite — son « rectangle »

    /// Vrai pour la variante EN MIROIR d'une pièce au pli — celle qui ouvre l'autre bord.
    bool mirrored{false};

    QString name{}; ///< pour nommer la pièce quand aucun pliage ne peut la porter
    quint32 id{0};
};

/// La largeur d'un rang — pour la recherche, qui raisonne par rang et non par pose.
auto talkWidth(const QVector<PieceTalk> &talks, int rank) -> double
{
    return rank >= 0 && rank < talks.size() ? talks.at(rank).widthPx : 0.0;
}

/// Une pose au pli : **ses deux variantes**, dont une seule sera posée.
///
/// La pièce est dessinée pour un bord ; son miroir est la même pièce retournée, qui va sur l'autre.
/// Les deux sont des empreintes distinctes, avec des clés distinctes, et le nesting n'en garde
/// qu'une : celle dont le côté correspond au rectangle où la pose atterrit. L'autre n'entre ni dans
/// le résultat ni dans le dessin.
struct FoldPose
{
    int natural{-1}; ///< le rang de la pièce telle qu'elle est dessinée
    int mirror{-1};  ///< le rang de son miroir, ou -1 quand elle n'en a pas
};

/// Le rang à poser dans un rectangle dont le pli est de ce côté — la variante qui va de ce côté.
///
/// ⚠️ Le côté se lit sur la variante elle-même, jamais sur une convention : `foldOnTheRight` dit
/// qu'il faut un pli sur le bord droit, donc ce rectangle-là ne peut la porter que si son pli y est.
auto VariantFor(const QVector<PieceTalk> &talks, const FoldPose &pose, bool foldOnTheLeft) -> int
{
    const auto suits = [&talks, foldOnTheLeft](int rank) {
        return rank >= 0 && talks.at(rank).foldOnTheRight != foldOnTheLeft;
    };

    if (suits(pose.natural))
    {
        return pose.natural;
    }
    return suits(pose.mirror) ? pose.mirror : -1;
}

/// Ce qu'un rectangle demande à une pièce : les angles qu'elle y prend, et la paroi où elle colle.
struct ZoneConstraint
{
    std::vector<double> angles{};
    Anchor anchor{Anchor::Free};
};

/// Une nidification : un sous-ensemble d'empreintes, et ce que le moteur en a fait.
struct ZonePlan
{
    std::vector<int> ranks{};                 ///< les rangs du footprint, dans l'ordre des placements
    std::vector<Clipper2Lib::Path64> polygons{}; ///< parallèle à @ref ranks — ce que le moteur a reçu
    std::vector<Placement> placements{};      ///< parallèle à @ref ranks
    double usedHeightPx{0.0};
};

/// La largeur la plus étroite d'un chemin, sur les angles qu'on lui donne.
///
/// **C'est le « rectangle » d'une pièce**, et c'est lui qui décide si elle rentre dans une bande :
/// elle n'y entre que si l'un de ses angles y tient. Prendre le plus étroit est le bon test — le
/// moteur essaiera les autres, et s'il n'y arrive pas il le dit (pièce non placée) plutôt que de
/// la laisser dépasser.
///
/// ⚠️ Les angles sont ceux du MOTEUR : c'est `NestGeom::rotatePath` qui tourne, le même que celui
/// qui posera la pièce. Un signe faux ici ne se verrait nulle part ailleurs — la pièce serait
/// simplement jugée trop large pour une bande où elle entrait.
auto NarrowestWidth(const Clipper2Lib::Path64 &path, const std::vector<double> &angles) -> double
{
    if (path.empty())
    {
        return 0.0;
    }

    bool any = false;
    double narrowest = 0.0;
    for (const double angle : angles)
    {
        const BBox box = NestGeom::bboxFromPath(NestGeom::rotatePath(path, angle));
        const double width = static_cast<double>(box.xMax - box.xMin) / kTrameScale;
        if (!any || width < narrowest)
        {
            narrowest = width;
            any = true;
        }
    }

    // Aucun angle : un plan de coupe en donne toujours. Un appelant qui n'en donnerait pas ne doit
    // pas lire une largeur nulle — la pièce ne rentrerait nulle part.
    if (!any)
    {
        const BBox box = NestGeom::bboxFromPath(path);
        narrowest = static_cast<double>(box.xMax - box.xMin) / kTrameScale;
    }

    return narrowest;
}

/// Les contraintes qu'un rectangle impose aux pièces qu'il porte.
///
/// ⚠️ **C'est ici que le pli l'emporte sur la grainline**, et c'est le seul endroit où ça se dit :
/// une pièce coupée au pli est portée par le pli du tissu, pas par son droit-fil. Ailleurs — un plan
/// déplié — la grainline reste la direction, et la pièce est dépliée.
auto ConstraintsFor(const QVector<PieceTalk> &talks, const std::vector<int> &ranks,
                    bool foldOnTheLeft) -> std::vector<ZoneConstraint>
{
    std::vector<ZoneConstraint> out;
    out.reserve(ranks.size());

    for (const int rank : ranks)
    {
        const PieceTalk &talk = talks.at(rank);
        ZoneConstraint constraint;
        if (talk.onFold)
        {
            // ⚠️ Le rectangle ne choisit pas la rotation : elle est celle de la pièce dans son bon
            // sens. Le côté, lui, doit correspondre — c'est la répartition qui le garantit, et une
            // pièce au pli du mauvais côté serait posée à l'envers.
            constraint.angles = {talk.foldAngle};
            constraint.anchor = foldOnTheLeft ? Anchor::LeftWall : Anchor::RightWall;
        }
        else
        {
            constraint.angles = talk.angles;
        }
        out.push_back(std::move(constraint));
    }

    return out;
}


/// Le message d'un plan qui ne rentre pas — **et il a désormais deux causes, dont une seule est
/// nommée par le moteur.**
///
/// Un espace depuis les bords trop grand rétrécit le bac sans que la laize y soit pour quoi que ce
/// soit. Le moteur a raison de dire « ça ne rentre pas » : il ignore qu'un espace a été demandé.
/// L'appelant, lui, le sait — d'où cette phrase, écrite ici une fois pour les deux chemins. Sans
/// elle, la seule piste laissée à l'utilisatrice est sa laize, qui n'y est pour rien.
auto TooNarrowError(const OExportNestingParams &params) -> QString
{
    if (params.marginPx > 0.0)
    {
        return QObject::tr("The document is too narrow: some pieces could not be placed. "
                           "The space kept from the fabric's edges may be what leaves too little "
                           "room.");
    }

    return QObject::tr("The document is too narrow: some pieces could not be placed.");
}

/// Nidifie les rangs d'un plan dans un bac de largeur @p widthPx.
///
/// Rend faux quand le moteur a été interrompu — et pose @p abandoned, l'appelant devant alors tout
/// jeter : une nidification à moitié faite n'est pas un plan — ou quand le bac ne peut pas tout
/// porter, avec le message qui le dit.
auto NestRanks(const Footprint::Result &footprint, const std::vector<ZoneConstraint> &constraints,
               double widthPx, const Bin::Margins &margins, const OExportNestingParams &params,
               const std::vector<Bin::Zone> &zones, const Engine::Config &config, ZonePlan *plan,
               QString *error, bool *abandoned) -> bool
{
    plan->polygons.clear();
    std::vector<std::vector<double>> angles;
    std::vector<Anchor> anchors;
    plan->polygons.reserve(plan->ranks.size());
    angles.reserve(plan->ranks.size());
    anchors.reserve(plan->ranks.size());

    for (std::size_t i = 0; i < plan->ranks.size(); ++i)
    {
        const std::size_t rank = static_cast<std::size_t>(plan->ranks.at(i));
        plan->polygons.push_back(footprint.polygons.at(rank));
        angles.push_back(constraints.at(i).angles);
        anchors.push_back(constraints.at(i).anchor);
    }

    // Le bac, un par rectangle : `Bin` prend ses zones par valeur, donc chacun a la sienne — et
    // les trois voient le même bloc interdit. **Ses marges viennent de l'appelant**, parce qu'elles
    // diffèrent d'un rectangle à l'autre : le bord d'un pli ne paie rien. Voir `SeatMargins`.
    const Bin bin(widthPx, margins, zones);

    bool stopped = false;
    plan->placements = Engine{config}.place(bin, plan->polygons, angles, anchors, &stopped);

    if (stopped)
    {
        *abandoned = true;
        return false;
    }

    if (plan->placements.size() != plan->polygons.size())
    {
        *error = QObject::tr("The nesting engine returned an incomplete result.");
        return false;
    }

    if (!AllPlaced(plan->placements))
    {
        *error = TooNarrowError(params);
        return false;
    }

    plan->usedHeightPx = Engine::UsedHeight(plan->placements, plan->polygons, bin);
    return true;
}

/// Un rectangle du plan plié : son côté de pli, sa largeur, et ce qu'il laisse libre sur ses bords.
///
/// **Les trois vont ensemble** parce qu'ils répondent à une seule question — *où une pièce peut-elle
/// aller dans ce rectangle ?* — et c'est de les avoir tenus séparés que venait le défaut : une pièce
/// jugée déplaçable d'après la largeur BRUTE d'un rectangle que le moteur refusait ensuite de
/// remplir.
struct Seat
{
    bool foldOnTheLeft{true};
    double widthPx{0.0};
    Bin::Margins margins{};

    /// La largeur où une pièce peut réellement aller.
    auto usableWidthPx() const -> double { return widthPx - margins.left - margins.right; }
};

/// Les marges d'un rectangle plié : **trois côtés qui paient, et le pli qui ne paie rien.**
///
/// **Le bord du pli vaut zéro, et c'est une contrainte, pas un choix.** Une pièce au pli a sa
/// foldline SUR le pli — `ConstraintsFor` l'ancre à la paroi, et la paroi EST la ligne de marge —
/// donc une marge de `m` de ce côté l'en écarte de `m`, et la pièce dépliée sort trop étroite de
/// `2 m`. C'est la règle de la spec §18.2, et **le moteur ne peut pas la vérifier** : il ne connaît
/// pas les plis. C'est ce helper, et lui seul, qui la pose — d'où le soin à mettre dans ces quatre
/// lignes.
///
/// **Les trois autres paient `m` sans exception, le haut et le bas compris.** Deux rectangles
/// empilés paient donc chacun la ligne qui les sépare, et la bande vide entre eux vaut `2 m` : c'est
/// voulu, et c'est la règle telle qu'elle a été donnée — *« pour chaque rectangle, on applique la
/// marge sur trois côtés, donc tous les côtés sauf le pli »*. Un plan se lit comme une pile de
/// rectangles, et chacun garde son espace sur tout ce qui l'entoure — sans quoi la bande vide
/// manquerait précisément là où l'œil la cherche, entre deux rectangles dessinés côte à côte.
///
/// ⚠️ **Et ça ne passe PAS par `Bin::Zone`**, si tentant que ce soit : `NestZone::Build` gonfle
/// chaque zone du jeu ENTIER, donc une zone qui dirait « garde `m` du bord » garderait `m + gap`.
/// Deux règles différentes, deux endroits différents.
auto SeatMargins(bool foldOnTheLeft, double edgePx) -> Bin::Margins
{
    Bin::Margins out;
    out.left = foldOnTheLeft ? 0.0 : edgePx;
    out.right = foldOnTheLeft ? edgePx : 0.0;
    out.top = edgePx;
    out.bottom = edgePx;
    return out;
}

/// Le plan d'un tissu plié — **deux pliages en concurrence, et le plus court gagne.**
///
/// **Un bord** : la colonne entière, le pli sur un seul grand côté. C'est le plan validé, et c'est
/// aussi ce que donne un tissu dont les deux plus larges pièces au pli ne tiennent pas côte à côte.
///
/// **Deux bords** : deux bandes dont les plis sont les DEUX grands côtés de la pile, et sous elles
/// la colonne. Le partage des bandes est une **règle**, pas une recherche : chacune prend la largeur
/// de sa pièce au pli, et **le surplus est partagé également**. Comme le surplus est positif,
/// chaque bande est forcément au moins aussi large que la pièce qu'elle porte.
///
/// Puis c'est le **nesting qui choisit** où vont les autres pièces : il essaie de les déplacer,
/// renidifie les rectangles touchés, et garde le déplacement quand la hauteur totale baisse. La
/// hauteur gardée est donc **mesurée**, jamais estimée. ⚠️ Les deux pièces au pli des bandes, elles,
/// ne bougent pas : ce sont elles qui justifient le double bord.
///
/// ⚠️ **Le chevauchement est impossible par construction** : chaque rectangle est un bac à part, et
/// le moteur ne pose jamais hors de son bac. La règle dure ne coûte pas une ligne.
auto PlanFoldedFabric(const Footprint::Result &footprint, const QVector<PieceTalk> &talks,
                      const OExportNestingParams &params, const std::vector<Bin::Zone> &zones,
                      const Engine::Config &config, QVector<OExportPlacement> *placements,
                      std::vector<int> *orderedRanks, QVector<OExportFoldedZone> *foldedZones,
                      double *usedHeightPx, QString *error, bool *abandoned) -> bool
{
    const double columnPx = params.binWidthPx;
    // ── Les poses au pli, et leurs deux variantes ──────────────────────────────────────────────
    //
    // Une pièce au pli arrive en DEUX empreintes : elle-même, et son miroir. Une seule sera posée —
    // celle dont le côté correspond au rectangle où la pose atterrit. **Le miroir est ce qui ouvre
    // les deux bords** : sans lui, une pièce dessinée pour la gauche ne pourrait jamais entrer dans
    // la bande de droite, et tout ce qui est du mauvais côté finirait sous les bandes, en hauteur
    // perdue. Le côté reste la nature de la pièce ; il cesse d'être une contrainte.
    QVector<FoldPose> poses;
    QVector<int> poseOfRank(talks.size(), -1);
    QHash<QString, int> poseOfKey;
    for (int rank = 0; rank < talks.size(); ++rank)
    {
        if (!talks.at(rank).onFold)
        {
            continue;
        }

        // ⚠️ **Le rattachement ne suppose pas l'ordre.** Les deux variantes sortent d'une même
        // boucle, mais `Footprint::compute` regroupe et retrie : rien ne dit que la pièce passe
        // avant son miroir — elles ont la même aire, et c'est la clé qui tranche. Chaque clé trouve
        // ou crée sa pose, et la seconde des deux la complète.
        const QString &key = footprint.keys.at(static_cast<std::size_t>(rank));
        const QString poseKey = talks.at(rank).mirrored && key.endsWith(QStringLiteral("-m"))
                                    ? key.left(static_cast<int>(key.size()) - 2)
                                    : key;

        int pose = poseOfKey.value(poseKey, -1);
        if (pose < 0)
        {
            pose = static_cast<int>(poses.size());
            poses.append(FoldPose{});
            poseOfKey.insert(poseKey, pose);
        }

        if (talks.at(rank).mirrored)
        {
            poses[pose].mirror = rank;
        }
        else
        {
            poses[pose].natural = rank;
        }
        poseOfRank[rank] = pose;
    }

    // La largeur d'une pose : la plus large de ses deux variantes — le miroir donne la même, mais
    // le lire plutôt que le supposer coûte une ligne.
    const auto widthOfPose = [&talks](const FoldPose &pose) {
        return std::max(pose.natural >= 0 ? talks.at(pose.natural).widthPx : 0.0,
                        pose.mirror >= 0 ? talks.at(pose.mirror).widthPx : 0.0);
    };

    std::vector<int> posesByWidth(static_cast<std::size_t>(poses.size()));
    for (int pose = 0; pose < poses.size(); ++pose)
    {
        posesByWidth[static_cast<std::size_t>(pose)] = pose;
    }
    std::sort(posesByWidth.begin(), posesByWidth.end(), [&poses, &widthOfPose](int a, int b) {
        return widthOfPose(poses.at(a)) > widthOfPose(poses.at(b));
    });

    // ── Les deux pliages possibles ────────────────────────────────────────────────────────────
    //
    // **Un bord** : une colonne, et son pli se met d'un côté — lequel n'importe plus, puisque le
    // miroir ouvre l'autre à toutes les poses. Il concourt toujours.
    //
    // **Deux bords** : deux bandes dont les plis sont les deux grands côtés de la pile, donc **les
    // deux plus larges poses au pli**, une par bande. Le surplus est partagé également : chaque
    // bande prend la largeur de sa pose, plus la moitié de ce qui reste. Le centième de pixel
    // d'indulgence évite qu'une somme juste à l'arrondi près ferme la porte au double bord.
    //
    // ⚠️ **Et chaque bande paie son espace depuis les bords.** Elle a un pli — qui ne coûte rien,
    // sa marge est nulle — et une lisière, qui coûte `m` : d'où les `2 m` ci-dessous, écrits une
    // seule fois, ici, avant que les rectangles existent.
    //
    // Ce n'est pas une précaution mais l'inégalité qui rend le partage juste, et elle se démontre.
    // Le partage donne `leftWidth = w_A + surplus/2`, soit `(columnPx + w_A − w_B)/2` — donc la
    // largeur UTILE de la bande gauche vaut `leftWidth − m = w_A + surplus/2 − m`, et elle garde la
    // largeur de sa pose si et seulement si `surplus/2 ≥ m`, c'est-à-dire `w_A + w_B + 2m ≤
    // columnPx` : exactement ce que la porte achète. Idem à droite, par symétrie.
    const double bandsEdgeCostPx = 2.0 * params.marginPx;

    const bool doubleEdge =
        poses.size() >= 2
        && widthOfPose(poses.at(posesByWidth.at(0))) + widthOfPose(poses.at(posesByWidth.at(1)))
               + bandsEdgeCostPx
               <= columnPx + kWideningPx;

    // L'affectation se lit par RANG, mais **se décide par pose** : les deux variantes d'une pose
    // vont toujours au même rectangle, et c'est celle du bon côté qui y est posée. Une pose n'est
    // donc jamais coupée deux fois.
    std::vector<int> zoneOfRank(static_cast<std::size_t>(talks.size()), kBelowZone);

    double bestHeight = std::numeric_limits<double>::max();
    bool bestIsDouble = false;
    ZonePlan best[3];
    double leftWidth = 0.0;
    double rightWidth = 0.0;

    // Nidifie un candidat : ses rectangles — leur côté de pli, leur largeur, leurs marges — et la
    // pose de chaque rang.
    const auto evaluate = [&](const std::vector<Seat> &seats, const std::vector<int> &assignment,
                              ZonePlan *plans, double *height) -> bool {
        for (std::size_t seat = 0; seat < seats.size(); ++seat)
        {
            const Seat &here = seats.at(seat);
            std::vector<int> ranks;
            for (int rank = 0; rank < talks.size(); ++rank)
            {
                if (assignment.at(static_cast<std::size_t>(rank)) != static_cast<int>(seat))
                {
                    continue;
                }

                if (talks.at(rank).onFold)
                {
                    // Une pose : seule la variante du côté de ce rectangle entre dans ce bac.
                    const int pose = poseOfRank.at(rank);
                    if (pose < 0 || rank != VariantFor(talks, poses.at(pose), here.foldOnTheLeft))
                    {
                        continue;
                    }
                }

                ranks.push_back(rank);
            }

            plans[seat].ranks = ranks;
            if (!NestRanks(footprint, ConstraintsFor(talks, ranks, here.foldOnTheLeft), here.widthPx,
                           here.margins, params, zones, config, &plans[seat], error, abandoned))
            {
                return false;
            }
        }

        // La hauteur de la colonne : les bandes côte à côte, puis ce qui est dessous. Un seul
        // rectangle — le bord unique — et c'est simplement sa hauteur.
        //
        // ⚠️ **Un rectangle resté vide ne doit rien coûter.** `Engine::UsedHeight` d'un plan vide
        // rend sa marge BASSE — et depuis que les trois côtés paient, tout rectangle en a une, y
        // compris ceux du haut. Sans ce test, un plan dont rien n'est descendu sous les bandes
        // finirait sur `m` de tissu que personne n'utilise, et que le canvas ne dessine même pas :
        // un rectangle vide ne s'écrit pas.
        const auto heightOf = [&plans](int seat) {
            return plans[seat].ranks.empty() ? 0.0 : plans[seat].usedHeightPx;
        };
        *height = seats.size() < 3 ? heightOf(0)
                                   : std::max(heightOf(kLeftZone), heightOf(kRightZone))
                                         + heightOf(kBelowZone);
        return true;
    };

    if (doubleEdge)
    {
        const double surplus = columnPx - widthOfPose(poses.at(posesByWidth.at(0)))
                               - widthOfPose(poses.at(posesByWidth.at(1)));
        leftWidth = widthOfPose(poses.at(posesByWidth.at(0))) + surplus / 2.0;
        rightWidth = columnPx - leftWidth;
    }

    // ── Le candidat à UN bord ────────────────────────────────────────────────────────────────
    {
        // Le pli à gauche, et la colonne entière : elle paie ses trois autres côtés.
        const std::vector<Seat> seats{
            Seat{true, columnPx, SeatMargins(true, params.marginPx)}};
        std::vector<int> assignment(static_cast<std::size_t>(talks.size()), 0);

        ZonePlan plans[3];
        double height = 0.0;
        if (!evaluate(seats, assignment, plans, &height))
        {
            return false;
        }

        bestHeight = height;
        best[kLeftZone] = plans[0];
    }

    // ── Le candidat à DEUX bords ─────────────────────────────────────────────────────────────
    if (doubleEdge)
    {
        // L et R sont les deux bandes du haut, chacune avec son pli ; B est la colonne entière sous
        // elles. **Les trois paient leurs trois côtés** : les deux rectangles du haut paient donc
        // chacun la ligne qui les sépare de celui du bas, et celui du bas la paie aussi — la bande
        // vide entre eux vaut `2 m`, et c'est la règle.
        //
        // ⚠️ `foldOnTheLeft` est le PREMIER argument, et il vaut `false` pour R — c'est ce qui met
        // la marge de R du bon côté. Les quatre nombres ne sont écrits nulle part ailleurs que dans
        // `SeatMargins` : c'est la seule défense contre une inversion gauche/droite.
        const std::vector<Seat> seats{
            Seat{true, leftWidth, SeatMargins(true, params.marginPx)},
            Seat{false, rightWidth, SeatMargins(false, params.marginPx)},
            Seat{true, columnPx, SeatMargins(true, params.marginPx)}};

        // Le départ : les deux poses que les bandes ont été dimensionnées pour, une par bande — et
        // **les deux variantes ensemble**, puisque c'est le côté du rectangle qui décide laquelle
        // sera posée. Tout le reste dans la bande du bas.
        std::vector<int> assignment(static_cast<std::size_t>(talks.size()), kBelowZone);
        const auto settle = [&](int pose, int seat) {
            const FoldPose &foldPose = poses.at(pose);
            if (foldPose.natural >= 0)
            {
                assignment[static_cast<std::size_t>(foldPose.natural)] = seat;
            }
            if (foldPose.mirror >= 0)
            {
                assignment[static_cast<std::size_t>(foldPose.mirror)] = seat;
            }
        };
        settle(posesByWidth.at(0), kLeftZone);
        settle(posesByWidth.at(1), kRightZone);

        ZonePlan plans[3];
        double height = 0.0;
        if (!evaluate(seats, assignment, plans, &height))
        {
            return false;
        }

        // ⚠️ **Une pose se déplace entière** : ses deux variantes suivent, sinon la pièce serait
        // assignée à un rectangle du mauvais côté et sa variante n'y entrerait pas. Et les deux
        // poses que les bandes accueillent ne bougent pas : ce sont elles qui justifient le pliage,
        // les sortir viderait leur bande de sa raison.
        const auto canMove = [&](int rank, int seat) {
            const int pose = talks.at(rank).onFold ? poseOfRank.at(rank) : -1;
            if (pose == posesByWidth.at(0) || pose == posesByWidth.at(1))
            {
                return false;
            }

            // ⚠️ **La largeur UTILE du rectangle, jamais sa largeur brute.** Il paie sa lisière
            // avant d'accueillir quoi que ce soit, et comparer à la largeur brute envoie une pièce
            // dans une bande où le moteur refuse de la poser — `ifpX1 <= ifpX0`, et la pièce
            // revient avec `rotation = -1`.
            //
            // Le prix de cette erreur n'est pas une case perdue : `evaluate` rend faux, et
            // `PlanFoldedFabric` **jette alors le candidat à un bord qui avait déjà réussi**. Un
            // plan juste devient un échec, avec pour toute explication « The document is too
            // narrow ». Et la fenêtre où le défaut se produit n'est plus de 0,01 px comme quand la
            // marge valait zéro : elle vaut `m`, ce qui rend un espace de 2 cm franchement
            // atteignable.
            if (talkWidth(talks, rank)
                > seats.at(static_cast<std::size_t>(seat)).usableWidthPx() + kWideningPx)
            {
                return false;
            }
            return true;
        };

        bool improved = true;
        while (improved)
        {
            improved = false;
            for (int rank = 0; rank < talks.size() && !improved; ++rank)
            {
                const int pose = talks.at(rank).onFold ? poseOfRank.at(rank) : -1;
                const int from = assignment.at(static_cast<std::size_t>(rank));

                for (int seat = 0; seat < 3 && !improved; ++seat)
                {
                    if (seat == from || !canMove(rank, seat))
                    {
                        continue;
                    }

                    std::vector<int> trial = assignment;
                    if (pose >= 0)
                    {
                        const FoldPose &foldPose = poses.at(pose);
                        if (foldPose.natural >= 0)
                        {
                            trial[static_cast<std::size_t>(foldPose.natural)] = seat;
                        }
                        if (foldPose.mirror >= 0)
                        {
                            trial[static_cast<std::size_t>(foldPose.mirror)] = seat;
                        }
                    }
                    else
                    {
                        trial[static_cast<std::size_t>(rank)] = seat;
                    }

                    ZonePlan trialPlans[3];
                    double trialHeight = 0.0;
                    // ⚠️ **`&trialHeight`, jamais `&height`.** La hauteur de l'essai et celle qui
                    // est tenue sont deux choses : écrire l'essai dans `height` la remplace AVANT
                    // la comparaison, `trialHeight` reste à zéro, et `0.0 < height` accepte alors
                    // tous les essais — la recherche ne redescend plus, `improved` reste vrai à
                    // chaque tour et la boucle ne rend jamais la main. Le nesting se bloquait là.
                    if (!evaluate(seats, trial, trialPlans, &trialHeight))
                    {
                        return false;
                    }

                    if (trialHeight < height)
                    {
                        assignment = std::move(trial);
                        for (int z = 0; z < 3; ++z)
                        {
                            plans[z] = trialPlans[z];
                        }
                        height = trialHeight;
                        improved = true;
                    }
                }
            }
        }

        // Strictement moins : à égalité, le bord unique reste la référence.
        if (height < bestHeight)
        {
            bestHeight = height;
            bestIsDouble = true;
            for (int z = 0; z < 3; ++z)
            {
                best[z] = plans[z];
            }
        }
    }


    // ── La sortie ────────────────────────────────────────────────────────────────────────────
    placements->clear();
    orderedRanks->clear();
    foldedZones->clear();
    *usedHeightPx = bestHeight;

    const auto append = [&](const ZonePlan &plan, double dx, double dy) {
        const QVector<OExportPlacement> placed = ToExportPlacements(plan.placements, plan.polygons);
        for (int i = 0; i < placed.size(); ++i)
        {
            OExportPlacement moved = placed.at(i);
            moved.x += dx;
            moved.y += dy;
            placements->append(moved);
            orderedRanks->push_back(plan.ranks.at(static_cast<std::size_t>(i)));
        }
    };

    if (!bestIsDouble)
    {
        // Le bord unique : **un seul rectangle, la colonne elle-même**, dont le pli est du côté que
        // ses pièces au pli demandent. Ce n'est pas un découpage — le canvas ne dessine les
        // rectangles qu'à partir de deux, celui du bac dit déjà où est la colonne — mais son côté
        // de pli, lui, se dit ici : c'est lui qui met le trait au bon bord.
        append(best[kLeftZone], 0.0, 0.0);

        OExportFoldedZone zone;
        zone.rect = QRectF(0.0, 0.0, columnPx, bestHeight);
        zone.foldOnTheLeft = true;
        foldedZones->append(zone);
        return true;
    }

    // ⚠️ **Les trois rectangles sont translatés dans le repère de la colonne** — L en (0, 0),
    // R en (leftWidth, 0), B sous les deux autres. C'est ce qui permet au canvas de tout dessiner
    // comme un seul plan, sans rien savoir du découpage.
    const double upperHeight = std::max(best[kLeftZone].usedHeightPx, best[kRightZone].usedHeightPx);

    const struct
    {
        int zone;
        double dx;
        double dy;
    } seats[] = {
        {kLeftZone, 0.0, 0.0},
        {kRightZone, leftWidth, 0.0},
        {kBelowZone, 0.0, upperHeight},
    };

    for (const auto &seat : seats)
    {
        const ZonePlan &plan = best[seat.zone];
        if (plan.ranks.empty())
        {
            continue;   // un rectangle vide ne se dessine pas
        }

        append(plan, seat.dx, seat.dy);

        OExportFoldedZone zone;
        zone.rect = QRectF(seat.dx, seat.dy,
                           seat.zone == kLeftZone ? leftWidth
                                                  : (seat.zone == kRightZone ? rightWidth : columnPx),
                           plan.usedHeightPx);
        zone.foldOnTheLeft = seat.zone != kRightZone;
        foldedZones->append(zone);
    }

    return true;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportNesting::Run(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params)
    -> OExportNestingResult
{
    OExportNestingResult result;
    result.binWidthPx = params.binWidthPx;

    if (layers.isEmpty())
    {
        result.error = QObject::tr("The size range carries no piece to nest.");
        return result;
    }
    if (params.binWidthPx <= 0.0)
    {
        result.error = QObject::tr("The document width must be greater than zero.");
        return result;
    }

    // The engine never sees a size: it sees one footprint per piece — or per group,
    // which then moves as a single rigid block.
    QVector<SizeLayer> engineLayers;
    engineLayers.reserve(layers.size());

    QHash<QString, QVector<double>> anglesByKey;

    /// Ce que chaque pièce dit au découpage du tissu plié — sa largeur, ses angles, et ses deux
    /// rotations au pli. Une carte à part des angles : sur du tissu plié, ce n'est plus une liste
    /// d'angles qu'on transmet au moteur, c'est un **découpage** qu'on décide.
    QHash<QString, PieceTalk> talkByKey;

    for (const OExportSizeLayer &layer : layers)
    {
        SizeLayer engineLayer;
        engineLayer.label = layer.label;

        for (const OExportPieceGeometry &piece : layer.pieces)
        {
            Piece enginePiece;
            enginePiece.name = piece.name;
            enginePiece.groupName = piece.groupName;
            // The group's identity travels separately from its name: the engine groups
            // on the key, and displays the name.
            enginePiece.groupKey = piece.groupId == 0 ? QString() : piece.key;

            ContourNode node;
            node.type = QStringLiteral("Contour");
            // Le contour MAPPÉ : c'est lui qui porte le miroir d'un plan de coupe. Sur un run
            // d'impression la matrice de la pièce est l'identité, donc les deux getters rendent le
            // même contour et rien ne bouge.
            node.polyline = ToPath64(piece.piece.GetMappedExternalContourPoints(), piece.alignDelta);
            if (node.polyline.size() < 3)
            {
                continue;
            }
            enginePiece.contourNodes.append(node);

            engineLayer.pieces.append(enginePiece);

            if (params.cuttingPlan)
            {
                // Keyed, because the footprint is what the engine constrains — a group is nested as
                // one block, and its members share the key.
                if (!params.foldedFabric)
                {
                    anglesByKey.insert(piece.key,
                                       OExportCuttingDirection::AllowedAngles(piece.direction, params.oneWayLayout));
                }
                else
                {
                    // ── Ce que la pièce dit au découpage du tissu plié ──────────────────────────
                    //
                    // Sa **largeur** — son « rectangle », celui dont le bord décide si elle rentre
                    // dans une bande. Et si elle est **au pli**, ses deux rotations : une pièce au
                    // pli n'a plus qu'une orientation par paroi, celle qui met sa foldline verticale
                    // avec le corps dans le tissu. L'autre le mettrait dehors.
                    PieceTalk talk;
                    talk.onFold = piece.onFold;
                    talk.mirrored = piece.mirrored;
                    talk.name = piece.name;
                    talk.id = piece.id;

                    if (talk.onFold)
                    {
                        // ⚠️ Le pli l'emporte sur la grainline, ici et pas ailleurs : c'est le pli
                        // du tissu qui porte une pièce qu'on coupe au pli, pas son droit-fil.
                        // Et c'est SA rotation qui décide de son côté — la pièce est posée dans le
                        // sens où elle est dessinée, jamais retournée.
                        if (!OExportCuttingDirection::FoldAngle(piece.piece, &talk.foldAngle,
                                                                &talk.foldOnTheRight))
                        {
                            // Aucune rotation ne met le pli sur un bord de sa propre pièce : il n'y
                            // a pas de rotation à trouver, et poser la pièce quand même serait un
                            // plan faux qui a l'air fini. On le dit, et on nomme la pièce.
                            result.error =
                                QObject::tr("The piece \"%1\" (id %2) is cut on the fold, but its "
                                            "fold line is not one of its own edges. Check its fold "
                                            "line in the Details mode.")
                                    .arg(piece.name)
                                    .arg(piece.id);
                            return result;
                        }

                        // ⚠️ Sa largeur se mesure À SA ROTATION DE PLI, jamais sur les angles de sa
                        // direction : une pièce qui aurait une grainline ET un pli verrait sa
                        // largeur mesurée dans la mauvaise orientation, et une bande jugée trop
                        // étroite pour elle.
                        talk.widthPx = NarrowestWidth(node.polyline, {talk.foldAngle});
                    }
                    else
                    {
                        const QVector<double> angles = OExportCuttingDirection::AllowedAngles(
                            piece.direction, params.oneWayLayout);
                        talk.angles.assign(angles.cbegin(), angles.cend());
                        talk.widthPx = NarrowestWidth(node.polyline, talk.angles);
                    }

                    talkByKey.insert(piece.key, std::move(talk));
                }
            }
        }

        if (!engineLayer.pieces.isEmpty())
        {
            engineLayers.append(engineLayer);
        }
    }

    if (engineLayers.isEmpty())
    {
        result.error = QObject::tr("No piece of this range could be prepared for nesting.");
        return result;
    }

    // The direction of each piece, by footprint key — and in the FOOTPRINT's order, not the layer's:
    // `Footprint::compute` regroups and reorders what it is given, and the engine's per-piece
    // constraints have to line up with what the engine will actually receive.
    //
    // Empty on a print run, and an empty list is a piece nobody constrained: the engine keeps its
    // whole sweep, exactly as it did before this parameter existed.
    std::vector<std::vector<double>> allowedAngles;

    const Footprint::Result footprint = Footprint::compute(engineLayers);
    if (footprint.polygons.empty())
    {
        result.error = QObject::tr("The size range carries no usable contour.");
        return result;
    }

    // Fixed width, unbounded height: the height is what the nesting produces.
    //
    // The margin and the gap arrive already in pixels, like the width and the polygons:
    // the engine consumes every length on the same scale, and the conversion from the
    // millimetres the user set is made once, at the caller — handed over raw, 5.0 would
    // mean 5 px, which is 1.3 mm, and not the policy Rabnest applies.
    // The zones the run has to keep clear — the cover page's block, and nothing else so far.
    // Handed to BOTH engines through the one Bin, because that is what the arbitration below
    // compares: two plans measured over ONE obstacle set.
    std::vector<Bin::Zone> zones;
    if (!params.coverZonePx.isEmpty())
    {
        zones.push_back({params.coverZonePx.x(), params.coverZonePx.y(),
                         params.coverZonePx.width(), params.coverZonePx.height()});
    }

    // **The explicit-list constructor, and not the corner one.** An empty list is the absence of
    // a zone; the corner constructor appends one of its own the moment a dimension is non-zero,
    // which is how the strip engine's A0 emulation works and is not what a run without a cover
    // page asks for.
    // ⚠️ `zones` n'est PAS déplacé : le tissu plié construit un bac par rectangle, et chacun doit
    // voir le même bloc interdit. La liste est minuscule, la copie ne coûte rien.
    const Bin bin(params.binWidthPx, params.marginPx, zones);
    Engine::Config config;
    config.gap = params.gapPx;

    // The grid, and only on a tiled run. It is what lets the engine adjust the
    // plan once it is placed — keeping the footprint borders off the cut lines,
    // and spending the dead band the last artboard row keeps for free. Left at
    // zero, the engine places exactly as it did before tiling existed.
    config.artboardCount  = params.artboardCount;
    config.artboardHeight = params.artboardHeightPx;

    // **The run has to be able to be given up.** It is the whole reason the dialog in front of it
    // can be closed: the engine has no abort hook of its own, so one is handed to it, and it reads
    // it between its iterations. Null — a caller with nobody watching — leaves it as it was.
    config.abandoned = params.abandoned;

    // ── Le tissu plié s'arrête ici : il ne remplit pas un bac, il découpe la colonne ──────────
    //
    // Il a sa propre route parce que tout ce qui suit — l'arbitrage tuilé, le résultat — suppose
    // UN bac de la largeur `binWidthPx`. Le sien en fait trois, et il rend un plan déjà translaté
    // dans le repère de la colonne : le canvas n'a donc rien à savoir du découpage.
    const bool foldedFabricPlan = params.cuttingPlan && params.foldedFabric;

    if (foldedFabricPlan)
    {
        QVector<PieceTalk> talks;
        talks.reserve(static_cast<int>(footprint.keys.size()));
        for (const QString &key : footprint.keys)
        {
            talks.append(talkByKey.value(key));
        }

        QVector<OExportPlacement> cutPlacements;
        std::vector<int> cutRanks;
        double cutHeightPx = 0.0;
        if (!PlanFoldedFabric(footprint, talks, params, zones, config, &cutPlacements, &cutRanks,
                              &result.foldedZones, &cutHeightPx, &result.error, &result.abandoned))
        {
            return result;   // abandonné ou en erreur : les deux sont déjà posés dans le résultat
        }

        result.placements = cutPlacements;
        result.keys.reserve(static_cast<int>(cutRanks.size()));
        for (const int rank : cutRanks)
        {
            result.keys.append(footprint.keys.at(static_cast<std::size_t>(rank)));
        }
        result.usedHeightPx = cutHeightPx;

        result.ok = true;
        return result;
    }

    if (params.cuttingPlan)
    {
        allowedAngles.reserve(footprint.keys.size());
        for (const QString &key : footprint.keys)
        {
            const QVector<double> angles = anglesByKey.value(key);
            allowedAngles.emplace_back(angles.cbegin(), angles.cend());
        }
    }

    const Engine engine{config};
    bool abandoned = false;
    std::vector<Placement> placements =
        engine.place(bin, footprint.polygons, allowedAngles, {}, &abandoned);

    if (abandoned)
    {
        // **What it holds is a half-built plan, and it is not read.** Every check below would be
        // asking a question about a result that was never produced.
        result.abandoned = true;
        return result;
    }

    if (placements.size() != footprint.polygons.size())
    {
        result.error = QObject::tr("The nesting engine returned an incomplete result.");
        return result;
    }

    result.keys.reserve(static_cast<int>(footprint.keys.size()));
    for (const QString &key : footprint.keys)
    {
        result.keys.append(key);
    }

    // The engine marks an unplaced piece with a negative rotation and reports
    // nothing else — no status, no exception.
    if (!AllPlaced(placements))
    {
        result.error = TooNarrowError(params);
        return result;
    }

    // ── L'arbitrage : deux moteurs, le moins de feuilles gagne ────────────
    //
    // Sur un run TUILÉ, un second moteur entre en jeu. Le moteur de bande
    // minimise une HAUTEUR : la page n'existe pas pour lui, et la grille est une
    // grille posée après coup sur le résultat — mesuré sur un patron réel,
    // 1088,050 mm pour une frontière de rangée à 1036, payés d'une rangée
    // entière. Le moteur Tiled paie en PAGES : les grosses pièces d'abord, puis
    // les petites glissées dans les trous au VRAI contour.
    //
    // Les deux plans sont calculés et c'est celui qui imprime le MOINS de
    // feuilles qui est gardé. Le moteur de bande garde donc son mot à dire, et
    // le résultat ne peut pas être pire qu'avant.
    //
    // ⚠️ Le prédicat est `artboardHeightPx > 0.0` — celui d'Engine.cpp — et
    // surtout PAS `artboardCount == 0`, qui n'arrive jamais : `Columns()` rend
    // 1 sur un roulé, jamais 0. Un moteur qui testerait ça se tromperait sur
    // TOUS les runs actuels.
    // ⚠️ **Et jamais quand une pièce est collée au bord.** Le moteur Tiled ne connaît ni les angles
    // par pièce ni l'ancrage : il tourne au quart de tour et pose librement. Lui laisser gagner
    // l'arbitrage jetterait un plan juste pour un plan qui imprime moins de feuilles — et rien ne
    // le dirait. Aujourd'hui il est déjà hors jeu (un plan de coupe ne pose pas d'artboard, donc
    // `artboardHeightPx` vaut zéro), mais c'est une dépendance implicite : le jour où un plan de
    // coupe recevra un cadre, elle tomberait sans bruit.
    if (params.artboardHeightPx > 0.0 && params.artboardCount > 0 && !foldedFabricPlan)
    {
        TiledEngine::Spec spec;
        spec.sheetWidth    = params.binWidthPx / params.artboardCount;
        spec.sheetHeight   = params.artboardHeightPx;
        spec.gap           = params.gapPx;
        spec.allowRotation = true;
        // The second engine is asked the same question as the first, and for the same reason: a
        // run given up during the tiled pass has to stop there too.
        spec.abandoned     = params.abandoned;

        // ── TROIS ORDRES, TROIS PLANS, ET C'EST ICI QU'ON CHOISIT ─────────────
        //
        // Le moteur est glouton : il pose les pièces dans un ordre, et ne revient jamais. La
        // première prend la meilleure place, la dernière se contente du reste — donc **l'ordre
        // décide de tout le plan**. Un seul ordre, c'est un seul plan, et rien ne peut rattraper
        // une page qu'il a laissée.
        //
        // On demande donc trois plans au même moteur, à partir des MÊMES pièces, et on garde
        // celui qui imprime le moins de feuilles — compté ici, avec `OExportSheets::Compute`,
        // qui reste le seul endroit où cette règle est écrite.
        //
        // **Le plan historique reste la référence** : un autre ne le remplace que s'il imprime
        // STRICTEMENT moins. Si aucun ne fait mieux, rien ne change — au plan près.
        const TiledEngine::Spec::Order orders[] = {TiledEngine::Spec::Order::HolderArea,
                                             TiledEngine::Spec::Order::HolderHeight,
                                             TiledEngine::Spec::Order::HolderWidth};

        const QVector<OExportPlacement> stripPlan =
            ToExportPlacements(placements, footprint.polygons);
        int bestSheets = PrintedSheets(
            stripPlan, Engine::UsedHeight(placements, footprint.polygons, bin), params);

        for (const TiledEngine::Spec::Order order : orders)
        {
            spec.order = order;

            bool tiledAbandoned = false;
            const std::vector<Placement> tiled =
                TiledEngine(spec).place(bin, footprint.polygons, &tiledAbandoned);

            if (tiledAbandoned)
            {
                result.abandoned = true;
                return result;
            }

            if (tiled.size() != footprint.polygons.size() || !AllPlaced(tiled))
            {
                continue;   // ce départ-là n'a pas su tout poser : il ne concourt pas
            }

            const int tiledSheets = PrintedSheets(
                ToExportPlacements(tiled, footprint.polygons),
                Engine::UsedHeight(tiled, footprint.polygons, bin), params);

            if (tiledSheets < bestSheets)
            {
                bestSheets  = tiledSheets;
                placements  = tiled;
            }
        }

    }

    result.placements = ToExportPlacements(placements, footprint.polygons);
    result.usedHeightPx = Engine::UsedHeight(placements, footprint.polygons, bin);

    result.ok = true;
    return result;
}
