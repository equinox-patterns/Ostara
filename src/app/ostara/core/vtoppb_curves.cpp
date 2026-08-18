/************************************************************************
 **
 **  @file   vtoppb_curves.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  curves property browser methods — declarative descriptors
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
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vpropertyexplorer/plugins/vstringproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/vlinetypeproperty.h"
#include "../vpropertyexplorer/plugins/vlinecolorproperty.h"
#include "../vpropertyexplorer/plugins/vnumberproperty.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "ifcdef.h"
#include "../vmisc/def.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vpropertyexplorer/plugins/vcolorpropertyeditor.h"
#include "../vgeometry/vsplinepath.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vcubicbezier.h"

namespace
{
auto T(const char *s) -> QString { return VToolOptionsPropertyBrowser::tr(s); }

auto SegmentDotPixmap(VColorRole role) -> QPixmap
{
    return VPE::VColorPropertyEditor::GetColorPixmap(VSceneStylesheet::Color(role), 16);
}

/**
 * @brief CurveDisplayName simplified name shown by the property browser.
 *
 * Ostara drives the user away from point-derived technical names: Spl_ and SplPath_ both
 * collapse to Curve_, arcs keep Arc_ and ElArc_. When the user has not named the object yet
 * the suffix reads (Unnamed), which is the nudge to name it.
 *
 * @param objectName a VGObject::ObjectName() result, either "Spl_A_B" or "Spl_MyName (Spl_A_B)".
 */
auto CurveDisplayName(const QString &objectName) -> QString
{
    const qsizetype headEnd = objectName.indexOf(QLatin1Char('_'));
    if (headEnd < 0)
    {
        return objectName;
    }

    const QString head = objectName.left(headEnd + 1);

    QString prefix;
    if (head == QLatin1String(SPL_) || head == splPath_V + QLatin1Char('_'))
    {
        prefix = QStringLiteral("Curve_");
    }
    else if (head == QLatin1String(ARC_) || head == QLatin1String(ELARC_))
    {
        prefix = head;
    }
    else
    {
        return objectName; // not a curve or an arc, leave it alone
    }

    // The alias carries the user suffix: "Spl_MyName (Spl_A_B)" -> "MyName".
    QString suffix;
    if (const qsizetype aliasEnd = objectName.indexOf(QLatin1String(" (")); aliasEnd > 0)
    {
        if (const QString alias = objectName.left(aliasEnd); alias.startsWith(head))
        {
            suffix = alias.mid(head.length());
        }
    }

    return prefix + (suffix.isEmpty() ? T("(Unnamed)") : suffix);
}

// --- Helpers ---

template <typename Tool>
auto MakeObjectNameReadOnly(const QString &label) -> PropertyDesc
{
    return {AttrName, label, true,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VLabelProperty(label);
                p->setValue(CurveDisplayName(t->name()));
                return p;
            }, nullptr,
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(CurveDisplayName(t->name()));
            }};
}

template <typename Tool>
auto MakeObjectName(const QString &label) -> PropertyDesc
{
    return {AttrName, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VStringProperty(label);
                p->setValue(t->name());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QString const n = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (n != t->name()) t->setName(n);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->name());
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
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue((t->*getter)());
            }};
}

// Same as MakeParentPoint, but the parent is a curve or an arc, so its name goes through the
// simplified display naming. Kept apart on purpose: MakeParentPoint also serves real points
// (Center point:, Bezier handles), whose names must stay untouched.
template <typename Tool, typename Getter>
auto MakeSourceCurve(const QString &attrId, const QString &label, Getter getter) -> PropertyDesc
{
    return {attrId, label, true,
            [getter, label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VLabelProperty(label);
                p->setValue(CurveDisplayName((t->*getter)()));
                return p;
            }, nullptr,
            [getter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(CurveDisplayName((t->*getter)()));
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
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetNotes());
            }};
}

