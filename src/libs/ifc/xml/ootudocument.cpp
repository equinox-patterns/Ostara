/************************************************************************
 **
 **  @file   ootudocument.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  In-memory model for .otu (Ostara Taille Unique) measurement files.
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

#include "ootudocument.h"
#include "../../vmisc/def.h"
#include "../exception/vexceptionobjecterror.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(vOtuDoc, "v.otu.doc")

// ----- Root -----
const QString OOtuDocument::TagOtu = QStringLiteral("otu");
const QString OOtuDocument::AttrVersion = QStringLiteral("version");
const QString OOtuDocument::TagName = QStringLiteral("name");

// ----- Unit -----
const QString OOtuDocument::TagUnit = QStringLiteral("unit");

// ----- System -----
const QString OOtuDocument::TagSystem = QStringLiteral("system");

// ----- Measurements -----
const QString OOtuDocument::TagMeasurements = QStringLiteral("measurements");
const QString OOtuDocument::TagM = QStringLiteral("m");
const QString OOtuDocument::TagValue = QStringLiteral("value");
const QString OOtuDocument::AttrName = QStringLiteral("name");
const QString OOtuDocument::AttrFullName = QStringLiteral("fullName");
const QString OOtuDocument::AttrDescription = QStringLiteral("description");
const QString OOtuDocument::AttrType = QStringLiteral("type");
const QString OOtuDocument::AttrSpecialUnits = QStringLiteral("specialUnits");
const QString OOtuDocument::AttrCollapsed = QStringLiteral("collapsed");
const QString OOtuDocument::AttrValue = QStringLiteral("value");

// ===== Constructor =====

OOtuDocument::OOtuDocument(QObject *parent)
  : VDomDocument(parent)
{
}

OOtuDocument::~OOtuDocument() = default;

// ===== Factory =====

auto OOtuDocument::CreateEmptyFile(Unit unit) -> bool
{
    clear();
    m_unknownElements.clear();

    QDomNode xmlHeader = createProcessingInstruction("xml", R"(version="1.0" encoding="UTF-8")");
    appendChild(xmlHeader);

    QDomElement root = createElement(TagOtu);
    root.setAttribute(AttrVersion, QString::fromLatin1(CurrentFormatVersion));
    appendChild(root);

    // Unit element
    QDomElement unitEl = createElement(TagUnit);
    QDomText unitText = createTextNode(UnitsToStr(unit));
    unitEl.appendChild(unitText);
    root.appendChild(unitEl);

    // Empty measurements
    QDomElement measEl = createElement(TagMeasurements);
    root.appendChild(measEl);

    qCDebug(vOtuDoc) << "Created empty .otu document v" << CurrentFormatVersion;
    return true;
}

// ===== I/O =====

void OOtuDocument::setXMLContent(const QString &fileName)
{
    VDomDocument::setXMLContent(fileName);

    // Validate root tag
    QDomElement root = RootElement();
    if (root.isNull() || root.tagName() != TagOtu)
    {
        QString tag = root.isNull() ? QStringLiteral("(null)") : root.tagName();
        throw VExceptionObjectError(tr("File is not a valid Ostara measurement table. "
                             "Expected root: <%1>, got: <%2>.")
                             .arg(TagOtu, tag));
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
                    tag != TagSystem && tag != TagMeasurements)
                {
                    qCInfo(vOtuDoc) << "Preserving unknown element:" << tag;
                    m_unknownElements.append(el);
                }
            }
            child = child.nextSibling();
        }
    }
}

auto OOtuDocument::SaveDocument(const QString &fileName, QString &error) -> bool
{
    // Re-attach unknown elements before saving
    QDomElement root = RootElement();
    for (const auto &el : m_unknownElements)
    {
        root.appendChild(el);
    }

    return VDomDocument::SaveDocument(fileName, error);
}

auto OOtuDocument::GetFormatVersionStr() const -> QString
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return QString::fromLatin1(CurrentFormatVersion);
    }
    return root.attribute(AttrVersion, QString::fromLatin1(CurrentFormatVersion));
}

// ===== Table Name =====

auto OOtuDocument::GetTableName() const -> QString
{
    return UniqueTagText(TagName);
}

void OOtuDocument::SetTableName(const QString &name)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    SetChildText(root, TagName, name);
}

// ===== Unit =====

auto OOtuDocument::GetUnits() const -> Unit
{
    QString unitStr = UniqueTagText(TagUnit);
    return StrToUnits(unitStr);
}

void OOtuDocument::SetUnits(Unit unit)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    SetChildText(root, TagUnit, UnitsToStr(unit));
}

// ===== System =====

auto OOtuDocument::GetSystem() const -> QString
{
    return UniqueTagText(TagSystem);
}

void OOtuDocument::SetSystem(const QString &uuid)
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return;
    }
    if (uuid.isEmpty())
    {
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

// ===== Measurements =====

auto OOtuDocument::MeasurementsElement() const -> QDomElement
{
    QDomElement root = RootElement();
    if (root.isNull())
    {
        return {};
    }
    return root.firstChildElement(TagMeasurements);
}

auto OOtuDocument::FindMElement(const QString &name) const -> QDomElement
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

auto OOtuDocument::GetMCount() const -> int
{
    QDomElement measEl = MeasurementsElement();
    if (measEl.isNull())
    {
        return 0;
    }
    return measEl.elementsByTagName(TagM).count();
}

auto OOtuDocument::GetAllMNames() const -> QStringList
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

auto OOtuDocument::GetMFullName(const QString &name) const -> QString
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return {};
    }
    return mEl.attribute(AttrFullName);
}

auto OOtuDocument::GetMDescription(const QString &name) const -> QString
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return {};
    }
    return mEl.attribute(AttrDescription);
}

auto OOtuDocument::GetMType(const QString &name) const -> QString
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return QStringLiteral("measurement");
    }
    return mEl.attribute(AttrType, QStringLiteral("measurement"));
}

auto OOtuDocument::GetMValue(const QString &name) const -> double
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        qCWarning(vOtuDoc) << "GetMValue: measurement" << name << "not found";
        return 0.0;
    }
    QDomElement valEl = mEl.firstChildElement(TagValue);
    if (valEl.isNull())
    {
        qCWarning(vOtuDoc) << "GetMValue: no <value> element for measurement" << name;
        return 0.0;
    }
    // The value is stored as TEXT content of <value>, not as an attribute
    QString text = valEl.text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    double v = text.toDouble(&ok);
    return ok ? v : 0.0;
}

auto OOtuDocument::GetMSpecialUnits(const QString &name) const -> bool
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return false;
    }
    return mEl.attribute(AttrSpecialUnits, QStringLiteral("false")) == QStringLiteral("true");
}

auto OOtuDocument::GetMCollapsed(const QString &name) const -> bool
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return false;
    }
    return mEl.attribute(AttrCollapsed, QStringLiteral("false")) == QStringLiteral("true");
}

void OOtuDocument::SetMCollapsed(const QString &name, bool collapsed)
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

void OOtuDocument::AddM(const QString &name, const QString &fullName, const QString &description,
                         double value, const QString &type, bool specialUnits)
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
    QDomText valText = createTextNode(QString::number(value, 'f', 2));
    valEl.appendChild(valText);
    mEl.appendChild(valEl);

    measEl.appendChild(mEl);
}

void OOtuDocument::SetMValue(const QString &name, double value)
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return;
    }
    SetChildText(mEl, TagValue, QString::number(value, 'f', 2));
}

void OOtuDocument::RemoveM(const QString &name)
{
    QDomElement mEl = FindMElement(name);
    if (mEl.isNull())
    {
        return;
    }
    QDomElement measEl = MeasurementsElement();
    measEl.removeChild(mEl);
}

// ===== Forward compatibility =====

void OOtuDocument::AdoptUnknownElement(const QDomElement &el)
{
    m_unknownElements.append(el);
}

auto OOtuDocument::UnknownElements() const -> const QList<QDomElement> &
{
    return m_unknownElements;
}

// ===== Utilities =====

auto OOtuDocument::RootElement() const -> QDomElement
{
    return documentElement();
}

auto OOtuDocument::EnsureChildElement(QDomElement &parent, const QString &tagName) -> QDomElement
{
    QDomElement child = parent.firstChildElement(tagName);
    if (child.isNull())
    {
        child = createElement(tagName);
        parent.appendChild(child);
    }
    return child;
}

void OOtuDocument::SetChildText(QDomElement &parent, const QString &tagName, const QString &text)
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
