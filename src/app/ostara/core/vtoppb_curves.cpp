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
#include "../vtools/tools/drawTools/toolcurve/otoolarmholecurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vtools/tools/drawTools/toolcurve/otoolspiral.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vpropertyexplorer/plugins/vboolproperty.h"
#include "ocheckboxrowproperty.h"
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
#include "../vgeometry/vsplinepoint.h"
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
        prefix = VAbstractApplication::VApp()->TrVars()->InternalVarToUser(QStringLiteral("Curve_"));
    }
    else if (head == QLatin1String(ARC_) || head == QLatin1String(ELARC_))
    {
        prefix = VAbstractApplication::VApp()->TrVars()->InternalVarToUser(head);
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

    return prefix + (suffix.isEmpty() ? T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "(Unnamed)")) : suffix);
}

// --- Helpers ---

template <typename Tool>
auto MakeObjectNameReadOnly(const QString &label) -> PropertyDesc
{
    return {AttrName, label,
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
                        if (n != t->name()) t->setName(n);
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
    return {attrId, label,
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

// A cut tool can replace the base curve by its two sub-curves on the canvas, so the sub-curves
// become directly clickable. Purely a display switch: the geometry is never changed.
template<typename Tool>
auto MakeProlongationType(const QString &label) -> PropertyDesc
{
    return {AttrTypeLine,
            label,
            [label](VToolOptionsPropertyBrowser *b, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                QPalette const cp = b->ComboBoxPalette();
                auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                if (t->ProlongedSegment() == 0)
                {
                    return nullptr; // No prolongation on this cut: the setting would have nothing to draw.
                }
                auto *p = new VPE::VLineTypeProperty(label);
                p->setStyles(styles);
                auto idx = VPE::VLineTypeProperty::IndexOfStyle(styles, t->GetProlongationType());
                p->setValue(idx >= 0 ? idx : 0);
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const s = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (s != t->GetProlongationType())
                        t->SetProlongationType(s);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QPalette const cp = b->ComboBoxPalette();
                    auto styles = LineStylesPics(cp.color(QPalette::Base), cp.color(QPalette::Text));
                    auto idx = VPE::VLineTypeProperty::IndexOfStyle(styles, t->GetProlongationType());
                    prop->setValue(idx >= 0 ? idx : 0);
                }
            }};
}

template<typename Tool>
auto MakeProlongationColor(const QString &label) -> PropertyDesc
{
    return {AttrLineColor,
            label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                if (t->ProlongedSegment() == 0)
                {
                    return nullptr; // No prolongation on this cut: the setting would have nothing to draw.
                }
                auto *p = new VPE::VLineColorProperty(label);
                p->SetDefaultColors(VAbstractTool::ColorsList());
                p->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
                p->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
                p->setValue(t->GetProlongationColor());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const c = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (c != t->GetProlongationColor())
                        t->SetProlongationColor(c);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetProlongationColor());
            }};
}

template <typename Tool>
auto MakeShowTwoSegments(const QString &label) -> PropertyDesc
{
    return {AttrShowTwoSegments, label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new OCheckBoxRowProperty(label);
                p->setValue(t->IsShowTwoSegments());
                return p;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem())) {
                    const bool value = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
                    if (value != t->IsShowTwoSegments()) t->SetShowTwoSegments(value);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->IsShowTwoSegments());
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
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetNotes());
            }};
}

