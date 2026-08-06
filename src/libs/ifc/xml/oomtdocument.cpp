/************************************************************************
 **
 **  @file   oomtdocument.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  In-memory model for .omt (Ostara Multi-Tailles) measurement files.
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

#include "oomtdocument.h"
#include "../../vmisc/def.h"
#include "../exception/vexceptionobjecterror.h"

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(vOmtDoc, "v.omt.doc")

// ----- Root -----
const QString OOmtDocument::TagOmt = QStringLiteral("omt");
const QString OOmtDocument::AttrVersion = QStringLiteral("version");
const QString OOmtDocument::TagName = QStringLiteral("name");

// ----- Unit -----
const QString OOmtDocument::TagUnit = QStringLiteral("unit");
const QString OOmtDocument::AttrValue = QStringLiteral("value");

// ----- System -----
const QString OOmtDocument::TagSystem = QStringLiteral("system");

// ----- Size groups -----
const QString OOmtDocument::TagSizeGroups = QStringLiteral("sizeGroups");
const QString OOmtDocument::TagSizeGroup = QStringLiteral("sizeGroup");
const QString OOmtDocument::AttrName = QStringLiteral("name");
const QString OOmtDocument::AttrValues = QStringLiteral("values");

// ----- Anchor -----
const QString OOmtDocument::TagAnchor = QStringLiteral("anchor");
const QString OOmtDocument::TagBind = QStringLiteral("bind");
const QString OOmtDocument::AttrGroup = QStringLiteral("group");

// ----- Measurements -----
const QString OOmtDocument::TagMeasurements = QStringLiteral("measurements");
const QString OOmtDocument::TagM = QStringLiteral("m");
const QString OOmtDocument::TagValue = QStringLiteral("value");
const QString OOmtDocument::TagInc = QStringLiteral("inc");
const QString OOmtDocument::AttrFullName = QStringLiteral("fullName");
const QString OOmtDocument::AttrDescription = QStringLiteral("description");
const QString OOmtDocument::AttrSpecialUnits = QStringLiteral("specialUnits");
const QString OOmtDocument::AttrCollapsed = QStringLiteral("collapsed");

// ----- Range groups -----
const QString OOmtDocument::TagRangeGroups = QStringLiteral("rangeGroups");
const QString OOmtDocument::TagRangeGroup = QStringLiteral("rangeGroup");
const QString OOmtDocument::TagRange = QStringLiteral("range");

// ----- Grid -----
const QString OOmtDocument::TagGrid = QStringLiteral("grid");
const QString OOmtDocument::TagCell = QStringLiteral("cell");
const QString OOmtDocument::AttrM = QStringLiteral("m");
const QString OOmtDocument::AttrSg = QStringLiteral("sg");
const QString OOmtDocument::AttrV = QStringLiteral("v");

// ===== Constructor =====

OOmtDocument::OOmtDocument(QObject *parent)
  : VDomDocument(parent)
{
}

OOmtDocument::~OOmtDocument() = default;

// ===== Factory =====

auto OOmtDocument::CreateEmptyFile(Unit unit) -> bool
{
    clear();
    m_unknownElements.clear();

    QDomNode xmlHeader = createProcessingInstruction("xml", R"(version="1.0" encoding="UTF-8")");
    appendChild(xmlHeader);

    QDomElement root = createElement(TagOmt);
    root.setAttribute(AttrVersion, QString::fromLatin1(CurrentFormatVersion));
    appendChild(root);

    // Unit element
    QDomElement unitEl = createElement(TagUnit);
    QDomText unitText = createTextNode(UnitsToStr(unit));
    unitEl.appendChild(unitText);
    root.appendChild(unitEl);

    // Empty size groups
    QDomElement sgEl = createElement(TagSizeGroups);
    root.appendChild(sgEl);

    // Empty anchor
    QDomElement anchorEl = createElement(TagAnchor);
    root.appendChild(anchorEl);

    // Empty measurements
    QDomElement measEl = createElement(TagMeasurements);
    root.appendChild(measEl);

    qCDebug(vOmtDoc) << "Created empty .omt document v" << CurrentFormatVersion;
    return true;
}

// ===== I/O =====

void OOmtDocument::setXMLContent(const QString &fileName)
{
    VDomDocument::setXMLContent(fileName);

    // Validate root tag
    QDomElement root = RootElement();
    if (root.isNull() || root.tagName() != TagOmt)
    {
        QString tag = root.isNull() ? QStringLiteral("(null)") : root.tagName();
        throw VExceptionObjectError(tr("File is not a valid Ostara measurement table. "
                             "Expected root: <%1>, got: <%2>.")
                             .arg(TagOmt, tag));
    }

    // Scan for unknown top-level elements for forward-compat
    m_unknownElements.clear();
    if (!root.isNull())
    {
        QDomNode child = root.firstChild();
        while (!child.isNull())
        {
            if (child.isElement())
            {
                QDomElement el = child.toElement();
                QString tag = el.tagName();
                // Known tags (skip them)
                if (tag != TagName && tag != TagUnit &&
                    tag != TagSystem && tag != TagSizeGroups &&
                    tag != TagAnchor && tag != TagMeasurements &&
                    tag != TagGrid && tag != TagRangeGroups)
                {
                    qCInfo(vOmtDoc) << "Preserving unknown element:" << tag;
                    m_unknownElements.append(el);
                }
            }
            child = child.nextSibling();
        }
    }
}

auto OOmtDocument::SaveDocument(const QString &fileName, QString &error) -> bool
{
    // Re-attach unknown elements before saving
    QDomElement root = RootElement();
    for (const auto &el : m_unknownElements)
    {
        root.appendChild(el);
    }

    return VDomDocument::SaveDocument(fileName, error);
}

auto OOmtDocument::GetFormatVersionStr() const -> QString
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return QString::fromLatin1(CurrentFormatVersion);
    }
    return root.attribute(AttrVersion, QString::fromLatin1(CurrentFormatVersion));
}

// ===== Unit =====

auto OOmtDocument::GetUnits() const -> Unit
{
    QString unitStr = UniqueTagText(TagUnit);
    return StrToUnits(unitStr);
}

void OOmtDocument::SetUnits(Unit unit)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    SetChildText(root, TagUnit, UnitsToStr(unit));
}

// ===== Table Name =====

auto OOmtDocument::GetTableName() const -> QString
{
    return UniqueTagText(TagName);
}

void OOmtDocument::SetTableName(const QString &name)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    SetChildText(root, TagName, name);
}

// ===== System =====

auto OOmtDocument::GetSystem() const -> QString
{
    return UniqueTagText(TagSystem);
}

void OOmtDocument::SetSystem(const QString &uuid)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    if (uuid.isEmpty())
    {
        // Remove system element if empty
        QDomElement sysEl = root.firstChildElement(TagSystem);
        if (!sysEl.isNull())
        {
            root.removeChild(sysEl);
        }
    }
    else
    {
        SetChildText(root, TagSystem, uuid);
    }
}

// ===== Size groups =====

auto OOmtDocument::GetSizeGroupCount() const -> int
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return 0;
    }
    QDomElement sgEl = root.firstChildElement(TagSizeGroups);
    if (sgEl.isNull())
    {
        return 0;
    }
    return sgEl.elementsByTagName(TagSizeGroup).count();
}

auto OOmtDocument::GetSizeGroupNames() const -> QStringList
{
    QStringList names;
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return names;
    }
    QDomElement sgEl = root.firstChildElement(TagSizeGroups);
    if (sgEl.isNull())
    {
        return names;
    }
    QDomNodeList groups = sgEl.elementsByTagName(TagSizeGroup);
    for (int i = 0; i < groups.count(); ++i)
    {
        QDomElement g = groups.at(i).toElement();
        names.append(g.attribute(AttrName));
    }
    return names;
}

auto OOmtDocument::GetSizeGroupValues(const QString &groupName) const -> QStringList
{
    QDomElement sgEl = RootElement().firstChildElement(TagSizeGroups);
    if (sgEl.isNull())
    {
        return {};
    }
    QDomNodeList groups = sgEl.elementsByTagName(TagSizeGroup);
    for (int i = 0; i < groups.count(); ++i)
    {
        QDomElement g = groups.at(i).toElement();
        if (g.attribute(AttrName) == groupName)
        {
            return ParseValuesString(g.attribute(AttrValues));
        }
    }
    return {};
}

void OOmtDocument::AddSizeGroup(const QString &name, const QStringList &values)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    QDomElement sgEl = root.firstChildElement(TagSizeGroups);
    if (sgEl.isNull())
    {
        return;
    }

    QDomElement group = createElement(TagSizeGroup);
    group.setAttribute(AttrName, name);
    group.setAttribute(AttrValues, values.join(QLatin1Char(',')));
    sgEl.appendChild(group);
}

void OOmtDocument::RemoveSizeGroup(const QString &name)
{
    QDomElement sgEl = RootElement().firstChildElement(TagSizeGroups);
    if (sgEl.isNull())
    {
        return;
    }
    QDomNodeList groups = sgEl.elementsByTagName(TagSizeGroup);
    for (int i = 0; i < groups.count(); ++i)
    {
        QDomElement g = groups.at(i).toElement();
        if (g.attribute(AttrName) == name)
        {
            sgEl.removeChild(g);
            return;
        }
    }
}

void OOmtDocument::ClearSizeGroups()
{
    QDomElement sgEl = RootElement().firstChildElement(TagSizeGroups);
    if (sgEl.isNull())
    {
        return;
    }
    while (!sgEl.firstChild().isNull())
    {
        sgEl.removeChild(sgEl.firstChild());
    }
}

// ===== Anchor =====

auto OOmtDocument::GetAnchorBindings() const -> QMap<QString, QString>
{
    QMap<QString, QString> bindings;
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return bindings;
    }
    QDomElement anchorEl = root.firstChildElement(TagAnchor);
    if (anchorEl.isNull())
    {
        return bindings;
    }
    QDomNodeList binds = anchorEl.elementsByTagName(TagBind);
    for (int i = 0; i < binds.count(); ++i)
    {
        QDomElement b = binds.at(i).toElement();
        bindings.insert(b.attribute(AttrGroup), b.attribute(AttrValue));
    }
    return bindings;
}

auto OOmtDocument::GetAnchorValue(const QString &groupName) const -> QString
{
    QDomElement bindEl = FindBindElement(groupName);
    if (bindEl.isNull())
    {
        return {};
    }
    return bindEl.attribute(AttrValue);
}

void OOmtDocument::SetAnchorBinding(const QString &groupName, const QString &value)
{
    QDomElement bindEl = FindBindElement(groupName);
    if (bindEl.isNull())
    {
        // Create new bind
        QDomElement root = RootElement();
        if (root.isNull())
        {
            return;
        }
        QDomElement anchorEl = root.firstChildElement(TagAnchor);
        if (anchorEl.isNull())
        {
            return;
        }
        bindEl = createElement(TagBind);
        bindEl.setAttribute(AttrGroup, groupName);
        anchorEl.appendChild(bindEl);
    }
    bindEl.setAttribute(AttrValue, value);
}

void OOmtDocument::ClearAnchor()
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    QDomElement anchorEl = root.firstChildElement(TagAnchor);
    if (anchorEl.isNull())
    {
        return;
    }
    while (!anchorEl.firstChild().isNull())
    {
        anchorEl.removeChild(anchorEl.firstChild());
    }
}

// ===== Measurements =====

auto OOmtDocument::MeasurementsElement() const -> QDomElement
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return {};
    }
    return root.firstChildElement(TagMeasurements);
}

auto OOmtDocument::FindMElement(const QString &name) const -> QDomElement
{
    QDomElement measEl = MeasurementsElement();
    if (measEl.isNull())
    {
        return {};
    }
    QDomNodeList mList = measEl.elementsByTagName(TagM);
    for (int i = 0; i < mList.count(); ++i)
    {
        QDomElement m = mList.at(i).toElement();
        if (m.attribute(AttrName) == name)
        {
            return m;
        }
    }
    return {};
}

auto OOmtDocument::FindIncElement(const QDomElement &mEl, const QString &group) const -> QDomElement
{
    QDomNodeList incs = mEl.elementsByTagName(TagInc);
    for (int i = 0; i < incs.count(); ++i)
    {
        QDomElement inc = incs.at(i).toElement();
        if (inc.attribute(AttrGroup) == group)
        {
            return inc;
        }
    }
    return {};
}

auto OOmtDocument::GetMCount() const -> int
{
    QDomElement measEl = MeasurementsElement();
    if (measEl.isNull())
    {
        return 0;
    }
    return measEl.elementsByTagName(TagM).count();
}

auto OOmtDocument::GetAllMNames() const -> QStringList
{
    QStringList names;
    QDomElement measEl = MeasurementsElement();
    if (measEl.isNull())
    {
        return names;
    }
    QDomNodeList mList = measEl.elementsByTagName(TagM);
    for (int i = 0; i < mList.count(); ++i)
    {
        QDomElement m = mList.at(i).toElement();
        names.append(m.attribute(AttrName));
    }
    return names;
}

auto OOmtDocument::GetMFullName(const QString &name) const -> QString
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return {};
    }
    return mEl.attribute(AttrFullName);
}

auto OOmtDocument::GetMDescription(const QString &name) const -> QString
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return {};
    }
    return mEl.attribute(AttrDescription);
}

auto OOmtDocument::GetMType(const QString &name) const -> QString
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return QStringLiteral("measurement");
    }
    return mEl.attribute(AttrType, QStringLiteral("measurement"));
}

auto OOmtDocument::GetMValue(const QString &name) const -> double
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        qCWarning(vOmtDoc) << "GetMValue: measurement" << name << "not found";
        return 0.0;
    }
    QDomElement valEl = mEl.firstChildElement(TagValue);
    if (valEl.isNull())
    {
        qCWarning(vOmtDoc) << "GetMValue: no <value> element for measurement" << name;
        return 0.0;
    }
    // The value is stored as TEXT content of <value>, not as an attribute
    QString text = valEl.text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    double v = text.toDouble(&ok);
    return ok ? v : 0.0;
}

auto OOmtDocument::GetMIncrements(const QString &name) const -> QMap<QString, GroupIncrement>
{
    QMap<QString, GroupIncrement> incs;
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return incs;
    }
    QDomNodeList incList = mEl.elementsByTagName(TagInc);
    for (int i = 0; i < incList.count(); ++i)
    {
        QDomElement inc = incList.at(i).toElement();
        QString group = inc.attribute(AttrGroup);
        GroupIncrement gi;

        // Check for ranges mode
        QString mode = inc.attribute(QStringLiteral("mode"));
        if (mode == QStringLiteral("ranges"))
        {
            gi.isLinear = false;
            gi.rangeGroupName = inc.attribute(QStringLiteral("config"));
            // Parse CSV of range increment values
            QString csv = inc.text().trimmed();
            const QStringList parts = csv.split(QLatin1Char(','));
            for (const QString &part : parts)
            {
                bool ok = false;
                double v = part.trimmed().toDouble(&ok);
                gi.rangeValues.append(ok ? v : 0.0);
            }
        }
        else
        {
            // Linear (default) — read text content
            gi.isLinear = true;
            QString linText = inc.text().trimmed();
            linText.replace(QLatin1Char(','), QLatin1Char('.'));
            bool linOk = false;
            gi.linearValue = linText.toDouble(&linOk);
        }

        incs.insert(group, gi);
    }
    return incs;
}

auto OOmtDocument::GetMIncrement(const QString &name, const QString &group) const -> double
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return 0.0;
    }
    QDomElement incEl = FindIncElement(mEl, group);
    if (incEl.isNull())
    {
        return 0.0;
    }
    QString text = incEl.text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    double v = text.toDouble(&ok);
    return ok ? v : 0.0;
}

auto OOmtDocument::GetMSpecialUnits(const QString &name) const -> bool
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return false;
    }
    return mEl.attribute(AttrSpecialUnits, QStringLiteral("false")) == QStringLiteral("true");
}

auto OOmtDocument::GetMCollapsed(const QString &name) const -> bool
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return false;
    }
    return mEl.attribute(AttrCollapsed, QStringLiteral("false")) == QStringLiteral("true");
}

void OOmtDocument::SetMCollapsed(const QString &name, bool collapsed)
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return;
    }
    if (collapsed)
    {
        mEl.setAttribute(AttrCollapsed, QStringLiteral("true"));
    }
    else
    {
        mEl.removeAttribute(AttrCollapsed);
    }
}

void OOmtDocument::AddM(const QString &name, const QString &fullName, const QString &description,
                         double anchorValue, const QMap<QString, GroupIncrement> &increments,
                         const QString &type, bool specialUnits)
{
    QDomElement measEl = MeasurementsElement();
    if (measEl.isNull())
    {
        return;
    }

    QDomElement mEl = createElement(TagM);
    mEl.setAttribute(AttrName, name);
    mEl.setAttribute(AttrFullName, fullName);
    if (!description.isEmpty())
    {
        mEl.setAttribute(AttrDescription, description);
    }
    mEl.setAttribute(AttrType, type);
    if (specialUnits)
    {
        mEl.setAttribute(AttrSpecialUnits, QStringLiteral("true"));
    }

    // Value child element
    QDomElement valEl = createElement(TagValue);
    QDomText valText = createTextNode(QString::number(anchorValue, 'f', 2));
    valEl.appendChild(valText);
    mEl.appendChild(valEl);

    // Increment child elements
    for (auto it = increments.cbegin(); it != increments.cend(); ++it)
    {
        const GroupIncrement &gi = it.value();
        QDomElement incEl = createElement(TagInc);
        incEl.setAttribute(AttrGroup, it.key());

        if (gi.isLinear)
        {
            QDomText incText = createTextNode(QString::number(gi.linearValue, 'f', 2));
            incEl.appendChild(incText);
        }
        else
        {
            incEl.setAttribute(QStringLiteral("mode"), QStringLiteral("ranges"));
            incEl.setAttribute(QStringLiteral("config"), gi.rangeGroupName);
            // Store increment values as CSV
            QStringList csvParts;
            for (double v : gi.rangeValues)
            {
                csvParts.append(QString::number(v, 'f', 2));
            }
            QDomText incText = createTextNode(csvParts.join(QLatin1Char(',')));
            incEl.appendChild(incText);
        }

        mEl.appendChild(incEl);
    }

    measEl.appendChild(mEl);
}

void OOmtDocument::SetMValue(const QString &name, double value)
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return;
    }
    SetChildText(mEl, TagValue, QString::number(value, 'f', 2));
}

void OOmtDocument::SetMIncrement(const QString &name, const QString &group, double inc)
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return;
    }
    QDomElement incEl = FindIncElement(mEl, group);
    if (incEl.isNull())
    {
        incEl = createElement(TagInc);
        incEl.setAttribute(AttrGroup, group);
        mEl.appendChild(incEl);
    }
    // Replace text content
    while (!incEl.firstChild().isNull())
    {
        incEl.removeChild(incEl.firstChild());
    }
    QDomText t = createTextNode(QString::number(inc, 'f', 2));
    incEl.appendChild(t);
}

void OOmtDocument::RemoveM(const QString &name)
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return;
    }
    QDomElement measEl = MeasurementsElement();
    measEl.removeChild(mEl);
}

// ===== Range groups =====

auto OOmtDocument::GetRangeGroups(const QString &axisGroupName) const -> QVector<RangeGroup>
{
    QVector<RangeGroup> result;
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return result;
    }
    QDomNodeList rgList = root.elementsByTagName(TagRangeGroups);
    for (int i = 0; i < rgList.count(); ++i)
    {
        QDomElement rgEl = rgList.at(i).toElement();
        if (rgEl.attribute(AttrGroup) != axisGroupName)
        {
            continue;
        }
        QDomNodeList groups = rgEl.elementsByTagName(TagRangeGroup);
        for (int g = 0; g < groups.count(); ++g)
        {
            QDomElement gEl = groups.at(g).toElement();
            RangeGroup rg;
            rg.name = gEl.attribute(AttrName);
            QDomNodeList ranges = gEl.elementsByTagName(TagRange);
            for (int r = 0; r < ranges.count(); ++r)
            {
                QDomElement rEl = ranges.at(r).toElement();
                RangeDef rd;
                rd.fromValue = rEl.attribute(QStringLiteral("from"));
                rd.toValue = rEl.attribute(QStringLiteral("to"));
                rg.ranges.append(rd);
            }
            result.append(rg);
        }
    }
    return result;
}

void OOmtDocument::AddRangeGroup(const QString &axisGroupName, const RangeGroup &rg)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    // Find or create <rangeGroups group="...">
    QDomElement rgEl;
    QDomNodeList rgList = root.elementsByTagName(TagRangeGroups);
    for (int i = 0; i < rgList.count(); ++i)
    {
        QDomElement el = rgList.at(i).toElement();
        if (el.attribute(AttrGroup) == axisGroupName)
        {
            rgEl = el;
            break;
        }
    }
    if (rgEl.isNull())
    {
        rgEl = createElement(TagRangeGroups);
        rgEl.setAttribute(AttrGroup, axisGroupName);
        root.appendChild(rgEl);
    }

    QDomElement gEl = createElement(TagRangeGroup);
    gEl.setAttribute(AttrName, rg.name);
    for (const auto &rd : rg.ranges)
    {
        QDomElement rEl = createElement(TagRange);
        rEl.setAttribute(QStringLiteral("from"), rd.fromValue);
        rEl.setAttribute(QStringLiteral("to"), rd.toValue);
        gEl.appendChild(rEl);
    }
    rgEl.appendChild(gEl);
}

void OOmtDocument::RemoveRangeGroup(const QString &axisGroupName, const QString &rgName)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    QDomNodeList rgList = root.elementsByTagName(TagRangeGroups);
    for (int i = 0; i < rgList.count(); ++i)
    {
        QDomElement rgEl = rgList.at(i).toElement();
        if (rgEl.attribute(AttrGroup) != axisGroupName)
        {
            continue;
        }
        QDomNodeList groups = rgEl.elementsByTagName(TagRangeGroup);
        for (int g = 0; g < groups.count(); ++g)
        {
            QDomElement gEl = groups.at(g).toElement();
            if (gEl.attribute(AttrName) == rgName)
            {
                rgEl.removeChild(gEl);
                return;
            }
        }
    }
}

void OOmtDocument::UpdateRangeGroup(const QString &axisGroupName, const RangeGroup &rg)
{
    RemoveRangeGroup(axisGroupName, rg.name);
    AddRangeGroup(axisGroupName, rg);
}

void OOmtDocument::ClearRangeGroups(const QString &axisGroupName)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    QDomNodeList rgList = root.elementsByTagName(TagRangeGroups);
    for (int i = 0; i < rgList.count(); ++i)
    {
        QDomElement rgEl = rgList.at(i).toElement();
        if (rgEl.attribute(AttrGroup) == axisGroupName)
        {
            root.removeChild(rgEl);
            return;
        }
    }
}

// ===== Grid =====

auto OOmtDocument::GetGridCell(const QString &mName, const QString &coord) const -> double
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return 0.0;
    }
    QDomElement gridEl = root.firstChildElement(TagGrid);
    if (gridEl.isNull())
    {
        return 0.0;
    }
    QDomNodeList cells = gridEl.elementsByTagName(TagCell);
    for (int i = 0; i < cells.count(); ++i)
    {
        QDomElement cell = cells.at(i).toElement();
        if (cell.attribute(AttrM) == mName && cell.attribute(AttrSg) == coord)
        {
            return GetParametrDouble(cell, AttrV, "0");
        }
    }
    return 0.0;
}

void OOmtDocument::SetGridCell(const QString &mName, const QString &coord, double value)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    QDomElement gridEl = EnsureChildElement(root, TagGrid);

    // Check if cell already exists, update if so
    QDomNodeList cells = gridEl.elementsByTagName(TagCell);
    for (int i = 0; i < cells.count(); ++i)
    {
        QDomElement cell = cells.at(i).toElement();
        if (cell.attribute(AttrM) == mName && cell.attribute(AttrSg) == coord)
        {
            SetAttribute(cell, AttrV, value);
            return;
        }
    }

    // Create new cell
    QDomElement cell = createElement(TagCell);
    cell.setAttribute(AttrM, mName);
    cell.setAttribute(AttrSg, coord);
    SetAttribute(cell, AttrV, value);
    gridEl.appendChild(cell);
}

void OOmtDocument::ClearGrid()
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    QDomElement gridEl = root.firstChildElement(TagGrid);
    if (gridEl.isNull())
    {
        return;
    }
    while (!gridEl.firstChild().isNull())
    {
        gridEl.removeChild(gridEl.firstChild());
    }
}

// ===== Computation =====

void OOmtDocument::ComputeGrid()
{
    ClearGrid();

    QStringList groupNames = GetSizeGroupNames();
    if (groupNames.isEmpty())
    {
        qCWarning(vOmtDoc) << "ComputeGrid: no size groups defined";
        return;
    }

    // Build ordered list of value lists (one per group, in group order)
    QList<QStringList> groupValues;
    for (const QString &gn : groupNames)
    {
        QStringList vals = GetSizeGroupValues(gn);
        if (vals.isEmpty())
        {
            qCWarning(vOmtDoc) << "ComputeGrid: group" << gn << "has no values";
            return;
        }
        groupValues.append(vals);
    }

    // Get anchor bindings
    QMap<QString, QString> anchor = GetAnchorBindings();

    // Iterate over all measurements
    QStringList mNames = GetAllMNames();
    for (const QString &mName : mNames)
    {
        double anchorValue = GetMValue(mName);
        QMap<QString, GroupIncrement> incs = GetMIncrements(mName);

        // Precompute RangeGroup ranges for non-linear groups
        QMap<QString, QVector<RangeDef>> rangeLookup;
        for (auto incIt = incs.cbegin(); incIt != incs.cend(); ++incIt)
        {
            if (!incIt.value().isLinear && !incIt.value().rangeGroupName.isEmpty())
            {
                QVector<RangeGroup> rgs = GetRangeGroups(incIt.key());
                for (const auto &rg : rgs)
                {
                    if (rg.name == incIt.value().rangeGroupName)
                    {
                        rangeLookup[incIt.key()] = rg.ranges;
                        break;
                    }
                }
            }
        }

        // Precompute value→index maps for each group to avoid repeated indexOf
        QList<QHash<QString, int>> groupValueIndex;
        for (int d = 0; d < groupNames.size(); ++d)
        {
            QHash<QString, int> idxMap;
            const QStringList &vals = groupValues.at(d);
            for (int vi = 0; vi < vals.size(); ++vi)
            {
                idxMap.insert(vals.at(vi), vi);
            }
            groupValueIndex.append(idxMap);
        }

        // Compute the cartesian product of all group values
        QStringList currentCoord(groupNames.size());

        auto generate = [&](auto &self, int dim) -> void
        {
            if (dim >= groupNames.size())
            {
                // All dimensions fixed — compute and store cell
                QString coord = currentCoord.join(QLatin1Char(','));
                double value = anchorValue;

                for (int d = 0; d < groupNames.size(); ++d)
                {
                    const QString &gn = groupNames.at(d);
                    QString anchorVal = anchor.value(gn);

                    int currentIdx = groupValueIndex.at(d).value(currentCoord.at(d), -1);
                    int anchorIdx = groupValueIndex.at(d).value(anchorVal, -1);

                    if (currentIdx < 0)
                    {
                        qCWarning(vOmtDoc) << "ComputeGrid: value" << currentCoord.at(d)
                                           << "not found in group" << gn;
                        continue;
                    }
                    if (anchorIdx < 0)
                    {
                        qCWarning(vOmtDoc) << "ComputeGrid: anchor value" << anchorVal
                                           << "not found in group" << gn;
                        continue;
                    }

                    const GroupIncrement &gi = incs.value(gn);
                    int steps = currentIdx - anchorIdx;
                    if (steps != 0)
                    {
                        int direction = (steps > 0) ? 1 : -1;
                        const QStringList &gvals = groupValues.at(d);
                        for (int s = 0; s != steps; s += direction)
                        {
                            int srcIdx = anchorIdx + s;
                            if (srcIdx >= 0 && srcIdx < gvals.size())
                            {
                                double inc = FindIncrementForTransition(gi, gvals.at(srcIdx), gvals, rangeLookup.value(gn), direction);
                                value += inc * direction;
                            }
                        }
                    }
                }

                SetGridCell(mName, coord, value);
                return;
            }

            const QStringList &vals = groupValues.at(dim);
            for (const QString &v : vals)
            {
                currentCoord[dim] = v;
                self(self, dim + 1);
            }
        };

        generate(generate, 0);
    }

    qCDebug(vOmtDoc) << "ComputeGrid: computed grid for" << mNames.size()
                      << "measurements ×" << groupNames.size() << "groups";
}

// ===== Single-combination computation =====

auto OOmtDocument::ComputeValueForCombination(double anchorValue,
                                              const QMap<QString, GroupIncrement> &increments,
                                              const QMap<QString, QStringList> &groupValues,
                                              const QMap<QString, QString> &anchorBindings,
                                              const QMap<QString, QString> &selectedValues,
                                              const QMap<QString, QVector<RangeDef>> &rangeLookup) -> double
{
    double value = anchorValue;

    for (auto it = groupValues.cbegin(); it != groupValues.cend(); ++it)
    {
        const QString &group = it.key();
        const QStringList &vals = it.value();

        int selIdx = static_cast<int>(vals.indexOf(selectedValues.value(group)));
        int ancIdx = static_cast<int>(vals.indexOf(anchorBindings.value(group)));

        if (selIdx < 0 || ancIdx < 0 || selIdx == ancIdx)
        {
            continue;
        }

        const GroupIncrement &gi = increments.value(group);
        int direction = (selIdx > ancIdx) ? 1 : -1;

        // Cascade transition by transition from anchor to selected
        for (int i = ancIdx; i != selIdx; i += direction)
        {
            QString sourceVal = vals.at(i);
            double inc = FindIncrementForTransition(gi, sourceVal, vals, rangeLookup.value(group), direction);
            value += inc * direction;
        }
    }

    return value;
}

// ===== Forward compatibility =====

void OOmtDocument::AdoptUnknownElement(const QDomElement &el)
{
    m_unknownElements.append(el);
}

auto OOmtDocument::UnknownElements() const -> const QList<QDomElement> &
{
    return m_unknownElements;
}

// ===== Utilities =====

auto OOmtDocument::RootElement() const -> QDomElement
{
    return documentElement();
}

auto OOmtDocument::EnsureChildElement(QDomElement &parent, const QString &tagName) -> QDomElement
{
    QDomElement child = parent.firstChildElement(tagName);
    if (child.isNull())
    {
        child = createElement(tagName);
        parent.appendChild(child);
    }
    return child;
}

void OOmtDocument::SetChildText(QDomElement &parent, const QString &tagName, const QString &text)
{
    QDomElement child = parent.firstChildElement(tagName);
    if (child.isNull())
    {
        child = createElement(tagName);
        parent.appendChild(child);
    }
    // Replace or add text node
    auto textNode = child.firstChild();
    if (textNode.isNull() || !textNode.isText())
    {
        QDomText t = createTextNode(text);
        // Remove any existing child nodes
        while (!child.firstChild().isNull())
        {
            child.removeChild(child.firstChild());
        }
        child.appendChild(t);
    }
    else
    {
        textNode.setNodeValue(text);
    }
}

auto OOmtDocument::FindBindElement(const QString &groupName) const -> QDomElement
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return {};
    }
    QDomElement anchorEl = root.firstChildElement(TagAnchor);
    if (anchorEl.isNull())
    {
        return {};
    }
    QDomNodeList binds = anchorEl.elementsByTagName(TagBind);
    for (int i = 0; i < binds.count(); ++i)
    {
        QDomElement b = binds.at(i).toElement();
        if (b.attribute(AttrGroup) == groupName)
        {
            return b;
        }
    }
    return {};
}

auto OOmtDocument::ParseValuesString(const QString &csv) -> QStringList
{
    QStringList result;
    const QStringList parts = csv.split(QLatin1Char(','));
    for (const QString &part : parts)
    {
        QString trimmed = part.trimmed();
        if (!trimmed.isEmpty())
        {
            result.append(trimmed);
        }
    }
    return result;
}
