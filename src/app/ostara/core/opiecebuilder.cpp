/************************************************************************
 **
 **  @file   opiecebuilder.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  Cutting a piece out of a pattern: what travels with it, and what it will need elsewhere.
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

#include "opiecebuilder.h"

#include "def.h"
#include "vcontainer.h"
#include "vformula.h"
#include "variables/vinternalvariable.h"
#include "xml/vabstractpattern.h"
#include "xml/vpatternblockmapper.h"

#include <QCoreApplication>
#include <QDomDocument>
#include <QHash>
#include <QMap>
#include <QQueue>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace
{
const QString kAttrLibraryId = QStringLiteral("libraryId");
const QString kAttrCategory = QStringLiteral("category");

/** @brief Where a value read from outside the block comes from. */
struct Origin
{
    QString name{};
    QString libraryId{};
    QString category{};
};

/**
 * @brief Every token the expression cites.
 *
 * The formula walks its own tokens and hands each one to the map; leaving it unchanged is the same walk
 * with nothing rewritten, so the token list has a single author in VFormula.
 */
void CollectTokens(const QString &expression, QSet<QString> &tokens)
{
    VFormula::MapTokens(expression,
                        [&tokens](const QString &token)
                        {
                            tokens.insert(token);
                            return token;
                        });
}

/**
 * @brief The names an expression defines for itself.
 *
 * `#X = …` is an assignment the formula keeps to itself: it lives inside the evaluation and is gone
 * afterwards, so it is a definition and never something the piece must be given.
 */
auto LocalDefinitions(const QString &expression) -> QSet<QString>
{
    const QRegularExpression pattern(QRegularExpression::escape(CustomIncrSign)
                                     + QStringLiteral("([A-Za-z0-9_]+)\\s*="));

    QSet<QString> names;
    auto matches = pattern.globalMatch(expression);
    while (matches.hasNext())
    {
        names.insert(CustomIncrSign + matches.next().captured(1));
    }
    return names;
}

/** @brief Every object identifier the elements of @p element declare — their own `id`. */
void CollectDeclaredIds(const QDomElement &element, QSet<quint32> &ids)
{
    if (element.hasAttribute(VDomDocument::AttrId))
    {
        bool ok = false;
        const quint32 id = element.attribute(VDomDocument::AttrId).toUInt(&ok);
        if (ok)
        {
            ids.insert(id);
        }
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        CollectDeclaredIds(child, ids);
    }
}

/**
 * @brief Which block declares each object of the pattern.
 *
 * A declaration is never written over: two blocks can name the same object, and the one that *holds* it is
 * the one that declares it.
 */
void CollectOrigins(const QDomElement &root, QHash<quint32, Origin> &origins)
{
    for (QDomElement draw = root.firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        const Origin origin{draw.attribute(VAbstractPattern::AttrName),
                            draw.attribute(kAttrLibraryId),
                            draw.attribute(kAttrCategory)};

        QSet<quint32> ids;
        CollectDeclaredIds(draw, ids);

        for (const quint32 id : ids)
        {
            if (not origins.contains(id))
            {
                origins.insert(id, origin);
            }
        }
    }
}