template <typename Tool>
auto MakeCurvePenStyle(const QString &label) -> PropertyDesc
{
    return {AttrPenStyle, label, false,
            [label](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                QPalette const cp = b->ComboBoxPalette();
                auto styles = CurvePenStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                auto *p = new VPE::VLineTypeProperty(label);
                p->setStyles(styles);
                auto idx = VPE::VLineTypeProperty::IndexOfStyle(styles, t->GetPenStyle());
                p->setValue(idx >= 0 ? idx : 0);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QString const s = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (s != t->GetPenStyle()) t->SetPenStyle(s);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    QPalette const cp = b->ComboBoxPalette();
                    auto styles = CurvePenStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                    auto idx = VPE::VLineTypeProperty::IndexOfStyle(styles, t->GetPenStyle());
                    prop->setValue(idx >= 0 ? idx : 0);
                }
            }};
}

template <typename Tool>
auto MakeLineColor(const QString &label) -> PropertyDesc
{
    return {AttrColor, label, false,
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
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetLineColor());
            }};
}

template <typename Tool>
auto MakeApproximationScale(const QString &label) -> PropertyDesc
{
    return {AttrAScale, label, false,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                QMap<QString, QVariant> s{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), maxCurveApproximationScale},
                                           {QStringLiteral("Step"), 0.1}, {QStringLiteral("Precision"), 1}};
                auto *p = new VPE::VDoubleProperty(label, s);
                p->setValue(t->GetApproximationScale());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                    if (!VFuzzyComparePossibleNulls(v, t->GetApproximationScale()))
                        t->SetApproximationScale(v);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetApproximationScale());
            }};
}

} // anonymous namespace

// Forward declarations
auto ArcProperties() -> std::vector<PropertyDesc>;
auto ArcWithLengthProperties() -> std::vector<PropertyDesc>;
auto CutArcProperties() -> std::vector<PropertyDesc>;
auto CutSplineProperties() -> std::vector<PropertyDesc>;
auto CutSplinePathProperties() -> std::vector<PropertyDesc>;
auto SplineProperties() -> std::vector<PropertyDesc>;
auto CubicBezierProperties() -> std::vector<PropertyDesc>;
auto ArmholeCurveProperties() -> std::vector<PropertyDesc>;
auto SplinePathProperties() -> std::vector<PropertyDesc>;
auto CubicBezierPathProperties() -> std::vector<PropertyDesc>;
auto EllipticalArcProperties() -> std::vector<PropertyDesc>;
auto EllipticalArcWithLengthProperties() -> std::vector<PropertyDesc>;
auto ParallelCurveProperties() -> std::vector<PropertyDesc>;
auto GraduatedCurveProperties() -> std::vector<PropertyDesc>;

// ====== Tool descriptors ======

// --- VToolArc ---
auto ArcProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolArc>(T("Name:")),
        MakeParentPoint<VToolArc>(AttrCenter, T("Center point:"), &VToolArc::CenterPointName),
        {AttrRadius, T("Radius:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VFormulaProperty(T("Radius:"));
             p->SetFormula(t->GetFormulaRadius()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius()) t->SetFormulaRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius()); prop->setValue(v);
             }
         }},
        {AttrAngle1, T("First angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VFormulaProperty(T("First angle:"));
             p->SetFormula(t->GetFormulaF1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaF1()) t->SetFormulaF1(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaF1()); prop->setValue(v);
             }
         }},
        {AttrAngle2, T("Second angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VFormulaProperty(T("Second angle:"));
             p->SetFormula(t->GetFormulaF2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaF2()) t->SetFormulaF2(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaF2()); prop->setValue(v);
             }
         }},
        MakeStringPropWithValidator<VToolArc>(AttrAlias, T("Arc:"), &VToolArc::GetAliasSuffix, &VToolArc::SetAliasSuffix),
        MakeCurvePenStyle<VToolArc>(T("Pen style:")),
        MakeLineColor<VToolArc>(T("Color:")),
        MakeApproximationScale<VToolArc>(T("Approximation scale:")),
        {QStringLiteral("createStartPoint"), T("Create start point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create start point"));
             p->setValue(t->HasStartPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 t->SetCreateStartPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 prop->setValue(t->HasStartPoint());
             }
         }},
        {QStringLiteral("createEndPoint"), T("Create end point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create end point"));
             p->setValue(t->HasEndPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 t->SetCreateEndPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArc *>(b->currentItem())) {
                 prop->setValue(t->HasEndPoint());
             }
         }},
        MakeNotes<VToolArc>(T("Notes:")),
    };
}

