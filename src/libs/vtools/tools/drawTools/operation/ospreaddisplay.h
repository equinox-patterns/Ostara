/************************************************************************
 **
 **  @file   ospreaddisplay.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The visual representation of a slash-and-spread: the mark of every rotation, and the pen it is
 **          drawn with.
 **
 **          A rotation turns a piece around a pivot. The mark of that rotation is what makes the movement
 **          readable: its apex is the pivot, its base the two positions of the start point, the one the
 **          rotation begins at and the one it ends at. The apex and the end are objects the operation already
 **          draws, its own copies of the pivot and of the start; the beginning is a position nothing else
 **          materializes, so the mark creates a point for it, except where it is the start point as the user
 **          drew it: the first rotation begins at the source object itself, which is never copied.
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

#ifndef OSPREADDISPLAY_H
#define OSPREADDISPLAY_H

#include <QDomElement>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "ospreadrule.h"

class VAbstractPattern;
class VContainer;

/**
 * @brief The objects one mark holds.
 *
 * @a point is the third vertex, the position the start point comes from: NULL_ID when the mark creates no
 * point, which is the case of the first rotation, whose third vertex is the start point itself. @a line1 joins
 * the pivot to the copy of the start point, @a line2 joins the pivot to the third vertex. All three ids are
 * NULL_ID for a rotation the rule skips: its pair describes no region, so it draws nothing.
 */
struct OSpreadMark
{
    quint32 point{NULL_ID};
    quint32 line1{NULL_ID};
    quint32 line2{NULL_ID};
    QString name{};

    /** @brief Where the label of the third vertex sits, relative to it. Zero means the figure never said — a file
     *  written before a label could be moved on the display — and the place of the point the vertex comes from then
     *  stands, which is what the mark did every time until then. Kept by the mark because the vertex is no
     *  destination: the operation keeps no record of it to write the place in (see OToolSpread). */
    qreal mx{0.};
    qreal my{0.};
};

/**
 * @brief The visual representation of a slash-and-spread, as the operation and the file carry it.
 *
 * It is a display, not part of the transformation: the user turns it off when it gets in the way, and nothing
 * of it is then created. What it does create are pattern objects all the same, owned by the operation and
 * rebuilt by every parse, which is why their ids are kept in the file.
 */
struct OSpreadDisplay
{
    static const QString TagDisplay;
    static const QString AttrShow;
    static const QString AttrPoint;
    static const QString AttrLine1;
    static const QString AttrLine2;

    bool show{true};

    /** @brief The pen of every line of the display, and its colour. A solid line in black, which is what a file
     *  written before the display existed gets: the combos of the editor offer no "default", and the layer's own
     *  pen and colour are what they start on when the operation is created from the interface. */
    QString typeLine{TypeLineLine};
    QString lineColor{ColorBlack};

    /** @brief One mark per rotation, indexed like the rotations whatever the rule accepted. */
    QVector<OSpreadMark> marks{};

    /** @brief Complete every mark of the display: an id for the objects it still misses, and a name for its
     *  point. One point per rotation whose start point an earlier rotation already carried, two lines per
     *  rotation; what a mark already holds is kept as it is, so a parse never moves an object. */
    static void EnsureIds(OSpreadDisplay &display, const QVector<OSpreadRotation> &rotations, const OSpreadPlan &plan,
                          VContainer *data);

    /** @brief Create the point of every mark under the id it carries, or move the point that already exists to
     *  where its rotation puts it. Nothing is created for a mark without a point: its third vertex is a source
     *  object, and a source object is never copied. */
    static void Create(quint32 idTool, const OSpreadDisplay &display, const OSpreadPlan &plan, VContainer *data);

    /** @brief The ids of the points the marks hold, in rotation order, NULL_ID and the points the container does
     *  not know left out: the list a line factory reads as points that are their own image. */
    static auto PointImages(const OSpreadDisplay &display, const VContainer *data) -> QVector<quint32>;

    static void Save(VAbstractPattern *doc, QDomElement &tag, const OSpreadDisplay &display);
    static auto Extract(const QDomElement &domElement) -> OSpreadDisplay;
};

#endif // OSPREADDISPLAY_H
