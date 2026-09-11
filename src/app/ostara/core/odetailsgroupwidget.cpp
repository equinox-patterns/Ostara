/************************************************************************
 **
 **  @file   odetailsgroupwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Group view of the Details options dock.
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

#include "odetailsgroupwidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QEnterEvent>
#include <QEvent>
#include <QFrame>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QUndoStack>

#include "../vlayout/vfoldline.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/floatItemData/floatitemdef.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/savepiecegroupoptions.h"
#include "../widgets/ogradedalignwidget.h"
#include "odetailsgrainlinewidget.h"
#include "../vtools/undocommands/savepieceoptions.h"
#include "../vtheme/vtheme.h"
#include "oannotationsconfigwidget.h"
#include "oblockconfigwidget.h"
#include "odetailsfoldlinevalueswidget.h"
#include "ofoldlinetypes.h"
#include "opieceinfowidget.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// Toggle button of a member fold line: theme-aware caret-right (collapsed) / caret-down
// (expanded); while collapsed, the caret-right switches to the dark set on hover so it stays
// visible on the hover/accent background (dark theme already uses the dark set → no change).
class MemberFoldToggleButton : public QPushButton
{
public:
    explicit MemberFoldToggleButton(const QString &text, const QString &theme, QWidget *parent)
      : QPushButton(text, parent), m_theme(theme)
    {
        connect(this, &QPushButton::toggled, this, [this]() { UpdateIcon(); });
        UpdateIcon(); // initial state: collapsed, not hovered → theme caret-right
    }

    void SetTheme(const QString &theme)
    {
        m_theme = theme;
        UpdateIcon();
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        QPushButton::enterEvent(event);
        UpdateIcon();
    }
    void leaveEvent(QEvent *event) override
    {
        QPushButton::leaveEvent(event);
        UpdateIcon();
    }

private:
    void UpdateIcon()
    {
        if (isChecked())
        {
            setIcon(QIcon(QStringLiteral(":/icons/%1/caret-down_%1.svg").arg(m_theme)));
        }
        else if (underMouse())
        {
            setIcon(QIcon(QStringLiteral(":/icons/dark/caret-right_dark.svg")));
        }
        else
        {
            setIcon(QIcon(QStringLiteral(":/icons/%1/caret-right_%1.svg").arg(m_theme)));
        }
    }

    QString m_theme;
};

//---------------------------------------------------------------------------------------------------------------------
auto FoldLineDefined(const VPiece &det) -> bool
{
    return det.GetMirrorLineStartPoint() != NULL_ID && det.GetMirrorLineEndPoint() != NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto ValidateMirrorLinePoints(const VPiecePath &path, quint32 startPoint, quint32 endPoint) -> bool
{
    if (startPoint == NULL_ID || endPoint == NULL_ID || startPoint == endPoint)
    {
        return false;
    }

    const int startIdx = path.indexOfNode(startPoint);
    const int endIdx = path.indexOfNode(endPoint);
    if (startIdx == -1 || endIdx == -1)
    {
        return false;
    }

    if (path.at(startIdx).IsExcluded() || path.at(endIdx).IsExcluded())
    {
        return false;
    }

    // Walk clockwise from startIdx (wrapping around the closed loop) to check
    // that endPoint is reachable without crossing excluded nodes.
    const auto nodeCount = path.CountNodes();
    for (int i = 1; i < nodeCount; ++i)
    {
        const auto idx = (startIdx + i) % nodeCount;
        const VPieceNode &node = path.at(idx);
        if (!node.IsExcluded() && node.GetTypeTool() == Tool::NodePoint)
        {
            if (node.GetId() == endPoint)
            {
                return true;
            }
        }
    }
    return false;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsGroupWidget::ODetailsGroupWidget(QWidget *parent)
  : QWidget(parent)
{
    m_tabWidget = new QTabWidget(this);

    // ── General tab: QGroup Alignment only ───────────────────────────
    // Same widget as everywhere else (Dialog Piece step 2, PB Piece general tab):
    // Horizontal/Vertical labels + themed alignment icons. Page style matches the
    // PB Piece tab (SubtiliteCode §23): QFrame#detailsOptionsTabPage carries $DockBody.
    auto *generalPage = new QFrame(this);
    generalPage->setFrameStyle(QFrame::NoFrame);
    generalPage->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *generalLayout = new QVBoxLayout(generalPage);
    m_alignWidget = new OGradedAlignWidget(generalPage);
    generalLayout->addWidget(m_alignWidget);
    generalLayout->addStretch();

    connect(m_alignWidget, &OGradedAlignWidget::AlignHChanged, this,
            [this](int v)
    {
        if (m_updating)
        {
            return;
        }
        Commit([v](OPieceGroup &group) { group.alignH = v; });
    });
    connect(m_alignWidget, &OGradedAlignWidget::AlignVChanged, this,
            [this](int v)
    {
        if (m_updating)
        {
            return;
        }
        Commit([v](OPieceGroup &group) { group.alignV = v; });
    });

    m_tabWidget->addTab(generalPage, tr("General"));

    // ── Grainline tab: common grainline + member foldlines ──────────
    // Same widget as the PB Piece (ODetailsGrainlineWidget) in a QFrame#detailsOptionsTabPage
    // (SubtiliteCode §23): activate grainline, length presets, rotation, simple/double.
    auto *grainlinePage = new QFrame(this);
    grainlinePage->setFrameStyle(QFrame::NoFrame);
    grainlinePage->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *grainlineLayout = new QVBoxLayout(grainlinePage);

    m_grainlineWidget = new ODetailsGrainlineWidget(grainlinePage);
    grainlineLayout->addWidget(m_grainlineWidget);

    auto *memberSection = new QGroupBox(tr("Member fold lines"), grainlinePage);
    auto *memberLayout = new QVBoxLayout(memberSection);
    m_memberFLLayout = memberLayout;
    grainlineLayout->addWidget(memberSection);
    grainlineLayout->addStretch();

    m_tabWidget->addTab(grainlinePage, tr("Grainline"));

    // ── Information tab: Group / Blocks / Annotations ────────────────
    // Same structure as the PB Piece's Information tab: the PAGE is the container
    // (QFrame#detailsOptionsTabPage carries $DockBody, margins 0), and each sub-tab
    // mirrors its PB Piece counterpart (SubtiliteCode §23).
    auto *infoPage = new QFrame(this);
    infoPage->setFrameStyle(QFrame::NoFrame);
    infoPage->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *infoLayout = new QVBoxLayout(infoPage);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    auto *infoTabs = new QTabWidget(infoPage);
    infoTabs->setObjectName(QStringLiteral("informationTabWidget"));

    auto *groupTab = new QFrame(infoTabs);
    groupTab->setFrameStyle(QFrame::NoFrame);
    groupTab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
    auto *groupLayout = new QVBoxLayout(groupTab);
    groupLayout->setContentsMargins(8, 8, 8, 0);
    groupLayout->setSpacing(8);

    auto *form = new QFormLayout();
    m_nameEdit = new QLineEdit(groupTab);
    form->addRow(tr("Name"), m_nameEdit);
    groupLayout->addLayout(form);

    m_groupInfoWidget = new OPieceInfoWidget(groupTab);
    groupLayout->addWidget(m_groupInfoWidget);
    groupLayout->addStretch(1);

    m_blockConfigWidget = new OBlockConfigWidget(infoTabs);
    m_annotationsConfigWidget = new OAnnotationsConfigWidget(infoTabs);
    infoTabs->addTab(groupTab, tr("Group"));
    infoTabs->addTab(m_blockConfigWidget, tr("Blocks"));
    infoTabs->addTab(m_annotationsConfigWidget, tr("Annotations"));
    infoLayout->addWidget(infoTabs);
    m_tabWidget->addTab(infoPage, tr("Information"));

    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]()
    {
        if (m_updating)
        {
            return;
        }
        const QString name = m_nameEdit->text();
        Commit([name](OPieceGroup &group) { group.name = name; });
    });

    // Information sub-widgets commit through the group snapshot pipeline.
    const auto modifyGroup = [this](std::function<void(IPieceInfo &)> func)
    {
        Commit([func](OPieceGroup &group) { func(group); });
    };
    m_groupInfoWidget->SetModifyPending(modifyGroup);
    m_blockConfigWidget->SetModifyPending(modifyGroup);
    m_annotationsConfigWidget->SetModifyPending(modifyGroup);

    // The common grainline commits through the group snapshot pipeline as well.
    m_grainlineWidget->SetModifyPending([this](std::function<void(VGrainlineData &)> func)
    {
        Commit([func](OPieceGroup &group) { func(group.m_glGrainline); });
    });

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(m_tabWidget);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::Show(quint32 groupId, VContainer *data, VAbstractPattern *doc)
{
    m_groupId = groupId;
    m_data = data;
    m_doc = doc;
    m_updating = true;

    const auto *groups = data != nullptr ? data->DataPieceGroups() : nullptr;
    const OPieceGroup group = groups != nullptr ? groups->value(groupId) : OPieceGroup();

    // Name (Group sub-tab, same as the Piece sub-tab)
    if (m_nameEdit != nullptr)
    {
        m_nameEdit->setText(group.name);
    }

    // Alignment (OGradedAlignWidget — same widget as the piece dialog / piece browser)
    if (m_alignWidget != nullptr)
    {
        m_alignWidget->SetAlignH(group.alignH);
        m_alignWidget->SetAlignV(group.alignV);
    }

    // Common grainline editor (same widget as the PB Piece); the canvas holder
    // centers a freshly-activated grainline itself (no centerProvider here).
    m_grainlineWidget->Show(group.m_glGrainline);

    m_groupInfoWidget->Show(group, data);
    m_blockConfigWidget->Show(group, data);
    m_annotationsConfigWidget->Show(group, data);

    BuildMemberFoldLines();

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::Clear()
{
    m_groupId = NULL_ID;
    m_data = nullptr;
    m_doc = nullptr;
    m_updating = true;
    if (m_nameEdit != nullptr)
    {
        m_nameEdit->clear();
    }
    m_grainlineWidget->Clear();
    m_groupInfoWidget->Clear();
    m_blockConfigWidget->Clear();
    m_annotationsConfigWidget->Clear();

    // Delete the member-row widgets (same cleanup as BuildMemberFoldLines) so a
    // later Show rebuilds a clean list instead of stacking duplicates.
    for (auto it = m_memberFLRows.constBegin(); it != m_memberFLRows.constEnd(); ++it)
    {
        const MemberFLRow &row = it.value();
        delete row.toggleBtn;
        delete row.content;
    }
    m_memberFLRows.clear();
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::Commit(std::function<void(OPieceGroup &)> func)
{
    if (m_updating || m_groupId == NULL_ID || m_data == nullptr || m_doc == nullptr)
    {
        return;
    }
    auto *groups = m_data->DataPieceGroups();
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return;
    }
    OPieceGroup oldGroup = groups->value(m_groupId);
    OPieceGroup newGroup = oldGroup;
    func(newGroup);
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SavePieceGroupOptions(oldGroup, newGroup, m_doc, m_data, m_groupId));

    // Re-sync the whole group view after the commit (the piece PB is re-shown by the
    // undo refresh; the group dock is not). Keeps the sub-tab lists — annotations,
    // blocks — in sync right after an add/edit/remove without re-clicking the group.
    Show(m_groupId, m_data, m_doc);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ModifyMemberPiece(quint32 pieceId, std::function<void(VPiece &)> func)
{
    if (m_data == nullptr || m_doc == nullptr || m_data->DataPieces() == nullptr ||
        not m_data->DataPieces()->contains(pieceId))
    {
        return;
    }
    VPiece oldDet = m_data->GetPiece(pieceId);
    VPiece newDet = oldDet;
    func(newDet);
    VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, m_doc, pieceId));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::RefreshGroupMemberFoldLines()
{
    // The foldline mask decision (UpdateFoldLine) depends on which member is the group's foldline
    // carrier — re-render every member so the new carrier shows and the previous one re-masks.
    const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return;
    }
    for (quint32 pid : groups->value(m_groupId).pieceIds)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool != nullptr)
        {
            tool->RefreshGroupedChildren(); // re-derives the mask; includes RefreshFoldLine
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::BuildMemberFoldLines()
{
    // Drop stale rows (delete the tracked widgets; deleting a widget also removes it from its layout).
    for (auto it = m_memberFLRows.constBegin(); it != m_memberFLRows.constEnd(); ++it)
    {
        const MemberFLRow &row = it.value();
        delete row.toggleBtn;
        delete row.content;
    }
    m_memberFLRows.clear();

    const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return;
    }
    const OPieceGroup group = groups->value(m_groupId);

    for (quint32 pieceId : group.pieceIds)
    {
        // A member may have been deleted while still listed in the group — skip it.
        if (m_data->DataPieces() == nullptr || not m_data->DataPieces()->contains(pieceId))
        {
            continue;
        }
        const VPiece det = m_data->GetPiece(pieceId);
        const QString name = det.GetName().isEmpty() ? tr("Unnamed") : det.GetName();

        MemberFLRow row;

        // Toggle button: caret-right (collapsed) / caret-down (expanded), regular set, theme-aware,
        // with the dark-set caret-right on hover while collapsed (MemberFoldToggleButton).
        const bool dark = VAbstractApplication::VApp()->Settings()->GetThemeMode() == VThemeMode::Dark;
        const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");

        auto *toggleBtn = new MemberFoldToggleButton(name, theme, this);
        toggleBtn->setCheckable(true);
        toggleBtn->setObjectName(QStringLiteral("memberFoldToggle"));
        row.toggleBtn = toggleBtn;
        const quint32 pid = pieceId;
        connect(toggleBtn, &QPushButton::toggled, this, [this, pid](bool expanded) { OnMemberToggle(pid, expanded); });
        m_memberFLLayout->addWidget(toggleBtn);

        // Transparent container: the "Member fold lines" QGroupBox carries the background
        // (SubtiliteCode §23 — no inner box with a background).
        row.content = new QFrame(this);
        row.content->setFrameStyle(QFrame::NoFrame);
        auto *contentLayout = new QVBoxLayout(row.content);

        row.statusLabel = new QLabel(row.content);
        contentLayout->addWidget(row.statusLabel);

        auto *btnRow = new QHBoxLayout();
        row.defineBtn = new QPushButton(tr("Define foldline"), row.content);
        row.deactivateBtn = new QPushButton(tr("Deactivate"), row.content);
        btnRow->addWidget(row.defineBtn);
        btnRow->addWidget(row.deactivateBtn);
        contentLayout->addLayout(btnRow);

        row.showFullPieceCheck = new QCheckBox(tr("Show unfolded piece"), row.content);
        row.showMirrorLineCheck = new QCheckBox(tr("Show symmetry line"), row.content);
        contentLayout->addWidget(row.showFullPieceCheck);
        contentLayout->addWidget(row.showMirrorLineCheck);

        // Foldline type — the very same buttons as the piece foldline widget ("With text" is the
        // default foldline type), built from the shared list so the two pages can never offer
        // different sets. Both rows (buttons + label edit) span the member content width (~85% of
        // the Member fold lines group box), like the piece.
        auto *typeLayout = new QHBoxLayout();
        row.typeGroup = new QButtonGroup(row.content);
        row.typeGroup->setExclusive(true);
        for (const auto &item : OFoldLineTypeItems())
        {
            auto *btn = new QPushButton(OFoldLineTypeText(item), row.content);
            btn->setCheckable(true);
            row.typeGroup->addButton(btn, static_cast<int>(item.type));
            typeLayout->addWidget(btn);
        }
        contentLayout->addLayout(typeLayout);

        // Built on every member row, but shown on the carrier's one only — see
        // UpdateMemberFoldLineStatus. Building it everywhere is deliberate: a carrier change then
        // only has to flip visibility, instead of rebuilding rows from inside the click handler of
        // one of their own buttons, which would destroy the emitting widget mid-signal.
        row.valuesWidget = new ODetailsFoldLineValuesWidget(row.content);
        row.valuesWidget->SetModifyPending([this, pid](std::function<void(VPiece &)> func)
        {
            ModifyMemberPiece(pid, func);
        });
        contentLayout->addWidget(row.valuesWidget);

        row.labelEdit = new QLineEdit(row.content);
        row.labelEdit->setPlaceholderText(tr("Cut on fold"));
        contentLayout->addWidget(row.labelEdit);

        row.content->hide();
        m_memberFLLayout->addWidget(row.content);

        connect(row.defineBtn, &QPushButton::clicked, this, [this, pid]() { OnMemberDefineClicked(pid); });
        connect(row.deactivateBtn, &QPushButton::clicked, this, [this, pid]() { OnMemberDeactivate(pid); });
        connect(row.showFullPieceCheck, &QCheckBox::toggled, this, [this, pid](bool checked)
        {
            if (m_updating)
            {
                return;
            }
            ModifyMemberPiece(pid, [checked](VPiece &det)
            {
                det.SetShowFullPiece(checked);
                if (checked)
                {
                    det.SetShowMirrorLine(true);
                }
            });
            UpdateMemberFoldLineStatus(pid);
        });
        connect(row.showMirrorLineCheck, &QCheckBox::toggled, this, [this, pid](bool checked)
        {
            if (m_updating)
            {
                return;
            }
            ModifyMemberPiece(pid, [checked](VPiece &det) { det.SetShowMirrorLine(checked); });
            UpdateMemberFoldLineStatus(pid);
        });
        connect(row.typeGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled), this,
                [this, pid, row](QAbstractButton *button, bool checked)
        {
            if (m_updating || !checked)
            {
                return;
            }
            const auto lineType = static_cast<FoldLineType>(row.typeGroup->id(button));
            ModifyMemberPiece(pid, [lineType](VPiece &det) { det.SetFoldLineType(lineType); });
        });
        connect(row.labelEdit, &QLineEdit::editingFinished, this, [this, pid, row]
        {
            if (m_updating)
            {
                return;
            }
            const QString label = row.labelEdit->text();
            ModifyMemberPiece(pid, [label](VPiece &det) { det.SetFoldLineLabel(label); });
        });

        UpdateMemberFoldLineStatus(pieceId);
        m_memberFLRows.insert(pieceId, row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnMemberToggle(quint32 pieceId, bool expanded)
{
    if (m_memberFLRows.contains(pieceId))
    {
        m_memberFLRows.value(pieceId).content->setVisible(expanded);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnMemberDefineClicked(quint32 pieceId)
{
    // Toggle: if already selecting for this piece, cancel.
    if (m_pendingFoldLineMemberId == pieceId)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
        if (tool != nullptr && tool->IsInPieceIsolation())
        {
            tool->ExitPieceIsolation();
        }
        m_pendingFoldLineMemberId = NULL_ID;
        m_foldLineFirstPoint = NULL_ID;
        m_foldLinePath = VPiecePath();
        SetMemberFoldLineSelecting(pieceId, false);
        emit ShowStatusTip(QString());
        return;
    }

    // Selecting for a different member: cancel the previous one first.
    if (m_pendingFoldLineMemberId != NULL_ID)
    {
        auto *prevTool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(m_pendingFoldLineMemberId));
        if (prevTool != nullptr && prevTool->IsInPieceIsolation())
        {
            prevTool->ExitPieceIsolation();
        }
        SetMemberFoldLineSelecting(m_pendingFoldLineMemberId, false);
    }

    auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
    if (tool == nullptr)
    {
        return;
    }

    tool->EnterPieceIsolation();
    m_pendingFoldLineMemberId = pieceId;
    m_foldLineFirstPoint = NULL_ID;
    m_foldLinePath = m_data->GetPiece(pieceId).GetPath();
    SetMemberFoldLineSelecting(pieceId, true);
    emit StartFoldLineMember(pieceId);
    emit ShowStatusTip(tr("Select the first point of the foldline — respect clockwise direction"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnMemberDeactivate(quint32 pieceId)
{
    if (m_pendingFoldLineMemberId == pieceId)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
        if (tool != nullptr && tool->IsInPieceIsolation())
        {
            tool->ExitPieceIsolation();
        }
        m_pendingFoldLineMemberId = NULL_ID;
        m_foldLineFirstPoint = NULL_ID;
        m_foldLinePath = VPiecePath();
    }
    ModifyMemberPiece(pieceId, [](VPiece &det)
    {
        det.SetMirrorLineStartPoint(NULL_ID);
        det.SetMirrorLineEndPoint(NULL_ID);
        det.SetShowMirrorLine(false);
        det.SetShowFullPiece(false);
    });
    // This member no longer carries the group's foldline.
    Commit([pieceId](OPieceGroup &group)
    {
        if (group.m_foldLineMemberId == pieceId)
        {
            group.m_foldLineMemberId = NULL_ID;
        }
    });
    RefreshGroupMemberFoldLines();
    // No carrier any more: every row has to drop the dimensions.
    RefreshMemberFoldLineStatuses();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::OnNodeSelectedForFoldLine(quint32 nodeId)
{
    if (m_pendingFoldLineMemberId == NULL_ID)
    {
        return;
    }

    if (m_foldLinePath.indexOfNode(nodeId) == -1)
    {
        ResetFoldLineHighlight();
        m_foldLineFirstPoint = NULL_ID;
        emit ShowStatusTip(tr("This point does not belong to the selected piece"));
        return;
    }

    if (m_foldLineFirstPoint == NULL_ID)
    {
        m_foldLineFirstPoint = nodeId;
        emit ShowStatusTip(tr("Select the second point of the foldline — respect clockwise direction"));
        return;
    }

    const quint32 startPoint = m_foldLineFirstPoint;
    const quint32 endPoint = nodeId;
    const quint32 pieceId = m_pendingFoldLineMemberId;

    if (!ValidateMirrorLinePoints(m_foldLinePath, startPoint, endPoint))
    {
        ResetFoldLineHighlight();
        m_foldLineFirstPoint = NULL_ID;
        emit ShowStatusTip(tr("Invalid points — start over"));
        return;
    }

    auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
    if (tool != nullptr && tool->IsInPieceIsolation())
    {
        tool->ExitPieceIsolation();
    }
    m_pendingFoldLineMemberId = NULL_ID;
    m_foldLineFirstPoint = NULL_ID;
    m_foldLinePath = VPiecePath();
    SetMemberFoldLineSelecting(pieceId, false);
    emit ShowStatusTip(QString());

    ModifyMemberPiece(pieceId, [startPoint, endPoint](VPiece &det)
    {
        det.SetMirrorLineStartPoint(startPoint);
        det.SetMirrorLineEndPoint(endPoint);
        det.SetShowMirrorLine(true);
        det.SetShowFullPiece(false);
        det.SetFoldLineType(FoldLineType::TwoArrowsTextAbove);
    });
    // The group's foldline is carried by this member — its VToolSeamAllowance renders it.
    Commit([pieceId](OPieceGroup &group) { group.m_foldLineMemberId = pieceId; });
    // The mask decision depends on the carrier: re-render every member's foldline now.
    RefreshGroupMemberFoldLines();
    // The carrier moved: the previous one has to stop showing the dimensions.
    RefreshMemberFoldLineStatuses();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::CancelFoldLineSelection()
{
    if (m_pendingFoldLineMemberId == NULL_ID)
    {
        return;
    }
    auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(m_pendingFoldLineMemberId));
    if (tool != nullptr && tool->IsInPieceIsolation())
    {
        tool->ExitPieceIsolation();
    }
    SetMemberFoldLineSelecting(m_pendingFoldLineMemberId, false);
    m_pendingFoldLineMemberId = NULL_ID;
    m_foldLineFirstPoint = NULL_ID;
    m_foldLinePath = VPiecePath();
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::UpdateMemberFoldLineStatus(quint32 pieceId)
{
    if (m_data == nullptr || m_data->DataPieces() == nullptr || not m_data->DataPieces()->contains(pieceId) ||
        not m_memberFLRows.contains(pieceId))
    {
        return;
    }
    const VPiece det = m_data->GetPiece(pieceId);
    const bool defined = FoldLineDefined(det);
    MemberFLRow row = m_memberFLRows.value(pieceId);
    if (defined)
    {
        row.statusLabel->setText(
            QStringLiteral("<span style='color:#228B22;'>✓ %1</span>").arg(tr("Foldline defined")));
    }
    else
    {
        row.statusLabel->setText(
            QStringLiteral("<span style='color:#DC143C;'>✗ %1</span>").arg(tr("Foldline not defined")));
    }
    row.deactivateBtn->setVisible(defined);
    m_updating = true;
    row.showFullPieceCheck->setChecked(det.IsShowFullPiece());
    row.showMirrorLineCheck->setChecked(det.IsShowMirrorLine());
    row.showMirrorLineCheck->setEnabled(det.IsShowFullPiece());
    // No button matches a type Ostara no longer offers, and Qt refuses to uncheck the checked
    // button of an exclusive group — so the exclusivity is lifted for the pass.
    row.typeGroup->setExclusive(false);
    for (auto *btn : row.typeGroup->buttons())
    {
        btn->setChecked(static_cast<FoldLineType>(row.typeGroup->id(btn)) == det.GetFoldLineType());
    }
    row.typeGroup->setExclusive(true);

    // The label only exists for the type that draws one.
    row.labelEdit->setText(det.GetFoldLineLabel());
    row.labelEdit->setEnabled(VFoldLine::TypeHasLabel(det.GetFoldLineType()));

    // The three dimensions appear on the CARRIER's row only: the group's fold line is drawn by that
    // single member and the others are masked, so on any other row the fields would change nothing
    // the user could see. Hidden too on "Invisible fold line", which keeps the fold but draws no
    // marker at all.
    const bool showValues = defined && det.GetFoldLineType() != FoldLineType::None && IsFoldLineCarrier(pieceId);
    row.valuesWidget->setVisible(showValues);

    if (showValues)
    {
        row.valuesWidget->Show(det, m_data, pieceId);
    }
    else
    {
        // Also skips building the mirror axis, which is expensive when the main path is hidden.
        row.valuesWidget->Clear();
    }

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::RefreshMemberFoldLineStatuses()
{
    // Called whenever the carrier may have moved: every row decides again whether it shows the
    // dimensions. Deliberately not a row rebuild — one of these calls comes from a row's own button.
    const QList<quint32> pieceIds = m_memberFLRows.keys();
    for (quint32 pieceId : pieceIds)
    {
        UpdateMemberFoldLineStatus(pieceId);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsGroupWidget::IsFoldLineCarrier(quint32 pieceId) const -> bool
{
    const auto *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    if (groups == nullptr || not groups->contains(m_groupId))
    {
        return false;
    }
    return groups->value(m_groupId).m_foldLineMemberId == pieceId;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::SetMemberFoldLineSelecting(quint32 pieceId, bool selecting)
{
    if (m_memberFLRows.contains(pieceId))
    {
        MemberFLRow row = m_memberFLRows.value(pieceId);
        row.defineBtn->setText(selecting ? tr("Cancel") : tr("Define foldline"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsGroupWidget::ResetFoldLineHighlight()
{
    // No persistent node-highlight state in this widget; the status tip is enough.
}
