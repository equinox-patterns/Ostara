/************************************************************************
 **
 **  @file   vtoppb_points.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  points property browser methods — declarative descriptors
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
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoollineintersect.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolshoulderpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolnormal.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollineintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolheight.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolendline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolbisector.h"
#include "../vpropertyexplorer/plugins/vcolorpropertyeditor.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolalongline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooltruedarts.h"
#include "../vpropertyexplorer/plugins/vstringproperty.h"
#include "../vpropertyexplorer/plugins/vpointfproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/vlinetypeproperty.h"
#include "../vpropertyexplorer/plugins/vlinecolorproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vboolproperty.h"
#include "../vpropertyexplorer/plugins/vnumberproperty.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "ifcdef.h"
#include "../vmisc/def.h"

namespace
{
auto T(const char *s) -> QString { return VToolOptionsPropertyBrowser::tr(s); }

// ---- Reusable helpers ----

template <typename Tool>
auto MakePointName(const QString &label) -> PropertyDesc
{
    return {AttrName, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VStringProperty(label);
                p->setClearButtonEnable(true);
                p->setValue(t->name());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QString const n = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (n == t->name()) return;
                    if (QRegularExpression const rx(NameRegExp());
                        n.isEmpty() || !VContainer::IsUnique(n, valentinaNamespace) || !rx.match(n).hasMatch())
                        prop->setValue(t->name());
                    else t->setName(n);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) prop->setValue(t->name());
            }};
}

template <typename Tool>
auto MakeShowPointName(const QString &label) -> PropertyDesc
{
    return {AttrShowLabel, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VBoolProperty(label);
                p->setValue(t->IsLabelVisible(t->getId()));
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    t->SetLabelVisible(t->getId(), prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    prop->setValue(t->IsLabelVisible(t->getId()));
                }
            }};
}

template <typename Tool, typename Getter>
auto MakeParentPoint(const QString &attrId, const QString &label, Getter getter) -> PropertyDesc
{
    return {attrId, label, true,
            [getter, label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VLabelProperty(label);
                p->setValue((t->*getter)());
                return p;
            }, nullptr,
            [getter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) prop->setValue((t->*getter)());
            }};
}

template <typename Tool>
auto MakeNotes(const QString &label) -> PropertyDesc
{
    return {AttrNotes, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VTextProperty(label);
                p->setValue(t->GetNotes());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QString const s = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (s != t->GetNotes()) t->SetNotes(s);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) prop->setValue(t->GetNotes());
            }};
}

template <typename Tool>
auto MakeLineType(const QString &label) -> PropertyDesc
{
    return {AttrTypeLine, label, false,
            [label](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                QPalette const cp = b->ComboBoxPalette();
                auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                auto *p = new VPE::VLineTypeProperty(label);
                p->setStyles(styles);
                auto idx = VPE::VLineTypeProperty::IndexOfStyle(styles, t->getLineType());
                p->setValue(idx >= 0 ? idx : 0);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QString const s = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (s != t->getLineType()) t->SetLineType(s);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QPalette const cp = b->ComboBoxPalette();
                    auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                    auto idx = VPE::VLineTypeProperty::IndexOfStyle(styles, t->getLineType());
                    prop->setValue(idx >= 0 ? idx : 0);
                }
            }};
}

template <typename Tool>
auto MakeLineColor(const QString &label) -> PropertyDesc
{
    return {AttrLineColor, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VLineColorProperty(label);
                p->SetDefaultColors(VAbstractTool::ColorsList());
                p->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
                p->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
                p->setValue(t->GetLineColor());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QString const c = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (c != t->GetLineColor()) t->SetLineColor(c);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) prop->setValue(t->GetLineColor());
            }};
}

template <typename Tool>
auto MakeFormulaLength(const QString &label) -> PropertyDesc
{
    return {AttrLength, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VFormulaProperty(label);
                p->SetFormula(t->GetFormulaLength());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                    if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
                }
            }};
}

template <typename Tool>
auto MakeFormulaAngle(const QString &label) -> PropertyDesc
{
    return {AttrAngle, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VFormulaProperty(label);
                p->SetFormula(t->GetFormulaAngle());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                    if (f != t->GetFormulaAngle()) t->SetFormulaAngle(f);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QVariant v; v.setValue(t->GetFormulaAngle()); prop->setValue(v);
                }
            }};
}

template <typename Tool>
auto MakeCrossPoint(const QString &label) -> PropertyDesc
{
    return {AttrCrossPoint, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VEnumProperty(label);
                p->setLiterals(QStringList({T("First point"), T("Second point")}));
                p->setValue(static_cast<int>(t->GetCrossCirclesPoint()) - 1);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const QVariant v = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false; int val = v.toInt(&ok);
                    if (ok) t->SetCrossCirclesPoint(static_cast<CrossCirclesPoint>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetCrossCirclesPoint()) - 1);
            }};
}

template <typename Tool>
auto MakeAxisType(const QString &label) -> PropertyDesc
{
    return {AttrAxisType, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VEnumProperty(label);
                p->setLiterals(QStringList({T("Vertical axis"), T("Horizontal axis")}));
                p->setValue(static_cast<int>(t->GetAxisType()) - 1);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const QVariant v = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false; int val = v.toInt(&ok);
                    if (ok && val >= 0 && val <= 1) t->SetAxisType(static_cast<AxisType>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetAxisType()) - 1);
            }};
}

template <typename Tool>
auto MakeVCrossPoint(const QString &label) -> PropertyDesc
{
    return {AttrVCrossPoint, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VEnumProperty(label);
                p->setLiterals(QStringList({T("Highest point"), T("Lowest point")}));
                p->setValue(static_cast<int>(t->GetVCrossPoint()) - 1);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const QVariant v = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false; int val = v.toInt(&ok);
                    if (ok) t->SetVCrossPoint(static_cast<VCrossCurvesPoint>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetVCrossPoint()) - 1);
            }};
}

template <typename Tool>
auto MakeHCrossPoint(const QString &label) -> PropertyDesc
{
    return {AttrHCrossPoint, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VEnumProperty(label);
                p->setLiterals(QStringList({T("Leftmost point"), T("Rightmost point")}));
                p->setValue(static_cast<int>(t->GetHCrossPoint()) - 1);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const QVariant v = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                    bool ok = false; int val = v.toInt(&ok);
                    if (ok) t->SetHCrossPoint(static_cast<HCrossCurvesPoint>(val + 1));
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(static_cast<int>(t->GetHCrossPoint()) - 1);
            }};
}


static auto SegmentDotPixmap(VColorRole role) -> QPixmap
{
    return VPE::VColorPropertyEditor::GetColorPixmap(VSceneStylesheet::Color(role), 16);
}

} // anonymous namespace

// Forward declarations (one per tool)
auto SinglePointProperties() -> std::vector<PropertyDesc>;
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

// ====== Tool descriptors ======

// --- VToolBasePoint (SinglePoint) ---
auto SinglePointProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolBasePoint>(T("Point name:")),
        MakeShowPointName<VToolBasePoint>(T("Show Point name")),
        {QStringLiteral("position"), T("Position:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolBasePoint *>(tool);
             auto *p = new VPE::VPointFProperty(T("Position:"));
             p->setValue(t->GetBasePointPos());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolBasePoint *>(b->currentItem()))
                 t->SetBasePointPos(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toPointF());
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolBasePoint *>(b->currentItem()))
                 prop->setValue(t->GetBasePointPos());
         }},
        MakeNotes<VToolBasePoint>(T("Notes:")),
    };
}

// --- VToolEndLine ---
auto EndLineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolEndLine>(T("Point name:")),
        MakeShowPointName<VToolEndLine>(T("Show Point name")),
        MakeParentPoint<VToolEndLine>(AttrBasePoint, T("Base point:"), &VToolEndLine::BasePointName),
        MakeLineType<VToolEndLine>(T("Line type:")),
        MakeLineColor<VToolEndLine>(T("Line color:")),
        MakeStringPropWithValidator<VToolEndLine>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolEndLine::GetLineRoleName, &VToolEndLine::SetLineRoleName),
        MakeFormulaLength<VToolEndLine>(T("Length:")),
        MakeFormulaAngle<VToolEndLine>(T("Angle:")),
        MakeNotes<VToolEndLine>(T("Notes:")),
    };
}

// --- VToolAlongLine ---
auto AlongLineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolAlongLine>(T("Point name:")),
        MakeShowPointName<VToolAlongLine>(T("Show Point name")),
        MakeParentPoint<VToolAlongLine>(AttrBasePoint, T("First point:"), &VToolAlongLine::BasePointName),
        MakeParentPoint<VToolAlongLine>(AttrSecondPoint, T("Second point:"), &VToolAlongLine::SecondPointName),
        MakeLineType<VToolAlongLine>(T("Line type:")),
        MakeLineColor<VToolAlongLine>(T("Line color:")),
        MakeStringPropWithValidator<VToolAlongLine>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolAlongLine::GetLineRoleName, &VToolAlongLine::SetLineRoleName,
        SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolAlongLine>(QStringLiteral("secondLineRoleName"), T("Name 2:"),
        &VToolAlongLine::GetSecondLineRoleName, &VToolAlongLine::SetSecondLineRoleName,
        SegmentDotPixmap(VColorRole::VisSupportColor4)),
        MakeFormulaLength<VToolAlongLine>(T("Length:")),
        MakeNotes<VToolAlongLine>(T("Notes:")),
    };
}

// --- VToolBisector ---
auto BisectorProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolBisector>(T("Point name:")),
        MakeShowPointName<VToolBisector>(T("Show Point name")),
        MakeParentPoint<VToolBisector>(AttrFirstPoint, T("First point:"), &VToolBisector::FirstPointName),
        MakeParentPoint<VToolBisector>(AttrBasePoint, T("Second point:"), &VToolBisector::BasePointName),
        MakeParentPoint<VToolBisector>(AttrThirdPoint, T("Third point:"), &VToolBisector::ThirdPointName),
        MakeLineType<VToolBisector>(T("Line type:")),
        MakeLineColor<VToolBisector>(T("Line color:")),
        MakeStringPropWithValidator<VToolBisector>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolBisector::GetLineRoleName, &VToolBisector::SetLineRoleName),
        MakeFormulaLength<VToolBisector>(T("Length:")),
        MakeNotes<VToolBisector>(T("Notes:")),
    };
}

// --- VToolTrueDarts ---
auto TrueDartsProperties() -> std::vector<PropertyDesc>
{
    return {
        {AttrName1, T("Point 1 label:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolTrueDarts *>(tool);
             auto *p = new VPE::VStringProperty(T("Point 1 label:"));
             p->setClearButtonEnable(true); p->setValue(t->nameP1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolTrueDarts *>(b->currentItem())) {
                 QString const n = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (n == t->nameP1()) return;
                 if (QRegularExpression const rx(NameRegExp());
                     n.isEmpty() || !VContainer::IsUnique(n, valentinaNamespace) || !rx.match(n).hasMatch())
                     prop->setValue(t->nameP1());
                 else t->setNameP1(n);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolTrueDarts *>(b->currentItem())) prop->setValue(t->nameP1());
         }},
        {AttrName2, T("Point 2 label:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolTrueDarts *>(tool);
             auto *p = new VPE::VStringProperty(T("Point 2 label:"));
             p->setClearButtonEnable(true); p->setValue(t->nameP2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolTrueDarts *>(b->currentItem())) {
                 QString const n = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (n == t->nameP2()) return;
                 if (QRegularExpression const rx(NameRegExp());
                     n.isEmpty() || !VContainer::IsUnique(n, valentinaNamespace) || !rx.match(n).hasMatch())
                     prop->setValue(t->nameP2());
                 else t->setNameP2(n);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolTrueDarts *>(b->currentItem())) prop->setValue(t->nameP2());
         }},
        MakeParentPoint<VToolTrueDarts>(AttrFirstPoint, T("First base point:"), &VToolTrueDarts::BaseLineP1Name),
        MakeParentPoint<VToolTrueDarts>(AttrSecondPoint, T("Second base point:"), &VToolTrueDarts::BaseLineP2Name),
        MakeParentPoint<VToolTrueDarts>(AttrDartP1, T("First dart point:"), &VToolTrueDarts::DartP1Name),
        MakeParentPoint<VToolTrueDarts>(AttrDartP2, T("Second dart point:"), &VToolTrueDarts::DartP2Name),
        MakeParentPoint<VToolTrueDarts>(AttrDartP3, T("Third dart point:"), &VToolTrueDarts::DartP3Name),
        MakeNotes<VToolTrueDarts>(T("Notes:")),
    };
}

// --- VToolHeight ---
auto HeightProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolHeight>(T("Point name:")),
        MakeShowPointName<VToolHeight>(T("Show Point name")),
        MakeParentPoint<VToolHeight>(AttrBasePoint, T("Base point:"), &VToolHeight::BasePointName),
        MakeParentPoint<VToolHeight>(AttrP1Line, T("First line point:"), &VToolHeight::FirstLinePointName),
        MakeParentPoint<VToolHeight>(AttrP2Line, T("Second line point:"), &VToolHeight::SecondLinePointName),
        MakeLineType<VToolHeight>(T("Line type:")),
        MakeLineColor<VToolHeight>(T("Line color:")),
        MakeStringPropWithValidator<VToolHeight>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolHeight::GetLineRoleName, &VToolHeight::SetLineRoleName),
        MakeNotes<VToolHeight>(T("Notes:")),
    };
}

// --- VToolLine ---
auto LineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeLineType<VToolLine>(T("Line type:")),
        MakeLineColor<VToolLine>(T("Line color:")),
        MakeStringPropWithValidator<VToolLine>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolLine::GetLineRoleName, &VToolLine::SetLineRoleName),
        MakeParentPoint<VToolLine>(AttrFirstPoint, T("First point:"), &VToolLine::FirstPointName),
        MakeParentPoint<VToolLine>(AttrSecondPoint, T("Second point:"), &VToolLine::SecondPointName),
        MakeNotes<VToolLine>(T("Notes:")),
    };
}

// --- VToolLineIntersect ---
auto LineIntersectProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolLineIntersect>(T("Point name:")),
        MakeShowPointName<VToolLineIntersect>(T("Show Point name")),
        MakeParentPoint<VToolLineIntersect>(AttrP1Line1, T("First line (first point):"), &VToolLineIntersect::Line1P1Name),
        MakeParentPoint<VToolLineIntersect>(AttrP2Line1, T("First line (second point):"), &VToolLineIntersect::Line1P2Name),
        MakeParentPoint<VToolLineIntersect>(AttrP1Line2, T("Second line (first point):"), &VToolLineIntersect::Line2P1Name),
        MakeParentPoint<VToolLineIntersect>(AttrP2Line2, T("Second line (second point):"), &VToolLineIntersect::Line2P2Name),
        MakeStringPropWithValidator<VToolLineIntersect>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolLineIntersect::GetLineRoleName, &VToolLineIntersect::SetLineRoleName),
        MakeNotes<VToolLineIntersect>(T("Notes:")),
    };
}

// --- VToolNormal ---
auto NormalProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolNormal>(T("Point name:")),
        MakeShowPointName<VToolNormal>(T("Show Point name")),
        MakeParentPoint<VToolNormal>(AttrBasePoint, T("First point:"), &VToolNormal::BasePointName),
        MakeParentPoint<VToolNormal>(AttrSecondPoint, T("Second point:"), &VToolNormal::SecondPointName),
        MakeLineType<VToolNormal>(T("Line type:")),
        MakeLineColor<VToolNormal>(T("Line color:")),
        MakeStringPropWithValidator<VToolNormal>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolNormal::GetLineRoleName, &VToolNormal::SetLineRoleName),
        MakeFormulaLength<VToolNormal>(T("Length:")),
        // Custom: angle is a plain double, not a formula
        {AttrAngle, T("Additional angle degrees:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolNormal *>(tool);
             QMap<QString, QVariant> s{{QStringLiteral("Min"), -360}, {QStringLiteral("Max"), 360}, {QStringLiteral("Precision"), 3}};
             auto *p = new VPE::VDoubleProperty(T("Additional angle degrees:"), s);
             p->setValue(t->GetAngle());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolNormal *>(b->currentItem())) {
                 double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                 if (!VFuzzyComparePossibleNulls(v, t->GetAngle())) t->SetAngle(v);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolNormal *>(b->currentItem())) prop->setValue(t->GetAngle());
         }},
        MakeNotes<VToolNormal>(T("Notes:")),
    };
}

// --- VToolPointOfContact ---
auto PointOfContactProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolPointOfContact>(T("Point name:")),
        MakeShowPointName<VToolPointOfContact>(T("Show Point name")),
        MakeParentPoint<VToolPointOfContact>(AttrCenter, T("Center of arc:"), &VToolPointOfContact::ArcCenterPointName),
        MakeParentPoint<VToolPointOfContact>(AttrFirstPoint, T("Top of the line:"), &VToolPointOfContact::FirstPointName),
        MakeParentPoint<VToolPointOfContact>(AttrSecondPoint, T("End of the line:"), &VToolPointOfContact::SecondPointName),
        // Custom: arc radius formula (method is getArcRadius, not GetArcRadius)
        {AttrRadius, T("Radius:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointOfContact *>(tool);
             auto *p = new VFormulaProperty(T("Radius:"));
             p->SetFormula(t->getArcRadius());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointOfContact *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->getArcRadius()) t->setArcRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolPointOfContact *>(b->currentItem())) {
                 QVariant v; v.setValue(t->getArcRadius()); prop->setValue(v);
             }
         }},
        MakeStringPropWithValidator<VToolPointOfContact>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolPointOfContact::GetLineRoleName, &VToolPointOfContact::SetLineRoleName),
        MakeNotes<VToolPointOfContact>(T("Notes:")),
    };
}

// --- VToolPointOfIntersection ---
auto PointOfIntersectionProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolPointOfIntersection>(T("Point name:")),
        MakeShowPointName<VToolPointOfIntersection>(T("Show Point name")),
        MakeParentPoint<VToolPointOfIntersection>(AttrFirstPoint, T("X: vertical point:"), &VToolPointOfIntersection::FirstPointName),
        MakeParentPoint<VToolPointOfIntersection>(AttrSecondPoint, T("Y: horizontal point:"), &VToolPointOfIntersection::SecondPointName),
        MakeNotes<VToolPointOfIntersection>(T("Notes:")),
    };
}

// --- VToolShoulderPoint ---
auto ShoulderPointProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolShoulderPoint>(T("Point name:")),
        MakeShowPointName<VToolShoulderPoint>(T("Show Point name")),
        MakeParentPoint<VToolShoulderPoint>(AttrBasePoint, T("First point:"), &VToolShoulderPoint::BasePointName),
        MakeParentPoint<VToolShoulderPoint>(AttrSecondPoint, T("Second point:"), &VToolShoulderPoint::SecondPointName),
        MakeParentPoint<VToolShoulderPoint>(AttrThirdPoint, T("Third point:"), &VToolShoulderPoint::ShoulderPointName),
        MakeLineType<VToolShoulderPoint>(T("Line type:")),
        MakeLineColor<VToolShoulderPoint>(T("Line color:")),
        MakeStringPropWithValidator<VToolShoulderPoint>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolShoulderPoint::GetLineRoleName, &VToolShoulderPoint::SetLineRoleName),
        MakeFormulaLength<VToolShoulderPoint>(T("Length:")),
        MakeNotes<VToolShoulderPoint>(T("Notes:")),
    };
}

// --- VToolTriangle ---
auto TriangleProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolTriangle>(T("Point name:")),
        MakeShowPointName<VToolTriangle>(T("Show Point name")),
        MakeParentPoint<VToolTriangle>(AttrAxisP1, T("First point of axis:"), &VToolTriangle::AxisP1Name),
        MakeParentPoint<VToolTriangle>(AttrAxisP2, T("Second point of axis:"), &VToolTriangle::AxisP2Name),
        MakeParentPoint<VToolTriangle>(AttrFirstPoint, T("First point:"), &VToolTriangle::FirstPointName),
        MakeParentPoint<VToolTriangle>(AttrSecondPoint, T("Second point:"), &VToolTriangle::SecondPointName),
        MakeNotes<VToolTriangle>(T("Notes:")),
    };
}

// --- VToolLineIntersectAxis ---
auto LineIntersectAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolLineIntersectAxis>(T("Point name:")),
        MakeShowPointName<VToolLineIntersectAxis>(T("Show Point name")),
        {AttrBasePoint, T("Axis point:"), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolLineIntersectAxis *>(tool);
             auto *p = new VPE::VLabelProperty(T("Axis point:"));
             p->setValue(t->BasePointName()); return p;
         }, nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolLineIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->BasePointName());
         }},
        {AttrFirstPoint, T("First line point:"), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolLineIntersectAxis *>(tool);
             auto *p = new VPE::VLabelProperty(T("First line point:"));
             p->setValue(t->FirstLinePoint()); return p;
         }, nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolLineIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->FirstLinePoint());
         }},
        {AttrSecondPoint, T("Second line point:"), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolLineIntersectAxis *>(tool);
             auto *p = new VPE::VLabelProperty(T("Second line point:"));
             p->setValue(t->SecondLinePoint()); return p;
         }, nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolLineIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->SecondLinePoint());
         }},
        MakeLineType<VToolLineIntersectAxis>(T("Line type:")),
        MakeLineColor<VToolLineIntersectAxis>(T("Line color:")),
        MakeStringPropWithValidator<VToolLineIntersectAxis>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolLineIntersectAxis::GetLineRoleName, &VToolLineIntersectAxis::SetLineRoleName),
        MakeFormulaAngle<VToolLineIntersectAxis>(T("Angle:")),
        MakeNotes<VToolLineIntersectAxis>(T("Notes:")),
    };
}

// --- VToolCurveIntersectAxis ---
auto CurveIntersectAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        MakePointName<VToolCurveIntersectAxis>(T("Point name:")),
        MakeShowPointName<VToolCurveIntersectAxis>(T("Show Point name")),
        {AttrBasePoint, T("Axis point:"), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCurveIntersectAxis *>(tool);
             auto *p = new VPE::VLabelProperty(T("Axis point:"));
             p->setValue(t->BasePointName()); return p;
         }, nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->BasePointName());
         }},
        {AttrCurve, T("Curve:"), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCurveIntersectAxis *>(tool);
             auto *p = new VPE::VLabelProperty(T("Curve:"));
             p->setValue(t->CurveName()); return p;
         }, nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->CurveName());
         }},
        {AttrCurveName1, T("Name1:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCurveIntersectAxis *>(tool);
             auto *p = new VPE::VStringProperty(T("Name1:"));
             p->setClearButtonEnable(true); p->setValue(t->GetName1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem())) {
                 QString const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (v != t->GetName1()) t->SetName1(v);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->GetName1());
         }},
        {AttrCurveName2, T("Name2:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCurveIntersectAxis *>(tool);
             auto *p = new VPE::VStringProperty(T("Name2:"));
             p->setClearButtonEnable(true); p->setValue(t->GetName2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem())) {
                 QString const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (v != t->GetName2()) t->SetName2(v);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->GetName2());
         }},
        {AttrAlias1, T("Alias1:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCurveIntersectAxis *>(tool);
             auto *p = new VPE::VStringProperty(T("Alias1:"));
             p->setClearButtonEnable(true); p->setValue(t->GetAliasSuffix1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem())) {
                 QString const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (v != t->GetAliasSuffix1()) t->SetAliasSuffix1(v);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->GetAliasSuffix1());
         }},
        {AttrAlias2, T("Alias2:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCurveIntersectAxis *>(tool);
             auto *p = new VPE::VStringProperty(T("Alias2:"));
             p->setClearButtonEnable(true); p->setValue(t->GetAliasSuffix2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem())) {
                 QString const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (v != t->GetAliasSuffix2()) t->SetAliasSuffix2(v);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCurveIntersectAxis *>(b->currentItem()))
                 prop->setValue(t->GetAliasSuffix2());
         }},
        MakeLineType<VToolCurveIntersectAxis>(T("Line type:")),
        MakeLineColor<VToolCurveIntersectAxis>(T("Line color:")),
        MakeStringPropWithValidator<VToolCurveIntersectAxis>(QStringLiteral("lineRoleName"), T("Name:"),
        &VToolCurveIntersectAxis::GetLineRoleName, &VToolCurveIntersectAxis::SetLineRoleName),
        MakeFormulaAngle<VToolCurveIntersectAxis>(T("Angle:")),
        MakeNotes<VToolCurveIntersectAxis>(T("Notes:")),
    };
}
