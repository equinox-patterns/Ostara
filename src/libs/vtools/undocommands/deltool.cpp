/************************************************************************
 **
 **  @file   deltool.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   13 6, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 01/10/2026.
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

#include "deltool.h"

#include <QDomElement>
#include <QSet>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractvalapplication.h"
#include "vundocommand.h"
#include "../ifc/xml/vpatternblockmapper.h"

//---------------------------------------------------------------------------------------------------------------------
DelTool::DelTool(VAbstractPattern *doc, quint32 id, QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    parentNode(doc->ParentNodeById(id)),
    siblingId(doc->SiblingNodeId(id)),
    nameActivDraw(doc->PatternBlockMapper()->GetActive())
{
    setText(tr("delete tool"));
    SetElement(doc->CloneNodeById(id));

    // The layers are cleaned of everything the deleted element PRODUCED, and not only of the pairs that name it.
    // An operation owns objects of its own: the transformed lines it writes (`<lines><item idResult="24"/>`) are
    // registered in the layers under THEIR ids, so the pair `{24, 24}` no longer names the operation once it is
    // gone. Cleaning `item.second == id` alone left that pair behind for good — measured on a real file: the layer
    // then held an object the document could not draw, the dock refused to delete it because of that entry, and
    // the transfer offered nothing to take out: a layer nobody could get rid of.
    QSet<vidtype> produced{id};
    if (const QDomElement element = doc->FindElementById(id); not element.isNull())
    {
        for (const auto &object : VAbstractPattern::LayerableObjectsOfTool(element, id))
        {
            produced.insert(object.second);
        }
    }

    // What leaves the layers is remembered ENTRY BY ENTRY — (groupId, item) — and not as a copy of the layers
    // themselves. Undo and redo used to rebuild a whole layer from a snapshot taken here, which also wiped
    // everything that layer had gained since: an object created between the deletion and its Undo came back
    // belonging to NO layer at all. Measured on a real file, where six objects of the piece were in none — the
    // layers had simply been rolled back to the moment of the deletion.
    const QMap<quint32, VGroupData> groups = doc->GetGroups(nameActivDraw);
    for (auto i = groups.constBegin(); i != groups.constEnd(); ++i)
    {
        for (const auto &item : i.value().items)
        {
            if (produced.contains(item.second))
            {
                m_removed.append({i.key(), item});
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DelTool::undo()
{
    qCDebug(vUndo, "Undo.");

    UndoDeleteAfterSibling(parentNode, siblingId);

    // The layers take back exactly the entries this deletion took out — whatever else they gained since stays,
    // which is what keeps the objects created in between inside a layer.
    SetRemovedItemsInLayers(true);

    emit NeedFullParsing();

    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation)
    {                                          // Keep last!
        emit Doc()->ShowPatternBlock(nameActivDraw); // Without this user will not see this change
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DelTool::redo()
{
    qCDebug(vUndo, "Redo.");

    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation)
    {                                          // Keep first!
        emit Doc()->ShowPatternBlock(nameActivDraw); // Without this user will not see this change
    }
    QDomElement const domElement = Doc()->NodeById(ElementId());
    parentNode.removeChild(domElement);

    SetRemovedItemsInLayers(false);

    emit NeedFullParsing();
}

//---------------------------------------------------------------------------------------------------------------------
void DelTool::SetRemovedItemsInLayers(bool present) const
{
    for (const auto &entry : std::as_const(m_removed))
    {
        const QPair<vidtype, vidtype> &item = entry.second;

        QDomElement group = Doc()->FindElementById(entry.first, VAbstractPattern::TagGroup);
        if (group.isNull())
        {
            continue; // the layer itself is gone: its own command owns that
        }

        // The entry as the layers write it: `object` is left out when it equals `tool`.
        const auto IsThisItem = [&item](const QDomElement &candidate) -> bool
        {
            const vidtype tool = candidate.attribute(VAbstractPattern::AttrTool).toUInt();
            const vidtype object = candidate.hasAttribute(VAbstractPattern::AttrObject)
                                       ? candidate.attribute(VAbstractPattern::AttrObject).toUInt()
                                       : tool;
            return tool == item.second && object == item.first;
        };

        QDomElement found;
        for (QDomElement candidate = group.firstChildElement(VAbstractPattern::TagGroupItem); !candidate.isNull();
             candidate = candidate.nextSiblingElement(VAbstractPattern::TagGroupItem))
        {
            if (IsThisItem(candidate))
            {
                found = candidate;
                break;
            }
        }

        if (present && found.isNull())
        {
            QDomElement element = Doc()->createElement(VAbstractPattern::TagGroupItem);
            element.setAttribute(VAbstractPattern::AttrTool, item.second);
            if (item.first != item.second) // `object` is omitted when it equals `tool`
            {
                element.setAttribute(VAbstractPattern::AttrObject, item.first);
            }
            group.appendChild(element);
        }
        else if (not present && not found.isNull())
        {
            group.removeChild(found);
        }
    }
}