// --- VToolArcWithLength ---
auto ArcWithLengthProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolArcWithLength>(T("Name:")),
        MakeParentPoint<VToolArcWithLength>(AttrCenter, T("Center point:"), &VToolArcWithLength::CenterPointName),
        {AttrRadius, T("Radius:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("Radius:"));
             p->SetFormula(t->GetFormulaRadius()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius()) t->SetFormulaRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius()); prop->setValue(v);
             }
         }},
        {AttrAngle1, T("First angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("First angle:"));
             p->SetFormula(t->GetFormulaF1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaF1()) t->SetFormulaF1(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaF1()); prop->setValue(v);
             }
         }},
        {AttrLength, T("Length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("Length:"));
             p->SetFormula(t->GetFormulaLength()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
             }
         }},
        MakeStringPropWithValidator<VToolArcWithLength>(AttrAlias, T("Arc:"), &VToolArcWithLength::GetAliasSuffix, &VToolArcWithLength::SetAliasSuffix),
        MakeCurvePenStyle<VToolArcWithLength>(T("Pen style:")),
        MakeLineColor<VToolArcWithLength>(T("Color:")),
        MakeApproximationScale<VToolArcWithLength>(T("Approximation scale:")),
        {QStringLiteral("createStartPoint"), T("Create start point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create start point"));
             p->setValue(t->HasStartPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 t->SetCreateStartPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 prop->setValue(t->HasStartPoint());
             }
         }},
        {QStringLiteral("createEndPoint"), T("Create end point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create end point"));
             p->setValue(t->HasEndPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 t->SetCreateEndPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArcWithLength *>(b->currentItem())) {
                 prop->setValue(t->HasEndPoint());
             }
         }},
        MakeNotes<VToolArcWithLength>(T("Notes:")),
    };
}

// --- VToolCutArc ---
auto CutArcProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectName<VToolCutArc>(T("Point name:")),
        MakeSourceCurve<VToolCutArc>(AttrArc, T("Arc:"), &VToolCutArc::CurveName),
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCutArc>(AttrAlias1, T("Arc:"), &VToolCutArc::GetAliasSuffix1, &VToolCutArc::SetAliasSuffix1,
                                             SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCutArc>(AttrAlias2, T("Arc:"), &VToolCutArc::GetAliasSuffix2, &VToolCutArc::SetAliasSuffix2,
                                             SegmentDotPixmap(VColorRole::VisSupportColor4)),
        {AttrLength, T("Length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCutArc *>(tool);
             auto *p = new VFormulaProperty(T("Length:"));
             p->SetFormula(t->GetFormulaLength()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCutArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCutArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
             }
         }},
        MakeNotes<VToolCutArc>(T("Notes:")),
    };
}

// --- VToolCutSpline ---
auto CutSplineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectName<VToolCutSpline>(T("Point name:")),
        MakeSourceCurve<VToolCutSpline>(AttrCurve, T("Curve:"), &VToolCutSpline::CurveName),
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCutSpline>(AttrAlias1, T("Curve:"), &VToolCutSpline::GetAliasSuffix1, &VToolCutSpline::SetAliasSuffix1,
                                                SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCutSpline>(AttrAlias2, T("Curve:"), &VToolCutSpline::GetAliasSuffix2, &VToolCutSpline::SetAliasSuffix2,
                                                SegmentDotPixmap(VColorRole::VisSupportColor4)),
        {AttrLength, T("Length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCutSpline *>(tool);
             auto *p = new VFormulaProperty(T("Length:"));
             p->SetFormula(t->GetFormulaLength()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCutSpline *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCutSpline *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
             }
         }},
        MakeNotes<VToolCutSpline>(T("Notes:")),
    };
}

