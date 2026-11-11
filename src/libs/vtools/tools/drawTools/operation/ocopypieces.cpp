/************************************************************************
 **
 **  @file   ocopypieces.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  The pieces a copy inherits from the curve it was made of.
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

#include "ocopypieces.h"

#include <QDomElement>
#include <QDomNodeList>
#include <QMap>
#include <QScopedPointer>
#include <QSet>
#include <QSharedPointer>
#include <QtDebug>

#include "../../../tools/toolsdef.h"
#include "../../vdatatool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../toolcurve/osplinepathportions.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vgeometry/vabstractarc.h"
#include "../vgeometry/vabstractcubicbezier.h"
#include "../vgeometry/vabstractcubicbezierpath.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vpatterndb/vcontainer.h"
#include "ocopylineage.h"
#include "vabstractoperation.h"
#include "../ifc/exception/vexceptionbadid.h"

const QString OCopyPieces::AttrPieceIds = QStringLiteral("pieceIds");

namespace
{
using StoredPieces = QMap<QString, OSplinePathPortions::StoredPiece>;

//---------------------------------------------------------------------------------------------------------------------
/** @brief The cubics a curve is drawn with, in order: the sub-splines of a path, the curve itself for a spline. */
auto Cubics(const QSharedPointer<VAbstractCurve> &curve) -> QVector<QSharedPointer<VAbstractCubicBezier>>
{
    QVector<QSharedPointer<VAbstractCubicBezier>> cubics;

    if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(curve); not path.isNull())
    {
        for (vsizetype i = 1; i <= path->CountSubSpl(); ++i)
        {
            cubics.append(QSharedPointer<VSpline>::create(path->GetSpline(i)));
        }
        return cubics;
    }

    if (const auto cubic = qSharedPointerDynamicCast<VAbstractCubicBezier>(curve); not cubic.isNull())
    {
        cubics.append(cubic);
    }

    return cubics;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The length along @a copy of the place a length @a length along @a base was carried to.
 *
 * The cubic the place lies on, and the parameter it has on that cubic: an operation carries every cubic of a curve
 * whole, so the same cubic at the same parameter is the image of the place. A rotation, a move, a mirror keep the
 * length too; the smoothing of a slash-and-spread does not, and this is the answer it already gives a point posed on
 * a curve (OSpreadTransform::AttachPoints). A curve drawn with no cubic — an arc — is carried rigidly: its length is
 * kept, in proportion.
 */
