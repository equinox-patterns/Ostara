/************************************************************************
 **
 **  @file   opleatdisplay.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The objects a pleat draws: the departure lines and their copies, the fold lines, and the cap of each
 **          end of each pleat.
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
#include <QSet>
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
 * apex to the departure line, @a side2 to that line carried by the move: those are the two tracés of a dart cap.
 *
 * @a pleat is the index of the pleat among all the pleats of the operation, and @a end 0 for the first point of
 * its departure line, 1 for the other: the plan numbers its marks the same way.
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
 * @brief The copy of one end of a departure line at one place that line went to: the point the display created,
 *  and the name it carries in the dependency tree.
 *
 * @a departure is the index of the departure line among the ones the user gave, @a line the place on that line —
 * 0 for the line itself once the pleats before it have carried it, 1 for its first copy, and so on — and @a end
 * which of the two points of the line it is. @a point is NULL_ID when the operation already carries that place by
 * itself: the departure line as the user drew it, and the copy the operation made of a point that travels.
 */
struct OPleatCopy
{
    int departure{0};
    int line{0};
    int end{0};
    quint32 point{NULL_ID};
    QString name{};
};

/**
 * @brief The notch of one end of one pleat: the point that marks it, and the two halves the edge is cut into.
 *
 * @a pleat is the index of the pleat among all the pleats of the operation, and @a end 0 for the first point of
 * its departure line, 1 for the other: the plan numbers its notches the same way.
 *
 * @a point is the mark itself, carried by the operation. @a curve1 and @a curve2 are the two halves of the edge
 * the notch cuts — the first the piece from that edge's start to the notch, the second the piece from the notch to
 * its end — and both are NULL_ID when a line carries the edge, the notch being then a point and nothing else.
 */
struct OPleatNotchDisplay
{
    int pleat{0};
    int end{0};
    quint32 point{NULL_ID};
    quint32 curve1{NULL_ID};
    quint32 curve2{NULL_ID};
    QString name{};
};

