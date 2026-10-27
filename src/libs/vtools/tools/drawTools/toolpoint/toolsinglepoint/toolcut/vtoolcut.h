/************************************************************************
 **
 **  @file   vtoolcut.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 6, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 18/10/2026.
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

#ifndef VTOOLCUT_H
#define VTOOLCUT_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../../../../../visualization/visualization.h"
#include "../../../../vdatatool.h"
#include "../../../toolcurve/vtoolabstractcurve.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vtools/undocommands/renameobject.h"
#include "../vtoolsinglepoint.h"

class VFormula;
class VSegmentLabel;
class VSimpleCurve;
class VAbstractOperation;

struct VToolCutInitData : VToolSinglePointInitData
{
    using VToolSinglePointInitData::VToolSinglePointInitData;

    quint32 baseCurveId{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString formula{};            // NOLINT(misc-non-private-member-variables-in-classes)
    QString aliasSuffix1{};       // NOLINT(misc-non-private-member-variables-in-classes)
    QString aliasSuffix2{};       // NOLINT(misc-non-private-member-variables-in-classes)
    QString name1{};              // NOLINT(misc-non-private-member-variables-in-classes)
    QString name2{};              // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 segment1Id{NULL_ID};  // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 segment2Id{NULL_ID};  // NOLINT(misc-non-private-member-variables-in-classes)
    bool showTwoSegments{false};  // NOLINT(misc-non-private-member-variables-in-classes)
    QString lineType{};           // NOLINT(misc-non-private-member-variables-in-classes)
    QString lineColor{};          // NOLINT(misc-non-private-member-variables-in-classes)
    quint8 prolongedSegment{0};   // NOLINT(misc-non-private-member-variables-in-classes)
    qreal segment1Mx{labelMX};    // NOLINT(misc-non-private-member-variables-in-classes)
    qreal segment1My{labelMY};    // NOLINT(misc-non-private-member-variables-in-classes)
    qreal segment2Mx{labelMX};    // NOLINT(misc-non-private-member-variables-in-classes)
    qreal segment2My{labelMY};    // NOLINT(misc-non-private-member-variables-in-classes)
};

enum class VToolCutNameField : quint8
{
    Name1,
    Name2,
    AliasSuffix1,
    AliasSuffix2
};

class VToolCut : public VToolSinglePoint
{
    Q_OBJECT // NOLINT

public:
    explicit VToolCut(const VToolCutInitData &initData, QGraphicsItem *parent = nullptr);
    ~VToolCut() override = default;
    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::Cut)
    };

    auto GetFormulaLength() const -> VFormula;
    void SetFormulaLength(const VFormula &value);

    auto GetName1() const -> QString;
    void SetName1(const QString &name);

    auto GetName2() const -> QString;
    void SetName2(const QString &name);

    auto GetAliasSuffix1() const -> QString;
    void SetAliasSuffix1(const QString &alias);

    auto GetAliasSuffix2() const -> QString;
    void SetAliasSuffix2(const QString &alias);

    auto CurveName() const -> QString;

    auto BaseCurveId() const -> quint32;

    auto IsShowTwoSegments() const -> bool;
    void SetShowTwoSegments(bool value);

    /** @brief Look of the piece prolonging the curve, used only when the cut goes past an end. */
    auto GetProlongationType() const -> QString;
    void SetProlongationType(const QString &value);
    auto GetProlongationColor() const -> QString;
    void SetProlongationColor(const QString &value);

    /** @brief Which sub-curve prolongs the curve: 0 none, 1 the first, 2 the second. */
    auto ProlongedSegment() const -> quint8;
    void SetProlongedSegment(quint8 segment);

    /** @brief SetSegmentReplaced is called by a nested cut tool that replaces one of our sub-curves. */
    void SetSegmentReplaced(quint32 segmentId, bool replaced);

    /** @brief A layer's eye, object by object: the point itself, or one of the two sub-curves.
     *
     *  The cut used to hide as a whole — `VToolSinglePoint::GroupVisibility` ignores the object it is handed —
     *  because its halves were not addressable. They are now: each one can be filed in a layer of its own, so
     *  the eye of that layer must hide that half alone, and the point's layer must leave the halves alone. */
    void GroupVisibility(quint32 object, bool visible) override;

    /** @brief The scene item a layer pair speaks about: the point for the tool's own id, a half for a segment. */
    auto LayerObjectItem(quint32 objectId) const -> QGraphicsItem *;

    void ShowVisualization(bool show) override;
    void ChangeSegmentLabelPosition(SegmentLabel segment, const QPointF &pos) override;

    auto IsRemovable() const -> RemoveStatus override;

