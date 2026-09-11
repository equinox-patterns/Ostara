/************************************************************************
 **
 **  @file   ofoldlinetypes.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/09/2026
 **
 **  @brief  The fold-line types Ostara offers, shared by the piece and group Properties Browsers
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

#include "ofoldlinetypes.h"

namespace
{
// The two markers Ostara draws, plus "Invisible fold line" — the one that keeps the fold known and
// exploited (mirror, unfolding, placement) and removes only the marker, which no other type can
// express. The Valentina types Ostara dropped left gaps in FoldLineType; see its comment.
const QVector<OFoldLineTypeItem> kItems = {
    {FoldLineType::TwoArrows,          QT_TRANSLATE_NOOP("ODetailsFoldLineWidget", "No text")},
    {FoldLineType::TwoArrowsTextAbove, QT_TRANSLATE_NOOP("ODetailsFoldLineWidget", "With text")},
    {FoldLineType::None,               QT_TRANSLATE_NOOP("ODetailsFoldLineWidget", "Invisible fold line")},
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OFoldLineTypeItems() -> const QVector<OFoldLineTypeItem> &
{
    return kItems;
}
