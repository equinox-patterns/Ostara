/************************************************************************
 **
 **  @file   vabstracttool.h
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
// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#ifndef VABSTRACTTOOL_H
#define VABSTRACTTOOL_H

#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vtools/visualization/visualization.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "toolsdef.h"
#include "vdatatool.h"

class VGraphicsSimpleTextItem;
class VAbstractNode;
class VScaledLine;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_CLANG("-Wdeprecated")

struct VAbstractToolInitData
{
    /** @brief id tool id, 0 if tool doesn't exist yet.*/
    quint32 id{NULL_ID};
    VMainGraphicsScene *scene{nullptr};
    VAbstractPattern *doc{nullptr};
    VContainer *data{nullptr};
    Document parse{Document::FullParse};
    Source typeCreation{Source::FromFile};
};

QT_WARNING_POP

enum class RemoveStatus : quint8
{
    Removable, // Tool can be removed
    Blocked,   // Tool has dependencies which prevent removing
    Locked,    // Tool cannot be removed
    Pending    // Collecting data about dependecies
};

/**
 * @brief The VAbstractTool abstract class for all tools.
 */
class VAbstractTool : public VDataTool
{
    Q_OBJECT // NOLINT

public:
    VAbstractTool(VAbstractPattern *doc, VContainer *data, quint32 id, QObject *parent = nullptr);
    ~VAbstractTool() override;
    auto getId() const -> quint32;

    static bool m_suppressContextMenu;

    static auto CheckFormula(const quint32 &toolId, QString &formula, const VContainer *data) -> qreal;

    static auto Colors() -> QStringList;
    static auto ColorsList() -> QMap<QString, QString>;

    static auto GetRecord(quint32 id, const Tool &toolType, VAbstractPattern *doc) -> VToolRecord;
    static void AddRecord(const VToolRecord &record, VAbstractPattern *doc);
    static void AddRecord(quint32 id, const Tool &toolType, VAbstractPattern *doc);
    static void AddNodes(VAbstractPattern *doc, QDomElement &domElement, const VPiecePath &path);
    static void AddNodes(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece);

    auto getData() const -> const VContainer *;
    auto getData() -> VContainer *;

    auto PointsList() const -> QMap<QString, quint32>;
    virtual auto getTagName() const -> QString = 0;
    virtual void ShowVisualization(bool show) = 0;
    virtual void TriggerChoosed() {}
    virtual void ChangeLabelPosition(quint32 id, const QPointF &pos);
    virtual void ChangeSegmentLabelPosition(SegmentLabel segment, const QPointF &pos);
    /** @brief Toggle the point name visibility ("Label" = point name). Distinct from VSegmentLabel::SetLabelVisible. */
    virtual void SetLabelVisible(quint32 id, bool visible);
public slots:
    /**
     * @brief FullUpdateFromFile update tool data form file.
     */
    virtual void FullUpdateFromFile() = 0;
    virtual void AllowHover(bool enabled) = 0;
    virtual void AllowSelecting(bool enabled) = 0;
    virtual void ToolSelectionType(const SelectionType &selectionType);

    /** @brief The straight segments this tool draws, as the point pairs it registered in the container.
     *
     *  A line of the pattern is such a pair, and an operation transforms the lines of the objects it copies.
     *  Most tools draw one segment — the line tool its own, a line point the one towards its base point — and
     *  that one is known from the tool type. A tool that draws more (the four sides of a rectangle, the two
     *  tracés of a dart cap) declares them here: without it those segments exist as drawing and as variables,
     *  and no operation can take them.
     *
     *  Empty by default: a tool whose only segment is the one above must not override this. */
    virtual auto DrawnSegments() const -> QVector<VDrawnSegment> { return {}; }

    /** @brief The scene items this tool draws for its segments, in the very order of DrawnSegments().
     *
     *  A drawn segment has no tool of its own — the tool that draws it carries several, all under its own id —
     *  so a click can only be heard on the item drawn for that segment. Empty by default, exactly like
     *  DrawnSegments(): a tool whose segments are not clickable declares nothing here. */
    virtual auto DrawnSegmentItems() const -> QVector<VScaledLine *> { return {}; }

