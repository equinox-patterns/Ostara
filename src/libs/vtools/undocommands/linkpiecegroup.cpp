/************************************************************************
 **
 **  @file   linkpiecegroup.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Undo command that links pieces into a piece group.
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

#include "linkpiecegroup.h"

#include <QDomElement>

#include "../ifc/ifcdef.h"
#include "../vmisc/vabstractvalapplication.h"

LinkPieceGroup::LinkPieceGroup(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data,
                               QUndoCommand *parent)
  : VUndoCommand(doc, parent), m_group(group), m_data(data)
{
    setText(tr("link pieces"));
}

//---------------------------------------------------------------------------------------------------------------------
void LinkPieceGroup::undo()
{
    if (m_groupId == NULL_ID)
    {
        return;
    }

    m_data->RemovePieceGroup(m_groupId);
    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);

    // Restore the members' original position (Mx/My) and their own children: redo baked the
    // superimposed position into Mx/My, so move each member back by its stored offset.
    for (quint32 pid : m_group.pieceIds)
    {
        try
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
            if (tool != nullptr)
            {
                const VPiece piece = m_data->GetPiece(pid);
                const QPointF offset = m_group.memberOffsets.value(pid, QPointF());
                const QPointF original(piece.GetMx() - offset.x(), piece.GetMy() - offset.y());
                QDomElement domElement = Doc()->FindElementById(pid, VAbstractPattern::TagDetail);
                if (domElement.isElement())
                {
                    Doc()->SetAttribute(domElement, AttrMx,
                                        QString().setNum(VAbstractValApplication::VApp()->fromPixel(original.x())));
                    Doc()->SetAttribute(domElement, AttrMy,
                                        QString().setNum(VAbstractValApplication::VApp()->fromPixel(original.y())));
                }
                tool->Move(original.x(), original.y());
                tool->RefreshGroupedChildren();
            }
        }
        catch (const VExceptionBadId &)
        {
            continue;
        }
    }
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
void LinkPieceGroup::redo()
{
    // Assign a stable id on the first redo so undo/redo keep the same group id.
    if (m_groupId == NULL_ID)
    {
        m_groupId = m_data->getNextId();
        m_data->UpdateId(m_groupId);
        m_group.id = m_groupId;
        SetElementId(m_groupId);
    }

    m_data->AddPieceGroup(m_group);

    // Superimpose the common point by translating each member whose node is not already at the
    // reference position (Val Fork vwidgetdetails.cpp:1234-1238). The superimposed position is
    // BAKED into Mx/My (DOM + container + scene), like a piece: the file stores the members'
    // actual position, so reload needs no alignment and a later group move never leaves a stale
    // offset behind. memberOffsets keeps the delta for undo.
    for (quint32 pid : m_group.pieceIds)
    {
        const QPointF offset = m_group.memberOffsets.value(pid, QPointF());
        if (qFuzzyIsNull(offset.x()) && qFuzzyIsNull(offset.y()))
        {
            continue;
        }
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool == nullptr)
        {
            continue;
        }
        const QPointF superimposed = tool->pos() + offset;
        QDomElement domElement = Doc()->FindElementById(pid, VAbstractPattern::TagDetail);
        if (domElement.isElement())
        {
            Doc()->SetAttribute(domElement, AttrMx,
                                QString().setNum(VAbstractValApplication::VApp()->fromPixel(superimposed.x())));
            Doc()->SetAttribute(domElement, AttrMy,
                                QString().setNum(VAbstractValApplication::VApp()->fromPixel(superimposed.y())));
        }
        tool->Move(superimposed.x(), superimposed.y());
    }

    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto LinkPieceGroup::id() const -> int
{
    return static_cast<int>(UndoCommand::LinkPieceGroup);
}
