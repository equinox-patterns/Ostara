/************************************************************************
 **
 **  @file   toolsdef.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 11, 2020
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2020 Valentina project
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
#ifndef TOOLSDEF_H
#define TOOLSDEF_H

#include <QMetaType>
#include <QString>
#include <QUuid>

#include "../ifc/ifcdef.h"

class VGObject;
class VContainer;
class VPieceNode;

struct VRawGraduatedCurveOffset
{
    QString name{};
    QString formula{};
    QString description{};
};

struct SourceItem
{
    quint32 id{NULL_ID};
    QString name{};
    QString penStyle{TypeLineDefault};
    QString color{ColorDefault};
    QUuid recordId{QUuid::createUuid()};
};

Q_DECLARE_METATYPE(SourceItem)
Q_DECLARE_TYPEINFO(SourceItem, Q_MOVABLE_TYPE); // NOLINT

struct DestinationItem
{
    quint32 id{NULL_ID};
    qreal mx{1};
    qreal my{1};
    bool showLabel{false};
    QUuid recordId{};
};

Q_DECLARE_METATYPE(DestinationItem)
Q_DECLARE_TYPEINFO(DestinationItem, Q_MOVABLE_TYPE); // NOLINT

/**
 * @brief A straight segment a tool draws, described as the pair of points it registered in the container.
 *
 * @a point1 and @a point2 are given in the order the tool registered them through VContainer::AddLine — the
 * order the Line_ and AngleLine_ variables of the pattern are keyed on. The pen is carried per segment even
 * though every tool met so far uses one pen for all of its segments: a tool that draws two of them
 * differently must not have to change this interface.
 */
struct VDrawnSegment
{
    quint32 point1{NULL_ID};
    quint32 point2{NULL_ID};
    QString typeLine{TypeLineDefault};
    QString lineColor{ColorDefault};
};

Q_DECLARE_TYPEINFO(VDrawnSegment, Q_MOVABLE_TYPE); // NOLINT

enum class SegmentLabel : quint8
{
    Segment1,
    Segment2,
    Segment3,
    Segment4
};

// Default label position
constexpr qreal labelMX = ToPixel(4, Unit::Mm);
constexpr qreal labelMY = ToPixel(6.5, Unit::Mm);

auto SourceToObjects(const QVector<SourceItem> &source) -> QVector<quint32>;
auto NodesToObjects(const QVector<VPieceNode> &nodes) -> QVector<quint32>;

void FillDefSourceNames(QVector<SourceItem> &source, const VContainer *data, const QString &suffix);

auto GetDefSourceName(quint32 id, const VContainer *data, const QString &suffix, const QSet<QString> &occupiedNames)
    -> QString;

auto GetSourceItemName(const QString &name, quint32 id, const VContainer *data) -> QString;


auto ConfirmDeletion() -> int;

#endif // TOOLSDEF_H
