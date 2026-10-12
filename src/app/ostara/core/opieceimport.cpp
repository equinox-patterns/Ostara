/************************************************************************
 **
 **  @file   opieceimport.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  Bringing a piece of the library into a pattern: what it carries, and what the pattern already has.
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

#include "opieceimport.h"

#include "ifcdef.h"
#include "literals.h"
#include "opiecedocument.h"
#include "vcontainer.h"
#include "xml/vabstractpattern.h"
#include "xml/vpattern.h"

#include <QDomNode>

namespace
{
/// What a row says about itself, taken whole so two definitions can be compared without deciding anything.
struct RowDefinition
{
    QString formula{};
    QString description{};
    QString type{};
    QString specialUnits{};
};

auto DefinitionOf(const QDomElement &row) -> RowDefinition
{
    return {row.attribute(VAbstractPattern::AttrFormula), row.attribute(AttrDescription),
            row.attribute(VAbstractPattern::AttrType), row.attribute(VAbstractPattern::AttrSpecialUnits)};
}

auto SameDefinition(const RowDefinition &left, const RowDefinition &right) -> bool
{
    return left.formula == right.formula && left.description == right.description && left.type == right.type
           && left.specialUnits == right.specialUnits;
}

/**
 * @brief One table of the piece: what the pattern already holds, and what it takes.
 *
 * @p rows is the piece's own table, @p previewCalculation says which of the pattern's two tables this is.
 */
void ImportTable(const QDomElement &rows,
                 bool previewCalculation,
                 VPattern *host,
                 VContainer *data,
                 const QString &instanceName,
                 OPieceImport::IncrementReport &report)
{
    if (rows.isNull())
    {
        return;
    }

    // The group is created when the **first row really arrives**, and not before: importing the same piece a second
    // time adds nothing, and must therefore create no second group — the pattern keeps one group for that instance.
    quint32 groupId = NULL_ID;

    const auto GroupForNewRows = [&groupId, host, data, &instanceName, previewCalculation, &report]() -> quint32
    {
        if (groupId == NULL_ID)
        {
            groupId = host->InsertIncrementGroup(data->getNextId(), instanceName, previewCalculation);
            if (groupId != NULL_ID)
            {
                ++report.groupsAdded;
            }
        }

        return groupId;
    };

    for (QDomElement row = rows.firstChildElement(VAbstractPattern::TagIncrement); not row.isNull();
         row = row.nextSiblingElement(VAbstractPattern::TagIncrement))
    {
        const QString name = row.attribute(VAbstractPattern::AttrName);
        if (name.isEmpty())
        {
            continue;
        }

        // **The first import wins.** The row the pattern holds is left as it is, the piece's copy is dropped, and a
        // definition that differs is reported rather than arbitrated: one increment, one definition, shared.
        if (const QDomElement existing = host->IncrementRow(name, previewCalculation); not existing.isNull())
        {
            ++report.kept;

            if (not SameDefinition(DefinitionOf(existing), DefinitionOf(row)))
            {
                report.clashes.append({name, previewCalculation, true});
            }

            continue;
        }

        const auto type = StringToIncrementType(row.attribute(VAbstractPattern::AttrType));

        previewCalculation ? host->AddEmptyPreviewCalculation(name, type) : host->AddEmptyIncrement(name, type);

        if (type != IncrementType::Separator)
        {
            // A separator holds no formula: writing one would be forced back to '0' at the next reading.
            host->SetIncrementFormula(name, row.attribute(VAbstractPattern::AttrFormula));
        }

        if (row.attribute(VAbstractPattern::AttrSpecialUnits) == trueStr)
        {
            host->SetIncrementSpecialUnits(name, true);
        }

        if (const QString description = row.attribute(AttrDescription); not description.isEmpty())
        {
            host->SetIncrementDescription(name, description);
        }

        host->SetIncrementGroup(name, GroupForNewRows(), previewCalculation);

        ++report.added;
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImport::ImportIncrements(const OPieceDocument &piece,
                                    VPattern *host,
                                    VContainer *data,
                                    const QString &instanceName) -> IncrementReport
{
    IncrementReport report{};

    if (host == nullptr || data == nullptr || instanceName.isEmpty())
    {
        return report;
    }

    ImportTable(piece.Increments(), false, host, data, instanceName, report);
    ImportTable(piece.PreviewCalculations(), true, host, data, instanceName, report);

    return report;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceImport::SetOrigin(QDomElement &block, const OPieceDocument &piece)
{
    if (block.isNull() || piece.DrawElement().isNull())
    {
        return;
    }

    block.setAttribute(OPieceDocument::AttrLibraryId, piece.LibraryId());
    block.setAttribute(OPieceDocument::AttrCategory, piece.PieceName());
}
