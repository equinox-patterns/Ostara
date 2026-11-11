/************************************************************************
 **
 **  @file   osplinepathportions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  The runs of a spline path, once the user has cut it at some of its own nodes.
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

#include "osplinepathportions.h"

#include "vabstractcubicbezierpath.h"

#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vspline.h"
#include "../vpatterndb/vcontainer.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::Runs(const QVector<int> &cutNodes, vsizetype points) -> QVector<Run>
{
    QVector<Run> runs;

    if (points < 2)
    {
        return runs; // a path of one point carries no run at all
    }

    // The frontiers: both ends, and every node the user cut on. Sorted and without duplicates, so two runs can
    // never overlap and the order of the boxes cannot change the answer.
    QVector<int> frontiers{0};

    for (const int node : cutNodes)
    {
        if (node > 0 && node < points - 1)
        {
            frontiers.append(node);
        }
    }

    frontiers.append(static_cast<int>(points) - 1);

    std::sort(frontiers.begin(), frontiers.end());
    frontiers.erase(std::unique(frontiers.begin(), frontiers.end()), frontiers.end());

    for (qsizetype index = 1; index < frontiers.size(); ++index)
    {
        runs.append(Run{frontiers.at(index - 1), frontiers.at(index)});
    }

    return runs;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::RunEndingAt(const QVector<Run> &runs, int node) -> QVector<Run>
{
    for (const Run &run : runs)
    {
        if (run.to == node)
        {
            return {run};
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::Take(const VAbstractCubicBezierPath &path, const Run &run) -> VSplinePath
{
    QVector<VSpline> parts;
    parts.reserve(run.to - run.from);

    for (int index = run.from + 1; index <= run.to; ++index)
    {
        parts.append(path.GetSpline(index));
    }

    // A path is built from two sub-splines at the least; one alone is not a path and would come out empty.
    if (parts.size() < 2)
    {
        return VSplinePath(); // a path of one sub-spline has no geometry: the caller draws nothing
    }

    VSplinePath portion(parts);
    portion.SetApproximationScale(path.GetApproximationScale());
    return portion;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::EncodeNodes(const QVector<int> &cutNodes) -> QString
{
    QStringList parts;
    parts.reserve(cutNodes.size());

    for (const int node : cutNodes)
    {
        parts.append(QString::number(node));
    }

    return parts.join(QLatin1Char(' '));
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::DecodeNodes(const QString &value) -> QVector<int>
{
    QVector<int> nodes;

    const QStringList parts = value.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    for (const QString &part : parts)
    {
        bool ok = false;
        const int node = part.toInt(&ok);

        if (ok && node > 0)
        {
            nodes.append(node);
        }
    }

    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

//---------------------------------------------------------------------------------------------------------------------
void OSplinePathPortions::SyncGraph(VAbstractPattern *doc, quint32 motherId, const QVector<quint32> &before,
                                    const QVector<quint32> &after)
{
    if (doc == nullptr)
    {
        return;
    }

    VPatternGraph *const graph = doc->PatternGraph();

    if (graph == nullptr)
    {
        return;
    }

    // A piece that is gone takes its vertex and its edge with it: a graph that kept them would hold an id no
    // object answers to, and everything that walks the graph — the dependency tree, a deletion — would read it.
    for (const quint32 id : before)
    {
        if (id == NULL_ID || after.contains(id))
        {
            continue;
        }

        graph->RemoveEdge(motherId, id);
        graph->RemoveVertex(id);
    }

    const int blockId = doc->PatternBlockMapper()->GetActiveId();

    for (const quint32 id : after)
    {
        if (id == NULL_ID)
        {
            continue;
        }

        // The vertex is added only when it is missing: a rebuild happens on every parse, and a curve read again
        // with the same piece ids would otherwise log "vertex already exists" on each one.
        if (not graph->HasVertex(id))
        {
            graph->AddVertex(id, VNodeType::OBJECT, blockId);
        }

        graph->AddEdge(motherId, id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::PortionSuffix(int index) -> QString
{
    return QStringLiteral("__p%1").arg(index);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::RunKey(const Run &run) -> QString
{
    return QStringLiteral("%1-%2").arg(run.from).arg(run.to);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::ParseRunKey(const QString &key, Run &run) -> bool
{
    const QStringList ends = key.split(QLatin1Char('-'));

    if (ends.size() != 2)
    {
        return false;
    }

    bool okFrom = false;
    bool okTo = false;
    const int from = ends.at(0).toInt(&okFrom);
    const int to = ends.at(1).toInt(&okTo);

    if (not okFrom || not okTo || from < 0 || to <= from)
    {
        return false;
    }

    run = Run{from, to};
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::LegacyRunKey(int to) -> QString
{
    return QString::number(to);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::EncodePieces(const QMap<QString, StoredPiece> &pieces) -> QString
{
    QStringList parts;
    parts.reserve(pieces.size());

    for (auto it = pieces.cbegin(); it != pieces.cend(); ++it)
    {
        if (it.value().id == NULL_ID)
        {
            continue;
        }

        parts.append(it.value().name.isEmpty()
                         ? QStringLiteral("%1=%2").arg(it.key()).arg(it.value().id)
                         : QStringLiteral("%1=%2=%3").arg(it.key(), it.value().name).arg(it.value().id));
    }

    return parts.join(QLatin1Char(';'));
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::DecodePieces(const QString &value) -> QMap<QString, StoredPiece>
{
    QMap<QString, StoredPiece> pieces;

    for (const QString &part : value.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        const QStringList fields = part.split(QLatin1Char('='));

        if (fields.size() < 2 || fields.size() > 3)
        {
            continue;
        }

        bool okId = false;
        const quint32 id = fields.constLast().trimmed().toUInt(&okId);
        const QString key = fields.constFirst().trimmed();

        if (not okId || id == NULL_ID || key.isEmpty())
        {
            continue;
        }

        pieces.insert(key, StoredPiece{id, fields.size() == 3 ? fields.at(1).trimmed() : QString()});
    }

    return pieces;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::IsInUse(const VAbstractPattern *doc, quint32 id) -> bool
{
    if (doc == nullptr || id == NULL_ID)
    {
        return false;
    }

    if (not doc->IsPatternGraphComplete())
    {
        return true; // not known yet: keep it
    }

    const VPatternGraph *const graph = doc->PatternGraph();

    if (graph == nullptr)
    {
        return true;
    }

    // Every kind of dependent counts, a piece of the pattern included: a seam line built on the piece needs it as
    // much as an operation does.
    const auto dependents = graph->TryGetDependentNodes(id, 500, [](const VNode &) -> bool { return true; });

    return not dependents.has_value() || not dependents->isEmpty();
}

namespace
{
//---------------------------------------------------------------------------------------------------------------------
template <class T> auto StoreAs(VContainer *data, T curve, quint32 id) -> quint32
{
    if (id != NULL_ID)
    {
        curve.setId(id);
        data->UpdateGObject(id, curve);
        return id;
    }

    return data->AddGObject(new T(curve));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::RunCurve(const VAbstractCubicBezierPath &path, const Run &run) -> QSharedPointer<VAbstractCurve>
{
    if (run.from < 0 || run.to <= run.from || run.to > path.CountSubSpl())
    {
        return {};
    }

    if (run.to - run.from == 1)
    {
        return QSharedPointer<VSpline>::create(path.GetSpline(run.from + 1));
    }

    VSplinePath portion = Take(path, run);
    if (portion.CountPoints() < 2)
    {
        return {};
    }

    return QSharedPointer<VSplinePath>::create(portion);
}

//---------------------------------------------------------------------------------------------------------------------
void OSplinePathPortions::MarkPiece(VAbstractCurve &piece, const VAbstractCurve &from, quint32 ownerId,
                                    const QString &suffix)
{
    piece.SetColor(from.GetColor());
    piece.SetPenStyle(from.GetPenStyle());
    piece.SetApproximationScale(from.GetApproximationScale());
    piece.SetDerivative(true);
    piece.SetNameSuffix(suffix);
    piece.setIdObject(ownerId);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::StorePiece(VContainer *data, const QSharedPointer<VAbstractCurve> &piece, quint32 id)
    -> quint32
{
    if (data == nullptr || piece.isNull())
    {
        return NULL_ID;
    }

    if (id != NULL_ID && data->HasObject(id))
    {
        const QSharedPointer<VGObject> held = data->GetGObject(id);
        if (held.isNull() || held->getType() != piece->getType() || held->getIdObject() != piece->getIdObject())
        {
            qWarning("Piece id %u names another object of the pattern: the piece takes a new id.", id);
            id = NULL_ID;
        }
    }

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (piece->getType())
    {
        case GOType::Spline:
            id = StoreAs(data, *piece.staticCast<VSpline>(), id);
            data->AddSpline(data->GeometricObject<VSpline>(id), id);
            return id;
        case GOType::CubicBezier:
            id = StoreAs(data, *piece.staticCast<VCubicBezier>(), id);
            data->AddSpline(data->GeometricObject<VCubicBezier>(id), id);
            return id;
        case GOType::SplinePath:
            id = StoreAs(data, *piece.staticCast<VSplinePath>(), id);
            data->AddCurveWithSegments(data->GeometricObject<VSplinePath>(id), id);
            return id;
        case GOType::CubicBezierPath:
            id = StoreAs(data, *piece.staticCast<VCubicBezierPath>(), id);
            data->AddCurveWithSegments(data->GeometricObject<VCubicBezierPath>(id), id);
            return id;
        case GOType::Arc:
            id = StoreAs(data, *piece.staticCast<VArc>(), id);
            data->AddArc(data->GeometricObject<VArc>(id), id);
            return id;
        case GOType::EllipticalArc:
            id = StoreAs(data, *piece.staticCast<VEllipticalArc>(), id);
            data->AddArc(data->GeometricObject<VEllipticalArc>(id), id);
            return id;
        case GOType::Point:
        case GOType::PlaceLabel:
        case GOType::Unknown:
            break;
    }

    QT_WARNING_POP

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::KeptSuffix(const QString &curveName, const Run &run) -> QString
{
    return curveName + QStringLiteral("__p%1_%2").arg(run.from).arg(run.to);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::CurveOf(const VContainer *data, quint32 id) -> QSharedPointer<VAbstractCurve>
{
    if (data == nullptr || id == NULL_ID || not data->HasObject(id))
    {
        return {};
    }

    return qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(id));
}
