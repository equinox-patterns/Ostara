/************************************************************************
 **
 **  @file   tst_gradedalign.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/11/2026
 **
 **  @brief  L'écart d'alignement gradué, axe par axe : ancres, point forcé, et l'un sans l'autre.
 **
 **          Chaque axe répond pour lui-même : un point horizontal peut exister sans point vertical,
 **          et réciproquement. Les deux axes forcés en même temps épinglent la pièce sur son point.
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

#include "tst_gradedalign.h"

#include "ogradedgeometry.h"

#include <QtTest>
#include <QRectF>

namespace
{
// One unit across two sizes: the reference extent, and the extent of the layer being moved.
const QRectF kUnitRect(0, 0, 100, 50);
const QRectF kLayerRect(10, 5, 100, 50);

// Centred anchors: unit centre (50, 25) minus layer centre (60, 30).
constexpr qreal kAnchorDeltaX = -10;
constexpr qreal kAnchorDeltaY = -5;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_GradedAlign::TST_GradedAlign(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_GradedAlign::AnchorsDriveBothAxes()
{
    OGraded::AlignRule rule; // no forced point: NaN on both axes
    const QPointF delta = OGraded::UnitLayerDelta(rule, kUnitRect, kLayerRect, qQNaN(), qQNaN());

    QCOMPARE(delta.x(), kAnchorDeltaX);
    QCOMPARE(delta.y(), kAnchorDeltaY);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_GradedAlign::ForcedPointDrivesHorizontalOnly()
{
    OGraded::AlignRule rule;
    rule.refPointX = 40; // the reference size's point
    const QPointF delta = OGraded::UnitLayerDelta(rule, kUnitRect, kLayerRect, 55, qQNaN());

    QCOMPARE(delta.x(), -15.0); // 40 - 55, the point wins over the anchor
    QCOMPARE(delta.y(), kAnchorDeltaY);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_GradedAlign::ForcedPointDrivesVerticalOnly()
{
    OGraded::AlignRule rule;
    rule.refPointY = 20;
    const QPointF delta = OGraded::UnitLayerDelta(rule, kUnitRect, kLayerRect, qQNaN(), 32);

    QCOMPARE(delta.x(), kAnchorDeltaX);
    QCOMPARE(delta.y(), -12.0); // 20 - 32
}

//---------------------------------------------------------------------------------------------------------------------
void TST_GradedAlign::BothAxesFollowTheirOwnPoint()
{
    OGraded::AlignRule rule;
    rule.refPointX = 40;
    rule.refPointY = 20;
    const QPointF delta = OGraded::UnitLayerDelta(rule, kUnitRect, kLayerRect, 55, 32);

    QCOMPARE(delta.x(), -15.0);
    QCOMPARE(delta.y(), -12.0);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_GradedAlign::MissingLayerPointFallsBackOnItsAnchor()
{
    // The point is known for the reference size but cannot be resolved for this layer — a node the
    // user deleted after choosing it. The axis must fall back on its anchor, not on a NaN shift.
    OGraded::AlignRule rule;
    rule.refPointX = 40;
    rule.refPointY = 20;
    const QPointF delta = OGraded::UnitLayerDelta(rule, kUnitRect, kLayerRect, qQNaN(), qQNaN());

    QCOMPARE(delta.x(), kAnchorDeltaX);
    QCOMPARE(delta.y(), kAnchorDeltaY);
}

QTEST_MAIN(TST_GradedAlign)
#include "tst_gradedalign.moc"
