/************************************************************************
 **
 **  @file   ovistoolpleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The preview of the pleats: the objects that travel where the folds put them, and the folds themselves.
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

#ifndef OVISTOOLPLEAT_H
#define OVISTOOLPLEAT_H

#include <QPointF>
#include <QVector>

#include "../../../tools/drawTools/operation/opleattransform.h"
#include "visoperation.h"

/**
 * @brief The preview of the pleats: the objects that travel where the folds put them, and the folds themselves.
 *
 * No state of its own beyond what the tool gives it: the departure lines and the boxes, replayed on every refresh.
 * The values are read from the formulas the departure lines carry, exactly as the tool will read them.
 *
 * And because a pleat is decided by what it DRAWS — the fold line, the caps, the sense of each cap — the figure is
 * drawn as well, from the same plan the tool will use. Telling the two senses apart is impossible otherwise: the
 * point of a cap moves along the fold line, and nothing else on screen says it moved.
 */
class VScaledEllipse;
class VScaledLine;

class OVisToolPleat : public VisOperation
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolPleat(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolPleat() override = default;

    void RefreshGeometry() override;

    /** @brief Where a point of the selection lands: a pleat takes the fabric that lies beyond each of its lines, so
     *  two points do not travel the same distance, and the plan is what answers for one of them. */
    auto PointImage(const QPointF &point) const -> QPointF override;

    /** @brief The departure lines of the pleats being built, the point of the line the user is picking, the boxes,
     *  and the line being edited, so the preview replays the very folds the tool will build — figure included.
     *
     *  @a pending is the first point of the line being picked, when there is one: it has no line yet, and it is
     *  drawn as the green dot of the gesture, as the tool's own preview does.
     *
     *  @a active is the departure line the dialog is editing, or -1 when there is none: its two ends are the only
     *  ones drawn bright, and everything the other lines draw is dimmed. The colour tells the two ends of a line
     *  apart — green for the first, red for the second — and never which line it is; the light is what says that. */
    void SetPleat(const QVector<OPleatDeparture> &departures, const OPleatEnd &pending, bool foldLine, int active);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolPleat)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolPleat) // NOLINT

    QVector<OPleatDeparture> m_departures{};
    OPleatEnd m_pending{};
    bool m_foldLine{true};
    int m_active{-1};

    /** The rule, run once per refresh: what each departure line is carried by, where its places are, and the mark
     *  of each cap. A pleat the rule refused leaves an empty plan, which moves nothing and draws no figure. */
    OPleatPlan m_plan{};

    /** The figure of the preview, kept between two refreshes so a mouse move does not rebuild items: the lines of
     *  the fold, and the point of each cap. */
    QVector<VScaledLine *> m_figureLines{};
    QVector<VScaledEllipse *> m_figurePoints{};

    /** The two points of each departure line, in the colour that names them: green for the first, red for the
     *  second. Drawn here because the gesture has no pattern object to colour yet — after it, the colours are the
     *  ones the dialog and the dock show beside each cap. */
    QVector<VScaledEllipse *> m_firstDots{};
    QVector<VScaledEllipse *> m_secondDots{};

    /** @brief How bright the figure of one departure line is drawn: the line the dialog is editing is the only one
     *  at full light, and every other one is dimmed (see kDimmedLight). */
    auto Light(int departure) const -> qreal;

    /** @brief The value one departure line shows, in pixels, or 0 while the formula it carries holds nothing. */
    static auto ValueInPixels(const QString &value, const VContainer *data) -> qreal;

    /** @brief The value of every departure line, in pixels, in the order the lines were given. */
    auto Values() const -> QVector<qreal>;

    /** @brief What the objects that travel are, for the rule that reads the side of the fold. */
    auto Source() const -> QVector<SourceItem>;

    void CreateTransformedObjects(const OPleatTransform &transform);

    /** @brief Draw the figure of the plan: every place of every departure line, the fold lines, and the cap of each
     *  end. The items it does not need are hidden, so a box that is turned off takes its drawing away here too. */
    void DrawFigure();

    void DrawDeparturePoints();

    template <class Item> auto AddTransformedCurve(quint32 id, int i, const OPleatTransform &transform) -> int;
};

#endif // OVISTOOLPLEAT_H
