/************************************************************************
 **
 **  @file   vabstractcubicbezierpath.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   16 3, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 10/10/2026.
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

#ifndef VABSTRACTCUBICBEZIERPATH_H
#define VABSTRACTCUBICBEZIERPATH_H

#include <QCoreApplication>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "vabstractbezier.h"
#include "vgeometrydef.h"

class VPointF;
class VSpline;
class VSplinePoint;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wsuggest-final-types")
QT_WARNING_DISABLE_GCC("-Wsuggest-final-methods")

class VAbstractCubicBezierPath : public VAbstractBezier
{
    Q_DECLARE_TR_FUNCTIONS(VAbstractCubicBezierPath) // NOLINT

public:
    explicit VAbstractCubicBezierPath(const GOType &type, const quint32 &idObject = NULL_ID,
                                      const Draw &mode = Draw::Calculation);
    VAbstractCubicBezierPath(const VAbstractCubicBezierPath &curve) = default;
    auto operator=(const VAbstractCubicBezierPath &curve) -> VAbstractCubicBezierPath &;
    ~VAbstractCubicBezierPath() override;

    VAbstractCubicBezierPath(VAbstractCubicBezierPath &&curve) noexcept = default;
    auto operator=(VAbstractCubicBezierPath &&curve) noexcept -> VAbstractCubicBezierPath & = default;

    virtual auto CountSubSpl() const -> vsizetype = 0;
    virtual auto CountPoints() const -> vsizetype = 0;
    virtual void Clear() = 0;
    virtual auto GetSpline(vsizetype index) const -> VSpline = 0;
    virtual auto GetSplinePath() const -> QVector<VSplinePoint> = 0;

    auto GetPoints() const -> QVector<QPointF> override;
    auto GetPath() const -> QPainterPath override;
    auto GetLength() const -> qreal override;

    auto DirectionArrows() const -> QVector<DirectionArrow> override;

    auto Segment(const QPointF &p) const -> int;

    auto CutSplinePath(qreal length, qint32 &p1, qint32 &p2, QPointF &spl1p2, QPointF &spl1p3, QPointF &spl2p2,
                       QPointF &spl2p3, const QString &pointName) const -> QPointF;

    auto NameForHistory(const QString &toolName) const -> QString override;
    auto GetMainNameForHistory() const -> QString override;

    virtual auto FirstPoint() const -> VPointF = 0;
    virtual auto LastPoint() const -> VPointF = 0;

    auto HeadlessName() const -> QString override;

    auto GetMidpoint() const -> VPointF override;

    auto GetTypeHead() const -> QString override;
    void SetNameSuffix(const QString &suffix) override;

protected:
    void CreateName() override;
    void CreateAlias() override;

    auto IsRelaxed() const -> bool;

    /// \brief DropPointsMemo drops the polyline memoized by GetPoints().
    ///
    /// GetPoints() flattens every sub-spline of the chain and then removes the self-intersecting loops of a
    /// relaxed path. That result is a pure function of the control points and of the approximation scale, and
    /// a single parse reads the very same curve again and again: start point, end point, segment, then the
    /// same curve seen from the neighbouring node. So it is memoized.
    ///
    /// The memo is only sound while every write to the geometry drops it, which is what this is for. Nothing
    /// detects a forgotten call: the curve would simply keep handing out its previous outline, silently.
    void DropPointsMemo() const { m_pointsMemoValid = false; }

private:
    mutable QVector<QPointF> m_pointsMemo;
    mutable qreal m_pointsMemoScale{0};
    mutable bool m_pointsMemoValid{false};
};

QT_WARNING_POP

#endif // VABSTRACTCUBICBEZIERPATH_H
