/************************************************************************
 **
 **  @file   savegradedrangecontent.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Undo command saving a content change of a size range.
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

#include "savegradedrangecontent.h"

#include "ogradedrangerenames.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vpatterndb/vcontainer.h"
#include "../xml/opattern_graded.h"

#include <QDomElement>
#include <QHash>
#include <QObject>

//---------------------------------------------------------------------------------------------------------------------
SaveGradedRangeContent::SaveGradedRangeContent(VAbstractPattern *doc, VContainer *data,
                                               const OSizerange &oldRange, const OSizerange &newRange,
                                               QUndoCommand *parent)
  : QUndoCommand(parent)
  , m_doc(doc)
  , m_data(data)
  , m_oldRange(oldRange)
  , m_newRange(newRange)
{
    setText(QObject::tr("Edit graded layout"));
}

//---------------------------------------------------------------------------------------------------------------------
void SaveGradedRangeContent::ApplyRange(const OSizerange &range)
{
    // The name this command carries is the one the range had when the command was pushed, and
    // a range is found by its name. It may have been renamed since — resolved through the
    // renames of the session (see OGradedRangeRenames), which is the one thing Qt cannot do
    // for us: the commands on the stack are not reachable, so the names they hold cannot be
    // rewritten in place.
    //
    // ⚠️ And the resolved name is used for the WRITE as well as for the test. Resolving it for
    // the test alone would write the range back under the name it no longer has — resurrecting
    // it beside the renamed one, which is exactly what the guard below exists to prevent.
    const QString name = OGradedRangeRenames::Resolve(range.name);

    // A content command only ever edits an EXISTING range. If the range is no longer present
    // (deliberately dropped/replaced on re-entry), applying here would resurrect it via undo —
    // skip instead.
    if (!m_data->SizeRanges()->contains(name))
    {
        return;
    }

    OSizerange current = range;
    current.name = name;

    QHash<QString, OSizerange> ranges = *m_data->SizeRanges();
    ranges.insert(name, current);
    m_data->SetSizeRanges(ranges);

    if (m_doc != nullptr)
    {
        QDomElement rootElement = m_doc->documentElement();
        AddSizeRanges(m_doc, rootElement, m_data);
        m_doc->SetModified(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void SaveGradedRangeContent::redo()
{
    ApplyRange(m_newRange);
}

//---------------------------------------------------------------------------------------------------------------------
void SaveGradedRangeContent::undo()
{
    ApplyRange(m_oldRange);
}
