/************************************************************************
 **
 **  @file   vtoppb_misc.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  misc property browser methods
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
#include "vtoppb_templates.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtooltriangle.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncircles.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectionarcs.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersection.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofcontact.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromcircleandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromarcandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoollineintersect.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolshoulderpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolnormal.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollineintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolheight.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolendline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolbisector.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolalongline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooltruedarts.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"

#include "vformulaproperty.h"
#include "tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "ifcdef.h"
#include "../vwidgets/vsimplepoint.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/vcontrolpointspline.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vpropertyexplorer/plugins/vstringproperty.h"
#include "../vpropertyexplorer/plugins/vpointfproperty.h"
#include "../vpropertyexplorer/plugins/vnumberproperty.h"
#include "../vpropertyexplorer/plugins/vlinetypeproperty.h"
#include "../vpropertyexplorer/plugins/vlinecolorproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vboolproperty.h"
#include "../vpatterndb/vformula.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/def.h"
#include "../vgeometry/vsplinepath.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vcubicbezier.h"
#include "../qmuparser/qmudef.h"



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

