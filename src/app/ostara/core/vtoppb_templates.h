/************************************************************************
 **
 **  @file   vtoppb_templates.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Template definitions for VToolOptionsPropertyBrowser
 **
 **  @note   Extracted from vtooloptionspropertybrowser.cpp
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
#include <type_traits>
#include <vector>

#include <QPixmap>
#include <QPushButton>

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
#include "../vpropertyexplorer/plugins/vwidgetproperty.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vtools/tools/drawTools/vdrawtool.h"
#include "../vtools/undocommands/vchangelinerolename.h"
#include "oaxisrows.h"
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
} // namespace VtoppbConstants


struct PropertyDesc
{
    QString attrId;
    QString label;

    // create:  called in ShowOptions — creates a VPE::VProperty* from the tool
    // apply:   called in ChangeData — applies user edit to the tool (nullptr = read-only)
    // refresh: called in UpdateOptions — refreshes the property widget from the tool
    std::function<VPE::VProperty *(VToolOptionsPropertyBrowser *, void *tool)> create;
    std::function<void(VToolOptionsPropertyBrowser *, VPE::VProperty *)> apply;
    std::function<void(VToolOptionsPropertyBrowser *, VPE::VProperty *)> refresh;
};

/** @brief The row that says what direction the tool's angle follows: its axis, or "by hand".
 *
 *  The flat twin of the rows the curve handles put inside their group (see vtoppb_curves.cpp): a tool whose
 *  Options page is a plain list — a point placed by direction — reads the same sentence and pushes the same
 *  button, so the feature says the same thing everywhere. */
template <typename Tool> auto AxisSummaryProperty() -> PropertyDesc
{
    const QString label = VToolOptionsPropertyBrowser::tr(QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Axis:"));
    return {QStringLiteral("axis"), label,
            [label](VToolOptionsPropertyBrowser *, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                t->RefreshAxisFromFile();
                auto *prop = new VPE::VLabelProperty(label);
                prop->setValue(AxisSummaryText(t->GetAxis()));
                return prop;
            },
            nullptr,
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    t->RefreshAxisFromFile();
                    prop->setValue(AxisSummaryText(t->GetAxis()));
                }
            }};
}

/** @brief The button that starts the two-point pick for the tool's angle. */
template <typename Tool> auto AxisButtonProperty() -> PropertyDesc
{
    return {QStringLiteral("axisButton"), QString(),
            [](VToolOptionsPropertyBrowser *browser, void *tool) -> VPE::VProperty *
            {
                auto *t = static_cast<Tool *>(tool);
                t->RefreshAxisFromFile();
                auto *button = new QPushButton(AxisButtonText(t->GetAxis()));
                QObject::connect(button,
                                 &QPushButton::clicked,
                                 browser,
                                 [browser]()
                                 {
                                     if (auto *drawTool = dynamic_cast<VDrawTool *>(browser->currentItem()))
                                     {
                                         browser->StartAxisPicking(
                                             drawTool, 0,
                                             VToolOptionsPropertyBrowser::tr(
                                                 QT_TRANSLATE_NOOP("VToolOptionsPropertyBrowser", "Angle")));
                                     }
                                 });
                return new VPE::VWidgetProperty(AxisButtonText(t->GetAxis()), button);
            },
            nullptr,
            [](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop)
            {
                if (auto *t = qgraphicsitem_cast<Tool *>(b->currentItem()))
                {
                    t->RefreshAxisFromFile();
                    if (auto *buttonProp = dynamic_cast<VPE::VWidgetProperty *>(prop))
                    {
                        if (auto *button = qobject_cast<QPushButton *>(buttonProp->getWidget()))
                        {
                            button->setText(AxisButtonText(t->GetAxis()));
                        }
                    }
                }
            }};
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wheader-hygiene")
using namespace Qt::Literals::StringLiterals;
QT_WARNING_POP
//---------------------------------------------------------------------------------------------------------------------
inline void VToolOptionsPropertyBrowser::AddProperties(const QList<VPE::VProperty *> &properties,
                                                       const QStringList &ids)
{
    for (int i = 0; i < properties.size(); ++i)
    {
        m_propertyToId[properties.at(i)] = ids.at(i);
        m_idToProperty[ids.at(i)] = properties.at(i);
    }

    // One insertion for the whole set: the form view rebuilds its editors once, instead of once per property.
    m_PropertyModel->addProperties(properties, ids);
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

    QList<VPE::VProperty *> properties;
    QStringList ids;

    for (const auto &desc : descriptors)
    {
        if (desc.create)
        {
            VPE::VProperty *prop = nullptr;
            try
            {
                prop = desc.create(this, static_cast<void *>(i));
            }
            catch (const VException &e)
            {
                // One property that cannot read its value — a stale id, a formula variable that no longer fits —
                // must cost that property alone. The dock keeps working and the failure reaches the log.
                qWarning() << "Property" << desc.attrId << "could not be built:" << e.ErrorMessage();
                continue;
            }

            if (prop)
            {
                properties.append(prop);
                ids.append(desc.attrId);
            }
        }
    }

    AddProperties(properties, ids);
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
        try
        {
            it->apply(this, property);
        }
        catch (const VException &e)
        {
            // Report the failed edit and keep the dock — and the pattern — alive.
            qWarning() << "Property" << id << "could not be applied:" << e.ErrorMessage();
        }
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
                try
                {
                    desc.refresh(this, prop);
                }
                catch (const VException &e)
                {
                    // A property that cannot refresh keeps its last shown value; the dock is not torn down.
                    qWarning() << "Property" << desc.attrId << "could not be refreshed:" << e.ErrorMessage();
                }
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
    return {attrId, label,
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
