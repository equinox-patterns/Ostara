/************************************************************************
 **
 **  @file   vpattern_commonattrs.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Common attribute extraction helpers extracted from vpattern.cpp
 **
 **  @note   Extracted from vpattern.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vpattern.h"

#include "../core/vapplication.h"
#include "../vmisc/def.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollinepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolsinglepoint.h"
#include "../vtools/tools/nodeDetails/vabstractnode.h"

//---------------------------------------------------------------------------------------------------------------------
void VPattern::PointsWithLineCommonAttributes(const QDomElement &domElement, VToolLinePointInitData &initData)
{
    PointsCommonAttributes(domElement, initData);
    initData.typeLine = GetParametrString(domElement, AttrTypeLine, TypeLineLine);
    initData.lineColor = GetParametrString(domElement, AttrLineColor, ColorBlack);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::PointsCommonAttributes(const QDomElement &domElement, VToolSinglePointInitData &initData)
{
    DrawPointsCommonAttributes(domElement, initData.id, initData.mx, initData.my, initData.notes);
    initData.name = GetParametrString(domElement, AttrName, QChar('A'));
    initData.showLabel = GetParametrBool(domElement, AttrShowLabel, trueStr);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::PointsCommonAttributes(const QDomElement &domElement, quint32 &id, qreal &mx, qreal &my)
{
    ToolsCommonAttributes(domElement, id);
    const VAbstractValApplication *app = VAbstractValApplication::VApp();
    mx = app->toPixel(GetParametrDouble(domElement, AttrMx, QString::number(app->fromPixel(labelMX))));
    my = app->toPixel(GetParametrDouble(domElement, AttrMy, QString::number(app->fromPixel(labelMY))));
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::DrawPointsCommonAttributes(const QDomElement &domElement, quint32 &id, qreal &mx, qreal &my,
                                          QString &notes)
{
    DrawToolsCommonAttributes(domElement, id, notes);
    const VAbstractValApplication *app = VAbstractValApplication::VApp();
    mx = app->toPixel(GetParametrDouble(domElement, AttrMx, QString::number(app->fromPixel(labelMX))));
    my = app->toPixel(GetParametrDouble(domElement, AttrMy, QString::number(app->fromPixel(labelMY))));
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SplinesCommonAttributes(const QDomElement &domElement, quint32 &id, quint32 &idObject, quint32 &idTool)
{
    ToolsCommonAttributes(domElement, id);
    idObject = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
    idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);
}
