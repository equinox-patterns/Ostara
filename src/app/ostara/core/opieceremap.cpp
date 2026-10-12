/************************************************************************
 **
 **  @file   opieceremap.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  Fresh identifiers for a piece about to enter a pattern: which slots carry one, and how they are given
 **          a new one.
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

#include "opieceremap.h"

#include "ifcdef.h"
#include "vcontainer.h"

#include <QFile>
#include <QHash>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

namespace
{
const QString kSchema = QStringLiteral("://schema/ostara/v1.0.0.xsd");

// ── What the schema cannot say ───────────────────────────────────────────────────────────────────────────────

/// The names no slot ever holds an object under, wherever they are written. Each is read for what it is by the
/// code: `spline/@duplicate` numbers a name (`VAbstractCurve::GetDuplicate`), `node/@reverse` and
/// `detail/@closed` are read into a `bool`, `detail/@pieceNumber` is the number the reader typed in the piece
/// dialog, `backgroudImage/@zValue` is a stacking rank, `thumbnail/@highestId` is the document's own counter,
/// `tools/@indexD1` and `tools/@indexD2` are edges of `VToolUnionDetails`, and `increment/@groupId` names a group
/// that belongs to the pattern, never to the piece.
auto NotIdentifiers() -> const QSet<QString> &
{
    static const QSet<QString> names{QStringLiteral("duplicate"),
                                     QStringLiteral("reverse"),
                                     QStringLiteral("closed"),
                                     QStringLiteral("zValue"),
                                     QStringLiteral("highestId"),
                                     QStringLiteral("pieceNumber"),
                                     QStringLiteral("cuttingNumber"),
                                     QStringLiteral("indexD1"),
                                     QStringLiteral("indexD2"),
                                     QStringLiteral("groupId")};
    return names;
}

/// The slots the schema types `xs:unsignedInt` while the code writes a number of its own in them:
/// `OPleatDisplay`'s `<item>` says `departure` (a place in the departure list), `place` (a line on it), `pleat`
/// (a rank among the pleats) and `end` (which end of that line, 0 or 1). Its neighbours on the same element —
/// `point`, `line`, `foldLine`, `copy`, `side1`, `side2`, `curve1`, `curve2`, `notch` — are identifiers.
auto Indexes() -> const QSet<QString> &
{
    static const QSet<QString> indexes{QStringLiteral("item/@departure"),
                                       QStringLiteral("item/@place"),
                                       QStringLiteral("item/@pleat"),
                                       QStringLiteral("item/@end")};
    return indexes;
}

/// The nodes a seam allowance group is made of, written as one comma-separated list — the only identifier slot
/// the schema types `xs:string`, and so the only one no type can find. `VToolSeamAllowance::AddSAMarginProperties`
/// joins them, `VPattern::ParseDetailInternals` splits them.
const QString AttrNodeIds = QStringLiteral("nodeIds");

// ── The schema, asked once ───────────────────────────────────────────────────────────────────────────────────

/// The type a declaration carries, without its prefix — the schema writes `xs:unsignedInt`.
auto DeclaredType(const QXmlStreamAttributes &attributes) -> QStringView
{
    const QStringView type = attributes.value(QStringLiteral("type"));
    const qsizetype colon = type.lastIndexOf(QLatin1Char(':'));
    return colon < 0 ? type : type.mid(colon + 1);
}

struct Slots
{
    QSet<QString> attributes{}; ///< `element/@attribute`, the slots that hold an identifier.
    QSet<QString> elements{};   ///< The elements whose text holds one.
};

/**
 * @brief Reads the pattern schema: every slot it types `xs:unsignedInt`, and every declaration that says the same
 * name is something else on the same element.
 *
 * Both are needed, because a name of the schema can be an identifier in one place and a formula, a flag or a file
 * name in another — `center` on `<mirrorLine>`, `foldLine` and `notch` on `<operation>`, `path` on
 * `<backgroudImage>`. Every declaration of the schema sits inside the `<xs:element>` that carries it, which is
 * what lets the walk name a slot by its element; the named types (`senseType`, `axisType`, `crossType`…) are
 * never `xs:unsignedInt`, so the enumerations that hold values in the same 1..N range as the identifiers fall
 * away by themselves, without being named.
 */
