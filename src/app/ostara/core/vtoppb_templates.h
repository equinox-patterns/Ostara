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
#include <type_traits>
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
#include "../vpatterndb/variables/vlinelength.h"
#include "../vtools/undocommands/vchangelinerolename.h"
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

// ====== The names of the lines a tool draws ======

/** @brief Apply a name to the line of @a pairKey, on the tool @a tool and in the pattern.
 *
 *  One definition, for every tool that lets its lines be named: the container holds the pair, the command
 *  writes it to `<lineNames>` and the pattern re-reads it. The uniqueness and the format are the container's
 *  business — it refuses a name already taken and the dock shows the value it kept.
 *
 *  @return true when the name was applied. */
inline auto ApplyLineName(VAbstractTool *tool, const QString &pairKey, const QString &suffix) -> bool
{
    if (tool == nullptr || pairKey.isEmpty())
    {
        return false;
    }

    VContainer *const data = tool->getData();
    if (data == nullptr)
    {
        return false;
    }

    const QString oldSuffix = data->LineRoleName(pairKey);
    if (oldSuffix == suffix)
    {
        return true; // nothing to do, and the caller must not revert the field
    }

    data->SetLineRoleName(pairKey, suffix);
    if (data->LineRoleName(pairKey) != suffix)
    {
        return false; // the container refused it: a name already taken by another line
    }

    VAbstractPattern *const doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return true;
    }

    auto *command = new VChangeLineRoleName(doc, pairKey, oldSuffix, suffix);
    QObject::connect(command, &VChangeLineRoleName::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(command);
    return true;
}

/** @brief The lines a tool draws that its own fields do not name, so the dock can offer to name them.
 *
 *  A tool draws one line per `AddLine` of its `Create`, and only some of them are covered by the "Line:"
 *  fields of its descriptor list. The others had no name to give at all — so no formula could ever write them,
 *  and the field asking for a name could never be satisfied. The pairs are read from the container: each
 *  `AddLine` registers its variables against the id of the tool that made them.
 *
 *  A pair is skipped when it already carries a name, or when the descriptor list already shows it. */
inline auto LineNameProperties(VAbstractTool *tool) -> std::vector<PropertyDesc>
{
    std::vector<PropertyDesc> extra;
    if (tool == nullptr || tool->getData() == nullptr)
    {
        return extra;
    }

    VContainer *const data = tool->getData();
    const quint32 toolId = tool->getId();
    int lineNumber = 1;

    for (const auto &var : data->DataLengthLines())
    {
        if (var.isNull())
        {
            continue;
        }

        const QList<quint32> refs = var->GetReferences();
        if (refs.isEmpty() || refs.constFirst() != toolId)
        {
            continue; // a line of another tool
        }

        // `AngleLine_` keeps the same pair, so the length variable is the one to read the pair from.
        const QString &prefix = line_;
        if (not var->GetName().startsWith(prefix))
        {
            continue;
        }
        const QString pairKey = var->GetName().mid(prefix.length());

        // Named already: the field that names it is the one that shows it, and the order the fields were
        // built in is the pattern's own order — the first line first. The alias and the role name are written
        // by `SetLineRoleName` before `AddLine` reads the pair, so either one answers.
        if (not var->GetAlias().isEmpty() || not data->LineRoleName(pairKey).isEmpty())
        {
            continue;
        }

        const QString attrId = QStringLiteral("lineName:") + pairKey;

        // The label numbers the lines of the tool rather than naming their points: Ostara's user is not meant
        // to read pairs, and the assistant points at the line anyway — this field only has to be findable.
        const QString label = QObject::tr("Line %1:").arg(++lineNumber);
        extra.push_back({attrId, label,
                         [label, pairKey, tool](VToolOptionsPropertyBrowser *b, void *) -> VPE::VProperty * {
                             Q_UNUSED(b)
                             auto *p = new VPE::VStringProperty(label);
                             if (tool->getData() != nullptr)
                             {
                                 p->setValue(tool->getData()->LineRoleName(pairKey));
                             }
                             return p;
                         },
                         [pairKey, tool](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                             Q_UNUSED(b)
                             const QString wanted = prop->data(VPE::VProperty::DPC_Data, Qt::EditRole).toString();
                             if (tool->getData() == nullptr)
                             {
                                 return;
                             }
                             if (wanted == tool->getData()->LineRoleName(pairKey))
                             {
                                 return;
                             }
                             if (not ApplyLineName(tool, pairKey, wanted))
                             {
                                 prop->setValue(tool->getData()->LineRoleName(pairKey));
                             }
                         },
                         [pairKey, tool](VToolOptionsPropertyBrowser *b, VPE::VProperty *prop) {
                             Q_UNUSED(b)
                             if (tool->getData() != nullptr)
                             {
                                 prop->setValue(tool->getData()->LineRoleName(pairKey));
                             }
                         }});
    }

    return extra;
}

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

    // The lines the tool draws that its own fields do not name: offered here, so that every line a tool makes
    // can be given a name — and so a formula can write it.
    // Not every item the dock shows is a tool — a background image is not, and it draws no line. The lines are
    // the tools' business, and only theirs.
    if constexpr (std::is_base_of_v<VAbstractTool, Tool>)
    {
        for (const auto &desc : LineNameProperties(i))
        {
            VPE::VProperty *prop = nullptr;
            try
            {
                prop = desc.create(this, static_cast<void *>(i));
            }
            catch (const VException &e)
            {
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
