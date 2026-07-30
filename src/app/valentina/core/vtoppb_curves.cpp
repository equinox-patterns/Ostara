/************************************************************************
 **
 **  @file   vtoppb_curves.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  curves property browser methods
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