auto ReadSchema() -> Slots
{
    Slots found;

    QFile schema(kSchema);
    if (not schema.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "OPieceRemap: the pattern schema could not be read:" << kSchema;
        return found;
    }

    QSet<QString> identifiers;
    QSet<QString> others;
    QVector<QString> owners;

    QXmlStreamReader reader(&schema);
    while (not reader.atEnd())
    {
        const QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::EndElement)
        {
            if (reader.name() == u"element" && not owners.isEmpty())
            {
                owners.removeLast();
            }
            continue;
        }

        if (token != QXmlStreamReader::StartElement)
        {
            continue;
        }

        const QStringView tag = reader.name();
        const QXmlStreamAttributes attributes = reader.attributes();
        const QString name = attributes.value(QStringLiteral("name")).toString();

        if (tag == u"element")
        {
            if (DeclaredType(attributes) == u"unsignedInt" && not name.isEmpty()
                && not NotIdentifiers().contains(name))
            {
                found.elements.insert(name);
            }
            owners.append(name);
            continue;
        }

        if (tag != u"attribute" || name.isEmpty() || owners.isEmpty() || owners.last().isEmpty())
        {
            continue;
        }

        const QStringView type = DeclaredType(attributes);
        if (type.isEmpty())
        {
            continue;
        }

        const QString slot = owners.last() + QStringLiteral("/@") + name;
        if (type != u"unsignedInt")
        {
            others.insert(slot);
        }
        else if (not NotIdentifiers().contains(name))
        {
            identifiers.insert(slot);
        }
    }

    if (reader.hasError())
    {
        qWarning() << "OPieceRemap: the pattern schema could not be parsed:" << reader.errorString();
    }

    found.attributes = identifiers;
    found.attributes.subtract(others);
    found.attributes.subtract(Indexes());
    return found;
}

auto PatternSlots() -> const Slots &
{
    static const Slots schema = ReadSchema();
    return schema;
}

// ── Walking the block ────────────────────────────────────────────────────────────────────────────────────────

/// The fresh value of one identifier, and the same value every time it is met again. What is remembered is the
/// old identifier, not the place it is written: a reference can come before the declaration it names, and the two
/// still have to agree. Zero is not an identifier — it means "no object" and stays where it is.
auto FreshValue(const QString &value,
                VContainer *pattern,
                QHash<quint32, quint32> &fresh,
                OPieceRemap::Report &report) -> QString
{
    bool isNumber = false;
    const quint32 id = value.toUInt(&isNumber);
    if (not isNumber || id == NULL_ID)
    {
        return value;
    }

    ++report.written;

    const auto known = fresh.constFind(id);
    if (known != fresh.constEnd())
    {
        return QString::number(known.value());
    }

    const quint32 replacement = pattern->getNextId();
    fresh.insert(id, replacement);
    return QString::number(replacement);
}

/// The same, for the one slot that holds a list of them.
auto FreshList(const QString &value,
               VContainer *pattern,
               QHash<quint32, quint32> &fresh,
               OPieceRemap::Report &report) -> QString
{
    const QStringList ids = value.split(QLatin1Char(','));
    QStringList replacements;
    replacements.reserve(ids.size());
    for (const QString &id : ids)
    {
        replacements.append(FreshValue(id.trimmed(), pattern, fresh, report));
    }
    return replacements.join(QLatin1Char(','));
}

/// Writes @p value as the text of @p element. The elements concerned hold that number and nothing else — the
/// schema types them `xs:unsignedInt` — so there is a single text node to overwrite; an element that holds
/// elements instead (a `<record>` of internal paths carries its id in an attribute) is left alone.
auto SetText(QDomElement &element, const QString &value) -> void
{
    for (QDomNode node = element.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (node.isText() || node.isCDATASection())
        {
            node.setNodeValue(value);
            return;
        }
    }
    element.appendChild(element.ownerDocument().createTextNode(value));
}

auto RemapAttributes(QDomElement &element,
                     VContainer *pattern,
                     QHash<quint32, quint32> &fresh,
                     OPieceRemap::Report &report) -> void
{
    const QDomNamedNodeMap attributes = element.attributes();
    for (int i = 0; i < attributes.count(); ++i)
    {
        QDomAttr attribute = attributes.item(i).toAttr();
        if (attribute.isNull())
        {
            continue;
        }

        const QString name = attribute.name();
        QString value;
        if (name == AttrNodeIds)
        {
            value = FreshList(attribute.value(), pattern, fresh, report);
        }
        else if (OPieceRemap::IdSlots().contains(element.tagName() + QStringLiteral("/@") + name))
        {
            value = FreshValue(attribute.value(), pattern, fresh, report);
        }
        else
        {
            continue;
        }

        if (value != attribute.value())
        {
            attribute.setValue(value);
        }
    }
}

auto RemapElement(QDomElement &element, VContainer *pattern, QHash<quint32, quint32> &fresh, OPieceRemap::Report &report)
    -> void
{
    RemapAttributes(element, pattern, fresh, report);

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        if (OPieceRemap::IdElements().contains(child.tagName()))
        {
            const QString value = child.text().trimmed();
            const QString replacement = FreshValue(value, pattern, fresh, report);
            if (replacement != value)
            {
                SetText(child, replacement);
            }
        }

        RemapElement(child, pattern, fresh, report);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceRemap::IdSlots() -> const QSet<QString> &
{
    return PatternSlots().attributes;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceRemap::IdElements() -> const QSet<QString> &
{
    return PatternSlots().elements;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceRemap::RemapIds(QDomElement &block, VContainer *pattern) -> Report
{
    Report report;
    if (block.isNull() || pattern == nullptr)
    {
        return report;
    }

    QHash<quint32, quint32> fresh;
    RemapElement(block, pattern, fresh, report);
    report.ids = static_cast<int>(fresh.size());
    return report;
}
