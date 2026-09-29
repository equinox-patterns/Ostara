/************************************************************************
 **
 **  @file   undoattachment.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/08/2026
 **
 **  @brief  Undo commands for attachment groups (type Illustrator).
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

#include "undoattachment.h"

#include <QDomNode>

#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vmisc/vabstractvalapplication.h"
#include "vundocommand.h"

//RenameAttachment
//---------------------------------------------------------------------------------------------------------------------
RenameAttachment::RenameAttachment(VAbstractPattern *doc, quint32 id, const QString &name, QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    newName(name)
{
    setText(tr("rename attachment"));
    oldName = doc->GetAttachmentName(id);
}

//---------------------------------------------------------------------------------------------------------------------
void RenameAttachment::undo()
{
    qCDebug(vUndo, "Undo.");
    Doc()->SetAttachmentName(ElementId(), oldName);
    emit UpdateAttachments();
    emit Doc()->UpdateToolTip();
}

//---------------------------------------------------------------------------------------------------------------------
void RenameAttachment::redo()
{
    qCDebug(vUndo, "Redo.");
    Doc()->SetAttachmentName(ElementId(), newName);
    emit UpdateAttachments();
    emit Doc()->UpdateToolTip();
}

//AddItemToAttachment
//---------------------------------------------------------------------------------------------------------------------
AddItemToAttachment::AddItemToAttachment(const QDomElement &xml,
                                         VAbstractPattern *doc,
                                         quint32 attachmentId,
                                         QUndoCommand *parent)
  : VUndoCommand(xml, doc, parent),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId())
{
    setText(tr("Add item to attachment"));
    SetElementId(attachmentId);
}

//---------------------------------------------------------------------------------------------------------------------
void AddItemToAttachment::undo()
{
    qCDebug(vUndo, "Undo the add item to attachment");
    performUndoRedo(true);
}

//---------------------------------------------------------------------------------------------------------------------
void AddItemToAttachment::redo()
{
    qCDebug(vUndo, "Redo the add item to attachment");
    performUndoRedo(false);
}

//---------------------------------------------------------------------------------------------------------------------
void AddItemToAttachment::performUndoRedo(bool isUndo)
{
    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock); //Without this user will not see this change

    if (QDomElement attachment = Doc()->FindElementById(ElementId(), VAbstractPattern::TagAttachment);
        attachment.isElement())
    {
        if (isUndo)
        {
            if (attachment.removeChild(GetElement()).isNull())
            {
                qCDebug(vUndo, "Can't delete item.");
                return;
            }
        }
        else
        {
            if (attachment.appendChild(GetElement()).isNull())
            {
                qCDebug(vUndo, "Can't add item.");
                return;
            }
        }

        Doc()->SetModified(true);
        emit VAbstractValApplication::VApp()->getCurrentDocument()->patternChanged(false);

        emit UpdateAttachments();
    }
    else
    {
        qCDebug(vUndo, "Can't get attachment by id = %u.", ElementId());
        return;
    }
}

//RemoveItemFromAttachment
//---------------------------------------------------------------------------------------------------------------------
RemoveItemFromAttachment::RemoveItemFromAttachment(const QDomElement &xml,
                                                   VAbstractPattern *doc,
                                                   quint32 attachmentId,
                                                   QUndoCommand *parent)
  : VUndoCommand(xml, doc, parent),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId())
{
    setText(tr("Remove item from attachment"));
    SetElementId(attachmentId);
}

//---------------------------------------------------------------------------------------------------------------------
void RemoveItemFromAttachment::undo()
{
    qCDebug(vUndo, "Undo the remove item from attachment");
    performUndoRedo(true);
}

//---------------------------------------------------------------------------------------------------------------------
void RemoveItemFromAttachment::redo()
{
    qCDebug(vUndo, "Redo the remove item from attachment");
    performUndoRedo(false);
}

//---------------------------------------------------------------------------------------------------------------------
void RemoveItemFromAttachment::performUndoRedo(bool isUndo)
{
    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock); //Without this user will not see this change

    if (QDomElement attachment = Doc()->FindElementById(ElementId(), VAbstractPattern::TagAttachment);
        attachment.isElement())
    {
        if (isUndo)
        {
            if (attachment.appendChild(GetElement()).isNull())
            {
                qCDebug(vUndo, "Can't add the item.");
                return;
            }
        }
        else
        {
            if (attachment.removeChild(GetElement()).isNull())
            {
                qCDebug(vUndo, "Can't delete item.");
                return;
            }
        }

        Doc()->SetModified(true);
        emit VAbstractValApplication::VApp()->getCurrentDocument()->patternChanged(false);

        emit UpdateAttachments();
    }
    else
    {
        qCDebug(vUndo, "Can't get attachment by id = %u.", ElementId());
        return;
    }
}

//DelAttachment
//---------------------------------------------------------------------------------------------------------------------
DelAttachment::DelAttachment(VAbstractPattern *doc, quint32 id, QUndoCommand *parent)
  : VUndoCommand(doc, id, parent),
    m_indexPatternBlock(doc->PatternBlockMapper()->GetActiveId())
{
    setText(tr("delete attachment"));
    SetElement(doc->CloneNodeById(id));
}

//---------------------------------------------------------------------------------------------------------------------
void DelAttachment::undo()
{
    qCDebug(vUndo, "Undo.");

    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock); //Without this user will not see this change

    if (QDomElement attachments = Doc()->CreateAttachments(); not attachments.isNull())
    {
        attachments.appendChild(GetElement());
        emit UpdateAttachments();
    }
    else
    {
        qCDebug(vUndo, "Can't get tag Attachments.");
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DelAttachment::redo()
{
    qCDebug(vUndo, "Redo.");

    Doc()->PatternBlockMapper()->SetActiveById(m_indexPatternBlock); //Without this user will not see this change

    if (QDomElement attachments = Doc()->CreateAttachments(); not attachments.isNull())
    {
        if (QDomElement attachment = Doc()->FindElementById(ElementId(), VAbstractPattern::TagAttachment);
            attachment.isElement())
        {
            if (attachments.removeChild(attachment).isNull())
            {
                qCDebug(vUndo, "Can't delete attachment.");
                return;
            }

            if (attachments.childNodes().isEmpty())
            {
                QDomNode parent = attachments.parentNode();
                parent.removeChild(attachments);
            }

            emit UpdateAttachments();
        }
        else
        {
            qCDebug(vUndo, "Can't get attachment by id = %u.", ElementId());
            return;
        }
    }
    else
    {
        qCDebug(vUndo, "Can't get tag Attachments.");
        return;
    }
}
