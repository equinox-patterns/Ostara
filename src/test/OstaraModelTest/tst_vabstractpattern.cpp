/************************************************************************
 **
 **  @file   tst_vabstractpattern.cpp
 **  @author Paco Arjonilla <pacoarjonilla(at)yahoo.es>
 **  @date   15 7, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "tst_vabstractpattern.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatterngraph.h"

#include <QElapsedTimer>
#include <QThreadPool>
#include <QtTest>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
constexpr int tokenCount = 20000;
constexpr int referencesPerToken = 20;
constexpr quint32 formulaOwnerId = 1;
constexpr quint32 firstReferenceId = 1000;

//---------------------------------------------------------------------------------------------------------------------
// Minimal concrete document. The dependency-check machinery lives in VAbstractPattern itself.
class TestDoc : public VAbstractPattern
{
public:
    explicit TestDoc(QObject *parent = nullptr)
      : VAbstractPattern(parent)
    {
    }

    void CreateEmptyFile() override {}
    auto GenerateLabel(const LabelType &type, const QString &reservedName = QString()) const -> QString override
    {
        Q_UNUSED(type)
        Q_UNUSED(reservedName)
        return {};
    }
    void UpdateToolData(const quint32 &id, VContainer *data) override
    {
        Q_UNUSED(id)
        Q_UNUSED(data)
    }
    void LiteParseTree(const Document &parse) override { Q_UNUSED(parse) }

    using VAbstractPattern::CancelFormulaDependencyChecks;
};

//---------------------------------------------------------------------------------------------------------------------
auto BuildVariables() -> QHash<QString, QList<quint32>>
{
    QHash<QString, QList<quint32>> variables;
    variables.reserve(tokenCount);
    quint32 referenceId = firstReferenceId;
    for (int i = 0; i < tokenCount; ++i)
    {
        QList<quint32> references;
        references.reserve(referencesPerToken);
        for (int j = 0; j < referencesPerToken; ++j)
        {
            references.append(referenceId++);
        }
        variables.insert(u"v%1"_s.arg(i), references);
    }
    return variables;
}

//---------------------------------------------------------------------------------------------------------------------
auto BuildFormula() -> QString
{
    QStringList terms;
    terms.reserve(tokenCount);
    for (int i = 0; i < tokenCount; ++i)
    {
        terms.append(u"v%1"_s.arg(i));
    }
    return terms.join('+'_L1);
}

//---------------------------------------------------------------------------------------------------------------------
void AddVertices(VPatternGraph *graph)
{
    graph->AddVertex(formulaOwnerId, VNodeType::MODELING_OBJECT, 0);
    const auto lastReferenceId =
        firstReferenceId + static_cast<quint32>(tokenCount) * static_cast<quint32>(referencesPerToken);
    for (quint32 id = firstReferenceId; id < lastReferenceId; ++id)
    {
        graph->AddVertex(id, VNodeType::MODELING_OBJECT, 0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Busy-wait until the worker starts adding edges so cancellation happens mid-run.
auto WaitForFirstEdge(const VPatternGraph *graph) -> bool
{
    QElapsedTimer timer;
    timer.start();
    while (graph->EdgeCount() == 0 && timer.elapsed() < 10000)
    {
    }
    return graph->EdgeCount() > 0;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_VAbstractPattern::TST_VAbstractPattern(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
// A worker in the middle of a large dependency check must stop early on cancellation instead of
// finishing its full workload.
void TST_VAbstractPattern::CancelStopsRunningWorkers()
{
    TestDoc doc;
    VPatternGraph *graph = doc.PatternGraph();
    QVERIFY(graph != nullptr);

    AddVertices(graph);
    doc.FindFormulaDependencies(BuildFormula(), formulaOwnerId, BuildVariables());

    QVERIFY(WaitForFirstEdge(graph));
    doc.CancelFormulaDependencyChecks();
    QThreadPool::globalInstance()->waitForDone();

    const auto totalEdges = static_cast<std::size_t>(tokenCount) * static_cast<std::size_t>(referencesPerToken);
    QVERIFY2(graph->EdgeCount() < totalEdges / 2,
             qUtf8Printable(u"Worker was not cancelled: %1 of %2 edges were added"_s.arg(graph->EdgeCount())
                                .arg(totalEdges)));
}

//---------------------------------------------------------------------------------------------------------------------
// Clearing a document while a dependency check is running must not leak edges into the graph once the
// next document re-adds vertices with the same ids.
void TST_VAbstractPattern::ClearCancelsPendingWorkers()
{
    TestDoc doc;
    VPatternGraph *graph = doc.PatternGraph();
    QVERIFY(graph != nullptr);

    AddVertices(graph);
    doc.FindFormulaDependencies(BuildFormula(), formulaOwnerId, BuildVariables());

    QVERIFY(WaitForFirstEdge(graph));
    doc.Clear();
    AddVertices(graph);
    QThreadPool::globalInstance()->waitForDone();

    QCOMPARE(graph->EdgeCount(), static_cast<std::size_t>(0));
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------------------------------------------------
// An operation copy of a cut curve, inside a document that holds the cut it was split at. `showTwoSegments` is
// the whole question: with it on, the copy gives way to two drawn halves; with it off, nothing replaces the copy.
auto BuildCopyDocument(TestDoc &doc, bool showTwoSegments) -> QDomElement
{
    QDomElement root = doc.createElement(u"pattern"_s);
    doc.appendChild(root);

    QDomElement draw = doc.createElement(u"draw"_s);
    draw.setAttribute(AttrName, u"Buste"_s);
    root.appendChild(draw);

    QDomElement calculation = doc.createElement(u"calculation"_s);
    draw.appendChild(calculation);

    QDomElement cut = doc.createElement(u"point"_s);
    cut.setAttribute(VDomDocument::AttrId, u"173"_s);
    cut.setAttribute(AttrType, u"cutSplinePath"_s);
    cut.setAttribute(AttrSegment1Id, u"2363"_s);
    cut.setAttribute(AttrSegment2Id, u"2364"_s);
    if (showTwoSegments)
    {
        cut.setAttribute(AttrShowTwoSegments, u"true"_s);
    }
    calculation.appendChild(cut);

    QDomElement operation = doc.createElement(u"operation"_s);
    operation.setAttribute(VDomDocument::AttrId, u"1027"_s);
    operation.setAttribute(AttrType, u"rotation"_s);
    calculation.appendChild(operation);

    QDomElement destination = doc.createElement(u"destination"_s);
    operation.appendChild(destination);

    QDomElement item = doc.createElement(u"item"_s);
    item.setAttribute(AttrIdObject, u"1035"_s);
    item.setAttribute(u"pieceIds"_s, u"c173-1=2511;c173-2=2512"_s);
    destination.appendChild(item);

    return operation;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
// The copy is what the canvas draws when no cut box is ticked, so a layer must be able to name it. Reading
// `pieceIds` alone and stopping there took it out of every layer at load time — and a curve no layer names is
// drawn whatever the layers say, which is what put two rotations back on screen outside their hidden layers.
void TST_VAbstractPattern::CopyStaysLayerableWhileNoPieceIsDrawn()
{
    TestDoc doc;
    const QDomElement operation = BuildCopyDocument(doc, false);

    const QVector<QPair<quint32, quint32>> pairs
        = VAbstractPattern::LayerableObjectsOfTool(operation, 1027, &doc);

    QCOMPARE(pairs, (QVector<QPair<quint32, quint32>>{{1035, 1027}}));
}

//---------------------------------------------------------------------------------------------------------------------
// The loading pass asks the OTHER question: what the element declares, drawn or not. A layer written before the
// pieces existed names the copy, and that pair designates something real — deleting it is what left curves
// belonging to no layer, and a curve no layer names is drawn whatever the layers say.
void TST_VAbstractPattern::DeclaredPairsSurviveWhileNotDrawn()
{
    TestDoc doc;
    const QDomElement operation = BuildCopyDocument(doc, false);

    const QVector<QPair<quint32, quint32>> declared
        = VAbstractPattern::LayerableObjectsOfTool(operation, 1027, &doc, false);

    QCOMPARE(declared, (QVector<QPair<quint32, quint32>>{{2511, 1027}, {2512, 1027}, {1035, 1027}}));
}

//---------------------------------------------------------------------------------------------------------------------
// With a cut showing its two segments the copy is replaced on the canvas: the layer must name the halves instead,
// and only those — naming the copy as well would hide an object nobody draws.
void TST_VAbstractPattern::CopyGivesWayToThePiecesItDraws()
{
    TestDoc doc;
    const QDomElement operation = BuildCopyDocument(doc, true);

    const QVector<QPair<quint32, quint32>> pairs
        = VAbstractPattern::LayerableObjectsOfTool(operation, 1027, &doc);

    QCOMPARE(pairs, (QVector<QPair<quint32, quint32>>{{2511, 1027}, {2512, 1027}}));
}