// --- VToolCutSplinePath ---
auto CutSplinePathProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectName<VToolCutSplinePath>(T("Point name:")),
        MakeSourceCurve<VToolCutSplinePath>(AttrCurve, T("Curve:"), &VToolCutSplinePath::CurveName),
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCutSplinePath>(AttrAlias1, T("Curve:"), &VToolCutSplinePath::GetAliasSuffix1, &VToolCutSplinePath::SetAliasSuffix1,
                                                   SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCutSplinePath>(AttrAlias2, T("Curve:"), &VToolCutSplinePath::GetAliasSuffix2, &VToolCutSplinePath::SetAliasSuffix2,
                                                   SegmentDotPixmap(VColorRole::VisSupportColor4)),
        {AttrLength, T("Length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCutSplinePath *>(tool);
             auto *p = new VFormulaProperty(T("Length:"));
             p->SetFormula(t->GetFormulaLength()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCutSplinePath *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCutSplinePath *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
             }
         }},
        MakeNotes<VToolCutSplinePath>(T("Notes:")),
    };
}

// --- VToolSpline ---
auto SplineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolSpline>(T("Name:")),
        MakeStringPropWithValidator<VToolSpline>(AttrAlias, T("Curve:"), &VToolSpline::GetAliasSuffix, &VToolSpline::SetAliasSuffix),
        // C1 angle — custom VFormula construction
        {AttrAngle1, T("C1: angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolSpline *>(tool);
             const auto spl = t->getSpline();
             VFormula angle1(spl.GetStartAngleFormula(), t->getData());
             angle1.setToolId(t->getId());
             angle1.setPostfix(degreeSymbol);
             angle1.Eval();
             auto *p = new VFormulaProperty(T("C1: angle:"));
             p->SetFormula(angle1); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (!f.error()) {
                     VSpline spl = t->getSpline();
                     spl.SetStartAngle(f.getDoubleValue(), f.GetFormula(FormulaType::FromUser));
                     t->setSpline(spl);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 const VSpline spl = t->getSpline();
                 VFormula angleF(spl.GetStartAngleFormula(), t->getData());
                 angleF.setToolId(t->getId());
                 angleF.setPostfix(degreeSymbol);
                 angleF.Eval();
                 QVariant v; v.setValue(angleF); prop->setValue(v);
             }
         }},
        // C1 length
        {AttrLength1, T("C1: length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolSpline *>(tool);
             const auto spl = t->getSpline();
             VFormula length1(spl.GetC1LengthFormula(), t->getData());
             length1.setToolId(t->getId());
             length1.Eval();
             auto *p = new VFormulaProperty(T("C1: length:"));
             p->SetFormula(length1); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (!f.error()) {
                     VSpline spl = t->getSpline();
                     spl.SetC1Length(f.getDoubleValue(), f.GetFormula(FormulaType::FromUser));
                     t->setSpline(spl);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 const VSpline spl = t->getSpline();
                 VFormula lenF(spl.GetC1LengthFormula(), t->getData());
                 lenF.setToolId(t->getId());
                 lenF.Eval();
                 QVariant v; v.setValue(lenF); prop->setValue(v);
             }
         }},
        // C2 angle
        {AttrAngle2, T("C2: angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolSpline *>(tool);
             const auto spl = t->getSpline();
             VFormula angle2(spl.GetEndAngleFormula(), t->getData());
             angle2.setToolId(t->getId());
             angle2.setPostfix(degreeSymbol);
             angle2.Eval();
             auto *p = new VFormulaProperty(T("C2: angle:"));
             p->SetFormula(angle2); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (!f.error()) {
                     VSpline spl = t->getSpline();
                     spl.SetEndAngle(f.getDoubleValue(), f.GetFormula(FormulaType::FromUser));
                     t->setSpline(spl);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 const VSpline spl = t->getSpline();
                 VFormula angleF(spl.GetEndAngleFormula(), t->getData());
                 angleF.setToolId(t->getId());
                 angleF.setPostfix(degreeSymbol);
                 angleF.Eval();
                 QVariant v; v.setValue(angleF); prop->setValue(v);
             }
         }},
        // C2 length
        {AttrLength2, T("C2: length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolSpline *>(tool);
             const auto spl = t->getSpline();
             VFormula length2(spl.GetC2LengthFormula(), t->getData());
             length2.setToolId(t->getId());
             length2.Eval();
             auto *p = new VFormulaProperty(T("C2: length:"));
             p->SetFormula(length2); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (!f.error()) {
                     VSpline spl = t->getSpline();
                     spl.SetC2Length(f.getDoubleValue(), f.GetFormula(FormulaType::FromUser));
                     t->setSpline(spl);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 const VSpline spl = t->getSpline();
                 VFormula lenF(spl.GetC2LengthFormula(), t->getData());
                 lenF.setToolId(t->getId());
                 lenF.Eval();
                 QVariant v; v.setValue(lenF); prop->setValue(v);
             }
         }},
        MakeCurvePenStyle<VToolSpline>(T("Pen style:")),
        MakeLineColor<VToolSpline>(T("Color:")),
        {AttrAScale, T("Approximation scale:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolSpline *>(tool);
             QMap<QString, QVariant> s{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), maxCurveApproximationScale},
                                        {QStringLiteral("Step"), 0.1}, {QStringLiteral("Precision"), 1}};
             auto *p = new VPE::VDoubleProperty(T("Approximation scale:"), s);
             p->setValue(t->getSpline().GetApproximationScale());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem())) {
                 double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                 VSpline spl = t->getSpline();
                 if (!VFuzzyComparePossibleNulls(v, spl.GetApproximationScale())) {
                     spl.SetApproximationScale(v);
                     t->setSpline(spl);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSpline *>(b->currentItem()))
                 prop->setValue(t->getSpline().GetApproximationScale());
         }},
        MakeNotes<VToolSpline>(T("Notes:")),
    };
}

