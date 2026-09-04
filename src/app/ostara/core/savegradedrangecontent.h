/************************************************************************
 **
 **  @file   savegradedrangecontent.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Undo command saving a content change of a size range (offsets /
 **          overrides). Rewrites the range in the container and the <sizeRange>
 **          DOM element under <sizeRanges>.
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

#ifndef SAVEGRADEDRANGECONTENT_H
#define SAVEGRADEDRANGECONTENT_H

#include <QUndoCommand>

#include "../vpatterndb/osizerange.h"

class VAbstractPattern;
class VContainer;

/// Saves a content change of one size range (definition unchanged).
class SaveGradedRangeContent : public QUndoCommand
{
public:
    SaveGradedRangeContent(VAbstractPattern *doc, VContainer *data, const OSizerange &oldRange,
                           const OSizerange &newRange, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    void ApplyRange(const OSizerange &range);

    VAbstractPattern *m_doc;
    VContainer *m_data;
    OSizerange m_oldRange;
    OSizerange m_newRange;
};

#endif // SAVEGRADEDRANGECONTENT_H
