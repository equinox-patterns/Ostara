/************************************************************************
 **
 **  @file   vsimplecurve.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   7 5, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 28/09/2026.
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

#ifndef VSIMPLECURVE_H
#define VSIMPLECURVE_H

#include <QColor>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "vabstractsimple.h"
#include "vcurvepathitem.h"

class VAbstractCurve;
template <class T> class QSharedPointer;

class VSimpleCurve : public VAbstractSimple, public VCurvePathItem
{
    Q_OBJECT // NOLINT

public:
    VSimpleCurve(quint32 id, const QSharedPointer<VAbstractCurve> &curve, QObject *parent = nullptr);
    ~VSimpleCurve() override = default;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::SimpleCurve)
    };

    void TriggerChoosed() override;

    void RefreshGeometry(const QSharedPointer<VAbstractCurve> &curve);

    /// Marks the curve as picked for the layer-add workflow: drawn orange and bold, like the active point in
    /// Details mode. This renderer is what draws the curve results of an operation — an arc, a spline, an
    /// ellipse — so without it a marked operation showed its points and its lines turning, and left its curves
    /// in their own colour.
    void SetLayerSelected(bool selected);

    /// Marks the curve as clicked for removal from a layer: magenta and bold, on top of SetLayerSelected.
    void SetInLayer(bool picked);

signals:
    /**
     * @brief Choosed send id when clicked.
     * @param id point id.
     */
    void Choosed(quint32 id);
    void Selected(bool selected, quint32 id);

public slots:
    void CurveChoosed();
    void CurveSelected(bool selected);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void ScalePenWidth() override;

private:
    Q_DISABLE_COPY_MOVE(VSimpleCurve) // NOLINT

    QSharedPointer<VAbstractCurve> m_curve;
    bool m_isHovered;
    bool m_layerSelected{false};
    bool m_inLayer{false};
};

#endif // VSIMPLECURVE_H
