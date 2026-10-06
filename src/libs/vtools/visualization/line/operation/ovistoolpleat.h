/************************************************************************
 **
 **  @file   ovistoolpleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  What a pleat previews: the objects that travel, and where the fold carries them.
 **
 **          The user picks the prise and the objects while the dialog is not up yet, so the preview is what tells
 **          them the pick is complete: every object that travels is drawn again where the pleat will put it, at the
 **          very place the tool will build it — the same rule, the same transform.
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

#include <QGraphicsItem>
#include <QPointF>
#include <QVector>
#include <QtGlobal>

#include "../../../tools/drawTools/operation/opleatrule.h"
#include "../../../tools/drawTools/operation/opleattransform.h"
#include "visoperation.h"

/**
 * @brief The preview of a pleat: the objects that travel where the fold puts them, and the fold itself.
 *
 * No state of its own beyond what the tool gives it: the prise, the value, the kind and the boxes, replayed on every
 * refresh.
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

    /** @brief Where a point of the selection lands: the fold carries everything by the same vector, so the copy of a
     *  line the user selected is the line between the images of its two ends. */
    auto PointImage(const QPointF &point) const -> QPointF override;

    /** @brief The prise, the value, the kind and the boxes of the pleat being built, so the preview replays the very
     *  fold the tool will build — figure included. */
    void SetPleat(const OPleatEnd &first, const OPleatEnd &second, const QString &value, OPleatKind kind,
                  bool foldLine);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolPleat)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolPleat) // NOLINT

    OPleatEnd m_first{};
    OPleatEnd m_second{};
    QString m_value{};
    OPleatKind m_kind{OPleatKind::Simple};
    bool m_foldLine{true};

    /** The vector the fold carries every object by, kept so a line of the selection can be copied on screen before
     *  the operation exists. Null while the pleat cannot be built: a point is then its own image. */
    QPointF m_move{};

    /** The figure of the preview, kept between two refreshes so a mouse move does not rebuild items: the lines of
     *  the fold, and the point of each cap. */
    QVector<VScaledLine *> m_figureLines{};
    QVector<VScaledEllipse *> m_figurePoints{};

    /** The two points of the departure line, in the colour that names them: green for the first, red for the second.
     *  Drawn here because the gesture has no pattern object to colour yet — after it, the colours are the ones the
     *  dialog and the dock show beside each cap. */
    VScaledEllipse *m_firstDot{nullptr};
    VScaledEllipse *m_secondDot{nullptr};

    /** @brief The value the pleat shows, in pixels, or 0 while the user has not typed one that holds. */
    auto ValueInPixels() const -> qreal;

    void CreateTransformedObjects(const OPleatTransform &transform);

    /** @brief Draw the figure of a plan: the prise and its copies, the fold lines, and the cap of each end. The items
     *  it does not need are hidden, so a box that is turned off takes its drawing away here too. */
    void DrawFigure(const OPleatPlan &plan);

    void DrawDeparturePoints();

    template <class Item> auto AddTransformedCurve(quint32 id, int i, const OPleatTransform &transform) -> int;
};

#endif // OVISTOOLPLEAT_H
