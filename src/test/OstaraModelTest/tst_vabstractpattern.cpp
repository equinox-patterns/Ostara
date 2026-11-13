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
#include "../ifc/exception/vexceptionwrongid.h"
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
// The versions of the pattern, as the document holds them. Three invariants: the "all versions" entry is where
// every piece that belongs to none lives and can never be deleted, ids are never reused (a piece that comes back
// after an undo finds the same version), and turning the mode off keeps the list — the switch decides about the
// pieces, not about the names the reader typed.
void TST_VAbstractPattern::PatternVersions()
{
    TestDoc doc;

    const QDomDocument::ParseResult parsed =
        doc.setContent(QByteArrayLiteral("<pattern><version>1.2.0</version><unit>cm</unit></pattern>"));
    QVERIFY2(parsed, qUtf8Printable(parsed.errorMessage));

    // A pattern that never declared versions carries nothing — not an empty list.
    QVERIFY(!doc.IsMultiVersion());
    QCOMPARE(doc.GetPatternVersions().size(), 0);
    QCOMPARE(doc.AllVersionsId(), 1u);

    doc.SetMultiVersion(true);
    QVERIFY(doc.IsMultiVersion());
    QCOMPARE(doc.GetPatternVersions().size(), 1);

    // **A version's key is not a pattern object id.** The document refuses to save when two objects share an
    // id, and the versions used to carry `id="1"` — which made every save fail as soon as a version existed
    // (a point with id 1 is in every pattern). The list must stay out of that namespace.
    {
        QDomElement draw = doc.createElement(u"draw"_s);
        draw.setAttribute(AttrName, u"B"_s);
        doc.documentElement().appendChild(draw);
        QDomElement calculation = doc.createElement(u"calculation"_s);
        draw.appendChild(calculation);
        QDomElement point = doc.createElement(u"point"_s);
        point.setAttribute(VDomDocument::AttrId, u"1"_s);
        calculation.appendChild(point);
    }
    bool idsAreUnique = true;
    try
    {
        doc.TestUniqueId();
    }
    catch (const VExceptionWrongId &)
    {
        idsAreUnique = false;
    }
    QVERIFY2(idsAreUnique, "A version's key collided with a pattern object id: the file could not be saved.");
    const quint32 allVersions = doc.AllVersionsId();
    QCOMPARE(allVersions, 1u);
    QCOMPARE(doc.GetPatternVersion(allVersions).name, VAbstractPattern::AllVersionsName());

    // The schema's order, or the file cannot be saved at all: the element goes after <unit>, its predecessor in
    // <xs:sequence> among the elements this document holds.
    const QDomElement container = doc.documentElement().firstChildElement(VAbstractPattern::TagPatternVersions);
    QVERIFY(!container.isNull());
    QCOMPARE(container.previousSiblingElement().tagName(), VDomDocument::TagUnit);

    const quint32 versionA = doc.AddPatternVersion(u"Version A"_s);
    QCOMPARE(versionA, 2u);
    QCOMPARE(doc.GetPatternVersions().size(), 2);
    QCOMPARE(doc.GetPatternVersion(versionA).name, u"Version A"_s);

    // **Round trip.** What the document writes is what it reads back — and the list carries keys, never ids,
    // which is the whole reason a pattern with versions can be saved at all (a version is not an object of
    // the pattern, and the document refuses two objects sharing an id).
    const QString saved = doc.toString();
    QVERIFY2(saved.contains(u"<patternVersion key=\"1\""_s), qUtf8Printable(saved));
    QVERIFY2(not saved.contains(u"<patternVersion id="_s), qUtf8Printable(saved));

    TestDoc reloaded;
    const QDomDocument::ParseResult parsedAgain = reloaded.setContent(saved.toUtf8());
    QVERIFY2(parsedAgain, qUtf8Printable(parsedAgain.errorMessage));
    QCOMPARE(reloaded.GetPatternVersions().size(), 2);
    QCOMPARE(reloaded.GetPatternVersion(allVersions).name, VAbstractPattern::AllVersionsName());
    QCOMPARE(reloaded.GetPatternVersion(versionA).name, u"Version A"_s);
    QVERIFY(reloaded.IsMultiVersion());

    // An id the document does not know reads as the entry every piece belongs to — never as nothing.
    QCOMPARE(doc.GetPatternVersion(99).id, allVersions);

    // "All versions" is not one version among others: it is where the pieces that belong to none live.
    QVERIFY(!doc.RemovePatternVersion(allVersions));
    QVERIFY(doc.RemovePatternVersion(versionA));
    QCOMPARE(doc.GetPatternVersions().size(), 1);

    // Switching off keeps the list: switching back on must find the names where they were.
    doc.SetMultiVersion(false);
    QVERIFY(!doc.IsMultiVersion());
    QCOMPARE(doc.GetPatternVersions().size(), 1);
    QCOMPARE(container.attribute(VAbstractPattern::AttrActive), u"false"_s);

    doc.SetMultiVersion(true);
    QVERIFY(doc.IsMultiVersion());
    QCOMPARE(doc.GetPatternVersions().size(), 1);
    QVERIFY(container.attribute(VAbstractPattern::AttrActive).isEmpty());
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

//---------------------------------------------------------------------------------------------------------------------
namespace
{
// Every object id 0..maxId answered with and without the index; the first difference is reported.
auto CompareVisibility(const TestDoc &doc, quint32 maxId) -> QString
{
    QVector<bool> walked;
    walked.reserve(static_cast<qsizetype>(maxId) + 1);
    for (quint32 id = 0; id <= maxId; ++id)
    {
        walked.append(doc.IsObjectVisible(id));
    }

    const VAbstractPattern::VisibilityIndexScope scope(&doc);
    for (quint32 id = 0; id <= maxId; ++id)
    {
        if (doc.IsObjectVisible(id) != walked.at(static_cast<qsizetype>(id)))
        {
            return u"object %1: walk %2, index %3"_s.arg(id).arg(walked.at(static_cast<qsizetype>(id)))
                .arg(doc.IsObjectVisible(id));
        }
    }
    return {};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void TST_VAbstractPattern::VisibilityIndexAnswersLikeTheWalk()
{
    // Every rule of the walk: shown as soon as one holding layer shows it (3, 6), hidden when every holding layer
    // hides it (4, 8), a layer without `visible` shows (7), an object no layer holds is shown (2, 9), and an item
    // without `object` names the tool's own object (1, 4, 7, 8). Two blocks, so the answer is the document's.
    TestDoc doc;
    QVERIFY(doc.setContent(QByteArrayLiteral(
        "<pattern><version>1.2.0</version><unit>cm</unit>"
        "<draw name=\"A\"><groups>"
        "<group id=\"100\" name=\"shown\" visible=\"true\"><item tool=\"1\"/><item tool=\"2\" object=\"3\"/>"
        "<item tool=\"5\" object=\"6\"/></group>"
        "<group id=\"101\" name=\"hidden\" visible=\"false\"><item tool=\"2\" object=\"3\"/><item tool=\"4\"/>"
        "<item tool=\"5\" object=\"6\"/></group>"
        "</groups></draw>"
        "<draw name=\"B\"><groups>"
        "<group id=\"102\" name=\"no attribute\"><item tool=\"7\"/></group>"
        "<group id=\"103\" name=\"hidden too\" visible=\"false\"><item tool=\"8\"/></group>"
        "</groups></draw></pattern>")));

    QCOMPARE(CompareVisibility(doc, 12), QString());
    {
        const VAbstractPattern::VisibilityIndexScope scope(&doc);
        QVERIFY(not doc.IsObjectVisible(4));
        QVERIFY(not doc.IsObjectVisible(8));
        QVERIFY(doc.IsObjectVisible(6));
        QVERIFY(doc.IsObjectVisible(9));

        // A layer written during the scope is seen at once: the index is not the answer of a moment ago.
        doc.SetGroupVisibility(101, true);
        QVERIFY(doc.IsObjectVisible(4));
        doc.SetGroupVisibility(101, false);
        QVERIFY(not doc.IsObjectVisible(4));
    }

    // An item that cannot be read: the walk answers "shown" for everything, and so does the index.
    TestDoc unreadable;
    QVERIFY(unreadable.setContent(QByteArrayLiteral(
        "<pattern><version>1.2.0</version><unit>cm</unit><draw name=\"A\"><groups>"
        "<group id=\"100\" name=\"hidden\" visible=\"false\"><item tool=\"4\"/><item tool=\"x\"/></group>"
        "</groups></draw></pattern>")));
    QCOMPARE(CompareVisibility(unreadable, 6), QString());

    // A real pattern, when one is given: every id it can hold.
    if (const QString file = qEnvironmentVariable("OSTARA_VISIBILITY_FILE"); not file.isEmpty())
    {
        QFile source(file);
        QVERIFY(source.open(QIODevice::ReadOnly));
        TestDoc pattern;
        QVERIFY(pattern.setContent(source.readAll()));
        QCOMPARE(CompareVisibility(pattern, 5000), QString());
    }
}
