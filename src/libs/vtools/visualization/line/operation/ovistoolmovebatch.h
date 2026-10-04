/************************************************************************
 **
 **  @file   ovistoolmovebatch.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The preview of a move by batch: the sources, and the ghost of every copy the chain will make.
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

#ifndef OVISTOOLMOVEBATCH_H
#define OVISTOOLMOVEBATCH_H

#include <QMetaObject>
#include <QObject>
#include <QVector>
#include <QtGlobal>

#include "../../../tools/drawTools/operation/omovebatchrule.h"
#include "../vmisc/def.h"
#include "visoperation.h"

class OMoveBatchTransform;

class OVisToolMoveBatch : public VisOperation
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolMoveBatch(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolMoveBatch() override = default;

    void RefreshGeometry() override;

    /** @brief The ordered source ids and the moves of the operation, so the preview can replay the very chain
     *  the tool will. */
    void SetMoves(const QVector<quint32> &sourceIds, const QVector<OMoveBatchMove> &moves);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolMoveBatch)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolMoveBatch) // NOLINT

    QVector<quint32> m_sourceIds{};
    QVector<OMoveBatchMove> m_moves{};

    void CreateTransformedObjects(int &iPoint, int &iCurve, const OMoveBatchTransform &transform);

    template <class Item> auto AddTransformedCurve(quint32 id, int i, const OMoveBatchTransform &transform) -> int;
};

#endif // OVISTOOLMOVEBATCH_H