public slots:
    void SetDetailsMode(bool mode) override;
    void Enable() override;
    void EnableToolMove(bool move) override;
    void AllowSegmentHover(bool enabled);
    void ToolSelectionType(const SelectionType &selectionType) override;
    void SetSegmentLabelVisible(bool visible);
    void AllowLabelSelecting(bool enabled) override;

protected:
    /** @brief formula keep formula of length */
    QString formula;

    quint32 baseCurveId;
    bool detailsMode;

    QString m_name1;
    QString m_name2;

    QString m_aliasSuffix1{};
    QString m_aliasSuffix2{};

    quint32 m_segment1Id{NULL_ID};
    quint32 m_segment2Id{NULL_ID};

    qreal m_segment1Mx{labelMX};
    qreal m_segment1My{labelMY};
    qreal m_segment2Mx{labelMX};
    qreal m_segment2My{labelMY};

    VSegmentLabel *m_segment1Label{nullptr};
    VSegmentLabel *m_segment2Label{nullptr};

    struct ToolChanges
    {
        QString oldLabel{};
        QString newLabel{};
        QString oldName1{};
        QString newName1{};
        QString oldName2{};
        QString newName2{};
        QString oldAliasSuffix1{};
        QString newAliasSuffix1{};
        QString oldAliasSuffix2{};
        QString newAliasSuffix2{};

        auto HasChanges() const -> bool
        {
            return oldLabel != newLabel || oldName1 != newName1 || oldName2 != newName2
                   || oldAliasSuffix1 != newAliasSuffix1 || oldAliasSuffix2 != newAliasSuffix2;
        }

        auto LabelChanged() const -> bool { return oldLabel != newLabel; }
        auto Name1Changed() const -> bool { return oldName1 != newName1; }
        auto Name2Changed() const -> bool { return oldName2 != newName2; }
        auto AliasSuffix1Changed() const -> bool { return oldAliasSuffix1 != newAliasSuffix1; }
        auto AliasSuffix2Changed() const -> bool { return oldAliasSuffix2 != newAliasSuffix2; }
    };

    void ProcessToolCutOptions(const QDomElement &oldDomElement,
                               const QDomElement &newDomElement,
                               const ToolChanges &changes);

    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void RefreshGeometry() override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;
    void ReadToolAttributes(const QDomElement &domElement) override;

    template <typename T> void ShowToolVisualization(bool show);

    template<typename T>
    static void FixSubCurveNames(VToolCutInitData &initData,
                                 const QSharedPointer<T> &baseCurve,
                                 const QSharedPointer<T> &leftSub,
                                 const QSharedPointer<T> &rightSub);

private slots:
    void SegmentChoosed(quint32 id, SceneObject type);
    void Segment1LabelPositionChanged(const QPointF &pos);
    void Segment2LabelPositionChanged(const QPointF &pos);

