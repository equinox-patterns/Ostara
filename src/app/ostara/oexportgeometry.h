/************************************************************************
 **
 **  @file   oexportgeometry.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — resolve every piece of the pattern for every size
 **          of a size range, straight from the model.
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

#ifndef OEXPORTGEOMETRY_H
#define OEXPORTGEOMETRY_H

#include <QPointF>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtNumeric>

#include "../vlayout/vlayoutpiece.h"
#include "core/oexportcuttingdirection.h"
#include "core/oexportcuttingguard.h"
#include "widgets/InfoBlock/oinfoblockexport.h"

class OSizerange;
class VContainer;
class VMeasurements;
class VPattern;

/// @brief One piece's decor, RESOLVED — its cartouche and its annotations, as they are drawn.
///
/// The children of a piece do not live in the piece: they live in the DOCUMENT. A block's position is
/// re-materialised there every time the document is put on a size (`OChildAnchor::Materialize`), and
/// an annotation's rotation is a formula evaluated against the live variables. Drawing them from the
/// document therefore draws whatever size it happens to be on NOW, not the size the run was generated
/// at — outlines frozen, children alive, and the two drift apart the moment the user generates at one
/// size and looks at another. That is the bug this type exists to close.
///
/// ⚠️ The record form is the SVG writer's own — the very one it has always consumed — because it is
/// already enough to redraw both items, and because it means screen and paper read ONE computation
/// instead of two that can disagree. The graded exporter has been resolving-then-writing this way all
/// along; the export path is the one that was still resolving at draw time.
struct OExportPieceDecor
{
    QVector<OInfoBlockSvg::Block> blocks;
    QVector<OInfoBlockSvg::Annotation> annotations;

    /// The owner's miniatures — a piece's own, or the one a group carries per member.
    ///
    /// A record rather than an item, for the same reason the blocks and the annotations are: nobody
    /// ever placed a miniature on the export scene, and the frame it is drawn in is the owner's.
    ///
    /// ⚠️ **Un plan de coupe n'en porte aucune** (décidé) : un plan de coupe dit
    /// quoi tailler, la miniature est une aide de papier. La règle est posée **à la résolution**, dans
    /// `ResolveOwnerMiniatures`, et non au dessin — un record jamais fait ne peut pas être dessiné par
    /// mégarde. Le champ est donc vide sur tout run de coupe, et sur lui seul.
    QVector<OInfoBlockSvg::Thumbnail> thumbnails;

    /// Whether this piece's decor was ever resolved. False on a piece the run had no decor to record
    /// for — an unworn layer, or a piece the document no longer holds.
    bool resolved{false};

    auto IsEmpty() const -> bool
    {
        return not resolved or (blocks.isEmpty() and annotations.isEmpty() and thumbnails.isEmpty());
    }
};

/// @brief The reference layer's grainline for one piece, RESOLVED — see @ref OExportPieceDecor.
///
/// Exactly the same story, found the same way: the grainline the range asks for is resolved from the
/// DOCUMENT — the piece's stored grainline geometry, and its rotation formula evaluated against the
/// live container. Resolving it at draw time drew the size the document was on *then*, not the one the
/// run was generated at. It stayed invisible for as long as a single-size run left the document on the
/// run's own size; giving the document back at the end of the run is what showed it.
///
/// The LINE is stored rather than the three numbers it is built from. Those three are turned into a
/// line by @ref VPieceGrainline, whose constructor also takes an arrow type — and freezing the line
/// leaves nothing about that conversion to be re-decided at draw time.
struct OExportPieceGrainline
{
    bool resolved{false};
    QLineF mainLine;
};

/// @brief The reference layer's grainline for one GROUP, RESOLVED — see @ref OExportPieceDecor.
///
/// ⚠️ **Frozen as a PLACEMENT, not as a line**, because a group's grainline is not written onto any
/// model object the way a piece's is: it is drawn straight into the scene under the carrier
/// (`OGraded::MakeGrainlineItem`). So what has to survive the trip is where it starts and how long it
/// is, plus its angle.
///
/// ⚠️ Of those three, only the POSITION depends on the size: a group's length and angle are literals,
/// not formulas evaluated against the container. But they are frozen together all the same — one
/// record, read once, at the size that matters.
struct OExportGroupGrainline
{
    bool resolved{false};
    QPointF pos;
    qreal lengthPx{0.0};
    qreal rotationDegrees{0.0};
};

/// @brief One GROUP's decor, RESOLVED — its cartouche, its annotations, and its grainline.
///
/// A group is not a piece: it has no slot of its own in the layers, and its children are stored
/// against the top-left corner of its members rather than against the document origin. Only its
/// carrier differs, though, and the resolution is the pieces' own — which is why the cartouche and the
/// annotations travel in an @ref OExportPieceDecor.
struct OExportGroupDecor
{
    OExportPieceDecor owner{};
    OExportGroupGrainline grainline{};
    bool resolved{false};
};

/// @brief One piece of the pattern, resolved for one size of the range.
struct OExportPieceGeometry
{
    /// Footprint key. Pieces of a group share it: the group is nested as one rigid block.
    QString key;
    QString name;
    QString groupName{}; ///< empty when the piece does not belong to a group
    VLayoutPiece piece;  ///< geometry resolved for this size, in pixels

    quint32 id{0};      ///< piece id, the stable identity — names are not unique
    quint32 groupId{0}; ///< 0 when the piece is not part of a group

    /// Pixels. Shifts this size onto the others of the same piece, so that unioning
    /// them into one footprint means something. Set by OExportAlignment; applied by
    /// the engine and by the canvas alike.
    QPointF alignDelta{};

    /// Pattern-frame X of the unit's forced alignment point for this size, or NaN when
    /// the unit has none. Resolved at collection time, because the container only ever
    /// holds the size being collected — OExportAlignment runs later, on the document
    /// put back on its own size.
    qreal alignPointX{qQNaN()};

    /// The piece's direction, resolved for a cutting plan and left unknown otherwise.
    ///
    /// ⚠️ **A grouped member takes the GROUP's direction, never its own.** `VLayoutPiece::Create`
    /// does not mask a member's grainline — it reads `IsEnabled()` and nothing else — so the flag
    /// can be true while the direction belongs to the group. The masking is done on the item.
    OExportCuttingDirection::Direction direction{};

    /// Whether this piece is cut ON THE FOLD — its fold line is one of its own edges.
    ///
    /// **Read on the LINE, never on the marking's type**: `FoldLineType` is born at
    /// `TwoArrowsTextAbove`, so "the type is not None" is true of every piece ever created — the
    /// error that left the guard silent the first time (§15.2). What says a piece has a fold is
    /// `SeamAllowanceMirrorLine` being there, and that is how the canvas reads it too.
    ///
    /// On a folded plan such a piece is laid **folded**, with that line glued to the fabric's fold;
    /// on a flat plan it is laid unfolded, as it always was.
    bool onFold{false};

    /// Whether this geometry is the **mirrored twin** of a piece cut on the fold.
    ///
    /// A piece cut on the fold is drawn for one edge, and the mirror (the flip vertical) opens it
    /// the other one: mirrored, a piece meant for the left edge goes on the right. Both twins are
    /// emitted, as two footprints with two keys, and **the nesting keeps only one** — the one whose
    /// edge matches the rectangle the pose lands in. The other is in no result and on no drawing.
    bool mirrored{false};

    /// Pattern-frame X of the vertical line this geometry is REFLECTED about, or NaN when it is not
    /// reflected at all.
    ///
    /// The reflection lives in the piece's matrix, which the engine reads through the mapped contour
    /// and the outline composes rather than overwrites. The information block and the annotations do
    /// **not** go through it: they belong to the document, and the carrier that draws them knows only
    /// how to translate and to turn — a reflection is neither. So the axis is stated here, in the very
    /// frame those children store their coordinates in, and the decor mirrors each of them by hand.
    ///
    /// ⚠️ It is set by the three places that reflect a piece — the folded twin, `Force flipping` and
    /// the symmetrical copy — and it is the x of the piece's outline box centre. It is read on the
    /// UNMAPPED contour (@ref VLayoutPiece::DetailBoundingRect), so the value is the same before and
    /// after the reflection: a box reflected about its own centre is its own image.
    qreal mirrorX{qQNaN()};

    /// This piece's cartouche and annotations, resolved at generation — see @ref OExportPieceDecor.
    ///
    /// Filled for the reference layer only: that is the one layer the canvas draws decor for, every
    /// other being stripped of it. Empty on a flat plan that never carried any.
    OExportPieceDecor decor{};

    /// The reference layer's grainline for this piece, resolved at generation — see
    /// @ref OExportPieceGrainline. Filled for the reference layer only, like the decor.
    OExportPieceGrainline grainline{};
};

/// @brief Every piece of the pattern, resolved for one size of the range.
struct OExportSizeLayer
{
    QString label;
    QVector<OExportPieceGeometry> pieces;
};

/// @brief Turns a size range of the pattern into resolved geometry.
///
/// The pattern is evaluated one size at a time — the container is shared, and
/// re-parsing it is exactly what the application does when the user switches
/// size. The document is left on the size it was found on.
///
/// This is what replaces reading the graded SVG: the geometry never leaves the
/// application, so there is nothing to write and nothing to parse back.
class OExportGeometry
{
public:
    /// @brief One size of a range: which dimension carries it, its base, its label.
    struct OExportSizeRef
    {
        int axisIndex{0}; ///< 0 = A, 1 = B, 2 = C
        qreal base{0.0};
        QString label;
    };

    /// @brief The base a range pins each axis at — one per slot. Zero means the range
    ///        pins nothing there and the document's own value stands.
    struct OExportFixedBases
    {
        qreal a{0.0};
        qreal b{0.0};
        qreal c{0.0};
    };

    OExportGeometry(VContainer *container, VPattern *doc,
                    const QSharedPointer<VMeasurements> &measurements);

    /// @brief The sizes a range covers, in order. Empty when the range does not
    ///        exist, is empty, or the measurements table carries no usable axis.
    ///
    /// It stops at naming the sizes on purpose: applying one is the application's
    /// own business, and it has exactly one way of doing it.
    auto RangeSizes(const QString &rangeName) -> QVector<OExportSizeRef>;

    /// @brief The value each axis is pinned at by a range — the geometry a range
    ///        describes, and the values the pieces must be resolved with.
    ///
    /// Walking a range means putting the document on each of its sizes; the axes it does
    /// not grade keep the value the range pins them at. Taking the document's own value
    /// instead sends an axis it does not carry back to the measurement file's base, and
    /// the exported geometry is then not the one the range names. The graded capture
    /// feeds its pieces exactly these values.
    auto RangeFixedBases(const QString &rangeName) -> OExportFixedBases;

    /// @brief Resolve every piece of the pattern at the size the document is on.
    ///
    /// Nothing about the measurements is touched: this is the geometry as Details
    /// and Layout already hold it. One layer comes back, never several.
    ///
    /// The caller is responsible for putting the document on the size it wants —
    /// through the path the application itself uses — before calling this.
    auto CollectCurrentSize(const QString &label) -> QVector<OExportSizeLayer>;

    /// @brief Why the last traversal produced nothing usable. Empty when it worked.
    ///
    /// A silent empty result is indistinguishable from a pattern with no piece:
    /// whoever calls this has to be able to say what actually happened.
    auto LastDiagnostic() const -> QString;

    /// @brief Whether the run is a cutting plan.
    ///
    /// A cutting plan lays pieces on fabric, and fabric has a grain — so the pieces are prepared
    /// here rather than later: their direction is resolved while the container is still at hand,
    /// and it is the only thing this flag changes today. A print run is left exactly as it was.
    void SetCuttingPlan(bool cutting);

    /// @brief La matière du plan de coupe que cette traversée prépare : tissu, ou doublure.
    ///
    /// **Ce n'est pas un réglage de mise en page, c'est la source des quantités.** Un run peut décrire
    /// deux plans — le tissu et la doublure — et une pièce porte une quantité de coupe par matière :
    /// c'est donc ici, et ici seulement, que se décide *quelles pièces* entrent dans le plan et
    /// *combien de fois* chacune est posée. Tout le reste de la traversée est indifférent à la matière.
    ///
    /// Sans plan de coupe, ce champ ne change rien — voir @ref SetCuttingPlan.
    void SetCuttingMaterial(OExportCuttingGuard::Material material);

    /// @brief Keep only these pieces; empty keeps every piece the layout holds.
    ///
    /// **This is how a cutting plan decomposes a group.** A group serves printing, where two
    /// pieces fill one shape on a sheet; on fabric a piece is cut once. The plan keeps the member
    /// the user chose and drops the others, so the footprint keyed on the group covers that one
    /// piece instead of the union of all its members.
    void SetPieceFilter(const QSet<quint32> &pieceIds);

    /// @brief Leave these pieces out of the run, whatever else it keeps. Empty leaves none out.
    ///
    /// **C'est ainsi que le choix de l'onglet Plotter devient de la géométrie.** Un plotter
    /// *décompose* les groupes — chaque membre est une pièce à couper — donc l'onglet ne choisit pas
    /// un membre, il en raye. D'où une liste d'EXCLUSIONS et non un filtre : `SetPieceFilter` répond
    /// « garde celles-ci », et ne sait pas dire « n'en garde aucune » — un ensemble vide y garde tout.
    /// Une exclusion vide, elle, n'exclut rien : c'est le comportement d'avant les cases.
    ///
    /// Les deux se composent : une pièce survit si le filtre la garde ET si l'exclusion ne la raye pas.
    void SetPieceExclusion(const QSet<quint32> &pieceIds);

    /// @brief Whether the fabric this cutting plan is laid on is folded in two.
    ///
    /// It decides two things here, and both are about the same piece: a piece cut on the fold is
    /// laid **folded** — half, its fold line on the fold — instead of being unfolded as a flat plan
    /// unfolds it; and a piece cut twice is laid **once**, because the fold mirrors it. Without a
    /// cutting plan this flag changes nothing.
    void SetFoldedFabric(bool folded);

    /// @brief La zone utile du tapis d'un plotter domestique, en pixels. **Nulle : pas de tapis.**
    ///
    /// Ce n'est pas un paramètre de mise en page, c'est une **contrainte de machine** : un plotter
    /// domestique découpe sur des tapis, et son logiciel refuse une forme plus grande que le sien.
    /// Une pièce qui dépasse est donc **découpée aux bords des tapis**, et chacun de ses morceaux
    /// devient une géométrie à part entière.
    ///
    /// Posé, ce tapis change la nature de ce que la traversée produit — des morceaux au lieu de
    /// pièces — et rien d'autre : la mise en page, elle, se décide ailleurs.
    void SetCuttingMat(qreal widthPx, qreal heightPx);

private:
    /// The graded axis of a range, and the sizes it covers.
    struct AxisSelection
    {
        int index{1};           ///< position of the graded axis among the table's dimensions
        QVector<qreal> bases{}; ///< the sizes of the range, on that axis
    };

    auto SelectAxis(const OSizerange &range) const -> AxisSelection;
    auto LabelOf(double base, int axisIndex) const -> QString;
    void CollectPieces(OExportSizeLayer &layer) const;
    auto GroupNameOf(quint32 pieceId) const -> QString;

    VContainer *m_container{nullptr};
    VPattern *m_doc{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};
    /// @brief Pieces to keep. Empty keeps them all — see SetPieceFilter.
    QSet<quint32> m_pieceFilter{};
    /// @brief Pieces to leave out. Empty leaves none out — see SetPieceExclusion.
    QSet<quint32> m_pieceExclusion{};
    /// @brief Whether this traversal prepares a cutting plan — see SetCuttingPlan.
    bool m_cuttingPlan{false};
    /// @brief La matière du plan de coupe que cette traversée prépare — voir SetCuttingMaterial.
    OExportCuttingGuard::Material m_cuttingMaterial{OExportCuttingGuard::Material::Fabric};
    /// @brief Whether that cutting plan is laid on folded fabric — see SetFoldedFabric.
    bool m_foldedFabric{false};
    /// @brief La zone utile du tapis de découpe, en pixels — voir SetCuttingMat. Nulle : pas de tapis.
    qreal m_matWidthPx{0.0};
    qreal m_matHeightPx{0.0};
    /// @brief Diagnostics are a side channel: they report on a const traversal.
    mutable QStringList m_diagnostics{};
};

#endif // OEXPORTGEOMETRY_H
