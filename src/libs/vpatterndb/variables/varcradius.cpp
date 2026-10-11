/************************************************************************
 **
 **  @file   varcradius.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30 5, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
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

#include "varcradius.h"

#include <QMessageLogger>
#include <QString>

#include "../ifc/ifcdef.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vellipticalarc.h"

//---------------------------------------------------------------------------------------------------------------------
VArcRadius::VArcRadius()
{
    SetType(VarType::ArcRadius);
}

//---------------------------------------------------------------------------------------------------------------------
VArcRadius::VArcRadius(const quint32 &id, const quint32 &parentId, const VArc *arc, Unit patternUnit)
  : VCurveVariable(id, parentId)
{
    // cppcheck-suppress unknownMacro
    SCASSERT(arc != nullptr)

    SetType(VarType::ArcRadius);
    SetName(radius_V + arc->name());

    if (not arc->GetAlias().isEmpty())
    {
        SetAlias(radius_V + arc->GetAlias());
    }
    StoreValue(FromPixel(arc->GetRadius(), patternUnit));
}

//---------------------------------------------------------------------------------------------------------------------
VArcRadius::VArcRadius(const quint32 &id, const quint32 &parentId, const VEllipticalArc *elArc, const int numberRadius,
                       Unit patternUnit)
  : VCurveVariable(id, parentId)
{
    SCASSERT(elArc != nullptr)

    SetType(VarType::ArcRadius);
    SetName(radius_V + QStringLiteral("%1%2").arg(numberRadius).arg(elArc->name()));

    if (not elArc->GetAlias().isEmpty())
    {
        // The number belongs in the alias as it does in the name: the two radii of one ellipse are two
        // distinct values, and without it both answered to the very same name — so a formula could not tell
        // which one it meant, and the assistant listed the two of them as one. The alias now mirrors the name,
        // `Radius1ElArc_<suffix>` and `Radius2ElArc_<suffix>`.
        //
        // The unnumbered form those two used to share keeps resolving, as a route registered by
        // VContainer::AddCurve: a formula written before this change must still find a value.
        SetAlias(radius_V + QStringLiteral("%1%2").arg(numberRadius).arg(elArc->GetAlias()));
    }

    if (numberRadius == 1)
    {
        StoreValue(FromPixel(elArc->GetRadius1(), patternUnit));
    }
    else
    {
        StoreValue(FromPixel(elArc->GetRadius2(), patternUnit));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VArcRadius::operator=(const VArcRadius &var) -> VArcRadius &
{
    if (&var == this)
    {
        return *this;
    }
    VCurveVariable::operator=(var);
    return *this;
}
