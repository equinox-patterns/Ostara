/************************************************************************
 **
 **  @file   vtoppb_templates.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Template definitions for VToolOptionsPropertyBrowser
 **
 **  @note   Extracted from vtooloptionspropertybrowser.cpp by Sophie Denys
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

#ifndef VTOPPB_TEMPLATES_H
#define VTOPPB_TEMPLATES_H

#include <algorithm>
#include <functional>
#include <vector>

#include <QPixmap>

#include "vtooloptionspropertybrowser.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vpropertyexplorer/plugins/vboolproperty.h"
#include "../vpropertyexplorer/plugins/venumproperty.h"
#include "../vpropertyexplorer/plugins/vlabelproperty.h"
#include "../vpropertyexplorer/plugins/vlinecolorproperty.h"
#include "../vpropertyexplorer/plugins/vlinetypeproperty.h"
#include "../vpropertyexplorer/plugins/vnumberproperty.h"
#include "../vpropertyexplorer/plugins/vpointfproperty.h"
#include "../vpropertyexplorer/plugins/vstringproperty.h"
#include "../vpropertyexplorer/plugins/vtextproperty.h"
#include "../vpatterndb/vformula.h"
#include "vformulaproperty.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "ifcdef.h"
#include "../vmisc/def.h"
#include "../vtools/tools/vabstracttool.h"

namespace VtoppbConstants
{
inline const QString AttrHold(QStringLiteral("hold"));
inline const QString AttrVisible(QStringLiteral("visible"));
inline const QString AttrOpacity(QStringLiteral("opacity"));
inline const QString AttrSubName(QStringLiteral("subName"));
inline const QString AttrAttachmentName(QStringLiteral("attachmentName"));
} // namespace VtoppbConstants


struct PropertyDesc
{
    QString attrId;
    QString label;
    bool readOnly = false;

    // create:  called in ShowOptions — creates a VPE::VProperty* from the tool
    // apply:   called in ChangeData — applies user edit to the tool (nullptr = read-only)
    // refresh: called in UpdateOptions — refreshes the property widget from the tool
    std::function<VPE::VProperty *(VToolOptionsPropertyBrowser *, void *tool)> create;
    std::function<void(VToolOptionsPropertyBrowser *, VPE::VProperty *)> apply;
    std::function<void(VToolOptionsPropertyBrowser *, VPE::VProperty *)> refresh;
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wheader-hygiene")
using namespace Qt::Literals::StringLiterals;
QT_WARNING_POP

template <class Tool>
void VToolOptionsPropertyBrowser::AddPropertyObjectName(Tool *i, const QString &propertyName, bool readOnly)
{
    if (readOnly)
    {
        auto *itemName = new VPE::VLabelProperty(propertyName);
        itemName->setValue(i->name());
        AddProperty(itemName, AttrName);
    }
    else
    {
        auto *itemName = new VPE::VStringProperty(propertyName);
        itemName->setValue(i->name());
        itemName->setReadOnly(readOnly);
        AddProperty(itemName, AttrName);
    }
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertySubName(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetName());
    AddProperty(itemName, VtoppbConstants::AttrSubName);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurveName1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetName1());
    AddProperty(itemName, AttrCurveName1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurveName2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetName2());
    AddProperty(itemName, AttrCurveName2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve1Name1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve1Name1());
    AddProperty(itemName, AttrCurve1Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve1Name2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve1Name2());
    AddProperty(itemName, AttrCurve1Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve2Name1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve2Name1());
    AddProperty(itemName, AttrCurve2Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve2Name2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve2Name2());
    AddProperty(itemName, AttrCurve2Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve1Alias1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve1AliasSuffix1());
    AddProperty(itemName, AttrCurve1Alias1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve1Alias2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve1AliasSuffix2());
    AddProperty(itemName, AttrCurve1Alias2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve2Alias1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve2AliasSuffix1());
    AddProperty(itemName, AttrCurve2Alias1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurve2Alias2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetCurve2AliasSuffix2());
    AddProperty(itemName, AttrCurve2Alias2);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyAlias(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetAliasSuffix());
    AddProperty(itemName, AttrAlias);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyAlias1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetAliasSuffix1());
    AddProperty(itemName, AttrAlias1);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyAlias2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetAliasSuffix2());
    AddProperty(itemName, AttrAlias2);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyPointName1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->nameP1());
    AddProperty(itemName, AttrName1);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyPointName2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->nameP2());
    AddProperty(itemName, AttrName2);
}

//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddPropertyParentPointName(const QString &pointName, const QString &propertyName,
                                                             const QString &propertyAttribure)
{
    auto *itemParentPoint = new VPE::VLabelProperty(propertyName);
    itemParentPoint->setValue(pointName);
    AddProperty(itemParentPoint, propertyAttribure);
}

//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddPropertyText(const QString &propertyName, const QString &text,
                                                  const QString &attrName)
{
    auto *itemText = new VPE::VTextProperty(propertyName);
    itemText->setValue(text);
    AddProperty(itemText, attrName);
}

//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddPropertyBool(const QString &propertyName, bool value, const QString &attrName)
{
    auto *itemBool = new VPE::VBoolProperty(propertyName);
    itemBool->setValue(value);
    AddProperty(itemBool, attrName);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyCrossPoint(Tool *i, const QString &propertyName)
{
    auto *itemProperty = new VPE::VEnumProperty(propertyName);
    itemProperty->setLiterals(
        QStringList({VToolOptionsPropertyBrowser::tr("First point"), VToolOptionsPropertyBrowser::tr("Second point")}));
    itemProperty->setValue(static_cast<int>(i->GetCrossCirclesPoint()) - 1);
    AddProperty(itemProperty, AttrCrossPoint);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyVCrossPoint(Tool *i, const QString &propertyName)
{
    auto *itemProperty = new VPE::VEnumProperty(propertyName);
    itemProperty->setLiterals(QStringList(
        {VToolOptionsPropertyBrowser::tr("Highest point"), VToolOptionsPropertyBrowser::tr("Lowest point")}));
    itemProperty->setValue(static_cast<int>(i->GetVCrossPoint()) - 1);
    AddProperty(itemProperty, AttrVCrossPoint);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyHCrossPoint(Tool *i, const QString &propertyName)
{
    auto *itemProperty = new VPE::VEnumProperty(propertyName);
    itemProperty->setLiterals(QStringList(
        {VToolOptionsPropertyBrowser::tr("Leftmost point"), VToolOptionsPropertyBrowser::tr("Rightmost point")}));
    itemProperty->setValue(static_cast<int>(i->GetHCrossPoint()) - 1);
    AddProperty(itemProperty, AttrHCrossPoint);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::AddPropertyAxisType(Tool *i, const QString &propertyName)
{
    auto *itemProperty = new VPE::VEnumProperty(propertyName);
    itemProperty->setLiterals(QStringList(
        {VToolOptionsPropertyBrowser::tr("Vertical axis"), VToolOptionsPropertyBrowser::tr("Horizontal axis")}));
    itemProperty->setValue(static_cast<int>(i->GetAxisType()) - 1);
    AddProperty(itemProperty, AttrAxisType);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool>
void VToolOptionsPropertyBrowser::AddPropertyLineType(Tool *i, const QString &propertyName,
                                                      const QMap<QString, QIcon> &styles)
{
    auto *lineTypeProperty = new VPE::VLineTypeProperty(propertyName);
    lineTypeProperty->setStyles(styles);
    const auto index = VPE::VLineTypeProperty::IndexOfStyle(styles, i->getLineType());
    if (index == -1)
    {
        qWarning() << "Can't find line style" << i->getLineType() << "in list";
    }
    lineTypeProperty->setValue(index);
    AddProperty(lineTypeProperty, AttrTypeLine);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool>
void VToolOptionsPropertyBrowser::AddPropertyCurvePenStyle(Tool *i, const QString &propertyName,
                                                           const QMap<QString, QIcon> &styles)
{
    auto *penStyleProperty = new VPE::VLineTypeProperty(propertyName);
    penStyleProperty->setStyles(styles);
    const auto index = VPE::VLineTypeProperty::IndexOfStyle(styles, i->GetPenStyle());
    if (index == -1)
    {
        qWarning() << "Can't find pen style" << i->getLineType() << "in list";
    }
    penStyleProperty->setValue(index);
    AddProperty(penStyleProperty, AttrPenStyle);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool>
void VToolOptionsPropertyBrowser::AddPropertyLineColor(Tool *i, const QString &propertyName,
                                                       const QMap<QString, QString> &colors, const QString &id)
{
    auto *lineColorProperty = new VPE::VLineColorProperty(propertyName);
    lineColorProperty->SetDefaultColors(colors);
    lineColorProperty->SetCustomColors(VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
    lineColorProperty->SetUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
    lineColorProperty->setValue(i->GetLineColor());
    AddProperty(lineColorProperty, id);
}

//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddPropertyApproximationScale(const QString &propertyName, qreal aScale)
{
    QMap<QString, QVariant> settings;
    settings.insert(QStringLiteral("Min"), 0);
    settings.insert(QStringLiteral("Max"), maxCurveApproximationScale);
    settings.insert(QStringLiteral("Step"), 0.1);
    settings.insert(QStringLiteral("Precision"), 1);

    auto *aScaleProperty = new VPE::VDoubleProperty(propertyName, settings);
    aScaleProperty->setValue(aScale);
    AddProperty(aScaleProperty, AttrAScale);
}

//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddPropertyOpacity(const QString &propertyName, int opacity)
{
    QMap<QString, QVariant> const settings{{QStringLiteral("Min"), 0},
                                           {QStringLiteral("Max"), 100},
                                           {QStringLiteral("Step"), 1},
                                           {QStringLiteral("Suffix"), QChar('%')}};

    auto *opacityProperty = new VPE::VIntegerProperty(propertyName, settings);
    opacityProperty->setValue(opacity);
    AddProperty(opacityProperty, VtoppbConstants::AttrOpacity);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool, typename Getter, typename Setter>
void VToolOptionsPropertyBrowser::SetStringPropertyGeneric(VPE::VProperty *property, Getter getter, Setter setter)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        QString const name = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
        if (name == (i->*getter)())
        {
            return;
        }
        (i->*setter)(name);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool, typename Getter, typename Setter>
void VToolOptionsPropertyBrowser::SetFormulaPropertyGeneric(VPE::VProperty *property, Getter getter, Setter setter)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        auto formula = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).value<VFormula>();
        if (formula == (i->*getter)())
        {
            return;
        }

        (i->*setter)(formula);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetName(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::name, &Tool::setName);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetHold(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        bool const hold = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
        if (hold == i->IsHold())
        {
            return;
        }

        i->SetHold(hold);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetVisible(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        bool const visible = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toBool();
        if (visible == i->IsVisible())
        {
            return;
        }

        i->SetVisible(visible);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetOpacity(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        const int opacity = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toInt();
        if (VFuzzyComparePossibleNulls(opacity, i->GetOpacity()))
        {
            return;
        }

        i->SetOpacity(opacity / 100.);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetPointName(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        QString const name = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
        if (name == i->name())
        {
            return;
        }

        if (QRegularExpression const rx(NameRegExp());
            name.isEmpty() || not VContainer::IsUnique(name, valentinaNamespace) || not rx.match(name).hasMatch())
        {
            m_idToProperty[AttrName]->setValue(i->name());
        }
        else
        {
            i->setName(name);
        }
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetPointName1(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        QString const name = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
        if (name == i->nameP1())
        {
            return;
        }

        if (QRegularExpression const rx(NameRegExp());
            name.isEmpty() || not VContainer::IsUnique(name, valentinaNamespace) || not rx.match(name).hasMatch())
        {
            m_idToProperty[AttrName1]->setValue(i->nameP1());
        }
        else
        {
            i->setNameP1(name);
        }
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetPointName2(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        QString const name = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
        if (name == i->nameP2())
        {
            return;
        }

        if (QRegularExpression const rx(NameRegExp());
            name.isEmpty() || not VContainer::IsUnique(name, valentinaNamespace) || not rx.match(name).hasMatch())
        {
            m_idToProperty[AttrName2]->setValue(i->nameP2());
        }
        else
        {
            i->setNameP2(name);
        }
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetSubName(VPE::VProperty *property)
{
    if (auto *item = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        QString const name = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();

        if (name == item->GetName())
        {
            return;
        }

        if (name.isEmpty())
        {
            m_idToProperty[VtoppbConstants::AttrSubName]->setValue(item->GetName());
            return;
        }

        QRegularExpression const rx(NameRegExp());
        const QString curveName = splPath_V + '_'_L1 + name;
        if (!rx.match(curveName).hasMatch() || !VContainer::IsUnique(curveName, valentinaNamespace))
        {
            m_idToProperty[VtoppbConstants::AttrSubName]->setValue(item->GetName());
            return;
        }

        item->SetName(name);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Type> auto VToolOptionsPropertyBrowser::GetCrossPoint(const QVariant &value) -> Type
{
    bool ok = false;
    const int val = value.toInt(&ok);

    auto cross = static_cast<Type>(1);
    if (ok)
    {
        switch (val)
        {
            case 0:
            case 1:
                cross = static_cast<Type>(val + 1);
                break;
            default:
                break;
        }
    }

    return cross;
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetCrossCirclesPoint(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        const QVariant value = property->data(VPE::VProperty::DPC_Data, Qt::EditRole);
        i->SetCrossCirclesPoint(GetCrossPoint<CrossCirclesPoint>(value));
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetVCrossCurvesPoint(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        const QVariant value = property->data(VPE::VProperty::DPC_Data, Qt::EditRole);
        auto type = GetCrossPoint<VCrossCurvesPoint>(value);
        if (type == i->GetVCrossPoint())
        {
            return;
        }

        i->SetVCrossPoint(type);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetHCrossCurvesPoint(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        const QVariant value = property->data(VPE::VProperty::DPC_Data, Qt::EditRole);
        if (auto type = GetCrossPoint<HCrossCurvesPoint>(value); type == i->GetHCrossPoint())
        {
            return;
        }

        i->SetHCrossPoint(GetCrossPoint<HCrossCurvesPoint>(value));
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetAxisType(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        const QVariant value = property->data(VPE::VProperty::DPC_Data, Qt::EditRole);
        auto type = GetCrossPoint<AxisType>(value);
        if (type == i->GetAxisType())
        {
            return;
        }

        i->SetAxisType(type);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetNotes(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetNotes, &Tool::SetNotes);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetAlias(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetAliasSuffix, &Tool::SetAliasSuffix);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetAlias1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetAliasSuffix1, &Tool::SetAliasSuffix1);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetAlias2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetAliasSuffix2, &Tool::SetAliasSuffix2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurveName1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetName1, &Tool::SetName1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurveName2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetName2, &Tool::SetName2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve1Name1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve1Name1, &Tool::SetCurve1Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve1Name2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve1Name2, &Tool::SetCurve1Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve2Name1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve2Name1, &Tool::SetCurve2Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve2Name2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve2Name2, &Tool::SetCurve2Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve1Alias1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve1AliasSuffix1, &Tool::SetCurve1AliasSuffix1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve1Alias2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve1AliasSuffix2, &Tool::SetCurve1AliasSuffix2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve2Alias1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve2AliasSuffix1, &Tool::SetCurve2AliasSuffix1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetCurve2Alias2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetCurve2AliasSuffix2, &Tool::SetCurve2AliasSuffix2);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetLineType(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::getLineType, &Tool::SetLineType);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetLineColor(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        QString const color = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toString();
        if (color == i->GetLineColor())
        {
            return;
        }

        i->SetLineColor(color);

        auto const customColors = property->getSetting("customColors"_L1).value<QVector<QColor>>();
        VAbstractValApplication::VApp()->ValentinaSettings()->SetUserToolColors(customColors);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetFormulaLength(VPE::VProperty *property)
{
    SetFormulaPropertyGeneric<Tool>(property, &Tool::GetFormulaLength, &Tool::SetFormulaLength);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetFormulaAngle(VPE::VProperty *property)
{
    SetFormulaPropertyGeneric<Tool>(property, &Tool::GetFormulaAngle, &Tool::SetFormulaAngle);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetFormulaRadius(VPE::VProperty *property)
{
    SetFormulaPropertyGeneric<Tool>(property, &Tool::GetFormulaRadius, &Tool::SetFormulaRadius);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetFormulaF1(VPE::VProperty *property)
{
    SetFormulaPropertyGeneric<Tool>(property, &Tool::GetFormulaF1, &Tool::SetFormulaF1);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetFormulaF2(VPE::VProperty *property)
{
    SetFormulaPropertyGeneric<Tool>(property, &Tool::GetFormulaF2, &Tool::SetFormulaF2);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetPenStyle(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetPenStyle, &Tool::SetPenStyle);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetFormulaRotationAngle(VPE::VProperty *property)
{
    SetFormulaPropertyGeneric<Tool>(property, &Tool::GetFormulaRotationAngle, &Tool::SetFormulaRotationAngle);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool> void VToolOptionsPropertyBrowser::SetApproximationScale(VPE::VProperty *property)
{
    if (auto *i = qgraphicsitem_cast<Tool *>(m_currentItem))
    {
        double const scale = property->data(VPE::VProperty::DPC_Data, Qt::DisplayRole).toDouble();
        if (VFuzzyComparePossibleNulls(scale, i->GetApproximationScale()))
        {
            return;
        }

        i->SetApproximationScale(scale);
    }
    else
    {
        qWarning() << "Can't cast item";
    }
}

//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddProperty(VPE::VProperty *property, const QString &id)
{
    m_propertyToId[property] = id;
    m_idToProperty[id] = property;
    m_PropertyModel->addProperty(property, id);
}

template<class Tool>
void VToolOptionsPropertyBrowser::SetArc1Name1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc1Name1, &Tool::SetArc1Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc1Name2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc1Name2, &Tool::SetArc1Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc2Name1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc2Name1, &Tool::SetArc2Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc2Name2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc2Name2, &Tool::SetArc2Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc1Alias1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc1AliasSuffix1, &Tool::SetArc1AliasSuffix1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc1Alias2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc1AliasSuffix2, &Tool::SetArc1AliasSuffix2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc2Alias1(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc2AliasSuffix1, &Tool::SetArc2AliasSuffix1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::SetArc2Alias2(VPE::VProperty *property)
{
    SetStringPropertyGeneric<Tool>(property, &Tool::GetArc2AliasSuffix2, &Tool::SetArc2AliasSuffix2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc1Name1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc1Name1());
    AddProperty(itemName, AttrCurve1Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc1Name2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc1Name2());
    AddProperty(itemName, AttrCurve1Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc2Name1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc2Name1());
    AddProperty(itemName, AttrCurve2Name1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc2Name2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc2Name2());
    AddProperty(itemName, AttrCurve2Name2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc1Alias1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc1AliasSuffix1());
    AddProperty(itemName, AttrCurve1Alias1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc1Alias2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc1AliasSuffix2());
    AddProperty(itemName, AttrCurve1Alias2);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc2Alias1(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc2AliasSuffix1());
    AddProperty(itemName, AttrCurve2Alias1);
}

//---------------------------------------------------------------------------------------------------------------------
template<class Tool>
void VToolOptionsPropertyBrowser::AddPropertyArc2Alias2(Tool *i, const QString &propertyName)
{
    auto *itemName = new VPE::VStringProperty(propertyName);
    itemName->setValue(i->GetArc2AliasSuffix2());
    AddProperty(itemName, AttrCurve2Alias2);
}

// ============================================================================
// Declarative property system — generic template functions
// ============================================================================

template <class Tool>
void VToolOptionsPropertyBrowser::ShowToolOptions(QGraphicsItem *item, const QString &title,
                                                   const std::vector<PropertyDesc> &descriptors)
{
    auto *i = qgraphicsitem_cast<Tool *>(item);
    if constexpr (requires(Tool & t) { t.ShowVisualization(true); })
    {
        i->ShowVisualization(true);
    }
    m_formView->setTitle(title);

    for (const auto &desc : descriptors)
    {
        if (desc.create)
        {
            auto *prop = desc.create(this, static_cast<void *>(i));
            if (prop)
            {
                AddProperty(prop, desc.attrId);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool>
void VToolOptionsPropertyBrowser::ChangeToolData(VPE::VProperty *property,
                                                  const std::vector<PropertyDesc> &descriptors)
{
    SCASSERT(property != nullptr)

    const QString id = m_propertyToId[property];

    auto it = std::find_if(descriptors.begin(), descriptors.end(),
                           [&id](const PropertyDesc &d) { return d.attrId == id; });
    if (it == descriptors.end())
    {
        qWarning() << "Unknown property type. id =" << id;
        return;
    }

    if (it->apply)
    {
        it->apply(this, property);
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Tool>
void VToolOptionsPropertyBrowser::UpdateToolOptions(const std::vector<PropertyDesc> &descriptors)
{
    for (const auto &desc : descriptors)
    {
        if (desc.refresh)
        {
            auto *prop = m_idToProperty.value(desc.attrId);
            if (prop)
            {
                desc.refresh(this, prop);
            }
        }
    }
}

// ====== Shared Property Browser string-property factories ======

// Plain string property — setter is called directly, no validation.
template <typename Tool, typename Getter, typename Setter>
auto MakeStringProp(const QString &attrId, const QString &label, Getter getter, Setter setter) -> PropertyDesc
{
    return {attrId, label, false,
            [getter, label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VStringProperty(label);
                p->setValue((t->*getter)());
                return p;
            },
            [getter, setter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                const QVariant v = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const QString newValue = v.toString();
                    if (newValue != (t->*getter)())
                    {
                        (t->*setter)(newValue);
                    }
                }
            },
            [getter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    prop->setValue((t->*getter)());
                }
            }};
}

// String property with post-setter validation.
// The setter internally validates uniqueness/format and silently rejects
// invalid input. After calling the setter, if the value didn't change,
// the widget is reverted to show the rejection visually.
template <typename Tool, typename Getter, typename Setter>
auto MakeStringPropWithValidator(const QString &attrId, const QString &label, Getter getter, Setter setter,
                                 const QPixmap &nameDecoration = QPixmap())
    -> PropertyDesc
{
    return {attrId, label, false,
            [getter, label, nameDecoration](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty * {
                auto *t = static_cast<Tool *>(tool);
                auto *p = new VPE::VStringProperty(label);
                p->setNameDecoration(nameDecoration);
                p->setValue((t->*getter)());
                return p;
            },
            [getter, setter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                const QVariant v = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole);
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    const QString newValue = v.toString();
                    if (newValue == (t->*getter)())
                    {
                        return; // no change
                    }
                    (t->*setter)(newValue);
                    // If the setter silently rejected (didn't change the value), revert the widget
                    if ((t->*getter)() != newValue)
                    {
                        prop->setValue((t->*getter)());
                    }
                }
            },
            [getter](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    prop->setValue((t->*getter)());
                }
            }};
}

#endif // VTOPPB_TEMPLATES_H
