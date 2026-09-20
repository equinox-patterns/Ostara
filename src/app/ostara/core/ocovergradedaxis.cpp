/************************************************************************
 **
 **  @file   ocovergradedaxis.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The graded axis of a cover page's Size(s) information: which sizes the page
 **          lists, what each is called, and how long a trace sample is drawn for it.
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

#include "ocovergradedaxis.h"

#include <QStringList>

#include <cmath>

namespace
{
/// Read a two-column map out of the option it is stored in.
auto MapFromOption(const QVariant &value) -> QHash<QString, QString>
{
    QHash<QString, QString> map;
    const QVariantMap stored = value.toMap();
    map.reserve(static_cast<int>(stored.size()));
    for (auto it = stored.cbegin(); it != stored.cend(); ++it)
    {
        const QString text = it.value().toString();
        // An entry that says nothing is not an entry: the map holds overrides, and « no override »
        // is the absence of one. Storing the emptiness would make a file say what it does not mean.
        if (!text.isEmpty())
        {
            map.insert(it.key(), text);
        }
    }
    return map;
}

/// The option a two-column map is stored as.
auto OptionFromMap(const QHash<QString, QString> &map) -> QVariant
{
    QVariantMap stored;
    for (auto it = map.cbegin(); it != map.cend(); ++it)
    {
        stored.insert(it.key(), it.value());
    }
    return stored;
}

/// Read one child element's two attributes, when both are there.
void ReadEntry(const QDomElement &parent, const QString &tag, const QString &keyAttr,
               const QString &valueAttr, QHash<QString, QString> &into)
{
    for (QDomElement child = parent.firstChildElement(tag); !child.isNull();
         child = child.nextSiblingElement(tag))
    {
        const QString key = child.attribute(keyAttr);
        const QString value = child.attribute(valueAttr);
        if (!key.isEmpty() && !value.isEmpty())
        {
            into.insert(key, value);
        }
    }
}
} // namespace

auto OCoverGradedAxis::HasRange() const -> bool
{
    return !measurement.isEmpty() && !axis.isEmpty() && !minValue.isEmpty() && !maxValue.isEmpty();
}

namespace OCoverGradedAxisOptions
{
auto DefaultOptions() -> QHash<QString, QVariant>
{
    // **No range chosen.** The page says so by holding nothing, rather than by holding a range that
    // happens to be empty: a section that has never been set up must not look like one that has.
    return {{kRange, QString()},
            {kAxis, QString()},
            {kMin, QString()},
            {kMax, QString()},
            {kNames, QVariantMap{}},
            {kTraceLength, OCoverGradedAxis::kDefaultTraceLengthCm}};
}

auto FromOptions(const QHash<QString, QVariant> &options) -> OCoverGradedAxis
{
    OCoverGradedAxis chosen;
    chosen.measurement = options.value(kRange).toString();
    chosen.axis = options.value(kAxis).toString();
    chosen.minValue = options.value(kMin).toString();
    chosen.maxValue = options.value(kMax).toString();
    chosen.names = MapFromOption(options.value(kNames));

    // Defensive read: the option is written by a control, but a template is a file on disk and a
    // file can be edited. A length that is not a positive number would draw nothing, or draw a
    // sample that runs off the page.
    const qreal length =
        options.value(kTraceLength, OCoverGradedAxis::kDefaultTraceLengthCm).toDouble();
    chosen.traceLengthCm = (length > 0.0 && std::isfinite(length))
                               ? length
                               : OCoverGradedAxis::kDefaultTraceLengthCm;
    return chosen;
}

void Write(QDomDocument &doc, QDomElement &sectionEl, const QHash<QString, QVariant> &options)
{
    const OCoverGradedAxis chosen = FromOptions(options);
    if (!chosen.HasRange())
    {
        // Nothing was chosen here, so nothing is written: an information that states one size has
        // no graded axis, and the file says that by saying nothing.
        return;
    }

    sectionEl.setAttribute(kRange, chosen.measurement);
    sectionEl.setAttribute(kAxis, chosen.axis);
    sectionEl.setAttribute(kMin, chosen.minValue);
    sectionEl.setAttribute(kMax, chosen.maxValue);
    sectionEl.setAttribute(kTraceLength, QString::number(chosen.traceLengthCm));

    for (auto it = chosen.names.cbegin(); it != chosen.names.cend(); ++it)
    {
        QDomElement nameEl = doc.createElement(kTagName);
        nameEl.setAttribute(kAttrLabel, it.key());
        nameEl.setAttribute(kAttrShown, it.value());
        sectionEl.appendChild(nameEl);
    }
}

void Read(const QDomElement &sectionEl, QHash<QString, QVariant> &options)
{
    // **Read only what is there.** A section that names no range keeps the empty one it starts
    // with, which is what tells the resolver there is nothing to list yet.
    const QString measurement = sectionEl.attribute(kRange);
    if (measurement.isEmpty())
    {
        return;
    }

    options.insert(kRange, measurement);
    options.insert(kAxis, sectionEl.attribute(kAxis));
    options.insert(kMin, sectionEl.attribute(kMin));
    options.insert(kMax, sectionEl.attribute(kMax));

    const QString length = sectionEl.attribute(kTraceLength);
    if (!length.isEmpty())
    {
        options.insert(kTraceLength, length.toDouble());
    }

    QHash<QString, QString> names;
    ReadEntry(sectionEl, kTagName, kAttrLabel, kAttrShown, names);
    options.insert(kNames, OptionFromMap(names));
}
} // namespace OCoverGradedAxisOptions