private:
    Q_DISABLE_COPY_MOVE(VToolCut) // NOLINT

    bool m_hoverOwnsVisualization{false};

    /** @brief m_showTwoSegments when true the two sub-curves replace the base curve on the canvas. */
    bool m_showTwoSegments{false};

    /** @brief Look of the prolongation piece; meaningless while the cut stays on the curve. */
    QString m_prolongationType{};
    QString m_prolongationColor{};

    /** @brief Sub-curve prolonging the curve (0 none, 1 first, 2 second). Derived, never stored. */
    quint8 m_prolongedSegment{0};

    /** @brief Items drawing the two sub-curves on the canvas, so they can be clicked directly.
     *
     *  They are top-level items of the scene, not children of the point: a layer must be able to hold one half
     *  while the point's layer is hidden, and Qt hides every child with its parent. They are QObject children of
     *  the tool all the same, so they die with it. */
    VSimpleCurve *m_segment1Curve{nullptr};
    VSimpleCurve *m_segment2Curve{nullptr};

    /** @brief Whether the layer of each half lets it be drawn. The "show two segments" box decides the rest. */
    bool m_segment1LayerVisible{true};
    bool m_segment2LayerVisible{true};

    /** @brief A nested cut tool may replace one of our sub-curves by its own two sub-curves. */
    bool m_segment1Replaced{false};
    bool m_segment2Replaced{false};

    void RefreshSegmentCurves();
    void ApplySegmentsVisibility();
    void EnsureSegmentsInScene(QGraphicsScene *scene);
    auto BaseCurveItem() const -> VToolAbstractCurve *;
    auto BaseCurveOperation() const -> VAbstractOperation *;
    auto BaseCurveCut() const -> VToolCut *;

    void UpdateNameField(VToolCutNameField field, const QString &value);
    auto HasConflict(const QString &value, VToolCutNameField currentField) const -> bool;
};

//---------------------------------------------------------------------------------------------------------------------
template<typename T>
inline void VToolCut::FixSubCurveNames(VToolCutInitData &initData,
                                       const QSharedPointer<T> &baseCurve,
                                       const QSharedPointer<T> &leftSub,
                                       const QSharedPointer<T> &rightSub)
{
    bool fixName1 = false;
    bool fixName2 = false;
    bool saveName1 = false;
    bool saveName2 = false;
    if (initData.name1.isEmpty())
    {
        if (!baseCurve->IsDerivative())
        {
            initData.name1 = leftSub->HeadlessName();
        }
        else
        {
            initData.name1 = GenerateUniqueCurveName(initData.data,
                                                     leftSub->GetTypeHead(),
                                                     QStringLiteral("LSubCurve"),
                                                     initData.name);
            fixName1 = true;
        }

        saveName1 = true;
    }

    if (initData.name2.isEmpty())
    {
        if (!baseCurve->IsDerivative())
        {
            initData.name2 = rightSub->HeadlessName();
        }
        else
        {
            initData.name2 = GenerateUniqueCurveName(initData.data,
                                                     rightSub->GetTypeHead(),
                                                     QStringLiteral("RSubCurve"),
                                                     initData.name);
            fixName2 = true;
        }

        saveName2 = true;
    }

    if (fixName1 || fixName2)
    {
        VAbstractApplication::VApp()->getUndoStack()->push(
            new RenameSegmentCurves(RenameAlias::CurveType(baseCurve->getType()),
                                    initData.name,
                                    fixName1 ? initData.name1 : QString(),
                                    fixName2 ? initData.name2 : QString(),
                                    initData.doc));
    }

    if (saveName1 || saveName2)
    {
        QDomElement domElement = initData.doc->FindElementById(initData.id);
        if (!domElement.isElement())
        {
            qCDebug(vTool, "Can't find tool with id = %u", initData.id);
            return;
        }

        if (saveName1)
        {
            initData.doc->SetAttribute(domElement, AttrCurveName1, initData.name1);
        }

        if (saveName2)
        {
            initData.doc->SetAttribute(domElement, AttrCurveName2, initData.name2);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VToolCut::BaseCurveId() const -> quint32
{
    return baseCurveId;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> inline void VToolCut::ShowToolVisualization(bool show)
{
    if (show)
    {
        if (vis.isNull())
        {
            AddVisualization<T>();
            SetVisualization();
        }
        else
        {
            if (T *visual = qobject_cast<T *>(vis))
            {
                visual->show();
            }
        }
    }
    else
    {
        delete vis.data();
    }

    VDataTool *parent = VAbstractPattern::getTool(VAbstractTool::data.GetGObject(baseCurveId)->getIdTool());
    if (auto *parentCurve = qobject_cast<VToolAbstractCurve *>(parent))
    {
        detailsMode ? parentCurve->ShowHandles(detailsMode) : parentCurve->ShowHandles(show);
    }
}

#endif // VTOOLCUT_H
