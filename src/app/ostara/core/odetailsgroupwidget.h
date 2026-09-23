/************************************************************************
 **
 **  @file   odetailsgroupwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Group view of the Details options dock: General (alignment),
 **          Grainline (common grainline + per-member foldlines) and
 **          Information (Group / Blocks / Annotations).
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef ODETAILSGROUPWIDGET_H
#define ODETAILSGROUPWIDGET_H

#include <QWidget>

#include <functional>

#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"

class QCheckBox;
class QButtonGroup;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class OAnnotationsConfigWidget;
class ODetailsGrainlineWidget;
class OGradedAlignWidget;
class ODetailsFoldLineValuesWidget;
class OPieceInfoWidget;
class OBlockConfigWidget;
class VAbstractPattern;
class VContainer;

/**
 * @brief The ODetailsGroupWidget class — Options dock page for a selected group.
 *
 * All edits commit through the undo pipeline: group fields via
 * SavePieceGroupOptions, member foldlines via SavePieceOptions (the foldline
 * geometry lives on the member piece).
 */
class ODetailsGroupWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsGroupWidget(QWidget *parent = nullptr);
    ~ODetailsGroupWidget() override = default;

    void Clear();
    void Show(quint32 groupId, VContainer *data, VAbstractPattern *doc);
    /** @brief True if this group is the one currently displayed. The canvas holder re-emits
     *  Clicked on the press that starts a drag, so the property browser skips a redundant
     *  re-population that would flicker the lists once. */
    auto IsShowing(quint32 groupId) const -> bool { return m_groupId == groupId; }

    void OnNodeSelectedForFoldLine(quint32 nodeId);
    void CancelFoldLineSelection();
    auto IsSelectingFoldLinePoint() const -> bool { return m_pendingFoldLineMemberId != NULL_ID; }

    /** @brief Drop the orange highlight on the group's common point, if any. */
    void ClearAlignPointHighlight();

signals:
    void ShowStatusTip(const QString &text);
    void StartFoldLineMember(quint32 pieceId);
    void HighlightNode(quint32 nodeId, bool highlight);

private:
    struct MemberFLRow
    {
        QPushButton *toggleBtn{nullptr};
        QFrame *content{nullptr};
        QPushButton *defineBtn{nullptr};
        QPushButton *deactivateBtn{nullptr};
        QLabel *statusLabel{nullptr};
        QCheckBox *showFullPieceCheck{nullptr};
        QCheckBox *showMirrorLineCheck{nullptr};
        QButtonGroup *typeGroup{nullptr}; // same type buttons as the piece foldline
        QLineEdit *labelEdit{nullptr};
        // Width / Height / Center, built on the carrier's row only: the group's fold line is drawn
        // by that single member, the others are masked.
        ODetailsFoldLineValuesWidget *valuesWidget{nullptr};
    };

    void Commit(std::function<void(OPieceGroup &)> func);
    void ModifyMemberPiece(quint32 pieceId, std::function<void(VPiece &)> func);
    // The layout flags belong to the members, not to the group: reading shows what the members say, writing
    // writes into all of them (one undo step). See OPieceGroupLayoutFlags.
    auto MemberPieceIds() const -> QVector<quint32>;
    void ShowLayoutFlags();
    void WriteLayoutFlags(bool forceFlipping, bool symmetricalCopy);
    void RefreshGroupMemberFoldLines();
    void BuildMemberFoldLines();
    void OnMemberToggle(quint32 pieceId, bool expanded);
    void OnMemberDefineClicked(quint32 pieceId);
    void OnMemberDeactivate(quint32 pieceId);
    void UpdateMemberFoldLineStatus(quint32 pieceId);
    void RefreshMemberFoldLineStatuses();
    auto IsFoldLineCarrier(quint32 pieceId) const -> bool;
    void SetMemberFoldLineSelecting(quint32 pieceId, bool selecting);
    void ResetFoldLineHighlight();

    QTabWidget *m_tabWidget{nullptr};
    QTabWidget *m_infoSubTabs{nullptr};
    OGradedAlignWidget *m_alignWidget{nullptr};

    // Forced alignment point: the group's common point, the one chosen when the pieces were linked.
    QCheckBox *m_alignOnPointCheck{nullptr};
    QCheckBox *m_showAlignPointCheck{nullptr};

    // Layout flags of the members. Checked when EVERY member carries the flag.
    QCheckBox *m_forceFlippingCheck{nullptr};
    QCheckBox *m_symmetricalCopyCheck{nullptr};
    bool m_showAlignPointState{true};          // UI state, per group: on by default
    quint32 m_alignPointHighlightId{NULL_ID};  // node currently painted orange, 0 when none

    void ApplyAlignPointControls(const OPieceGroup &group);
    void RefreshAlignPointHighlight();
    void OnAlignOnPointToggled(bool checked);
    void OnShowAlignPointToggled(bool checked);
    auto ReferenceMemberNode(const OPieceGroup &group) const -> quint32;

    // Information — Group sub-tab (same structure as the PB Piece's Piece sub-tab)
    QLineEdit *m_nameEdit{nullptr};

    // Common grainline editor (same widget as the piece)
    ODetailsGrainlineWidget *m_grainlineWidget{nullptr};

    // Information sub-tabs
    OPieceInfoWidget *m_groupInfoWidget{nullptr};
    OBlockConfigWidget *m_blockConfigWidget{nullptr};
    OAnnotationsConfigWidget *m_annotationsConfigWidget{nullptr};

    // Member foldlines
    QVBoxLayout *m_memberFLLayout{nullptr};
    QHash<quint32, MemberFLRow> m_memberFLRows{};

    quint32 m_groupId{NULL_ID};
    VContainer *m_data{nullptr};
    VAbstractPattern *m_doc{nullptr};

    // Foldline definition state (one member at a time)
    quint32 m_pendingFoldLineMemberId{NULL_ID};
    quint32 m_foldLineFirstPoint{NULL_ID};
    VPiecePath m_foldLinePath{};

    bool m_updating{false};

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(ODetailsGroupWidget) // NOLINT
};

#endif // ODETAILSGROUPWIDGET_H
