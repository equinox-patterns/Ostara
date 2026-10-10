/************************************************************************
 **
 **  @file   oflouncespiraldisplay.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The figure of a flounce spiral, as the operation and the file
 **          carry it: the two edges, the two joining segments and the
 **          three points the tool adds.
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

#ifndef OFLOUNCESPIRALDISPLAY_H
#define OFLOUNCESPIRALDISPLAY_H

#include <QDomElement>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"

class VAbstractPattern;
class VContainer;

/**
 * @brief One of the three points the tool adds, with the name it carries in the dependency tree.
 *
 * @a role says which of the three it is, so that the three stay told apart when they are placed, saved and read
 * back. The point the user clicked is NOT one of them: it already exists, the tool only remembers it.
 */
struct OFlounceSpiralPoint
{
    enum class Role : int
    {
        OuterStart = 0, ///< the point the outer edge starts at, one band width from the clicked point
        InnerEnd = 1,   ///< the far end of the inner edge, the edge that is sewn on
        OuterEnd = 2    ///< the far end of the outer edge
    };

    Role role{Role::OuterStart};
    quint32 id{NULL_ID};
    QString name{};

    /** @brief Whether the name of the point is shown on the canvas.
     *
     *  The figure's own element has no place for it — the schema declares no such attribute — so the file
     *  carries the switch in the `<destination>` item of the point. This field is what the Points tab of the
     *  dialog hands over at creation; a figure read from a file takes it from those items. */
    bool labelVisible{false};
};

/**
 * @brief The figure of a flounce spiral.
 *
 * A band of constant width, both of its edges spirals, closed on itself: the two edges are joined at both ends
 * by a straight segment, and the outer edge comes back on the point the inner one started from.
 *
 * The two curves and the three points are objects of the container, carried by the operation: they are written
 * as ids and as formulas of this one element, never as elements of their own. That is what makes the whole
 * figure one undo step and one deletion, the way a rectangle already behaves — there is nothing to group,
 * because there is only ever one element.
 */
struct OFlounceSpiralDisplay
{
    static const QString TagDisplay;
    static const QString AttrInner;
    static const QString AttrOuter;
    static const QString AttrLine;
    static const QString AttrPoint;
    static const QString AttrRole;
    static const QString AttrWidth;
    static const QString AttrLength;
    static const QString AttrAngle;
    static const QString AttrTypeLine;
    static const QString AttrLineColor;
    static const QString AttrAlias;
    static const QString AttrPenStyle;

    /// Ids of the two edges, as curves of the container.
    quint32 inner{NULL_ID};
    quint32 outer{NULL_ID};

    /// Ids of the two joining segments. A segment is a line of the operation, not an object of its own.
    quint32 line1{NULL_ID};
    quint32 line2{NULL_ID};

    /// The three points the tool adds, in the order they are placed.
    QVector<OFlounceSpiralPoint> points{};

    /// The two values the user types, as formulas, and the direction of A from the clicked point.
    QString width{'0'};
    QString length{'0'};
    QString angle{QStringLiteral("180")};

    /// ONE pen and ONE colour for the whole figure — both edges and both joining segments. The flounce is a
    /// whole: nothing on the canvas tells its parts apart, and its style is therefore set once, like the style
    /// of a rectangle. The file keeps three places for that pair — the attributes of the element, the penStyle
    /// and lineColor of every item, and the <lines> element the base writes — and they all carry this one.
    QString typeLine{TypeLineLine};
    QString lineColor{ColorBlack};

    /// For the formula table: which object each edge is, so that a formula can name it. The figure offers ONE
    /// name, the one of the curve A_2 → A_3, the outer edge. The inner alias is still read and written: a
    /// pattern saved before the figure was given a single name carries it, and dropping it would take away a
    /// variable the formulas of that pattern may use. Nothing offers to edit it any more.
    QString innerAlias{};
    QString outerAlias{};

    /// Id of the point of each entry, or NULL_ID when the role was not given.
    static auto PointId(const OFlounceSpiralDisplay &display, OFlounceSpiralPoint::Role role) -> quint32;

    /// Give an id to every entry that has none, so that the figure is complete before it is written.
    static void EnsureIds(OFlounceSpiralDisplay &display, VContainer *data);

    /// Every id of the figure, as the display holds them.
    static auto OwnedIds(const OFlounceSpiralDisplay &display) -> QSet<quint32>;

    /// Every id of the figure, as the file names them.
    static auto ObjectIds(const QDomElement &element) -> QSet<quint32>;

    /// Ids of the points of the figure, as the file names them.
    static auto PointIds(const QDomElement &element) -> QVector<quint32>;

    /// Ids of the curves of the figure, as the file names them.
    static auto CurveIds(const QDomElement &element) -> QVector<quint32>;

    /// Write the figure under @p tag, replacing any figure already there.
    static void Save(VAbstractPattern *doc, QDomElement &tag, const OFlounceSpiralDisplay &display);

    /// Read the figure out of @p domElement. A missing figure gives an empty one.
    static auto Extract(const QDomElement &domElement) -> OFlounceSpiralDisplay;

    /// Whether two figures carry the same values — used to decide whether a file has to be rewritten.
    static auto Same(const OFlounceSpiralDisplay &first, const OFlounceSpiralDisplay &second) -> bool;
};

#endif // OFLOUNCESPIRALDISPLAY_H
