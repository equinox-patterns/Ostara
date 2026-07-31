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
#include "ifcdef.h"
#include "../vmisc/def.h"

namespace
{
auto T(const char *s) -> QString
{
    return VToolOptionsPropertyBrowser::tr(s);
}

// --- Reusable helpers ---

template <typename Tool>
auto MakePointName(const QString &label) -> PropertyDesc
{
    return {AttrName, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VStringProperty(label);
                prop->setClearButtonEnable(true);
                prop->setValue(t->name());
                return prop;
            },
            // apply: validated SetPointName logic
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const name = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (name == t->name()) return;

                    if (QRegularExpression const rx(NameRegExp());
                        name.isEmpty() || not VContainer::IsUnique(name, valentinaNamespace) || not rx.match(name).hasMatch())
                    {
                        prop->setValue(t->name());
                    }
                    else
                    {
                        t->setName(name);
                    }
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->name());
            }};
}

template <typename Tool, typename Getter>
auto MakeParentPoint(const QString &attrId, const QString &label, Getter getter) -> PropertyDesc
{
    return {attrId, label, true,
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
    return {AttrNotes, label, false,
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
    return {AttrCrossPoint, label, false,
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
    return {AttrVCrossPoint, label, false,
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
    return {AttrHCrossPoint, label, false,
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

template <typename Tool, typename Getter, typename Setter>
auto MakeStringProp(const QString &attrId, const QString &label, Getter getter, Setter setter) -> PropertyDesc
{
    return {attrId, label, false,
            [getter, label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VStringProperty(label);
                prop->setClearButtonEnable(true);
                prop->setValue((t->*getter)());
                return prop;
            },
            [getter, setter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const val = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (val != (t->*getter)()) (t->*setter)(val);
                }
            },
            [getter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue((t->*getter)());
            }};
}

template <typename Tool>
auto MakeObjectName(const QString &label) -> PropertyDesc
{
    return {AttrName, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VStringProperty(label);
                prop->setClearButtonEnable(true);
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
        MakePointName<VToolPointOfIntersectionArcs>(T("Point label:")),
        MakeParentPoint<VToolPointOfIntersectionArcs>(AttrFirstArc, T("First arc:"), &VToolPointOfIntersectionArcs::FirstArcName),
        MakeParentPoint<VToolPointOfIntersectionArcs>(AttrSecondArc, T("Second arc:"), &VToolPointOfIntersectionArcs::SecondArcName),
        MakeCrossPoint<VToolPointOfIntersectionArcs>(T("Take:")),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve1Name1, T("First arc. Left sub arc name:"), &VToolPointOfIntersectionArcs::GetArc1Name1, &VToolPointOfIntersectionArcs::SetArc1Name1),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve1Name2, T("First arc. Right sub arc name:"), &VToolPointOfIntersectionArcs::GetArc1Name2, &VToolPointOfIntersectionArcs::SetArc1Name2),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve1Alias1, T("First arc. Left sub arc alias:"), &VToolPointOfIntersectionArcs::GetArc1AliasSuffix1, &VToolPointOfIntersectionArcs::SetArc1AliasSuffix1),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve1Alias2, T("First arc. Right sub arc alias:"), &VToolPointOfIntersectionArcs::GetArc1AliasSuffix2, &VToolPointOfIntersectionArcs::SetArc1AliasSuffix2),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve2Name1, T("Second arc. Left sub arc name:"), &VToolPointOfIntersectionArcs::GetArc2Name1, &VToolPointOfIntersectionArcs::SetArc2Name1),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve2Name2, T("Second arc. Right sub arc name:"), &VToolPointOfIntersectionArcs::GetArc2Name2, &VToolPointOfIntersectionArcs::SetArc2Name2),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve2Alias1, T("Second arc. Left sub arc alias:"), &VToolPointOfIntersectionArcs::GetArc2AliasSuffix1, &VToolPointOfIntersectionArcs::SetArc2AliasSuffix1),
        MakeStringProp<VToolPointOfIntersectionArcs>(AttrCurve2Alias2, T("Second arc. Right sub arc alias:"), &VToolPointOfIntersectionArcs::GetArc2AliasSuffix2, &VToolPointOfIntersectionArcs::SetArc2AliasSuffix2),
        MakeNotes<VToolPointOfIntersectionArcs>(T("Notes:")),
    };
}

