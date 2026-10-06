/************************************************************************
 **
 **  @file   opleatdisplay.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The objects a pleat draws: the prise and its copies, the fold line, and the cap of each end.
 **
 **          A pleat produces no geometry of its own — the objects that travel are copies like any other — but it
 **          does draw a figure: the prise it was taken on, the same line carried to where the fabric lands, the
 **          fold line in between, and, wherever the piece's edge breaks across the fold, a cap: the V whose point
 **          sits on the fold line and whose two sides follow the edge.
 **
 **          What it draws it owns. Each end has its own checkbox, the fold line can be turned off, and the whole
 **          figure can be hidden: none of that is possible on an object the pattern also holds for itself, which is
 **          why every one of them is a point or a line of the operation, created and placed by every parse. The
 **          file carries their ids so they keep them, exactly as the rotation display of a slash-and-spread does.
 **
 **          The prise itself is not created: its two points are the ones the user drew. Their copies are created
 **          only when they do not already travel with the operation — a copy the operation makes of a source point
 **          is the same object, and making a second one on top of it would leave two points where the pattern only
 **          needs one (see Endpoint).
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

#ifndef OPLEATDISPLAY_H
#define OPLEATDISPLAY_H

#include <QDomElement>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "opleatrule.h"

class VAbstractPattern;
class VContainer;

/**
 * @brief The point that materializes the cap of one end of one pleat, and the two sides that make it a cap.
 *
 * @a point is the apex of the V — or the middle of the opening when the edge is parallel to the move and there is
 * no V to draw, in which case @a side1 and @a side2 stay NULL_ID and the point stands alone. @a side1 joins the
 * apex to the prise point, @a side2 to the same point carried by the move: those are the two tracés of a dart cap.
 */
struct OPleatMarkDisplay
{
    int pleat{0};
    int end{0};
    quint32 point{NULL_ID};
    quint32 side1{NULL_ID};
    quint32 side2{NULL_ID};
    QString name{};
};

/**
 * @brief The copy of one prise point at one place the prise went to: the point the display created, and the name it
 *  carries in the dependency tree. @a point is NULL_ID when the operation already copies that prise point by itself,
 *  because it travels with the other objects — there is then nothing to create, and nothing to name.
 */
struct OPleatCopy
{
    quint32 point{NULL_ID};
    QString name{};
};

/**
 * @brief The figure of a pleat, as the operation and the file carry it.
 *
 * The lines are read by position, the marks by pleat and end: a position is one place the prise went to, and every
 * prise line is drawn there. A double pleat has three of them — the prise, the shared line where the two folds
 * meet, and the copy — and two fold lines.
 */
struct OPleatDisplay
{
    static const QString TagDisplay;
    static const QString AttrShow;
    static const QString AttrTypeLine;
    static const QString AttrLineColor;
    static const QString AttrFoldTypeLine;
    static const QString AttrFoldColor;
    static const QString AttrLine;
    static const QString AttrFoldLine;
    static const QString AttrCopy;
    static const QString AttrPoint;
    static const QString AttrSide1;
    static const QString AttrSide2;

    bool show{true};

    /** @brief The pen of the lines of the fold — the prise, its copies and the sides of the caps — and their
     *  colour. A solid orange line, which is what a file written before the display existed gets. */
    QString lineTypeLine{TypeLineLine};
    QString lineColor{ColorOrange};

    /** @brief The pen and the colour of the fold line itself: what the pleat is pressed along. A dotted light
     *  green, so that it never reads as a line of the pattern. */
    QString foldTypeLine{TypeLineDotLine};
    QString foldColor{ColorLightGreen};

    /** @brief The line drawn at each position the prise went to: @c pleats + 1 of them, from the prise to the
     *  copy. The middle one of a double pleat is the line the two folds share. */
    QVector<quint32> lines{};

    /** @brief The fold line of each pleat: @c pleats of them, from the mark of one end to the mark of the other. */
    QVector<quint32> foldLines{};

    /** @brief The copies of the prise points the display has to create, two per position beyond the first, in
     *  position order and, in each position, first end then second. NULL_ID where the operation already copies
     *  that prise point by itself (see Endpoint). */
    QVector<OPleatCopy> copies{};

    /** @brief One entry per cap the pleat draws, in the order of the plan. Empty for a disabled cap. */
    QVector<OPleatMarkDisplay> marks{};

    /** @brief How many pleats a kind is made of: one, or two side by side. */
    static auto PleatCount(OPleatKind kind) -> int;

    /** @brief The number of places the prise goes to: the prise itself, and one more per pleat. */
    static auto PositionCount(OPleatKind kind) -> int;

    /** @brief The id of one endpoint of one position.
     *
     *  Position 0 is the prise point itself. Any later position is either a point of the display or, for the last
     *  one, the copy the operation already made of that prise point when it travels with the other objects: the
     *  same question Create asks when it decides whether to place a point, so the two cannot disagree. */
    static auto Endpoint(const OPleatDisplay &display, int position, int end, const OPleatEnd &first,
                         const OPleatEnd &second, const QVector<SourceItem> &source,
                         const QVector<DestinationItem> &destination) -> quint32;

    /** @brief Complete the figure: an id for every object it still misses, and a name for every point. Nothing is
     *  allocated when the rule refused the pleat — there is no figure to draw then, and no id must be left in the
     *  file for an object that will not exist. */
    static void EnsureIds(OPleatDisplay &display, const OPleatEnd &first, const OPleatEnd &second, OPleatKind kind,
                          bool foldLine, const OPleatPlan &plan, const QVector<SourceItem> &source,
                          const QVector<DestinationItem> &destination, VContainer *data);

    /** @brief Place the points of the figure: the copies of the prise points, and the apex or the middle of every
     *  cap. The lines are built by the operation, which is the only one that can put an item in the scene. */
    static void Create(quint32 idTool, OPleatDisplay &display, const OPleatPlan &plan, OPleatKind kind,
                       const OPleatEnd &first, const OPleatEnd &second, VContainer *data);

    /** @brief The ids of the points the display created, the ones the operation has to own: every copy it placed
     *  and every mark. A prise point is not one of them, and neither is a copy the operation made by itself. */
    static auto PointImages(const OPleatDisplay &display, const VContainer *data) -> QVector<quint32>;

    static void Save(VAbstractPattern *doc, QDomElement &tag, const OPleatDisplay &display);
    static auto Extract(const QDomElement &domElement) -> OPleatDisplay;
};

#endif // OPLEATDISPLAY_H
