/************************************************************************
 **
 **  @file   vtoolarcpoint.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/08/2026
 **
 **  @brief  Point at the start (P1) or end (P2) of an arc, without
 **          cutting the arc into sub-arcs.
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

#ifndef VTOOLARCPOINT_H
#define VTOOLARCPOINT_H

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
class VArc;

/// Which end of the arc the point materializes.
enum class ArcPointPosition : qint8
{
    Start,
    End
};

struct VToolArcPointInitData : VToolSinglePointInitData
{
    quint32 arcId{NULL_ID};
    ArcPointPosition position{ArcPointPosition::Start};
};

/**
 * @brief The VToolArcPoint class point at the start (P1) or end (P2) of an arc.
 *
 * Unlike VToolCutArc, this tool does not cut the arc into sub-arcs: it only
 * materializes one of the arc's two endpoints as a real, named point. The
 * point's position is not persisted — it is recomputed from the parent arc on
 * every (re)parse, so it follows the arc when its radius or angles change.
 */
class VToolArcPoint : public VToolSinglePoint
{
    Q_OBJECT // NOLINT

public:
    ~VToolArcPoint() override = default;

    static auto Create(VToolArcPointInitData initData) -> VToolArcPoint *;

    static const QString ToolTypeStart;
    static const QString ToolTypeEnd;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::ArcPoint)
    };

    auto ArcName() const -> QString;

    auto TypeString() const -> QString;

    auto GetPosition() const -> ArcPointPosition;

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override {}

private:
    Q_DISABLE_COPY_MOVE(VToolArcPoint) // NOLINT

    quint32 arcId{NULL_ID};
    ArcPointPosition position{ArcPointPosition::Start};

    explicit VToolArcPoint(const VToolArcPointInitData &initData, QGraphicsItem *parent = nullptr);
};

#endif // VTOOLARCPOINT_H
