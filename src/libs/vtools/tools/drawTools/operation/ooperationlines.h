/************************************************************************
 **
 **  @file   ooperationlines.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  Lines attached to an operation tool. A line has no geometric object of its own
 **          (see GOType): the Line tool stores two point ids, and the point tools draw an
 **          implicit line towards their base point. Both families are described here, so an
 **          operation can select them, preview them and create their transformed copies.
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

#ifndef OOPERATIONLINES_H
#define OOPERATIONLINES_H

#include <QDomElement>
#include <QHash>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"

class VAbstractPattern;
class VContainer;
class VMainGraphicsScene;
class VToolLine;

/** @brief The parse that is running, as VAbstractPattern::Parse receives it. Declared here to keep the
 *  parse type readable in the signatures below without pulling the whole pattern document in. */
enum class Document : qint8;

/**
 * @brief The OOperationLine struct describes one line attached to an operation.
 *
 * A line exists only through its two endpoints. @a source identifies what carries the line in
 * the pattern: the id of the tool that draws it — a Line tool, a rectangle carrying four sides, a dart cap
 * carrying two tracés — and the pair of its endpoints, which is what tells two lines of the same tool apart.
 * @a result is the id of the line created by the operation, NULL_ID until it is allocated.
 */
struct OOperationLine
{
    quint32 source{NULL_ID};
    quint32 point1{NULL_ID};
    quint32 point2{NULL_ID};
    quint32 result{NULL_ID};
    QString typeLine{};
    QString lineColor{};
};

/**
 * @brief The OOperationLines class gathers the operations done on the lines of an operation tool.
 *
 * No state: every method is a transformation or a serialization helper, so the operation tool stays
 * the single owner of its data.
 */
class OOperationLines
{
public:
    static const QString TagLines;
    static const QString AttrIdSource;
    static const QString AttrIdPoint1;
    static const QString AttrIdPoint2;
    static const QString AttrIdResult;

    /** @brief Describes a line the user clicked: the Line tool that carries it, its two endpoints, its pen and
     *  its colour. Empty — NULL_ID endpoints — when the id names no line tool. */
    static auto LineDescriptor(VToolLine *line) -> OOperationLine;

    /** @brief The lines an operation carries, from the selection the dialog hands over: one descriptor per line
     *  the user picked, whatever draws it. A descriptor whose endpoints do not describe a line is dropped here,
     *  the single place every caller goes through, and the result ids are cleared — the operation allocates them,
     *  the selection never carries them. */
    static auto Collect(const QVector<OOperationLine> &selected) -> QVector<OOperationLine>;

    /** @brief Give a new id to every line result that has none yet (creation from the GUI). */
    static void AllocateResultIds(VContainer *data, QVector<OOperationLine> &lines);

    /** @brief Reconcile the lines a dialog is about to persist with the ones the operation already holds:
     *  a line that is still selected keeps the id of its transformed copy, a new one gets a fresh id, a
     *  line that was dropped simply disappears from the list. */
    static auto SyncLines(VContainer *data, const QVector<OOperationLine> &oldLines,
                          const QVector<OOperationLine> &selected) -> QVector<OOperationLine>;

    /** @brief Image of a line endpoint: the destination object of that source point, or the point
     *  itself when it is a fixed point of the transformation, or NULL_ID when it has no image.
     *
     *  @a referencedPoints holds the source points an operation references instead of copying — the merge
     *  point of a transfer, whose image is the merge point of the host part. It is read first: such a point
     *  is in the source list but has no destination object of its own, so the positional lookup below would
     *  answer NULL_ID for it. Empty for every operation that copies all of its sources. */
    static auto PointImage(quint32 pointId, const QVector<SourceItem> &source,
                           const QVector<DestinationItem> &destination, const QVector<quint32> &fixedPoints,
                           const QHash<quint32, quint32> &referencedPoints = {}) -> quint32;

    /** @brief Create or refresh the transformed line of every descriptor whose two endpoints have an image.
     *
     *  A line that does not exist yet is built by @a parse == Document::FullParse, and by that parse only:
     *  creating a scene item anywhere else would give the pattern a second set of tools. A line that does
     *  exist is left untouched by the full parse (it was built by it) and refreshed by every other one — a
     *  transformed line has no element of its own, so this call is the only thing that re-reads it, exactly
     *  as the parser re-reads a normal line from its <line> element. Without it the Line_ and AngleLine_
     *  variables of the transformed line, which every parse clears, would stay missing until the file is
     *  reopened. */
    static void CreateLines(VAbstractPattern *doc, VContainer *data, VMainGraphicsScene *scene,
                            const QVector<OOperationLine> &lines, const QVector<SourceItem> &source,
                            const QVector<DestinationItem> &destination, const QVector<quint32> &fixedPoints,
                            const Document &parse, const QHash<quint32, quint32> &referencedPoints = {});

    static void SaveLines(VAbstractPattern *doc, QDomElement &tag, const QVector<OOperationLine> &lines);
    static auto ExtractLines(const QDomElement &domElement) -> QVector<OOperationLine>;

private:
    Q_DISABLE_COPY_MOVE(OOperationLines) // NOLINT
    OOperationLines() = default;
};

#endif // OOPERATIONLINES_H
