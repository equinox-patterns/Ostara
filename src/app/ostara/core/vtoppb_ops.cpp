/************************************************************************
 **
 **  @file   vtoppb_ops.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  ops property browser methods — declarative descriptors
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
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "ifcdef.h"
#include "../vmisc/def.h"

namespace
{
auto T(const char *s) -> QString
{
    return VToolOptionsPropertyBrowser::tr(s);
}
} // anonymous namespace

// Forward declarations
auto RotationProperties() -> std::vector<PropertyDesc>;
auto MoveProperties() -> std::vector<PropertyDesc>;
auto FlippingByLineProperties() -> std::vector<PropertyDesc>;
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>;

// --- VToolRotation ---
auto RotationProperties() -> std::vector<PropertyDesc>
{
    return {
        // Pivot point (read-only label)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")));
             prop->setValue(t->OriginPointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
                 prop->setValue(t->OriginPointName());
         }},

        // Angle formula
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
             prop->SetFormula(t->GetFormulaAngle());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaAngle())
                     t->SetFormulaAngle(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaAngle());
                 prop->setValue(v);
             }
         }},

        // Notes
        {AttrNotes, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             auto *prop = new VPE::VTextProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")));
             prop->setValue(t->GetNotes());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (text != t->GetNotes())
                     t->SetNotes(text);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
                 prop->setValue(t->GetNotes());
         }},
    };
}

// --- VToolMove ---
auto MoveProperties() -> std::vector<PropertyDesc>
{
    return {
        // Angle formula
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")));
             prop->SetFormula(t->GetFormulaAngle());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaAngle())
                     t->SetFormulaAngle(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaAngle());
                 prop->setValue(v);
             }
         }},

        // Length formula
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             prop->SetFormula(t->GetFormulaLength());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaLength())
                     t->SetFormulaLength(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},

        // Rotation angle formula
        {AttrRotationAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")));
             prop->SetFormula(t->GetFormulaRotationAngle());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
                 if (formula != t->GetFormulaRotationAngle())
                     t->SetFormulaRotationAngle(formula);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaRotationAngle());
                 prop->setValue(v);
             }
         }},

        // Pivot point (read-only)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")));
             prop->setValue(t->OriginPointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
                 prop->setValue(t->OriginPointName());
         }},

        // Notes
        {AttrNotes, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolMove *>(tool);
             auto *prop = new VPE::VTextProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")));
             prop->setValue(t->GetNotes());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
             {
                 QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (text != t->GetNotes())
                     t->SetNotes(text);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolMove *>(b->currentItem()))
                 prop->setValue(t->GetNotes());
         }},
    };
}

// --- VToolFlippingByLine ---
auto FlippingByLineProperties() -> std::vector<PropertyDesc>
{
    return {
        // First line point (read-only)
        {AttrFirstPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByLine *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")));
             prop->setValue(t->FirstLinePointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByLine *>(b->currentItem()))
                 prop->setValue(t->FirstLinePointName());
         }},

        // Second line point (read-only)
        {AttrSecondPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByLine *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")));
             prop->setValue(t->SecondLinePointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByLine *>(b->currentItem()))
                 prop->setValue(t->SecondLinePointName());
         }},

        // Notes
        {AttrNotes, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByLine *>(tool);
             auto *prop = new VPE::VTextProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")));
             prop->setValue(t->GetNotes());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByLine *>(b->currentItem()))
             {
                 QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (text != t->GetNotes())
                     t->SetNotes(text);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByLine *>(b->currentItem()))
                 prop->setValue(t->GetNotes());
         }},
    };
}

// --- VToolFlippingByAxis ---
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        // Axis type enum
        {AttrAxisType, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis type:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByAxis *>(tool);
             auto *prop = new VPE::VEnumProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis type:")));
             prop->setLiterals(QStringList({VToolOptionsPropertyBrowser::tr("Vertical axis"),
                                            VToolOptionsPropertyBrowser::tr("Horizontal axis")}));
             prop->setValue(static_cast<int>(t->GetAxisType()) - 1);
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
             {
                 const QVariant value = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                 bool ok = false;
                 const int val = value.toInt(&ok);
                 if (ok)
                 {
                     AxisType type = AxisType::VerticalAxis;
                     switch (val)
                     {
                         case 0: type = AxisType::VerticalAxis; break;
                         case 1: type = AxisType::HorizontalAxis; break;
                         default: break;
                     }
                     if (type != t->GetAxisType())
                         t->SetAxisType(type);
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
                 prop->setValue(static_cast<int>(t->GetAxisType()) - 1);
         }},

        // Origin point (read-only)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Origin point:")), true,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByAxis *>(tool);
             auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Origin point:")));
             prop->setValue(t->OriginPointName());
             return prop;
         },
         nullptr,
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
                 prop->setValue(t->OriginPointName());
         }},

        // Notes
        {AttrNotes, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")), false,
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolFlippingByAxis *>(tool);
             auto *prop = new VPE::VTextProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")));
             prop->setValue(t->GetNotes());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
             {
                 QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                 if (text != t->GetNotes())
                     t->SetNotes(text);
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolFlippingByAxis *>(b->currentItem()))
                 prop->setValue(t->GetNotes());
         }},
    };
}