// --- VToolPointOfIntersectionCircles ---
auto PointOfIntersectionCirclesProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolPointOfIntersectionCircles>(T("Point label:")),
        MakeParentPoint<VToolPointOfIntersectionCircles>(AttrC1Center, T("Center of the first circle:"), &VToolPointOfIntersectionCircles::FirstCircleCenterPointName),
        MakeParentPoint<VToolPointOfIntersectionCircles>(AttrC2Center, T("Center of the second circle:"), &VToolPointOfIntersectionCircles::SecondCircleCenterPointName),
        // First circle radius (custom formula)
        {AttrC1Radius, T("First circle radius:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointOfIntersectionCircles *>(tool);
             auto *prop = new VFormulaProperty(T("First circle radius:"));
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
        {AttrC2Radius, T("Second circle radius:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointOfIntersectionCircles *>(tool);
             auto *prop = new VFormulaProperty(T("Second circle radius:"));
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
        MakeCrossPoint<VToolPointOfIntersectionCircles>(T("Take:")),
        MakeNotes<VToolPointOfIntersectionCircles>(T("Notes:")),
    };
}

// --- VToolPointOfIntersectionCurves ---
auto PointOfIntersectionCurvesProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolPointOfIntersectionCurves>(T("Point label:")),
        MakeParentPoint<VToolPointOfIntersectionCurves>(AttrCurve1, T("First curve:"), &VToolPointOfIntersectionCurves::FirstCurveName),
        MakeParentPoint<VToolPointOfIntersectionCurves>(AttrCurve2, T("Second curve:"), &VToolPointOfIntersectionCurves::SecondCurveName),
        MakeVCrossPoint<VToolPointOfIntersectionCurves>(T("Vertical correction:")),
        MakeHCrossPoint<VToolPointOfIntersectionCurves>(T("Horizontal correction:")),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve1Name1, T("First curve. Left sub curve name:"), &VToolPointOfIntersectionCurves::GetCurve1Name1, &VToolPointOfIntersectionCurves::SetCurve1Name1),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve1Name2, T("First curve. Right sub curve name:"), &VToolPointOfIntersectionCurves::GetCurve1Name2, &VToolPointOfIntersectionCurves::SetCurve1Name2),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve1Alias1, T("First curve. Left sub curve alias:"), &VToolPointOfIntersectionCurves::GetCurve1AliasSuffix1, &VToolPointOfIntersectionCurves::SetCurve1AliasSuffix1),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve1Alias2, T("First curve. Right sub curve alias:"), &VToolPointOfIntersectionCurves::GetCurve1AliasSuffix2, &VToolPointOfIntersectionCurves::SetCurve1AliasSuffix2),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve2Name1, T("Second curve. Left sub curve name:"), &VToolPointOfIntersectionCurves::GetCurve2Name1, &VToolPointOfIntersectionCurves::SetCurve2Name1),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve2Name2, T("Second curve. Right sub curve name:"), &VToolPointOfIntersectionCurves::GetCurve2Name2, &VToolPointOfIntersectionCurves::SetCurve2Name2),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve2Alias1, T("Second curve. Left sub curve alias:"), &VToolPointOfIntersectionCurves::GetCurve2AliasSuffix1, &VToolPointOfIntersectionCurves::SetCurve2AliasSuffix1),
        MakeStringProp<VToolPointOfIntersectionCurves>(AttrCurve2Alias2, T("Second curve. Right sub curve alias:"), &VToolPointOfIntersectionCurves::GetCurve2AliasSuffix2, &VToolPointOfIntersectionCurves::SetCurve2AliasSuffix2),
        MakeNotes<VToolPointOfIntersectionCurves>(T("Notes:")),
    };
}

// --- VToolPointFromCircleAndTangent ---
auto PointFromCircleAndTangentProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolPointFromCircleAndTangent>(T("Point label:")),
        MakeParentPoint<VToolPointFromCircleAndTangent>(AttrCCenter, T("Center of the circle:"), &VToolPointFromCircleAndTangent::CircleCenterPointName),
        MakeParentPoint<VToolPointFromCircleAndTangent>(AttrTangent, T("Tangent point:"), &VToolPointFromCircleAndTangent::TangentPointName),
        // Circle radius (custom formula)
        {AttrCRadius, T("Circle radius:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointFromCircleAndTangent *>(tool);
             auto *prop = new VFormulaProperty(T("Circle radius:"));
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
        MakeCrossPoint<VToolPointFromCircleAndTangent>(T("Take:")),
        MakeNotes<VToolPointFromCircleAndTangent>(T("Notes:")),
    };
}

// --- VToolPointFromArcAndTangent ---
auto PointFromArcAndTangentProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolPointFromArcAndTangent>(T("Point label:")),
        MakeParentPoint<VToolPointFromArcAndTangent>(AttrTangent, T("Tangent point:"), &VToolPointFromArcAndTangent::TangentPointName),
        MakeParentPoint<VToolPointFromArcAndTangent>(AttrArc, T("Arc:"), &VToolPointFromArcAndTangent::ArcName),
        MakeCrossPoint<VToolPointFromArcAndTangent>(T("Take:")),
        MakeNotes<VToolPointFromArcAndTangent>(T("Notes:")),
    };
}

// --- VBackgroundPixmapItem ---
auto BackgroundPixmapItemProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectName<VBackgroundPixmapItem>(T("Name:")),
        {VtoppbConstants::AttrHold, T("Hold:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundPixmapItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T("Hold:"));
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
        {VtoppbConstants::AttrVisible, T("Visible:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundPixmapItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T("Visible:"));
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
        {VtoppbConstants::AttrOpacity, T("Opacity:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundPixmapItem *>(tool);
             QMap<QString, QVariant> const settings{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), 100},
                                                     {QStringLiteral("Step"), 1}, {QStringLiteral("Suffix"), QChar('%')}};
             auto *prop = new VPE::VIntegerProperty(T("Opacity:"), settings);
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
        MakeObjectName<VBackgroundSVGItem>(T("Name:")),
        {VtoppbConstants::AttrHold, T("Hold:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundSVGItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T("Hold:"));
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
        {VtoppbConstants::AttrVisible, T("Visible:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundSVGItem *>(tool);
             auto *prop = new VPE::VBoolProperty(T("Visible:"));
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
        {VtoppbConstants::AttrOpacity, T("Opacity:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VBackgroundSVGItem *>(tool);
             QMap<QString, QVariant> const settings{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), 100},
                                                     {QStringLiteral("Step"), 1}, {QStringLiteral("Suffix"), QChar('%')}};
             auto *prop = new VPE::VIntegerProperty(T("Opacity:"), settings);
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
