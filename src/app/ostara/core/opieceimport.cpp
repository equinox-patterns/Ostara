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
#include "vmisc/vabstractvalapplication.h"
#include "xml/vabstractpattern.h"
#include "xml/vpattern.h"

#include <QCoreApplication>
#include <QDomNode>

namespace
{
/// The units a piece may carry — `units` of the piece schema, which the pattern schema shares. Read as words,
/// never through `StrToUnits`: that one answers centimetres to anything it does not know, and an unreadable unit
/// would then pass for a piece in centimetres.
auto PieceUnits() -> const QHash<QString, Unit> &
{
    static const QHash<QString, Unit> units{{unitMM, Unit::Mm}, {unitCM, Unit::Cm}, {unitINCH, Unit::Inch}};
    return units;
}

/// The unit @p piece says its lengths are written in, or `Unit::LAST_UNIT_DO_NOT_USE` when it says nothing, or
/// says something no piece may carry.
auto UnitOf(const OPieceDocument &piece) -> Unit
{
    return OPieceImport::PieceUnit(piece.Unit());
}

/// The unit of the pattern the piece would enter: the unit of the **file**, and the one the application is
/// working in when the file does not say yet — a pattern in memory has not been written, and its unit is the
/// unit of the work.
auto UnitOf(const VAbstractPattern *host) -> Unit
{
    if (host != nullptr && host->Units() != Unit::LAST_UNIT_DO_NOT_USE)
    {
        return host->Units();
    }

    return VAbstractValApplication::VApp() == nullptr ? Unit::Cm : VAbstractValApplication::VApp()->patternUnits();
}

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
auto OPieceImport::ImportLineNames(const OPieceDocument &piece,
                                   VContainer *data,
                                   const QHash<QString, QString> &renamed) -> LineNameReport
{
    LineNameReport report{};
    if (data == nullptr)
    {
        return report;
    }

    const auto Renamed = [&renamed](const QString &name) -> QString { return renamed.value(name, name); };

    for (QDomElement row = piece.LineNames().firstChildElement(VDomDocument::TagLine); not row.isNull();
         row = row.nextSiblingElement(VDomDocument::TagLine))
    {
        const QStringList pair = row.attribute(AttrPoints).split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const QString name = Renamed(row.attribute(AttrName));
        if (pair.size() != 2 || name.isEmpty())
        {
            continue;
        }

        const QString key = Renamed(pair.at(0)) + '_' + Renamed(pair.at(1));
        if (const QString held = data->LineRoleName(key); not held.isEmpty())
        {
            if (held == name)
            {
                ++report.kept;
            }
            else
            {
                report.clashes.append({row.attribute(AttrPoints), name, false, held});
            }
            continue;
        }

        // The name has to be free before the pair is filed: the pattern refuses a second line under a name
        // already taken — it says so at parse time and drops the entry — so a name that is taken is reported
        // rather than written.
        const QHash<QString, QString> &roles = data->LineRoleNames();
        QString owner;
        for (auto it = roles.constBegin(); it != roles.constEnd(); ++it)
        {
            if (it.value() == name)
            {
                owner = it.key();
                break;
            }
        }
        if (not owner.isEmpty())
        {
            report.clashes.append({row.attribute(AttrPoints), name, true, owner});
            continue;
        }

        data->SetLineRoleName(Renamed(pair.at(0)), Renamed(pair.at(1)), name);
        ++report.added;
    }

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

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImport::CheckUnit(const OPieceDocument &piece, const VAbstractPattern *host, QString &error) -> bool
{
    const QString carried = piece.Unit().trimmed();
    const Unit pieceUnit = UnitOf(piece);
    if (pieceUnit == Unit::LAST_UNIT_DO_NOT_USE)
    {
        error = carried.isEmpty()
                    ? QCoreApplication::translate("OPieceImport",
                                                  "The piece does not say which unit its lengths are written in, "
                                                  "and a piece is written in millimetres, centimetres or inches. "
                                                  "It cannot be read as it stands.")
                    : QCoreApplication::translate("OPieceImport",
                                                  "The piece says its lengths are written in %1, which is not a "
                                                  "unit a piece may carry: a piece is written in millimetres, "
                                                  "centimetres or inches. It cannot be read as it stands.")
                          .arg(carried);
        return false;
    }

    const Unit hostUnit = UnitOf(host);
    if (pieceUnit != hostUnit)
    {
        error = QCoreApplication::translate("OPieceImport",
                                            "The piece is in %1 and this pattern is in %2. A pattern holds one "
                                            "unit from end to end, and a piece is never converted: import it into "
                                            "a pattern of its own unit.")
                    .arg(UnitsToStr(pieceUnit, true), UnitsToStr(hostUnit, true));
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImport::PieceUnit(const QString &unit) -> Unit
{
    return PieceUnits().value(unit.trimmed(), Unit::LAST_UNIT_DO_NOT_USE);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImport::HostUnit(const VAbstractPattern *host) -> Unit
{
    return UnitOf(host);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImport::CheckMeasurements(const OPieceDocument &piece, const VAbstractPattern *host, QString &error) -> bool
{
    Q_UNUSED(piece)

    if (host == nullptr || not host->MPath().isEmpty())
    {
        return true;
    }

    error = QCoreApplication::translate("OPieceImport",
                                        "This pattern has no measurement table yet, and a piece is read against "
                                        "one: its formulas are evaluated with the measurements of the table. Load "
                                        "a measurement table, then insert the piece.");
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceImport::CheckDependencies(const OPieceDocument &piece, const VAbstractPattern *host, QString &error) -> bool
{
    if (host == nullptr)
    {
        return true;
    }

    // The blocks of the host, in the order the document holds them: what a value can be read back from.
    QList<QDomElement> blocks;
    const QDomElement root = host->documentElement();
    for (QDomElement draw = root.firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        blocks.append(draw);
    }

    // A candidate is looked for by the key that survives the longest first: the identity a piece keeps when it is
    // renamed in the library, then the name it wore, and only then the name of the block — which a block drawn by
    // hand carries alone.
    const auto held = [&blocks](const QString &attribute, const QString &value)
    {
        if (value.isEmpty())
        {
            return false;
        }

        for (const QDomElement &draw : blocks)
        {
            if (draw.attribute(attribute) == value)
            {
                return true;
            }
        }
        return false;
    };

    for (const OPieceDocument::Dependency &dependency : piece.Dependencies())
    {
        if (held(OPieceDocument::AttrLibraryId, dependency.libraryId)
            || held(OPieceDocument::AttrCategory, dependency.category)
            || held(VAbstractPattern::AttrName, dependency.origin))
        {
            continue;
        }

        // Nothing holds it. What is missing is said by the value itself when nothing names it: the one who reads
        // the message has to be able to go and look for the piece it comes from.
        const QString missing = dependency.origin.isEmpty() ? dependency.token : dependency.origin;
        error = QCoreApplication::translate("OPieceImport",
                                            "This piece reads %1, and no piece of this pattern declares it. "
                                            "Import the piece it comes from first.")
                    .arg(missing);
        return false;
    }

    return true;
}
