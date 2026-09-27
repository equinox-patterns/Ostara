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
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/otoolarcpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/otoolcurvepoint.h"
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
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/otooldartcap.h"
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
                                n.isEmpty() || !VContainer::IsUnique(n, valentinaNamespace) || !rx.match(n).hasMatch())
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

template <typename Tool>
struct ParentPointDesc
{
    QString label;
    QString (Tool::*getter)() const;
};

template <typename Tool>
auto MakePointGroup(const QString &title, const QString &nameLabel, const QString &showLabel,
                    const QVector<ParentPointDesc<Tool>> &parentPoints) -> PropertyDesc
{
    return {QStringLiteral("point"), title,
            [title, nameLabel, showLabel, parentPoints](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *group = new VPE::VProperty(title);
                group->setCollapsible(true);
                auto *nameChild = new VPE::VStringProperty(nameLabel);
                nameChild->setValue(t->name());
                auto *showChild = new VPE::VBoolProperty(showLabel);
                showChild->setValue(t->IsLabelVisible(t->getId()));
                group->addChild(nameChild);
                group->addChild(showChild);
                for (const auto &p : parentPoints)
                {
                    auto *child = new VPE::VLabelProperty(p.label);
                    child->setValue((t->*p.getter)());
                    group->addChild(child);
                }
                return group;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const auto children = prop->getChildren();
                    if (children.size() >= 2) {
                        QString const n = children[0]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                        if (n != t->name()) {
                            if (QRegularExpression const rx(NameRegExp());
                                n.isEmpty() || !VContainer::IsUnique(n, valentinaNamespace) || !rx.match(n).hasMatch())
                                children[0]->setValue(t->name());
                            else t->setName(n);
                        }
                        t->SetLabelVisible(t->getId(),
                            children[1]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
                    }
                }
            },
            [parentPoints](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const auto children = prop->getChildren();
                    if (children.size() >= 2) {
                        children[0]->setValue(t->name());
                        children[1]->setValue(t->IsLabelVisible(t->getId()));
                        for (int i = 0; i < parentPoints.size(); ++i)
                        {
                            if (children.size() > 2 + i)
                            {
                                children[2 + i]->setValue((t->*parentPoints.at(i).getter)());
                            }
                        }
                    }
                }
            }};
}

