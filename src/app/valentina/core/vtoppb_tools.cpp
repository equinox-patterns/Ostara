/************************************************************************
 **
 **  @file   vtoppb_tools.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Tool-specific property browser methods extracted from vtooloptionspropertybrowser.cpp
 **
 **  @note   Extracted from vtooloptionspropertybrowser.cpp by Sophie Denys <contact@sophiedenys.com>
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

#include "vtooloptionspropertybrowser.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vformula.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vwidgets/vcontrolpointspline.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsimplepoint.h"
#include "ifcdef.h"
#include "tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooltruedarts.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolalongline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolbisector.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolendline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolheight.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollineintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolnormal.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolshoulderpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoollineintersect.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromarcandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromcircleandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofcontact.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersection.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectionarcs.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncircles.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtooltriangle.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "vtoppb_templates.h"
#include "../vmisc/def.h"
#include "../vpropertyexplorer/plugins/vlinetypeproperty.h"
#include "../vwidgets/vcontrolpointspline.h"


void VToolOptionsPropertyBrowser::ChangeDataToolSinglePoint(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetPosition = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolBasePoint *>(m_currentItem))
        {
            QVariant const value = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole);
            i->SetBasePointPos(value.toPointF());
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolBasePoint>(property);
            break;
        case 1: // QLatin1String("position")
            SetPosition(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolBasePoint>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolEndLine(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolEndLine>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolEndLine>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolEndLine>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolEndLine>(property);
            break;
        case 5: // AttrAngle
            SetFormulaAngle<VToolEndLine>(property);
            break;
        case 2: // AttrBasePoint (read only)
            break;
        case 61: // AttrNotes
            SetNotes<VToolEndLine>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolAlongLine(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolAlongLine>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolAlongLine>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolAlongLine>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolAlongLine>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolAlongLine>(property);
            break;
        case 2: // AttrBasePoint (read only)
        case 7: // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolArc(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 8: // AttrRadius
            SetFormulaRadius<VToolArc>(property);
            break;
        case 9: // AttrAngle1
            SetFormulaF1<VToolArc>(property);
            break;
        case 10: // AttrAngle2
            SetFormulaF2<VToolArc>(property);
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolArc>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolArc>(property);
            break;
        case 60: // AttrAScale
            SetApproximationScale<VToolArc>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolArc>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolArc>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolArcWithLength(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 8: // AttrRadius
            SetFormulaRadius<VToolArcWithLength>(property);
            break;
        case 9: // AttrAngle1
            SetFormulaF1<VToolArcWithLength>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolArcWithLength>(property);
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolArcWithLength>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolArcWithLength>(property);
            break;
        case 60: // AttrAScale
            SetApproximationScale<VToolArcWithLength>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolArcWithLength>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolArcWithLength>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolBisector(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolBisector>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolBisector>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolBisector>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolBisector>(property);
            break;
        case 6:  // AttrFirstPoint (read only)
        case 2:  // AttrBasePoint (read only)
        case 12: // AttrThirdPoint (read only)
            break;
        case 61: // AttrNotes
            SetNotes<VToolBisector>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolTrueDarts(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 32: // AttrName1
            SetPointName1<VToolTrueDarts>(property);
            break;
        case 33: // AttrName2
            SetPointName2<VToolTrueDarts>(property);
            break;
        case 6:  // AttrFirstPoint (read only)
        case 7:  // AttrSecondPoint (read only)
        case 43: // AttrDartP1 (read only)
        case 44: // AttrDartP2 (read only)
        case 45: // AttrDartP3 (read only)
            break;
        case 61: // AttrNotes
            SetNotes<VToolTrueDarts>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolCutArc(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolCutArc>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolCutArc>(property);
            break;
        case 13: // AttrArc (read only)
            break;
        case 61: // AttrNotes
            SetNotes<VToolCutArc>(property);
            break;
        case 63: // AttrAlias1
            SetAlias1<VToolCutArc>(property);
            break;
        case 64: // AttrAlias2
            SetAlias2<VToolCutArc>(property);
            break;
        case 69: // AttrCurveName1
            SetCurveName1<VToolCutArc>(property);
            break;
        case 70: // AttrCurveName2
            SetCurveName2<VToolCutArc>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolCutSpline(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolCutSpline>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolCutSpline>(property);
            break;
        case 46: // AttrCurve (read only)
            break;
        case 61: // AttrNotes
            SetNotes<VToolCutSpline>(property);
            break;
        case 63: // AttrAlias1
            SetAlias1<VToolCutSpline>(property);
            break;
        case 64: // AttrAlias2
            SetAlias2<VToolCutSpline>(property);
            break;
        case 69: // AttrCurveName1
            SetCurveName1<VToolCutSpline>(property);
            break;
        case 70: // AttrCurveName2
            SetCurveName2<VToolCutSpline>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolCutSplinePath(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolCutSplinePath>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolCutSplinePath>(property);
            break;
        case 46: // AttrCurve (read only)
            break;
        case 61: // AttrNotes
            SetNotes<VToolCutSplinePath>(property);
            break;
        case 63: // AttrAlias1
            SetAlias1<VToolCutSplinePath>(property);
            break;
        case 64: // AttrAlias2
            SetAlias2<VToolCutSplinePath>(property);
            break;
        case 69: // AttrCurveName1
            SetCurveName1<VToolCutSplinePath>(property);
            break;
        case 70: // AttrCurveName2
            SetCurveName2<VToolCutSplinePath>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolHeight(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolHeight>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolHeight>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolHeight>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolHeight>(property);
            break;
        case 2:  // AttrBasePoint (read only)
        case 16: // AttrP1Line (read only)
        case 17: // AttrP2Line (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolLine(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 3: // AttrTypeLine
            SetLineType<VToolLine>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolLine>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolLine>(property);
            break;
        case 6: // AttrFirstPoint (read only)
        case 7: // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolLineIntersect(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolLineIntersect>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolLineIntersect>(property);
            break;
        case 18: // AttrP1Line1 (read only)
        case 19: // AttrP2Line1 (read only)
        case 20: // AttrP1Line2 (read only)
        case 21: // AttrP2Line2 (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolNormal(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetAngle = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolNormal *>(m_currentItem))
        {
            double const value = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
            if (VFuzzyComparePossibleNulls(value, i->GetAngle()))
            {
                return;
            }

            i->SetAngle(value);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 4: // AttrLength
            SetFormulaLength<VToolNormal>(property);
            break;
        case 0: // AttrName
            SetPointName<VToolNormal>(property);
            break;
        case 5: // AttrAngle
            SetAngle(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolNormal>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolNormal>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolNormal>(property);
            break;
        case 2: // AttrBasePoint (read only)
        case 7: // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointOfContact(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetArcRadius = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolPointOfContact *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->getArcRadius())
            {
                return;
            }

            i->setArcRadius(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 8: // AttrRadius
            SetArcRadius(property);
            break;
        case 0: // AttrName
            SetPointName<VToolPointOfContact>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointOfContact>(property);
            break;
        case 11: // AttrCenter (read only)
        case 6:  // AttrFirstPoint (read only)
        case 7:  // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointOfIntersection(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolPointOfIntersection>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointOfIntersection>(property);
            break;
        case 6: // AttrFirstPoint (read only)
        case 7: // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointOfIntersectionArcs(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolPointOfIntersectionArcs>(property);
            break;
        case 28: // AttrCrossPoint
            SetCrossCirclesPoint<VToolPointOfIntersectionArcs>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointOfIntersectionArcs>(property);
            break;
        case 47: // AttrFirstArc (read only)
        case 48: // AttrSecondArc (read only)
            break;
        case 71: // AttrCurve1Name1
            SetArc1Name1<VToolPointOfIntersectionArcs>(property);
            break;
        case 72: // AttrCurve1Name2
            SetArc1Name2<VToolPointOfIntersectionArcs>(property);
            break;
        case 73: // AttrCurve2Name1
            SetArc2Name1<VToolPointOfIntersectionArcs>(property);
            break;
        case 74: // AttrCurve2Name2
            SetArc2Name2<VToolPointOfIntersectionArcs>(property);
            break;
        case 75: // AttrCurve1Alias1
            SetArc1Alias1<VToolPointOfIntersectionArcs>(property);
            break;
        case 76: // AttrCurve1Alias2
            SetArc1Alias2<VToolPointOfIntersectionArcs>(property);
            break;
        case 77: // AttrCurve2Alias1
            SetArc2Alias1<VToolPointOfIntersectionArcs>(property);
            break;
        case 78: // AttrCurve2Alias2
            SetArc2Alias2<VToolPointOfIntersectionArcs>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointOfIntersectionCircles(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetFirstCircleRadius = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetFirstCircleRadius())
            {
                return;
            }

            i->SetFirstCircleRadius(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    auto SetSecondCircleRadius = [this](VPE::VProperty *property) -> void
    {
        if (auto *i = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetSecondCircleRadius())
            {
                return;
            }

            i->SetSecondCircleRadius(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };
    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolPointOfIntersectionCircles>(property);
            break;
        case 28: // AttrCrossPoint
            SetCrossCirclesPoint<VToolPointOfIntersectionCircles>(property);
            break;
        case 29: // AttrC1Radius
            SetFirstCircleRadius(property);
            break;
        case 30: // AttrC2Radius
            SetSecondCircleRadius(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointOfIntersectionCircles>(property);
            break;
        case 49: // AttrC1Center (read only)
        case 50: // AttrC2Center (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointOfIntersectionCurves(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolPointOfIntersectionCurves>(property);
            break;
        case 34: // AttrVCrossPoint
            SetVCrossCurvesPoint<VToolPointOfIntersectionCurves>(property);
            break;
        case 35: // AttrHCrossPoint
            SetHCrossCurvesPoint<VToolPointOfIntersectionCurves>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointOfIntersectionCurves>(property);
            break;
        case 51: // AttrCurve1 (read only)
        case 52: // AttrCurve2 (read only)
            break;
        case 71: // AttrCurve1Name1
            SetCurve1Name1<VToolPointOfIntersectionCurves>(property);
            break;
        case 72: // AttrCurve1Name2
            SetCurve1Name2<VToolPointOfIntersectionCurves>(property);
            break;
        case 73: // AttrCurve2Name1
            SetCurve2Name1<VToolPointOfIntersectionCurves>(property);
            break;
        case 74: // AttrCurve2Name2
            SetCurve2Name2<VToolPointOfIntersectionCurves>(property);
            break;
        case 75: // AttrCurve1Alias1
            SetCurve1Alias1<VToolPointOfIntersectionCurves>(property);
            break;
        case 76: // AttrCurve1Alias2
            SetCurve1Alias2<VToolPointOfIntersectionCurves>(property);
            break;
        case 77: // AttrCurve2Alias1
            SetCurve2Alias1<VToolPointOfIntersectionCurves>(property);
            break;
        case 78: // AttrCurve2Alias2
            SetCurve2Alias2<VToolPointOfIntersectionCurves>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointFromCircleAndTangent(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetCircleRadius = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolPointFromCircleAndTangent *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetCircleRadius())
            {
                return;
            }

            i->SetCircleRadius(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolPointFromCircleAndTangent>(property);
            break;
        case 31: // AttrCRadius
            SetCircleRadius(property);
            break;
        case 28: // AttrCrossPoint
            SetCrossCirclesPoint<VToolPointFromCircleAndTangent>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointFromCircleAndTangent>(property);
            break;
        case 53: // AttrCCenter (read only)
        case 54: // AttrTangent (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolPointFromArcAndTangent(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolPointFromArcAndTangent>(property);
            break;
        case 28: // AttrCrossPoint
            SetCrossCirclesPoint<VToolPointFromArcAndTangent>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolPointFromArcAndTangent>(property);
            break;
        case 54: // AttrTangent (read only)
        case 13: // AttrArc (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolShoulderPoint(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 4: // AttrLength
            SetFormulaLength<VToolShoulderPoint>(property);
            break;
        case 0: // AttrName
            SetPointName<VToolShoulderPoint>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolShoulderPoint>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolShoulderPoint>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolShoulderPoint>(property);
            break;
        case 6:  // AttrFirstPoint (read only)
        case 2:  // AttrBasePoint (read only)
        case 12: // AttrThirdPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolSpline(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    QVariant const value = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole);
    const QString id = m_propertyToId[property];

    auto *i = qgraphicsitem_cast<VToolSpline *>(m_currentItem);
    SCASSERT(i != nullptr)

    VSpline spl = i->getSpline();
    const auto f = value.value<VFormula>();

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 9: // AttrAngle1
            if (not f.error())
            {
                spl.SetStartAngle(f.getDoubleValue(), f.GetFormula(FormulaType::FromUser));
                i->setSpline(spl);
            }
            break;
        case 10: // AttrAngle2
            if (not f.error())
            {
                spl.SetEndAngle(f.getDoubleValue(), f.GetFormula(FormulaType::FromUser));
                i->setSpline(spl);
            }
            break;
        case 36: // AttrLength1
            if (not f.error() && f.getDoubleValue() >= 0)
            {
                spl.SetC1Length(VAbstractValApplication::VApp()->toPixel(f.getDoubleValue()),
                                f.GetFormula(FormulaType::FromUser));
                i->setSpline(spl);
            }
            break;
        case 37: // AttrLength2
            if (not f.error() && f.getDoubleValue() >= 0)
            {
                spl.SetC2Length(VAbstractValApplication::VApp()->toPixel(f.getDoubleValue()),
                                f.GetFormula(FormulaType::FromUser));
                i->setSpline(spl);
            }
            break;
        case 27: // AttrColor
            SetLineColor<VToolSpline>(property);
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolSpline>(property);
            break;
        case 60: // AttrAScale
            spl.SetApproximationScale(value.toDouble());
            i->setSpline(spl);
            break;
        case 61: // AttrNotes
            SetNotes<VToolSpline>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolSpline>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolCubicBezier(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolCubicBezier>(property);
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolCubicBezier>(property);
            break;
        case 60: // AttrAScale
        {
            auto *i = qgraphicsitem_cast<VToolCubicBezier *>(m_currentItem);
            SCASSERT(i != nullptr)

            const QVariant value = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole);
            VCubicBezier spl = i->getSpline();
            spl.SetApproximationScale(value.toDouble());
            i->setSpline(spl);
            break;
        }
        case 61: // AttrNotes
            SetNotes<VToolCubicBezier>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolCubicBezier>(property);
            break;
        case 55: // AttrPoint1 (read only)
        case 56: // AttrPoint2 (read only)
        case 57: // AttrPoint3 (read only)
        case 58: // AttrPoint4 (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolSplinePath(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolSplinePath>(property);
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolSplinePath>(property);
            break;
        case 60: // AttrAScale
        {
            auto *i = qgraphicsitem_cast<VToolSplinePath *>(m_currentItem);
            SCASSERT(i != nullptr)

            QVariant const value = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole);
            VSplinePath spl = i->getSplinePath();
            spl.SetApproximationScale(value.toDouble());
            i->setSplinePath(spl);
            break;
        }
        case 61: // AttrNotes
            SetNotes<VToolSplinePath>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolSplinePath>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolCubicBezierPath(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolCubicBezierPath>(property);
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolCubicBezierPath>(property);
            break;
        case 60: // AttrAScale
        {
            auto *i = qgraphicsitem_cast<VToolCubicBezierPath *>(m_currentItem);
            SCASSERT(i != nullptr)

            QVariant const value = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole);
            VCubicBezierPath spl = i->getSplinePath();
            spl.SetApproximationScale(value.toDouble());
            i->setSplinePath(spl);
            break;
        }
        case 61: // AttrNotes
            SetNotes<VToolCubicBezierPath>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolCubicBezierPath>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolTriangle(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolTriangle>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolTriangle>(property);
            break;
        case 23: // AttrAxisP1 (read only)
        case 24: // AttrAxisP2 (read only)
        case 6:  // AttrFirstPoint (read only)
        case 7:  // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolLineIntersectAxis(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolLineIntersectAxis>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolLineIntersectAxis>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolLineIntersectAxis>(property);
            break;
        case 5: // AttrAngle
            SetFormulaAngle<VToolLineIntersectAxis>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolLineIntersectAxis>(property);
            break;
        case 2: // AttrBasePoint (read only)
        case 6: // AttrFirstPoint (read only)
        case 7: // AttrSecondPoint (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolCurveIntersectAxis(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetPointName<VToolCurveIntersectAxis>(property);
            break;
        case 3: // AttrTypeLine
            SetLineType<VToolCurveIntersectAxis>(property);
            break;
        case 26: // AttrTypeLineColor
            SetLineColor<VToolCurveIntersectAxis>(property);
            break;
        case 5: // AttrAngle
            SetFormulaAngle<VToolCurveIntersectAxis>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolCurveIntersectAxis>(property);
            break;
        case 2:  // AttrBasePoint (read only)
        case 46: // AttrCurve (read only)
            break;
        case 69: // AttrCurveName1
            SetCurveName1<VToolCurveIntersectAxis>(property);
            break;
        case 70: // AttrCurveName2
            SetCurveName2<VToolCurveIntersectAxis>(property);
            break;
        case 63: // AttrAlias1
            SetAlias1<VToolCurveIntersectAxis>(property);
            break;
        case 64: // AttrAlias2
            SetAlias2<VToolCurveIntersectAxis>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolRotation(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 5: // AttrAngle
            SetFormulaAngle<VToolRotation>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolRotation>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolMove(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 5: // AttrAngle
            SetFormulaAngle<VToolMove>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolMove>(property);
            break;
        case 42: // AttrRotationAngle
            SetFormulaRotationAngle<VToolMove>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolMove>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolFlippingByLine(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 61: // AttrNotes
            SetNotes<VToolFlippingByLine>(property);
            break;
        case 6: // AttrFirstPoint
        case 7: // AttrSecondPoint
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolFlippingByAxis(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 39: // AttrAxisType
            SetAxisType<VToolFlippingByAxis>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolFlippingByAxis>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolEllipticalArc(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetFormulaRadius1 = [this](const VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolEllipticalArc *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetFormulaRadius1())
            {
                return;
            }

            i->SetFormulaRadius1(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    auto SetFormulaRadius2 = [this](const VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolEllipticalArc *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetFormulaRadius2())
            {
                return;
            }

            i->SetFormulaRadius2(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 40: // AttrRadius1
            SetFormulaRadius1(property);
            break;
        case 41: // AttrRadius2
            SetFormulaRadius2(property);
            break;
        case 9: // AttrAngle1
            SetFormulaF1<VToolEllipticalArc>(property);
            break;
        case 10: // AttrAngle2
            SetFormulaF2<VToolEllipticalArc>(property);
            break;
        case 42: // AttrRotationAngle
            SetFormulaRotationAngle<VToolEllipticalArc>(property);
            break;
        case 27: // AttrColor
            SetLineColor<VToolEllipticalArc>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolEllipticalArc>(property);
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolEllipticalArc>(property);
            break;
        case 60: // AttrAScale
            SetApproximationScale<VToolEllipticalArc>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolEllipticalArc>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataBackgroundPixmapItem(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId.value(property);

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetName<VBackgroundPixmapItem>(property);
            break;
        case 65: // AttrHold
            SetHold<VBackgroundPixmapItem>(property);
            break;
        case 66: // AttrVisible
            SetVisible<VBackgroundPixmapItem>(property);
            break;
        case 67: // AttrOpacity
            SetOpacity<VBackgroundPixmapItem>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataBackgroundSVGItem(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId.value(property);

    switch (PropertiesList().indexOf(id))
    {
        case 0: // AttrName
            SetName<VBackgroundSVGItem>(property);
            break;
        case 65: // AttrHold
            SetHold<VBackgroundSVGItem>(property);
            break;
        case 66: // AttrVisible
            SetVisible<VBackgroundSVGItem>(property);
            break;
        case 67: // AttrOpacity
            SetOpacity<VBackgroundPixmapItem>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolEllipticalArcWithLength(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetFormulaRadius1 = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetFormulaRadius1())
            {
                return;
            }

            i->SetFormulaRadius1(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    auto SetFormulaRadius2 = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetFormulaRadius2())
            {
                return;
            }

            i->SetFormulaRadius2(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 40: // AttrRadius1
            SetFormulaRadius1(property);
            break;
        case 41: // AttrRadius2
            SetFormulaRadius2(property);
            break;
        case 9: // AttrAngle1
            SetFormulaF1<VToolEllipticalArcWithLength>(property);
            break;
        case 4: // AttrLength
            SetFormulaLength<VToolEllipticalArcWithLength>(property);
            break;
        case 42: // AttrRotationAngle
            SetFormulaRotationAngle<VToolEllipticalArcWithLength>(property);
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolEllipticalArcWithLength>(property);
            break;
        case 11: // AttrCenter (read only)
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolEllipticalArcWithLength>(property);
            break;
        case 60: // AttrAScale
            SetApproximationScale<VToolEllipticalArcWithLength>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolEllipticalArcWithLength>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolEllipticalArcWithLength>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolParallelCurve(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto SetFormulaWidth = [this](VPE::VProperty *property)
    {
        if (auto *i = qgraphicsitem_cast<VToolParallelCurve *>(m_currentItem))
        {
            auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
            if (formula == i->GetFormulaWidth())
            {
                return;
            }

            i->SetFormulaWidth(formula);
        }
        else
        {
            qWarning() << "Can't cast item";
        }
    };

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 25: // AttrWidth
            SetFormulaWidth(property);
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolParallelCurve>(property);
            break;
        case 46: // AttrCurve (read only)
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolParallelCurve>(property);
            break;
        case 60: // AttrAScale
            SetApproximationScale<VToolParallelCurve>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolParallelCurve>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolParallelCurve>(property);
            break;
        case 68: // AttrSubName
            SetSubName<VToolParallelCurve>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ChangeDataToolGraduatedCurve(VPE::VProperty *property)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    switch (PropertiesList().indexOf(id))
    {
        case 0:              // AttrName
            Q_UNREACHABLE(); // The attribute is read only
            break;
        case 27: // AttrTypeColor
            SetLineColor<VToolGraduatedCurve>(property);
            break;
        case 46: // AttrCurve (read only)
            break;
        case 59: // AttrPenStyle
            SetPenStyle<VToolGraduatedCurve>(property);
            break;
        case 60: // AttrAScale
            SetApproximationScale<VToolGraduatedCurve>(property);
            break;
        case 61: // AttrNotes
            SetNotes<VToolGraduatedCurve>(property);
            break;
        case 62: // AttrAlias
            SetAlias<VToolGraduatedCurve>(property);
            break;
        case 68: // AttrSubName
            SetSubName<VToolGraduatedCurve>(property);
            break;
        default:
            qWarning() << "Unknown property type. id = " << id;
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolSinglePoint(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolBasePoint *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Base point"));

    AddPropertyObjectName(i, tr("Point label:"));

    auto *itemPosition = new VPE::VPointFProperty(tr("Position:"));
    itemPosition->setValue(i->GetBasePointPos());
    AddProperty(itemPosition, QStringLiteral("position"));

    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolEndLine(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolEndLine *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point at distance and angle"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("Base point:"), AttrBasePoint);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyFormula(tr("Angle:"), i->GetFormulaAngle(), AttrAngle);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolAlongLine(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolAlongLine *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point at distance along line"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("First point:"), AttrBasePoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("Second point:"), AttrSecondPoint);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolArc(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolArc *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Arc"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyParentPointName(i->CenterPointName(), tr("Center point:"), AttrCenter);
    AddPropertyFormula(tr("Radius:"), i->GetFormulaRadius(), AttrRadius);
    AddPropertyFormula(tr("First angle:"), i->GetFormulaF1(), AttrAngle1);
    AddPropertyFormula(tr("Second angle:"), i->GetFormulaF2(), AttrAngle2);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(
        i, tr("Pen style:"),
        CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolArcWithLength(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolArcWithLength *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Arc with given length"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyParentPointName(i->CenterPointName(), tr("Center point:"), AttrCenter);
    AddPropertyFormula(tr("Radius:"), i->GetFormulaRadius(), AttrRadius);
    AddPropertyFormula(tr("First angle:"), i->GetFormulaF1(), AttrAngle1);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(
        i, tr("Pen style:"),
        CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolBisector(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolBisector *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point along bisector"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->FirstPointName(), tr("First point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->BasePointName(), tr("Second point:"), AttrBasePoint);
    AddPropertyParentPointName(i->ThirdPointName(), tr("Third point:"), AttrThirdPoint);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolTrueDarts(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolTrueDarts *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("True darts"));

    AddPropertyPointName1(i, tr("Point 1 label:"));
    AddPropertyPointName2(i, tr("Point 2 label:"));
    AddPropertyParentPointName(i->BaseLineP1Name(), tr("First base point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->BaseLineP2Name(), tr("Second base point:"), AttrSecondPoint);
    AddPropertyParentPointName(i->DartP1Name(), tr("First dart point:"), AttrDartP1);
    AddPropertyParentPointName(i->DartP2Name(), tr("Second dart point:"), AttrDartP2);
    AddPropertyParentPointName(i->DartP3Name(), tr("Third dart point:"), AttrDartP3);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolCutArc(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolCutArc *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Cut arc tool"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->CurveName(), tr("Arc:"), AttrArc);
    AddPropertyCurveName1(i, tr("Name1:"));
    AddPropertyCurveName2(i, tr("Name2:"));
    AddPropertyAlias1(i, tr("Alias1:"));
    AddPropertyAlias2(i, tr("Alias2:"));
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolCutSpline(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolCutSpline *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool for segmenting a curve"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->CurveName(), tr("Curve:"), AttrCurve);
    AddPropertyCurveName1(i, tr("Name1:"));
    AddPropertyCurveName2(i, tr("Name2:"));
    AddPropertyAlias1(i, tr("Alias1:"));
    AddPropertyAlias2(i, tr("Alias2:"));
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolCutSplinePath(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolCutSplinePath *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool segment a pathed curve"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->CurveName(), tr("Curve:"), AttrCurve);
    AddPropertyCurveName1(i, tr("Name1:"));
    AddPropertyCurveName2(i, tr("Name2:"));
    AddPropertyAlias1(i, tr("Alias1:"));
    AddPropertyAlias2(i, tr("Alias2:"));
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolHeight(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolHeight *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Perpendicular point along line"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("Base point:"), AttrBasePoint);
    AddPropertyParentPointName(i->FirstLinePointName(), tr("First line point:"), AttrP1Line);
    AddPropertyParentPointName(i->SecondLinePointName(), tr("Second line point:"), AttrP2Line);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolLine(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolLine *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Line between points"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyParentPointName(i->FirstPointName(), tr("First point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("Second point:"), AttrSecondPoint);
    QMap<QString, QIcon> styles =
        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text));
    styles.remove(TypeLineNone);
    AddPropertyLineType(i, tr("Line type:"), styles);
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolLineIntersect(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolLineIntersect *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point at line intersection"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->Line1P1Name(), tr("First line (first point):"), AttrP1Line1);
    AddPropertyParentPointName(i->Line1P2Name(), tr("First line (second point):"), AttrP2Line1);
    AddPropertyParentPointName(i->Line2P1Name(), tr("Second line (first point):"), AttrP1Line2);
    AddPropertyParentPointName(i->Line2P2Name(), tr("Second line (second point):"), AttrP2Line2);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolNormal(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolNormal *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point along perpendicular"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("First point:"), AttrBasePoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("Second point:"), AttrSecondPoint);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);

    auto *itemAngle = new VPE::VDoubleProperty(tr("Additional angle degrees:"));
    itemAngle->setValue(i->GetAngle());
    itemAngle->setSetting(QStringLiteral("Min"), -360);
    itemAngle->setSetting(QStringLiteral("Max"), 360);
    itemAngle->setSetting(QStringLiteral("Precision"), 3);
    AddProperty(itemAngle, AttrAngle);

    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointOfContact(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointOfContact *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point of intersection circle and segment"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->ArcCenterPointName(), tr("Center of arc:"), AttrCenter);
    AddPropertyParentPointName(i->FirstPointName(), tr("Top of the line:"), AttrFirstPoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("End of the line:"), AttrSecondPoint);
    AddPropertyFormula(tr("Radius:"), i->getArcRadius(), AttrRadius);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointOfIntersection(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersection *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool to make point from x & y of two other points"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->FirstPointName(), tr("X: vertical point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("Y: horizontal point:"), AttrSecondPoint);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointOfIntersectionArcs(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersectionArcs *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool to make point from intersection two arcs"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->FirstArcName(), tr("First arc:"), AttrFirstArc);
    AddPropertyParentPointName(i->SecondArcName(), tr("Second arc:"), AttrSecondArc);
    AddPropertyCrossPoint(i, tr("Take:"));
    AddPropertyArc1Name1(i, tr("First arc. Left sub arc name:"));
    AddPropertyArc1Name2(i, tr("First arc. Right sub arc name:"));
    AddPropertyArc1Alias1(i, tr("First arc. Left sub arc alias:"));
    AddPropertyArc1Alias2(i, tr("First arc. Right sub arc alias:"));
    AddPropertyArc2Name1(i, tr("Second arc. Left sub arc name:"));
    AddPropertyArc2Name2(i, tr("Second arc. Right sub arc name:"));
    AddPropertyArc2Alias1(i, tr("Second arc. Left sub arc alias:"));
    AddPropertyArc2Alias2(i, tr("Second arc. Right sub arc alias:"));
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointOfIntersectionCircles(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool to make point from intersection two circles"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->FirstCircleCenterPointName(), tr("Center of the first circle:"), AttrC1Center);
    AddPropertyParentPointName(i->SecondCircleCenterPointName(), tr("Center of the second circle:"), AttrC2Center);
    AddPropertyFormula(tr("First circle radius:"), i->GetFirstCircleRadius(), AttrC1Radius);
    AddPropertyFormula(tr("Second circle radius:"), i->GetSecondCircleRadius(), AttrC2Radius);
    AddPropertyCrossPoint(i, tr("Take:"));
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointOfIntersectionCurves(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersectionCurves *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool to make point from intersection two curves"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->FirstCurveName(), tr("First curve:"), AttrCurve1);
    AddPropertyParentPointName(i->SecondCurveName(), tr("Second curve:"), AttrCurve2);
    AddPropertyVCrossPoint(i, tr("Vertical correction:"));
    AddPropertyHCrossPoint(i, tr("Horizontal correction:"));
    AddPropertyCurve1Name1(i, tr("First curve. Left sub curve name:"));
    AddPropertyCurve1Name2(i, tr("First curve. Right sub curve name:"));
    AddPropertyCurve1Alias1(i, tr("First curve. Left sub curve alias:"));
    AddPropertyCurve1Alias2(i, tr("First curve. Right sub curve alias:"));
    AddPropertyCurve2Name1(i, tr("Second curve. Left sub curve name:"));
    AddPropertyCurve2Name2(i, tr("Second curve. Right sub curve name:"));
    AddPropertyCurve2Alias1(i, tr("Second curve. Left sub curve alias:"));
    AddPropertyCurve2Alias2(i, tr("Second curve. Right sub curve alias:"));
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointFromCircleAndTangent(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointFromCircleAndTangent *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool to make point from circle and tangent"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->CircleCenterPointName(), tr("Center of the circle:"), AttrCCenter);
    AddPropertyParentPointName(i->TangentPointName(), tr("Tangent point:"), AttrTangent);
    AddPropertyFormula(tr("Circle radius:"), i->GetCircleRadius(), AttrCRadius);
    AddPropertyCrossPoint(i, tr("Take:"));
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolPointFromArcAndTangent(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolPointFromArcAndTangent *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool to make point from arc and tangent"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->TangentPointName(), tr("Tangent point:"), AttrTangent);
    AddPropertyParentPointName(i->ArcName(), tr("Arc:"), AttrArc);
    AddPropertyCrossPoint(i, tr("Take:"));
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolShoulderPoint(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolShoulderPoint *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Special point on shoulder"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("First point:"), AttrBasePoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("Second point:"), AttrSecondPoint);
    AddPropertyParentPointName(i->ShoulderPointName(), tr("Third point:"), AttrThirdPoint);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolSpline(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolSpline *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Curve tool"));

    const auto spl = i->getSpline();

    AddPropertyObjectName(i, tr("Name:"), true);

    VFormula angle1(spl.GetStartAngleFormula(), i->getData());
    angle1.setToolId(i->getId());
    angle1.setPostfix(degreeSymbol);
    angle1.Eval();
    AddPropertyFormula(tr("C1: angle:"), angle1, AttrAngle1);

    VFormula length1(spl.GetC1LengthFormula(), i->getData());
    length1.setToolId(i->getId());
    length1.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    length1.Eval();
    AddPropertyFormula(tr("C1: length:"), length1, AttrLength1);

    VFormula angle2(spl.GetEndAngleFormula(), i->getData());
    angle2.setToolId(i->getId());
    angle2.setPostfix(degreeSymbol);
    angle2.Eval();
    AddPropertyFormula(tr("C2: angle:"), angle2, AttrAngle2);

    VFormula length2(spl.GetC2LengthFormula(), i->getData());
    length2.setToolId(i->getId());
    length2.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    length2.Eval();
    AddPropertyFormula(tr("C2: length:"), length2, AttrLength2);

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(
        i, tr("Pen style:"),
        CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), spl.GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolCubicBezier(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolCubicBezier *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Cubic bezier curve"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyParentPointName(i->FirstPointName(), tr("First point:"), AttrPoint1);
    AddPropertyParentPointName(i->SecondPointName(), tr("Second point:"), AttrPoint2);
    AddPropertyParentPointName(i->ThirdPointName(), tr("Third point:"), AttrPoint3);
    AddPropertyParentPointName(i->ForthPointName(), tr("Fourth point:"), AttrPoint4);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(
        i, tr("Pen style:"),
        CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->getSpline().GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolSplinePath(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolSplinePath *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool for path curve"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(
        i, tr("Pen style:"),
        CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->getSplinePath().GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolCubicBezierPath(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolCubicBezierPath *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool cubic bezier curve"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(
        i, tr("Pen style:"),
        CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->getSplinePath().GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolTriangle(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolTriangle *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool triangle"));

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->AxisP1Name(), tr("First point of axis:"), AttrAxisP1);
    AddPropertyParentPointName(i->AxisP2Name(), tr("Second point of axis:"), AttrAxisP2);
    AddPropertyParentPointName(i->FirstPointName(), tr("First point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->SecondPointName(), tr("Second point:"), AttrSecondPoint);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolLineIntersectAxis(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolLineIntersectAxis *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point intersection line and axis"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("Axis point:"), AttrBasePoint);
    AddPropertyParentPointName(i->FirstLinePoint(), tr("First line point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->SecondLinePoint(), tr("Second line point:"), AttrSecondPoint);
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyFormula(tr("Angle:"), i->GetFormulaAngle(), AttrAngle);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolCurveIntersectAxis(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolCurveIntersectAxis *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Point intersection curve and axis"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Point label:"));
    AddPropertyParentPointName(i->BasePointName(), tr("Axis point:"), AttrBasePoint);
    AddPropertyParentPointName(i->CurveName(), tr("Curve:"), AttrCurve);
    AddPropertyCurveName1(i, tr("Name1:"));
    AddPropertyCurveName2(i, tr("Name2:"));
    AddPropertyAlias1(i, tr("Alias1:"));
    AddPropertyAlias2(i, tr("Alias2:"));
    AddPropertyLineType(i, tr("Line type:"),
                        LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Line color:"), VAbstractTool::ColorsList(), AttrLineColor);
    AddPropertyFormula(tr("Angle:"), i->GetFormulaAngle(), AttrAngle);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolRotation(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolRotation *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool rotation"));

    AddPropertyParentPointName(i->OriginPointName(), tr("Origin point:"), AttrCenter);
    AddPropertyFormula(tr("Angle:"), i->GetFormulaAngle(), AttrAngle);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolMove(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolMove *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool move"));

    AddPropertyFormula(tr("Angle:"), i->GetFormulaAngle(), AttrAngle);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyFormula(tr("Rotation angle:"), i->GetFormulaRotationAngle(), AttrRotationAngle);
    AddPropertyParentPointName(i->OriginPointName(), tr("Rotation origin point:"), AttrCenter);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolFlippingByLine(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolFlippingByLine *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool flipping by line"));

    AddPropertyParentPointName(i->FirstLinePointName(), tr("First line point:"), AttrFirstPoint);
    AddPropertyParentPointName(i->SecondLinePointName(), tr("Second line point:"), AttrSecondPoint);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolFlippingByAxis(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolFlippingByAxis *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Tool flipping by axis"));

    AddPropertyAxisType(i, tr("Axis type:"));
    AddPropertyParentPointName(i->OriginPointName(), tr("Origin point:"), AttrCenter);
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolEllipticalArc(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolEllipticalArc *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Elliptical arc"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyParentPointName(i->CenterPointName(), tr("Center point:"), AttrCenter);
    AddPropertyFormula(tr("Radius:"), i->GetFormulaRadius1(), AttrRadius1);
    AddPropertyFormula(tr("Radius:"), i->GetFormulaRadius2(), AttrRadius2);
    AddPropertyFormula(tr("First angle:"), i->GetFormulaF1(), AttrAngle1);
    AddPropertyFormula(tr("Second angle:"), i->GetFormulaF2(), AttrAngle2);
    AddPropertyFormula(tr("Rotation angle:"), i->GetFormulaRotationAngle(), AttrRotationAngle);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(i,
                             tr("Pen style:"),
                             CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsBackgroundPixmapItem(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VBackgroundPixmapItem *>(item);
    m_formView->setTitle(tr("Background image"));

    AddPropertyObjectName(i, tr("Name:"), false);
    AddPropertyBool(tr("Hold:"), i->IsHold(), VtoppbConstants::AttrHold);
    AddPropertyBool(tr("Visible:"), i->IsVisible(), VtoppbConstants::AttrVisible);
    AddPropertyOpacity(tr("Opacity:"), static_cast<int>(i->GetOpacity() * 100));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsBackgroundSVGItem(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VBackgroundSVGItem *>(item);
    m_formView->setTitle(tr("Background image"));

    AddPropertyObjectName(i, tr("Name:"), false);
    AddPropertyBool(tr("Hold:"), i->IsHold(), VtoppbConstants::AttrHold);
    AddPropertyBool(tr("Visible:"), i->IsVisible(), VtoppbConstants::AttrVisible);
    AddPropertyOpacity(tr("Opacity:"), static_cast<int>(i->GetOpacity() * 100));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolEllipticalArcWithLength(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Elliptical arc with given length"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Name:"), true);
    AddPropertyParentPointName(i->CenterPointName(), tr("Center point:"), AttrCenter);
    AddPropertyFormula(tr("Radius1:"), i->GetFormulaRadius1(), AttrRadius1);
    AddPropertyFormula(tr("Radius2:"), i->GetFormulaRadius2(), AttrRadius2);
    AddPropertyFormula(tr("First angle:"), i->GetFormulaF1(), AttrAngle1);
    AddPropertyFormula(tr("Length:"), i->GetFormulaLength(), AttrLength);
    AddPropertyFormula(tr("Rotation angle:"), i->GetFormulaRotationAngle(), AttrRotationAngle);
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(i,
                             tr("Pen style:"),
                             CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolParallelCurve(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolParallelCurve *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Parallel curve"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Full name:"), true);
    AddPropertyParentPointName(i->CurveName(), tr("Curve:"), AttrCurve);
    AddPropertyFormula(tr("Width:"), i->GetFormulaWidth(), AttrWidth);
    AddPropertySubName(i, tr("Name:"));
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(i,
                             tr("Pen style:"),
                             CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowOptionsToolGraduatedCurve(QGraphicsItem *item)
{
    auto *i = qgraphicsitem_cast<VToolGraduatedCurve *>(item);
    i->ShowVisualization(true);
    m_formView->setTitle(tr("Graduated curve"));

    QPalette const comboBoxPalette = ComboBoxPalette();

    AddPropertyObjectName(i, tr("Full name:"), true);
    AddPropertyParentPointName(i->CurveName(), tr("Curve:"), AttrCurve);
    AddPropertySubName(i, tr("Name:"));
    AddPropertyAlias(i, tr("Alias:"));
    AddPropertyCurvePenStyle(i,
                             tr("Pen style:"),
                             CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                comboBoxPalette.color(QPalette::Text)));
    AddPropertyLineColor(i, tr("Color:"), VAbstractTool::ColorsList(), AttrColor);
    AddPropertyApproximationScale(tr("Approximation scale:"), i->GetApproximationScale());
    AddPropertyText(tr("Notes:"), i->GetNotes(), AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolSinglePoint()
{
    auto *i = qgraphicsitem_cast<VToolBasePoint *>(m_currentItem);
    m_idToProperty[AttrName]->setValue(i->name());
    m_idToProperty[QStringLiteral("position")]->setValue(i->GetBasePointPos());
    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolEndLine()
{
    auto *i = qgraphicsitem_cast<VToolEndLine *>(m_currentItem);
    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    QVariant valueAngle;
    valueAngle.setValue(i->GetFormulaAngle());
    m_idToProperty[AttrAngle]->setValue(valueAngle);

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolAlongLine()
{
    auto *i = qgraphicsitem_cast<VToolAlongLine *>(m_currentItem);
    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolArc()
{
    auto *i = qgraphicsitem_cast<VToolArc *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueRadius;
    valueRadius.setValue(i->GetFormulaRadius());
    m_idToProperty[AttrRadius]->setValue(valueRadius);

    QVariant valueFirstAngle;
    valueFirstAngle.setValue(i->GetFormulaF1());
    m_idToProperty[AttrAngle1]->setValue(valueFirstAngle);

    QVariant valueSecondAngle;
    valueSecondAngle.setValue(i->GetFormulaF2());
    m_idToProperty[AttrAngle2]->setValue(valueSecondAngle);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueCenterPoint;
    valueCenterPoint.setValue(i->CenterPointName());
    m_idToProperty[AttrCenter]->setValue(valueCenterPoint);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolArcWithLength()
{
    auto *i = qgraphicsitem_cast<VToolArcWithLength *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueRadius;
    valueRadius.setValue(i->GetFormulaRadius());
    m_idToProperty[AttrRadius]->setValue(valueRadius);

    QVariant valueFirstAngle;
    valueFirstAngle.setValue(i->GetFormulaF1());
    m_idToProperty[AttrAngle1]->setValue(valueFirstAngle);

    QVariant valueLength;
    valueLength.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueLength);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueCenterPoint;
    valueCenterPoint.setValue(i->CenterPointName());
    m_idToProperty[AttrCenter]->setValue(valueCenterPoint);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolBisector()
{
    auto *i = qgraphicsitem_cast<VToolBisector *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstPointName());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueThirdPoint;
    valueThirdPoint.setValue(i->ThirdPointName());
    m_idToProperty[AttrThirdPoint]->setValue(valueThirdPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolTrueDarts()
{
    auto *i = qgraphicsitem_cast<VToolTrueDarts *>(m_currentItem);

    m_idToProperty[AttrName1]->setValue(i->nameP1());
    m_idToProperty[AttrName2]->setValue(i->nameP2());

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->BaseLineP1Name());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->BaseLineP2Name());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    QVariant valueDrartFirstPoint;
    valueDrartFirstPoint.setValue(i->DartP1Name());
    m_idToProperty[AttrDartP1]->setValue(valueDrartFirstPoint);

    QVariant valueDartSecondPoint;
    valueDartSecondPoint.setValue(i->DartP2Name());
    m_idToProperty[AttrDartP2]->setValue(valueDartSecondPoint);

    QVariant valueDartThirdPoint;
    valueDartThirdPoint.setValue(i->DartP3Name());
    m_idToProperty[AttrDartP3]->setValue(valueDartThirdPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolCutArc()
{
    auto *i = qgraphicsitem_cast<VToolCutArc *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    QVariant valueArc;
    valueArc.setValue(i->CurveName());
    m_idToProperty[AttrArc]->setValue(valueArc);

    m_idToProperty[AttrCurveName1]->setValue(i->GetName1());
    m_idToProperty[AttrCurveName2]->setValue(i->GetName2());

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias1]->setValue(i->GetAliasSuffix1());
    m_idToProperty[AttrAlias2]->setValue(i->GetAliasSuffix2());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolCutSpline()
{
    auto *i = qgraphicsitem_cast<VToolCutSpline *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    QVariant valueCurve;
    valueCurve.setValue(i->CurveName());
    m_idToProperty[AttrCurve]->setValue(valueCurve);

    m_idToProperty[AttrCurveName1]->setValue(i->GetName1());
    m_idToProperty[AttrCurveName2]->setValue(i->GetName2());

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias1]->setValue(i->GetAliasSuffix1());
    m_idToProperty[AttrAlias2]->setValue(i->GetAliasSuffix2());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolCutSplinePath()
{
    auto *i = qgraphicsitem_cast<VToolCutSplinePath *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    QVariant valueCurve;
    valueCurve.setValue(i->CurveName());
    m_idToProperty[AttrCurve]->setValue(valueCurve);

    m_idToProperty[AttrCurveName1]->setValue(i->GetName1());
    m_idToProperty[AttrCurveName2]->setValue(i->GetName2());

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias1]->setValue(i->GetAliasSuffix1());
    m_idToProperty[AttrAlias2]->setValue(i->GetAliasSuffix2());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolHeight()
{
    auto *i = qgraphicsitem_cast<VToolHeight *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueFirstLinePoint;
    valueFirstLinePoint.setValue(i->FirstLinePointName());
    m_idToProperty[AttrP1Line]->setValue(valueFirstLinePoint);

    QVariant valueSecondLinePoint;
    valueSecondLinePoint.setValue(i->SecondLinePointName());
    m_idToProperty[AttrP2Line]->setValue(valueSecondLinePoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolLine()
{
    auto *i = qgraphicsitem_cast<VToolLine *>(m_currentItem);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstPointName());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolLineIntersect()
{
    auto *i = qgraphicsitem_cast<VToolLineIntersect *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueLine1P1;
    valueLine1P1.setValue(i->Line1P1Name());
    m_idToProperty[AttrP1Line1]->setValue(valueLine1P1);

    QVariant valueLine1P2;
    valueLine1P2.setValue(i->Line1P2Name());
    m_idToProperty[AttrP2Line1]->setValue(valueLine1P2);

    QVariant valueLine2P1;
    valueLine2P1.setValue(i->Line2P1Name());
    m_idToProperty[AttrP1Line2]->setValue(valueLine2P1);

    QVariant valueLine2P2;
    valueLine2P2.setValue(i->Line2P2Name());
    m_idToProperty[AttrP2Line2]->setValue(valueLine2P2);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolNormal()
{
    auto *i = qgraphicsitem_cast<VToolNormal *>(m_currentItem);

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    m_idToProperty[AttrName]->setValue(i->name());

    m_idToProperty[AttrAngle]->setValue(i->GetAngle());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointOfContact()
{
    auto *i = qgraphicsitem_cast<VToolPointOfContact *>(m_currentItem);

    QVariant valueFormula;
    valueFormula.setValue(i->getArcRadius());
    m_idToProperty[AttrRadius]->setValue(valueFormula);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueCenterPoint;
    valueCenterPoint.setValue(i->ArcCenterPointName());
    m_idToProperty[AttrCenter]->setValue(valueCenterPoint);

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstPointName());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointOfIntersection()
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersection *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstPointName());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointOfIntersectionArcs()
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersectionArcs *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());
    m_idToProperty[AttrCrossPoint]->setValue(static_cast<int>(i->GetCrossCirclesPoint()) - 1);

    QVariant valueFirstArc;
    valueFirstArc.setValue(i->FirstArcName());
    m_idToProperty[AttrFirstArc]->setValue(valueFirstArc);

    QVariant valueSecondArc;
    valueSecondArc.setValue(i->SecondArcName());
    m_idToProperty[AttrSecondArc]->setValue(valueSecondArc);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrCurve1Name1]->setValue(i->GetArc1Name1());
    m_idToProperty[AttrCurve1Name2]->setValue(i->GetArc1Name2());
    m_idToProperty[AttrCurve2Name1]->setValue(i->GetArc2Name1());
    m_idToProperty[AttrCurve2Name2]->setValue(i->GetArc2Name2());

    m_idToProperty[AttrCurve1Alias1]->setValue(i->GetArc1AliasSuffix1());
    m_idToProperty[AttrCurve1Alias2]->setValue(i->GetArc1AliasSuffix2());
    m_idToProperty[AttrCurve2Alias1]->setValue(i->GetArc2AliasSuffix1());
    m_idToProperty[AttrCurve2Alias2]->setValue(i->GetArc2AliasSuffix2());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointOfIntersectionCircles()
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());
    m_idToProperty[AttrCrossPoint]->setValue(static_cast<int>(i->GetCrossCirclesPoint()) - 1);

    QVariant c1Radius;
    c1Radius.setValue(i->GetFirstCircleRadius());
    m_idToProperty[AttrC1Radius]->setValue(c1Radius);

    QVariant c2Radius;
    c2Radius.setValue(i->GetSecondCircleRadius());
    m_idToProperty[AttrC2Radius]->setValue(c2Radius);

    QVariant valueC1Center;
    valueC1Center.setValue(i->FirstCircleCenterPointName());
    m_idToProperty[AttrC1Center]->setValue(valueC1Center);

    QVariant valueC2Center;
    valueC2Center.setValue(i->SecondCircleCenterPointName());
    m_idToProperty[AttrC2Center]->setValue(valueC2Center);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointOfIntersectionCurves()
{
    auto *i = qgraphicsitem_cast<VToolPointOfIntersectionCurves *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());
    m_idToProperty[AttrVCrossPoint]->setValue(static_cast<int>(i->GetVCrossPoint()) - 1);
    m_idToProperty[AttrHCrossPoint]->setValue(static_cast<int>(i->GetHCrossPoint()) - 1);

    QVariant valueCurve1;
    valueCurve1.setValue(i->FirstCurveName());
    m_idToProperty[AttrCurve1]->setValue(valueCurve1);

    QVariant valueCurve2;
    valueCurve2.setValue(i->SecondCurveName());
    m_idToProperty[AttrCurve2]->setValue(valueCurve2);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrCurve1Name1]->setValue(i->GetCurve1Name1());
    m_idToProperty[AttrCurve1Name2]->setValue(i->GetCurve1Name2());
    m_idToProperty[AttrCurve2Name1]->setValue(i->GetCurve2Name1());
    m_idToProperty[AttrCurve2Name2]->setValue(i->GetCurve2Name2());

    m_idToProperty[AttrCurve1Alias1]->setValue(i->GetCurve1AliasSuffix1());
    m_idToProperty[AttrCurve1Alias2]->setValue(i->GetCurve1AliasSuffix2());
    m_idToProperty[AttrCurve2Alias1]->setValue(i->GetCurve2AliasSuffix1());
    m_idToProperty[AttrCurve2Alias2]->setValue(i->GetCurve2AliasSuffix2());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointFromCircleAndTangent()
{
    auto *i = qgraphicsitem_cast<VToolPointFromCircleAndTangent *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());
    m_idToProperty[AttrCrossPoint]->setValue(static_cast<int>(i->GetCrossCirclesPoint()) - 1);

    QVariant cRadius;
    cRadius.setValue(i->GetCircleRadius());
    m_idToProperty[AttrCRadius]->setValue(cRadius);

    QVariant valueCenterPoint;
    valueCenterPoint.setValue(i->CircleCenterPointName());
    m_idToProperty[AttrCCenter]->setValue(valueCenterPoint);

    QVariant valueTangentPoint;
    valueTangentPoint.setValue(i->TangentPointName());
    m_idToProperty[AttrTangent]->setValue(valueTangentPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolPointFromArcAndTangent()
{
    auto *i = qgraphicsitem_cast<VToolPointFromArcAndTangent *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());
    m_idToProperty[AttrCrossPoint]->setValue(static_cast<int>(i->GetCrossCirclesPoint()) - 1);

    QVariant valueTangentPoint;
    valueTangentPoint.setValue(i->TangentPointName());
    m_idToProperty[AttrTangent]->setValue(valueTangentPoint);

    QVariant valueArc;
    valueArc.setValue(i->ArcName());
    m_idToProperty[AttrArc]->setValue(valueArc);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolShoulderPoint()
{
    auto *i = qgraphicsitem_cast<VToolShoulderPoint *>(m_currentItem);

    QVariant valueFormula;
    valueFormula.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueFormula);

    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    QVariant valueThirdPoint;
    valueThirdPoint.setValue(i->ShoulderPointName());
    m_idToProperty[AttrThirdPoint]->setValue(valueThirdPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolSpline()
{
    auto *i = qgraphicsitem_cast<VToolSpline *>(m_currentItem);
    const VSpline spl = i->getSpline();

    m_idToProperty[AttrName]->setValue(i->name());

    VFormula angle1F(spl.GetStartAngleFormula(), i->getData());
    angle1F.setToolId(i->getId());
    angle1F.setPostfix(degreeSymbol);
    angle1F.Eval();
    QVariant angle1;
    angle1.setValue(angle1F);
    m_idToProperty[AttrAngle1]->setValue(angle1);

    VFormula length1F(spl.GetC1LengthFormula(), i->getData());
    length1F.setToolId(i->getId());
    length1F.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    length1F.Eval();
    QVariant length1;
    length1.setValue(length1F);
    m_idToProperty[AttrLength1]->setValue(length1);

    VFormula angle2F(spl.GetEndAngleFormula(), i->getData());
    angle2F.setToolId(i->getId());
    angle2F.setPostfix(degreeSymbol);
    angle2F.Eval();
    QVariant angle2;
    angle2.setValue(angle2F);
    m_idToProperty[AttrAngle2]->setValue(angle2);

    VFormula length2F(spl.GetC2LengthFormula(), i->getData());
    length2F.setToolId(i->getId());
    length2F.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    length2F.Eval();
    QVariant length2;
    length2.setValue(length2F);
    m_idToProperty[AttrLength2]->setValue(length2);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(spl.GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolCubicBezier()
{
    auto *i = qgraphicsitem_cast<VToolCubicBezier *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstPointName());
    m_idToProperty[AttrPoint1]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrPoint2]->setValue(valueSecondPoint);

    QVariant valueThirdPoint;
    valueThirdPoint.setValue(i->ThirdPointName());
    m_idToProperty[AttrPoint3]->setValue(valueThirdPoint);

    QVariant valueForthPoint;
    valueForthPoint.setValue(i->ForthPointName());
    m_idToProperty[AttrPoint4]->setValue(valueForthPoint);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->getSpline().GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolSplinePath()
{
    auto *i = qgraphicsitem_cast<VToolSplinePath *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->getSplinePath().GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolCubicBezierPath()
{
    auto *i = qgraphicsitem_cast<VToolCubicBezierPath *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            CurvePenStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->getSplinePath().GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolTriangle()
{
    auto *i = qgraphicsitem_cast<VToolTriangle *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueAxisP1;
    valueAxisP1.setValue(i->AxisP1Name());
    m_idToProperty[AttrAxisP1]->setValue(valueAxisP1);

    QVariant valueAxisP2;
    valueAxisP2.setValue(i->AxisP2Name());
    m_idToProperty[AttrAxisP2]->setValue(valueAxisP2);

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstPointName());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondPointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolLineIntersectAxis()
{
    auto *i = qgraphicsitem_cast<VToolLineIntersectAxis *>(m_currentItem);
    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueAngle;
    valueAngle.setValue(i->GetFormulaAngle());
    m_idToProperty[AttrAngle]->setValue(valueAngle);

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstLinePoint());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondLinePoint());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolCurveIntersectAxis()
{
    auto *i = qgraphicsitem_cast<VToolCurveIntersectAxis *>(m_currentItem);
    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(
            LineStylesPics(comboBoxPalette.color(QPalette::Base), comboBoxPalette.color(QPalette::Text)),
            i->getLineType());
        m_idToProperty[AttrTypeLine]->setValue(index);
    }

    m_idToProperty[AttrLineColor]->setValue(i->GetLineColor());

    QVariant valueAngle;
    valueAngle.setValue(i->GetFormulaAngle());
    m_idToProperty[AttrAngle]->setValue(valueAngle);

    QVariant valueBasePoint;
    valueBasePoint.setValue(i->BasePointName());
    m_idToProperty[AttrBasePoint]->setValue(valueBasePoint);

    QVariant valueCurve;
    valueCurve.setValue(i->CurveName());
    m_idToProperty[AttrCurve]->setValue(valueCurve);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrCurveName1]->setValue(i->GetName1());
    m_idToProperty[AttrCurveName2]->setValue(i->GetName2());

    m_idToProperty[AttrAlias1]->setValue(i->GetAliasSuffix1());
    m_idToProperty[AttrAlias2]->setValue(i->GetAliasSuffix2());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolRotation()
{
    auto *i = qgraphicsitem_cast<VToolRotation *>(m_currentItem);

    QVariant valueAngle;
    valueAngle.setValue(i->GetFormulaAngle());
    m_idToProperty[AttrAngle]->setValue(valueAngle);

    QVariant valueOriginPoint;
    valueOriginPoint.setValue(i->OriginPointName());
    m_idToProperty[AttrCenter]->setValue(valueOriginPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolMove()
{
    auto *i = qgraphicsitem_cast<VToolMove *>(m_currentItem);

    QVariant valueAngle;
    valueAngle.setValue(i->GetFormulaAngle());
    m_idToProperty[AttrAngle]->setValue(valueAngle);

    QVariant valueLength;
    valueLength.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueLength);

    QVariant valueRotationAngle;
    valueRotationAngle.setValue(i->GetFormulaRotationAngle());
    m_idToProperty[AttrRotationAngle]->setValue(valueRotationAngle);

    QVariant valueRotationOriginPoint;
    valueRotationOriginPoint.setValue(i->OriginPointName());
    m_idToProperty[AttrCenter]->setValue(valueRotationOriginPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolFlippingByLine()
{
    auto *i = qgraphicsitem_cast<VToolFlippingByLine *>(m_currentItem);

    QVariant valueFirstPoint;
    valueFirstPoint.setValue(i->FirstLinePointName());
    m_idToProperty[AttrFirstPoint]->setValue(valueFirstPoint);

    QVariant valueSecondPoint;
    valueSecondPoint.setValue(i->SecondLinePointName());
    m_idToProperty[AttrSecondPoint]->setValue(valueSecondPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolFlippingByAxis()
{
    auto *i = qgraphicsitem_cast<VToolFlippingByAxis *>(m_currentItem);
    m_idToProperty[AttrAxisType]->setValue(static_cast<int>(i->GetAxisType()) - 1);

    QVariant valueOriginPoint;
    valueOriginPoint.setValue(i->OriginPointName());
    m_idToProperty[AttrCenter]->setValue(valueOriginPoint);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolEllipticalArc()
{
    auto *i = qgraphicsitem_cast<VToolEllipticalArc *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueFormulaRadius1;
    valueFormulaRadius1.setValue(i->GetFormulaRadius1());
    m_idToProperty[AttrRadius1]->setValue(valueFormulaRadius1);

    QVariant valueFormulaRadius2;
    valueFormulaRadius2.setValue(i->GetFormulaRadius2());
    m_idToProperty[AttrRadius2]->setValue(valueFormulaRadius2);

    QVariant valueFormulaF1;
    valueFormulaF1.setValue(i->GetFormulaF1());
    m_idToProperty[AttrAngle1]->setValue(valueFormulaF1);

    QVariant valueFormulaF2;
    valueFormulaF2.setValue(i->GetFormulaF2());
    m_idToProperty[AttrAngle2]->setValue(valueFormulaF2);

    QVariant valueFormulaRotationAngle;
    valueFormulaRotationAngle.setValue(i->GetFormulaRotationAngle());
    m_idToProperty[AttrRotationAngle]->setValue(valueFormulaRotationAngle);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                                                   comboBoxPalette.color(
                                                                                       QPalette::Text)),
                                                                i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueCenterPoint;
    valueCenterPoint.setValue(i->CenterPointName());
    m_idToProperty[AttrCenter]->setValue(valueCenterPoint);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsBackgroundPixmapItem()
{
    auto *i = qgraphicsitem_cast<VBackgroundPixmapItem *>(m_currentItem);

    m_idToProperty.value(AttrName)->setValue(i->name());
    m_idToProperty.value(VtoppbConstants::AttrHold)->setValue(i->IsHold());
    m_idToProperty.value(VtoppbConstants::AttrVisible)->setValue(i->IsVisible());
    m_idToProperty.value(VtoppbConstants::AttrOpacity)->setValue(static_cast<int>(i->GetOpacity() * 100));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsBackgroundSVGItem()
{
    auto *i = qgraphicsitem_cast<VBackgroundSVGItem *>(m_currentItem);

    m_idToProperty.value(AttrName)->setValue(i->name());
    m_idToProperty.value(VtoppbConstants::AttrHold)->setValue(i->IsHold());
    m_idToProperty.value(VtoppbConstants::AttrVisible)->setValue(i->IsVisible());
    m_idToProperty.value(VtoppbConstants::AttrOpacity)->setValue(static_cast<int>(i->GetOpacity() * 100));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolEllipticalArcWithLength()
{
    auto *i = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueRadius1;
    valueRadius1.setValue(i->GetFormulaRadius1());
    m_idToProperty[AttrRadius1]->setValue(valueRadius1);

    QVariant valueFormulaRadius2;
    valueFormulaRadius2.setValue(i->GetFormulaRadius2());
    m_idToProperty[AttrRadius2]->setValue(valueFormulaRadius2);

    QVariant valueFirstAngle;
    valueFirstAngle.setValue(i->GetFormulaF1());
    m_idToProperty[AttrAngle1]->setValue(valueFirstAngle);

    QVariant valueLength;
    valueLength.setValue(i->GetFormulaLength());
    m_idToProperty[AttrLength]->setValue(valueLength);

    QVariant valueFormulaRotationAngle;
    valueFormulaRotationAngle.setValue(i->GetFormulaRotationAngle());
    m_idToProperty[AttrRotationAngle]->setValue(valueFormulaRotationAngle);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                                                   comboBoxPalette.color(
                                                                                       QPalette::Text)),
                                                                i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueCenterPoint;
    valueCenterPoint.setValue(i->CenterPointName());
    m_idToProperty[AttrCenter]->setValue(valueCenterPoint);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolParallelCurve()
{
    auto *i = qgraphicsitem_cast<VToolParallelCurve *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    QVariant valueWidth;
    valueWidth.setValue(i->GetFormulaWidth());
    m_idToProperty[AttrWidth]->setValue(valueWidth);

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                                                   comboBoxPalette.color(
                                                                                       QPalette::Text)),
                                                                i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueCurve;
    valueCurve.setValue(i->CurveName());
    m_idToProperty[AttrCurve]->setValue(valueCurve);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[VtoppbConstants::AttrSubName]->setValue(i->GetName());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptionsToolGraduatedCurve()
{
    auto *i = qgraphicsitem_cast<VToolGraduatedCurve *>(m_currentItem);

    m_idToProperty[AttrName]->setValue(i->name());

    {
        QPalette const comboBoxPalette = ComboBoxPalette();
        const auto index = VPE::VLineTypeProperty::IndexOfStyle(CurvePenStylesPics(comboBoxPalette.color(QPalette::Base),
                                                                                   comboBoxPalette.color(
                                                                                       QPalette::Text)),
                                                                i->GetPenStyle());
        m_idToProperty[AttrPenStyle]->setValue(index);
    }

    m_idToProperty[AttrColor]->setValue(i->GetLineColor());

    QVariant valueCurve;
    valueCurve.setValue(i->CurveName());
    m_idToProperty[AttrCurve]->setValue(valueCurve);

    QVariant valueApproximationScale;
    valueApproximationScale.setValue(i->GetApproximationScale());
    m_idToProperty[AttrAScale]->setValue(valueApproximationScale);

    m_idToProperty[AttrNotes]->setValue(i->GetNotes());

    m_idToProperty[VtoppbConstants::AttrSubName]->setValue(i->GetName());

    m_idToProperty[AttrAlias]->setValue(i->GetAliasSuffix());
}

//---------------------------------------------------------------------------------------------------------------------
