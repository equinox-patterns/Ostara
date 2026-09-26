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
//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddProperty(VPE::VProperty *property, const QString &id)
{
    m_propertyToId[property] = id;
    m_idToProperty[id] = property;
    m_PropertyModel->addProperty(property, id);
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
        // A property the dock knows nothing about is a programming mistake: it belongs in the log, not in a
        // modal box raised from inside the property browser.
        qDebug() << "Unknown property type. id =" << id;
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