/**
 * @brief The figure of the pleats, as the operation and the file carry it.
 *
 * The lines are read by place — one per departure line, and one per pleat that line is taken on — and the marks
 * by pleat and end. A simple pleat of a single line has two places and one fold line; a double one has three
 * places and two fold lines, the middle place being the line the two folds share.
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
    static const QString AttrDeparture;
    static const QString AttrPlace;
    static const QString AttrEnd;
    static const QString AttrPoint;
    static const QString AttrSide1;
    static const QString AttrSide2;
    static const QString AttrNotch;
    static const QString AttrCurve1;
    static const QString AttrCurve2;

    bool show{true};

    /** @brief The pen of the lines of the fold — the departure lines, their copies and the sides of the caps —
     *  and their colour. A solid orange line, which is what a file written before the display existed gets. */
    QString lineTypeLine{TypeLineLine};
    QString lineColor{ColorOrange};

    /** @brief The pen and the colour of the fold line itself: what each pleat is pressed along. A dotted light
     *  green, so that it never reads as a line of the pattern. */
    QString foldTypeLine{TypeLineDotLine};
    QString foldColor{ColorLightGreen};

    /** @brief The line drawn at each place of each departure line, in the order of @a LineIndex. */
    QVector<quint32> lines{};

    /** @brief The fold line of each pleat, in the order of the pleats: one per pleat of the operation. */
    QVector<quint32> foldLines{};

    /** @brief The copies the display has to create, in the order of @a CopyIndex. NULL_ID where the operation
     *  already carries that place by itself (see Endpoint). */
    QVector<OPleatCopy> copies{};

    /** @brief One entry per cap the pleats draw, in the order of the plan. Empty for a disabled cap. */
    QVector<OPleatMarkDisplay> marks{};

    /** @brief One entry per notch the pleats ask for, in the order of the plan. Empty for an end that asks for
     *  none. */
    QVector<OPleatNotchDisplay> notches{};

    /** @brief How many pleats one departure line is taken on: one, or two side by side. */
    static auto PleatCount(const OPleatDeparture &departure) -> int;

    /** @brief How many places one departure line is drawn at: the line itself, carried, and one more per pleat. */
    static auto LineCount(const OPleatDeparture &departure) -> int;

    /** @brief How many lines the whole figure holds: the places of every departure line, one after the other. */
    static auto LineCount(const QVector<OPleatDeparture> &departures) -> int;

    /** @brief How many copies the whole figure holds: two per place, one per end of the departure line. */
    static auto CopyCount(const QVector<OPleatDeparture> &departures) -> int;

    /** @brief The index, in @a lines, of one place of one departure line. */
    static auto LineIndex(const QVector<OPleatDeparture> &departures, int departure, int line) -> int;

    /** @brief The index, in @a copies, of one end of one place of one departure line. */
    static auto CopyIndex(const QVector<OPleatDeparture> &departures, int departure, int line, int end) -> int;

    /** @brief The id of one endpoint of one place of one departure line.
     *
     *  The line as the user drew it is read at the points themselves. Any later place is either a point of the
     *  display or the copy the operation already made of that point when it travels: the same question Create
     *  asks when it decides whether to place a point, so the two cannot disagree. */
    static auto Endpoint(const OPleatDisplay &display, const OPleatPlan &plan,
                         const QVector<OPleatDeparture> &departures, int departure, int line, int end,
                         const QVector<SourceItem> &source, const QVector<DestinationItem> &destination) -> quint32;

    /** @brief Complete the figure: an id for every object it still misses, and a name for every point. Nothing is
     *  allocated when the rule refused the pleats — there is no figure to draw then, and no id must be left in
     *  the file for an object that will not exist.
     *
     *  An entry the file holds whose place no longer exists — a departure line the user took away — is dropped:
     *  the places of the others are then given new ids, since nothing tells which old id stood for which new
     *  place. */
    static void EnsureIds(OPleatDisplay &display, const QVector<OPleatDeparture> &departures, bool foldLine,
                          const OPleatPlan &plan, const QVector<SourceItem> &source,
                          const QVector<DestinationItem> &destination, VContainer *data);

    /** @brief Place the points of the figure: the copies of the departure lines, and the apex or the middle of
     *  every cap. The lines are built by the operation, which is the only one that can put an item in the scene. */
    static void Create(quint32 idTool, OPleatDisplay &display, const OPleatPlan &plan,
                       const QVector<OPleatDeparture> &departures, const QVector<SourceItem> &source,
                       const QVector<DestinationItem> &destination, VContainer *data);

    /** @brief The ids of the points the display created, the ones the operation has to own: every copy it placed
     *  and every mark. A point of a departure line is not one of them, and neither is a copy the operation made
     *  by itself. */
    static auto PointImages(const OPleatDisplay &display, const VContainer *data) -> QVector<quint32>;

    /** @brief The ids of the curves the display cut for its notches: the two halves of every edge a notch was
     *  placed on. The operation owns them exactly as it owns the points of its figure, and a curve answers to its
     *  own id in a layer. */
    static auto CurveImages(const OPleatDisplay &display, const VContainer *data) -> QVector<quint32>;

    static void Save(VAbstractPattern *doc, QDomElement &tag, const OPleatDisplay &display);
    static auto Extract(const QDomElement &domElement) -> OPleatDisplay;

    /** @brief The ids of the lines the figure an element holds is made of: the places of the departure lines, the
     *  fold lines, and the two sides of every cap.
     *
     *  A line is a tool of the pattern, and only a full parse builds one — so this is what tells a change of the
     *  figure a lite parse can carry from one it cannot (see VAbstractOperation::FigureFollowsLiteParse). */
    static auto LineIds(const QDomElement &element) -> QSet<quint32>;
};

#endif // OPLEATDISPLAY_H
