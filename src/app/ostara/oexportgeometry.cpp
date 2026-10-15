/************************************************************************
 **
 **  @file   oexportgeometry.cpp
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

#include "oexportgeometry.h"

#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "core/oexportcuttingguard.h"
#include "oexportfragment.h"
#include "oexportplotter.h"
#include "oexportpuzzlereference.h"
#include "ogradedgeometry.h"
#include "xml/vpattern.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vexception/vexception.h"
#include "../vlayout/vfoldline.h"
#include "../vlayout/vlayoutpiecepath.h"
#include "../vlayout/vlayoutpoint.h"

#include <QDebug>
#include <QLineF>
#include <QPainterPath>
#include <QPolygonF>
#include <QTransform>

#include <algorithm>

namespace
{
// Labels and bases are both doubles coming from the measurements table: they are
// compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;

/// Enough to identify the cause, few enough to stay readable in a message box.
constexpr int kMaxDiagnostics = 4;

/// Le repère de recollage : **un centimètre**. Assez grand pour se voir sur du tissu, assez petit
/// pour ne pas gêner. **Ce n'est pas un réglage** : rien dans l'interface ne le demande, et deux
/// plans du même patron doivent porter les mêmes repères pour se relire l'un contre l'autre.
///
/// Un centimètre pour TOUTES les formes du répertoire — le rond comme le carré — et non la taille
/// d'un rond : c'est la boîte où chaque forme se dessine. Sinon un carré paraîtrait plus gros que le
/// rond qu'il remplace, et un repère se reconnaît avant de se mesurer.
constexpr double kPuzzleReferenceMm = 10.0;

/// Ce qu'une pièce devient sur un tapis de découpe : **un morceau par cellule**.
///
/// Le morceau naît ICI, dans la géométrie, et non dans le peintre. C'est ce qui fait que l'aperçu du
/// canvas montre exactement ce qui sera coupé — écran et fichier disent la même chose — et que
/// l'écriture n'a plus rien à découper : elle écrit ce qu'on lui donne, un objet par morceau.
///
/// ⚠️ **Ce qui est remis à zéro, et pourquoi.**
///
/// `id` et `groupId` : un morceau n'est **pas** la pièce du modèle. Garder l'id de la mère ferait
/// résoudre son cartouche — un morceau porterait la carte d'identité de la pièce entière — et ferait
/// **remplacer sa grainline** par celle de la pièce, qui ne passe plus par lui. Et `groupId` à zéro
/// est ce qui **décompose le groupe** : `IsGroupedMember` répond faux, chaque membre est un objet.
/// C'est la réponse du plotter à une question que `cuttingPlan` porte aujourd'hui pour le tissu —
/// sans avoir à réutiliser ce drapeau, qui décide de dix choses sans rapport.
///
/// `alignDelta` : l'alignement fait correspondre une même pièce d'une taille à l'autre. Un run
/// plotter est en taille unique et ses morceaux ne se correspondent pas — l'écart est nul.
///
/// `decor` et `grainline` gelés : la pièce du modèle n'en a plus besoin ici, le morceau porte les
/// siens — son contour, et ses lignes.
/// Les polylignes d'une FORME : une marque est un contour, et le canal de découpe ne connaît que des
/// polylignes. C'est ici que la forme redevient des points, avant d'être coupée à la cellule.
///
/// ⚠️ Un sous-chemin fermé (une flèche, une lettre) revient avec son premier point répété à la fin :
/// la découpe le traite donc comme une polyligne qui se referme sur elle-même, et le contour est
/// conservé. Une plume trace des contours — elle ne remplit pas — donc ce qui est porté est
/// exactement ce qui sera tracé.
void AppendSubpaths(const QPainterPath &shape, QVector<QVector<QPointF>> &lines)
{
    for (const QPolygonF &subpath : shape.toSubpathPolygons())
    {
        if (subpath.size() < 2)
        {
            continue;
        }

        QVector<QPointF> points;
        points.reserve(subpath.size());
        for (const QPointF &point : subpath)
        {
            points.append(point);
        }

        lines.append(points);
    }
}

auto FragmentsOf(const OExportPieceGeometry &geometry, const OExportPlotter::Mat &mat)
    -> QVector<OExportPieceGeometry>
{
    QVector<OExportPieceGeometry> parts;

    // La ligne de coupe : la marge de couture quand elle existe, le contour sinon — exactement le
    // bord que le canvas dessine comme la pièce. Les crans y sont cousus (`togetherWithNotches`) :
    // sans eux, un morceau perdrait ses repères de montage, et ça ne se voit qu'à la machine.
    const QVector<VLayoutPoint> external = geometry.piece.IsSeamAllowance()
                                               ? geometry.piece.GetMappedFullSeamAllowancePoints(true)
                                               : geometry.piece.GetMappedFullContourPoints(true);

    QVector<QPointF> outline;
    outline.reserve(external.size());
    for (const VLayoutPoint &point : external)
    {
        outline.append(point);
    }

    // ── Les lignes de la pièce ──────────────────────────────────────────────────────────────────
    //
    // Une pièce, c'est un contour, une marge de couture, des crans sur le contour — **et des CHEMINS
    // INTERNES**. Les trois premiers font la ligne de coupe ; les derniers se DESSINENT, ils ne se
    // découpent pas. Et il n'appartient pas à Ostara d'en juger : le logiciel du plotter assigne
    // « couper » ou « dessiner » à chaque ligne du fichier, et le travail d'Ostara est de lui donner
    // la géométrie. Elles sont donc portées TOUTES, y compris celles que le modèle marque à couper.
    //
    // Mappées par la matrice de la pièce — le même geste que `OExportCuttingDirection::MappedFoldLine`,
    // et pour la même raison : une ligne se lit dans le repère où le contour est dessiné.
    QVector<QVector<QPointF>> lines;

    const QTransform matrix = geometry.piece.GetMatrix();
    for (const VLayoutPiecePath &path : geometry.piece.GetInternalPaths())
    {
        QVector<QPointF> points;
        points.reserve(path.Points().size());
        for (const VLayoutPoint &point : path.Points())
        {
            points.append(matrix.map(point));
        }

        if (points.size() >= 2)
        {
            lines.append(points);
        }
    }

    // ── Les MARQUES ─────────────────────────────────────────────────────────────────────────────
    //
    // ⚠️ **Une grainline n'est pas un trait, et une foldline non plus.** La première porte ses
    // flèches, la seconde sa marque et son libellé : c'est ainsi que l'application les dessine
    // PARTOUT — `CreateGrainlineItem` et `CreateFoldLineItem` — et c'est ce que l'utilisatrice
    // attend sur son papier. Porter l'AXE à la place donne deux traits nus : celui du droit-fil, et
    // pour une pièce au pli un trait posé sur le bord, puisque l'axe du pli EST le bord de la
    // demi-pièce.
    //
    // Les deux sont donc prises en FORME, exactement celles que ces deux fonctions dessinent.
    if (geometry.piece.IsGrainlineEnabled() && geometry.piece.IsGrainlineVisible())
    {
        AppendSubpaths(VLayoutPiece::GrainlinePath(geometry.piece.GetMappedGrainlineShape()), lines);
    }

    // La marque du pli vit dans le repère de la pièce — elle se mappe par la matrice, comme le
    // contour.
    //
    // ⚠️ **La marque, jamais le libellé.** `FoldLineMarkPoints` rend les FLÈCHES, là où
    // `FoldLinePath` y ajoute le texte du pli, vectorisé. Un fichier de découpe ne porte pas de
    // texte : le §4.2 le dit, et la règle vaut aussi pour des lettres tracées en contours — le
    // logiciel du plotter les graverait. Les flèches, elles, disent le pli, et le bord de la
    // demi-pièce le dit déjà par lui-même : l'axe du pli EST ce bord.
    for (const QVector<QPointF> &arrow : geometry.piece.FoldLine().FoldLineMarkPoints())
    {
        QVector<QPointF> points;
        points.reserve(arrow.size());
        for (const QPointF &point : arrow)
        {
            points.append(matrix.map(point));
        }

        if (points.size() >= 2)
        {
            lines.append(points);
        }
    }

    // ── La LIGNE MIROIR, quand la pièce est dépliée ─────────────────────────────────────────────
    //
    // Une pièce au pli que le mode Details affiche ENTIÈRE est dépliée ici aussi — même
    // `IsShowFullPiece`, lu par le même getter, et rien dans ce fichier ne le force. Ce qui manquait
    // au fichier, c'est ce que l'écran dessine alors **en plus** de la foldline : la ligne miroir,
    // l'axe du pli raccourci aux deux points où il rencontre le contour déplié.
    //
    // ⚠️ **Les deux mêmes conditions que l'écran, ni plus ni moins.** `RenderMirrorLine`
    // (`vtoolseamallowance.cpp`) et `PaintMirrorLine` (`vpgraphicspiece.cpp`) la gardent toutes deux
    // par `IsShowFullPiece() && IsShowMirrorLine()`. Un fichier de découpe dit ce que le patron dit :
    // il n'a pas sa propre idée de ce qu'est une pièce dépliée.
    //
    // Le getter applique lui-même la matrice de la pièce, et son repère est donc celui du contour et
    // des marques — pas celui des chemins internes, qui se mappent. `togetherWithNotches` suit le
    // contour pris plus haut : l'axe se raccourcit sur le bord qui découpe.
    if (geometry.piece.IsShowFullPiece() && geometry.piece.IsShowMirrorLine())
    {
        const QLineF mirror = geometry.piece.GetMappedCorrectedMirrorLine(true);
        if (!mirror.isNull())
        {
            lines.append(QVector<QPointF>{mirror.p1(), mirror.p2()});
        }
    }

    const OExportPlotter::PiecePlan plan =
        OExportPlotter::Plan(geometry.piece.MappedDetailBoundingRect(), mat);

    // ── Les repères de recollage ────────────────────────────────────────────────────────────────
    //
    // Une pièce coupée en plusieurs morceaux ne dit plus, une fois coupée, lequel va à côté de
    // lequel : le nom est dans le fichier, pas sur le tissu. Une FORME posée à cheval sur chaque
    // coupe le dit — chaque morceau emporte sa moitié, et deux moitiés ne reforment la forme qu'à une
    // seule position : le bon voisin, à la bonne place. Un trait droit, lui, glisserait le long de la
    // coupe et ne dirait rien.
    //
    // Le répertoire des formes et l'ordre des coupes vivent dans `OExportPuzzleReference` : la règle
    // est là-bas, et elle n'a qu'un lecteur.
    //
    // Ce sont des lignes comme les autres : la découpe à la cellule les sépare d'elle-même, chacune
    // du bon côté, et elles arrivent donc à l'écran comme au fichier sans une ligne de plus.
    lines += OExportPuzzleReference::Marks(outline, plan.turn, plan,
                                           ToPixel(kPuzzleReferenceMm, Unit::Mm));

    const QVector<OExportFragment::Part> split = OExportFragment::Split(outline, lines, plan);

    int index = 0;
    for (const OExportFragment::Part &part : split)
    {
        ++index;

        OExportPieceGeometry one = geometry;

        // Un morceau neuf, fait de son contour et de ses lignes. Rien d'autre : ni marge de couture
        // — elle EST devenue la ligne de coupe — ni allowance de nesting, ni crans séparés.
        QVector<VLayoutPoint> points;
        points.reserve(part.outline.size());
        for (const QPointF &point : part.outline)
        {
            // Tous les sommets en coin vif. Sans ça la déduplication d'un contour tolère un demi
            // millimètre et replierait un morceau étroit sur lui-même.
            VLayoutPoint corner(point);
            corner.SetTurnPoint(true);
            points.append(corner);
        }

        VLayoutPiece piece;
        piece.SetContourPoints(points);
        piece.SetName(one.name);

        QVector<VLayoutPiecePath> internal;
        internal.reserve(part.lines.size());
        for (const QVector<QPointF> &line : part.lines)
        {
            QVector<VLayoutPoint> linePoints;
            linePoints.reserve(line.size());
            for (const QPointF &point : line)
            {
                linePoints.append(VLayoutPoint(point));
            }
            internal.append(VLayoutPiecePath(linePoints));
        }
        piece.SetInternalPaths(internal);

        one.piece = piece;
        one.id = 0;
        one.groupId = 0;
        one.groupName.clear();
        one.alignDelta = QPointF();
        one.decor = OExportPieceDecor();
        one.grainline = OExportPieceGrainline();

        // ⚠️ **Le nom ET la clé viennent du nom de la PIÈCE, jamais de la clé du groupe.**
        //
        // Deux membres d'un groupe partagent la clé `g<groupId>` — et c'est voulu pour le nesting,
        // où le groupe est posé comme un bloc rigide. Mais un run plotter **décompose les groupes** :
        // chaque membre est un objet à part entière. Deux morceaux qui garderaient cette clé
        // commune seraient **dessinés l'un sur l'autre** — le canvas apparie les géométries par clé,
        // donc chaque placement dessinerait les deux.
        //
        // ⚠️ Et **pas de suffixe `_1`** : l'écrivain de Trame retire `_` + chiffres pour retrouver
        // le transform, et deux morceaux nommés ainsi retomberaient l'un sur l'autre. La clé est le
        // nom, qui est déjà unique.
        const QString suffix = split.size() > 1 ? QStringLiteral("-f%1").arg(index) : QString();
        one.name = geometry.name + suffix;
        one.key = one.name;

        parts.append(one);
    }

    return parts;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportGeometry::OExportGeometry(VContainer *container, VPattern *doc,
                                 const QSharedPointer<VMeasurements> &measurements)
  : m_container(container),
    m_doc(doc),
    m_measurements(measurements)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::RangeSizes(const QString &rangeName) -> QVector<OExportSizeRef>
{
    QVector<OExportSizeRef> sizes;
    m_diagnostics.clear();

    if (m_container == nullptr || m_doc == nullptr || m_measurements.isNull())
    {
        return sizes;
    }

    const QHash<QString, OSizerange> *ranges = m_container->SizeRanges();
    if (ranges == nullptr || !ranges->contains(rangeName))
    {
        return sizes;
    }

    const OSizerange range = ranges->value(rangeName);
    const AxisSelection selection = SelectAxis(range);

    sizes.reserve(selection.bases.size());
    for (const qreal base : selection.bases)
    {
        sizes.append({selection.index, base, LabelOf(base, selection.index)});
    }

    return sizes;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::RangeFixedBases(const QString &rangeName) -> OExportFixedBases
{
    OExportFixedBases fixed;

    if (m_container == nullptr || m_measurements.isNull())
    {
        return fixed;
    }

    const QHash<QString, OSizerange> *ranges = m_container->SizeRanges();
    if (ranges == nullptr || !ranges->contains(rangeName))
    {
        return fixed;
    }

    const OSizerange range = ranges->value(rangeName);
    const QList<MeasurementDimension_p> dimensions = m_measurements->Dimensions().values();

    // The range pins each axis as the label the user sees; the axis carries the
    // label → base correspondence, the same one the graded axis goes through.
    for (int i = 0; i < dimensions.size() && i < 3; ++i)
    {
        const QString label = range.axisValues.value(dimensions.at(i)->CustomName());
        if (label.isEmpty())
        {
            continue;
        }

        qreal base = 0.0;
        const DimesionLabels labels = dimensions.at(i)->Labels();
        for (auto it = labels.constBegin(); it != labels.constEnd(); ++it)
        {
            if (it.value() == label)
            {
                base = it.key();
                break;
            }
        }

        if (i == 0)
        {
            fixed.a = base;
        }
        else if (i == 1)
        {
            fixed.b = base;
        }
        else
        {
            fixed.c = base;
        }
    }

    return fixed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::CollectCurrentSize(const QString &label) -> QVector<OExportSizeLayer>
{
    QVector<OExportSizeLayer> layers;
    m_diagnostics.clear();

    if (m_container == nullptr)
    {
        return layers;
    }

    // Deliberately no EvaluateAt here. The document is already on the size the user
    // chose — Details set it, through its own drop-downs — so the geometry is read
    // as it stands, exactly like the graded capture does when it draws a size.
    OExportSizeLayer layer;
    layer.label = label;
    CollectPieces(layer);

    if (!layer.pieces.isEmpty())
    {
        layers.append(layer);
    }

    return layers;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::LastDiagnostic() const -> QString
{
    return m_diagnostics.join(QStringLiteral("\n"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGeometry::SetPieceFilter(const QSet<quint32> &pieceIds)
{
    m_pieceFilter = pieceIds;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGeometry::SetPieceExclusion(const QSet<quint32> &pieceIds)
{
    m_pieceExclusion = pieceIds;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGeometry::SetCuttingPlan(bool cutting)
{
    m_cuttingPlan = cutting;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGeometry::SetFoldedFabric(bool folded)
{
    m_foldedFabric = folded;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGeometry::SetCuttingMat(qreal widthPx, qreal heightPx)
{
    m_matWidthPx = widthPx;
    m_matHeightPx = heightPx;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::SelectAxis(const OSizerange &range) const -> AxisSelection
{
    AxisSelection selection;

    const QList<MeasurementDimension_p> dimensions = m_measurements->Dimensions().values();
    if (dimensions.isEmpty())
    {
        return selection;
    }

    // The range stores its graded axis as the axis' display name, not as an index.
    int index = -1;
    for (int i = 0; i < dimensions.size(); ++i)
    {
        if (dimensions.at(i)->CustomName() == range.sizeAxis)
        {
            index = i;
            break;
        }
    }
    if (index < 0)
    {
        // No match: fall back to the size axis, which is the second dimension of a
        // standard multisize table.
        index = dimensions.size() > 1 ? 1 : 0;
    }
    selection.index = index;

    const MeasurementDimension_p &dimension = dimensions.at(index);

    // The range stores its bounds as the labels the user sees; the axis carries the
    // label → base correspondence.
    qreal minBase = dimension->MinValue();
    qreal maxBase = dimension->MaxValue();
    const DimesionLabels labels = dimension->Labels();
    for (auto it = labels.constBegin(); it != labels.constEnd(); ++it)
    {
        if (it.value() == range.minValue)
        {
            minBase = it.key();
        }
        if (it.value() == range.maxValue)
        {
            maxBase = it.key();
        }
    }
    if (minBase > maxBase)
    {
        std::swap(minBase, maxBase);
    }

    const QVector<qreal> validBases = dimension->ValidBases();
    for (const qreal base : validBases)
    {
        if (base >= minBase - kBaseTolerance && base <= maxBase + kBaseTolerance)
        {
            selection.bases.append(base);
        }
    }

    return selection;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::LabelOf(double base, int axisIndex) const -> QString
{
    const QList<MeasurementDimension_p> dimensions = m_measurements->Dimensions().values();
    if (axisIndex >= 0 && axisIndex < dimensions.size())
    {
        const DimesionLabels labels = dimensions.at(axisIndex)->Labels();
        for (auto it = labels.constBegin(); it != labels.constEnd(); ++it)
        {
            if (qAbs(it.key() - base) < kBaseTolerance)
            {
                return it.value();
            }
        }
    }

    return QString::number(base);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGeometry::CollectPieces(OExportSizeLayer &layer) const
{
    const QHash<quint32, VPiece> *pieces = m_container->DataPieces();
    if (pieces == nullptr)
    {
        m_diagnostics.append(QStringLiteral("container returned no piece list"));
        return;
    }

    if (pieces->isEmpty())
    {
        m_diagnostics.append(QStringLiteral("container holds no piece"));
    }

    // The same filter Details and Layout apply: a piece the user excluded from the
    // layout has no business ending up in the bin.
    const QVector<DetailForLayout> details = SortDetailsForLayout(pieces, m_container, QString());

    for (const DetailForLayout &detail : details)
    {
        const quint32 id = detail.id;
        const VPiece &piece = detail.piece;

        // A cutting plan keeps only the pieces it will cut: the member it chose for each group,
        // and every piece outside one. The others are dropped here rather than later, so the
        // group's footprint is that single piece and not the union of all its members.
        if (not m_pieceFilter.isEmpty() && not m_pieceFilter.contains(id))
        {
            continue;
        }

        // Et une pièce DÉCOCHÉE dans l'onglet Plotter ne se coupe pas. C'est ici, et ici seulement,
        // que ce choix devient de la géométrie — et, comme tout ce que la traversée laisse de côté,
        // elle quitte AUSSI l'aperçu : le canvas et le fichier lisent les mêmes `layers`, et c'est
        // exactement ce qu'on veut d'eux. L'écran montre ce qui sera coupé, et une pièce écartée n'en
        // fait plus partie. Elle allège d'ailleurs le plan : un morceau de moins, c'est parfois un
        // tapis de moins.
        if (not m_pieceExclusion.isEmpty() && m_pieceExclusion.contains(id))
        {
            continue;
        }

        // ── Et le cut : un plan de coupe ne coupe que ce qui a une quantité de tissu ──────────────
        //
        // Un plan de coupe est un plan de TISSU : une pièce dont la section Cut est restée vide ne
        // se coupe pas. Elle est « in layout », elle reste dans le patron, mais elle ne prend
        // aucune part au plan — pas une place, pas un aperçu, pas une ligne du fichier.
        //
        // Écartée ICI, avant d'être résolue : une pièce qu'on ne coupe pas ne coûte rien à
        // construire, et surtout elle ne peut plus faire échouer le run (un pli qui n'est pas un
        // bord, par exemple — le moteur refuse alors le plan entier).
        //
        // La quantité est lue une fois pour toutes : `CutQuantity` fait exactement la lecture que le
        // bloc « quantité » appliquait plus bas — la pièce, sinon son groupe — avec une différence,
        // et c'est la règle : 0, la valeur d'une pièce jamais renseignée, veut dire « pas de coupe »
        // au lieu de « une fois ».
        int cutQuantity = 1;
        if (m_cuttingPlan)
        {
            cutQuantity = OExportCuttingGuard::CutQuantity(id, m_container);
            if (cutQuantity <= 0)
            {
                continue;
            }
        }

        // A piece is resolved against the container of the drawing tool that carries
        // it — that is what the graded capture does (MainWindowsNoGUI::PrepareDetailsForLayout
        // passes tool->getData()), and it is NOT necessarily the pattern container the
        // export was handed. Resolving against the wrong container leaves the nodes
        // unresolved: the piece comes back as a handful of points around the origin
        // instead of its real outline.
        const VContainer *target = m_container;
        auto *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::getTool(id));
        if (tool != nullptr && tool->getData() != nullptr)
        {
            target = tool->getData();
        }

        VLayoutPiece resolved;
        try
        {
            resolved = VLayoutPiece::Create(piece, id, target, m_doc);
        }
        catch (const VException &e)
        {
            // A piece whose contour cannot be built is not nestable — it is dropped,
            // and said out loud rather than silently missing from the result.
            qWarning() << "Export mode: piece" << piece.GetName() << "skipped -" << e.ErrorMessage();
            if (m_diagnostics.size() < kMaxDiagnostics)
            {
                m_diagnostics.append(QStringLiteral("piece '%1': %2").arg(piece.GetName(), e.ErrorMessage()));
            }
            continue;
        }

        // isNull() does NOT mean "the piece is broken": it answers "this piece has no
        // nesting envelope yet" (m_layoutWidth is only set by VBank::PrepareDetails,
        // which VLayoutPiece::Create never calls). What matters here is that the piece
        // has a contour at all, and that its seam allowance was actually built when it
        // is supposed to have one.
        const bool hasShape = !resolved.GetContourPoints().isEmpty()
                              && (!resolved.IsSeamAllowance() || !resolved.GetSeamAllowancePoints().isEmpty());
        if (!hasShape)
        {
            if (m_diagnostics.size() < kMaxDiagnostics)
            {
                m_diagnostics.append(QStringLiteral("piece '%1': no usable contour").arg(piece.GetName()));
            }
            continue;
        }

        if (m_diagnostics.isEmpty())
        {
            // The first piece that got through: its contour sizes tell whether the
            // geometry was actually recomputed for this size.
            m_diagnostics.append(QStringLiteral("piece '%1': contour %2 pts, seam allowance %3 pts, external %4 pts, %5")
                                     .arg(piece.GetName())
                                     .arg(resolved.GetContourPoints().size())
                                     .arg(resolved.GetSeamAllowancePoints().size())
                                     .arg(resolved.GetExternalContourPoints().size())
                                     .arg(tool == nullptr ? QStringLiteral("outil introuvable")
                                                          : (target == m_container
                                                                 ? QStringLiteral("conteneur = patron")
                                                                 : QStringLiteral("conteneur = outil (différent)"))));
        }

        OExportPieceGeometry geometry;
        geometry.name = piece.GetName();
        geometry.id = id;
        geometry.groupId = m_container->GetPieceGroupId(id);
        geometry.groupName = GroupNameOf(id);

        // The footprint key is the group's ID, not its name. Group names are not unique
        // — nothing in the application enforces it — so two groups called "Groupe" used
        // to be unioned into a single footprint. Details and Graded key on the id too.
        geometry.key = geometry.groupId == 0 ? geometry.name : QStringLiteral("g%1").arg(geometry.groupId);
        geometry.piece = resolved;

        if (m_cuttingPlan)
        {
            // ── Le pli, puis le dépliage ──────────────────────────────────────────────────────
            //
            // Une pièce au pli part **dépliée** : sur un plan déplié, on coupe la forme entière dans
            // le tissu, et le pli est un repère de patronage, pas une consigne de coupe.
            //
            // ⚠️ **Sauf sur du tissu plié.** Là, c'est la moitié qu'on trace sur le tissu plié, et
            // le pli donne la seconde : la pièce reste donc pliée, et sa foldline tombe sur le bord
            // du tissu. Le réglage de la pièce en Details n'est pas touché — c'est la destination
            // qui force, dans un sens comme dans l'autre, exactement ce que le cadrage §4 nomme.
            //
            // Le getter du contour honore déjà ce drapeau : c'est donc la MÊME pièce que l'écran
            // dessine et que le moteur nidifie, et rien ne peut diverger.
            // ⚠️ Sur `geometry.piece`, JAMAIS sur `resolved` : la copie vers `geometry.piece` a déjà
            // eu lieu plus haut, et muter le local ne changeait rien — le drapeau partait à la
            // poubelle et la pièce sortait à moitié. C'est l'erreur qui a coûté ce chantier.
            //
            // ⚠️ Et « au pli » se lit par `FoldLine` — la SEULE lecture du pli, celle que le
            // résolveur de direction emploie déjà. Jamais sur `FoldLineType` : il naît à
            // `TwoArrowsTextAbove`, donc « le type n'est pas None » est vrai de toute pièce jamais
            // créée. C'est le défaut qui a rendu le garde-fou muet.
            geometry.onFold = not OExportCuttingDirection::FoldLine(resolved).isNull();
            geometry.piece.SetShowFullPiece(not (m_foldedFabric and geometry.onFold));

            // ⚠️ The GROUP first, and the piece only when the group has nothing: a member's own
            // grainline can be enabled while the direction belongs to the group — Create does not
            // mask it, the masking is done on the item. Read here rather than at nesting time
            // because this is the only place that still holds the container.
            geometry.direction = OExportCuttingDirection::PieceDirection(resolved);

            const auto *groups = m_container->DataPieceGroups();
            if (groups != nullptr && geometry.groupId != NULL_ID)
            {
                const auto groupIt = groups->constFind(geometry.groupId);
                if (groupIt != groups->constEnd())
                {
                    const OExportCuttingDirection::Direction groupDirection =
                        OExportCuttingDirection::GroupDirection(groupIt.value());
                    if (groupDirection.known)
                    {
                        geometry.direction = groupDirection;
                    }
                }
            }

            // ── Le miroir — APRÈS la direction, jamais avant ──────────────────────────────────
            //
            // Pas de miroir sauf demande explicite : `Force flipping` est le seul chemin. Décision
            // — le legacy retournait une pièce sans qu'on demande rien, et un plan de
            // coupe qui retourne tout seul est faux sans le dire.
            //
            // ⚠️ Le miroir va dans la MATRICE, et les deux lecteurs doivent l'appliquer : le moteur
            // par le getter MAPPÉ du contour, l'écran en composant la matrice de placement au lieu
            // de l'écraser. Poser le drapeau `SetVerticallyFlipped` ne mire rien — il ne fait que
            // renverser l'ordre des points.
            //
            // ⚠️ Et il vient **après** la direction, parce qu'il en CHANGE l'angle : la matrice ne
            // touche pas la ligne de droit-fil, donc le moteur lirait l'angle d'avant le miroir et
            // tournerait la pièce comme si elle n'était pas retournée. Un biais à 45° sortait à
            // l'horizontale au lieu de la verticale. Trouvé.
            if (piece.IsForceFlipping())
            {
                const QPointF centre = geometry.piece.DetailBoundingRect().center();
                geometry.piece.Mirror(QLineF(centre.x(), centre.y(), centre.x(), centre.y() - 100.0));
                geometry.mirrorX = centre.x();
                geometry.direction = OExportCuttingDirection::Mirrored(geometry.direction);
            }
        }

        // Forced alignment point, resolved against `target` while it still holds THIS size.
        geometry.alignPointX = OGraded::UnitAlignPointX(id, piece, target);

        // ── La quantité ─────────────────────────────────────────────────────────────────────────
        //
        // Elle a été lue à l'entrée de la boucle, avec le cut : le membre d'abord, sinon le groupe.
        // Il n'y a plus de repli « sinon 1 » ici — une pièce sans quantité a déjà quitté le plan —
        // et ce qui reste vaut donc au moins 1.
        //
        // Un run d'impression garde une entrée par pièce : une feuille porte une silhouette, et
        // c'est le moteur qui décide où. Seul un plan de coupe répète.
        int quantity = cutQuantity;
        if (m_cuttingPlan)
        {
            // ── Et sur du tissu plié, la moitié ──────────────────────────────────────────────
            //
            // ⚠️ **Une pièce hors pli se coupe dans les DEUX épaisseurs** : une seule pose donne
            // donc deux pièces, la seconde étant le miroir de la première — c'est le pli qui la
            // fait. Poser « cut » fois en donnerait le double. C'est la moitié qu'on pose,
            // **arrondie au-dessus** : 1 reste 1 (la pièce sera coupée en double, et c'est sans
            // conséquence), 3 devient 2, 5 devient 3. Décidé.
            //
            // Une pièce **au pli** ne se divise pas : une pose sur le pli donne UNE pièce entière,
            // pas deux — le pli n'y ajoute pas une seconde pièce, il complète la première.
            if (m_foldedFabric and not geometry.onFold)
            {
                quantity = (quantity + 1) / 2;
            }
        }

        for (int copy = 0; copy < quantity; ++copy)
        {
            OExportPieceGeometry one = geometry;

            // ⚠️ Chaque copie a **son propre nom ET sa propre clé**. `Footprint::compute` regroupe
            // sur le nom pour une pièce isolée : même nom = une seule empreinte, et la copie
            // n'existe nulle part — sans qu'aucune erreur ne soit levée. Même clé = les deux
            // dessinées empilées, et le second placement réserve une zone vide.
            //
            // ⚠️ Et pas de suffixe `_1` : `NestedSvgWriter::stripDedupSuffix` le RETIRE pour
            // retrouver le transform — deux copies nommées ainsi retomberaient sur le même
            // placement à la relecture.
            if (copy > 0)
            {
                const QString suffix = QStringLiteral("-c%1").arg(copy + 1);
                one.name = geometry.name + suffix;
                one.key = geometry.key + suffix;
            }

            // Une copie sur deux en miroir, quand la copie symétrique est demandée — le motif de
            // HandleSymmetricalCopy (libs/vlayout/vbank.cpp), qui retourne les index impairs.
            if (copy % 2 == 1 && piece.IsSymmetricalCopy())
            {
                const QPointF centre = one.piece.DetailBoundingRect().center();
                one.piece.Mirror(QLineF(centre.x(), centre.y(), centre.x(), centre.y() - 100.0));
                one.mirrorX = centre.x();

                // ⚠️ Même piège que pour `Force flipping` : la copie retournée a un droit-fil à
                // `180 − a`, et sans ceci elle serait tournée comme si elle était encore à `a`.
                one.direction = OExportCuttingDirection::Mirrored(one.direction);
            }

            // Un tapis de découpe découpe la pièce : elle devient plusieurs morceaux, un par
            // cellule, et chacun est une géométrie à part entière — c'est ce qui donnera un objet
            // distinct dans le fichier. Sans tapis, la pièce reste entière.
            if (m_matWidthPx > 0.0 && m_matHeightPx > 0.0)
            {
                layer.pieces += FragmentsOf(one, {m_matWidthPx, m_matHeightPx});
            }
            else
            {
                layer.pieces.append(one);
            }

            // ── Et la variante EN MIROIR d'une pièce au pli ────────────────────────────────────
            //
            // Une pièce au pli est **dessinée** pour un bord — gauche ou droite — et c'est sa
            // nature. Mais le miroir (flip vertical) lui ouvre l'autre bord : mise en miroir, une
            // pièce destinée à gauche va sur le bord droit. C'est ce qui permet au nesting de
            // remplir indifféremment les deux bandes, au lieu de rejeter sous elles tout ce qui est
            // du mauvais côté — et c'est de la hauteur gagnée.
            //
            // ⚠️ **Le moteur ne sait pas refléter** : sa banque est purement rotationnelle, et un
            // placement n'a ni miroir ni flip. La variante doit donc EXISTER en géométrie — comme
            // une copie — et c'est la même mécanique que `Force flipping` et la copie symétrique :
            // le miroir va dans la matrice, que le moteur lit par le contour MAPPÉ et que l'écran
            // compose au lieu de l'écraser.
            //
            // ⚠️ Une variante par pose, et **le nesting n'en garde qu'une** : celle qui correspond
            // au côté du rectangle où la pose atterrit. L'autre n'entre ni dans le résultat ni dans
            // le dessin — le canvas ne dessine que ce que le nesting a gardé. Les deux clés
            // diffèrent, sans quoi `Footprint::compute` les unirait en une seule empreinte, et la
            // pose serait une fois et demie.
            if (m_foldedFabric and one.onFold)
            {
                OExportPieceGeometry twin = one;
                twin.name = one.name + QStringLiteral("-m");
                twin.key = one.key + QStringLiteral("-m");
                twin.mirrored = true;

                const QPointF centre = twin.piece.DetailBoundingRect().center();
                twin.piece.Mirror(QLineF(centre.x(), centre.y(), centre.x(), centre.y() - 100.0));
                twin.mirrorX = centre.x();
                twin.direction = OExportCuttingDirection::Mirrored(twin.direction);

                layer.pieces.append(twin);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGeometry::GroupNameOf(quint32 pieceId) const -> QString
{
    const quint32 groupId = m_container->GetPieceGroupId(pieceId);
    if (groupId == 0)
    {
        return {};
    }

    const QHash<quint32, OPieceGroup> *groups = m_container->DataPieceGroups();
    if (groups == nullptr)
    {
        return {};
    }

    return groups->value(groupId).GetName();
}
