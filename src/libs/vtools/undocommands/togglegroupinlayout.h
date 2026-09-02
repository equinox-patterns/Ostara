/************************************************************************
 **
 **  @file   togglegroupinlayout.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   02/09/2026
 **
 **  @brief  Undo command that toggles a piece group's in-layout (check/x) state.
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

#ifndef TOGGLEGROUPINLAYOUT_H
#define TOGGLEGROUPINLAYOUT_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "vundocommand.h"

/**
 * @brief The ToggleGroupInLayout class — toggles a piece group's in-layout state.
 *
 * Like TogglePieceInLayout, but for a <pieceGroup> element and the OPieceGroup in the
 * container. A group behaves like one piece: its state governs the export and the
 * "Hide not in layout" canvas masking of the whole group.
 */
class ToggleGroupInLayout : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    ToggleGroupInLayout(quint32 groupId, bool state, VContainer *data, VAbstractPattern *doc,
                        QUndoCommand *parent = nullptr);
    ~ToggleGroupInLayout() override = default;

    void undo() override;
    void redo() override;

signals:
    void Toggled(quint32 groupId);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(ToggleGroupInLayout) // NOLINT
    quint32 m_groupId;
    VContainer *m_data;
    bool m_oldState;
    bool m_newState;

    void Do(bool state);
};

#endif // TOGGLEGROUPINLAYOUT_H
