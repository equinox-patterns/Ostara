/************************************************************************
 **
 **  @file   vtoppb_points.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  points property browser methods
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

