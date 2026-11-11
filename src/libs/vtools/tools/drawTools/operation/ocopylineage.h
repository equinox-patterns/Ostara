/************************************************************************
 **
 **  @file   ocopylineage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  Where a copy an operation made comes from, and what has been copied from a curve.
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

#ifndef OCOPYLINEAGE_H
#define OCOPYLINEAGE_H

#include <QDomElement>
#include <QHash>
#include <QVector>
#include <QtGlobal>

class VAbstractPattern;
class VContainer;

/**
 * @brief The family of a copy: the object an operation copied, the object that one was copied from, and back up to
 * the first object no operation made — and, the other way, every copy made of an object.
 *
 * Read from the file and from nothing else. A copy names the operation that made it, never the object it copies
 * (see OOperationDestination), and the two lists of the operation are paired by rank: destination[i] is the image of
 * source[i]. The file holds every operation from the first line of a parse on, where the tools are built one by one
 * in file order — a question asked while a pattern is being read gets the same answer as one asked afterwards.
 *
 * No state: the rule alone, so the copy that is built and the curve whose box is ticked cannot read the family
 * differently.
 */
class OCopyLineage
{
public:
    /** @brief One copy an operation made: the operation, the copy, and the rank both hold in its two lists. */
    struct Image
    {
        quint32 operationId{0};
        quint32 copyId{0};
        quint32 sourceId{0};
    };

    /** @brief The object @a copyId is the image of, or 0 when no operation made it. */
    static auto SourceOf(VAbstractPattern *doc, const VContainer *data, quint32 copyId) -> quint32;

    /** @brief The source of @a copyId, its own source, and so on up to the first object no operation made — which
     *  is the last one of the list. Empty when @a copyId is not a copy. */
    static auto Ancestors(VAbstractPattern *doc, const VContainer *data, quint32 copyId) -> QVector<quint32>;

    /** @brief Every copy made of @a objectId, then every copy made of those, in that order. */
    static auto Descendants(VAbstractPattern *doc, quint32 objectId) -> QVector<Image>;

    /**
     * @brief The interior nodes the user cut @a curveId on, read from its own element: the boxes of a path, the
     * chest/back width of an armhole curve shown as its two pieces. Empty for a curve with no such box, and for an
     * object that has no element of its own — a copy, a piece, a half.
     */
    static auto CutNodesOf(VAbstractPattern *doc, quint32 curveId) -> QVector<int>;

    /** @brief The cut tools placed on @a curveId — a spline, a path or an arc cut at a length — in file order. */
    static auto CutsOn(VAbstractPattern *doc, quint32 curveId) -> QVector<QDomElement>;

    /** @brief Every cut tool of the pattern, by the curve it is placed on, in file order: one reading of the file for
     *  a whole operation rather than one per copy and per curve of its family. */
    static auto CutsByCurve(VAbstractPattern *doc) -> QHash<quint32, QVector<QDomElement>>;

private:
    /** @brief The operation element that made @a copyId, null when none did. */
    static auto OperationOf(VAbstractPattern *doc, const VContainer *data, quint32 copyId) -> QDomElement;
};

#endif // OCOPYLINEAGE_H
