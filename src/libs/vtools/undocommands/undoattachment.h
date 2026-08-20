/************************************************************************
 **
 **  @file   undoattachment.h
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
#ifndef UNDOATTACHMENT_H
#define UNDOATTACHMENT_H

#include <QDomElement>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "vundocommand.h"

class AddAttachment : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AddAttachment(const QDomElement &xml, VAbstractPattern *doc, QUndoCommand *parent = nullptr);
    ~AddAttachment() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateAttachments();

private:
    Q_DISABLE_COPY_MOVE(AddAttachment) // NOLINT
    int m_indexPatternBlock{-1};
};

class RenameAttachment : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    RenameAttachment(VAbstractPattern *doc, quint32 id, const QString &name, QUndoCommand *parent = nullptr);
    ~RenameAttachment() override = default;
    void undo() override;
    void redo() override;

signals:
    void UpdateAttachments();

private:
    Q_DISABLE_COPY_MOVE(RenameAttachment) // NOLINT
    QString newName;
    QString oldName{};
};

class AddItemToAttachment : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AddItemToAttachment(const QDomElement &xml, VAbstractPattern *doc, quint32 attachmentId,
                        QUndoCommand *parent = nullptr);
    ~AddItemToAttachment() override = default;
    void undo() override;
    void redo() override;

signals:
    void UpdateAttachments();

protected:
    void performUndoRedo(bool isUndo);

private:
    Q_DISABLE_COPY_MOVE(AddItemToAttachment) // NOLINT
    int m_indexPatternBlock{-1};
};

class RemoveItemFromAttachment : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    RemoveItemFromAttachment(const QDomElement &xml, VAbstractPattern *doc, quint32 attachmentId,
                             QUndoCommand *parent = nullptr);
    ~RemoveItemFromAttachment() override = default;
    void undo() override;
    void redo() override;

signals:
    void UpdateAttachments();

protected:
    void performUndoRedo(bool isUndo);

private:
    Q_DISABLE_COPY_MOVE(RemoveItemFromAttachment) // NOLINT
    int m_indexPatternBlock{-1};
};

class DelAttachment : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    DelAttachment(VAbstractPattern *doc, quint32 id, QUndoCommand *parent = nullptr);
    ~DelAttachment() override = default;
    void undo() override;
    void redo() override;

signals:
    void UpdateAttachments();

private:
    Q_DISABLE_COPY_MOVE(DelAttachment) // NOLINT
    int m_indexPatternBlock{-1};
};

#endif // UNDOATTACHMENT_H
