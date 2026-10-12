/************************************************************************
 **
 **  @file   opiecedocument.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  A piece of the library: the `.opiece` file, held the way a pattern holds its `.ostara`.
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

#include "opiecedocument.h"

#include <QDomDocument>
#include <QStringList>

namespace
{
const QString kTagPiece = QStringLiteral("piece");
const QString kTagVersion = QStringLiteral("version");
const QString kTagUnit = QStringLiteral("unit");
const QString kTagIncrements = QStringLiteral("increments");
const QString kTagIncrementGroups = QStringLiteral("incrementGroups");
const QString kTagPreviewCalculations = QStringLiteral("previewCalculations");
const QString kTagPreviewCalculationGroups = QStringLiteral("previewCalculationGroups");
const QString kTagDependencies = QStringLiteral("dependencies");
const QString kTagDependency = QStringLiteral("dependency");
const QString kTagDraw = QStringLiteral("draw");

const QString kAttrName = QStringLiteral("name");
const QString kAttrLibraryId = QStringLiteral("libraryId");
const QString kAttrToken = QStringLiteral("token");
const QString kAttrOrigin = QStringLiteral("origin");
const QString kAttrCategory = QStringLiteral("category");

const QString kPieceVersion = QStringLiteral("1.0.0");

/**
 * @brief The order of `<piece>`'s children, from the `<xs:sequence>` of opiece/v1.0.0.xsd.
 *
 * Written once, here, and read by Section() alone. The document is validated against that schema before
 * every save, so a section out of this order is not a piece that reads oddly — it is a piece that cannot
 * be written at all.
 */
auto SectionOrder() -> const QStringList &
{
    static const QStringList order{kTagVersion,
                                   kTagUnit,
                                   kTagIncrements,
                                   kTagIncrementGroups,
                                   kTagPreviewCalculations,
                                   kTagPreviewCalculationGroups,
                                   kTagDependencies,
                                   kTagDraw};
    return order;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPieceDocument::OPieceDocument(QObject *parent)
    : VDomDocument(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::SaveSchema() const -> QString
{
    return QStringLiteral("://schema/opiece/v1.0.0.xsd");
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDocument::Create(const QString &unit)
{
    clear();
    appendChild(createElement(kTagPiece));

    QDomElement version = Section(kTagVersion);
    version.appendChild(createTextNode(kPieceVersion));

    QDomElement unitElement = Section(kTagUnit);
    unitElement.appendChild(createTextNode(unit));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::ExistingSection(const QString &tag) const -> QDomElement
{
    const QDomElement root = documentElement();
    if (root.isNull())
    {
        return {};
    }

    return root.firstChildElement(tag);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::Section(const QString &tag) -> QDomElement
{
    QDomElement root = documentElement();
    if (root.isNull())
    {
        return {};
    }

    if (const QDomElement existing = ExistingSection(tag); not existing.isNull())
    {
        return existing;
    }

    QDomElement element = createElement(tag);

    // Before the first section that comes after it: appending would put it past the block.
    const QStringList &order = SectionOrder();
    for (qsizetype i = order.indexOf(tag) + 1; i < order.size(); ++i)
    {
        if (const QDomElement next = root.firstChildElement(order.at(i)); not next.isNull())
        {
            root.insertBefore(element, next);
            return element;
        }
    }

    root.appendChild(element);
    return element;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::Unit() const -> QString
{
    return ExistingSection(kTagUnit).text();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDocument::SetDraw(const QDomElement &source, const QString &name, const QString &libraryId)
{
    if (source.isNull())
    {
        return;
    }

    QDomElement root = documentElement();
    if (const QDomElement previous = ExistingSection(kTagDraw); not previous.isNull())
    {
        root.removeChild(previous);
    }

    QDomElement imported = importNode(source, true).toElement();
    imported.setAttribute(kAttrName, name);
    imported.setAttribute(kAttrLibraryId, libraryId);

    // The block is the last section of the sequence, so appending is inserting in order.
    root.appendChild(imported);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::DrawElement() const -> QDomElement
{
    return ExistingSection(kTagDraw);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::PieceName() const -> QString
{
    return DrawElement().attribute(kAttrName);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::LibraryId() const -> QString
{
    return DrawElement().attribute(kAttrLibraryId);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDocument::SetIncrements(const QDomElement &increments,
                                   const QDomElement &incrementGroups,
                                   const QDomElement &previewCalculations,
                                   const QDomElement &previewCalculationGroups)
{
    const auto copyRows = [this](const QDomElement &source, const QString &tag)
    {
        if (source.isNull())
        {
            return; // Nothing to carry: the section stays out, as the schema allows.
        }

        QDomElement target = Section(tag);
        RemoveAllChildren(target);

        for (QDomNode row = source.firstChild(); not row.isNull(); row = row.nextSibling())
        {
            target.appendChild(importNode(row, true));
        }
    };

    copyRows(increments, kTagIncrements);
    copyRows(incrementGroups, kTagIncrementGroups);
    copyRows(previewCalculations, kTagPreviewCalculations);
    copyRows(previewCalculationGroups, kTagPreviewCalculationGroups);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::Increments() const -> QDomElement
{
    return ExistingSection(kTagIncrements);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::IncrementGroups() const -> QDomElement
{
    return ExistingSection(kTagIncrementGroups);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::PreviewCalculations() const -> QDomElement
{
    return ExistingSection(kTagPreviewCalculations);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::PreviewCalculationGroups() const -> QDomElement
{
    return ExistingSection(kTagPreviewCalculationGroups);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceDocument::SetDependencies(const QList<Dependency> &dependencies)
{
    if (dependencies.isEmpty())
    {
        return; // A piece that needs nothing from elsewhere says nothing.
    }

    QDomElement target = Section(kTagDependencies);
    RemoveAllChildren(target);

    for (const Dependency &dependency : dependencies)
    {
        QDomElement element = createElement(kTagDependency);
        element.setAttribute(kAttrToken, dependency.token);
        element.setAttribute(kAttrOrigin, dependency.origin);

        if (not dependency.libraryId.isEmpty())
        {
            element.setAttribute(kAttrLibraryId, dependency.libraryId);
        }
        if (not dependency.category.isEmpty())
        {
            element.setAttribute(kAttrCategory, dependency.category);
        }

        target.appendChild(element);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceDocument::Dependencies() const -> QList<Dependency>
{
    QList<Dependency> dependencies;

    const QDomElement section = ExistingSection(kTagDependencies);
    for (QDomElement element = section.firstChildElement(kTagDependency); not element.isNull();
         element = element.nextSiblingElement(kTagDependency))
    {
        dependencies.append(Dependency{element.attribute(kAttrToken),
                                       element.attribute(kAttrOrigin),
                                       element.attribute(kAttrLibraryId),
                                       element.attribute(kAttrCategory)});
    }

    return dependencies;
}
