/************************************************************************
 **
 **  @file   opieceremap.h
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

#ifndef OPIECEREMAP_H
#define OPIECEREMAP_H

#include <QDomElement>
#include <QSet>
#include <QString>

class VContainer;

/**
 * @brief The identifier slots of a piece's `<draw>` block, and the fresh values they take.
 *
 * A block cut from one pattern and read into another carries the identifiers of the document it came from. Left
 * as they are, they name objects of the host — or name nothing at all: two symptoms, one cause. Every slot that
 * holds an identifier is therefore given a fresh one, **the same replacement everywhere**, so that the block
 * goes on pointing at itself.
 *
 * The slots come from the **pattern schema** (`://schema/ostara/v1.0.0.xsd`), never from a list written by hand:
 * every attribute the schema types `xs:unsignedInt` is one, and so is the text of every element it types that
 * way — `<record>` of a pin or of a place label, `<child>` of the children a union names. No name is ever
 * tested for the word `id`: read that way the rule also catches `width` and `hidden`; read by value it catches
 * every angle and every length. The named types are what makes the schema the authority — an enumeration is a
 * named type, so it is never `xs:unsignedInt` and falls away by construction, while the enumerations that
 * *are* typed `xs:unsignedInt` are named one by one below.
 *
 * What the schema cannot say is kept here, and only that:
 *  - the counters and flags it types like an identifier — `duplicate`, `reverse`, `closed`, `zValue`, `highestId`,
 *    `pieceNumber`, `cuttingNumber`, `indexD1`, `indexD2`, and the group a row belongs to (`groupId`, which is
 *    an identifier elsewhere but belongs to the host, never to a piece);
 *  - the slots where the code stores an index or a small enumeration **while the schema calls it an id** —
 *    `OPleatDisplay` writes `departure`, `place`, `pleat` and `end` as numbers of its own choosing;
 *  - the node list of a seam allowance group, written as one comma-separated string under an attribute the
 *    schema types `xs:string` (`nodeIds`), which no type can find.
 *
 * The caller owes the order: the block is rewritten **before** it is inserted, and the pattern it is entering is
 * what hands out the fresh values (`VContainer::getNextId`).
 */
namespace OPieceRemap
{
/**
 * @brief The slots that carry an identifier, as `element/@attribute` — read from the pattern schema.
 *
 * What the schema declares, minus what only the code can say. An identifier written by a tool under a slot this
 * set does not name is left alone, which is harmless: the host re-validates the whole pattern at its next save,
 * so a slot the schema does not allow cannot survive anyway.
 */
auto IdSlots() -> const QSet<QString> &;

/**
 * @brief The names of the elements whose **text** is an identifier, read from the pattern schema.
 *
 * Only two, and both are guarded when they are read: `<record>` and `<child>` also appear as containers holding
 * attributes and nothing else, and an element that holds no number is left untouched.
 */
auto IdElements() -> const QSet<QString> &;

/// What a rewrite did: the identifiers that were given a fresh value, and the places that were rewritten.
struct Report
{
    int ids = 0;     ///< Distinct identifiers met — never a new value twice.
    int written = 0; ///< Places rewritten, one count per occurrence.
};

/**
 * @brief Gives every identifier of @p block a fresh one, drawn from @p pattern, the same replacement everywhere.
 * @param block the `<draw>` block, rewritten in place.
 * @param pattern the pattern the block is about to enter; it hands out the fresh values.
 * @return what was rewritten. A block that is null, or a null pattern, rewrites nothing.
 */
auto RemapIds(QDomElement &block, VContainer *pattern) -> Report;
} // namespace OPieceRemap

#endif // OPIECEREMAP_H
