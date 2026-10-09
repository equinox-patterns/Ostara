/************************************************************************
 **
 **  @file   olayergroupcommands.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Undo commands for the groups of layers — the level above the
 **          layers themselves.
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

#include "olayergroupcommands.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"

//AddLayerGroup
//---------------------------------------------------------------------------------------------------------------------
AddLayerGroup::AddLayerGroup(const QDomElement &xml, VAbstractPattern *doc, QUndoCommand *parent)
  : VUndoCommand(xml, doc, parent),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId())
{
    setText(tr("add a group of layers"));
    SetElementId(VDomDocument::GetParametrId(xml));
}

//---------------------------------------------------------------------------------------------------------------------
void AddLayerGroup::undo()
{
    qCDebug(vUndo, "Undo.");

    Do(false);
}

//---------------------------------------------------------------------------------------------------------------------
void AddLayerGroup::redo()
{
    qCDebug(vUndo, "Redo.");

    Do(true);
}

//---------------------------------------------------------------------------------------------------------------------
void AddLayerGroup::Do(bool restore)
{
    // The command was built on a given piece: the active one is put back so the group lands in (or leaves) the
    // piece it was created for, whatever the user is showing when the undo runs.
    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock);

    QDomElement layerGroups = Doc()->CreateLayerGroups();

    if (layerGroups.isNull())
    {
        qCDebug(vUndo, "Can't get tag LayerGroups.");
        return;
    }

    if (restore)
    {
        layerGroups.appendChild(GetElement());
        emit UpdateGroups();
        return;
    }

    QDomElement layerGroup = Doc()->FindElementById(ElementId(), VAbstractPattern::TagLayerGroup);

    if (not layerGroup.isElement())
    {
        qCDebug(vUndo, "Can't get layer group by id = %u.", ElementId());
        return;
    }

    if (layerGroups.removeChild(layerGroup).isNull())
    {
        qCDebug(vUndo, "Can't delete layer group.");
        return;
    }

    emit UpdateGroups();

    // The block itself goes when the last group leaves it: an empty `<layerGroups>` is noise in the file, and
    // CreateLayerGroups puts it back the day it is needed.
    if (layerGroups.childNodes().isEmpty())
    {
        QDomNode parent = layerGroups.parentNode();
        parent.removeChild(layerGroups);
    }
}

//DelLayerGroup
//---------------------------------------------------------------------------------------------------------------------
DelLayerGroup::DelLayerGroup(VAbstractPattern *doc, quint32 id, QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId()),
    m_pieceName(doc->PatternBlockMapper()->GetActive())
{
    setText(tr("delete a group of layers"));
    SetElement(doc->CloneNodeById(id));
}

//---------------------------------------------------------------------------------------------------------------------
void DelLayerGroup::undo()
{
    qCDebug(vUndo, "Undo.");

    Do(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DelLayerGroup::redo()
{
    qCDebug(vUndo, "Redo.");

    Do(false);
}

//---------------------------------------------------------------------------------------------------------------------
void DelLayerGroup::Do(bool restore)
{
    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock);

    QDomElement layerGroups = Doc()->CreateLayerGroups(m_pieceName);

    if (layerGroups.isNull())
    {
        qCDebug(vUndo, "Can't get tag LayerGroups.");
        return;
    }

    if (restore)
    {
        layerGroups.appendChild(GetElement());
        emit UpdateGroups();
        return;
    }

    QDomElement layerGroup = Doc()->FindElementById(ElementId(), VAbstractPattern::TagLayerGroup);

    if (not layerGroup.isElement())
    {
        qCDebug(vUndo, "Can't get layer group by id = %u.", ElementId());
        return;
    }

    if (layerGroups.removeChild(layerGroup).isNull())
    {
        qCDebug(vUndo, "Can't delete layer group.");
        return;
    }

    emit UpdateGroups();

    if (layerGroups.childNodes().isEmpty())
    {
        QDomNode parent = layerGroups.parentNode();
        parent.removeChild(layerGroups);
    }
}

//ChangeLayerGroupOptions
//---------------------------------------------------------------------------------------------------------------------
ChangeLayerGroupOptions::ChangeLayerGroupOptions(VAbstractPattern *doc,
                                                 quint32 id,
                                                 const QString &name,
                                                 const QVector<quint32> &layers,
                                                 QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    m_newName(name),
    m_newLayers(layers),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId())
{
    setText(tr("edit a group of layers"));
    m_oldName = doc->GetLayerGroupName(id);
    m_oldLayers = doc->LayerGroupLayers(id);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayerGroupOptions::undo()
{
    qCDebug(vUndo, "Undo.");

    Do(m_oldName, m_oldLayers);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayerGroupOptions::redo()
{
    qCDebug(vUndo, "Redo.");

    Do(m_newName, m_newLayers);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayerGroupOptions::Do(const QString &name, const QVector<quint32> &layers)
{
    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock);
    Doc()->SetLayerGroupName(ElementId(), name);

    for (const quint32 layer : layers)
    {
        Doc()->SetLayerGroup(layer, ElementId());
    }

    // The dialog settles the whole content of the group, it does not add to it: a layer that was in it and is not
    // in the list any more leaves it — back to the top level, where the user can gather it again.
    const QVector<quint32> current = Doc()->LayerGroupLayers(ElementId());
    for (const quint32 layer : current)
    {
        if (not layers.contains(layer))
        {
            Doc()->SetLayerGroup(layer, NULL_ID);
        }
    }

    emit UpdateGroups();
}

//ChangeLayerGroupCollapsed
//---------------------------------------------------------------------------------------------------------------------
ChangeLayerGroupCollapsed::ChangeLayerGroupCollapsed(VAbstractPattern *doc,
                                                     quint32 id,
                                                     bool collapsed,
                                                     QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    m_newCollapsed(collapsed)
{
    setText(tr("fold a group of layers"));
    m_oldCollapsed = doc->GetLayerGroupCollapsed(id);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayerGroupCollapsed::undo()
{
    qCDebug(vUndo, "Undo.");

    Do(m_oldCollapsed);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayerGroupCollapsed::redo()
{
    qCDebug(vUndo, "Redo.");

    Do(m_newCollapsed);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayerGroupCollapsed::Do(bool collapsed)
{
    Doc()->SetLayerGroupCollapsed(ElementId(), collapsed);
    emit UpdateGroups();
}

//ChangeLayersGroup
//---------------------------------------------------------------------------------------------------------------------
ChangeLayersGroup::ChangeLayersGroup(VAbstractPattern *doc,
                                     const QVector<quint32> &layers,
                                     quint32 groupId,
                                     QUndoCommand *parent)
  : VUndoCommand(doc, parent),
    m_layers(layers),
    m_groupId(groupId),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId())
{
    setText(tr("change the group of layers"));

    for (const quint32 layer : layers)
    {
        m_oldGroups.insert(layer, doc->GetLayerGroup(layer));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayersGroup::undo()
{
    qCDebug(vUndo, "Undo.");

    Do(m_oldGroups);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayersGroup::redo()
{
    qCDebug(vUndo, "Redo.");

    QMap<quint32, quint32> groups;
    for (const quint32 layer : m_layers)
    {
        groups.insert(layer, m_groupId);
    }

    Do(groups);
}

//---------------------------------------------------------------------------------------------------------------------
void ChangeLayersGroup::Do(const QMap<quint32, quint32> &groups)
{
    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock);

    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
    {
        Doc()->SetLayerGroup(it.key(), it.value());
    }

    emit UpdateGroups();
}
