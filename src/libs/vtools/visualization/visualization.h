/************************************************************************
 **
 **  @file   visualization.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 8, 2014
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

#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <type_traits>

#include <QGraphicsItem>
#include <QLoggingCategory>
#include <QObject>
#include <QtGlobal>

#include "../vgeometry/vabstractcurve.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vwidgets/vcurvepathitem.h"
#include "../vwidgets/vmaingraphicsscene.h"

Q_DECLARE_LOGGING_CATEGORY(vVis) // NOLINT

class VScaledEllipse;
class VScaledLine;
class VContainer;
class VInternalVariable;

enum class Mode : qint8
{
    Creation,
    Show
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wsuggest-final-types")
QT_WARNING_DISABLE_GCC("-Wsuggest-final-methods")

class Visualization : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit Visualization(const VContainer *data);
    ~Visualization() override = default;

    virtual void RefreshGeometry() = 0;
    virtual void VisualMode(quint32 id) = 0;

    void SetLineStyle(const Qt::PenStyle &value);
    auto LineStyle() const -> Qt::PenStyle;

    auto GetData() const -> const VContainer *;
    void SetData(const VContainer *data);

    auto GetMode() const -> Mode;
    void SetMode(const Mode &value);

    static auto FindLengthFromUser(const QString &expression,
                                   const QHash<QString, QSharedPointer<VInternalVariable>> *vars, bool fromUser = true)
        -> qreal;
    static auto FindValFromUser(const QString &expression,
                                const QHash<QString, QSharedPointer<VInternalVariable>> *vars, bool fromUser = true)
        -> qreal;
    static auto CorrectAngle(qreal angle) -> qreal;

    auto CurrentToolTip() const -> QString;

    void RefreshToolTip() const;

signals:
    void ToolTip(const QString &toolTip) const; // clazy:exclude=const-signal-or-slot

public slots:
    void MousePos(const QPointF &scenePos);

protected:
    virtual void InitPen() = 0;
    virtual void AddOnScene() = 0;

    static auto InitPoint(VColorRole role, QGraphicsItem *parent, qreal z = 0) -> VScaledEllipse *;
    static void DrawPoint(QGraphicsEllipseItem *point, const QPointF &pos, Qt::PenStyle style = Qt::SolidLine);
    virtual void DrawLine(VScaledLine *lineItem, const QLineF &line, Qt::PenStyle style = Qt::SolidLine);
    static void DrawPath(VCurvePathItem *pathItem, const QPainterPath &path, Qt::PenStyle style = Qt::SolidLine,
                         Qt::PenCapStyle cap = Qt::SquareCap);
    static void DrawPath(VCurvePathItem *pathItem, const QPainterPath &path,
                         const QVector<DirectionArrow> &directionArrows, Qt::PenStyle style = Qt::SolidLine,
                         Qt::PenCapStyle cap = Qt::SquareCap);

    template <typename Item> void AddItem(Item *item);

    template <class Item> auto InitItem(VColorRole role, QGraphicsItem *parent) -> Item *;

    /** @brief A mark of the gesture: the answer the user has to read — which end is green, which one is red — painted
     *  above the echo of the objects the preview lays over them.
     *
     *  It takes two things, because Qt orders two siblings by the ItemStacksBehindParent flag first and by the Z only
     *  afterwards (see qt_closestLeaf, qgraphicsitem_p.h): the flag the item inherits from InitItem is cleared, or
     *  the echo points — which wear none — stay above it whatever its Z, and kMarkZ is given, which puts it above the
     *  echo points, in the same pass, where the Z does decide. */
    template <class Item> auto InitMark(VColorRole role, QGraphicsItem *parent) -> Item *;

    /** @brief The Z of a mark of the gesture: above the echo of the objects, whose points sit at 0 and whose lines
     *  and curves sit at the 1 InitItem gives them. */
    static constexpr qreal kMarkZ = 2.;

    /** @brief The Z of every preview of a tool — the single number the two roots of the previews share, `VisLine`
     *  and `VisPath`, and the whole of its meaning is a comparison with **the pattern**.
     *
     *  The pattern's own table, which this value answers, is written in three files: the piece at **10**
     *  (`vtoolseamallowance.cpp`), the handle of a curve at **10,5** (`vcontrolpointspline.cpp`), a point at **11**
     *  (`vtoolsinglepoint.cpp`, `kPointZ`) and the base point at **12** (`vtoolbasepoint.cpp`, `kBasePointZ`).
     *
     *  ⚠️ **The previews were left at 1 on 13-10-2026** (`47ff6de3`), the very commit that raised the points and the
     *  piece to 10-12: every one of them — the orange echo of the objects the user picks, and the green and red dots
     *  of the gesture, in **every** operation — passed nine levels under the piece and under the points, where
     *  nothing could be read. This number is what puts them back above, and the two roots are the only two places it
     *  is applied: a preview item is a child of its root, so one Z lifts the whole group.
     *
     *  The two dots of the gesture stay above the echo by their own Z (@a kMarkZ), which is **relative** to this one
     *  and did not move — the rule of 10-10-2026 (see InitMark) still holds. */
    static constexpr qreal kPreviewZ = 12.5;

    static auto GetPointItem(QVector<VScaledEllipse *> &points, quint32 i, VColorRole role, QGraphicsItem *parent)
        -> VScaledEllipse *;
    static auto GetCurveItem(QVector<VCurvePathItem *> &curves, quint32 i, VColorRole role, QGraphicsItem *parent)
        -> VCurvePathItem *;

    static auto LengthToUser(qreal value) -> QString;
    static auto AngleToUser(qreal value) -> QString;

    void SetScenePos(QPointF pos);
    auto ScenePos() const -> QPointF;

    void SetToolTip(const QString &tooltip);

    void StartVisualMode();