// --- VToolCubicBezier ---
auto CubicBezierProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolCubicBezier>(T("Name:")),
        MakeParentPoint<VToolCubicBezier>(AttrPoint1, T("First point:"), &VToolCubicBezier::FirstPointName),
        MakeParentPoint<VToolCubicBezier>(AttrPoint2, T("Second point:"), &VToolCubicBezier::SecondPointName),
        MakeParentPoint<VToolCubicBezier>(AttrPoint3, T("Third point:"), &VToolCubicBezier::ThirdPointName),
        MakeParentPoint<VToolCubicBezier>(AttrPoint4, T("Fourth point:"), &VToolCubicBezier::ForthPointName),
        MakeStringPropWithValidator<VToolCubicBezier>(AttrAlias, T("Curve:"), &VToolCubicBezier::GetAliasSuffix, &VToolCubicBezier::SetAliasSuffix),
        MakeCurvePenStyle<VToolCubicBezier>(T("Pen style:")),
        MakeLineColor<VToolCubicBezier>(T("Color:")),
        {AttrAScale, T("Approximation scale:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCubicBezier *>(tool);
             QMap<QString, QVariant> s{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), maxCurveApproximationScale},
                                        {QStringLiteral("Step"), 0.1}, {QStringLiteral("Precision"), 1}};
             auto *p = new VPE::VDoubleProperty(T("Approximation scale:"), s);
             p->setValue(t->getSpline().GetApproximationScale());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCubicBezier *>(b->currentItem())) {
                 double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                 VCubicBezier spl = t->getSpline();
                 if (!VFuzzyComparePossibleNulls(v, spl.GetApproximationScale())) {
                     spl.SetApproximationScale(v);
                     t->setSpline(spl);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCubicBezier *>(b->currentItem()))
                 prop->setValue(t->getSpline().GetApproximationScale());
         }},
        MakeNotes<VToolCubicBezier>(T("Notes:")),
    };
}