auto CarriedLength(const QSharedPointer<VAbstractCurve> &base, qreal length, const QSharedPointer<VAbstractCurve> &copy)
    -> qreal
{
    const QVector<QSharedPointer<VAbstractCubicBezier>> baseCubics = Cubics(base);
    const QVector<QSharedPointer<VAbstractCubicBezier>> copyCubics = Cubics(copy);

    if (baseCubics.isEmpty() || baseCubics.size() != copyCubics.size())
    {
        const qreal full = base->GetLength();
        return qFuzzyIsNull(full) ? length : length * copy->GetLength() / full;
    }

    qreal before = 0;
    qreal carried = 0;

    for (int k = 0; k < baseCubics.size(); ++k)
    {
        const qreal cubicLength = baseCubics.at(k)->GetLength();

        if (length <= before + cubicLength || k == baseCubics.size() - 1)
        {
            const qreal t = baseCubics.at(k)->GetParmT(qBound(0.0, length - before, cubicLength));
            return carried + copyCubics.at(k)->RealLengthByT(t);
        }

        before += cubicLength;
        carried += copyCubics.at(k)->GetLength();
    }

    return carried;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The two halves of @a copy at @a length, cut by the rule the cut tools follow — a length past an end puts
 *  the cut on the curve's prolongation, which becomes one of the two halves. */
auto CutCopy(const QSharedPointer<VAbstractCurve> &copy, qreal length)
    -> QPair<QSharedPointer<VAbstractCurve>, QSharedPointer<VAbstractCurve>>
{
    if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(copy); not path.isNull())
    {
        VSplinePath *first = nullptr;
        VSplinePath *second = nullptr;
        const QScopedPointer<VPointF> point(VToolCutSplinePath::CutSplinePath(length, path, QString(), &first, &second));

        return {QSharedPointer<VSplinePath>(first), QSharedPointer<VSplinePath>(second)};
    }

    if (const auto cubic = qSharedPointerDynamicCast<VAbstractCubicBezier>(copy); not cubic.isNull())
    {
        const VToolCutSpline::SubCurves curves = VToolCutSpline::SplitCurve(length, cubic);

        VSpline first;
        VSpline second;
        VToolCutSpline::BuildSubCurves(curves, cubic, VPointF(curves.cutPoint), first, second);

        return {QSharedPointer<VSpline>::create(first), QSharedPointer<VSpline>::create(second)};
    }

    if (copy->getType() == GOType::Arc)
    {
        VArc first;
        VArc second;
        copy.staticCast<VAbstractArc>()->CutArc(length, &first, &second, QString());
        return {QSharedPointer<VArc>::create(first), QSharedPointer<VArc>::create(second)};
    }

    if (copy->getType() == GOType::EllipticalArc)
    {
        VEllipticalArc first;
        VEllipticalArc second;
        copy.staticCast<VAbstractArc>()->CutArc(length, &first, &second, QString());
        return {QSharedPointer<VEllipticalArc>::create(first), QSharedPointer<VEllipticalArc>::create(second)};
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Where a cut lies on its own curve, and the length to hand the cutter on a copy of that curve. */
struct CutPlace
{
    qreal length{0};
    /// 0 on the curve, 1 before its start (the first half is the prolongation), 2 past its end (the second is).
    int prolonged{0};
    bool known{false};
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Where the cut that made @a first and @a second lies on @a base, told by where its halves meet the curve.
 *
 * The length the cut was given is a formula only the cut can evaluate; its halves say the rest. Past the end, the
 * first half is the whole curve and the second starts where the curve ends; before the start, the second half is the
 * whole curve and the first ends where it starts; anywhere else the cut is on the curve. Told by the ends rather than
 * by lengths alone: two lengths compared within a tolerance confused a short curve with its prolongation.
 */
auto PlaceOf(const QSharedPointer<VAbstractCurve> &base, const QSharedPointer<VAbstractCurve> &first,
             const QSharedPointer<VAbstractCurve> &second, const QSharedPointer<VAbstractCurve> &copy) -> CutPlace
{
    const QVector<QPointF> basePoints = base->GetPoints();
    const QVector<QPointF> firstPoints = first->GetPoints();
    const QVector<QPointF> secondPoints = second->GetPoints();

    if (basePoints.isEmpty() || firstPoints.isEmpty() || secondPoints.isEmpty())
    {
        return {};
    }

    const qreal full = base->GetLength();
    const qreal tolerance = qMax(full * 1e-3, accuracyPointOnLine);
    auto Same = [](const QPointF &a, const QPointF &b) -> bool
    { return QLineF(a, b).length() <= accuracyPointOnLine; };

    // Past the end: the second half leaves the curve at its last point, and is not the empty piece of a cut placed
    // exactly on that point.
    if (Same(secondPoints.constFirst(), basePoints.constLast()) && second->GetLength() > tolerance
        && qAbs(first->GetLength() - full) <= tolerance)
    {
        return {.length = copy->GetLength() + second->GetLength(), .prolonged = 2, .known = true};
    }

    // Before the start: the first half reaches the curve at its first point.
    if (Same(firstPoints.constLast(), basePoints.constFirst()) && first->GetLength() > tolerance
        && qAbs(second->GetLength() - full) <= tolerance)
    {
        // A negative length is measured from the end of the copy (OBezierExtension::Normalize): this one lands the
        // first half's length before its start.
        return {.length = -(copy->GetLength() + first->GetLength()), .prolonged = 1, .known = true};
    }

    return {.length = CarriedLength(base, first->GetLength(), copy), .prolonged = 0, .known = true};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Whether the curve a copy was made of is one of the kinds a copy cuts. */
auto IsCurve(GOType type) -> bool
{
    return type == GOType::Spline || type == GOType::CubicBezier || type == GOType::SplinePath
           || type == GOType::CubicBezierPath || type == GOType::Arc || type == GOType::EllipticalArc;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OCopyPieces::CutKey(quint32 cutId, int half) -> QString
{
    return QStringLiteral("c%1-%2").arg(cutId).arg(half);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyPieces::NodeKey(const QString &runKey) -> QString
{
    return QLatin1Char('n') + runKey;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyPieces::Build(VAbstractPattern *doc, VContainer *data, quint32 operationId) -> Pieces
{
    Pieces result;

    if (doc == nullptr || data == nullptr || operationId == NULL_ID)
    {
        return result;
    }

    const QDomElement operation = doc->FindElementById(operationId, VAbstractPattern::TagOperation);
    if (operation.isNull())
    {
        return result; // created from the dialog, not written yet: the operation builds them once it is
    }

    const QDomElement destinationTag = operation.firstChildElement(VAbstractOperation::TagDestination);

    // Read once for the whole operation, and only when one of its copies is a curve: most operations copy points.
    QVector<SourceItem> source;
    QHash<quint32, QVector<QDomElement>> cutsByCurve;
    bool curvesRead = false;

    int rank = -1;
    for (QDomElement item = destinationTag.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
         item = item.nextSiblingElement(VAbstractOperation::TagItem))
    {
        ++rank;

        const quint32 copyId = VDomDocument::GetParametrUInt(item, AttrIdObject, NULL_ID_STR);
        const QSharedPointer<VAbstractCurve> copy = OSplinePathPortions::CurveOf(data, copyId);

        if (copy.isNull() || not IsCurve(copy->getType()))
        {
            continue;
        }

        if (not curvesRead)
        {
            source = VAbstractOperation::ExtractSourceData(operation);
            cutsByCurve = OCopyLineage::CutsByCurve(doc);
            curvesRead = true;
        }

        if (rank >= source.size())
        {
            continue;
        }

        const StoredPieces stored = OSplinePathPortions::DecodePieces(item.attribute(AttrPieceIds));
        StoredPieces kept;
        QVector<Piece> pieces;
        bool replaced = false;

        // The names the halves of this copy already answer to — the ones written beside their ids, and the ones given
        // in this pass — so a new half never takes a name another half of the copy has: two cuts on one curve give
        // its copy `__ls`, then `__ls_2`.
        QSet<QString> takenNames;
        for (const OSplinePathPortions::StoredPiece &piece : stored)
        {
            if (not piece.name.isEmpty())
            {
                takenNames.insert(piece.name);
            }
        }

        auto FreeName = [data, &takenNames](const QString &wanted) -> QString
        {
            QString name = wanted;
            for (int n = 2; takenNames.contains(name) || not data->IsUnique(name); ++n)
            {
                name = wanted + QLatin1Char('_') + QString::number(n);
            }
            takenNames.insert(name);
            return name;
        };

        try
        {
            // The family of the copy: the curve it was made of first, the curve at the top of the family last.
            QVector<quint32> family = OCopyLineage::Ancestors(doc, data, copyId);
            if (family.isEmpty())
            {
                // The operation is being built: its element already names the source, the container's family not yet.
                family.append(source.at(rank).id);
            }

            // **The boxes**, read on the curve at the top of the family: the only one that has them.
            if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(copy); not path.isNull())
            {
                const vsizetype nodes = path->CountSubSpl() + 1;
                const QVector<int> cutNodes = OCopyLineage::CutNodesOf(doc, family.constLast());
                const QVector<OSplinePathPortions::Run> runs = cutNodes.isEmpty()
                                                                   ? QVector<OSplinePathPortions::Run>{}
                                                                   : OSplinePathPortions::Runs(cutNodes, nodes);

                int index = 0;
                for (const OSplinePathPortions::Run &run : runs)
                {
                    ++index;
                    const QString key = NodeKey(OSplinePathPortions::RunKey(run));
                    const QSharedPointer<VAbstractCurve> piece = OSplinePathPortions::RunCurve(*path, run);
                    if (piece.isNull())
                    {
                        continue;
                    }

                    OSplinePathPortions::MarkPiece(*piece, *copy, operationId,
                                                   copy->HeadlessName() + OSplinePathPortions::PortionSuffix(index));
                    const quint32 id = OSplinePathPortions::StorePiece(data, piece, stored.value(key).id);
                    if (id == NULL_ID)
                    {
                        continue;
                    }

                    kept.insert(key, OSplinePathPortions::StoredPiece{id, QString()});
                    pieces.append({.id = id, .copyId = copyId, .shown = true});
                    replaced = true;
                }

                // **The pieces kept for their users**: a run of the boxes that stopped being one, and that something
                // is built on (PruneUnused left it). Same id, same name, the geometry of the copy it still lies on;
                // not drawn.
                for (auto it = stored.cbegin(); it != stored.cend(); ++it)
                {
                    OSplinePathPortions::Run run;
                    if (kept.contains(it.key()) || not it.key().startsWith(QLatin1Char('n'))
                        || not OSplinePathPortions::ParseRunKey(it.key().mid(1), run) || run.to >= nodes)
                    {
                        continue;
                    }

                    const QSharedPointer<VAbstractCurve> piece = OSplinePathPortions::RunCurve(*path, run);
                    if (piece.isNull())
                    {
                        continue;
                    }

                    QString suffix = it.value().name;
                    if (const QSharedPointer<VAbstractCurve> held = OSplinePathPortions::CurveOf(data, it.value().id);
                        suffix.isEmpty() && not held.isNull())
                    {
                        suffix = held->HeadlessName();
                    }
                    if (suffix.isEmpty())
                    {
                        suffix = OSplinePathPortions::KeptSuffix(copy->HeadlessName(), run);
                    }

                    OSplinePathPortions::MarkPiece(*piece, *copy, operationId, suffix);
                    const quint32 id = OSplinePathPortions::StorePiece(data, piece, it.value().id);
                    if (id != NULL_ID)
                    {
                        kept.insert(it.key(), OSplinePathPortions::StoredPiece{id, suffix});
                        pieces.append({.id = id, .copyId = copyId, .shown = false});
                    }
                }
            }

            // **The cuts**, placed on any curve of the family: the copy is cut at the place each cut point was carried
            // to. A cut not built yet — it comes later in the file — keeps the ids and names the file gave its halves,
            // and cuts the copy when it is built (FollowCut). A cut deleted is gone with its halves: deleting it was
            // refused while one of them was in use.
            for (const quint32 curveId : std::as_const(family))
            {
                const QSharedPointer<VAbstractCurve> base = OSplinePathPortions::CurveOf(data, curveId);

                for (const QDomElement &cut : cutsByCurve.value(curveId))
                {
                    const quint32 cutId = VDomDocument::GetParametrId(cut);
                    const QSharedPointer<VAbstractCurve> first =
                        OSplinePathPortions::CurveOf(data, VDomDocument::GetParametrUInt(cut, AttrSegment1Id, NULL_ID_STR));
                    const QSharedPointer<VAbstractCurve> second =
                        OSplinePathPortions::CurveOf(data, VDomDocument::GetParametrUInt(cut, AttrSegment2Id, NULL_ID_STR));

                    const CutPlace place = (base.isNull() || first.isNull() || second.isNull() || not data->HasObject(cutId))
                                               ? CutPlace{}
                                               : PlaceOf(base, first, second, copy);

                    const auto [half1, half2] =
                        place.known ? CutCopy(copy, place.length)
                                    : QPair<QSharedPointer<VAbstractCurve>, QSharedPointer<VAbstractCurve>>{};

                    if (half1.isNull() || half2.isNull())
                    {
                        for (const int half : {1, 2})
                        {
                            if (const QString key = CutKey(cutId, half); stored.contains(key))
                            {
                                kept.insert(key, stored.value(key));
                            }
                        }
                        continue;
                    }

                    // Each half keeps the name it was given the first time, written beside its id: a formula reads it
                    // by that name, and a name computed afresh would move when a cut is added before it. The first
                    // name is the copy's own plus the mark of a cut's halves, made unique in the pattern — a cut the
                    // user placed on the copy itself may have taken it already.
                    const QString key1 = CutKey(cutId, 1);
                    const QString key2 = CutKey(cutId, 2);
                    const QString name1 = stored.value(key1).name.isEmpty()
                                              ? FreeName(copy->HeadlessName() + CopySuffix::SubCurveLeft)
                                              : stored.value(key1).name;
                    const QString name2 = stored.value(key2).name.isEmpty()
                                              ? FreeName(copy->HeadlessName() + CopySuffix::SubCurveRight)
                                              : stored.value(key2).name;

                    OSplinePathPortions::MarkPiece(*half1, *copy, operationId, name1);
                    OSplinePathPortions::MarkPiece(*half2, *copy, operationId, name2);

                    // The prolongation is not the curve: it is drawn with the look its cut gives it.
                    const QString prolongationColor = cut.attribute(AttrLineColor, ColorBlack);
                    const QString prolongationType = cut.attribute(AttrTypeLine, TypeLineLine);
                    if (place.prolonged == 1)
                    {
                        half1->SetColor(prolongationColor);
                        half1->SetPenStyle(prolongationType);
                    }
                    else if (place.prolonged == 2)
                    {
                        half2->SetColor(prolongationColor);
                        half2->SetPenStyle(prolongationType);
                    }

                    const bool showTwo = VDomDocument::GetParametrBool(cut, AttrShowTwoSegments, falseStr);

                    const quint32 id1 = OSplinePathPortions::StorePiece(data, half1, stored.value(key1).id);
                    const quint32 id2 = OSplinePathPortions::StorePiece(data, half2, stored.value(key2).id);

                    if (id1 != NULL_ID)
                    {
                        kept.insert(key1, OSplinePathPortions::StoredPiece{id1, name1});
                        pieces.append({.id = id1, .copyId = copyId, .shown = showTwo || place.prolonged == 1});
                    }

                    if (id2 != NULL_ID)
                    {
                        kept.insert(key2, OSplinePathPortions::StoredPiece{id2, name2});
                        pieces.append({.id = id2, .copyId = copyId, .shown = showTwo || place.prolonged == 2});
                    }

                    replaced = replaced || showTwo;
                }
            }
        }
        catch (const VExceptionBadId &e)
        {
            // A piece that cannot be built leaves its copy whole: the pattern still opens, and says why in the log.
            qCWarning(vTool, "The pieces of copy %u could not be built: %s", copyId, qUtf8Printable(e.ErrorMessage()));
            continue;
        }

        result.pieces.append(pieces);
        if (replaced)
        {
            result.replacedCopies.insert(copyId);
        }

        // The ids are written straight into the item, without the undo stack: they are not a decision of the user,
        // and a parse that did not find them would allocate a new set of pieces beside the old ones on every pass.
        const QString value = OSplinePathPortions::EncodePieces(kept);
        if (value != item.attribute(AttrPieceIds))
        {
            doc->SetAttributeOrRemoveIf<QString>(item,
                                                 AttrPieceIds,
                                                 value,
                                                 [](const QString &text) noexcept -> bool { return text.isEmpty(); });
            qCDebug(vTool, "Copy %u of operation %u: pieces %s", copyId, operationId, qUtf8Printable(value));
        }

        QVector<quint32> before;
        for (const OSplinePathPortions::StoredPiece &piece : stored)
        {
            before.append(piece.id);
        }

        QVector<quint32> after;
        for (const OSplinePathPortions::StoredPiece &piece : std::as_const(kept))
        {
            after.append(piece.id);
        }

        // A piece is an object of the pattern: the graph holds it, with an edge from its copy, so the tools built on
        // it are its dependents, and deleting the copy's operation asks about them.
        OSplinePathPortions::SyncGraph(doc, copyId, before, after);
    }

    return result;
}

//---------------------------------------------------------------------------------------------------------------------
void OCopyPieces::FollowCut(VAbstractPattern *doc, VContainer *data, quint32 curveId)
{
    if (doc == nullptr || data == nullptr)
    {
        return;
    }

    QSet<quint32> operations;
    for (const OCopyLineage::Image &image : OCopyLineage::Descendants(doc, curveId))
    {
        // Only the copies already built: a copy further down the file is built after this cut, and follows it then.
        if (data->HasObject(image.copyId))
        {
            operations.insert(image.operationId);
        }
    }

    for (const quint32 operationId : std::as_const(operations))
    {
        // An operation already on the canvas builds its pieces and draws them; one that is not — a full parse has
        // not reached its item yet — has its pieces built here, and its own copy of the container given them: that
        // copy is what the pattern rebuilds the live container from.
        if (VDataTool *tool = VAbstractPattern::FindTool(operationId); tool != nullptr)
        {
            tool->RebuildDerivedObjects();
        }
        else
        {
            Build(doc, data, operationId);
            doc->UpdateToolData(operationId, data);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCopyPieces::PruneUnused(VAbstractPattern *doc, VContainer *data, quint32 curveId, const QVector<int> &cutNodes)
{
    if (doc == nullptr || data == nullptr)
    {
        return;
    }

    for (const OCopyLineage::Image &image : OCopyLineage::Descendants(doc, curveId))
    {
        if (not data->HasObject(image.copyId))
        {
            continue;
        }

        const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(data->GetGObject(image.copyId));
        if (path.isNull())
        {
            continue; // a copy that is not a path has no box to follow
        }

        QDomElement operation = doc->FindElementById(image.operationId, VAbstractPattern::TagOperation);
        const QDomElement destinationTag = operation.firstChildElement(VAbstractOperation::TagDestination);

        QDomElement item;
        for (QDomElement candidate = destinationTag.firstChildElement(VAbstractOperation::TagItem);
             not candidate.isNull();
             candidate = candidate.nextSiblingElement(VAbstractOperation::TagItem))
        {
            if (VDomDocument::GetParametrUInt(candidate, AttrIdObject, NULL_ID_STR) == image.copyId)
            {
                item = candidate;
                break;
            }
        }

        if (item.isNull())
        {
            continue;
        }

        QSet<QString> runKeys;
        if (not cutNodes.isEmpty())
        {
            for (const OSplinePathPortions::Run &run : OSplinePathPortions::Runs(cutNodes, path->CountSubSpl() + 1))
            {
                runKeys.insert(NodeKey(OSplinePathPortions::RunKey(run)));
            }
        }

        StoredPieces stored = OSplinePathPortions::DecodePieces(item.attribute(AttrPieceIds));
        bool changed = false;

        for (auto it = stored.begin(); it != stored.end();)
        {
            // The halves of a cut answer to their cut, and a run that stays a run keeps its name and its id.
            if (not it.key().startsWith(QLatin1Char('n')) || runKeys.contains(it.key()))
            {
                ++it;
                continue;
            }

            if (not OSplinePathPortions::IsInUse(doc, it.value().id))
            {
                // It leaves the graph here too: the rebuild that follows no longer knows it.
                OSplinePathPortions::SyncGraph(doc, image.copyId, {it.value().id}, {});
                it = stored.erase(it);
                changed = true;
                continue;
            }

            // Kept for its users, under the name they know it by: the rank it was named after is gone with its run.
            if (const QSharedPointer<VAbstractCurve> held = OSplinePathPortions::CurveOf(data, it.value().id);
                it.value().name.isEmpty() && not held.isNull())
            {
                it.value().name = held->HeadlessName();
                changed = true;
            }

            ++it;
        }

        if (changed)
        {
            doc->SetAttributeOrRemoveIf<QString>(item,
                                                 AttrPieceIds,
                                                 OSplinePathPortions::EncodePieces(stored),
                                                 [](const QString &text) noexcept -> bool { return text.isEmpty(); });
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyPieces::HalvesOfCut(VAbstractPattern *doc, quint32 cutId) -> QVector<quint32>
{
    QVector<quint32> halves;

    if (doc == nullptr || cutId == NULL_ID)
    {
        return halves;
    }

    const QString prefix = QStringLiteral("c%1-").arg(cutId);

    const QDomNodeList operations = doc->elementsByTagName(VAbstractPattern::TagOperation);
    for (int i = 0; i < operations.size(); ++i)
    {
        const QDomElement destinationTag =
            operations.at(i).toElement().firstChildElement(VAbstractOperation::TagDestination);

        for (QDomElement item = destinationTag.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
             item = item.nextSiblingElement(VAbstractOperation::TagItem))
        {
            const StoredPieces stored = OSplinePathPortions::DecodePieces(item.attribute(AttrPieceIds));
            for (auto it = stored.cbegin(); it != stored.cend(); ++it)
            {
                if (it.key().startsWith(prefix))
                {
                    halves.append(it.value().id);
                }
            }
        }
    }

    return halves;
}
