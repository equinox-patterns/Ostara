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
#include "../vtheme/vtheme.h"
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
#include "../vpropertyexplorer/plugins/vwidgetproperty.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vwidgets/vcontrolpointspline.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/scalesceneitems.h"
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
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolarcpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolcurvepoint.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarmholecurve.h"
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
#include <QVBoxLayout>
#include <QObject>
#include <QApplication>
#include <QPalette>
#include <QRegularExpression>
#include <QScrollArea>
#include <QPushButton>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

// Forward declarations — declarative property descriptors (defined in vtoppb_*.cpp)
auto RotationProperties() -> std::vector<PropertyDesc>;
auto MoveProperties() -> std::vector<PropertyDesc>;
auto FlippingByLineProperties() -> std::vector<PropertyDesc>;
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>;
auto PointOfIntersectionArcsProperties() -> std::vector<PropertyDesc>;
auto PointOfIntersectionCirclesProperties() -> std::vector<PropertyDesc>;
auto PointOfIntersectionCurvesProperties() -> std::vector<PropertyDesc>;
auto PointFromCircleAndTangentProperties() -> std::vector<PropertyDesc>;
auto PointFromArcAndTangentProperties() -> std::vector<PropertyDesc>;
auto BackgroundPixmapItemProperties() -> std::vector<PropertyDesc>;
auto BackgroundSVGItemProperties() -> std::vector<PropertyDesc>;
auto SinglePointProperties() -> std::vector<PropertyDesc>;
auto ArcPointProperties() -> std::vector<PropertyDesc>;
auto CurvePointProperties() -> std::vector<PropertyDesc>;
auto EndLineProperties() -> std::vector<PropertyDesc>;
auto AlongLineProperties() -> std::vector<PropertyDesc>;
auto BisectorProperties() -> std::vector<PropertyDesc>;
auto TrueDartsProperties() -> std::vector<PropertyDesc>;
auto HeightProperties() -> std::vector<PropertyDesc>;
auto LineProperties() -> std::vector<PropertyDesc>;
auto LineIntersectProperties() -> std::vector<PropertyDesc>;
auto NormalProperties() -> std::vector<PropertyDesc>;
auto PointOfContactProperties() -> std::vector<PropertyDesc>;
auto PointOfIntersectionProperties() -> std::vector<PropertyDesc>;
auto ShoulderPointProperties() -> std::vector<PropertyDesc>;
auto TriangleProperties() -> std::vector<PropertyDesc>;
auto LineIntersectAxisProperties() -> std::vector<PropertyDesc>;
auto CurveIntersectAxisProperties() -> std::vector<PropertyDesc>;
auto ArcProperties() -> std::vector<PropertyDesc>;
auto ArcWithLengthProperties() -> std::vector<PropertyDesc>;
auto CutArcProperties() -> std::vector<PropertyDesc>;
auto CutSplineProperties() -> std::vector<PropertyDesc>;
auto CutSplinePathProperties() -> std::vector<PropertyDesc>;
auto SplineProperties() -> std::vector<PropertyDesc>;
auto CubicBezierProperties() -> std::vector<PropertyDesc>;
auto ArmholeCurveProperties() -> std::vector<PropertyDesc>;
auto SplinePathProperties(const VToolSplinePath *tool) -> std::vector<PropertyDesc>;
auto CubicBezierPathProperties() -> std::vector<PropertyDesc>;
auto EllipticalArcProperties() -> std::vector<PropertyDesc>;
auto EllipticalArcWithLengthProperties() -> std::vector<PropertyDesc>;
auto ParallelCurveProperties() -> std::vector<PropertyDesc>;
auto GraduatedCurveProperties() -> std::vector<PropertyDesc>;