/** @brief True when @p node sits inside @p block. */
auto IsInside(const QDomNode &node, const QDomElement &block) -> bool
{
    for (QDomNode parent = node; not parent.isNull(); parent = parent.parentNode())
    {
        if (parent == block)
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief The rows of @p section the piece takes, as a detached element of @p section's own document.
 *
 * Element by element, so a piece carries the increments it uses and nothing of the table it came from.
 */
auto CollectRows(const QDomElement &section, const QSet<QString> &names) -> QDomElement
{
    if (section.isNull() || names.isEmpty())
    {
        return {};
    }

    QDomDocument owner = section.ownerDocument();
    QDomElement collected = owner.createElement(section.tagName());

    for (QDomElement row = section.firstChildElement(VAbstractPattern::TagIncrement); not row.isNull();
         row = row.nextSiblingElement(VAbstractPattern::TagIncrement))
    {
        if (names.contains(row.attribute(VAbstractPattern::AttrName)))
        {
            collected.appendChild(row.cloneNode(true));
        }
    }

    return collected;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceBuilder::Build(VAbstractPattern *doc,
                          VContainer *data,
                          const QString &blockName,
                          const QString &name,
                          OPieceDocument *piece,
                          QString &error) -> bool
{
    if (doc == nullptr || data == nullptr || piece == nullptr)
    {
        return false;
    }

    const QDomElement block = doc->PatternBlockMapper()->GetElement(blockName);
    if (block.isNull())
    {
        error = QCoreApplication::translate("OPieceBuilder", "There is no block named %1.").arg(blockName);
        return false;
    }

    const QDomElement root = doc->documentElement();
    piece->Create(root.firstChildElement(VDomDocument::TagUnit).text());

    QSet<quint32> blockIds;
    CollectDeclaredIds(block, blockIds);

    QHash<quint32, Origin> origins;
    CollectOrigins(root, origins);

    // The formulas of the block first. Every increment the block turns out to need brings its own formula
    // with it, and an increment can lean on another one: the walk follows them until it settles.
    QQueue<QString> expressions;
    for (const VFormulaField &field : doc->ListExpressions())
    {
        if (IsInside(field.element, block))
        {
            expressions.enqueue(field.expression);
        }
    }

    QHash<QString, QString> incrementFormulas;
    for (const VFormulaField &field : doc->ListIncrementExpressions())
    {
        incrementFormulas.insert(field.element.attribute(VAbstractPattern::AttrName), field.expression);
    }

    const QHash<QString, QSharedPointer<VInternalVariable>> *variables = data->DataVariables();

    QSet<QString> carried;              // the increments the piece takes with it
    QMap<QString, Origin> dependencies; // the values it reads from elsewhere, and where they come from
    QSet<QString> walked;

    while (not expressions.isEmpty() && variables != nullptr)
    {
        const QString expression = expressions.dequeue();
        if (expression.isEmpty() || walked.contains(expression))
        {
            continue;
        }
        walked.insert(expression);

        QSet<QString> tokens;
        CollectTokens(expression, tokens);
        tokens.subtract(LocalDefinitions(expression));

        for (const QString &token : tokens)
        {
            const auto found = variables->constFind(token);
            if (found == variables->constEnd())
            {
                continue; // a function, a constant, or a value nobody defines
            }

            switch (found.value()->GetType())
            {
                case VarType::Measurement:
                case VarType::MeasurementSeparator:
                    // From the .omt: every pattern of the file shares them, they are not a piece's to carry.
                    break;

                case VarType::Increment:
                case VarType::IncrementSeparator:
                    if (not carried.contains(token))
                    {
                        carried.insert(token);
                        expressions.enqueue(incrementFormulas.value(token));
                    }
                    break;

                default:
                {
                    const QList<quint32> references = found.value()->GetReferences();
                    if (references.isEmpty())
                    {
                        break; // not built on an object of the pattern: nothing to place
                    }

                    bool blockOnly = true;
                    Origin origin;
                    for (const quint32 id : references)
                    {
                        if (blockIds.contains(id))
                        {
                            continue;
                        }

                        blockOnly = false;
                        if (const Origin known = origins.value(id); not known.name.isEmpty())
                        {
                            origin = known;
                        }
                    }

                    if (not blockOnly && not dependencies.contains(token))
                    {
                        dependencies.insert(token, origin);
                    }
                    break;
                }
            }
        }
    }

    QList<OPieceDocument::Dependency> needed;
    needed.reserve(dependencies.size());
    for (auto it = dependencies.constBegin(); it != dependencies.constEnd(); ++it)
    {
        needed.append(OPieceDocument::Dependency{it.key(), it.value().name, it.value().libraryId,
                                                 it.value().category});
    }

    // The groups come with the table that holds them; until the tables have any, the piece carries the
    // increments alone.
    piece->SetIncrements(CollectRows(root.firstChildElement(VAbstractPattern::TagIncrements), carried),
                         {},
                         CollectRows(root.firstChildElement(VAbstractPattern::TagPreviewCalculations), carried),
                         {});
    piece->SetDependencies(needed);

    // Last, because the block closes the sequence. Its identity is minted here and never moves again.
    piece->SetDraw(block, name, QUuid::createUuid().toString());

    return true;
}