template <typename Tool>
auto MakeCurvePenStyle(const QString &label) -> PropertyDesc
{
    return {AttrPenStyle, label,
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
    return {AttrColor, label,
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

// A titled, collapsible group (expanded by default) holding two formula fields — Length then
// Angle. Used for the two spline handles: "First handle" is the control point attached to the
// start point (P1), "Second handle" the one attached to the end point (P4). Length comes before
// Angle on purpose, matching the order used everywhere else in the UI.
template <typename Tool>
auto MakeSplineHandleGroup(const QString &attrId, const QString &title,
                           QString (VSpline::*lengthFormulaGetter)() const,
                           void (VSpline::*lengthSetter)(qreal, const QString &),
                           QString (VSpline::*angleFormulaGetter)() const,
                           void (VSpline::*angleSetter)(qreal, const QString &)) -> PropertyDesc
{
    return {attrId, title,
            [title, lengthFormulaGetter, angleFormulaGetter](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                const VSpline spl = t->getSpline();

                auto *group = new VPE::VProperty(title);
                group->setCollapsible(true);
                group->setExpandedByDefault(true);

                auto *lengthProp = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
                VFormula lenF((spl.*lengthFormulaGetter)(), t->getData());
                lenF.setToolId(t->getId());
                lenF.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
                lenF.Eval();
                lengthProp->SetFormula(lenF);

                auto *angleProp = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
                VFormula angF((spl.*angleFormulaGetter)(), t->getData());
                angF.setToolId(t->getId());
                angF.setPostfix(degreeSymbol);
                angF.Eval();
                angleProp->SetFormula(angF);

                group->addChild(lengthProp);
                group->addChild(angleProp);
                return group;
            },
            [lengthSetter, angleSetter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const auto children = prop->getChildren();
                    if (children.size() >= 2)
                    {
                        VSpline spl = t->getSpline();
                        const auto lenF = children[0]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                        if (!lenF.error())
                        {
                            // Length is stored in pixels (see VToolSpline::Create); convert from user
                            // units. Angle is stored in degrees and needs no conversion.
                            (spl.*lengthSetter)(VAbstractValApplication::VApp()->toPixel(lenF.getDoubleValue()),
                                                lenF.GetFormula(FormulaType::FromUser));
                        }
                        const auto angF = children[1]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                        if (!angF.error())
                        {
                            (spl.*angleSetter)(angF.getDoubleValue(), angF.GetFormula(FormulaType::FromUser));
                        }
                        t->setSpline(spl);
                    }
                }
            },
            [lengthFormulaGetter, angleFormulaGetter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const auto children = prop->getChildren();
                    if (children.size() >= 2)
                    {
                        const VSpline spl = t->getSpline();

                        VFormula lenF((spl.*lengthFormulaGetter)(), t->getData());
                        lenF.setToolId(t->getId());
                        lenF.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
                        lenF.Eval();
                        QVariant v; v.setValue(lenF); children[0]->setValue(v);

                        VFormula angF((spl.*angleFormulaGetter)(), t->getData());
                        angF.setToolId(t->getId());
                        angF.setPostfix(degreeSymbol);
                        angF.Eval();
                        QVariant va; va.setValue(angF); children[1]->setValue(va);
                    }
                }
            }};
}

// One spline path handle = one Length formula + one Angle formula, read/written on a VSplinePoint.
struct SplinePathHandle
{
    QString (VSplinePoint::*lengthFormulaGetter)() const;
    void (VSplinePoint::*lengthSetter)(const qreal &, const QString &);
    qreal (VSplinePoint::*angleGetter)() const;
    QString (VSplinePoint::*angleFormulaGetter)() const;
    void (VSplinePoint::*angleSetter)(const qreal &, const QString &);
};

// A titled, collapsible group (collapsed by default) holding one or two handles, each rendered as
// "Length:" then "Angle:". Used for the SplinePath point toggles: Start/End hold a single handle,
// intermediate points hold two (incoming then outgoing), following the curve direction arrows.
template <typename Tool>
auto MakeSplinePathPointGroup(const QString &attrId, const QString &title, vsizetype pointIndex,
                              const QVector<SplinePathHandle> &handles) -> PropertyDesc
{
    return {attrId, title,
            [title, pointIndex, handles](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                const VSplinePoint point = t->getSplinePath().at(pointIndex);

                auto *group = new VPE::VProperty(title);
                group->setCollapsible(true);
                group->setExpandedByDefault(false);

                for (int h = 0; h < handles.size(); ++h)
                {
                    auto *lengthProp = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
                    VFormula lenF((point.*handles[h].lengthFormulaGetter)(), t->getData());
                    lenF.setToolId(t->getId());
                    lenF.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
                    lenF.Eval();
                    lengthProp->SetFormula(lenF);

                    auto *angleProp = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
                    VFormula angF((point.*handles[h].angleFormulaGetter)(), t->getData());
                    angF.setToolId(t->getId());
                    angF.setPostfix(degreeSymbol);
                    angF.Eval();
                    angleProp->SetFormula(angF);

                    group->addChild(lengthProp);
                    group->addChild(angleProp);
                }
                return group;
            },
            [pointIndex, handles](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const auto children = prop->getChildren();
                    if (children.size() >= handles.size() * 2)
                    {
                        VSplinePath path = t->getSplinePath();

                        // VSplinePoint copies alias the same shared data, so snapshot the handle
                        // angles as plain values before mutating the path.
                        QVector<qreal> originalAngles;
                        originalAngles.reserve(handles.size());
                        for (int h = 0; h < handles.size(); ++h)
                        {
                            originalAngles.append((path[pointIndex].*handles[h].angleGetter)());
                        }

                        // Length is independent for each handle — always write it (in pixels).
                        for (int h = 0; h < handles.size(); ++h)
                        {
                            const auto lenF =
                                children[h * 2]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                            if (!lenF.error())
                            {
                                (path[pointIndex].*handles[h].lengthSetter)(
                                    VAbstractValApplication::VApp()->toPixel(lenF.getDoubleValue()),
                                    lenF.GetFormula(FormulaType::FromUser));
                            }
                        }

                        // Angles of a strict (smooth) point are linked: SetAngle1 derives Angle2 and
                        // SetAngle2 derives Angle1. Writing both would let the stale sibling overwrite
                        // the edited angle, so write only the angle that actually changed.
                        for (int h = 0; h < handles.size(); ++h)
                        {
                            const auto angF =
                                children[h * 2 + 1]->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                            if (angF.error() || VFuzzyComparePossibleNulls(angF.getDoubleValue(), originalAngles.at(h)))
                            {
                                continue; // unchanged — leave the derived sibling untouched
                            }
                            (path[pointIndex].*handles[h].angleSetter)(angF.getDoubleValue(),
                                                                       angF.GetFormula(FormulaType::FromUser));
                        }

                        t->setSplinePath(path);

                        // The strict coupling may have derived the sibling angle — sync both angle
                        // fields back so the form doesn't keep showing a stale value.
                        const VSplinePoint finalPoint = path[pointIndex];
                        for (int h = 0; h < handles.size(); ++h)
                        {
                            VFormula angF((finalPoint.*handles[h].angleFormulaGetter)(), t->getData());
                            angF.setToolId(t->getId());
                            angF.setPostfix(degreeSymbol);
                            angF.Eval();
                            QVariant va; va.setValue(angF); children[h * 2 + 1]->setValue(va);
                        }
                    }
                }
            },
            [pointIndex, handles](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const auto children = prop->getChildren();
                    if (children.size() >= handles.size() * 2)
                    {
                        const VSplinePoint point = t->getSplinePath().at(pointIndex);
                        for (int h = 0; h < handles.size(); ++h)
                        {
                            VFormula lenF((point.*handles[h].lengthFormulaGetter)(), t->getData());
                            lenF.setToolId(t->getId());
                            lenF.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
                            lenF.Eval();
                            QVariant v; v.setValue(lenF); children[h * 2]->setValue(v);

                            VFormula angF((point.*handles[h].angleFormulaGetter)(), t->getData());
                            angF.setToolId(t->getId());
                            angF.setPostfix(degreeSymbol);
                            angF.Eval();
                            QVariant va; va.setValue(angF); children[h * 2 + 1]->setValue(va);
                        }
                    }
                }
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
auto SplinePathProperties(const VToolSplinePath *tool) -> std::vector<PropertyDesc>;
auto CubicBezierPathProperties() -> std::vector<PropertyDesc>;
auto EllipticalArcProperties() -> std::vector<PropertyDesc>;
auto EllipticalArcWithLengthProperties() -> std::vector<PropertyDesc>;
auto ParallelCurveProperties() -> std::vector<PropertyDesc>;
auto GraduatedCurveProperties() -> std::vector<PropertyDesc>;
auto SpiralProperties() -> std::vector<PropertyDesc>;

// ====== Tool descriptors ======

// --- VToolArc ---
auto ArcProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolArc>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")), &VToolArc::GetAliasSuffix, &VToolArc::SetAliasSuffix),
        {QStringLiteral("createStartPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")));
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
        {QStringLiteral("createEndPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")));
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
        {AttrRadius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")));
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
        {AttrAngle1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")));
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
        {AttrAngle2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second angle:")));
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
        MakeCurvePenStyle<VToolArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolArcWithLength ---
auto ArcWithLengthProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolArcWithLength>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")), &VToolArcWithLength::GetAliasSuffix, &VToolArcWithLength::SetAliasSuffix),
        {QStringLiteral("createStartPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")));
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
        {QStringLiteral("createEndPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")));
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
        {AttrRadius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")));
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
        {AttrAngle1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")));
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
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
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
        MakeCurvePenStyle<VToolArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolCutArc ---
auto CutArcProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeSourceCurve<VToolCutArc>(AttrArc,
                                     T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")),
                                     &VToolCutArc::CurveName),
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCutArc>(AttrAlias1,
                                                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")),
                                                 &VToolCutArc::GetAliasSuffix1,
                                                 &VToolCutArc::SetAliasSuffix1,
                                                 SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCutArc>(AttrAlias2,
                                                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Arc:")),
                                                 &VToolCutArc::GetAliasSuffix2,
                                                 &VToolCutArc::SetAliasSuffix2,
                                                 SegmentDotPixmap(VColorRole::VisSupportColor4)),
        {AttrLength,
         T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolCutArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             p->SetFormula(t->GetFormulaLength());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolCutArc *>(b->currentItem()))
             {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength())
                     t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolCutArc *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},
        MakeShowTwoSegments<VToolCutArc>(
            T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show the two segments instead of the curve"))),
        MakeProlongationType<VToolCutArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Type of line"))),
        MakeProlongationColor<VToolCutArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color"))),
        MakeNotes<VToolCutArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolCutArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")),
                                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")),
                                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolCutSpline ---
auto CutSplineProperties() -> std::vector<PropertyDesc>
{
    return {
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCutSpline>(AttrAlias1,
                                                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")),
                                                    &VToolCutSpline::GetAliasSuffix1,
                                                    &VToolCutSpline::SetAliasSuffix1,
                                                    SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCutSpline>(AttrAlias2,
                                                    T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")),
                                                    &VToolCutSpline::GetAliasSuffix2,
                                                    &VToolCutSpline::SetAliasSuffix2,
                                                    SegmentDotPixmap(VColorRole::VisSupportColor4)),
        {AttrLength,
         T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolCutSpline *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             p->SetFormula(t->GetFormulaLength());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolCutSpline *>(b->currentItem()))
             {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength())
                     t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolCutSpline *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},
        MakeShowTwoSegments<VToolCutSpline>(
            T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show the two segments instead of the curve"))),
        MakeProlongationType<VToolCutSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Type of line"))),
        MakeProlongationColor<VToolCutSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color"))),
        MakeNotes<VToolCutSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolCutSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")),
                                       T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")),
                                       T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolCutSplinePath ---
auto CutSplinePathProperties() -> std::vector<PropertyDesc>
{
    return {
        // Name1/Name2 stay auto-generated in the backend, they are just not shown any more.
        MakeStringPropWithValidator<VToolCutSplinePath>(AttrAlias1,
                                                        T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")),
                                                        &VToolCutSplinePath::GetAliasSuffix1,
                                                        &VToolCutSplinePath::SetAliasSuffix1,
                                                        SegmentDotPixmap(VColorRole::VisSupportColor2)),
        MakeStringPropWithValidator<VToolCutSplinePath>(AttrAlias2,
                                                        T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")),
                                                        &VToolCutSplinePath::GetAliasSuffix2,
                                                        &VToolCutSplinePath::SetAliasSuffix2,
                                                        SegmentDotPixmap(VColorRole::VisSupportColor4)),
        {AttrLength,
         T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolCutSplinePath *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             p->SetFormula(t->GetFormulaLength());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolCutSplinePath *>(b->currentItem()))
             {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength())
                     t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolCutSplinePath *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},
        MakeShowTwoSegments<VToolCutSplinePath>(
            T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show the two segments instead of the curve"))),
        MakeProlongationType<VToolCutSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Type of line"))),
        MakeProlongationColor<VToolCutSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color"))),
        MakeNotes<VToolCutSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        MakePointGroup<VToolCutSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point")),
                                           T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Point name:")),
                                           T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name"))),
    };
}

// --- VToolSpline ---
auto SplineProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolSpline>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolSpline::GetAliasSuffix, &VToolSpline::SetAliasSuffix),
        MakeSplineHandleGroup<VToolSpline>(QStringLiteral("firstHandle"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First handle:")),
                                           &VSpline::GetC1LengthFormula, &VSpline::SetC1Length,
                                           &VSpline::GetStartAngleFormula, &VSpline::SetStartAngle),
        MakeSplineHandleGroup<VToolSpline>(QStringLiteral("secondHandle"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second handle:")),
                                           &VSpline::GetC2LengthFormula, &VSpline::SetC2Length,
                                           &VSpline::GetEndAngleFormula, &VSpline::SetEndAngle),
        MakeCurvePenStyle<VToolSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolSpline>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolCubicBezier ---
auto CubicBezierProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolCubicBezier>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolCubicBezier>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolCubicBezier::GetAliasSuffix, &VToolCubicBezier::SetAliasSuffix),
        MakeCurvePenStyle<VToolCubicBezier>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolCubicBezier>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolCubicBezier>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- OToolArmholeCurve ---
auto ArmholeCurveProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<OToolArmholeCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<OToolArmholeCurve>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &OToolArmholeCurve::GetAliasSuffix, &OToolArmholeCurve::SetAliasSuffix),
        MakeCurvePenStyle<OToolArmholeCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<OToolArmholeCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<OToolArmholeCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolSplinePath ---
auto SplinePathProperties(const VToolSplinePath *tool) -> std::vector<PropertyDesc>
{
    std::vector<PropertyDesc> descs = {
        MakeObjectNameReadOnly<VToolSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolSplinePath>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolSplinePath::GetAliasSuffix, &VToolSplinePath::SetAliasSuffix),
    };

    const vsizetype count = tool != nullptr ? tool->getSplinePath().CountPoints() : 0;

    if (count >= 1)
    {
        // Start point: only the outgoing handle (Angle2/Length2) is used.
        descs.push_back(MakeSplinePathPointGroup<VToolSplinePath>(
            QStringLiteral("startPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Start point")), 0,
            {{&VSplinePoint::Length2Formula, &VSplinePoint::SetLength2, &VSplinePoint::Angle2,
              &VSplinePoint::Angle2Formula, &VSplinePoint::SetAngle2}}));
    }

    for (vsizetype i = 1; i < count - 1; ++i)
    {
        // Intermediate point: incoming handle (1) then outgoing handle (2), following the arrows.
        const QString title = VToolOptionsPropertyBrowser::tr("Point %1").arg(i + 1);
        descs.push_back(MakeSplinePathPointGroup<VToolSplinePath>(
            QStringLiteral("point%1").arg(i + 1), title, i,
            {{&VSplinePoint::Length1Formula, &VSplinePoint::SetLength1, &VSplinePoint::Angle1,
              &VSplinePoint::Angle1Formula, &VSplinePoint::SetAngle1},
             {&VSplinePoint::Length2Formula, &VSplinePoint::SetLength2, &VSplinePoint::Angle2,
              &VSplinePoint::Angle2Formula, &VSplinePoint::SetAngle2}}));
    }

    if (count >= 2)
    {
        // End point: only the incoming handle (Angle1/Length1) is used.
        descs.push_back(MakeSplinePathPointGroup<VToolSplinePath>(
            QStringLiteral("endPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "End point")), count - 1,
            {{&VSplinePoint::Length1Formula, &VSplinePoint::SetLength1, &VSplinePoint::Angle1,
              &VSplinePoint::Angle1Formula, &VSplinePoint::SetAngle1}}));
    }

    descs.push_back(MakeCurvePenStyle<VToolSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))));
    descs.push_back(MakeLineColor<VToolSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))));
    descs.push_back(MakeNotes<VToolSplinePath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))));

    return descs;
}

