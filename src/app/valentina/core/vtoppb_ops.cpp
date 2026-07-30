/************************************************************************
 **
 **  @file   vtoppb_ops.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  ops property browser methods
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

