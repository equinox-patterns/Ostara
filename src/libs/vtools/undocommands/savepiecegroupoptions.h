/************************************************************************
 **
 **  @file   savepiecegroupoptions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Undo command for editing a piece group's options.
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

#ifndef SAVEPIECEGROUPOPTIONS_H
#define SAVEPIECEGROUPOPTIONS_H

#include <QDomElement>
#include <QObject>
#include <QtGlobal>

#include "../tools/vtoolseamallowance.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "vundocommand.h"

class SavePieceGroupOptions : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    SavePieceGroupOptions(OPieceGroup oldGroup, OPieceGroup newGroup, VAbstractPattern *doc, VContainer *data,
                          quint32 groupId, QUndoCommand *parent = nullptr);
    ~SavePieceGroupOptions() override = default;

    void undo() override;
    void redo() override;
    auto mergeWith(const QUndoCommand *command) -> bool override;
    auto id() const -> int override;

signals:
    void UpdateGroups();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(SavePieceGroupOptions) // NOLINT

    const OPieceGroup m_oldGroup;
    OPieceGroup m_newGroup;
    VContainer *m_data;
    quint32 m_groupId{NULL_ID};
};

#endif // SAVEPIECEGROUPOPTIONS_H