//---------------------------------------------------------------------------------------------------------------------
VToolOptionsPropertyBrowser::VToolOptionsPropertyBrowser(QWidget *parent)
  : QObject(parent)
{
    m_PropertyModel = new VPE::VPropertyModel(this);
    m_formView = new VPE::VPropertyFormView(m_PropertyModel, parent);
    m_formView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_formView->setDarkTheme(VTheme::ColorSheme() == VColorSheme::Dark);

    auto *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);

    // Wrap QGroupBox in a widget whose layout has only top margin.
    // QSS margin-top on the QGroupBox is clipped by the scroll area's
    // viewport (widget is at y=0), so we compensate with a top-only layout margin.
    auto *scrollContents = new QWidget(scroll);
    auto *scrollLayout = new QVBoxLayout(scrollContents);
    scrollLayout->setContentsMargins(0, 8, 0, 0);
    scrollLayout->setSpacing(0);
    scrollLayout->addWidget(m_formView);
    scroll->setWidget(scrollContents);

    m_scroll = scroll;

    connect(m_PropertyModel, &VPE::VPropertyModel::onDataChangedByEditor, this,
            &VToolOptionsPropertyBrowser::userChangedData);
}

//---------------------------------------------------------------------------------------------------------------------
QWidget *VToolOptionsPropertyBrowser::widget() const
{
    return m_scroll;
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
void VToolOptionsPropertyBrowser::ShowGroupOptions(const QString &groupName, quint32 attachmentId)
{
    ClearPropertyBrowser();
    m_groupAttachmentId = attachmentId;
    m_formView->setTitle(tr("Group"));

    auto *nameProperty = new VPE::VStringProperty(tr("Group name:"));
    nameProperty->setValue(groupName);
    AddProperty(nameProperty, AttrAttachmentName);

    auto *detachButton = new QPushButton(tr("Detach all"), m_formView);
    connect(detachButton, &QPushButton::clicked, this,
            [this]() { emit DetachAllRequested(m_groupAttachmentId); });
    AddProperty(new VPE::VWidgetProperty(QString(), detachButton), QStringLiteral("detachButton"));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::ShowItemOptions(QGraphicsItem *item)
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 70, "Not all tools were used in switch.");

    switch (item->type())
    {
        case VToolBasePoint::Type:
            ShowToolOptions<VToolBasePoint>(item, tr("Base point"), SinglePointProperties());
            break;
        case VToolArcPoint::Type:
            ShowToolOptions<VToolArcPoint>(item, tr("Arc point"), ArcPointProperties());
            break;
        case VToolCurvePoint::Type:
            ShowToolOptions<VToolCurvePoint>(item, tr("Curve point"), CurvePointProperties());
            break;
        case VToolEndLine::Type:
            ShowToolOptions<VToolEndLine>(item, tr("Point at distance and angle"), EndLineProperties());
            break;
        case VToolAlongLine::Type:
            ShowToolOptions<VToolAlongLine>(item, tr("Point at distance along line"), AlongLineProperties());
            break;
        case VToolArc::Type:
            ShowToolOptions<VToolArc>(item, tr("Arc"), ArcProperties());
            break;
        case VToolArcWithLength::Type:
            ShowToolOptions<VToolArcWithLength>(item, tr("Arc with given length"), ArcWithLengthProperties());
            break;
        case VToolBisector::Type:
            ShowToolOptions<VToolBisector>(item, tr("Point along bisector"), BisectorProperties());
            break;
        case VToolCutArc::Type:
            ShowToolOptions<VToolCutArc>(item, tr("Cut arc tool"), CutArcProperties());
            break;
        case VToolCutSpline::Type:
            ShowToolOptions<VToolCutSpline>(item, tr("Tool segment a curve"), CutSplineProperties());
            break;
        case VToolCutSplinePath::Type:
            ShowToolOptions<VToolCutSplinePath>(item, tr("Tool segment a curve"), CutSplinePathProperties());
            break;
        case VToolHeight::Type:
            ShowToolOptions<VToolHeight>(item, tr("Perpendicular point along line"), HeightProperties());
            break;
        case VToolLine::Type:
            ShowToolOptions<VToolLine>(item, tr("Line between points"), LineProperties());
            break;
        case VToolLineIntersect::Type:
            ShowToolOptions<VToolLineIntersect>(item, tr("Point at intersection of two lines"), LineIntersectProperties());
            break;
        case VToolNormal::Type:
            ShowToolOptions<VToolNormal>(item, tr("Point along normal to line"), NormalProperties());
            break;
        case VToolPointOfContact::Type:
            ShowToolOptions<VToolPointOfContact>(item, tr("Point of contact"), PointOfContactProperties());
            break;
        case VToolPointOfIntersection::Type:
            ShowToolOptions<VToolPointOfIntersection>(item, tr("Point of intersection two lines"), PointOfIntersectionProperties());
            break;
        case VToolPointOfIntersectionArcs::Type:
            ShowToolOptions<VToolPointOfIntersectionArcs>(item, tr("Tool to make point from intersection two arcs"), PointOfIntersectionArcsProperties());
            break;
        case VToolPointOfIntersectionCircles::Type:
            ShowToolOptions<VToolPointOfIntersectionCircles>(item, tr("Tool to make point from intersection two circles"), PointOfIntersectionCirclesProperties());
            break;
        case VToolPointOfIntersectionCurves::Type:
            ShowToolOptions<VToolPointOfIntersectionCurves>(item, tr("Tool to make point from intersection two curves"), PointOfIntersectionCurvesProperties());
            break;
        case VToolShoulderPoint::Type:
            ShowToolOptions<VToolShoulderPoint>(item, tr("Shoulder point"), ShoulderPointProperties());
            break;
        case VToolSpline::Type:
            ShowToolOptions<VToolSpline>(item, tr("Curve tool"), SplineProperties());
            break;
        case VToolCubicBezier::Type:
            ShowToolOptions<VToolCubicBezier>(item, tr("Cubic bezier curve"), CubicBezierProperties());
            break;
        case VToolArmholeCurve::Type:
            ShowToolOptions<VToolArmholeCurve>(item, tr("Armhole curve"), ArmholeCurveProperties());
            break;
        case VToolSplinePath::Type:
            ShowToolOptions<VToolSplinePath>(item, tr("Curve tool"),
                                             SplinePathProperties(qgraphicsitem_cast<VToolSplinePath *>(item)));
            break;
        case VToolCubicBezierPath::Type:
            ShowToolOptions<VToolCubicBezierPath>(item, tr("Tool cubic bezier curve"), CubicBezierPathProperties());
            break;
        case VToolTriangle::Type:
            ShowToolOptions<VToolTriangle>(item, tr("Triangle"), TriangleProperties());
            break;
        case VGraphicsSimpleTextItem::Type:
        case VControlPointSpline::Type:
        case VSimplePoint::Type:
        case VSimpleCurve::Type:
        case VSegmentLabel::Type:
        case VScaledLine::Type: // line child of point tools (EndLine, Bisector…)
            m_currentItem = item->parentItem();
            m_currentObject = dynamic_cast<QObject *>(m_currentItem);
            ShowItemOptions(m_currentItem);
            break;
        case VToolLineIntersectAxis::Type:
            ShowToolOptions<VToolLineIntersectAxis>(item, tr("Point of intersection line and axis"), LineIntersectAxisProperties());
            break;
        case VToolCurveIntersectAxis::Type:
            ShowToolOptions<VToolCurveIntersectAxis>(item, tr("Point of intersection curve and axis"), CurveIntersectAxisProperties());
            break;
        case VToolPointFromCircleAndTangent::Type:
            ShowToolOptions<VToolPointFromCircleAndTangent>(item, tr("Tool to make point from circle and tangent"), PointFromCircleAndTangentProperties());
            break;
        case VToolPointFromArcAndTangent::Type:
            ShowToolOptions<VToolPointFromArcAndTangent>(item, tr("Tool to make point from arc and tangent"), PointFromArcAndTangentProperties());
            break;
        case VToolTrueDarts::Type:
            ShowToolOptions<VToolTrueDarts>(item, tr("True darts"), TrueDartsProperties());
            break;
        case VToolRotation::Type:
            ShowToolOptions<VToolRotation>(item, tr("Tool rotation"), RotationProperties());
            break;
        case VToolFlippingByLine::Type:
            ShowToolOptions<VToolFlippingByLine>(item, tr("Tool flipping by line"), FlippingByLineProperties());
            break;
        case VToolFlippingByAxis::Type:
            ShowToolOptions<VToolFlippingByAxis>(item, tr("Tool flipping by axis"), FlippingByAxisProperties());
            break;
        case VToolMove::Type:
            ShowToolOptions<VToolMove>(item, tr("Tool move"), MoveProperties());
            break;
        case VToolEllipticalArc::Type:
            ShowToolOptions<VToolEllipticalArc>(item, tr("Elliptical arc"), EllipticalArcProperties());
            break;
        case VBackgroundPixmapItem::Type:
            ShowToolOptions<VBackgroundPixmapItem>(item, tr("Background image"), BackgroundPixmapItemProperties());
            break;
        case VBackgroundSVGItem::Type:
            ShowToolOptions<VBackgroundSVGItem>(item, tr("Background image"), BackgroundSVGItemProperties());
            break;
        case VToolEllipticalArcWithLength::Type:
            ShowToolOptions<VToolEllipticalArcWithLength>(item, tr("Elliptical arc with length"), EllipticalArcWithLengthProperties());
            break;
        case VToolParallelCurve::Type:
            ShowToolOptions<VToolParallelCurve>(item, tr("Tool parallel curve"), ParallelCurveProperties());
            break;
        case VToolGraduatedCurve::Type:
            ShowToolOptions<VToolGraduatedCurve>(item, tr("Tool graduated curve"), GraduatedCurveProperties());
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
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 70, "Not all tools were used in switch.");

    switch (m_currentItem->type())
    {
        case VToolBasePoint::Type:
            UpdateToolOptions<VToolBasePoint>(SinglePointProperties());
            break;
        case VToolArcPoint::Type:
            UpdateToolOptions<VToolArcPoint>(ArcPointProperties());
            break;
        case VToolCurvePoint::Type:
            UpdateToolOptions<VToolCurvePoint>(CurvePointProperties());
            break;
        case VToolEndLine::Type:
            UpdateToolOptions<VToolEndLine>(EndLineProperties());
            break;
        case VToolAlongLine::Type:
            UpdateToolOptions<VToolAlongLine>(AlongLineProperties());
            break;
        case VToolArc::Type:
            UpdateToolOptions<VToolArc>(ArcProperties());
            break;
        case VToolArcWithLength::Type:
            UpdateToolOptions<VToolArcWithLength>(ArcWithLengthProperties());
            break;
        case VToolBisector::Type:
            UpdateToolOptions<VToolBisector>(BisectorProperties());
            break;
        case VToolCutArc::Type:
            UpdateToolOptions<VToolCutArc>(CutArcProperties());
            break;
        case VToolCutSpline::Type:
            UpdateToolOptions<VToolCutSpline>(CutSplineProperties());
            break;
        case VToolCutSplinePath::Type:
            UpdateToolOptions<VToolCutSplinePath>(CutSplinePathProperties());
            break;
        case VToolHeight::Type:
            UpdateToolOptions<VToolHeight>(HeightProperties());
            break;
        case VToolLine::Type:
            UpdateToolOptions<VToolLine>(LineProperties());
            break;
        case VToolLineIntersect::Type:
            UpdateToolOptions<VToolLineIntersect>(LineIntersectProperties());
            break;
        case VToolNormal::Type:
            UpdateToolOptions<VToolNormal>(NormalProperties());
            break;
        case VToolPointOfContact::Type:
            UpdateToolOptions<VToolPointOfContact>(PointOfContactProperties());
            break;
        case VToolPointOfIntersection::Type:
            UpdateToolOptions<VToolPointOfIntersection>(PointOfIntersectionProperties());
            break;
        case VToolPointOfIntersectionArcs::Type:
            UpdateToolOptions<VToolPointOfIntersectionArcs>(PointOfIntersectionArcsProperties());
            break;
        case VToolPointOfIntersectionCircles::Type:
            UpdateToolOptions<VToolPointOfIntersectionCircles>(PointOfIntersectionCirclesProperties());
            break;
        case VToolPointOfIntersectionCurves::Type:
            UpdateToolOptions<VToolPointOfIntersectionCurves>(PointOfIntersectionCurvesProperties());
            break;
        case VToolShoulderPoint::Type:
            UpdateToolOptions<VToolShoulderPoint>(ShoulderPointProperties());
            break;
        case VToolSpline::Type:
            UpdateToolOptions<VToolSpline>(SplineProperties());
            break;
        case VToolCubicBezier::Type:
            UpdateToolOptions<VToolCubicBezier>(CubicBezierProperties());
            break;
        case VToolArmholeCurve::Type:
            UpdateToolOptions<VToolArmholeCurve>(ArmholeCurveProperties());
            break;
        case VToolSplinePath::Type:
            UpdateToolOptions<VToolSplinePath>(
                SplinePathProperties(qgraphicsitem_cast<VToolSplinePath *>(m_currentItem)));
            break;
        case VToolCubicBezierPath::Type:
            UpdateToolOptions<VToolCubicBezierPath>(CubicBezierPathProperties());
            break;
        case VToolTriangle::Type:
            UpdateToolOptions<VToolTriangle>(TriangleProperties());
            break;
        case VGraphicsSimpleTextItem::Type:
        case VControlPointSpline::Type:
            ShowItemOptions(m_currentItem->parentItem());
            break;
        case VToolLineIntersectAxis::Type:
            UpdateToolOptions<VToolLineIntersectAxis>(LineIntersectAxisProperties());
            break;
        case VToolCurveIntersectAxis::Type:
            UpdateToolOptions<VToolCurveIntersectAxis>(CurveIntersectAxisProperties());
            break;
        case VToolPointFromCircleAndTangent::Type:
            UpdateToolOptions<VToolPointFromCircleAndTangent>(PointFromCircleAndTangentProperties());
            break;
        case VToolPointFromArcAndTangent::Type:
            UpdateToolOptions<VToolPointFromArcAndTangent>(PointFromArcAndTangentProperties());
            break;
        case VToolTrueDarts::Type:
            UpdateToolOptions<VToolTrueDarts>(TrueDartsProperties());
            break;
        case VToolRotation::Type:
            UpdateToolOptions<VToolRotation>(RotationProperties());
            break;
        case VToolFlippingByLine::Type:
            UpdateToolOptions<VToolFlippingByLine>(FlippingByLineProperties());
            break;
        case VToolFlippingByAxis::Type:
            UpdateToolOptions<VToolFlippingByAxis>(FlippingByAxisProperties());
            break;
        case VToolMove::Type:
            UpdateToolOptions<VToolMove>(MoveProperties());
            break;
        case VToolEllipticalArc::Type:
            UpdateToolOptions<VToolEllipticalArc>(EllipticalArcProperties());
            break;
        case VBackgroundPixmapItem::Type:
            UpdateToolOptions<VBackgroundPixmapItem>(BackgroundPixmapItemProperties());
            break;
        case VBackgroundSVGItem::Type:
            UpdateToolOptions<VBackgroundSVGItem>(BackgroundSVGItemProperties());
            break;
        case VToolEllipticalArcWithLength::Type:
            UpdateToolOptions<VToolEllipticalArcWithLength>(EllipticalArcWithLengthProperties());
            break;
        case VToolParallelCurve::Type:
            UpdateToolOptions<VToolParallelCurve>(ParallelCurveProperties());
            break;
        case VToolGraduatedCurve::Type:
            UpdateToolOptions<VToolGraduatedCurve>(GraduatedCurveProperties());
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
void VToolOptionsPropertyBrowser::RetranslateUI()
{
    // Re-open the current item's options so the property names/titles are re-evaluated.
    RefreshOptions();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolOptionsPropertyBrowser::userChangedData(VPE::VProperty *property)
{
    // Walk up to the nearest registered property. A collapsible group (e.g. a spline handle)
    // holds VFormulaProperty children whose own "Formula" child is the one that changed, so we
    // may need to climb more than one level before reaching a property known to ChangeToolData.
    VPE::VProperty *prop = property;
    while (prop != nullptr && !m_propertyToId.contains(prop))
    {
        prop = prop->getParent();
    }

    if (prop == nullptr)
    {
        return;
    }

    // Attachment group name edit — no tool item is selected while navigating a group.
    if (m_propertyToId.value(prop) == AttrAttachmentName)
    {
        const QString newName = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).toString();
        emit RenameRequested(m_groupAttachmentId, newName);
        return;
    }

    if (!m_currentItem)
    {
        return;
    }

    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 70, "Not all tools were used in switch.");

    switch (m_currentItem->type())
    {
        case VToolBasePoint::Type:
            ChangeToolData<VToolBasePoint>(prop, SinglePointProperties());
            break;
        case VToolArcPoint::Type:
            ChangeToolData<VToolArcPoint>(prop, ArcPointProperties());
            break;
        case VToolCurvePoint::Type:
            ChangeToolData<VToolCurvePoint>(prop, CurvePointProperties());
            break;
        case VToolEndLine::Type:
            ChangeToolData<VToolEndLine>(prop, EndLineProperties());
            break;
        case VToolAlongLine::Type:
            ChangeToolData<VToolAlongLine>(prop, AlongLineProperties());
            break;
        case VToolArc::Type:
            ChangeToolData<VToolArc>(prop, ArcProperties());
            break;
        case VToolArcWithLength::Type:
            ChangeToolData<VToolArcWithLength>(prop, ArcWithLengthProperties());
            break;
        case VToolBisector::Type:
            ChangeToolData<VToolBisector>(prop, BisectorProperties());
            break;
        case VToolCutArc::Type:
            ChangeToolData<VToolCutArc>(prop, CutArcProperties());
            break;
        case VToolCutSpline::Type:
            ChangeToolData<VToolCutSpline>(prop, CutSplineProperties());
            break;
        case VToolCutSplinePath::Type:
            ChangeToolData<VToolCutSplinePath>(prop, CutSplinePathProperties());
            break;
        case VToolHeight::Type:
            ChangeToolData<VToolHeight>(prop, HeightProperties());
            break;
        case VToolLine::Type:
            ChangeToolData<VToolLine>(prop, LineProperties());
            break;
        case VToolLineIntersect::Type:
            ChangeToolData<VToolLineIntersect>(prop, LineIntersectProperties());
            break;
        case VToolNormal::Type:
            ChangeToolData<VToolNormal>(prop, NormalProperties());
            break;
        case VToolPointOfContact::Type:
            ChangeToolData<VToolPointOfContact>(prop, PointOfContactProperties());
            break;
        case VToolPointOfIntersection::Type:
            ChangeToolData<VToolPointOfIntersection>(prop, PointOfIntersectionProperties());
            break;
        case VToolPointOfIntersectionArcs::Type:
            ChangeToolData<VToolPointOfIntersectionArcs>(prop, PointOfIntersectionArcsProperties());
            break;
        case VToolPointOfIntersectionCircles::Type:
            ChangeToolData<VToolPointOfIntersectionCircles>(prop, PointOfIntersectionCirclesProperties());
            break;
        case VToolPointOfIntersectionCurves::Type:
            ChangeToolData<VToolPointOfIntersectionCurves>(prop, PointOfIntersectionCurvesProperties());
            break;
        case VToolShoulderPoint::Type:
            ChangeToolData<VToolShoulderPoint>(prop, ShoulderPointProperties());
            break;
        case VToolSpline::Type:
            ChangeToolData<VToolSpline>(prop, SplineProperties());
            break;
        case VToolCubicBezier::Type:
            ChangeToolData<VToolCubicBezier>(prop, CubicBezierProperties());
            break;
        case VToolArmholeCurve::Type:
            ChangeToolData<VToolArmholeCurve>(prop, ArmholeCurveProperties());
            break;
        case VToolSplinePath::Type:
            ChangeToolData<VToolSplinePath>(
                prop, SplinePathProperties(qgraphicsitem_cast<VToolSplinePath *>(m_currentItem)));
            break;
        case VToolCubicBezierPath::Type:
            ChangeToolData<VToolCubicBezierPath>(prop, CubicBezierPathProperties());
            break;
        case VToolTriangle::Type:
            ChangeToolData<VToolTriangle>(prop, TriangleProperties());
            break;
        case VToolLineIntersectAxis::Type:
            ChangeToolData<VToolLineIntersectAxis>(prop, LineIntersectAxisProperties());
            break;
        case VToolCurveIntersectAxis::Type:
            ChangeToolData<VToolCurveIntersectAxis>(prop, CurveIntersectAxisProperties());
            break;
        case VToolPointFromCircleAndTangent::Type:
            ChangeToolData<VToolPointFromCircleAndTangent>(prop, PointFromCircleAndTangentProperties());
            break;
        case VToolPointFromArcAndTangent::Type:
            ChangeToolData<VToolPointFromArcAndTangent>(prop, PointFromArcAndTangentProperties());
            break;
        case VToolTrueDarts::Type:
            ChangeToolData<VToolTrueDarts>(prop, TrueDartsProperties());
            break;
        case VToolRotation::Type:
            ChangeToolData<VToolRotation>(prop, RotationProperties());
            break;
        case VToolFlippingByLine::Type:
            ChangeToolData<VToolFlippingByLine>(prop, FlippingByLineProperties());
            break;
        case VToolFlippingByAxis::Type:
            ChangeToolData<VToolFlippingByAxis>(prop, FlippingByAxisProperties());
            break;
        case VToolMove::Type:
            ChangeToolData<VToolMove>(prop, MoveProperties());
            break;
        case VToolEllipticalArc::Type:
            ChangeToolData<VToolEllipticalArc>(prop, EllipticalArcProperties());
            break;
        case VBackgroundPixmapItem::Type:
            ChangeToolData<VBackgroundPixmapItem>(prop, BackgroundPixmapItemProperties());
            break;
        case VBackgroundSVGItem::Type:
            ChangeToolData<VBackgroundSVGItem>(prop, BackgroundSVGItemProperties());
            break;
        case VToolEllipticalArcWithLength::Type:
            ChangeToolData<VToolEllipticalArcWithLength>(prop, EllipticalArcWithLengthProperties());
            break;
        case VToolParallelCurve::Type:
            ChangeToolData<VToolParallelCurve>(prop, ParallelCurveProperties());
            break;
        case VToolGraduatedCurve::Type:
            ChangeToolData<VToolGraduatedCurve>(prop, GraduatedCurveProperties());
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
        if (auto *previousPoint = dynamic_cast<VScenePoint *>(m_currentItem))
        {
            previousPoint->SetActivePoint(false);
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

    if (auto *newPoint = dynamic_cast<VScenePoint *>(m_currentItem))
    {
        newPoint->SetActivePoint(true);
    }
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
