/************************************************************************
 **
 **  @file   vpiecegrainline_p.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   27 4, 2023
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2023 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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
#ifndef VPIECEGRAINLINE_P_H
#define VPIECEGRAINLINE_P_H

#include "../vmisc/defglobal.h"
#include "../vpatterndb/floatItemData/floatitemdef.h"

#include <QLineF>
#include <QSharedData>

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

class VPieceGrainlinePrivate : public QSharedData
{
public:
    VPieceGrainlinePrivate() = default;
    VPieceGrainlinePrivate(const QLineF &mainLine, GrainlineArrowDirection arrowType);
    VPieceGrainlinePrivate(const VPieceGrainlinePrivate &data) = default;
    ~VPieceGrainlinePrivate() = default;

    static auto MainLine(const QPointF &p1, qreal length, qreal angle) -> QLineF;


    QLineF m_mainLine{}; // NOLINT(misc-non-private-member-variables-in-classes)
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    GrainlineArrowDirection m_arrowType{GrainlineArrowDirection::oneWayUp};
    bool m_enabled{false}; // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_visible{true};  // NOLINT(misc-non-private-member-variables-in-classes)

private:
    Q_DISABLE_ASSIGN_MOVE(VPieceGrainlinePrivate) // NOLINT

};

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
inline VPieceGrainlinePrivate::VPieceGrainlinePrivate(const QLineF &mainLine, GrainlineArrowDirection arrowType)
  : m_mainLine(mainLine),
    m_arrowType(arrowType),
    m_enabled(true)
{
}
#endif // VPIECEGRAINLINE_P_H
