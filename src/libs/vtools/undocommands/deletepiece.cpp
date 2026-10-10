/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   9 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

#include "deletepiece.h"

#include <QDomElement>
#include <QHash>
#include <QSet>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "vundocommand.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// The ids of the `<node>` children of @a nodes — the objects a piece path is drawn through.
void CollectNodeIds(const QDomElement &nodes, QSet<vidtype> &ids)
{
    for (QDomElement node = nodes.firstChildElement(VAbstractPattern::TagNode); !node.isNull();
         node = node.nextSiblingElement(VAbstractPattern::TagNode))
    {
        if (const vidtype id = VDomDocument::GetParametrUInt(node, AttrIdObject, NULL_ID_STR); id != NULL_ID)
        {
            ids.insert(id);
        }
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
DeletePiece::DeletePiece(
    VAbstractPattern *doc, quint32 id, const VContainer &data, VMainGraphicsScene *scene, QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    m_detail(data.GetPiece(id)),
    m_data(data),
    m_scene(scene),
    m_record(VAbstractTool::GetRecord(id, Tool::Piece, doc))
{
    setText(tr("delete tool"));
    QDomElement const domElement = doc->FindElementById(id, VAbstractPattern::TagDetail);
    if (!domElement.isElement())
    {
        qCDebug(vUndo, "Can't get detail by id = %u.", id);
        return;
    }

    SetElement(domElement.cloneNode().toElement());

    // What the piece owns besides its <detail>: the node copies it draws through and the internal paths it
    // holds. They go with it (see CollectOwnedModeling).
    CollectOwnedModeling(domElement);

    m_parentNode = domElement.parentNode();
    QDomNode const previousDetail = domElement.previousSibling();
    if (previousDetail.isNull())
    {
        m_siblingId = NULL_ID;
    }
    else
    {
        // Better save id of previous detail instead of reference to node.
        m_siblingId = VAbstractPattern::GetParametrUInt(previousDetail.toElement(), VDomDocument::AttrId, NULL_ID_STR);
    }

    VPatternGraph const *patternGraph = doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    if (const auto node = patternGraph->GetVertex(id))
    {
        m_indexPatternBlock = node->indexPatternBlock;
    }
}

//---------------------------------------------------------------------------------------------------------------------
DeletePiece::~DeletePiece()
{
    delete m_tool.data();
}

//---------------------------------------------------------------------------------------------------------------------
auto DeletePiece::ModelingRefsOf(const QDomElement &detail) const -> PieceModelingRefs
{
    PieceModelingRefs refs;

    // The nodes of the contour, then those of each internal path the <iPaths> element names. An internal path
    // is an element of <modeling> of its own, exactly like the node copies.
    CollectNodeIds(detail.firstChildElement(VAbstractPattern::TagNodes), refs.nodes);

    const QDomElement iPaths = detail.firstChildElement(VToolSeamAllowance::TagIPaths);
    for (QDomElement record = iPaths.firstChildElement(VToolSeamAllowance::TagRecord); !record.isNull();
         record = record.nextSiblingElement(VToolSeamAllowance::TagRecord))
    {
        const vidtype pathId = VDomDocument::GetParametrUInt(record, VAbstractPattern::AttrPath, NULL_ID_STR);
        if (pathId == NULL_ID)
        {
            continue;
        }

        if (const QDomElement path = Doc()->FindElementById(pathId, VAbstractPattern::TagPath); !path.isNull())
        {
            refs.paths.append(path);
            CollectNodeIds(path.firstChildElement(VAbstractPattern::TagNodes), refs.nodes);
        }
    }

    return refs;
}

//---------------------------------------------------------------------------------------------------------------------
void DeletePiece::CollectOwnedModeling(const QDomElement &detail)
{
    // A piece draws through objects of its own: the node copies its contour names, and the <path> elements of
    // its internal paths together with the node copies those name. Every one of them lives in <modeling> as an
    // element of its own, so removing the <detail> takes none of them with it.
    //
    // Left behind, they poison the pattern. The node copy keeps an id nothing else answers to; when the object
    // it wraps is deleted in turn — which the deletion check allows, since nothing reaches a piece through the
    // copy any more — the file holds an element naming an object that does not exist. The next parse then
    // reports "Broken relation" and leaves the path registered in the container with nodes nothing can
    // resolve, and the tool that reads it throws from FullUpdateFromFile: the exception reaches
    // VPattern::LiteParseTree, which disables the whole interface (VToolPiecePath::RefreshGeometry). So they
    // leave with the piece — and come back with it, this being an undo command.
    const PieceModelingRefs mine = ModelingRefsOf(detail);

    // Nothing is taken that another piece still draws through: the same copy, and the same internal path, can
    // be shared between pieces. The question is asked of the file, and not of the dependency graph: the
    // vertices of the piece being deleted are still there at this point.
    QSet<vidtype> nodesUsedElsewhere;
    QSet<vidtype> pathsUsedElsewhere;
    const QDomNodeList details = Doc()->elementsByTagName(VAbstractPattern::TagDetail);
    for (int i = 0; i < details.size(); ++i)
    {
        const QDomElement other = details.at(i).toElement();
        if (other.isNull() || other.attribute(VDomDocument::AttrId).toUInt() == ElementId())
        {
            continue;
        }

        const PieceModelingRefs refs = ModelingRefsOf(other);
        nodesUsedElsewhere.unite(refs.nodes);
        for (const QDomElement &path : refs.paths)
        {
            pathsUsedElsewhere.insert(path.attribute(VDomDocument::AttrId).toUInt());
        }
    }

    // An element is remembered once, whatever the number of places the piece names it from: a file could name
    // the same path twice, and putting two copies of one id back on undo would give the pattern two elements
    // for a single object.
    QSet<vidtype> remembered;
    auto Remember = [this, &remembered](const QDomElement &parent, const QDomElement &element)
    {
        if (parent.isNull() || element.isNull())
        {
            return;
        }

        const vidtype id = element.attribute(VDomDocument::AttrId).toUInt();
        if (id == NULL_ID || remembered.contains(id))
        {
            return;
        }
        remembered.insert(id);
        m_ownedModeling.append({parent, element.cloneNode().toElement()});
    };

    for (const QDomElement &path : mine.paths)
    {
        const vidtype pathId = path.attribute(VDomDocument::AttrId).toUInt();
        if (pathId != NULL_ID && !pathsUsedElsewhere.contains(pathId))
        {
            Remember(path.parentNode().toElement(), path);
        }
    }

    for (const vidtype nodeId : mine.nodes)
    {
        if (nodesUsedElsewhere.contains(nodeId))
        {
            continue;
        }

        const QDomElement node = Doc()->FindElementById(nodeId);
        if (node.isNull())
        {
            continue;
        }

        // Only a copy inside <modeling> belongs to the piece. The same id can name a node of the piece and,
        // in a file that had it, an object of the calculation block — that one is not the piece's to take.
        const QDomElement parent = node.parentNode().toElement();
        if (parent.isNull() || parent.tagName() != VAbstractPattern::TagModeling)
        {
            continue;
        }
        Remember(parent, node);
    }

    qCDebug(vUndo, "Piece %u takes %lld object(s) of <modeling> with it.", ElementId(),
            static_cast<long long>(m_ownedModeling.size()));
}

//---------------------------------------------------------------------------------------------------------------------
void DeletePiece::TakeOwnedModeling()
{
    // The live element is looked for in the parent it was read from, by tag and id: the command can be redone
    // long after the file has been written again, and a QDomElement read back then would no longer be in
    // this tree.
    for (OwnedModelingElement &owned : m_ownedModeling)
    {
        if (owned.parent.isNull() || owned.element.isNull())
        {
            continue;
        }

        const QString tag = owned.element.tagName();
        const QString id = owned.element.attribute(VDomDocument::AttrId);
        for (QDomElement child = owned.parent.firstChildElement(tag); !child.isNull();
             child = child.nextSiblingElement(tag))
        {
            if (child.attribute(VDomDocument::AttrId) == id)
            {
                owned.parent.removeChild(child);
                break;
            }
        }
    }
    Doc()->RefreshElementIdCache();
}

//---------------------------------------------------------------------------------------------------------------------
void DeletePiece::RestoreOwnedModeling()
{
    for (OwnedModelingElement &owned : m_ownedModeling)
    {
        if (owned.parent.isNull() || owned.element.isNull())
        {
            continue;
        }
        owned.parent.appendChild(owned.element);
    }
    Doc()->RefreshElementIdCache();
}

//---------------------------------------------------------------------------------------------------------------------
void DeletePiece::undo()
{
    qCDebug(vUndo, "Undo.");

    UndoDeleteAfterSibling(m_parentNode, m_siblingId, VAbstractPattern::TagDetail);

    // The node copies and the internal paths the piece draws through are put back before anything reads the
    // piece: they are what its nodes name.
    RestoreOwnedModeling();

    if (not m_tool.isNull())
    {
        VAbstractPattern::AddTool(ElementId(), m_tool);
    }
    m_data.UpdatePiece(ElementId(), m_detail);

    VPatternGraph *patternGraph = Doc()->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(ElementId(), VNodeType::PIECE, m_indexPatternBlock);

    const auto varData = m_data.DataDependencyVariables();
    VToolSeamAllowance::AddPieceDependencies(ElementId(), m_detail, Doc(), varData);

    VAbstractTool::AddRecord(m_record, Doc());
    if (not m_tool.isNull())
    {
        m_tool->ReinitInternals(m_detail, m_scene);
        m_scene->addItem(m_tool);
        m_tool->ConnectOutsideSignals();
        m_tool->show();
        VMainGraphicsView::NewSceneRect(m_scene, VAbstractValApplication::VApp()->getSceneView(), m_tool);
        m_tool.clear();
    }
    emit Doc()->UpdateInLayoutList();
}

//---------------------------------------------------------------------------------------------------------------------
void DeletePiece::redo()
{
    qCDebug(vUndo, "Redo.");

    QDomElement const domElement = Doc()->FindElementById(ElementId(), VAbstractPattern::TagDetail);
    if (!domElement.isElement())
    {
        qCDebug(vUndo, "Can't get detail by id = %u.", ElementId());
        return;
    }

    m_parentNode.removeChild(domElement);

    // The piece's node copies and its internal paths leave with it: nothing of the pattern may be left naming
    // an object that no longer exists (see CollectOwnedModeling).
    TakeOwnedModeling();

    m_tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(ElementId()));
    SCASSERT(not m_tool.isNull());
    if (not m_tool.isNull())
    {
        m_tool->DisconnectOutsideSignals();
        m_tool->EnableToolMove(true);
        m_tool->hide();

        m_scene->removeItem(m_tool);
    }

    VAbstractPattern::RemoveTool(ElementId());
    m_data.RemovePiece(ElementId());
    Doc()->getHistory()->removeOne(m_record);

    VPatternGraph *patternGraph = Doc()->PatternGraph();
    SCASSERT(patternGraph != nullptr)
    patternGraph->RemoveVertex(ElementId());

    emit Doc()->UpdateInLayoutList();

    if (Doc()->IsPatternGraphComplete())
    {
        emit Doc()->PatternDependencyGraphCompleted();
    }
}
