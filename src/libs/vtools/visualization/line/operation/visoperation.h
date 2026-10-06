/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 9, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

#ifndef VISOPERATION_H
#define VISOPERATION_H

#include <QtGlobal>

#include <QPair>

#include "../visline.h"
#include "../vpatterndb/vcontainer.h"

class VisOperation : public VisLine
{
    Q_OBJECT // NOLINT

public:
    explicit VisOperation(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~VisOperation() override;

    void SetObjects(const QVector<quint32> &objects);

    /** @brief Lines attached to the operation, as pairs of source endpoint ids. They are drawn like the
     *  operated objects: highlighted on the source side, ghosted on the transformed side. */
    void SetPreviewLines(const QVector<QPair<quint32, quint32>> &lines);

    void StartAction();
    void VisualMode(quint32 id) override;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::GroupOperation)
    };

protected:
    auto GetPoint(quint32 i, VColorRole role) -> VScaledEllipse *;
    auto GetCurve(quint32 i, VColorRole role) -> VCurvePathItem *;

    auto Objects() const -> const QVector<quint32> &;

    /** @brief True when at least one line is attached to the operation. The operated-object list can be
     *  empty while a line is already selected: an operation may be started from a line alone. */
    auto HasPreviewLines() const -> bool { return !m_previewLines.isEmpty(); }

    auto ObjectSelected() const -> bool;

    auto AddOriginCurve(quint32 id, int &i) -> QGraphicsPathItem *;
    auto AddOriginPoint(quint32 id, int &i) -> VScaledEllipse *;

    auto CreateOriginObjects(int &iPoint, int &iCurve) -> QVector<QGraphicsItem *>;

    /** @brief Image of a point under the transformation of the tool. One line per operation tool: a
     *  rotation, a translation, a reflection. The default keeps the point in place: an operation tool
     *  that cannot select lines yet never calls this, and one that can must override it. */
    virtual auto PointImage(const QPointF &point) const -> QPointF { Q_UNUSED(point) return point; }

    /** @brief Image of a point under the transformation of the tool, asked by the object it is.
     *
     *  An operation that moves whole objects — a pleat, which takes the objects a region holds — cannot answer
     *  from a place: two objects of the same place do not travel the same distance. The default hands the place
     *  over to PointImage, which is what every operation that reads a place answers. */
    virtual auto PointImage(quint32 id, const QPointF &point) const -> QPointF
    {
        Q_UNUSED(id)
        return PointImage(point);
    }

    /** @brief True when the point has an image without being in the object list: either the transformation
     *  leaves it in place (rotation centre, mirror axis points), or it maps it onto another point (the merge
     *  point of a transfer, whose image is the anchor). Either way a line anchored on it is transformed. */
    virtual auto IsFixedPoint(quint32 id) const -> bool
    {
        Q_UNUSED(id)
        return false;
    }

    /** @brief Draw the attached lines: source side always, transformed side only when both endpoints
     *  have an image and the transformation is known - so the preview never promises a line the operation
     *  will not create. */
    void RefreshPreviewLines(bool drawImages);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VisOperation) // NOLINT

    auto GetLine(quint32 i, VColorRole role) -> VScaledLine *;

    static auto PointPosition(const VContainer *data, quint32 id) -> QPointF;

    QVector<quint32> m_objects{};

    QVector<VScaledEllipse *> m_points{};
    QVector<VCurvePathItem *> m_curves{};
    QVector<VScaledLine *> m_lines{};

    QVector<QPair<quint32, quint32>> m_previewLines{};

    bool m_startAction{false};
};

//---------------------------------------------------------------------------------------------------------------------
inline void VisOperation::StartAction()
{
    m_startAction = true;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VisOperation::Objects() const -> const QVector<quint32> &
{
    return m_objects;
}

#endif // VISOPERATION_H
