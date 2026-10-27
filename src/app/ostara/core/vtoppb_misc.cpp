/************************************************************************
 **
 **  @file   vtoppb_misc.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  misc property browser methods — declarative descriptors
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
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncircles.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectionarcs.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromcircleandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromarcandtangent.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vpropertyexplorer/plugins/vboolproperty.h"
#include "../vpropertyexplorer/plugins/vnumberproperty.h"
#include "../vpropertyexplorer/plugins/vcolorpropertyeditor.h"
#include "../vtheme/vscenestylesheet.h"
#include "ifcdef.h"
#include "../vmisc/def.h"

namespace
{
auto T(const char *s) -> QString
{
    return VToolOptionsPropertyBrowser::tr(s);
}

auto SegmentDotPixmap(VColorRole role) -> QPixmap
{
    return VPE::VColorPropertyEditor::GetColorPixmap(VSceneStylesheet::Color(role), 16);
}

// --- Reusable helpers ---
template <typename Tool>
auto MakePointGroup(const QString &title, const QString &nameLabel, const QString &showLabel) -> PropertyDesc
{
    return {QStringLiteral("point"), title,
            [title, nameLabel, showLabel](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *group = new VPE::VProperty(title);
                group->setCollapsible(true);
                auto *nameChild = new VPE::VStringProperty(nameLabel);
                nameChild->setValue(t->name());
                auto *showChild = new VPE::VBoolProperty(showLabel);
                showChild->setValue(t->IsLabelVisible(t->getId()));
                group->addChild(nameChild);
                group->addChild(showChild);
                return group;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const auto children = prop->getChildren();
                    if (children.size() >= 2) {
                        QString const n = children[0]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                        if (n != t->name()) {
                            if (QRegularExpression const rx(NameRegExp());
                                n.isEmpty() || !VContainer::IsUnique(n, ostaraNamespace) || !rx.match(n).hasMatch())
                                children[0]->setValue(t->name());
                            else t->setName(n);
                        }
                        t->SetLabelVisible(t->getId(),
                            children[1]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
                    }
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const auto children = prop->getChildren();
                    if (children.size() >= 2) {
                        children[0]->setValue(t->name());
                        children[1]->setValue(t->IsLabelVisible(t->getId()));
                    }
                }
            }};
}

template <typename Tool, typename Getter>
auto MakeParentPoint(const QString &attrId, const QString &label, Getter getter) -> PropertyDesc
{
    return {attrId, label,
            [getter, label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VLabelProperty(label);
                prop->setValue((t->*getter)());
                return prop;
            },
            nullptr,
            [getter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue((t->*getter)());
            }};
}

template <typename Tool>
auto MakeNotes(const QString &label) -> PropertyDesc
{
    return {AttrNotes, label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VTextProperty(label);
                prop->setValue(t->GetNotes());
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (text != t->GetNotes()) t->SetNotes(text);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetNotes());
            }};
}

template <typename Tool>
auto MakeCrossPoint(const QString &label) -> PropertyDesc
{
    return {AttrCrossPoint, label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VEnumProperty(label);
                prop->setLiterals(QStringList({VToolOptionsPropertyBrowser::tr("First point"),
                                               VToolOptionsPropertyBrowser::tr("Second point")}));
                prop->setValue(static_cast<int>(t->GetCrossCirclesPoint()) - 1);
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const QVariant value = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false;
                    const int val = value.toInt(&ok);
                    if (ok) t->SetCrossCirclesPoint(static_cast<CrossCirclesPoint>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetCrossCirclesPoint()) - 1);
            }};
}

template <typename Tool>
auto MakeVCrossPoint(const QString &label) -> PropertyDesc
{
    return {AttrVCrossPoint, label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VEnumProperty(label);
                prop->setLiterals(QStringList({VToolOptionsPropertyBrowser::tr("Highest point"),
                                               VToolOptionsPropertyBrowser::tr("Lowest point")}));
                prop->setValue(static_cast<int>(t->GetVCrossPoint()) - 1);
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const QVariant value = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false;
                    const int val = value.toInt(&ok);
                    if (ok) t->SetVCrossPoint(static_cast<VCrossCurvesPoint>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetVCrossPoint()) - 1);
            }};
}

template <typename Tool>
auto MakeHCrossPoint(const QString &label) -> PropertyDesc
{
    return {AttrHCrossPoint, label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VEnumProperty(label);
                prop->setLiterals(QStringList({VToolOptionsPropertyBrowser::tr("Leftmost point"),
                                               VToolOptionsPropertyBrowser::tr("Rightmost point")}));
                prop->setValue(static_cast<int>(t->GetHCrossPoint()) - 1);
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const QVariant value = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false;
                    const int val = value.toInt(&ok);
                    if (ok) t->SetHCrossPoint(static_cast<HCrossCurvesPoint>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetHCrossPoint()) - 1);
            }};
}

template <typename Tool>
auto MakeObjectName(const QString &label) -> PropertyDesc
{
    return {AttrName, label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VStringProperty(label);
                prop->setValue(t->name());
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const name = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (name != t->name()) t->setName(name);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->name());
            }};
}

} // anonymous namespace

// Forward declarations
auto PointOfIntersectionArcsProperties() -> std::vector<PropertyDesc>;
auto PointOfIntersectionCirclesProperties() -> std::vector<PropertyDesc>;
auto PointOfIntersectionCurvesProperties() -> std::vector<PropertyDesc>;
auto PointFromCircleAndTangentProperties() -> std::vector<PropertyDesc>;
auto PointFromArcAndTangentProperties() -> std::vector<PropertyDesc>;
auto BackgroundPixmapItemProperties() -> std::vector<PropertyDesc>;
auto BackgroundSVGItemProperties() -> std::vector<PropertyDesc>;

// --- VToolPointOfIntersectionArcs ---
auto PointOfIntersectionArcsProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeCrossPoint<VToolPointOfIntersectionArcs>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Take:"))),
        MakeStringPropWithValidator<VToolPointOfIntersectionArcs>(AttrCurve1Alias1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")), &VToolPointOfIntersectionArcs::GetArc1AliasSuffix1, &VToolPointOfIntersectionArcs::SetArc1AliasSuffix1,
                                                                 SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolPointOfIntersectionArcs>(AttrCurve1Alias2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")), &VToolPointOfIntersectionArcs::GetArc1AliasSuffix2, &VToolPointOfIntersectionArcs::SetArc1AliasSuffix2,
                                                                 SegmentDotPixmap(VColorRole::VisSupportColor4)),
        MakeStringPropWithValidator<VToolPointOfIntersectionArcs>(AttrCurve2Alias1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")), &VToolPointOfIntersectionArcs::GetArc2AliasSuffix1, &VToolPointOfIntersectionArcs::SetArc2AliasSuffix1,
                                                                 SegmentDotPixmap(VColorRole::VisSupportColor)),
        MakeStringPropWithValidator<VToolPointOfIntersectionArcs>(AttrCurve2Alias2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")), &VToolPointOfIntersectionArcs::GetArc2AliasSuffix2, &VToolPointOfIntersectionArcs::SetArc2AliasSuffix2,
                                                                 SegmentDotPixmap(VColorRole::VisSupportColor3)),
        MakeNotes<VToolPointOfIntersectionArcs>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointOfIntersectionArcs>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolPointOfIntersectionCircles ---
auto PointOfIntersectionCirclesProperties() -> std::vector<PropertyDesc>
{
    return {
        // First circle radius (custom formula)
        {AttrC1Radius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First circle radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointOfIntersectionCircles *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First circle radius:")));
             prop->SetFormula(t->GetFirstCircleRadius());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFirstCircleRadius()) t->SetFirstCircleRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFirstCircleRadius()); prop->setValue(v);
             }
         }},
        // Second circle radius (custom formula)
        {AttrC2Radius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second circle radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointOfIntersectionCircles *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second circle radius:")));
             prop->SetFormula(t->GetSecondCircleRadius());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetSecondCircleRadius()) t->SetSecondCircleRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointOfIntersectionCircles *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetSecondCircleRadius()); prop->setValue(v);
             }
         }},
        MakeCrossPoint<VToolPointOfIntersectionCircles>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Take:"))),
        MakeNotes<VToolPointOfIntersectionCircles>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointOfIntersectionCircles>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolPointOfIntersectionCurves ---
auto PointOfIntersectionCurvesProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeVCrossPoint<VToolPointOfIntersectionCurves>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Vertical correction:"))),
        MakeHCrossPoint<VToolPointOfIntersectionCurves>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Horizontal correction:"))),
        MakeStringPropWithValidator<VToolPointOfIntersectionCurves>(AttrCurve1Alias1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolPointOfIntersectionCurves::GetCurve1AliasSuffix1, &VToolPointOfIntersectionCurves::SetCurve1AliasSuffix1,
                                                                   SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolPointOfIntersectionCurves>(AttrCurve1Alias2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolPointOfIntersectionCurves::GetCurve1AliasSuffix2, &VToolPointOfIntersectionCurves::SetCurve1AliasSuffix2,
                                                                   SegmentDotPixmap(VColorRole::VisSupportColor4)),
        MakeStringPropWithValidator<VToolPointOfIntersectionCurves>(AttrCurve2Alias1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolPointOfIntersectionCurves::GetCurve2AliasSuffix1, &VToolPointOfIntersectionCurves::SetCurve2AliasSuffix1,
                                                                   SegmentDotPixmap(VColorRole::VisSupportColor)),
        MakeStringPropWithValidator<VToolPointOfIntersectionCurves>(AttrCurve2Alias2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolPointOfIntersectionCurves::GetCurve2AliasSuffix2, &VToolPointOfIntersectionCurves::SetCurve2AliasSuffix2,
                                                                   SegmentDotPixmap(VColorRole::VisSupportColor3)),
        MakeNotes<VToolPointOfIntersectionCurves>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointOfIntersectionCurves>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolPointFromCircleAndTangent ---
auto PointFromCircleAndTangentProperties() -> std::vector<PropertyDesc>
{
    return {
        // Circle radius (custom formula)
        {AttrCRadius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Circle radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointFromCircleAndTangent *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Circle radius:")));
             prop->SetFormula(t->GetCircleRadius());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointFromCircleAndTangent *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetCircleRadius()) t->SetCircleRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointFromCircleAndTangent *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetCircleRadius()); prop->setValue(v);
             }
         }},
        MakeCrossPoint<VToolPointFromCircleAndTangent>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Take:"))),
        MakeNotes<VToolPointFromCircleAndTangent>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointFromCircleAndTangent>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolPointFromArcAndTangent ---
auto PointFromArcAndTangentProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeCrossPoint<VToolPointFromArcAndTangent>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Take:"))),
        MakeNotes<VToolPointFromArcAndTangent>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointFromArcAndTangent>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VBackgroundPixmapItem ---
auto BackgroundPixmapItemProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectName<VBackgroundPixmapItem>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        {VtoppbConstants::AttrHold, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Hold:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundPixmapItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Hold:")));
             prop->setValue(t->IsHold());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundPixmapItem *>(b->currentItem())) {
                 bool const hold = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
                 if (hold != t->IsHold()) t->SetHold(hold);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundPixmapItem *>(b->currentItem()))
                 prop->setValue(t->IsHold());
         }},
        {VtoppbConstants::AttrVisible, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Visible:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundPixmapItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Visible:")));
             prop->setValue(t->IsVisible());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundPixmapItem *>(b->currentItem())) {
                 bool const visible = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
                 if (visible != t->IsVisible()) t->SetVisible(visible);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundPixmapItem *>(b->currentItem()))
                 prop->setValue(t->IsVisible());
         }},
        {VtoppbConstants::AttrOpacity, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Opacity:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundPixmapItem *>(tool);
             QMap<QString, QVariant> const settings{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), 100},
                                                     {QStringLiteral("Step"), 1}, {QStringLiteral("Suffix"), QChar('%')}};
             auto *prop = new VPE::VIntegerProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Opacity:")), settings);
             prop->setValue(qRound(t->GetOpacity() * 100));
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundPixmapItem *>(b->currentItem())) {
                 const int opacity = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toInt();
                 if (!VFuzzyComparePossibleNulls(opacity, t->GetOpacity()))
                     t->SetOpacity(opacity / 100.);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundPixmapItem *>(b->currentItem()))
                 prop->setValue(qRound(t->GetOpacity() * 100));
         }},
    };
}

// --- VBackgroundSVGItem ---
auto BackgroundSVGItemProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectName<VBackgroundSVGItem>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        {VtoppbConstants::AttrHold, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Hold:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundSVGItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Hold:")));
             prop->setValue(t->IsHold());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundSVGItem *>(b->currentItem())) {
                 bool const hold = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
                 if (hold != t->IsHold()) t->SetHold(hold);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundSVGItem *>(b->currentItem()))
                 prop->setValue(t->IsHold());
         }},
        {VtoppbConstants::AttrVisible, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Visible:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundSVGItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Visible:")));
             prop->setValue(t->IsVisible());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundSVGItem *>(b->currentItem())) {
                 bool const visible = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
                 if (visible != t->IsVisible()) t->SetVisible(visible);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundSVGItem *>(b->currentItem()))
                 prop->setValue(t->IsVisible());
         }},
        {VtoppbConstants::AttrOpacity, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Opacity:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundSVGItem *>(tool);
             QMap<QString, QVariant> const settings{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), 100},
                                                     {QStringLiteral("Step"), 1}, {QStringLiteral("Suffix"), QChar('%')}};
             auto *prop = new VPE::VIntegerProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Opacity:")), settings);
             prop->setValue(qRound(t->GetOpacity() * 100));
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundSVGItem *>(b->currentItem())) {
                 const int opacity = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toInt();
                 if (!VFuzzyComparePossibleNulls(opacity, t->GetOpacity()))
                     t->SetOpacity(opacity / 100.);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VBackgroundSVGItem *>(b->currentItem()))
                 prop->setValue(qRound(t->GetOpacity() * 100));
         }},
    };
}
