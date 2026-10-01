/************************************************************************
 **
 **  @file   vtoolline.h
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

#ifndef VTOOLLINE_H
#define VTOOLLINE_H

#include <QDomElement>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vwidgets/scalesceneitems.h"
#include "vdrawtool.h"

template <class T> class QSharedPointer;
class QGraphicsSceneMouseEvent;

struct VToolLineInitData : VDrawToolInitData
{
    quint32 firstPoint{NULL_ID};
    quint32 secondPoint{NULL_ID};
    QString typeLine{TypeLineLine};
    QString lineColor{ColorBlack};
    QString lineRoleName{};

    /** @brief derived true when an operation tool creates the line: the line belongs to the operation,
     *  which persists its id and rebuilds it on parse. Such a line writes no element of its own and
     *  pushes no undo command. */
    bool derived{false};
};

/**
 * @brief The VToolLine class tool for creation line.
 */
class VToolLine : public VDrawTool, public VScaledLine
{
    Q_OBJECT // NOLINT

public:
    ~VToolLine() override = default;

    void SetDialog() override;
    static auto Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolLine *;
    static auto Create(VToolLineInitData initData) -> VToolLine *;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Line)
    };
    auto getTagName() const -> QString override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    auto FirstPointName() const -> QString;
    auto SecondPointName() const -> QString;

    auto FirstPointId() const -> quint32 { return firstPoint; }
    auto SecondPointId() const -> quint32 { return secondPoint; }

    /** @brief A line can be selected by an operation tool, so it must announce itself like any other
     *  selectable object. */
    void TriggerChoosed() override { emit ChoosedTool(m_id, SceneObject::Line); }

    auto GetLineRoleName() const -> QString;
    void SetLineRoleName(const QString &suffix);

    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &value);

    void SetNotes(const QString &notes) override;

    void ShowVisualization(bool show) override;

    void SetLineType(const QString &value) override;
    void GroupVisibility(quint32 object, bool visible) override;

    /** @brief Take the type and the colour a parse has just read, without writing anything.
     *
     * A transformed line has no element of its own — the operation that produced it keeps both — so there is
     * nothing to save here, only a pen to put on. Called for every line that already exists by
     * OOperationLines::CreateLines, the one place that re-reads what the operation holds: without it a lite parse
     * left the line with the pen it was built with, and an edit to that pen was invisible until the file was
     * reopened. */
    void RefreshPenFromParse(const QString &typeLine, const QString &lineColor);

signals:
    /** @brief DeleteRequested the user asked to delete this line. A derived line belongs to the operation
     *  that draws it: the operation connects this request to its own deletion, exactly like a destination
     *  point (see VAbstractOperation::InitOperatedObjects). */
    void DeleteRequested();

    /** @brief ContextMenuRequested a derived line asks its operation to show the context menu. A derived
     *  line has no menu of its own (it is rebuilt from the operation at every parse), so the right click is
     *  forwarded to the operation, exactly like a destination point (see VAbstractOperation::InitOperatedObjects). */
    void ContextMenuRequested(QGraphicsSceneContextMenuEvent *event, quint32 id);

public slots:
    void FullUpdateFromFile() override;
    void Enable() override;
    void AllowHover(bool enabled) override;
    void AllowSelecting(bool enabled) override;

protected slots:
    void ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id = NULL_ID) override;

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void AddToFile() override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;
    void RefreshPen() override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void SaveDialog(QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SetVisualization() override;
    auto MakeToolTip() const -> QString override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;

private:
    Q_DISABLE_COPY_MOVE(VToolLine) // NOLINT

    /** @brief firstPoint id first line point. */
    quint32 firstPoint;

    /** @brief secondPoint id second line point. */
    quint32 secondPoint;

    /** @brief lineColor color of a line. */
    QString lineColor;

    bool m_acceptHoverEvents;

    /** @brief True when an operation tool created this line. Such a line has no element of its own in the
     *  pattern: it is rebuilt from its operation, so it cannot be edited on its own. */
    bool m_derived{false};

    explicit VToolLine(const VToolLineInitData &initData, QGraphicsItem *parent = nullptr);

    void RefreshGeometry();
};

#endif // VTOOLLINE_H
