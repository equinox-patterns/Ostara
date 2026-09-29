/************************************************************************
 **
 **  @file   ovistooltransfert.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  What a transfer shows on the canvas: the portion it took, highlighted where it comes from, and
 **          the ghost of it translated by X → A, where it landed. Drawn when the tool is selected, so the
 **          two ends of the gesture stay readable long after it was made.
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

#ifndef OVISTOOLTRANSFERT_H
#define OVISTOOLTRANSFERT_H

#include <QGraphicsItem>
#include <QPointF>
#include <QtGlobal>

#include "visoperation.h"

class VScaledEllipse;

class OVisToolTransfert : public VisOperation
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolTransfert(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolTransfert() override = default;

    void RefreshGeometry() override;

    /** @brief X, the merge point of the source part: where the portion is anchored. */
    void SetOriginPointId(quint32 value);

    /** @brief A, the merge point of the host part: what X lands on, and therefore the vector of the whole
     *  translation. */
    void SetTargetPointId(quint32 value);

    /** @brief Translate the point by X → A, exactly like the copied objects of this tool. */
    auto PointImage(const QPointF &point) const -> QPointF override;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolTransfert)
    };

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OVisToolTransfert) // NOLINT

    quint32 m_originId{NULL_ID};
    quint32 m_targetId{NULL_ID};

    /** @brief The translation as it is displayed right now. Anchor() computes it once for the ghosts and for
     *  the attached lines, so the two can never disagree. */
    qreal m_displayedLength{0};
    qreal m_displayedAngle{0};
    QPointF m_displayedOrigin{};
    QPointF m_displayedTarget{};

    VScaledEllipse *m_pointOrigin{nullptr};
    VScaledEllipse *m_pointTarget{nullptr};

    /** @brief Read both merge points and the translation they define. False while one of them is unknown or
     *  gone — a preview then draws no image at all rather than an invented one. */
    auto Anchor() -> bool;

    template <class Item> auto AddTransferredCurve(quint32 id, int i) -> int;

    void CreateTransferredObjects(int &iPoint, int &iCurve);
};

#endif // OVISTOOLTRANSFERT_H
