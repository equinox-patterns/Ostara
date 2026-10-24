/************************************************************************
 **
 **  @file   oexportcanvas.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — draw the bin and the nested content.
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

#ifndef OEXPORTCANVAS_H
#define OEXPORTCANVAS_H

#include "oexportartboards.h"  // OExportArtboards::Grid
#include "oexportcoverpage.h"  // OExportCoverPage::Drawing
#include "ogradedsvgwriter.h"  // OGradedSvg::LayoutObject

#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QVector>

class QGraphicsItem;
class QGraphicsScene;
class VAbstractPattern;
class VContainer;
class OSizerange;
struct OExportNestingResult;
struct OExportGroupDecor;
struct OExportPieceDecor;
struct OExportPieceGeometry;
struct OExportPieceGrainline;
struct OExportSizeLayer;
struct OExportTemplate;

/// @brief The decor of one piece, resolved as the canvas would draw it — the frozen form a run stores.
///
/// It lives beside the drawing rather than beside the model on purpose: the resolution and the drawing
/// are two halves of one computation, and splitting them across two files is how the two readings of a
/// cartouche's position — one for the screen, one for the paper — came to exist in the first place.
///
/// ⚠️ It reads the DOCUMENT, so it is only correct while the document is on the size being recorded.
/// The caller resolves it during the run, on the reference layer, and never afterwards.
///
/// @param cuttingPlan un plan de coupe n'enregistre **aucune miniature** — voir
///                    @ref OExportPieceDecor::thumbnails. Le drapeau est lu ici, à la résolution.
/// @param sizeAxes    la taille que cette résolution décrit — valeur de chaque axe, nommée comme
///                    le tableau de mesures nomme ses axes. Vide : la taille de référence du
///                    tableau, ce que la résolution vivante veut. Voir @ref ResolveEffectiveState.
auto ResolvePieceDecor(const OExportPieceGeometry &geometry, const OSizerange *range,
                       const VContainer *pattern, VAbstractPattern *doc, bool cuttingPlan,
                       const QMap<QString, QString> &sizeAxes = {}) -> OExportPieceDecor;

/// @brief The reference layer's grainline for one piece, resolved as the canvas would draw it.
///
/// The same contract as @ref ResolvePieceDecor, and the same constraint: it reads the DOCUMENT, so it
/// is only correct while the document is on the size being recorded.
auto ResolvePieceGrainline(const OExportPieceGeometry &geometry, const OSizerange *range,
                           const Unit &unit, const VContainer *pattern, bool cuttingPlan,
                           const QRectF &referenceSeat = QRectF()) -> OExportPieceGrainline;

/// @brief One GROUP's decor, resolved as the canvas would draw it — the frozen form a run stores.
///
/// @param members the layer's pieces that belong to this group: it is on them that the group's frame
///                is measured — the union of their unplaced extents, whose centre is what a child that
///                was never placed falls back on. Measured here, at generation, on the same frozen
///                geometry the drawing will use, so the two agree.
///
/// ⚠️ The same constraint as the pieces: it reads the DOCUMENT, so it is only correct while the
/// document is on the size being recorded.
auto ResolveGroupDecor(const QVector<OExportPieceGeometry> &members, quint32 groupId,
                       const OSizerange *range, const Unit &unit, const VContainer *pattern,
                       VAbstractPattern *doc, bool cuttingPlan,
                       const QMap<QString, QString> &sizeAxes = {}) -> OExportGroupDecor;

/// @brief Quel genre de plan ce run est, pour ce que le canvas en dessine.
///
/// Les deux drapeaux voyagent ensemble parce qu'ils répondent à la même question — *quel plan
/// est-ce ?* — et parce qu'un booléen de plus à la queue de `Build` aurait été un paramètre de plus
/// à ne pas confondre avec les onze autres. Le prochain en trouvera sa place ici.
struct OExportRunFlags
{
    /// **Un plan de coupe décompose les groupes** : la pièce retenue n'est plus un membre, c'est la
    /// pièce qu'on taille — elle porte donc SON décor, et le groupe n'en dessine plus aucun. Voir
    /// IsGroupedMember, où tout se décide.
    bool cuttingPlan{false};

    /// **Un plan sur tissu plié est dessiné sur la DEMI-laize** — le bac fait la moitié du tissu
    /// acheté, et c'est ce qu'on trace. Son côté pli porte alors un trait distinct, pour qu'on voie
    /// du même coup d'œil où est le pli : c'est le bord auquel les pièces au pli sont collées.
    bool foldedFabric{false};

    /// **Un plan plotter est dessiné sur ses TAPIS.** Un tapis est une CELLULE DURE : rien de plus
    /// grand que lui ne peut être découpé, et c'est pourquoi l'aperçu les montre — chaque tapis est
    /// un chargement de machine.
    ///
    /// ⚠️ **Ils ne sont plus les lignes de coupe.** Ils l'étaient tant que chaque pièce gardait son
    /// bloc de tapis et que les morceaux tombaient sur les bords ; les morceaux se rangent maintenant
    /// là où ils tiennent, donc un bord de tapis ne coïncide plus avec une coupe — et c'est sans
    /// conséquence, la découpe étant dans la géométrie du morceau et non dans le cadre.
    ///
    /// ⚠️ **Affichage seulement.** Le fichier est écrit en parcourant le CONTENU, jamais la scène :
    /// cette grille n'y entre pas — et c'est heureux, le logiciel du plotter découperait les bords
    /// des tapis.
    bool plotterPlan{false};

    /// La zone utile du tapis, en MILLIMÈTRES. `Build` la convertit : c'est le seul endroit qui
    /// dessine, et il n'y a pas deux conversions à tenir en accord. Nulle : pas de tapis.
    qreal matWidthMm{0.0};
    qreal matHeightMm{0.0};

    /// Le nombre de tapis que le run a ouverts. **Zéro : il n'y a pas de tapis à dessiner.**
    ///
    /// Il vient du résultat et n'est pas recompté ici : la dernière rangée d'un rangement compact
    /// reste incomplète, donc dessiner la grille jusqu'au bord du canevas montrerait un tapis de
    /// plus que ce que la machine demandera — et c'est précisément ce que l'onglet fait descendre.
    int matCount{0};
};

/// @brief Où ce plan se pose dans la scène — et s'il est celui qui la vide.
///
/// Un run n'a qu'un plan : il part de l'origine, et la scène est à lui. **Une PAIRE n'en a pas qu'un** —
/// le tissu et la doublure sont deux nappes, et elles se lisent côte à côte — donc le second se pose à
/// droite du premier au lieu de par-dessus. C'est un décalage d'ORIGINE et non une transformation
/// d'items : la formule de placement et tout ce qui se mesure en coordonnées de scène (la
/// correction, les lecteurs de contenu) doivent lire la même chose.
///
/// `clearScene` reste au premier : la scène ne se vide qu'une fois, le second plan s'ajoute au premier.
struct OExportPlanSeat
{
    /// Le point de la scène où tombe l'origine du plan — son coin haut-gauche de bac.
    QPointF origin{};

    /// Vrai quand ce plan peut vider la scène : le premier plan d'un run, toujours.
    bool clearScene{true};
};

/// @brief What the canvas drew, in the shape a writer needs.
///
/// Collected by Build itself rather than rebuilt beside it: the decor has one definition
/// — the canvas's — and a second one written for the export would drift from the screen.
/// Everything here points into the scene the canvas filled, so it lives no longer than it.
struct OExportCanvasContent
{
    /// One drawn object: a lone piece, or the members a group was nested as.
    struct Object
    {
        QString name;
        bool isGroup{false};
        QGraphicsItem *loneItem{nullptr};
        /// Member name and silhouette, in the group's own member order.
        QVector<QPair<QString, QGraphicsItem *>> members;
    };

    /// One size layer, in the order the canvas drew them — reference first.
    struct Layer
    {
        /// The size itself: what the role, the styles and the trace all key on.
        QString label;

        /// What the layer is to be called, when the Size Styles give it a name. Empty when
        /// they do not, and the label is then the name.
        QString displayName;

        QVector<Object> objects;
    };

    QVector<Layer> layers;

    /// The decor of the reference layer, one entry per owner that carries any.
    QList<OGradedSvg::LayoutObject> decor;

    /// Les enfants qu'un membre de groupe porte POUR LUI — **payload seulement, jamais dessinés.**
    ///
    /// Le plan d'impression dessine le groupe : ses membres ne portent qu'une copie masquée de leurs
    /// propres enfants, construite pour que Rabnest puisse afficher un membre SEUL. Cette liste-là ne
    /// va donc qu'à `OExportPieces::Of`, quand `decor` va aussi bien au payload qu'à la couche
    /// visible — les mélanger ajouterait au plan l'encre de chaque membre.
    QList<OGradedSvg::LayoutObject> payloadDecor;
};

/// The index, in `OExportNestingResult::placements`, of the placement an item belongs to.
///
/// Set on every top-level item a placement produced — each size layer's silhouette, and the
/// decor carrier of the owner it decorates — so a footprint's whole follow set is found by
/// walking the scene and filtering on this. That is what lets a selection survive a rebuild:
/// Build() clears the scene, so a correction can hold the index and never an item pointer.
///
/// Two things depend on it, and both are easy to break silently. It must be carried by the
/// decor CARRIER and not by the child items, because a click on a cartouche lands on a child
/// and has to walk up to find the footprint. And an int on an item is invisible to the PDF
/// writer, which walks OExportCanvasContent and never reads roles.
constexpr int kExportFootprintRole = Qt::UserRole + 908;

/// @brief Draws the result of a nesting run into the Export scene.
///
/// The nesting places footprints, not sizes: one footprint covers every size of
/// the range. The canvas therefore draws each size layer with the transform its
/// footprint received — the same thing the nested SVG contains, where every size
/// layer is moved by the transform of the piece it belongs to.
class OExportCanvas
{
public:
    /// @param range   the range being nested, or nullptr when the run is a single size.
    ///                Its per-owner overrides are where the decor of the reference layer
    ///                takes its position from — never the Details-mode placement.
    /// @param pattern the pattern, source of the grainlines and of the groups. Decor is
    ///                skipped when it is nullptr.
    /// @param doc     the document, which the cartouche resolver reads the template from.
    ///                Blocks are skipped when it is nullptr.
    /// @param content when not nullptr, filled with what was drawn — see the note on
    ///                OExportCanvasContent. Drawing is unaffected either way.
    /// @param artboards the sheets a tiled plan is cut into. An invalid grid — the default,
    ///                and what every caller with no use for it leaves here — changes nothing
    ///                at all; a valid one makes the sheets the paper, and the bin the run's
    ///                own extent instead of what is drawn around it. The PDF writer leaves it
    ///                at its default deliberately: a page IS a sheet there, so its edge is not
    ///                drawn on it.
    ///                WHICH of those sheets gets paper is read here, from @p result's own
    ///                placements — see OExportSheets. Deliberately not a parameter: a paper the
    ///                caller could hand in is a paper the caller could hand in wrong, and the
    ///                PDF writer asks the same question of the same two things.
    /// @param marks   the template of assembly marks the plan is drawn with, or nullptr when
    ///                there is none — a library the user has emptied, or a name whose file has
    ///                gone. Nothing is drawn then, and the plan keeps the cut lines it always
    ///                had. The caller resolves the name: a canvas that looked one up would be a
    ///                second place deciding what gets drawn.
    /// @param framed  whether the format is one of the framed ones.
    ///                The frame and its scissors are drawn only there, and the cut lines below
    ///                stand down in their favour: a second line under the user's own would fight
    ///                the opacity she set (§11.5).
    /// @param seat    where the plan lands in the scene, and whether it may clear it. Left at its
    ///                default by every writer — a file holds ONE plan, and it starts at the origin —
    ///                and moved only by the screen, which shows a pair side by side.
    static void Build(QGraphicsScene *scene,
                      const QVector<OExportSizeLayer> &layers,
                      const OExportNestingResult &result,
                      const OSizerange *range,
                      const VContainer *pattern,
                      VAbstractPattern *doc,
                      OExportCanvasContent *content = nullptr,
                      const OExportArtboards::Grid &artboards = {},
                      const OExportTemplate *marks = nullptr,
                      bool framed = false,
                      const OExportCoverPage::Drawing *cover = nullptr,
                      const OExportRunFlags &run = {},
                      const OExportPlanSeat &seat = {});
};

#endif // OEXPORTCANVAS_H
