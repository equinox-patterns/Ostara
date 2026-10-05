/************************************************************************
 **
 **  @file   vtoollinepoint.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#ifndef VTOOLLINEPOINT_H
#define VTOOLLINEPOINT_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "../vpatterndb/vformula.h"
#include "../vtoolsinglepoint.h"

template <class T> class QSharedPointer;

struct VToolLinePointInitData : VToolSinglePointInitData
{
    using VToolSinglePointInitData::VToolSinglePointInitData;

    QString typeLine{TypeLineLine};
    QString lineColor{ColorBlack};
};

/**
 * @brief The VToolLinePoint class parent for all tools what create point with line.
 */
class VToolLinePoint : public VToolSinglePoint
{
    Q_OBJECT // NOLINT

public:
    VToolLinePoint(VAbstractPattern *doc, VContainer *data, const quint32 &id, const QString &typeLine,
                   const QString &lineColor, const QString &formula, const quint32 &basePointId, const qreal &angle,
                   const QString &notes, QGraphicsItem *parent = nullptr);
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::LinePoint)
    };

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    virtual auto GetFormulaLength() const -> VFormula;
    void SetFormulaLength(const VFormula &value);

    auto BasePointName() const -> QString;

    /** @brief Id of the point this tool draws its implicit line from. Used by the operation tools to
     *  transform that line together with the point. */
    auto BasePointId() const -> quint32 { return basePointId; }

    /** @brief The one segment a line point draws: towards its base point, in the order it was registered —
     *  the order the Line_ and AngleLine_ variables of the tool are named in. */
    auto DrawnSegments() const -> QVector<VDrawnSegment> override
    {
        return {{basePointId, getId(), getLineType(), GetLineColor()}};
    }

    /** @brief The item that draws that one segment, in the same order: a click on it picks the segment. */
    auto DrawnSegmentItems() const -> QVector<VScaledLine *> override { return {mainLine}; }

    auto GetAngle() const -> qreal;
    void SetAngle(const qreal &value);

    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &value);

    /** @brief The point holds the click (property browser): show the line it comes from, as on hover. */
    void SetActivePoint(bool active) override;

public slots:
    void Enable() override;

protected:
    /** @brief formula string with length formula. */
    QString formulaLength;

    /** @brief angle line angle. */
    qreal angle;

    /** @brief basePointId id base line point. */
    quint32 basePointId;

    /** @brief mainLine line item. */
    VScaledLine *mainLine{nullptr};

    /** @brief lineColor color of a line. */
    QString lineColor;

    void RefreshGeometry() override;

    /** @brief Writes the pen the tool asks for — its one colour, its one style — on one of the lines it draws,
     *  then converges the pen so that the emphasis of a hover or of a click still reads on top of it. Used for
     *  the inherited main line and, by a tool that draws more than one line, for each of the others.
     *  @param line a line the tool draws. nullptr is ignored. */
    void RefreshAuthoredPen(VScaledLine *line);

    /** @brief Tells a line the tool draws to ask this tool for its colour — see
     *  VScaledLine::SetAuthoredColorProvider. Called for every line whose pen the tool owns. */
    void AttachAuthoredColor(VScaledLine *line);

    /** @brief Writes the emphasis of a hover or of a click on every line the tool draws, at once. The colour of
     *  each line is rewritten from the tool first: the item that answers for a colour is the tool, never a
     *  neighbouring line. */
    void UpdateLinesHighlight();
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    auto MakeToolTip() const -> QString override;

    void ProcessLinePointToolOptions(const QDomElement &oldDomElement,
                                     const QDomElement &newDomElement,
                                     const QString &newLabel,
                                     const QString &newBasePointLabel);

    void SyncLineRoleNameToDom();
    void SyncLineRoleNameToDom(const QString &pairKey);

private:
    Q_DISABLE_COPY_MOVE(VToolLinePoint) // NOLINT
};

#endif // VTOOLLINEPOINT_H
