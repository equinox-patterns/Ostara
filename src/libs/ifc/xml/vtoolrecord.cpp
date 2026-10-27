/************************************************************************
 **
 **  @file   vtoolrecord.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 14/10/2026.
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

#include "vtoolrecord.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolRecord constructor.
 * @param id tool id.
 * @param toolType tool type.
 * @param indexPatternBlock pattern peace name.
 */
VToolRecord::VToolRecord(vidtype id, Tool toolType, int indexPatternBlock)
  : id(id),
    toolType(toolType),
    indexPatternBlock(indexPatternBlock)
{}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRecord::IsMandatory() const -> bool
{
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 79, "Check list.");
    return toolType != Tool::NodePoint && toolType != Tool::NodeArc
           && toolType != Tool::NodeElArc && toolType != Tool::NodeSpline && toolType != Tool::NodeSplinePath
           && toolType != Tool::PiecePath;
}
