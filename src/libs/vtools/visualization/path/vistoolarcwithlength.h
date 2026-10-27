/************************************************************************
 **
 **  @file   vistoolarcwithlength.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   9 6, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 17/08/2026.
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

#ifndef VISTOOLARCWITHLENGTH_H
#define VISTOOLARCWITHLENGTH_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "vispath.h"

class VisToolArcWithLength final : public VisPath
{
    Q_OBJECT // NOLINT

public:
    explicit VisToolArcWithLength(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~VisToolArcWithLength() override = default;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    void SetCenterId(quint32 newCenterId);
    void SetRadius(const QString &expression);
    void SetF1(const QString &expression);
    void SetLength(const QString &expression);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolArcWithLength)
    };

private:
    Q_DISABLE_COPY_MOVE(VisToolArcWithLength) // NOLINT
    VScaledEllipse *m_arcCenter{nullptr};
    VScaledEllipse *m_f1Point{nullptr};
    qreal m_radius{0};
    qreal m_f1{-1};
    bool m_f1Set{false};
    qreal m_length{0};
    quint32 m_centerId{NULL_ID};
};

//---------------------------------------------------------------------------------------------------------------------
inline void VisToolArcWithLength::SetCenterId(quint32 newCenterId)
{
    m_centerId = newCenterId;
}

#endif // VISTOOLARCWITHLENGTH_H
