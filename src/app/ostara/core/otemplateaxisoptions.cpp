/************************************************************************
 **
 **  @file   otemplateaxisoptions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   19/09/2026
 **
 **  @brief  Reading and writing the aliases an information gives a size axis.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
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

#include "otemplateaxisoptions.h"

#include <QDomDocument>
#include <QDomElement>

#include <algorithm>

namespace
{
const QString kTagRename = QStringLiteral("rename");
const QString kTagRenameValue = QStringLiteral("value");
const QString kAttrLocale = QStringLiteral("locale");
const QString kAttrRenameName = QStringLiteral("name");

/// The axis a rename describes. Written since an information can show more than one; absent from
/// the files that predate it, where it is the one axis the section named.
const QString kAttrRenameAxis = QStringLiteral("axis");
} // namespace

auto ParseAxisRenames(const QVariant &var) -> OAxisRenames
{
    OAxisRenames result;
    const QVariantMap map = var.toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        const QVariantMap entry = it.value().toMap();
        OAxisRename rename;
        rename.name = entry.value(QStringLiteral("name")).toString();
        const QVariantList list = entry.value(QStringLiteral("values")).toList();
        rename.values.reserve(list.size());
        for (const QVariant &value : list)
        {
            rename.values.append(value.toString());
        }
        result.insert(it.key(), rename);
    }
    return result;
}

auto MakeAxisRenames(const OAxisRenames &renames) -> QVariant
{
    QVariantMap map;
    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it)
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), it.value().name);
        QVariantList list;
        list.reserve(it.value().values.size());
        for (const QString &value : it.value().values)
        {
            list.append(value);
        }
        entry.insert(QStringLiteral("values"), list);
        map.insert(it.key(), entry);
    }
    return map;
}

//---------------------------------------------------------------------------------------------------------------------

auto ParseAxisRenamesByAxis(const QVariant &var, const QString &legacyAxis)
    -> QMap<QString, OAxisRenames>
{
    QMap<QString, OAxisRenames> result;
    const QVariantMap outer = var.toMap();
    if (outer.isEmpty())
    {
        return result;
    }

    // Which shape is this? In the axis-keyed one, every value is a map of languages, and a language
    // is never called "name" or "values". In the older one, every value *is* the entry, so it holds
    // exactly those two.
    bool legacy = false;
    for (auto it = outer.constBegin(); it != outer.constEnd(); ++it)
    {
        const QVariantMap inner = it.value().toMap();
        if (inner.contains(QStringLiteral("name")) || inner.contains(QStringLiteral("values")))
        {
            legacy = true;
            break;
        }
    }

    if (legacy)
    {
        // One axis, and the file named it under its own key. An empty one keeps the aliases under
        // an empty name rather than throwing them away: they are the user's, and a file that named
        // no axis has nothing else to say about them.
        result.insert(legacyAxis, ParseAxisRenames(var));
        return result;
    }

    for (auto it = outer.constBegin(); it != outer.constEnd(); ++it)
    {
        result.insert(it.key(), ParseAxisRenames(it.value()));
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------

auto MakeAxisRenamesByAxis(const QMap<QString, OAxisRenames> &renames) -> QVariant
{
    QVariantMap map;
    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it)
    {
        map.insert(it.key(), MakeAxisRenames(it.value()));
    }
    return map;
}

//---------------------------------------------------------------------------------------------------------------------

void WriteAxisOptions(QDomDocument &doc, QDomElement &sectionEl,
                      const QHash<QString, QVariant> &options)
{
    // The axes the information shows, as one comma-separated attribute. The names are the table's
    // own — short words, and never a comma.
    const QStringList axes = options.value(OTemplateAxisOptions::kAxes).toStringList();
    if (!axes.isEmpty())
    {
        sectionEl.setAttribute(OTemplateAxisOptions::kAxes, axes.join(QLatin1Char(',')));
    }

    // The one-line-per-language flag is an attribute; the aliases become child <rename> elements,
    // one per axis and language. Only non-empty aliases are persisted (empty = keep the original) —
    // and an axis nobody renamed writes nothing at all, so a file says only what was chosen.
    const QVariant oneLine = options.value(OTemplateAxisOptions::kOneLine);
    if (oneLine.isValid())
    {
        sectionEl.setAttribute(OTemplateAxisOptions::kOneLine,
                               oneLine.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
    }

    const QMap<QString, OAxisRenames> renamesByAxis = ParseAxisRenamesByAxis(
        options.value(OTemplateAxisOptions::kRenames),
        options.value(OTemplateAxisOptions::kLegacyAxis).toString());
    for (auto axisIt = renamesByAxis.constBegin(); axisIt != renamesByAxis.constEnd(); ++axisIt)
    {
        for (auto it = axisIt.value().constBegin(); it != axisIt.value().constEnd(); ++it)
        {
            const OAxisRename &rename = it.value();
            const bool nothingRenamed =
                rename.name.isEmpty()
                && std::all_of(rename.values.constBegin(), rename.values.constEnd(),
                               [](const QString &alias) { return alias.isEmpty(); });
            if (nothingRenamed)
            {
                continue;
            }

            QDomElement renameEl = doc.createElement(kTagRename);
            renameEl.setAttribute(kAttrRenameAxis, axisIt.key());
            renameEl.setAttribute(kAttrLocale, it.key());
            if (!rename.name.isEmpty())
            {
                renameEl.setAttribute(kAttrRenameName, rename.name);
            }
            for (const QString &valueAlias : rename.values)
            {
                if (valueAlias.isEmpty())
                {
                    continue;
                }
                QDomElement valueEl = doc.createElement(kTagRenameValue);
                valueEl.appendChild(doc.createTextNode(valueAlias));
                renameEl.appendChild(valueEl);
            }
            sectionEl.appendChild(renameEl);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void ReadAxisOptions(const QDomElement &sectionEl, QHash<QString, QVariant> &options)
{
    // The axes the information shows. The older key, which named a single axis, is kept as it is:
    // the control and the resolvers both look for it, and reading it is what lets a template saved
    // before today open showing what it showed.
    if (sectionEl.hasAttribute(OTemplateAxisOptions::kAxes))
    {
        options.insert(OTemplateAxisOptions::kAxes,
                       sectionEl.attribute(OTemplateAxisOptions::kAxes)
                           .split(QLatin1Char(','), Qt::SkipEmptyParts));
    }
    const QString legacyAxis = sectionEl.attribute(OTemplateAxisOptions::kLegacyAxis);
    if (sectionEl.hasAttribute(OTemplateAxisOptions::kLegacyAxis))
    {
        options.insert(OTemplateAxisOptions::kLegacyAxis, legacyAxis);
    }

    if (sectionEl.hasAttribute(OTemplateAxisOptions::kOneLine))
    {
        const QString attr = sectionEl.attribute(OTemplateAxisOptions::kOneLine);
        options.insert(OTemplateAxisOptions::kOneLine,
                       attr.toInt() != 0
                           || attr.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
    }

    // The aliases, axis by axis. A child written before an information could show more than one
    // axis carries no axis of its own: it belongs to the one the section named.
    QMap<QString, OAxisRenames> renamesByAxis;
    for (QDomElement renameEl = sectionEl.firstChildElement(kTagRename); !renameEl.isNull();
         renameEl = renameEl.nextSiblingElement(kTagRename))
    {
        OAxisRename rename;
        rename.name = renameEl.attribute(kAttrRenameName);
        for (QDomElement valueEl = renameEl.firstChildElement(kTagRenameValue); !valueEl.isNull();
             valueEl = valueEl.nextSiblingElement(kTagRenameValue))
        {
            rename.values.append(valueEl.text());
        }
        QString axis = renameEl.attribute(kAttrRenameAxis);
        if (axis.isEmpty())
        {
            axis = legacyAxis;
        }
        renamesByAxis[axis].insert(renameEl.attribute(kAttrLocale), rename);
    }
    if (!renamesByAxis.isEmpty())
    {
        options.insert(OTemplateAxisOptions::kRenames, MakeAxisRenamesByAxis(renamesByAxis));
    }
}