// --- VToolArmholeCurve ---
auto ArmholeCurveProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolArmholeCurve>(T("Name:")),
        MakeParentPoint<VToolArmholeCurve>(AttrPoint1, T("Armhole:"), &VToolArmholeCurve::FirstPointName),
        MakeParentPoint<VToolArmholeCurve>(AttrPoint4, T("Chest/back width:"), &VToolArmholeCurve::SecondPointName),
        MakeParentPoint<VToolArmholeCurve>(AttrPoint6, T("Perpendicular:"), &VToolArmholeCurve::ThirdPointName),
        MakeParentPoint<VToolArmholeCurve>(AttrPoint7, T("Shoulder:"), &VToolArmholeCurve::FourthPointName),
        MakeStringPropWithValidator<VToolArmholeCurve>(AttrAlias, T("Curve:"), &VToolArmholeCurve::GetAliasSuffix, &VToolArmholeCurve::SetAliasSuffix),
        MakeCurvePenStyle<VToolArmholeCurve>(T("Pen style:")),
        MakeLineColor<VToolArmholeCurve>(T("Color:")),
        {AttrAScale, T("Approximation scale:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArmholeCurve *>(tool);
             QMap<QString, QVariant> s{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), maxCurveApproximationScale},
                                        {QStringLiteral("Step"), 0.1}, {QStringLiteral("Precision"), 1}};
             auto *p = new VPE::VDoubleProperty(T("Approximation scale:"), s);
             p->setValue(t->GetPath().GetApproximationScale());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArmholeCurve *>(b->currentItem())) {
                 double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                 VCubicBezierPath path = t->GetPath();
                 if (!VFuzzyComparePossibleNulls(v, path.GetApproximationScale())) {
                     path.SetApproximationScale(v);
                     t->SetPath(path);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolArmholeCurve *>(b->currentItem()))
                 prop->setValue(t->GetPath().GetApproximationScale());
         }},
        MakeNotes<VToolArmholeCurve>(T("Notes:")),
    };
}

// --- VToolSplinePath ---
auto SplinePathProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolSplinePath>(T("Name:")),
        MakeStringPropWithValidator<VToolSplinePath>(AttrAlias, T("Curve:"), &VToolSplinePath::GetAliasSuffix, &VToolSplinePath::SetAliasSuffix),
        MakeCurvePenStyle<VToolSplinePath>(T("Pen style:")),
        MakeLineColor<VToolSplinePath>(T("Color:")),
        {AttrAScale, T("Approximation scale:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolSplinePath *>(tool);
             QMap<QString, QVariant> s{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), maxCurveApproximationScale},
                                        {QStringLiteral("Step"), 0.1}, {QStringLiteral("Precision"), 1}};
             auto *p = new VPE::VDoubleProperty(T("Approximation scale:"), s);
             p->setValue(t->getSplinePath().GetApproximationScale());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSplinePath *>(b->currentItem())) {
                 double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                 if (!VFuzzyComparePossibleNulls(v, t->getSplinePath().GetApproximationScale())) {
                     VSplinePath sp = t->getSplinePath();
                     sp.SetApproximationScale(v);
                     t->setSplinePath(sp);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolSplinePath *>(b->currentItem()))
                 prop->setValue(t->getSplinePath().GetApproximationScale());
         }},
        MakeNotes<VToolSplinePath>(T("Notes:")),
    };
}

