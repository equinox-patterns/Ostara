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
#include "../vtools/tools/drawTools/operation/otoolclosedart.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/operation/otooltransfert.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "oformularowproperty.h"
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
auto CloseDartProperties() -> std::vector<PropertyDesc>;
auto MoveProperties() -> std::vector<PropertyDesc>;
auto TransfertProperties() -> std::vector<PropertyDesc>;
auto FlippingByLineProperties() -> std::vector<PropertyDesc>;
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>;

//---------------------------------------------------------------------------------------------------------------------
/** @brief The "Notes:" descriptor every tool of this file ends its list with.
 *
 *  The three callbacks only differ by the concrete tool type, which is the one thing the caller knows. */
template <class Tool> auto NotesProperty() -> PropertyDesc
{
    return {AttrNotes, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")),
            [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                auto *prop = new VPE::VTextProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Notes:")));
                prop->setValue(t->GetNotes());
                return prop;
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    QString const text = prop->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
                    if (text != t->GetNotes())
                        t->SetNotes(text);
                }
            },
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                    prop->setValue(t->GetNotes());
            }};
}

// --- VToolRotation ---
auto RotationProperties() -> std::vector<PropertyDesc>
{
    return {
        // Pivot point (read-only label)
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")),
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
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")),
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
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).value<VFormula>();
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

        // Length, the same rotation read as the distance from the reference point to its image. It is not
        // stored: it is computed from the angle and the pivot (see Rotation.md).
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<VToolRotation *>(tool);
             if (!t->HasReferencePoint())
             {
                 // No point among the sources: there is no length to measure, and the row says so rather than
                 // showing a zero that would read as a real value.
                 auto *prop = new VPE::VLabelProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
                 prop->setValue(QStringLiteral("—"));
                 return prop;
             }
             auto *prop = new VFormulaProperty(T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")));
             prop->SetFormula(t->GetFormulaLength());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).value<VFormula>();
                 if (formula != t->GetFormulaLength())
                 {
                     if (!t->SetFormulaLength(formula))
                     {
                         // The length is further than the circle the reference point draws: the tool kept its
                         // rotation, so the field goes back to what it really says instead of showing a length
                         // that was refused without a word (same revert as MakeStringPropWithValidator).
                         QVariant v;
                         v.setValue(t->GetFormulaLength());
                         prop->setValue(v);
                     }
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<VToolRotation *>(b->currentItem()))
             {
                 if (!t->HasReferencePoint())
                 {
                     return; // the row is a plain label: there is nothing to refresh
                 }
                 QVariant v;
                 v.setValue(t->GetFormulaLength());
                 prop->setValue(v);
             }
         }},

        // Notes
        NotesProperty<VToolRotation>(),
    };
}

// --- OToolCloseDart ---
auto CloseDartProperties() -> std::vector<PropertyDesc>
{
    return {
        // The closing divisor, and nothing else. The angle belongs to the dart and the three points are the
        // ones clicked in the pattern, so neither the angle nor a point name is shown here. The row is drawn
        // inline — label, field and fx button on one line — so that its long label cannot widen the label
        // column and push the Notes field of this same dock to the right.
        {AttrClosingDivisor, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Closing divisor:")),
         [](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
         {
             auto *t = static_cast<OToolCloseDart *>(tool);
             auto *prop = new OFormulaRowProperty(
                 T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Closing divisor:")));
             prop->SetFormula(t->GetFormulaDivisor());
             return prop;
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<OToolCloseDart *>(b->currentItem()))
             {
                 auto formula = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).value<VFormula>();
                 if (formula != t->GetFormulaDivisor())
                 {
                     t->SetFormulaDivisor(formula);
                     // A divisor that cannot divide is refused, and the dock must not go on showing it as if the
                     // tool had taken it: same revert as every setter that is allowed to say no. An inline row
                     // owns its field, so it takes the revert itself — the field has to give in even though the
                     // user is still typing in it.
                     if (t->GetFormulaDivisor() != formula)
                     {
                         if (auto *row = qobject_cast<OFormulaRowProperty *>(prop); row != nullptr)
                         {
                             row->RevertTo(t->GetFormulaDivisor());
                         }
                         else
                         {
                             QVariant v;
                             v.setValue(t->GetFormulaDivisor());
                             prop->setValue(v);
                         }
                     }
                 }
             }
         },
         [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
         {
             if (auto *t = qgraphicsitem_cast<OToolCloseDart *>(b->currentItem()))
             {
                 QVariant v;
                 v.setValue(t->GetFormulaDivisor());
                 prop->setValue(v);
             }
         }},

        // Notes
        NotesProperty<OToolCloseDart>(),
    };
}

// --- VToolMove ---
auto MoveProperties() -> std::vector<PropertyDesc>
{
    return {
        // Angle formula
        {AttrAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle:")),
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
        {AttrLength, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Length:")),
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
        {AttrRotationAngle, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Rotation angle:")),
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
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Pivot point:")),
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
        NotesProperty<VToolMove>(),
    };
}

// --- VToolFlippingByLine ---
auto FlippingByLineProperties() -> std::vector<PropertyDesc>
{
    return {
        // First line point (read-only)
        {AttrFirstPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "First line point:")),
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
        {AttrSecondPoint, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Second line point:")),
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
        NotesProperty<VToolFlippingByLine>(),
    };
}

// --- VToolFlippingByAxis ---
auto FlippingByAxisProperties() -> std::vector<PropertyDesc>
{
    return {
        // Axis type enum
        {AttrAxisType, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis type:")),
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
        {AttrCenter, T(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Origin point:")),
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
        NotesProperty<VToolFlippingByAxis>(),
    };
}

// --- OToolTransfert ---
auto TransfertProperties() -> std::vector<PropertyDesc>
{
    // Notes, and nothing else: the two merge points are read in the pattern, and neither the portion nor the
    // layer it landed in is edited after the tool is created.
    return {
        NotesProperty<OToolTransfert>(),
    };
}
