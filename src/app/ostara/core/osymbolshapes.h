/************************************************************************
 **
 **  @file   osymbolshapes.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The marks Ostara knows how to draw, and the words they are
 **          shown under — one list, read by both.
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

#ifndef OSYMBOLSHAPES_H
#define OSYMBOLSHAPES_H

#include <QString>
#include <QVector>

#include <optional>

#include "vgeometry/vgeometrydef.h"
#include "vmisc/def.h"

/**
 * @brief The marks that can be drawn, by family, and how they are named.
 *
 * **A mark is offered under the same words wherever it is offered.** A notch is named once here and
 * the Details dock and the symbol library both read that name, so a translator writes "T mark" once
 * and no user ever sees the same mark called two things. The tables below are the docks' own,
 * moved rather than copied, and they keep the translation contexts they have always been looked up
 * under — which is why the words do not change and nothing has to be retranslated.
 *
 * **A shape is named by its family and its type, never by a place in a table.** An index file that
 * stored "the seventh entry" would break the day a type is inserted; one that stores the type
 * itself still reads years later.
 *
 * What a shape *is* — its geometry — is not here: this is the vocabulary, and @ref OSymbolRender is
 * the drawing. The two meet in @ref Shape.
 */
namespace OSymbolShapes
{
/** @brief Which family of marks a shape belongs to. */
enum class Family
{
    Grainline, ///< the line that follows the fabric's straight grain
    FoldLine,  ///< the two arrows that say "cut on the fold"
    Passmark,  ///< a notch cut into the seam allowance
    PlaceLabel ///< a mark placed inside a piece
};

/**
 * @brief A notch type, and the words it is shown under.
 *
 * **The clockwise opening belongs to the entry, not to the type.** A "Check mark" exists in two
 * openings and the enumeration cannot tell them apart — both are `PassmarkLineType::CheckMark`.
 * They are two shapes drawn as mirror images, which is why this flag is here and why the dock has
 * always offered them as two entries.
 */
struct PassmarkEntry
{
    PassmarkLineType type;
    const char *name;
    bool clockwise;
};

/** @brief A place label's type, and the words it is shown under. */
struct PlaceLabelEntry
{
    PlaceLabelType type;
    const char *name;
};

/** @brief The notch types, in the order they are offered. */
auto Passmarks() -> const QVector<PassmarkEntry> &;

/** @brief The place label types, in the order they are offered. */
auto PlaceLabels() -> const QVector<PlaceLabelEntry> &;

/**
 * @brief One shape — what a symbol is drawn with, and what a library entry stores.
 *
 * @ref type is a `PassmarkLineType` or a `PlaceLabelType`, whichever the family calls for; the two
 * lines carry none, being a single shape each.
 */
struct Shape
{
    Family family = Family::Grainline;
    int type = 0;
    bool clockwise = false; ///< the passmark "Check mark" only

    auto operator==(const Shape &other) const -> bool = default;
};

/**
 * @brief Every shape a symbol can be drawn with, in the order they are offered.
 *
 * **One shape per mark Ostara draws, and no variants of them.** What a symbol shows is that mark in
 * the form the canvas draws by default — a grain line is a grain line, whatever direction it was
 * given on the piece, because a symbol says what a mark *means* and not how this piece happens to
 * have set it.
 */
auto All() -> QVector<Shape>;

/** @brief The shape's name, in the user's language. Empty if the shape names no mark. */
auto Name(const Shape &shape) -> QString;

/**
 * @brief The shape as a token, for an index file.
 *
 * Stable from one release to the next: it names the mark, never a place in a table.
 */
auto Key(const Shape &shape) -> QString;

/** @brief The shape a token names, or nothing — an index may have been written by an older Ostara. */
auto FromKey(const QString &key) -> std::optional<Shape>;
} // namespace OSymbolShapes

#endif // OSYMBOLSHAPES_H