    /**
     * @brief PrepareNodes convert scene objects of a piece path into piece nodes.
     * Public so the app can build a piece from the new creation dialog (OPieceDialog).
     */
    static auto PrepareNodes(const VPiecePath &path, VMainGraphicsScene *scene, VAbstractPattern *doc, VContainer *data)
        -> QVector<VPieceNode>;
signals:
    /**
     * @brief toolhaveChange emit if tool create change that need save.
     */
    void toolhaveChange();
    /**
     * @brief ChoosedTool emit if object was clicked.
     * @param id object id in container.
     * @param type type of scene object.
     */
    void ChoosedTool(quint32 id, SceneObject type);
    /**
     * @brief ChoosedSegment emit when one of the segments the tool itself draws was clicked.
     *
     * A line is not an object of the pattern: the tool id alone cannot name it — a rectangle draws four sides
     * under one id — so the two endpoints travel with it, and they are what the operation records.
     * @param id id of the tool that draws the segment.
     * @param point1 first endpoint of the segment.
     * @param point2 second endpoint of the segment.
     */
    void ChoosedSegment(quint32 id, quint32 point1, quint32 point2);
    /**
     * @brief FullUpdateTree emit if need reparse pattern file.
     */
    void LiteUpdateTree(const Document &parse);

    void ToolTip(const QString &toolTip);

protected:
    /** @brief Wire every drawn segment item to this tool, so a click on one of them is announced as a picked
     *  segment carrying the two endpoints that tell it from the tool's other segments. Called at the end of the
     *  construction, once the items and the points they join exist.
     *
     *  DrawnSegments() and DrawnSegmentItems() are read in the same order: the i-th item stands for the i-th
     *  segment. A tool that declares more items than segments — or the other way round — gives its extra items no
     *  handler, which is what a tool that draws something it does not offer has to get. */
    void InitDrawnSegmentConnections();

    /** @brief doc dom document container */
    VAbstractPattern *doc;
    /** @brief id object id. */
    const quint32 m_id;

    QPointer<Visualization> vis{};
    SelectionType m_selectionType{SelectionType::ByMouseRelease};

    /**
     * @brief AddToFile add tag with informations about tool into file.
     */
    virtual void AddToFile() = 0;
    /**
     * @brief RefreshDataInFile refresh attributes in file. If attributes don't exist create them.
     */
    virtual void RefreshDataInFile();

    template <typename T> static auto CreateNode(VContainer *data, quint32 id) -> quint32;
    static auto CreateNodeSpline(VContainer *data, quint32 id) -> quint32;
    static auto CreateNodeSplinePath(VContainer *data, quint32 id) -> quint32;
    static auto CreateNodePoint(VContainer *data, quint32 id, const QSharedPointer<VPointF> &point) -> quint32;

    template <typename T> void AddVisualization();

    virtual void SetVisualization() = 0;
    virtual void ToolCreation(const Source &typeCreation);
    /// Put the objects this tool has just written into the active layer of their pattern piece. Called from
    /// ToolCreation, for interface-created objects only — never when a file is read back.
    void AttachToActiveLayer();

    static auto AddSANode(VAbstractPattern *doc, const QString &tagName, const VPieceNode &node) -> QDomElement;
    static void AddNode(VAbstractPattern *doc, QDomElement &domElement, const VPieceNode &node);

    static auto PrepareNode(const VPieceNode &node, VMainGraphicsScene *scene, VAbstractPattern *doc, VContainer *data)
        -> quint32;

private:
    Q_DISABLE_COPY_MOVE(VAbstractTool) // NOLINT
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getId return object id.
 * @return id.
 */
inline auto VAbstractTool::getId() const -> quint32
{
    return m_id;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getData return pointer to data container.
 * @return container.
 */
inline auto VAbstractTool::getData() const -> const VContainer *
{
    return &data;
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VAbstractTool::getData() -> VContainer *
{
    return &data;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> inline void VAbstractTool::AddVisualization()
{
    auto *visual = new T(getData());
    auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
    // cppcheck-suppress unknownMacro
    SCASSERT(scene != nullptr)
    scene->addItem(visual);

    connect(visual, &Visualization::ToolTip, this, &VAbstractTool::ToolTip);
    vis = visual;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
/**
 * @brief CreateNode create new node for detail.
 * @param data container.
 * @param id id parent object.
 * @return id for new object.
 */
auto VAbstractTool::CreateNode(VContainer *data, quint32 id) -> quint32
{
    // We can't use exist object. Need create new.
    auto *node = new T(*data->GeometricObject<T>(id).data());
    node->setMode(Draw::Modeling);
    node->setIdObject(id);
    return data->AddGObject(node);
}

#endif // VABSTRACTTOOL_H
