/************************************************************************
 **
 **  @file   vtooloptionspropertybrowser.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 8, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vtooloptionspropertybrowser.h"
#include "vtoppb_templates.h"
using namespace VtoppbConstants;
#include "../qmuparser/qmudef.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vformula.h"
#include "../vpropertyexplorer/plugins/vboolproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/vlinecolorproperty.h"
#include "../vpropertyexplorer/plugins/vlinetypeproperty.h"
#include "../vpropertyexplorer/plugins/vnumberproperty.h"
#include "../vpropertyexplorer/plugins/vpointfproperty.h"
#include "../vpropertyexplorer/plugins/vstringproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vwidgets/vcontrolpointspline.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsimplepoint.h"
#include "ifcdef.h"
#include "tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "vformulaproperty.h"

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

#include <QComboBox>
#include <QDebug>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QObject>
#include <QPalette>
#include <QRegularExpression>
#include <QScrollArea>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
VToolOptionsPropertyBrowser::VToolOptionsPropertyBrowser(QDockWidget *parent)
  : QObject(parent)
{
    m_PropertyModel = new VPE::VPropertyModel(this);
    m_formView = new VPE::VPropertyFormView(m_PropertyModel, parent);
    m_formView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_formView);
    // Dot-selector: cible uniquement le viewport sans cascader aux enfants (SUBTILITES_CODE.md §13)
    scroll->viewport()->setStyleSheet(QStringLiteral(".QWidget { background: transparent; }"));

    parent->setWidget(scroll);

    connect(m_PropertyModel, &VPE::VPropertyModel::onDataChangedByEditor, this,
            &VToolOptionsPropertyBrowser::userChangedData);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ClearPropertyBrowser()
{
    m_PropertyModel->clear();
    m_propertyToId.clear();
    m_idToProperty.clear();
    m_currentItem = nullptr;
    m_currentObject = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowItemOptions(QGraphicsItem *item)
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were used in switch.");

    switch (item->type())
    {
        case VToolBasePoint::Type:
            ShowOptionsToolSinglePoint(item);
            break;
        case VToolEndLine::Type:
            ShowOptionsToolEndLine(item);
            break;
        case VToolAlongLine::Type:
            ShowOptionsToolAlongLine(item);
            break;
        case VToolArc::Type:
            ShowOptionsToolArc(item);
            break;
        case VToolArcWithLength::Type:
            ShowOptionsToolArcWithLength(item);
            break;
        case VToolBisector::Type:
            ShowOptionsToolBisector(item);
            break;
        case VToolCutArc::Type:
            ShowOptionsToolCutArc(item);
            break;
        case VToolCutSpline::Type:
            ShowOptionsToolCutSpline(item);
            break;
        case VToolCutSplinePath::Type:
            ShowOptionsToolCutSplinePath(item);
            break;
        case VToolHeight::Type:
            ShowOptionsToolHeight(item);
            break;
        case VToolLine::Type:
            ShowOptionsToolLine(item);
            break;
        case VToolLineIntersect::Type:
            ShowOptionsToolLineIntersect(item);
            break;
        case VToolNormal::Type:
            ShowOptionsToolNormal(item);
            break;
        case VToolPointOfContact::Type:
            ShowOptionsToolPointOfContact(item);
            break;
        case VToolPointOfIntersection::Type:
            ShowOptionsToolPointOfIntersection(item);
            break;
        case VToolPointOfIntersectionArcs::Type:
            ShowOptionsToolPointOfIntersectionArcs(item);
            break;
        case VToolPointOfIntersectionCircles::Type:
            ShowOptionsToolPointOfIntersectionCircles(item);
            break;
        case VToolPointOfIntersectionCurves::Type:
            ShowOptionsToolPointOfIntersectionCurves(item);
            break;
        case VToolShoulderPoint::Type:
            ShowOptionsToolShoulderPoint(item);
            break;
        case VToolSpline::Type:
            ShowOptionsToolSpline(item);
            break;
        case VToolCubicBezier::Type:
            ShowOptionsToolCubicBezier(item);
            break;
        case VToolSplinePath::Type:
            ShowOptionsToolSplinePath(item);
            break;
        case VToolCubicBezierPath::Type:
            ShowOptionsToolCubicBezierPath(item);
            break;
        case VToolTriangle::Type:
            ShowOptionsToolTriangle(item);
            break;
        case VGraphicsSimpleTextItem::Type:
        case VControlPointSpline::Type:
        case VSimplePoint::Type:
        case VSimpleCurve::Type:
        case VSegmentLabel::Type:
            m_currentItem = item->parentItem();
            m_currentObject = dynamic_cast<QObject *>(m_currentItem);
            ShowItemOptions(m_currentItem);
            break;
        case VToolLineIntersectAxis::Type:
            ShowOptionsToolLineIntersectAxis(item);
            break;
        case VToolCurveIntersectAxis::Type:
            ShowOptionsToolCurveIntersectAxis(item);
            break;
        case VToolPointFromCircleAndTangent::Type:
            ShowOptionsToolPointFromCircleAndTangent(item);
            break;
        case VToolPointFromArcAndTangent::Type:
            ShowOptionsToolPointFromArcAndTangent(item);
            break;
        case VToolTrueDarts::Type:
            ShowOptionsToolTrueDarts(item);
            break;
        case VToolRotation::Type:
            ShowOptionsToolRotation(item);
            break;
        case VToolFlippingByLine::Type:
            ShowOptionsToolFlippingByLine(item);
            break;
        case VToolFlippingByAxis::Type:
            ShowOptionsToolFlippingByAxis(item);
            break;
        case VToolMove::Type:
            ShowOptionsToolMove(item);
            break;
        case VToolEllipticalArc::Type:
            ShowOptionsToolEllipticalArc(item);
            break;
        case VBackgroundPixmapItem::Type:
            ShowOptionsBackgroundPixmapItem(item);
            break;
        case VBackgroundSVGItem::Type:
            ShowOptionsBackgroundSVGItem(item);
            break;
        case VToolEllipticalArcWithLength::Type:
            ShowOptionsToolEllipticalArcWithLength(item);
            break;
        case VToolParallelCurve::Type:
            ShowOptionsToolParallelCurve(item);
            break;
        case VToolGraduatedCurve::Type:
            ShowOptionsToolGraduatedCurve(item);
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::UpdateOptions()
{
    if (m_currentItem == nullptr)
    {
        return;
    }

    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were used in switch.");

    switch (m_currentItem->type())
    {
        case VToolBasePoint::Type:
            UpdateOptionsToolSinglePoint();
            break;
        case VToolEndLine::Type:
            UpdateOptionsToolEndLine();
            break;
        case VToolAlongLine::Type:
            UpdateOptionsToolAlongLine();
            break;
        case VToolArc::Type:
            UpdateOptionsToolArc();
            break;
        case VToolArcWithLength::Type:
            UpdateOptionsToolArcWithLength();
            break;
        case VToolBisector::Type:
            UpdateOptionsToolBisector();
            break;
        case VToolCutArc::Type:
            UpdateOptionsToolCutArc();
            break;
        case VToolCutSpline::Type:
            UpdateOptionsToolCutSpline();
            break;
        case VToolCutSplinePath::Type:
            UpdateOptionsToolCutSplinePath();
            break;
        case VToolHeight::Type:
            UpdateOptionsToolHeight();
            break;
        case VToolLine::Type:
            UpdateOptionsToolLine();
            break;
        case VToolLineIntersect::Type:
            UpdateOptionsToolLineIntersect();
            break;
        case VToolNormal::Type:
            UpdateOptionsToolNormal();
            break;
        case VToolPointOfContact::Type:
            UpdateOptionsToolPointOfContact();
            break;
        case VToolPointOfIntersection::Type:
            UpdateOptionsToolPointOfIntersection();
            break;
        case VToolPointOfIntersectionArcs::Type:
            UpdateOptionsToolPointOfIntersectionArcs();
            break;
        case VToolPointOfIntersectionCircles::Type:
            UpdateOptionsToolPointOfIntersectionCircles();
            break;
        case VToolPointOfIntersectionCurves::Type:
            UpdateOptionsToolPointOfIntersectionCurves();
            break;
        case VToolShoulderPoint::Type:
            UpdateOptionsToolShoulderPoint();
            break;
        case VToolSpline::Type:
            UpdateOptionsToolSpline();
            break;
        case VToolCubicBezier::Type:
            UpdateOptionsToolCubicBezier();
            break;
        case VToolSplinePath::Type:
            UpdateOptionsToolSplinePath();
            break;
        case VToolCubicBezierPath::Type:
            UpdateOptionsToolCubicBezierPath();
            break;
        case VToolTriangle::Type:
            UpdateOptionsToolTriangle();
            break;
        case VGraphicsSimpleTextItem::Type:
        case VControlPointSpline::Type:
            ShowItemOptions(m_currentItem->parentItem());
            break;
        case VToolLineIntersectAxis::Type:
            UpdateOptionsToolLineIntersectAxis();
            break;
        case VToolCurveIntersectAxis::Type:
            UpdateOptionsToolCurveIntersectAxis();
            break;
        case VToolPointFromCircleAndTangent::Type:
            UpdateOptionsToolPointFromCircleAndTangent();
            break;
        case VToolPointFromArcAndTangent::Type:
            UpdateOptionsToolPointFromArcAndTangent();
            break;
        case VToolTrueDarts::Type:
            UpdateOptionsToolTrueDarts();
            break;
        case VToolRotation::Type:
            UpdateOptionsToolRotation();
            break;
        case VToolFlippingByLine::Type:
            UpdateOptionsToolFlippingByLine();
            break;
        case VToolFlippingByAxis::Type:
            UpdateOptionsToolFlippingByAxis();
            break;
        case VToolMove::Type:
            UpdateOptionsToolMove();
            break;
        case VToolEllipticalArc::Type:
            UpdateOptionsToolEllipticalArc();
            break;
        case VBackgroundPixmapItem::Type:
            UpdateOptionsBackgroundPixmapItem();
            break;
        case VBackgroundSVGItem::Type:
            UpdateOptionsBackgroundSVGItem();
            break;
        case VToolEllipticalArcWithLength::Type:
            UpdateOptionsToolEllipticalArcWithLength();
            break;
        case VToolParallelCurve::Type:
            UpdateOptionsToolParallelCurve();
            break;
        case VToolGraduatedCurve::Type:
            UpdateOptionsToolGraduatedCurve();
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::RefreshOptions()
{
    QGraphicsItem *item = m_currentItem;
    QPointer<QObject> object = m_currentObject;
    itemClicked(nullptr); // close options
    if (!object.isNull())
    {
        itemClicked(item); // reopen options only if item is still alive
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::userChangedData(VPE::VProperty *property)
{
    VPE::VProperty *prop = property;
    if (!m_propertyToId.contains(prop))
    {
        if (!m_propertyToId.contains(prop->getParent())) // Maybe we know parent
        {
            return;
        }

        prop = prop->getParent();
    }

    if (!m_currentItem)
    {
        return;
    }

    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were used in switch.");

    switch (m_currentItem->type())
    {
        case VToolBasePoint::Type:
            ChangeDataToolSinglePoint(prop);
            break;
        case VToolEndLine::Type:
            ChangeDataToolEndLine(prop);
            break;
        case VToolAlongLine::Type:
            ChangeDataToolAlongLine(prop);
            break;
        case VToolArc::Type:
            ChangeDataToolArc(prop);
            break;
        case VToolArcWithLength::Type:
            ChangeDataToolArcWithLength(prop);
            break;
        case VToolBisector::Type:
            ChangeDataToolBisector(prop);
            break;
        case VToolCutArc::Type:
            ChangeDataToolCutArc(prop);
            break;
        case VToolCutSpline::Type:
            ChangeDataToolCutSpline(prop);
            break;
        case VToolCutSplinePath::Type:
            ChangeDataToolCutSplinePath(prop);
            break;
        case VToolHeight::Type:
            ChangeDataToolHeight(prop);
            break;
        case VToolLine::Type:
            ChangeDataToolLine(prop);
            break;
        case VToolLineIntersect::Type:
            ChangeDataToolLineIntersect(prop);
            break;
        case VToolNormal::Type:
            ChangeDataToolNormal(prop);
            break;
        case VToolPointOfContact::Type:
            ChangeDataToolPointOfContact(prop);
            break;
        case VToolPointOfIntersection::Type:
            ChangeDataToolPointOfIntersection(prop);
            break;
        case VToolPointOfIntersectionArcs::Type:
            ChangeDataToolPointOfIntersectionArcs(prop);
            break;
        case VToolPointOfIntersectionCircles::Type:
            ChangeDataToolPointOfIntersectionCircles(prop);
            break;
        case VToolPointOfIntersectionCurves::Type:
            ChangeDataToolPointOfIntersectionCurves(prop);
            break;
        case VToolShoulderPoint::Type:
            ChangeDataToolShoulderPoint(prop);
            break;
        case VToolSpline::Type:
            ChangeDataToolSpline(prop);
            break;
        case VToolCubicBezier::Type:
            ChangeDataToolCubicBezier(prop);
            break;
        case VToolSplinePath::Type:
            ChangeDataToolSplinePath(prop);
            break;
        case VToolCubicBezierPath::Type:
            ChangeDataToolCubicBezierPath(prop);
            break;
        case VToolTriangle::Type:
            ChangeDataToolTriangle(prop);
            break;
        case VToolLineIntersectAxis::Type:
            ChangeDataToolLineIntersectAxis(prop);
            break;
        case VToolCurveIntersectAxis::Type:
            ChangeDataToolCurveIntersectAxis(prop);
            break;
        case VToolPointFromCircleAndTangent::Type:
            ChangeDataToolPointFromCircleAndTangent(prop);
            break;
        case VToolPointFromArcAndTangent::Type:
            ChangeDataToolPointFromArcAndTangent(prop);
            break;
        case VToolTrueDarts::Type:
            ChangeDataToolTrueDarts(prop);
            break;
        case VToolRotation::Type:
            ChangeDataToolRotation(prop);
            break;
        case VToolFlippingByLine::Type:
            ChangeDataToolFlippingByLine(prop);
            break;
        case VToolFlippingByAxis::Type:
            ChangeDataToolFlippingByAxis(prop);
            break;
        case VToolMove::Type:
            ChangeDataToolMove(prop);
            break;
        case VToolEllipticalArc::Type:
            ChangeDataToolEllipticalArc(prop);
            break;
        case VBackgroundPixmapItem::Type:
            ChangeDataBackgroundPixmapItem(prop);
            break;
        case VBackgroundSVGItem::Type:
            ChangeDataBackgroundSVGItem(prop);
            break;
        case VToolEllipticalArcWithLength::Type:
            ChangeDataToolEllipticalArcWithLength(prop);
            break;
        case VToolParallelCurve::Type:
            ChangeDataToolParallelCurve(prop);
            break;
        case VToolGraduatedCurve::Type:
            ChangeDataToolGraduatedCurve(prop);
            break;
        default:
            break;
    }
    VAbstractValApplication::VApp()->getSceneView()->update();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolOptionsPropertyBrowser::ComboBoxPalette() const -> QPalette
{
    QComboBox comboBox;
    comboBox.setPalette(m_formView->palette());
    return comboBox.palette();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::itemClicked(QGraphicsItem *item)
{
    if (item != nullptr && not item->isEnabled())
    {
        return;
    }

    if (m_currentItem == item && item != nullptr)
    {
        UpdateOptions();
        return;
    }

    m_PropertyModel->clear();
    m_propertyToId.clear();
    m_idToProperty.clear();

    if (m_currentItem != nullptr && !m_currentObject.isNull())
    {
        auto *previousTool = dynamic_cast<VAbstractTool *>(m_currentItem);
        if (previousTool != nullptr)
        {
            previousTool->ShowVisualization(false); // hide for previous tool
        }
    }

    m_currentItem = item;
    m_currentObject = dynamic_cast<QObject *>(m_currentItem);
    if (m_currentItem == nullptr)
    {
        m_formView->setTitle(QString());
        return;
    }

    ShowItemOptions(m_currentItem);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::AddPropertyFormula(const QString &propertyName, const VFormula &formula,
                                                     const QString &attrName)
{
    auto *itemLength = new VFormulaProperty(propertyName);
    itemLength->SetFormula(formula);
    AddProperty(itemLength, attrName);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolOptionsPropertyBrowser::PropertiesList() -> QStringList
{
    static QStringList const attr{
        AttrName,                           /* 0 */
        "position"_L1,                      /* 1 */
        AttrBasePoint,                      /* 2 */
        AttrTypeLine,                       /* 3 */
        AttrLength,                         /* 4 */
        AttrAngle,                          /* 5 */
        AttrFirstPoint,                     /* 6 */
        AttrSecondPoint,                    /* 7 */
        AttrRadius,                         /* 8 */
        AttrAngle1,                         /* 9 */
        AttrAngle2,                         /* 10 */
        AttrCenter,                         /* 11 */
        AttrThirdPoint,                     /* 12 */
        AttrArc,                            /* 13 */
        VToolCutSpline::AttrSpline,         /* 14 */
        VToolCutSplinePath::AttrSplinePath, /* 15 */
        AttrP1Line,                         /* 16 */
        AttrP2Line,                         /* 17 */
        AttrP1Line1,                        /* 18 */
        AttrP2Line1,                        /* 19 */
        AttrP1Line2,                        /* 20 */
        AttrP2Line2,                        /* 21 */
        AttrPShoulder,                      /* 22 */
        AttrAxisP1,                         /* 23 */
        AttrAxisP2,                         /* 24 */
        AttrWidth,                          /* 25 */
        AttrLineColor,                      /* 26 */
        AttrColor,                          /* 27 */
        AttrCrossPoint,                     /* 28 */
        AttrC1Radius,                       /* 29 */
        AttrC2Radius,                       /* 30 */
        AttrCRadius,                        /* 31 */
        AttrName1,                          /* 32 */
        AttrName2,                          /* 33 */
        AttrVCrossPoint,                    /* 34 */
        AttrHCrossPoint,                    /* 35 */
        AttrLength1,                        /* 36 */
        AttrLength2,                        /* 37 */
        AttrSuffix,                         /* 38 */
        AttrAxisType,                       /* 39 */
        AttrRadius1,                        /* 40 */
        AttrRadius2,                        /* 41 */
        AttrRotationAngle,                  /* 42 */
        AttrDartP1,                         /* 43 */
        AttrDartP2,                         /* 44 */
        AttrDartP3,                         /* 45 */
        AttrCurve,                          /* 46 */
        AttrFirstArc,                       /* 47 */
        AttrSecondArc,                      /* 48 */
        AttrC1Center,                       /* 49 */
        AttrC2Center,                       /* 50 */
        AttrCurve1,                         /* 51 */
        AttrCurve2,                         /* 52 */
        AttrCCenter,                        /* 53 */
        AttrTangent,                        /* 54 */
        AttrPoint1,                         /* 55 */
        AttrPoint2,                         /* 56 */
        AttrPoint3,                         /* 57 */
        AttrPoint4,                         /* 58 */
        AttrPenStyle,                       /* 59 */
        AttrAScale,                         /* 60 */
        AttrNotes,                          /* 61 */
        AttrAlias,                          /* 62 */
        AttrAlias1,                         /* 63 */
        AttrAlias2,                         /* 64 */
        VtoppbConstants::AttrHold,                          /* 65 */
        VtoppbConstants::AttrVisible,                       /* 66 */
        VtoppbConstants::AttrOpacity,                       /* 67 */
        VtoppbConstants::AttrSubName,                       /* 68 */
        AttrCurveName1,                     /* 69 */
        AttrCurveName2,                     /* 70 */
        AttrCurve1Name1,                    /* 71 */
        AttrCurve1Name2,                    /* 72 */
        AttrCurve2Name1,                    /* 73 */
        AttrCurve2Name2,                    /* 74 */
        AttrCurve1Alias1,                   /* 75 */
        AttrCurve1Alias2,                   /* 76 */
        AttrCurve2Alias1,                   /* 77 */
        AttrCurve2Alias2                    /* 78 */
    };
    return attr;
}

//---------------------------------------------------------------------------------------------------------------------
