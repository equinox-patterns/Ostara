/************************************************************************
 **
 **  @file   ogradedsizeranges.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/09/2026
 **
 **  @brief  The (measurements table, declared grading axis) context that decides
 **          which of a pattern's stored size ranges are usable.
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

#include "ogradedsizeranges.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/osizerange.h"
#include "../widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"

namespace OGradedSizeRanges
{

//---------------------------------------------------------------------------------------------------------------------
auto ContextFor(const QString &canonicalPath) -> Context
{
    Context context;

    if (canonicalPath.isEmpty())
    {
        return context;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const bool inLibrary = MeasurementLibraryScanner::IsInLibrary(
        settings->GetPathMeasurementLibrary(), canonicalPath);

    context.canonical = canonicalPath;
    context.inLibrary = inLibrary;
    // A table outside the library never persists its axis — the user re-picks it each
    // time. Leaving the axis empty is what lifts the constraint rather than tightening it.
    context.axis = inLibrary ? settings->GetSizeAxisForMeasurement(canonicalPath) : QString();
    return context;
}

//---------------------------------------------------------------------------------------------------------------------
auto CurrentContext(VAbstractPattern *doc) -> Context
{
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        return {};
    }
    return ContextFor(
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath()));
}

//---------------------------------------------------------------------------------------------------------------------
auto MatchesContext(const QString &canonical, const QString &axis, const Context &context) -> bool
{
    if (!context.canonical.isEmpty() && canonical != context.canonical)
    {
        return false;
    }
    if (!context.axis.isEmpty() && axis != context.axis)
    {
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto Matches(const OSizerange &range, const Context &context) -> bool
{
    return MatchesContext(range.measurement, range.sizeAxis, context);
}

}  // namespace OGradedSizeRanges