// --- VToolCubicBezierPath ---
auto CubicBezierPathProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolCubicBezierPath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolCubicBezierPath>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolCubicBezierPath::GetAliasSuffix, &VToolCubicBezierPath::SetAliasSuffix),
        MakeCurvePenStyle<VToolCubicBezierPath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolCubicBezierPath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolCubicBezierPath>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolEllipticalArc ---
auto EllipticalArcProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolEllipticalArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolEllipticalArc>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "ElArc:")), &VToolEllipticalArc::GetAliasSuffix, &VToolEllipticalArc::SetAliasSuffix),
        {QStringLiteral("createStartPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")));
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
        {QStringLiteral("createEndPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")));
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
        {AttrRadius1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 1:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 1:")));
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
        {AttrRadius2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 2:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 2:")));
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
        {AttrAngle1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")));
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
        {AttrAngle2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second angle:")));
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
        {AttrRotationAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArc *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")));
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
        MakeCurvePenStyle<VToolEllipticalArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolEllipticalArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolEllipticalArc>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolEllipticalArcWithLength ---
auto EllipticalArcWithLengthProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolEllipticalArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolEllipticalArcWithLength>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "ElArc:")), &VToolEllipticalArcWithLength::GetAliasSuffix, &VToolEllipticalArcWithLength::SetAliasSuffix),
        {QStringLiteral("createStartPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")));
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
        {QStringLiteral("createEndPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")));
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
        {AttrRadius1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 1:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 1:")));
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
        {AttrRadius2, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 2:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius 2:")));
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
        {AttrAngle1, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First angle:")));
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
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
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
        {AttrRotationAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolEllipticalArcWithLength *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")));
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
        MakeCurvePenStyle<VToolEllipticalArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolEllipticalArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolEllipticalArcWithLength>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- VToolParallelCurve ---
auto ParallelCurveProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolParallelCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<VToolParallelCurve>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolParallelCurve::GetAliasSuffix, &VToolParallelCurve::SetAliasSuffix),
        {QStringLiteral("createStartPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolParallelCurve *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")));
             p->setValue(t->HasStartPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolParallelCurve *>(b->currentItem())) {
                 t->SetCreateStartPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolParallelCurve *>(b->currentItem())) {
                 prop->setValue(t->HasStartPoint());
             }
         }},
        {QStringLiteral("createEndPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolParallelCurve *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")));
             p->setValue(t->HasEndPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolParallelCurve *>(b->currentItem())) {
                 t->SetCreateEndPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolParallelCurve *>(b->currentItem())) {
                 prop->setValue(t->HasEndPoint());
             }
         }},
        {AttrWidth, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Width:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolParallelCurve *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Width:")));
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
        MakeLineColor<VToolParallelCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolParallelCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}

