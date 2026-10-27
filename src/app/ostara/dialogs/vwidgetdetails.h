/************************************************************************
 **
 **  @file   vwidgetdetails.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 6, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and model patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 13/09/2026.
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

#ifndef VWIDGETDETAILS_H
#define VWIDGETDETAILS_H

#include <QSet>
#include <QWidget>

#include "ui_vwidgetdetails.h"

#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

class QEvent;
class QMenu;
class QPoint;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class VAbstractPattern;
class VContainer;

class VWidgetDetails : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit VWidgetDetails(VContainer *data, VAbstractPattern *doc, QWidget *parent = nullptr);
    ~VWidgetDetails() override;

    /** @brief Link workflow states (pattern of the passmark machine in ODetailsPieceWidget). */
    enum class GroupToolState
    {
        Idle,
        Selecting,     // clicking pieces to link
        NodeSelecting  // clicking the common point
    };

signals:
    void Highlight(quint32 id);
    void ShowStatusTip(const QString &text);
    void GroupSelected(quint32 groupId);
    void PieceSelected(quint32 id);
    void GroupToolStateChanged(GroupToolState state);
    // The group's in-layout state (or the "Hide not in layout" checkbox) changed: MainWindow
    // uses this to hide/show the whole group on the canvas (holder + members).
    void GroupVisibilityChanged(quint32 groupId, bool visible);

public:
    auto IsSelectingLinkPieces() const -> bool { return m_groupToolState == GroupToolState::Selecting; }
    auto IsSelectingLinkNode() const -> bool { return m_groupToolState == GroupToolState::NodeSelecting; }

public slots:
    void UpdateList();
    void SelectDetail(quint32 id);
    void ToggledPiece(quint32 id);
    void ToggledGroup(quint32 groupId);

    /** @brief Toggle a group's in-layout state. Owns the undo command and its connections, so the
     *  canvas context menu goes through it too: the tree icon, the member rows and the
     *  GroupVisibilityChanged signal (which masks the holder and the members) all follow. */
    void ToggleGroupInLayout(quint32 groupId);

    // Link workflow — driven by MainWindow canvas routing.
    void OnPieceClickedForGroup(quint32 pieceId);
    void OnNodeSelected(quint32 nodeId);
    void FinishGroupSelection();
    void CancelGroupSelection();
    void SetGroupToolState(GroupToolState state);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void ItemClicked(QTreeWidgetItem *item, int column);
    void ItemChanged(QTreeWidgetItem *item, int column);
    void ShowContextMenu(const QPoint &pos);
    void on_checkBoxHideNotInLayout_stateChanged();
    void on_linkPiecesButton_clicked();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VWidgetDetails) // NOLINT
    Ui::VWidgetDetails *ui;
    VAbstractPattern *m_doc;
    VContainer *m_data;
    QTimer *m_updateListTimer;

    void FillTree();
    void AddChildrenRows(QTreeWidgetItem *parent, const VPiece &det);
    void AddChildrenRows(QTreeWidgetItem *parent, const OPieceGroup &group);
    void ToggleSectionDetails(bool select);
    void ToggledPieceItem(QTreeWidgetItem *item);
    void ToggledGroupItem(QTreeWidgetItem *item);
    void ToggleInLayout(quint32 id);
    void SelectGroup(quint32 groupId);
    void ValidateGroupSelection();
    void EnterNodeSelectMode();
    void CompleteGroupCreation();
    void UnlinkGroup(quint32 groupId);
    void DeselectSelectedPieces();
    void RefreshGroupedChildren(const QVector<quint32> &pieceIds);

    QSet<quint32> m_selectedPiecesForGroup;
    QHash<quint32, QPointF> m_tempOffsets;
    // Common point of the group being linked: its construction source id, and the member whose own
    // copy of that point was clicked. The latter is the X reference of the graded alignment.
    quint32 m_pendingAlignSourceId{NULL_ID};
    quint32 m_pendingAlignMemberId{NULL_ID};
    GroupToolState m_groupToolState{GroupToolState::Idle};
    bool m_filling{false};
};

#endif // VWIDGETDETAILS_H
