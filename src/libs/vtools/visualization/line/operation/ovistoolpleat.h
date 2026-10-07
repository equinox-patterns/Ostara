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
 * @brief The preview of the pleats: the objects that travel where the folds put them, and the two ends of the line
 * being edited, where the pleats already taken have carried them.
 *
 * No state of its own beyond what the tool gives it: the departure lines and the boxes, replayed on every refresh.
 * The values are read from the formulas the departure lines carry, exactly as the tool will read them.
 *
 * The figure of the pleat — the fold line, the caps, the notches — is NOT drawn here. It is what the pattern holds
 * once the pleat is applied, and Apply draws it on the canvas without closing the dialog. Drawing it a second time
 * would mean two renderings of the same thing, and the one here would carry neither the colours nor the pen styles
 * the user chose: the figure is read where it is built, on the canvas.
 */
class VScaledEllipse;

class OVisToolPleat : public VisOperation
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolPleat(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolPleat() override = default;

    void RefreshGeometry() override;

    /** @brief Where an object of the selection lands: a pleat takes the run of the ordered selection that lies
     *  between the two points of its line, so two objects do not travel the same distance, and the plan answers
     *  for the object itself. */
    auto PointImage(quint32 id, const QPointF &point) const -> QPointF override;

    /** @brief The departure lines of the pleats being built, the point of the line the user is picking, and the
     *  line the dialog is editing.
     *
     *  @a pending is the first point of the line being picked, when there is one: it has no line yet, and it is
     *  drawn as the green dot of the gesture, as the tool's own preview does.
     *
     *  @a active is the departure line the dialog is editing, or -1 when there is none: its two ends are the only
     *  ones drawn bright, and the ends of the other lines are dimmed. The colour tells the two ends of a line
     *  apart — green for the first, red for the second — and never which line it is; the light is what says that. */
    void SetPleat(const QVector<OPleatDeparture> &departures, const OPleatEnd &pending, int active);

    /** @brief Tell the preview that the pleat is already in the pattern.
     *
     *  The pattern then draws everything the pleat produced — the objects its folds carried, the lines of its figure,
     *  their points — and the preview must not draw them a second time: the trace of the origin in orange and its
     *  image in magenta, laid over that drawing, would show the same objects twice. A pleat being built still has
     *  nothing in the pattern, and there the two traces are the only thing that shows what the folds will do: they
     *  stay. */
    void SetInPattern(bool inPattern);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolPleat)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolPleat) // NOLINT

    QVector<OPleatDeparture> m_departures{};
    OPleatEnd m_pending{};
    int m_active{-1};

    /** True when the pleat is in the pattern: the preview then keeps the ends of the gesture, and hides what the
     *  pattern draws itself (see SetInPattern). */
    bool m_inPattern{false};

    /** The rule, run once per refresh: what each departure line is carried by, where its places are, and the mark
     *  of each cap. A pleat the rule refused leaves an empty plan, which moves nothing and carries no end. */
    OPleatPlan m_plan{};

    /** The two points of each departure line, in the colour that names them: green for the first, red for the
     *  second. Drawn here because the gesture has no pattern object to colour yet — after it, the colours are the
     *  ones the dialog and the dock show beside each cap. */
    QVector<VScaledEllipse *> m_firstDots{};
    QVector<VScaledEllipse *> m_secondDots{};

    /** @brief How bright the ends of one departure line are drawn: the line the dialog is editing is the only one
     *  at full light, and every other one is dimmed (see kDimmedLight). */
    auto Light(int departure) const -> qreal;

    /** @brief The value one departure line shows, in pixels, or 0 while the formula it carries holds nothing. */
    static auto ValueInPixels(const QString &value, const VContainer *data) -> qreal;

    /** @brief The value of every departure line, in pixels, in the order the lines were given. */
    auto Values() const -> QVector<qreal>;

    /** @brief What the objects that travel are, for the rule that reads the side of the fold. */
    auto Source() const -> QVector<SourceItem>;

    void CreateTransformedObjects(const OPleatTransform &transform);

    void DrawDeparturePoints();

    /** @brief What the preview shows when there is no plan to draw: the lines attached to the pleat as they are, and
     *  the dots of the gesture. The single path both refreshes that give up on the plan go through, so what the pattern
     *  already draws is hidden once for the two of them. */
    void DrawWithoutAPlan();

    template <class Item> auto AddTransformedCurve(quint32 id, int i, const OPleatTransform &transform) -> int;
};

#endif // OVISTOOLPLEAT_H
