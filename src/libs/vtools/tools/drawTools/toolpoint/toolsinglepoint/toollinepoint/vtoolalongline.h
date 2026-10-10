/************************************************************************
 **
 **  @file   vtoolalongline.h
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

#ifndef VTOOLALONGLINE_H
#define VTOOLALONGLINE_H


#include <QDomElement>
#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "vtoollinepoint.h"

template <class T> class QSharedPointer;

struct VToolAlongLineInitData : VToolLinePointInitData
{
    QString formula{QStringLiteral("100.0")}; // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 firstPointId{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 secondPointId{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString secondLineRoleName{}; // NOLINT(misc-non-private-member-variables-in-classes)
};

/**
 * @brief The VToolAlongLine class tool for creation point along line.
 */
class VToolAlongLine : public VToolLinePoint
{
    Q_OBJECT // NOLINT
public:
    ~VToolAlongLine() override = default;
    
    void SetDialog() override;
    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene,
                       VAbstractPattern *doc, VContainer *data) -> VToolAlongLine*;
    static auto Create(VToolAlongLineInitData &initData) -> VToolAlongLine*;
    static const QString ToolType;
    auto type() const -> int override {return Type;}
    enum { Type = UserType + static_cast<int>(Tool::AlongLine)};

    auto SecondPointName() const -> QString;

    auto GetFormulaLength() const -> VFormula override;

    auto GetLineRoleName() const -> QString;
    void SetLineRoleName(const QString &suffix);

    auto GetSecondLineRoleName() const -> QString;
    void SetSecondLineRoleName(const QString &suffix);

    /** @brief The two segments the cut makes: the first point → the point, then the point → the second point,
     *  in the very order VToolAlongLine::Create registers them in the container.
     *
     *  This tool draws both — the first is the inherited main line, the second an item of its own — and without
     *  this declaration the second one existed only as the Line_ and AngleLine_ variables, so the pen chosen in
     *  the dialog never reached it and no operation could take it. */
    auto DrawnSegments() const -> QVector<VDrawnSegment> override
    {
        return {{basePointId, getId(), getLineType(), GetLineColor()},
                {getId(), m_secondPointId, getLineType(), GetLineColor()}};
    }

    /** @brief The two items that draw those segments, in the same order: a click on one picks that segment
     *  alone, so an operation takes either half of the cut line. */
    auto DrawnSegmentItems() const -> QVector<VScaledLine *> override { return {mainLine, m_secondLine}; }

    void ShowVisualization(bool show) override;
protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id=NULL_ID) override;
protected:
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override;
    void RefreshGeometry() override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private:
    Q_DISABLE_COPY_MOVE(VToolAlongLine) // NOLINT

    /** @brief secondPointId id second point of line. */
    quint32 m_secondPointId;

    /** @brief secondLine the item that draws the segment from the point to the second point of the line. */
    VScaledLine *m_secondLine{nullptr};

    bool m_hoverOwnsVisualization{false};

    explicit VToolAlongLine(const VToolAlongLineInitData &initData, QGraphicsItem *parent = nullptr);
};

#endif // VTOOLALONGLINE_H
