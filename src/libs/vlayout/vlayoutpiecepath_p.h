/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   8 2, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 16/10/2026.
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

#ifndef VLAYOUTPIECEPATH_P_H
#define VLAYOUTPIECEPATH_P_H

#include <QPointF>
#include <QSharedData>
#include <QVector>

#include "vlayoutpoint.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

class VLayoutPiecePathData : public QSharedData
{
public:
    VLayoutPiecePathData() = default;
    explicit VLayoutPiecePathData(const QVector<VLayoutPoint> &points);
    VLayoutPiecePathData(const VLayoutPiecePathData &path) = default;
    ~VLayoutPiecePathData() = default;


    /** @brief m_points list of path points. */
    QVector<VLayoutPoint> m_points{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_penStyle path pen style. */
    Qt::PenStyle m_penStyle{Qt::SolidLine}; // NOLINT(misc-non-private-member-variables-in-classes)

    bool m_cut{false};         // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_notMirrored{false}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_firstToCuttingContour / m_lastToCuttingContour tell that the first (resp. last) point of
     * the path is continued to the cutting contour: the path then carries one extra point at that end,
     * where it crosses the seam allowance. Kept so that the Clo3D/Style3D DXF export can write the path
     * alone, without that continuation. */
    bool m_firstToCuttingContour{false}; // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_lastToCuttingContour{false};  // NOLINT(misc-non-private-member-variables-in-classes)

private:
    Q_DISABLE_ASSIGN_MOVE(VLayoutPiecePathData) // NOLINT

};

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
inline VLayoutPiecePathData::VLayoutPiecePathData(const QVector<VLayoutPoint> &points)
  : m_points(points)
{
}
#endif // VLAYOUTPIECEPATH_P_H