private:
    Q_DISABLE_COPY_MOVE(Visualization) // NOLINT

    const VContainer *m_data;
    QPointF m_scenePos{};
    Qt::PenStyle m_lineStyle{Qt::SolidLine};
    QString m_toolTip{};
    Mode m_mode{Mode::Creation};
};

// cppcheck-suppress unknownMacro
QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
template <typename Item> inline void Visualization::AddItem(Item *item)
{
    SCASSERT(item != nullptr)
    auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
    SCASSERT(scene != nullptr)
    qCDebug(vVis) << "AddItem:" << static_cast<void *>(item)
                  << "→ scene:" << static_cast<void *>(scene);
    if (item->scene() != scene)
    {
        scene->addItem(item);
    }
    connect(scene, &VMainGraphicsScene::mouseMove, item, &Visualization::MousePos, Qt::UniqueConnection);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Item> inline auto Visualization::InitItem(VColorRole role, QGraphicsItem *parent) -> Item *
{
    auto *item = new Item(role, parent);
    item->setZValue(1);
    item->setFlags(QGraphicsItem::ItemStacksBehindParent);
    item->setVisible(false);
    item->setAcceptedMouseButtons(Qt::NoButton);
    if constexpr (std::is_same_v<Item, VScaledLine>)
    {
        item->SetShowArrows(true);
        item->RefreshPen();
    }
    else if constexpr (std::is_same_v<Item, VScaledEllipse>)
    {
        item->RefreshScale();
    }
    else if constexpr (std::is_same_v<Item, VCurvePathItem>)
    {
        item->RefreshScale();
    }
    // Note: the refresh above is required because items created after their parent is already in the
    // scene never receive ItemSceneHasChanged (the base QGraphicsItem constructor consumes it), so the
    // derived itemChange() would never run RefreshPen()/RefreshScale() and the item would keep the
    // default black pen.
    return item;
}

//---------------------------------------------------------------------------------------------------------------------
template <class Item> inline auto Visualization::InitMark(VColorRole role, QGraphicsItem *parent) -> Item *
{
    Item *item = InitItem<Item>(role, parent);
    item->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    item->setZValue(kMarkZ);
    return item;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto Visualization::GetMode() const -> Mode
{
    return m_mode;
}

//---------------------------------------------------------------------------------------------------------------------
inline void Visualization::SetMode(const Mode &value)
{
    m_mode = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto Visualization::CurrentToolTip() const -> QString
{
    return m_toolTip;
}

//---------------------------------------------------------------------------------------------------------------------
inline void Visualization::SetScenePos(QPointF pos)
{
    m_scenePos = pos;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto Visualization::ScenePos() const -> QPointF
{
    return m_scenePos;
}

//---------------------------------------------------------------------------------------------------------------------
inline void Visualization::SetToolTip(const QString &tooltip)
{
    this->m_toolTip = tooltip;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto Visualization::LineStyle() const -> Qt::PenStyle
{
    return GetMode() == Mode::Creation && m_lineStyle == Qt::NoPen ? Qt::SolidLine : m_lineStyle;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto Visualization::GetData() const -> const VContainer *
{
    return m_data;
}

//---------------------------------------------------------------------------------------------------------------------
inline void Visualization::SetData(const VContainer *data)
{
    this->m_data = data;
}

#endif // VISUALIZATION_H
