/************************************************************************
 **
 **  @file   otoolflouncespiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The flounce spiral tool.
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

#include "otoolflouncespiral.h"

#include <QDomElement>
#include <QGraphicsSceneContextMenuEvent>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <utility>
#include <QUndoStack>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogflouncespiral.h"
#include "../../../undocommands/savetooloptions.h"
#include "../../vabstracttool.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/oflouncespiral.h"
#include "../vgeometry/vabstractbezier.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "ooperationlines.h"

const QString OToolFlounceSpiral::ToolType = QStringLiteral("flounceSpiral");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// The name a point of the figure is given when the user did not choose one.
auto DefaultPointName(const QSharedPointer<VPointF> &start, OFlounceSpiralPoint::Role role) -> QString
{
    const QString base = start.isNull() ? QStringLiteral("A") : start->name();
    switch (role)
    {
        case OFlounceSpiralPoint::Role::OuterStart:
            return base + QStringLiteral("_2");
        case OFlounceSpiralPoint::Role::InnerEnd:
            return base + QStringLiteral("_1");
        case OFlounceSpiralPoint::Role::OuterEnd:
            return base + QStringLiteral("_3");
    }
    return base;
}

//---------------------------------------------------------------------------------------------------------------------
/// The tag names of the children of @a element, in the order they are written, as one string. Used by the traces
/// that check the order the schema imposes — a whole save fails when it is wrong, and the failure names the model
/// rather than the child that broke it.
auto ChildrenOf(const QDomElement &element) -> QString
{
    QStringList names;
    const QDomNodeList nodes = element.childNodes();
    for (int i = 0; i < nodes.size(); ++i)
    {
        const QDomElement child = nodes.at(i).toElement();
        if (not child.isNull())
        {
            names.append(child.tagName());
        }
    }
    return names.join(QStringLiteral(", "));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OToolFlounceSpiral::OToolFlounceSpiral(const OToolFlounceSpiralInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_startPointId(initData.startPointId),
    m_display(initData.display)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                                VContainer *data) -> OToolFlounceSpiral *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogFlounceSpiral> dialogTool = qobject_cast<ODialogFlounceSpiral *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolFlounceSpiralInitData initData;
    initData.startPointId = dialogTool->GetStartPointId();
    initData.display = dialogTool->GetDisplay();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    OToolFlounceSpiral *tool = Create(initData);
    if (tool != nullptr)
    {
        tool->m_dialog = dialog;
    }
    return tool;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::Create(OToolFlounceSpiralInitData &initData) -> OToolFlounceSpiral *
{
    VAbstractValApplication *app = VAbstractValApplication::VApp();

    // The operation is a tool of its own and needs its id BEFORE anything is written, exactly like the pleat:
    // the objects it carries are named by ids handed out here, and the element that carries them is written
    // under the tool's own id. Without this line the tool is built with id 0, and writing it aborts on
    // "This id (0) is not unique" — an id already carried by every element the file has.
    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->getNextId();
    }

    const QSharedPointer<VPointF> start = initData.data->GeometricObject<VPointF>(initData.startPointId);
    qDebug() << "FLOUNCE-CREATE: point trouve =" << start->name();

    OFlounceSpiralDisplay &display = initData.display;

    // The ids and the names of the figure come FIRST, before the figure is built: the two curves are named after
    // the two points each of them runs between, so those names have to be known before the geometry is asked for
    // them. Building first and naming afterwards left both curves called after the start point alone.
    if (initData.typeCreation == Source::FromGui)
    {
        display.inner = initData.data->getNextId();
        display.outer = initData.data->getNextId();
        // The two segments are the only objects of the figure the container does not hold, but they still need an
        // id each, and that id has to be handed out ONCE — see the else branch.
        display.line1 = initData.data->getNextId();
        display.line2 = initData.data->getNextId();

        // The names the user typed in the dialog are KEPT: the Points tab exists for them, and clearing the list
        // threw them away, so a figure created with its points already named came out with the default names.
        if (display.points.size() != 3)
        {
            display.points.clear();
            for (const OFlounceSpiralPoint::Role role :
                 {OFlounceSpiralPoint::Role::OuterStart, OFlounceSpiralPoint::Role::InnerEnd,
                  OFlounceSpiralPoint::Role::OuterEnd})
            {
                OFlounceSpiralPoint point;
                point.role = role;
                display.points.append(point);
            }
        }
        for (OFlounceSpiralPoint &point : display.points)
        {
            point.id = initData.data->getNextId();
            if (point.name.isEmpty())
            {
                point.name = DefaultPointName(start, point.role);
            }
        }
    }
    else
    {
        // Every id of the figure is one the parser has not seen yet: it is told about them so that a name is
        // never handed out twice.
        OFlounceSpiralDisplay::EnsureIds(display, initData.data);

        // The ids of the two segments come from the file, through the `line` attribute of the figure's items —
        // and they are kept. Handing out new ones at every reparse left the segments of the parse before behind,
        // alive in the scene and still registered, drawn at the positions they no longer had: every option change
        // added two more. A file that carries none (an older one) still gets a pair.
        if (display.line1 == NULL_ID)
        {
            display.line1 = initData.data->getNextId();
        }
        if (display.line2 == NULL_ID)
        {
            display.line2 = initData.data->getNextId();
        }
    }

    // The name a role carries in the figure, as the display knows it.
    auto NameOf = [&display](OFlounceSpiralPoint::Role role) -> QString
    {
        for (const OFlounceSpiralPoint &point : display.points)
        {
            if (point.role == role)
            {
                return point.name;
            }
        }
        return {};
    };

    // The dialog works in pattern units, the geometry in pixels.
    const qreal width = app->toPixel(CheckFormula(initData.id, display.width, initData.data));
    const qreal length = app->toPixel(CheckFormula(initData.id, display.length, initData.data));
    const qreal angle = CheckFormula(initData.id, display.angle, initData.data);
    qDebug() << "FLOUNCE-CREATE: formules width =" << width << " length =" << length << " angle =" << angle;

    // Each edge is named after the two points it runs between — the inner edge from the start point to the inner
    // end, the outer edge from the outer start to the outer end. That is what its tooltip, its history entry and
    // its aliases read.
    const OFlounceSpiral::Figure figure =
        OFlounceSpiral::Build(start->toQPointF(), width, length, angle,
                              {start->name(), NameOf(OFlounceSpiralPoint::Role::OuterEnd)},
                              {NameOf(OFlounceSpiralPoint::Role::OuterStart), NameOf(OFlounceSpiralPoint::Role::OuterEnd)});

    // What the user typed, said the way the tool says it: a zero length or width is raised to the smallest value
    // the figure can hold rather than refused, so that a half-written pattern still opens.
    if (display.width.isEmpty())
    {
        display.width = QStringLiteral("0");
    }
    if (display.length.isEmpty())
    {
        display.length = QStringLiteral("0");
    }

    qDebug() << "FLOUNCE-CREATE: figure batie, innerEnd ="
             << figure.innerEnd << " outerStart =" << figure.outerStart << " outerEnd =" << figure.outerEnd;

    // The points come first: the two segments and the two curves lean on them, and the figure is only whole once
    // they stand where the geometry put them.
    //
    // Whether the name of a point is shown is not written in the figure's own element — the schema has no
    // attribute for it — but in the <destination> item of that point, the one place an operation says showLabel.
    // A figure built from the dialog carries the switch in its display (the Points tab), a figure read from a file
    // in those items, and both are read here, before the three points are placed: a point is placed once.
    auto WantsLabel = [&display, &initData](OFlounceSpiralPoint::Role role) -> bool
    {
        const quint32 id = OFlounceSpiralDisplay::PointId(display, role);
        if (id != NULL_ID)
        {
            for (const DestinationItem &item : std::as_const(initData.destination))
            {
                if (item.id == id)
                {
                    return item.showLabel;
                }
            }
        }
        for (const OFlounceSpiralPoint &point : std::as_const(display.points))
        {
            if (point.role == role)
            {
                return point.labelVisible;
            }
        }
        return false;
    };

    auto PlacePoint = [&initData, &display, &NameOf, &WantsLabel](OFlounceSpiralPoint::Role role,
                                                                 const QPointF &position)
    {
        const quint32 id = OFlounceSpiralDisplay::PointId(display, role);
        if (id == NULL_ID)
        {
            return;
        }

        auto point = QSharedPointer<VPointF>::create(position, NameOf(role));
        point->setIdObject(initData.id);
        point->SetShowLabel(WantsLabel(role));
        try
        {
            initData.data->UpdateGObject(id, point);
        }
        catch (const VExceptionBadId &)
        {
            // The id already carries something else: the object is left alone.
        }
    };

    PlacePoint(OFlounceSpiralPoint::Role::OuterStart, figure.outerStart);
    PlacePoint(OFlounceSpiralPoint::Role::InnerEnd, figure.innerEnd);
    PlacePoint(OFlounceSpiralPoint::Role::OuterEnd, figure.outerEnd);
    qDebug() << "FLOUNCE-CREATE: les trois points sont places";

    auto inner = QSharedPointer<VSplinePath>::create(figure.begin);
    auto outer = QSharedPointer<VSplinePath>::create(figure.outer);

    inner->setIdObject(initData.id);
    outer->setIdObject(initData.id);
    // ONE pen and ONE colour for the whole figure: the two edges and the two joining segments are the parts of a
    // single object, and nothing on the canvas tells them apart.
    inner->SetPenStyle(display.typeLine);
    outer->SetPenStyle(display.typeLine);
    inner->SetColor(display.lineColor);
    outer->SetColor(display.lineColor);
    // The inner edge keeps the name an older pattern gave it: the figure offers one name now, but taking that
    // variable away would break the formulas of the pattern that carries it.
    inner->SetAliasSuffix(display.innerAlias);
    outer->SetAliasSuffix(display.outerAlias);

    qDebug() << "FLOUNCE-CREATE: inner id =" << display.inner << " outer id =" << display.outer;

    // Two calls, not one: AddSpline registers the VARIABLES of the curve — its length, its angles, its handles —
    // and puts the object in the container through nothing at all. The object itself goes in by UpdateGObject.
    // Registering only the variables leaves a curve the container does not hold, and the scene item cannot be
    // built: the operation dies on VExceptionBadId. The pleat does both calls, in this order, for every curve.
    initData.data->UpdateGObject(display.inner, inner);
    initData.data->UpdateGObject(display.outer, outer);
    initData.data->AddSpline(qSharedPointerDynamicCast<VAbstractBezier>(inner), display.inner);
    initData.data->AddSpline(qSharedPointerDynamicCast<VAbstractBezier>(outer), display.outer);

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    // Added on EVERY parse, not only when the tool is created from the interface. The graph is rebuilt from
    // nothing at each parse, and every edge below is recorded against this vertex: skipping it on the parses
    // that read the file left the whole figure — the tool, its two edges, its three points and its two
    // segments — as edges waiting for a node that never came, which the next rebuild reported six times over
    // as "an object depends on one the pattern does not define". Every other operation adds it unconditionally.
    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    initData.doc->FindFormulaDependencies(display.width, initData.id, varData);
    initData.doc->FindFormulaDependencies(display.length, initData.id, varData);
    initData.doc->FindFormulaDependencies(display.angle, initData.id, varData);

    // The ids of the two segments were settled above — handed out once on creation, read from the file on every
    // reparse — and this is where the segments the parse will build are described. The list is rebuilt whole so
    // that a reparse never doubles it.
    initData.lines.clear();

    auto AppendLine = [&initData, &display](quint32 result, quint32 first, quint32 second)
    {
        OOperationLine line;
        line.source = NULL_ID;
        line.point1 = first;
        line.point2 = second;
        line.result = result;
        line.typeLine = display.typeLine;
        line.lineColor = display.lineColor;
        initData.lines.append(line);
    };

    // Both segments join the two edges: the band is closed at each end by a straight piece of one band width.
    //
    // Which points pair up is decided by the geometry, not by the roles' names. The outer edge is built by
    // walking the inner one and stepping one band width out along the normal, and it is then emitted the
    // other way round: its FIRST point is the outer image of the inner edge's LAST point, and its LAST point
    // is the outer image of the inner edge's FIRST point — the start itself. So the two ends of the band are
    // (start, outerEnd) and (innerEnd, outerStart), and each pair is exactly one band width apart. Pairing
    // start with outerStart instead would draw a segment right across the figure.
    AppendLine(display.line1, initData.startPointId,
               OFlounceSpiralDisplay::PointId(display, OFlounceSpiralPoint::Role::OuterEnd));
    AppendLine(display.line2, OFlounceSpiralDisplay::PointId(display, OFlounceSpiralPoint::Role::InnerEnd),
               OFlounceSpiralDisplay::PointId(display, OFlounceSpiralPoint::Role::OuterStart));

    // The objects the operation carries, as children of the tool: deleting the figure must never be blocked by
    // its own parts, and a part must never be offered for deletion on its own.
    for (const OFlounceSpiralPoint::Role role :
         {OFlounceSpiralPoint::Role::OuterStart, OFlounceSpiralPoint::Role::InnerEnd,
          OFlounceSpiralPoint::Role::OuterEnd})
    {
        const quint32 id = OFlounceSpiralDisplay::PointId(display, role);
        if (id == NULL_ID)
        {
            continue;
        }

        patternGraph->AddVertex(id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, id);
    }

    // The two edges hang under the tool and go with it. The two segments are NOT listed here: a line is an
    // object of its own, and VToolLine::Create registers its vertex. Adding one here as well is what made the
    // pattern graph answer "vertex already exists" while the figure was being built.
    for (const quint32 id : {display.inner, display.outer})
    {
        if (id == NULL_ID)
        {
            continue;
        }
        patternGraph->AddVertex(id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, id);
    }

    patternGraph->AddEdge(initData.startPointId, initData.id);

    // The two segments are given their endpoints here, and NOT through CreateDestinationLines.
    //
    // That helper resolves an endpoint through the source/destination pairing of a transformation: it searches
    // the sources for the point and answers the destination at the SAME INDEX — the copy the operation made of
    // it. The start point of this figure IS a source of the operation (it is what the flounce is placed on), so
    // it was answered with the first object of the destination, the inner curve. The segment leaving it then
    // named a curve where a point is expected, could not be built, and was dropped without a word: it was drawn
    // while the figure was created and disappeared as soon as the pattern was reopened. The other segment
    // survived because neither of its endpoints is a source.
    //
    // Nothing is transformed here, so there is nothing to resolve: the endpoints are known, they are the point
    // ids the figure carries. OOperationLines::CreateLine exists for exactly this caller — "such a caller
    // resolves its own endpoints and comes here".
    //
    // Called on EVERY parse, out of the full-parse branch, as the pleat and the close-dart do: a transformed line
    // has no element of its own, so no parse ever re-reads it, and this call is the only thing that gives it its
    // new pen and its new endpoints after an option change.
    for (const OOperationLine &line : std::as_const(initData.lines))
    {
        qDebug() << "FLOUNCE-CREATE: ligne id =" << line.result << " de" << line.point1 << "a" << line.point2;
        OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, line.point1, line.point2,
                                    initData.parse);
    }

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    qDebug() << "FLOUNCE-CREATE: graphe et lignes faits, parse =" << static_cast<int>(initData.parse);

    // Ce que le conteneur detient vraiment, juste avant que l'item de scene ne soit bati.
    auto Existe = [&initData](quint32 id, const char *quoi)
    {
        qDebug() << "FLOUNCE-CREATE:   " << quoi << "id =" << id
                 << " present =" << initData.data->HasObject(id);
    };
    Existe(display.inner, "inner");
    Existe(display.outer, "outer");
    Existe(display.line1, "line1");
    Existe(display.line2, "line2");
    for (const OFlounceSpiralPoint &pt : display.points)
    {
        Existe(pt.id, "point");
    }

    if (initData.parse == Document::FullParse)
    {
        // The proof that the two segments really became objects: a line the resolver could not resolve leaves
        // nothing behind, and says so only through a category log that is off by default. Read it here instead.
        for (const quint32 id : {display.line1, display.line2})
        {
            qDebug() << "FLOUNCE-CREATE: segment id =" << id
                     << " construit =" << (VAbstractPattern::FindTool(id) != nullptr);
        }

        qDebug() << "FLOUNCE-CREATE: lignes creees, on ajoute le tool a la scene";
        return AddToolToScene<OToolFlounceSpiral>(initData, Tool::FlounceSpiral, tr("flounce spiral"),
                                                  [&initData]() { return new OToolFlounceSpiral(initData); });
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::CreateFigure(const OToolFlounceSpiralInitData &initData)
{
    Q_UNUSED(initData)
    RefreshFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::RefreshFigure()
{
    const QSharedPointer<VPointF> start = VAbstractTool::data.GeometricObject<VPointF>(m_startPointId);
    if (start.isNull())
    {
        return;
    }

    VAbstractValApplication *app = VAbstractValApplication::VApp();
    const qreal width = app->toPixel(CheckFormula(m_id, m_display.width, getData()));
    const qreal length = app->toPixel(CheckFormula(m_id, m_display.length, getData()));
    const qreal angle = CheckFormula(m_id, m_display.angle, getData());

    // The very same names as at creation: an edit has to leave the two curves named as they were, or the
    // tooltips would change under the user as soon as a value is touched.
    const OFlounceSpiral::Figure figure =
        OFlounceSpiral::Build(start->toQPointF(), width, length, angle,
                              {start->name(), PointName(OFlounceSpiralPoint::Role::OuterEnd)},
                              {PointName(OFlounceSpiralPoint::Role::OuterStart),
                               PointName(OFlounceSpiralPoint::Role::OuterEnd)});

    // Trace of the construction: the four corners of the band and the distance between them. The two closing
    // segments are the pairs exactly one band width apart; anything else is a segment across the figure. Read the
    // numbers here rather than the picture — they are what the file will hold.
    {
        auto Distance = [](const QPointF &one, const QPointF &other)
        { return std::hypot(one.x() - other.x(), one.y() - other.y()); };
        qDebug() << "FLOUNCE-FIGURE: largeur =" << figure.bandWidth << " longueur =" << figure.innerLength
                 << " externe =" << figure.outerLength << " angle balaye ="
                 << figure.sweptAngle * 180.0 / M_PI;
        qDebug() << "FLOUNCE-FIGURE:   depart    =" << figure.start << start->name()
                 << " id =" << m_startPointId;
        qDebug() << "FLOUNCE-FIGURE:   fin interne=" << figure.innerEnd << PointName(OFlounceSpiralPoint::Role::InnerEnd)
                 << " id =" << PointId(OFlounceSpiralPoint::Role::InnerEnd);
        qDebug() << "FLOUNCE-FIGURE:   debut ext.=" << figure.outerStart << PointName(OFlounceSpiralPoint::Role::OuterStart)
                 << " id =" << PointId(OFlounceSpiralPoint::Role::OuterStart);
        qDebug() << "FLOUNCE-FIGURE:   fin ext.  =" << figure.outerEnd << PointName(OFlounceSpiralPoint::Role::OuterEnd)
                 << " id =" << PointId(OFlounceSpiralPoint::Role::OuterEnd);
        qDebug() << "FLOUNCE-FIGURE:   d(depart, fin ext.)   =" << Distance(figure.start, figure.outerEnd)
                 << " <- ligne 1, doit valoir la largeur";
        qDebug() << "FLOUNCE-FIGURE:   d(fin int., debut ext.)=" << Distance(figure.innerEnd, figure.outerStart)
                 << " <- ligne 2, doit valoir la largeur";
        qDebug() << "FLOUNCE-FIGURE:   d(depart, debut ext.)  =" << Distance(figure.start, figure.outerStart)
                 << " (croise)";
        qDebug() << "FLOUNCE-FIGURE:   d(fin int., fin ext.)  =" << Distance(figure.innerEnd, figure.outerEnd)
                 << " (croise)";
    }

    auto inner = QSharedPointer<VSplinePath>::create(figure.begin);
    auto outer = QSharedPointer<VSplinePath>::create(figure.outer);
    inner->setIdObject(m_id);
    outer->setIdObject(m_id);
    inner->SetPenStyle(m_display.typeLine);
    outer->SetPenStyle(m_display.typeLine);
    inner->SetColor(m_display.lineColor);
    outer->SetColor(m_display.lineColor);
    // The single name of the figure is the outer curve's; the inner one keeps the name an older pattern gave it.
    inner->SetAliasSuffix(m_display.innerAlias);
    outer->SetAliasSuffix(m_display.outerAlias);

    VAbstractTool::data.UpdateGObject(m_display.inner, inner);
    VAbstractTool::data.UpdateGObject(m_display.outer, outer);

    auto MovePoint = [this](OFlounceSpiralPoint::Role role, const QPointF &position, const QString &name)
    {
        const quint32 id = OFlounceSpiralDisplay::PointId(m_display, role);
        if (id == NULL_ID)
        {
            return;
        }
        QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(id);
        if (point.isNull())
        {
            return;
        }
        point->setX(position.x());
        point->setY(position.y());
        point->setName(name);
        VAbstractTool::data.UpdateGObject(id, point);
    };

    MovePoint(OFlounceSpiralPoint::Role::OuterStart, figure.outerStart, PointName(OFlounceSpiralPoint::Role::OuterStart));
    MovePoint(OFlounceSpiralPoint::Role::InnerEnd, figure.innerEnd, PointName(OFlounceSpiralPoint::Role::InnerEnd));
    MovePoint(OFlounceSpiralPoint::Role::OuterEnd, figure.outerEnd, PointName(OFlounceSpiralPoint::Role::OuterEnd));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::EnsureLines()
{
    // The two segments are described in the display; the base only needs to be told what they are made of.
    m_lines.clear();

    auto Append = [this](quint32 result, quint32 first, quint32 second)
    {
        if (result == NULL_ID || first == NULL_ID || second == NULL_ID)
        {
            return;
        }
        OOperationLine line;
        line.source = NULL_ID;
        line.point1 = first;
        line.point2 = second;
        line.result = result;
        line.typeLine = m_display.typeLine;
        line.lineColor = m_display.lineColor;
        m_lines.append(line);
    };

    Append(m_display.line1, m_startPointId, PointId(OFlounceSpiralPoint::Role::OuterEnd));
    Append(m_display.line2, PointId(OFlounceSpiralPoint::Role::InnerEnd),
           PointId(OFlounceSpiralPoint::Role::OuterStart));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SyncSourceDestination()
{
    // The schema imposes both elements, and a reader of a pattern expects the figure of an operation to say which
    // objects it produced. The two edges and the three points are stated here as well as in the figure: the file
    // then says in the standard place what it also says in its own.
    m_source.clear();

    if (m_startPointId != NULL_ID)
    {
        SourceItem source;
        source.id = m_startPointId;
        source.recordId = QUuid::createUuid();
        m_source.append(source);
    }

    m_destination.clear();

    auto AppendDestination = [this](quint32 id, bool showLabel)
    {
        if (id == NULL_ID)
        {
            return;
        }
        DestinationItem destination;
        destination.id = id;
        destination.recordId = QUuid::createUuid();
        // The name of a point is shown only when the user asked for it, and this item is the only place the schema
        // offers an operation to say so — hence the switch is read here, on every save, and never assumed false.
        destination.showLabel = showLabel;
        m_destination.append(destination);
    };

    // An edge has no label of its own in this figure: the curves are not named on the canvas, only the points are.
    AppendDestination(m_display.inner, false);
    AppendDestination(m_display.outer, false);
    for (const OFlounceSpiralPoint &point : std::as_const(m_display.points))
    {
        AppendDestination(point.id, PointLabelVisible(point.role));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::IsADestination(quint32 id) const -> bool
{
    return std::any_of(m_destination.cbegin(),
                       m_destination.cend(),
                       [id](const DestinationItem &item) -> bool { return item.id == id; });
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::AttachFigureToLayer()
{
    if (doc == nullptr)
    {
        return;
    }

    const quint32 active = doc->GetActiveGroup();
    if (active == NULL_ID)
    {
        return; // the piece has no layer at all: nothing to draw into yet
    }

    QVector<QPair<quint32, quint32>> objects;

    // The ids of the figure, paired EXACTLY as the generic reader of the element pairs them — the two edges and
    // the three points through <destination>, the two segments through <lines>. Writing a different pair here for
    // the same id is what put each edge in the layer twice, under two owners, and a layer that holds an object
    // twice cannot say where it belongs: the figure came out as belonging to no layer at all.
    for (const quint32 id : {m_display.inner, m_display.outer})
    {
        if (id != NULL_ID)
        {
            objects.append(qMakePair(id, m_id));
        }
    }

    for (const OFlounceSpiralPoint &point : std::as_const(m_display.points))
    {
        if (point.id != NULL_ID)
        {
            objects.append(qMakePair(point.id, m_id));
        }
    }

    // A segment answers to its own id, like every line: it is a tool of its own, and <lines> names it that way.
    for (const quint32 id : {m_display.line1, m_display.line2})
    {
        if (id != NULL_ID)
        {
            objects.append(qMakePair(id, id));
        }
    }

    doc->AddItemsToGroup(objects, active);

    qDebug() << "FLOUNCE-LAYER: calque actif =" << active << " objets =" << objects.size();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::PointId(OFlounceSpiralPoint::Role role) const -> quint32
{
    return OFlounceSpiralDisplay::PointId(m_display, role);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::ExtraPointIds() const -> QVector<quint32>
{
    QVector<quint32> ids;
    for (const OFlounceSpiralPoint &point : m_display.points)
    {
        if (point.id != NULL_ID && !IsADestination(point.id))
        {
            ids.append(point.id);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::ExtraCurveIds() const -> QVector<quint32>
{
    QVector<quint32> ids;
    for (const quint32 id : {m_display.inner, m_display.outer})
    {
        if (id != NULL_ID && !IsADestination(id))
        {
            ids.append(id);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::ExtraLineIds() const -> QVector<quint32>
{
    // Empty on purpose: both segments are already in m_lines, and the base builds a scene item for each of them.
    // Returning them here as well ran InitOperatedLine twice per segment and connected ContextMenuRequested twice,
    // so a right click on a segment opened the operation menu twice in a row. Removing them from the operation's
    // dependency test is not a concern: IsRemovable unions m_lines with this list.
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::getTagName() const -> QString
{
    return VAbstractPattern::TagOperation;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetDisplay(const OFlounceSpiralDisplay &display)
{
    m_display.typeLine = display.typeLine;
    m_display.lineColor = display.lineColor;
    m_display.width = display.width;
    m_display.length = display.length;
    m_display.angle = display.angle;
    // The names come from the display, the switches from the points themselves: a whole figure is handed over
    // here, and the caller is not writing the element — nothing is persisted, the next save reads it all back.
    m_display.innerAlias = display.innerAlias;
    m_display.outerAlias = display.outerAlias;

    for (const OFlounceSpiralPoint &point : display.points)
    {
        ApplyPointName(point.role, point.name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::GetPenStyle() const -> QString
{
    return m_display.typeLine;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetPenStyle(const QString &penStyle)
{
    if (m_display.typeLine == penStyle)
    {
        return;
    }
    m_display.typeLine = penStyle;
    PersistFigure();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::GetLineColor() const -> QString
{
    return m_display.lineColor;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetLineColor(const QString &color)
{
    if (m_display.lineColor == color)
    {
        return;
    }
    m_display.lineColor = color;
    PersistFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::PersistFigure()
{
    // GetFakeGObject, not GetGObject: an operation has no object of its own in the container, so GetGObject throws
    // VExceptionBadId and the edit dies before it is written ("Property \"flounceWidth\" could not be applied").
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::PointName(OFlounceSpiralPoint::Role role) const -> QString
{
    const quint32 id = PointId(role);
    if (id == NULL_ID)
    {
        return {};
    }
    try
    {
        return VAbstractTool::data.GeometricObject<VPointF>(id)->name();
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetPointName(OFlounceSpiralPoint::Role role, const QString &name)
{
    if (name.isEmpty() || name == PointName(role))
    {
        return; // a point is never left without a name, and an unchanged one is not rewritten
    }

    // A name the pattern cannot carry, or one another object already answers to, is refused in silence: the caller
    // — the property browser — puts the old name back in front of the user. The rule lives HERE, and not in the
    // dock, because it is a rule of the figure: a point of the pattern is named the way every other one is.
    if (QRegularExpression const rx(NameRegExp()); not rx.match(name).hasMatch())
    {
        return;
    }
    if (not VContainer::IsUnique(name, valentinaNamespace))
    {
        return;
    }

    ApplyPointName(role, name);
    PersistFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::ApplyPointName(OFlounceSpiralPoint::Role role, const QString &name)
{
    if (name.isEmpty())
    {
        return;
    }

    const quint32 id = PointId(role);
    if (id == NULL_ID)
    {
        return;
    }

    for (OFlounceSpiralPoint &point : m_display.points)
    {
        if (point.role == role)
        {
            point.name = name;
            break;
        }
    }

    // The parse reads the name on the point itself, never on a copy kept by the tool, so the name is written
    // there: it is the point that carries it, exactly as a point created by any other tool.
    try
    {
        QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(id);
        point->setName(name);
        VAbstractTool::data.UpdateGObject(id, point);
    }
    catch (const VExceptionBadId &)
    {
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::PointLabelVisible(OFlounceSpiralPoint::Role role) const -> bool
{
    // Asked of the operation, not of the container object alone: the base holds the switch in the three places it
    // has to live — the object the scene draws, the destination record the property browser toggles, and the file.
    return VAbstractOperation::IsLabelVisible(PointId(role));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetPointLabelVisible(OFlounceSpiralPoint::Role role, bool visible)
{
    const quint32 id = PointId(role);
    if (id == NULL_ID || PointLabelVisible(role) == visible)
    {
        return;
    }

    // The base does the whole work: the point object, the scene item, the destination record and the attribute in
    // the document. Writing only the container object — as this did — left the switch in memory alone, and the next
    // parse put the name away again.
    VAbstractOperation::SetLabelVisible(id, visible);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::ApplyPointLabelVisible(OFlounceSpiralPoint::Role role, bool visible)
{
    const quint32 id = PointId(role);
    if (id == NULL_ID)
    {
        return;
    }
    try
    {
        QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(id);
        point->SetShowLabel(visible);
        VAbstractTool::data.UpdateGObject(id, point);
    }
    catch (const VExceptionBadId &)
    {
        // The point is not in the container yet: the next parse will place it, and the switch follows the file.
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::OuterAlias() const -> QString
{
    return m_display.outerAlias;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetOuterAlias(const QString &alias)
{
    if (m_display.outerAlias == alias)
    {
        return;
    }

    if (not alias.isEmpty())
    {
        // The user types a suffix, the object answers with the complete alias: the complete one is what has to be
        // well shaped and free. A value that is neither is refused in silence, which is the contract
        // MakeStringPropWithValidator expects — it puts the old name back in front of the user.
        VSplinePath probe;
        probe.SetAliasSuffix(alias);
        const QString fullAlias = probe.GetAlias();
        if (QRegularExpression const rx(NameRegExp()); not rx.match(fullAlias).hasMatch())
        {
            return;
        }
        if (not VAbstractTool::data.IsUnique(fullAlias))
        {
            return;
        }
    }

    m_display.outerAlias = alias;
    try
    {
        QSharedPointer<VSplinePath> curve = VAbstractTool::data.GeometricObject<VSplinePath>(m_display.outer);
        curve->SetAliasSuffix(alias);
        VAbstractTool::data.UpdateGObject(m_display.outer, curve);
    }
    catch (const VExceptionBadId &)
    {
    }

    // The name is part of the element the figure is written in: without this call the edit stayed in memory until
    // some other option happened to save the tool.
    PersistFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogFlounceSpiral> dialogTool = qobject_cast<ODialogFlounceSpiral *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetStartPointId(m_startPointId);
    dialogTool->SetDisplay(m_display);
    dialogTool->SetOuterAlias(m_display.outerAlias);

    // The switch that shows a name is carried by the point itself, not by the figure's element: the dialog asks
    // the tool for it, the way it asks for the names through the display.
    dialogTool->SetPointShowLabel1(PointLabelVisible(OFlounceSpiralPoint::Role::OuterStart));
    dialogTool->SetPointShowLabel2(PointLabelVisible(OFlounceSpiralPoint::Role::InnerEnd));
    dialogTool->SetPointShowLabel3(PointLabelVisible(OFlounceSpiralPoint::Role::OuterEnd));

    dialogTool->SetNotes(m_notes);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SaveDialog(QDomElement &domElement)
{
    qDebug() << "FLOUNCE-SAVEDIALOG: entree";
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogFlounceSpiral> dialogTool = qobject_cast<ODialogFlounceSpiral *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    // The dialog knows the names and the values, not the ids: the display it builds carries none. Taking it whole
    // would wipe every id of the figure and write a file whose points have no entry — the figure would come back
    // without them. So the ids and the roles stay the ones the tool already holds, and only the values are taken.
    const OFlounceSpiralDisplay fromDialog = dialogTool->GetDisplay();

    m_display.width = fromDialog.width;
    m_display.length = fromDialog.length;
    m_display.angle = fromDialog.angle;
    // One name, the outer curve's. The inner one is NOT taken from the dialog — nothing offers it there any more —
    // and the value the tool already holds is kept: a pattern that names the inner curve keeps its variable.
    m_display.outerAlias = dialogTool->GetOuterAlias();
    m_display.typeLine = dialogTool->GetPenStyle();
    m_display.lineColor = dialogTool->GetColor();

    // The names go on the points themselves, and so do the switches of the Points tab: SyncSourceDestination,
    // called below, reads both from the points the container holds. The switch is written on the object alone —
    // writing the attribute here would touch the element this very call is replacing.
    for (const OFlounceSpiralPoint &point : std::as_const(fromDialog.points))
    {
        ApplyPointName(point.role, point.name);
        ApplyPointLabelVisible(point.role, point.labelVisible);
    }

    doc->SetAttribute(domElement, AttrType, ToolType);
    doc->SetAttribute(domElement, AttrFirstPoint, m_startPointId);

    // The dialog edits a CLONE of the element, children included, so the whole element is rebuilt from scratch
    // here. SaveSourceDestination wipes the children and writes <source>, <destination> and <lines>; appending
    // the figure on top of the clone instead left a second <flounceSpiral> in the element, which the schema
    // refuses. This is the pleat's order, word for word.
    EnsureLines();
    SyncSourceDestination();
    SaveSourceDestination(domElement, m_source, m_destination);

    OFlounceSpiralDisplay::Save(doc, domElement, m_display);

    // A figure edited in the dialog enters the layer too: what it now holds is not what the layer last recorded.
    AttachFigureToLayer();

    qDebug() << "FLOUNCE-SAVEDIALOG: ecrit, startPointId =" << m_startPointId
             << " points =" << m_display.points.size() << " enfants =" << ChildrenOf(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    // The lines written below are the ones the figure really has: the base writes m_lines, so m_lines has to be
    // rebuilt from the display before anything is written.
    EnsureLines();
    SyncSourceDestination();

    // Writes the attributes, then <source>, <destination> and <lines>, in the order the schema imposes.
    // <lines> is written by THAT call and by no other: a second one, added by hand, put two <lines> elements in
    // the operation and made every save fail with "element 'lines' is not allowed for content model
    // (source, destination, lines?, …)" — the first one had already consumed the single slot the model allows.
    // Beware, it also REMOVES every child of the tag it is given (VAbstractPattern::RemoveAllChildren).
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrFirstPoint, m_startPointId);

    // Last child of the operation in the schema, so written last.
    OFlounceSpiralDisplay::Save(doc, tag, m_display);

    // The objects the change has just described belong to the active layer, like every object (the pleat's rule).
    AttachFigureToLayer();

    qDebug() << "FLOUNCE-SAVEOPTIONS: id =" << m_id << " lignes =" << m_lines.size()
             << " enfants =" << ChildrenOf(tag);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::ReadToolAttributes(const QDomElement &domElement)
{
    VDrawTool::ReadToolAttributes(domElement);

    m_startPointId = VDomDocument::GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
    m_display = OFlounceSpiralDisplay::Extract(domElement);
    m_lines = OOperationLines::ExtractLines(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    // The change has to reach the file, the undo stack and the scene. Doing only the third — as this method did —
    // made an edit made in the dialog disappear as soon as the pattern was saved: the element in the document was
    // never replaced. The base class replaces it and pushes the undo command; the figure follows underneath.
    qDebug() << "FLOUNCE-APPLY: ancien =" << ChildrenOf(oldDomElement)
             << " nouveau =" << ChildrenOf(newDomElement);

    VAbstractOperation::ApplyToolOptions(oldDomElement, newDomElement);

    RefreshFigure();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    // Trace: the context menu is built by RunContextMenu, which ALWAYS adds "Options" — so a right click that
    // shows nothing never reached the tool. This says which object id arrived, and whether the menu is suppressed
    // for the whole application at that moment.
    qDebug() << "FLOUNCE-CLIC: id recu =" << id << " menu supprime =" << m_suppressContextMenu;

    // The one call every other operation makes. It is ContextMenu<Dialog>(event, id) inside the shared
    // VExceptionToolWasDeleted catch, so the body is the same — and the id is what lets the layer entries speak
    // about the object that was clicked instead of about the tool.
    ShowContextMenuOf<ODialogFlounceSpiral>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::ShowVisualization(bool show)
{
    Q_UNUSED(show)
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetVisualization()
{
    // Nothing to draw beyond the objects themselves: the two curves and the two segments are the figure.
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::GetFormulaWidth() const -> VFormula
{
    VFormula width(m_display.width, getData());
    width.setToolId(m_id);
    width.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    width.Eval();
    return width;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetFormulaWidth(const VFormula &value)
{
    if (value.error())
    {
        return;
    }
    m_display.width = value.GetFormula(FormulaType::FromUser);
    PersistFigure();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::GetFormulaLength() const -> VFormula
{
    VFormula length(m_display.length, getData());
    length.setToolId(m_id);
    length.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    length.Eval();
    return length;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetFormulaLength(const VFormula &value)
{
    if (value.error())
    {
        return;
    }
    m_display.length = value.GetFormula(FormulaType::FromUser);
    PersistFigure();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolFlounceSpiral::GetFormulaAngle() const -> VFormula
{
    VFormula angle(m_display.angle, getData());
    angle.setToolId(m_id);
    angle.setPostfix(degreeSymbol);
    angle.Eval();
    return angle;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolFlounceSpiral::SetFormulaAngle(const VFormula &value)
{
    if (value.error())
    {
        return;
    }
    m_display.angle = value.GetFormula(FormulaType::FromUser);
    PersistFigure();
}