template <typename Tool, typename Getter>
auto MakeParentPoint(const QString &attrId, const QString &label, Getter getter) -> PropertyDesc
{
    return {attrId, label,
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
    return {AttrNotes, label,
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
    return {AttrTypeLine, label,
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
    return {AttrLineColor, label,
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
    return {AttrLength, label,
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
    return {AttrAngle, label,
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
auto DartCapProperties() -> std::vector<PropertyDesc>;
auto CurveIntersectAxisProperties() -> std::vector<PropertyDesc>;
auto CurvePointProperties() -> std::vector<PropertyDesc>;

// ====== Tool descriptors ======

// --- OToolArcPoint (point at arc start/end) ---
auto ArcPointProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeNotes<OToolArcPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<OToolArcPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- OToolCurvePoint (point at offset curve start/end) ---
auto CurvePointProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeNotes<OToolCurvePoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<OToolCurvePoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolBasePoint (SinglePoint) ---
auto SinglePointProperties() -> std::vector<PropertyDesc>
{
    return {
        {QStringLiteral("position"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Position:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolBasePoint *>(tool);
             auto *p = new VPE::VPointFProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Position:")));
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
        MakeNotes<VToolBasePoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolBasePoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolEndLine ---
auto EndLineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolEndLine>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolEndLine::GetLineRoleName, &VToolEndLine::SetLineRoleName),
        MakeFormulaLength<VToolEndLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:"))),
        MakeFormulaAngle<VToolEndLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:"))),
        MakeLineType<VToolEndLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolEndLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolEndLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolEndLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                     {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Base point:")), &VToolEndLine::BasePointName}}),
    };
}

// --- VToolAlongLine ---
auto AlongLineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolAlongLine>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolAlongLine::GetLineRoleName, &VToolAlongLine::SetLineRoleName,
        SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolAlongLine>(QStringLiteral("secondLineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolAlongLine::GetSecondLineRoleName, &VToolAlongLine::SetSecondLineRoleName,
        SegmentDotPixmap(VColorRole::VisSupportColor4)),
        MakeFormulaLength<VToolAlongLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:"))),
        MakeLineType<VToolAlongLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolAlongLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolAlongLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolAlongLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                       {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First point:")), &VToolAlongLine::BasePointName},
                                        {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second point:")), &VToolAlongLine::SecondPointName}}),
    };
}

// --- VToolBisector ---
auto BisectorProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolBisector>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolBisector::GetLineRoleName, &VToolBisector::SetLineRoleName),
        MakeFormulaLength<VToolBisector>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:"))),
        MakeLineType<VToolBisector>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolBisector>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolBisector>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolBisector>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                      {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First point:")), &VToolBisector::FirstPointName},
                                       {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second point:")), &VToolBisector::BasePointName},
                                       {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Third point:")), &VToolBisector::ThirdPointName}}),
    };
}

// --- VToolTrueDarts ---
auto TrueDartsProperties() -> std::vector<PropertyDesc>
{
    return {
        {AttrName1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point 1 label:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolTrueDarts *>(tool);
             auto *p = new VPE::VStringProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point 1 label:")));
             p->setValue(t->nameP1()); return p;
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
        {AttrName2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point 2 label:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolTrueDarts *>(tool);
             auto *p = new VPE::VStringProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point 2 label:")));
             p->setValue(t->nameP2()); return p;
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
        MakeParentPoint<VToolTrueDarts>(AttrFirstPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First base point:")), &VToolTrueDarts::BaseLineP1Name),
        MakeParentPoint<VToolTrueDarts>(AttrSecondPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second base point:")), &VToolTrueDarts::BaseLineP2Name),
        MakeParentPoint<VToolTrueDarts>(AttrDartP1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First dart point:")), &VToolTrueDarts::DartP1Name),
        MakeParentPoint<VToolTrueDarts>(AttrDartP2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second dart point:")), &VToolTrueDarts::DartP2Name),
        MakeParentPoint<VToolTrueDarts>(AttrDartP3, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Third dart point:")), &VToolTrueDarts::DartP3Name),
        MakeNotes<VToolTrueDarts>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolHeight ---
auto HeightProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolHeight>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolHeight::GetLineRoleName, &VToolHeight::SetLineRoleName),
        MakeLineType<VToolHeight>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolHeight>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolHeight>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolHeight>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                    {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Base point:")), &VToolHeight::BasePointName},
                                     {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")), &VToolHeight::FirstLinePointName},
                                     {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")), &VToolHeight::SecondLinePointName}}),
    };
}

// --- VToolLine ---
auto LineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolLine>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolLine::GetLineRoleName, &VToolLine::SetLineRoleName),
        MakeLineType<VToolLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolLine>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolLineIntersect ---
auto LineIntersectProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeNotes<VToolLineIntersect>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolLineIntersect>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                           {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line (first point):")), &VToolLineIntersect::Line1P1Name},
                                            {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line (second point):")), &VToolLineIntersect::Line1P2Name},
                                            {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line (first point):")), &VToolLineIntersect::Line2P1Name},
                                            {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line (second point):")), &VToolLineIntersect::Line2P2Name}}),
    };
}

// --- VToolNormal ---
auto NormalProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolNormal>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolNormal::GetLineRoleName, &VToolNormal::SetLineRoleName),
        MakeFormulaLength<VToolNormal>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:"))),
        // Custom: angle is a plain number entered as text (QLineEdit), not a formula nor a spinbox
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Additional angle degrees:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolNormal *>(tool);
             auto *p = new VPE::VStringProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Additional angle degrees:")));
             p->setValue(QString::number(RoundForDisplay(t->GetAngle())));
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolNormal *>(b->currentItem())) {
                 QString text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 text.replace(QLatin1Char(','), QLatin1Char('.'));
                 bool ok = false;
                 double const v = text.toDouble(&ok);
                 if (ok && !VFuzzyComparePossibleNulls(v, t->GetAngle())) {
                     t->SetAngle(v);
                 }
                 prop->setValue(QString::number(RoundForDisplay(t->GetAngle()))); // revert to canonical value (also on invalid input)
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolNormal *>(b->currentItem())) prop->setValue(QString::number(RoundForDisplay(t->GetAngle())));
         }},
        MakeLineType<VToolNormal>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolNormal>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolNormal>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolNormal>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                    {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First point:")), &VToolNormal::BasePointName},
                                     {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second point:")), &VToolNormal::SecondPointName}}),
    };
}

// --- VToolPointOfContact ---
auto PointOfContactProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolPointOfContact>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolPointOfContact::GetLineRoleName, &VToolPointOfContact::SetLineRoleName),
        // Custom: arc radius formula (method is getArcRadius, not GetArcRadius)
        {AttrRadius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolPointOfContact *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")));
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
        MakeNotes<VToolPointOfContact>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointOfContact>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolPointOfIntersection ---
auto PointOfIntersectionProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeNotes<VToolPointOfIntersection>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolPointOfIntersection>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                                 {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "X: vertical point:")), &VToolPointOfIntersection::FirstPointName},
                                                  {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Y: horizontal point:")), &VToolPointOfIntersection::SecondPointName}}),
    };
}

// --- VToolShoulderPoint ---
auto ShoulderPointProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolShoulderPoint>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolShoulderPoint::GetLineRoleName, &VToolShoulderPoint::SetLineRoleName),
        MakeFormulaLength<VToolShoulderPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:"))),
        MakeLineType<VToolShoulderPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolShoulderPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolShoulderPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolShoulderPoint>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                           {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First point:")), &VToolShoulderPoint::BasePointName},
                                            {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second point:")), &VToolShoulderPoint::SecondPointName},
                                            {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Third point:")), &VToolShoulderPoint::ShoulderPointName}}),
    };
}

// --- VToolTriangle ---
auto TriangleProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeNotes<VToolTriangle>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolTriangle>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                      {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First point of axis:")), &VToolTriangle::AxisP1Name},
                                       {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second point of axis:")), &VToolTriangle::AxisP2Name},
                                       {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First point:")), &VToolTriangle::FirstPointName},
                                       {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second point:")), &VToolTriangle::SecondPointName}}),
    };
}

// --- VToolLineIntersectAxis ---
auto LineIntersectAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeStringPropWithValidator<VToolLineIntersectAxis>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolLineIntersectAxis::GetLineRoleName, &VToolLineIntersectAxis::SetLineRoleName),
        MakeFormulaAngle<VToolLineIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:"))),
        MakeLineType<VToolLineIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolLineIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolLineIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolLineIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
                                               {{T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis point:")), &VToolLineIntersectAxis::BasePointName},
                                                {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")), &VToolLineIntersectAxis::FirstLinePoint},
                                                {T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")), &VToolLineIntersectAxis::SecondLinePoint}}),
    };
}

//---------------------------------------------------------------------------------------------------------------------
auto DartCapProperties() -> std::vector<PropertyDesc>
{
    return {
        // Whether the cap is drawn at all: the point exists either way, the two tracés are what the box adds.
        {QStringLiteral("drawCap"),
         T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Draw the cap")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolDartCap *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Draw the cap")));
             p->setValue(t->IsCapDrawn());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolDartCap *>(b->currentItem())) {
                 t->SetCapDrawn(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
                 // The two names below only mean something when the cap is drawn: the rows go grey with it.
                 b->UpdateOptions();
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolDartCap *>(b->currentItem())) prop->setValue(t->IsCapDrawn());
         }},

        // The two tracés of the cap, in the direction they read: C_P1 in green, C_P2 in red.
        {QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolDartCap *>(tool);
             auto *p = new VPE::VStringProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")));
             p->setNameDecoration(SegmentDotPixmap(VColorRole::VisSupportColor2));
             p->setValue(t->GetLine1RoleName());
             p->setEditorDisabled(not t->IsCapDrawn());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolDartCap *>(b->currentItem())) {
                 const QString suffix = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (suffix != t->GetLine1RoleName()) t->SetLine1RoleName(suffix);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolDartCap *>(b->currentItem())) {
                 prop->setValue(t->GetLine1RoleName());
                 prop->setEditorDisabled(not t->IsCapDrawn());
             }
         }},

        {QStringLiteral("secondLineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolDartCap *>(tool);
             auto *p = new VPE::VStringProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")));
             p->setNameDecoration(SegmentDotPixmap(VColorRole::VisSupportColor4));
             p->setValue(t->GetLine2RoleName());
             p->setEditorDisabled(not t->IsCapDrawn());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolDartCap *>(b->currentItem())) {
                 const QString suffix = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (suffix != t->GetLine2RoleName()) t->SetLine2RoleName(suffix);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolDartCap *>(b->currentItem())) {
                 prop->setValue(t->GetLine2RoleName());
                 prop->setEditorDisabled(not t->IsCapDrawn());
             }
         }},

        MakeLineType<OToolDartCap>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Type of line:"))),
        MakeLineColor<OToolDartCap>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<OToolDartCap>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<OToolDartCap>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")),
                                     T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")),
                                     T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolCurveIntersectAxis ---
auto CurveIntersectAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCurveIntersectAxis>(AttrAlias1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolCurveIntersectAxis::GetAliasSuffix1, &VToolCurveIntersectAxis::SetAliasSuffix1,
                                                             SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCurveIntersectAxis>(AttrAlias2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolCurveIntersectAxis::GetAliasSuffix2, &VToolCurveIntersectAxis::SetAliasSuffix2,
                                                             SegmentDotPixmap(VColorRole::VisSupportColor4)),
        MakeStringPropWithValidator<VToolCurveIntersectAxis>(QStringLiteral("lineRoleName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line:")),
        &VToolCurveIntersectAxis::GetLineRoleName, &VToolCurveIntersectAxis::SetLineRoleName),
        MakeFormulaAngle<VToolCurveIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:"))),
        MakeLineType<VToolCurveIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line type:"))),
        MakeLineColor<VToolCurveIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Line color:"))),
        MakeNotes<VToolCurveIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolCurveIntersectAxis>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}
