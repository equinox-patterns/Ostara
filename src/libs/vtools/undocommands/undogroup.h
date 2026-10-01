/************************************************************************
 **
 **  @file   undogroup.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   18 3, 2020
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2020 Valentina project
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
#ifndef UNDOGROUP_H
#define UNDOGROUP_H

#include <QDomElement>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "vundocommand.h"

class AddGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AddGroup(const QDomElement &xml, VAbstractPattern *doc, QUndoCommand *parent = nullptr);
    ~AddGroup() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateGroups();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(AddGroup) // NOLINT
    int m_indexPatternBlock{-1};
};

class RenameGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    RenameGroup(VAbstractPattern *doc, quint32 id, const QString &name, QUndoCommand *parent = nullptr);
    ~RenameGroup() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(RenameGroup) // NOLINT
    QString newName;
    QString oldName{};
};

class ChangeGroupOptions : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeGroupOptions(VAbstractPattern *doc, quint32 id, const QString &name, const QStringList &tags,
                       const QString &color, const QString &penStyle, QUndoCommand *parent = nullptr);
    ~ChangeGroupOptions() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(ChangeGroupOptions) // NOLINT
    QString newName;
    QString oldName{};
    QStringList newTags;
    QStringList oldTags{};
    QString newColor;
    QString oldColor{};
    QString newPenStyle;
    QString oldPenStyle{};
};

/// Give a layer a tool, or take the tool away from it — the link that makes a layer follow the operation which
/// created it (its dialog reopens on it, and editing the dialog edits it).
///
/// Taken away rather than the layer deleted when the tool goes: the layer the operation created holds the objects
/// the operation was built from, objects that existed before the tool and outlive it. Deleting them with the tool
/// would leave them in no layer at all, the very state the layers exist to prevent.
class ChangeGroupTool : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeGroupTool(VAbstractPattern *doc, vidtype id, vidtype tool, QUndoCommand *parent = nullptr);
    ~ChangeGroupTool() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(ChangeGroupTool) // NOLINT

    vidtype m_newTool{null_id};
    vidtype m_oldTool{null_id};
    int m_indexPatternBlock{-1};

    void Do(vidtype tool);
};

class AddItemToGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AddItemToGroup(const QDomElement &xml, VAbstractPattern *doc, quint32 groupId, QUndoCommand *parent = nullptr);
    ~AddItemToGroup() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

protected:
    void performUndoRedo(bool isUndo);

private:
    Q_DISABLE_COPY_MOVE(AddItemToGroup) // NOLINT
    int m_indexPatternBlock{-1};
};

class RemoveItemFromGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    RemoveItemFromGroup(const QDomElement &xml, VAbstractPattern *doc, quint32 groupId, QUndoCommand *parent = nullptr);
    ~RemoveItemFromGroup() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

protected:
    void performUndoRedo(bool isUndo);

private:
    Q_DISABLE_COPY_MOVE(RemoveItemFromGroup) // NOLINT
    int m_indexPatternBlock{-1};
};

class ChangeGroupVisibility : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeGroupVisibility(VAbstractPattern *doc, vidtype id, bool visible, QUndoCommand *parent = nullptr);
    ~ChangeGroupVisibility() override = default;
    void undo() override;
    void redo() override;

signals:
    void UpdateGroup(vidtype id, bool visible);

private:
    Q_DISABLE_COPY_MOVE(ChangeGroupVisibility) // NOLINT

    bool m_oldVisibility{true};
    bool m_newVisibility{true};
    int m_indexPatternBlock{-1};

    void Do(bool visible);
};

class ChangeMultipleGroupsVisibility : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeMultipleGroupsVisibility(VAbstractPattern *doc, const QVector<vidtype> &groups, bool visible,
                                   QUndoCommand *parent = nullptr);
    ~ChangeMultipleGroupsVisibility() override = default;
    void undo() override;
    void redo() override;

signals:
    void UpdateMultipleGroups(const QMap<vidtype, bool> &groups);

private:
    Q_DISABLE_COPY_MOVE(ChangeMultipleGroupsVisibility) // NOLINT

    QVector<vidtype> m_groups;
    bool m_newVisibility{true};
    QMap<vidtype, bool> m_oldVisibility{};
    int m_indexPatternBlock{-1};
};

class DelGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    DelGroup(VAbstractPattern *doc, quint32 id, QUndoCommand *parent = nullptr);
    ~DelGroup() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(DelGroup) // NOLINT
    int m_indexPatternBlock{-1};
    /// The piece the layers belong to: the layers are per piece, and `CreateGroups()` without a name would resolve
    /// the one being shown at the moment the undo runs — not necessarily the one this command was built on.
    QString m_pieceName;
    /// The layer the active flag goes to when the deleted one carried it: the automatic layer when the piece has
    /// one, the first of the piece otherwise (the rule the loading pass uses). NULL_ID when the deleted layer was
    /// not the active one, or was the last of its piece.
    vidtype m_newActiveId{NULL_ID};
    /// Whether that layer was visible before this command showed it by activating it.
    bool m_newActiveWasVisible{true};
};

/// Move the "active layer" flag — the layer the canvas draws into — from one layer to another.
class ChangeActiveGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeActiveGroup(VAbstractPattern *doc, quint32 newId, quint32 oldId, QUndoCommand *parent = nullptr);
    ~ChangeActiveGroup() override = default;
    void undo() override;
    void redo() override;
signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(ChangeActiveGroup) // NOLINT
    quint32 m_newId{NULL_ID};
    quint32 m_oldId{NULL_ID};
    /// Activating a layer shows it. Undoing has to put it back the way the user had it.
    bool m_newWasVisible{true};
    /// The piece the change belongs to. It is what the undo has to name: the layers are per piece, the undo stack
    /// survives a piece switch, and `SetActiveGroup` falls back on the piece being SHOWN — so an undo replayed
    /// after switching pieces used to look for a layer of the other one.
    QString m_pieceName;
};

#endif // UNDOGROUP_H