// --- OToolSpiral ---
auto SpiralProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<OToolSpiral>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Name:"))),
        MakeStringPropWithValidator<OToolSpiral>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &OToolSpiral::GetAliasSuffix, &OToolSpiral::SetAliasSuffix),
        {AttrRadius, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolSpiral *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Radius:")));
             p->SetFormula(t->GetFormulaRadius()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaRadius()) t->SetFormulaRadius(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaRadius()); prop->setValue(v);
             }
         }},
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolSpiral *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             p->SetFormula(t->GetFormulaLength()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaLength()) t->SetFormulaLength(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaLength()); prop->setValue(v);
             }
         }},
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolSpiral *>(tool);
             auto *p = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
             p->SetFormula(t->GetFormulaAngle()); return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 auto f = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (f != t->GetFormulaAngle()) t->SetFormulaAngle(f);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 QVariant v; v.setValue(t->GetFormulaAngle()); prop->setValue(v);
             }
         }},
        MakeLineColor<OToolSpiral>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<OToolSpiral>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
        // The point is always there: this only says whether its name is drawn, hence the last position, after
        // the notes, where nothing else depends on it.
        {QStringLiteral("showPointName"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<OToolSpiral *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Show Point name")));
             p->setValue(t->EndPointLabelVisible());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 t->SetEndPointLabelVisible(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<OToolSpiral *>(b->currentItem())) {
                 prop->setValue(t->EndPointLabelVisible());
             }
         }},
    };
}

