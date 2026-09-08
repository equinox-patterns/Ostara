/************************************************************************
 **
 **  @file   otoolcurvepoint.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/08/2026
 **
 **  @brief  Point at the start (FirstPoint) or end (LastPoint) of an
 **          offset curve (parallel or graduated), without cutting the
 **          curve into sub-curves.
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

#ifndef OTOOLCURVEPOINT_H
#define OTOOLCURVEPOINT_H

#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "vtoolsinglepoint.h"

template <class T> class QSharedPointer;

/// Which end of the offset curve the point materializes.
enum class CurvePointPosition : qint8
{
    Start,
    End
};

struct OToolCurvePointInitData : VToolSinglePointInitData
{
    quint32 splineId{NULL_ID};
    CurvePointPosition position{CurvePointPosition::Start};
};

/**
 * @brief The OToolCurvePoint class point at the start (FirstPoint) or end
 *        (LastPoint) of an offset curve.
 *
 * Unlike VToolCutSplinePath, this tool does not cut the curve into sub-curves:
 * it only materializes one of the curve's two endpoints as a real, named point.
 * The point's position is not persisted — it is recomputed from the parent
 * offset curve on every (re)parse, so it follows the curve when its width or
 * source curve change.
 */
class OToolCurvePoint : public VToolSinglePoint
{
    Q_OBJECT // NOLINT

public:
    ~OToolCurvePoint() override = default;

    static auto Create(OToolCurvePointInitData initData) -> OToolCurvePoint *;

    static const QString ToolTypeStart;
    static const QString ToolTypeEnd;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::CurvePoint)
    };

    auto SplineName() const -> QString;

    auto TypeString() const -> QString;

    auto GetPosition() const -> CurvePointPosition;

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override {}

private:
    Q_DISABLE_COPY_MOVE(OToolCurvePoint) // NOLINT

    quint32 splineId{NULL_ID};
    CurvePointPosition position{CurvePointPosition::Start};

    explicit OToolCurvePoint(const OToolCurvePointInitData &initData, QGraphicsItem *parent = nullptr);
};

#endif // OTOOLCURVEPOINT_H
