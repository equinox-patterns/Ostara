/************************************************************************
 **
 **  @file   linkpiecegroup.h
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

#ifndef LINKPIECEGROUP_H
#define LINKPIECEGROUP_H

#include <QDomElement>
#include <QObject>
#include <QtGlobal>

#include "../tools/vtoolseamallowance.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "vundocommand.h"

class LinkPieceGroup : public VUndoCommand
{
    Q_OBJECT // NOLINT

public:
    LinkPieceGroup(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data,
                   QUndoCommand *parent = nullptr);
    ~LinkPieceGroup() override = default;

    void undo() override;
    void redo() override;
    auto id() const -> int override;

signals:
    void UpdateGroups();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(LinkPieceGroup) // NOLINT

    OPieceGroup m_group;
    VContainer *m_data;
    quint32 m_groupId{NULL_ID};
    /** @brief Where each member stood on the canvas before it was sent to the workshop — what undo
     *  has to give back. The stack keeps that very position; the tool leaves it. */
    QHash<quint32, QPointF> m_preLinkPositions;
};

#endif // LINKPIECEGROUP_H