// --- VToolCubicBezierPath ---
auto CubicBezierPathProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolCubicBezierPath>(T("Name:")),
        MakeStringPropWithValidator<VToolCubicBezierPath>(AttrAlias, T("Curve:"), &VToolCubicBezierPath::GetAliasSuffix, &VToolCubicBezierPath::SetAliasSuffix),
        MakeCurvePenStyle<VToolCubicBezierPath>(T("Pen style:")),
        MakeLineColor<VToolCubicBezierPath>(T("Color:")),
        {AttrAScale, T("Approximation scale:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolCubicBezierPath *>(tool);
             QMap<QString, QVariant> s{{QStringLiteral("Min"), 0}, {QStringLiteral("Max"), maxCurveApproximationScale},
                                        {QStringLiteral("Step"), 0.1}, {QStringLiteral("Precision"), 1}};
             auto *p = new VPE::VDoubleProperty(T("Approximation scale:"), s);
             p->setValue(t->getSplinePath().GetApproximationScale());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCubicBezierPath *>(b->currentItem())) {
                 double const v = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
                 VCubicBezierPath sp = t->getSplinePath();
                 if (!VFuzzyComparePossibleNulls(v, sp.GetApproximationScale())) {
                     sp.SetApproximationScale(v);
                     t->setSplinePath(sp);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolCubicBezierPath *>(b->currentItem()))
                 prop->setValue(t->getSplinePath().GetApproximationScale());
         }},
        MakeNotes<VToolCubicBezierPath>(T("Notes:")),
    };
}

// --- VToolEllipticalArc ---
auto EllipticalArcProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolEllipticalArc>(T("Name:")),
        MakeParentPoint<VToolEllipticalArc>(AttrCenter, T("Center point:"), &VToolEllipticalArc::CenterPointName),
        {AttrRadius1, T("Radius 1:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T("Radius 1:"));
             p->SetFormula(t->GetFormulaRadius1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius1()) t->SetFormulaRadius1(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius1()); prop->setValue(v);
             }
         }},
        {AttrRadius2, T("Radius 2:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T("Radius 2:"));
             p->SetFormula(t->GetFormulaRadius2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius2()) t->SetFormulaRadius2(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius2()); prop->setValue(v);
             }
         }},
        {AttrAngle1, T("First angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T("First angle:"));
             p->SetFormula(t->GetFormulaF1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaF1()) t->SetFormulaF1(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaF1()); prop->setValue(v);
             }
         }},
        {AttrAngle2, T("Second angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T("Second angle:"));
             p->SetFormula(t->GetFormulaF2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaF2()) t->SetFormulaF2(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaF2()); prop->setValue(v);
             }
         }},
        {AttrRotationAngle, T("Rotation angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T("Rotation angle:"));
             p->SetFormula(t->GetFormulaRotationAngle()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRotationAngle()) t->SetFormulaRotationAngle(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRotationAngle()); prop->setValue(v);
             }
         }},
        MakeStringPropWithValidator<VToolEllipticalArc>(AttrAlias, T("ElArc:"), &VToolEllipticalArc::GetAliasSuffix, &VToolEllipticalArc::SetAliasSuffix),
        MakeCurvePenStyle<VToolEllipticalArc>(T("Pen style:")),
        MakeLineColor<VToolEllipticalArc>(T("Color:")),
        MakeApproximationScale<VToolEllipticalArc>(T("Approximation scale:")),
        {QStringLiteral("createStartPoint"), T("Create start point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create start point"));
             p->setValue(t->HasStartPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 t->SetCreateStartPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 prop->setValue(t->HasStartPoint());
             }
         }},
        {QStringLiteral("createEndPoint"), T("Create end point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create end point"));
             p->setValue(t->HasEndPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 t->SetCreateEndPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArc *>(b->currentItem())) {
                 prop->setValue(t->HasEndPoint());
             }
         }},
        MakeNotes<VToolEllipticalArc>(T("Notes:")),
    };
}

