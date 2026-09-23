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

#include <QDebug>

#include <algorithm>

namespace
{
// Labels and bases are both doubles coming from the measurements table: they are
// compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;

/// Enough to identify the cause, few enough to stay readable in a message box.
constexpr int kMaxDiagnostics = 4;
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
void OExportGeometry::SetCuttingPlan(bool cutting)
{
    m_cuttingPlan = cutting;
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
            resolved = VLayoutPiece::Create(piece, id, target);
        }
        catch (const VException &e)
        {
            // A piece whose contour cannot be built is not nestable — it is dropped,
            // and said out loud rather than silently missing from the result.
            qWarning() << "Export mode: piece" << piece.GetName() << "skipped —" << e.ErrorMessage();
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
            // ── Le dépliage ───────────────────────────────────────────────────────────────────
            //
            // Une pièce au pli part **dépliée**, même si « Show unfolded piece » est décochée en
            // Details : on coupe la forme entière dans le tissu, et le pli est un repère de
            // patronage, pas une consigne de coupe. Le réglage de la pièce n'est pas touché — c'est
            // la destination qui force, exactement ce que le cadrage §4 nomme.
            //
            // Le getter du contour honore déjà ce drapeau : c'est donc la MÊME pièce que l'écran
            // dessine et que le moteur nidifie, et rien ne peut diverger.
            // ⚠️ Sur `geometry.piece`, JAMAIS sur `resolved` : la copie vers `geometry.piece` a déjà
            // eu lieu plus haut, et muter le local ne changeait rien — le drapeau partait à la
            // poubelle et la pièce sortait à moitié. C'est l'erreur qui a coûté ce chantier.
            geometry.piece.SetShowFullPiece(true);

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
            // de Sophie — le legacy retournait une pièce sans qu'on demande rien, et un plan de
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
            // l'horizontale au lieu de la verticale. Trouvé par Sophie.
            if (piece.IsForceFlipping())
            {
                const QPointF centre = geometry.piece.DetailBoundingRect().center();
                geometry.piece.Mirror(QLineF(centre.x(), centre.y(), centre.x(), centre.y() - 100.0));
                geometry.direction = OExportCuttingDirection::Mirrored(geometry.direction);
            }
        }

        // Forced alignment point, resolved against `target` while it still holds THIS size.
        geometry.alignPointX = OGraded::UnitAlignPointX(id, piece, target);

        // ── La quantité ─────────────────────────────────────────────────────────────────────────
        //
        // Le membre d'abord, sinon le groupe, sinon 1 — décision de Sophie. Rien dans ce chemin ne
        // lisait la quantité jusqu'ici : c'est la section Cut du cartouche, et une pièce se coupe
        // rarement une seule fois.
        //
        // Un run d'impression garde une entrée par pièce : une feuille porte une silhouette, et
        // c'est le moteur qui décide où. Seul un plan de coupe répète.
        int quantity = 1;
        if (m_cuttingPlan)
        {
            quantity = piece.GetCutFabricQty();

            const auto *groups = m_container->DataPieceGroups();
            if (quantity <= 0 && groups != nullptr && geometry.groupId != NULL_ID)
            {
                const auto groupIt = groups->constFind(geometry.groupId);
                if (groupIt != groups->constEnd())
                {
                    quantity = groupIt.value().m_cutFabricQty;
                }
            }

            if (quantity <= 0)
            {
                quantity = 1;
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

                // ⚠️ Même piège que pour `Force flipping` : la copie retournée a un droit-fil à
                // `180 − a`, et sans ceci elle serait tournée comme si elle était encore à `a`.
                one.direction = OExportCuttingDirection::Mirrored(one.direction);
            }

            layer.pieces.append(one);
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