// --- VToolGraduatedCurve ---
auto GraduatedCurveProperties() -> std::vector<PropertyDesc>
{
    return {
        MakeObjectNameReadOnly<VToolGraduatedCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Full name:"))),
        MakeSourceCurve<VToolGraduatedCurve>(AttrCurve, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolGraduatedCurve::CurveName),
        // The subName property stays auto-generated in the backend, it is just not shown any more.
        MakeStringPropWithValidator<VToolGraduatedCurve>(AttrAlias, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Curve:")), &VToolGraduatedCurve::GetAliasSuffix, &VToolGraduatedCurve::SetAliasSuffix),
        {QStringLiteral("createStartPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolGraduatedCurve *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create start point")));
             p->setValue(t->HasStartPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolGraduatedCurve *>(b->currentItem())) {
                 t->SetCreateStartPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolGraduatedCurve *>(b->currentItem())) {
                 prop->setValue(t->HasStartPoint());
             }
         }},
        {QStringLiteral("createEndPoint"), T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
             auto *t = static_cast<VToolGraduatedCurve *>(tool);
             auto *p = new VPE::VBoolProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Create end point")));
             p->setValue(t->HasEndPoint());
             return p;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolGraduatedCurve *>(b->currentItem())) {
                 t->SetCreateEndPoint(prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool());
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
             if (auto *t = qgraphicsitem_cast<VToolGraduatedCurve *>(b->currentItem())) {
                 prop->setValue(t->HasEndPoint());
             }
         }},
        MakeCurvePenStyle<VToolGraduatedCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pen style:"))),
        MakeLineColor<VToolGraduatedCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Color:"))),
        MakeNotes<VToolGraduatedCurve>(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:"))),
    };
}