// --- VToolEllipticalArcWithLength ---
auto EllipticalArcWithLengthProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolEllipticalArcWithLength>(T("Name:")),
        MakeParentPoint<VToolEllipticalArcWithLength>(AttrCenter, T("Center point:"), &VToolEllipticalArcWithLength::CenterPointName),
        {AttrRadius1, T("Radius 1:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("Radius 1:"));
             p->SetFormula(t->GetFormulaRadius1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius1()) t->SetFormulaRadius1(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius1()); prop->setValue(v);
             }
         }},
        {AttrRadius2, T("Radius 2:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("Radius 2:"));
             p->SetFormula(t->GetFormulaRadius2()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius2()) t->SetFormulaRadius2(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius2()); prop->setValue(v);
             }
         }},
        {AttrAngle1, T("First angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("First angle:"));
             p->SetFormula(t->GetFormulaF1()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaF1()) t->SetFormulaF1(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaF1()); prop->setValue(v);
             }
         }},
        {AttrLength, T("Length:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("Length:"));
             p->SetFormula(t->GetFormulaLength()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
             }
         }},
        {AttrRotationAngle, T("Rotation angle:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T("Rotation angle:"));
             p->SetFormula(t->GetFormulaRotationAngle()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRotationAngle()) t->SetFormulaRotationAngle(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRotationAngle()); prop->setValue(v);
             }
         }},
        MakeStringPropWithValidator<VToolEllipticalArcWithLength>(AttrAlias, T("ElArc:"), &VToolEllipticalArcWithLength::GetAliasSuffix, &VToolEllipticalArcWithLength::SetAliasSuffix),
        MakeCurvePenStyle<VToolEllipticalArcWithLength>(T("Pen style:")),
        MakeLineColor<VToolEllipticalArcWithLength>(T("Color:")),
        MakeApproximationScale<VToolEllipticalArcWithLength>(T("Approximation scale:")),
        {QStringLiteral("createStartPoint"), T("Create start point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create start point"));
             p->setValue(t->HasStartPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 t->SetCreateStartPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 prop->setValue(t->HasStartPoint());
             }
         }},
        {QStringLiteral("createEndPoint"), T("Create end point"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T("Create end point"));
             p->setValue(t->HasEndPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 t->SetCreateEndPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolEllipticalArcWithLength *>(b->currentItem())) {
                 prop->setValue(t->HasEndPoint());
             }
         }},
        MakeNotes<VToolEllipticalArcWithLength>(T("Notes:")),
    };
}

// --- VToolParallelCurve ---
auto ParallelCurveProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolParallelCurve>(T("Full name:")),
        MakeSourceCurve<VToolParallelCurve>(AttrCurve, T("Curve:"), &VToolParallelCurve::CurveName),
        MakeStringPropWithValidator<VToolParallelCurve>(AttrAlias, T("Curve:"), &VToolParallelCurve::GetAliasSuffix, &VToolParallelCurve::SetAliasSuffix),
        {AttrWidth, T("Width:"), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolParallelCurve *>(tool);
             auto *p = new VFormulaProperty(T("Width:"));
             p->SetFormula(t->GetFormulaWidth()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolParallelCurve *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaWidth()) t->SetFormulaWidth(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolParallelCurve *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaWidth()); prop->setValue(v);
             }
         }},
        MakeLineColor<VToolParallelCurve>(T("Color:")),
        MakeNotes<VToolParallelCurve>(T("Notes:")),
    };
}

// --- VToolGraduatedCurve ---
auto GraduatedCurveProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolGraduatedCurve>(T("Full name:")),
        MakeSourceCurve<VToolGraduatedCurve>(AttrCurve, T("Curve:"), &VToolGraduatedCurve::CurveName),
        // The subName property stays auto-generated in the backend, it is just not shown any more.
        MakeStringPropWithValidator<VToolGraduatedCurve>(AttrAlias, T("Curve:"), &VToolGraduatedCurve::GetAliasSuffix, &VToolGraduatedCurve::SetAliasSuffix),
        MakeCurvePenStyle<VToolGraduatedCurve>(T("Pen style:")),
        MakeLineColor<VToolGraduatedCurve>(T("Color:")),
        MakeApproximationScale<VToolGraduatedCurve>(T("Approximation scale:")),
        MakeNotes<VToolGraduatedCurve>(T("Notes:")),
    };
}
