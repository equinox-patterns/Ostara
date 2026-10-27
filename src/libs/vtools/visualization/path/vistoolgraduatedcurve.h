/************************************************************************
 **
 **  @file   vistoolgraduatedcurve.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 12, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 09/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/
#ifndef VISTOOLGRADUATEDCURVE_H
#define VISTOOLGRADUATEDCURVE_H

#include "vispath.h"

struct VRawGraduatedCurveOffset;
class VAbstractCurve;
class VGraphicsSimpleTextItem;
class VScaledEllipse;
class VScaledLine;

class VisToolGraduatedCurve final : public VisPath
{
    Q_OBJECT // NOLINT

public:
    explicit VisToolGraduatedCurve(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~VisToolGraduatedCurve() override = default;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    void SetCurveId(quint32 newCurveId);
    void SetOffsets(const QVector<VRawGraduatedCurveOffset> &offsets);

    /**
     * @brief SetActiveOffset remembers which width is being edited — the rank of the row in the dialog's table — and
     * marks where that one width acts. Every other width keeps the colour of a mark of the preview.
     */
    void SetActiveOffset(int index);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolGraduatedCurve)
    };

private:
    Q_DISABLE_COPY_MOVE(VisToolGraduatedCurve) // NOLINT

    /**
     * @brief What shows where one width acts: the point of the base curve it is measured from, the point it lands on,
     * the normal between the two, the rank of the width and its value.
     */
    struct StationItems
    {
        int index{-1}; ///< rank of the width in the list of offsets, not the rank of this mark along the curve
        VScaledEllipse *anchor{nullptr};
        VScaledEllipse *target{nullptr};
        VScaledLine *normal{nullptr};
        VGraphicsSimpleTextItem *rank{nullptr};
        VGraphicsSimpleTextItem *value{nullptr};
    };

    VCurvePathItem *m_originalCurve{nullptr};

    QVector<qreal> m_offsets{};
    QVector<StationItems> m_stations{};
    quint32 m_curveId{NULL_ID};
    int m_activeOffset{-1};

    void RefreshStations(const VAbstractCurve &curve, const QVector<qreal> &widths, int activeOffset);
    void ApplyStationColors();
    auto MakeStationLabel() -> VGraphicsSimpleTextItem *;
    void PlaceStationLabel(VGraphicsSimpleTextItem *label, const QPointF &from, const QPointF &away);
};

//---------------------------------------------------------------------------------------------------------------------
inline void VisToolGraduatedCurve::SetCurveId(quint32 newCurveId)
{
    m_curveId = newCurveId;
}

#endif // VISTOOLGRADUATEDCURVE_H
