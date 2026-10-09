/************************************************************************
 **
 **  @file   olayergroupcommands.h
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
#ifndef OLAYERGROUPCOMMANDS_H
#define OLAYERGROUPCOMMANDS_H

#include <QDomElement>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "vundocommand.h"

/// Create a group of layers. Like AddGroup, the element is built by the caller (CreateLayerGroup) and handed over
/// detached: undo takes it out of the document, redo puts the very same element back.
class AddLayerGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    AddLayerGroup(const QDomElement &xml, VAbstractPattern *doc, QUndoCommand *parent = nullptr);
    ~AddLayerGroup() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(AddLayerGroup) // NOLINT

    int m_indexPatternBlock{-1};

    void Do(bool restore);
};

/// Take a group of layers out of the document. The group is expected to hold no layer — a group every layer has
/// left is still a group, and that is exactly the one this command deletes.
class DelLayerGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    DelLayerGroup(VAbstractPattern *doc, quint32 id, QUndoCommand *parent = nullptr);
    ~DelLayerGroup() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(DelLayerGroup) // NOLINT

    int m_indexPatternBlock{-1};
    /// The piece the group belongs to. The groups are per piece, and `CreateLayerGroups()` without a name would
    /// resolve the one being shown when the undo runs — not necessarily the one this command was built on.
    QString m_pieceName;

    void Do(bool restore);
};

/// Rename a group of layers and settle the layers it holds, in one step: the dialog edits both, and one Ctrl+Z has
/// to take both back.
class ChangeLayerGroupOptions : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeLayerGroupOptions(VAbstractPattern *doc,
                            quint32 id,
                            const QString &name,
                            const QVector<quint32> &layers,
                            QUndoCommand *parent = nullptr);
    ~ChangeLayerGroupOptions() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(ChangeLayerGroupOptions) // NOLINT

    QString m_newName;
    QString m_oldName{};
    QVector<quint32> m_newLayers;
    QVector<quint32> m_oldLayers{};
    int m_indexPatternBlock{-1};

    void Do(const QString &name, const QVector<quint32> &layers);
};

/// Fold or unfold a group in the dock. The state is written with the document, so it is undone like any other
/// change — and it is what marks the pattern modified when a reader only folded a group.
class ChangeLayerGroupCollapsed : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeLayerGroupCollapsed(VAbstractPattern *doc, quint32 id, bool collapsed, QUndoCommand *parent = nullptr);
    ~ChangeLayerGroupCollapsed() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(ChangeLayerGroupCollapsed) // NOLINT

    bool m_newCollapsed{false};
    bool m_oldCollapsed{false};

    void Do(bool collapsed);
};

/// Move layers from the group they are in to another one — or out of every group (NULL_ID), which is how a group is
/// emptied. One command for the whole move: the transfer dialog settles a set of layers, not a single one.
class ChangeLayersGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ChangeLayersGroup(VAbstractPattern *doc,
                      const QVector<quint32> &layers,
                      quint32 groupId,
                      QUndoCommand *parent = nullptr);
    ~ChangeLayersGroup() override = default;

    void undo() override;
    void redo() override;

signals:
    void UpdateGroups();

private:
    Q_DISABLE_COPY_MOVE(ChangeLayersGroup) // NOLINT

    QVector<quint32> m_layers;
    quint32 m_groupId{NULL_ID};
    /// The group each layer was in when the command was built — carried per layer, because a set of layers can be
    /// gathered from several groups at once.
    QMap<quint32, quint32> m_oldGroups;
    int m_indexPatternBlock{-1};

    void Do(const QMap<quint32, quint32> &groups);
};

#endif // OLAYERGROUPCOMMANDS_H
