/************************************************************************
 **
 **  @file   vtoolrotation.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2016
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

#ifndef VTOOLROTATION_H
#define VTOOLROTATION_H

#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "vabstractoperation.h"

template <class T> class QSharedPointer;
class VFormula;

struct VToolRotationInitData : VAbstractOperationInitData
{
    quint32 origin{NULL_ID};
    QString angle{};
};

class VToolRotation : public VAbstractOperation
{
    Q_OBJECT // NOLINT

public:
    ~VToolRotation() override = default;

    void SetDialog() override;

    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolRotation *;
    static auto Create(VToolRotationInitData &initData) -> VToolRotation *;

    static const QString ToolType;

    /** @brief The angle a rotation is built with when the pattern does not give one. The parser that draws the
     *  geometry and the tool that reads it back must agree on this value: two different defaults would open a
     *  file with the points at one angle and the dialog showing another — and save it that way. */
    static const QString DefaultAngle;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Rotation)
    };

    auto OriginPointName() const -> QString;

    auto GetFormulaAngle() const -> VFormula;
    void SetFormulaAngle(const VFormula &value);

    /** @brief The angle read as a length: the chord of the image of the reference point (see Rotation.md).
     *
     * The length is not stored, it is computed from the angle and the pivot, so the same rotation always reads
     * the same way. A zero formula when there is no reference point to measure — the options dock shows a dash
     * in that case rather than a zero that would read as a length. */
    auto GetFormulaLength() const -> VFormula;

    /** @brief Read the rotation as a length instead of an angle; it is stored as the angle it comes from.
     *
     * @return false when the length cannot belong to any rotation — further from the pivot than the circle
     *  diameter — in which case the tool is left untouched. */
    auto SetFormulaLength(const VFormula &value) -> bool;

    /** @brief Is there a point to measure a length on? A rotation built only from curves has none. */
    auto HasReferencePoint() const -> bool;

    void ShowVisualization(bool show) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void SetVisualization() override;
    void SaveDialog(QDomElement &domElement) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private:
    Q_DISABLE_COPY_MOVE(VToolRotation) // NOLINT
    quint32 m_origPointId;
    QString m_formulaAngle;

    explicit VToolRotation(const VToolRotationInitData &initData, QGraphicsItem *parent = nullptr);

    /** @brief Distance in pixels from the pivot to the point a length is measured on, 0 when there is none. */
    auto ReferenceRadius() const -> qreal;

    auto SyncDestination(const QVector<SourceItem> &source, QString &formulaAngle) -> QVector<DestinationItem>;
};

#endif // VTOOLROTATION_H
