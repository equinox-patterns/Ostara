/************************************************************************
 **
 **  @file   odetailspiecewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Piece properties widget for the Details mode options dock
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

#include "odetailspiecewidget.h"

#include "odetailsfoldlinewidget.h"
#include "odetailsgrainlinewidget.h"
#include "odetailsseamallowancewidget.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vtools/dialogs/tools/piece/dialogmainpath.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/savepieceoptions.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QUndoStack>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
ODetailsPieceWidget::ODetailsPieceWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget();
    m_tabWidget->setObjectName(QStringLiteral("detailsOptionsTabWidget"));

    // ── General tab ──────────────────────────────────────────────
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);
        auto *form = new QFormLayout();

        m_nameEdit = new QLineEdit();
        form->addRow(tr("Name"), m_nameEdit);

        auto *numberRow = new QHBoxLayout();
        numberRow->setSpacing(4);
        m_pieceNumberSpin = new QSpinBox();
        m_pieceNumberSpin->setRange(0, 99);
        m_pieceNumberSpin->setSpecialValueText(QStringLiteral("—"));
        // Number is a placeholder until OPieceCartouche (Phase C) provides the backend.
        m_pieceNumberSpin->setEnabled(false);
        numberRow->addWidget(m_pieceNumberSpin);
        m_pieceTotalLabel = new QLabel(tr("/ auto total"));
        numberRow->addWidget(m_pieceTotalLabel);
        numberRow->addStretch();
        form->addRow(tr("Number"), numberRow);

        layout->addLayout(form);

        // Graded alignment
        auto *alignGroupBox = new QGroupBox(tr("Graded alignment"));
        auto *alignLayout = new QVBoxLayout(alignGroupBox);

        // Horizontal
        auto *hLabel = new QLabel(tr("Horizontal"));
        auto *hLayout = new QHBoxLayout();
        m_alignHGroup = new QButtonGroup(this);
        m_alignHGroup->setExclusive(true);

        const struct
        {
            int value;
            const char *text;
        } hItems[] = {
            {1, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Left")},
            {2, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Center")},
            {3, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Right")},
        };
        for (int i = 0; i < 3; ++i)
        {
            auto *btn = new QPushButton(tr(hItems[i].text));
            btn->setCheckable(true);
            btn->setProperty("alignValue", hItems[i].value);
            m_alignHGroup->addButton(btn, hItems[i].value);
            hLayout->addWidget(btn);
            m_alignHBtns[i] = btn;
        }
        alignLayout->addWidget(hLabel);
        alignLayout->addLayout(hLayout);

        // Vertical
        auto *vLabel = new QLabel(tr("Vertical"));
        auto *vLayout = new QHBoxLayout();
        m_alignVGroup = new QButtonGroup(this);
        m_alignVGroup->setExclusive(true);

        const struct
        {
            int value;
            const char *text;
        } vItems[] = {
            {1, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Top")},
            {2, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Center")},
            {3, QT_TRANSLATE_NOOP("ODetailsPieceWidget", "Bottom")},
        };
        for (int i = 0; i < 3; ++i)
        {
            auto *btn = new QPushButton(tr(vItems[i].text));
            btn->setCheckable(true);
            btn->setProperty("alignValue", vItems[i].value);
            m_alignVGroup->addButton(btn, vItems[i].value);
            vLayout->addWidget(btn);
            m_alignVBtns[i] = btn;
        }
        alignLayout->addWidget(vLabel);
        alignLayout->addLayout(vLayout);

        layout->addWidget(alignGroupBox);

        // Passmark multi-selection
        auto *passmarkRow = new QHBoxLayout();
        m_addPassmarksBtn = new QPushButton(tr("+ Add passmarks"));
        passmarkRow->addWidget(m_addPassmarksBtn);

        m_removePassmarksBtn = new QPushButton(tr("- Remove passmarks"));
        passmarkRow->addWidget(m_removePassmarksBtn);
        layout->addLayout(passmarkRow);

        // Manage main path
        m_manageMainPathBtn = new QPushButton(tr("Manage main path"));
        layout->addWidget(m_manageMainPathBtn);

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("General"));
    }

    // ── Grainline tab ────────────────────────────────────────────
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);

        m_grainlineWidget = new ODetailsGrainlineWidget(tab);
        layout->addWidget(m_grainlineWidget);

        // Separator
        auto *separator = new QFrame();
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        layout->addWidget(separator);

        m_foldlineWidget = new ODetailsFoldLineWidget(tab);
        layout->addWidget(m_foldlineWidget);

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("Grainline"));
    }

    // ── Seam Allowance tab ───────────────────────────────────────
    {
        auto *tab = new QFrame();
        tab->setFrameStyle(QFrame::NoFrame);
        tab->setObjectName(QStringLiteral("detailsOptionsTabPage"));
        auto *layout = new QVBoxLayout(tab);

        m_saWidget = new ODetailsSeamAllowanceWidget(tab);
        layout->addWidget(m_saWidget);

        layout->addStretch();

        m_tabWidget->addTab(tab, tr("Seam Allowance"));
    }

    mainLayout->addWidget(m_tabWidget);

    ConnectControls();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ConnectControls()
{
    // Sub-widgets commit through the same snapshot + undo pipeline as the General tab.
    m_grainlineWidget->SetModifyPending([this](std::function<void(VPiece &)> func) { ModifyPiece(func); });
    m_foldlineWidget->SetModifyPending([this](std::function<void(VPiece &)> func) { ModifyPiece(func); });
    connect(m_foldlineWidget, &ODetailsFoldLineWidget::ShowStatusTip, this, &ODetailsPieceWidget::ShowStatusTip);

    m_saWidget->SetModifyPending([this](std::function<void(VPiece &)> func) { ModifyPiece(func); });
    connect(m_saWidget, &ODetailsSeamAllowanceWidget::ShowStatusTip, this, &ODetailsPieceWidget::ShowStatusTip);
    connect(m_saWidget, &ODetailsSeamAllowanceWidget::HighlightNode, this, [this](quint32 nodeId, bool highlight)
    {
        emit HighlightNode(nodeId, highlight);
    });
    connect(m_saWidget, &ODetailsSeamAllowanceWidget::ClearHighlight, this, &ODetailsPieceWidget::ClearHighlight);

    // Re-populate the tabs after undo/redo. Deferred via singleShot to avoid re-entrancy
    // while CommitPending() is mid-flight (indexChanged fires synchronously during push).
    connect(VAbstractApplication::VApp()->getUndoStack(), &QUndoStack::indexChanged, this, [this](int)
    {
        QTimer::singleShot(0, this, [this]()
        {
            if (!m_currentTool)
            {
                return;
            }
            const VPiece detail = m_currentTool->getData()->GetPiece(m_currentTool->getId());
            m_pendingDet = detail;
            m_grainlineWidget->Show(detail, m_currentTool->getData());
            m_foldlineWidget->Show(detail);
            m_saWidget->RefreshTable(detail);
        });
    });

    // ── General tab connections ──────────────────────────────────

    connect(m_nameEdit, &QLineEdit::editingFinished, this, [this]()
    {
        if (m_updating || !m_currentTool)
        {
            return;
        }
        m_pendingDet.SetName(m_nameEdit->text());
        CommitPending();
    });

    connect(m_alignHGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        const int v = button->property("alignValue").toInt();
        ModifyPiece([v](VPiece &det) { det.SetGradedAlignH(v); });
    });

    connect(m_alignVGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        const int v = button->property("alignValue").toInt();
        ModifyPiece([v](VPiece &det) { det.SetGradedAlignV(v); });
    });

    // ── Passmark multi-selection ──────────────────────────────────

    connect(m_addPassmarksBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnAddPassmarks);
    connect(m_removePassmarksBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnRemovePassmarks);

    // ── Manage main path ──────────────────────────────────────────

    connect(m_manageMainPathBtn, &QPushButton::clicked, this, &ODetailsPieceWidget::OnManageMainPath);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::ModifyPiece(std::function<void(VPiece &)> func)
{
    if (m_updating || !m_currentTool)
    {
        return;
    }
    func(m_pendingDet);
    CommitPending();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::Show(VToolSeamAllowance *tool)
{
    if (!tool)
    {
        Clear();
        return;
    }

    const VPiece detail = tool->getData()->GetPiece(tool->getId());

    m_currentTool = tool;
    m_pieceId = tool->getId();
    m_pendingDet = detail;

    m_updating = true;

    m_nameEdit->setText(detail.GetName());
    m_nameEdit->setCursorPosition(0);

    // Number is a placeholder — its backend is OPieceCartouche (Phase C).
    m_pieceNumberSpin->setValue(0);

    int h = detail.GetGradedAlignH();
    if (h == 0)
    {
        h = 2;
    }
    if (auto *hBtn = m_alignHGroup->button(h))
    {
        hBtn->setChecked(true);
    }

    int v = detail.GetGradedAlignV();
    if (v == 0)
    {
        v = 2;
    }
    if (auto *vBtn = m_alignVGroup->button(v))
    {
        vBtn->setChecked(true);
    }

    // ── Grainline tab ────────────────────────────────────────────
    m_grainlineWidget->Show(detail, tool->getData());
    m_foldlineWidget->Show(detail);
    m_saWidget->Show(detail);

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::Clear()
{
    m_currentTool = nullptr;
    m_pieceId = 0;
    m_pendingDet = VPiece{};
    m_passmarkSelectState = PassmarkSelectState::Idle;
    m_selectedPassmarkNodeIds.clear();
    m_grainlineWidget->Clear();
    m_foldlineWidget->Clear();
    m_saWidget->Clear();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CommitPending()
{
    if (!m_currentTool || m_updating)
    {
        return;
    }

    m_updating = true;

    const auto *data = m_currentTool->getData();
    const VPiece oldDet = data->GetPiece(m_currentTool->getId());

    VAbstractApplication::VApp()->getUndoStack()->push(
        new SavePieceOptions(oldDet, m_pendingDet,
                             VAbstractValApplication::VApp()->getCurrentDocument(),
                             m_currentTool->getId()));

    // Resync pending state with the container after the undo.
    m_pendingDet = data->GetPiece(m_currentTool->getId());

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingPassmarkPoints() const -> bool
{
    return m_passmarkSelectState != PassmarkSelectState::Idle;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnAddPassmarks()
{
    if (!m_currentTool)
    {
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Adding)
    {
        CancelPassmarkSelection();
        return;
    }

    m_passmarkSelectState = PassmarkSelectState::Adding;
    m_selectedPassmarkNodeIds.clear();
    m_addPassmarksBtn->setText(tr("Cancel"));

    // Highlight existing passmarks in orange.
    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    const VPiecePath &path = piece.GetPath();
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.GetTypeTool() == Tool::NodePoint && node.IsPassmark() && not node.IsExcluded())
        {
            emit HighlightNode(node.GetId(), true, QColor(255, 165, 0)); // orange
        }
    }

    emit ShowStatusTip(tr("Click points to add passmarks — Enter to confirm, Esc to cancel"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnRemovePassmarks()
{
    if (!m_currentTool)
    {
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Removing)
    {
        CancelPassmarkSelection();
        return;
    }

    m_passmarkSelectState = PassmarkSelectState::Removing;
    m_selectedPassmarkNodeIds.clear();
    m_removePassmarksBtn->setText(tr("Cancel"));

    // Highlight existing passmarks in orange.
    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    const VPiecePath &path = piece.GetPath();
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.GetTypeTool() == Tool::NodePoint && node.IsPassmark() && not node.IsExcluded())
        {
            emit HighlightNode(node.GetId(), true, QColor(255, 165, 0)); // orange
        }
    }

    emit ShowStatusTip(tr("Click passmark points to remove — Enter to confirm, Esc to cancel"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelPassmarkSelection()
{
    m_passmarkSelectState = PassmarkSelectState::Idle;
    m_selectedPassmarkNodeIds.clear();
    m_addPassmarksBtn->setText(tr("+ Add passmarks"));
    m_removePassmarksBtn->setText(tr("- Remove passmarks"));
    emit ClearHighlight();
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingFoldLinePoints() const -> bool
{
    return m_foldlineWidget->IsSelectingPoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForFoldLine(quint32 nodeId)
{
    m_foldlineWidget->OnNodeSelected(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelFoldLineSelection()
{
    m_foldlineWidget->CancelSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPieceWidget::IsSelectingSAPoints() const -> bool
{
    return m_saWidget->IsSelectingPoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForSA(quint32 nodeId)
{
    m_saWidget->OnNodeSelected(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::CancelSASelection()
{
    m_saWidget->CancelSelection();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnNodeSelectedForPassmark(quint32 nodeId)
{
    if (m_passmarkSelectState == PassmarkSelectState::Idle || !m_currentTool)
    {
        return;
    }

    const VPiece piece = m_currentTool->getData()->GetPiece(m_currentTool->getId());
    const VPiecePath &path = piece.GetPath();
    const int idx = path.indexOfNode(nodeId);
    if (idx == -1)
    {
        return;
    }

    const VPieceNode &node = path.at(idx);
    if (node.GetTypeTool() != Tool::NodePoint || node.IsExcluded())
    {
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Removing && not node.IsPassmark())
    {
        return; // in Remove mode, only existing passmark points may be selected
    }

    if (m_selectedPassmarkNodeIds.contains(nodeId))
    {
        m_selectedPassmarkNodeIds.remove(nodeId);
        emit HighlightNode(nodeId, false);
    }
    else
    {
        m_selectedPassmarkNodeIds.insert(nodeId);
        emit HighlightNode(nodeId, true);
    }

    const auto count = m_selectedPassmarkNodeIds.size();
    emit ShowStatusTip(tr("%1 point(s) selected — Enter to confirm, Esc to cancel").arg(count));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::FinishPassmarkSelection()
{
    if (m_passmarkSelectState == PassmarkSelectState::Idle || !m_currentTool)
    {
        return;
    }

    if (m_selectedPassmarkNodeIds.isEmpty())
    {
        CancelPassmarkSelection();
        return;
    }

    if (m_passmarkSelectState == PassmarkSelectState::Adding)
    {
        // Open modal to choose passmark type.
        auto *dialog = new QDialog(this);
        dialog->setWindowTitle(tr("Choose Passmark Type"));
        auto *dlgLayout = new QVBoxLayout(dialog);
        auto *combo = new QComboBox();
        const struct
        {
            PassmarkLineType type;
            const char *text;
        } items[] = {
            {PassmarkLineType::OneLine,       "One line"},
            {PassmarkLineType::TwoLines,      "Two lines"},
            {PassmarkLineType::ThreeLines,    "Three lines"},
            {PassmarkLineType::TMark,         "T mark"},
            {PassmarkLineType::ExternalVMark, "External V mark"},
            {PassmarkLineType::InternalVMark, "Internal V mark"},
            {PassmarkLineType::UMark,         "U mark"},
            {PassmarkLineType::BoxMark,       "Box mark"},
            {PassmarkLineType::CheckMark,     "Check mark"},
        };
        for (const auto &item : items)
        {
            combo->addItem(tr(item.text), static_cast<int>(item.type));
        }
        dlgLayout->addWidget(combo);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
        dlgLayout->addWidget(buttons);

        if (dialog->exec() != QDialog::Accepted)
        {
            CancelPassmarkSelection();
            dialog->deleteLater();
            return;
        }
        const auto lineType = static_cast<PassmarkLineType>(combo->currentData().toInt());
        dialog->deleteLater();

        VPiecePath &pendingPath = m_pendingDet.GetPath();
        for (int i = 0; i < pendingPath.CountNodes(); ++i)
        {
            VPieceNode &node = pendingPath[i];
            if (m_selectedPassmarkNodeIds.contains(node.GetId()))
            {
                node.SetPassmark(true);
                node.SetPassmarkLineType(lineType);
            }
        }
    }
    else if (m_passmarkSelectState == PassmarkSelectState::Removing)
    {
        VPiecePath &pendingPath = m_pendingDet.GetPath();
        for (int i = 0; i < pendingPath.CountNodes(); ++i)
        {
            VPieceNode &node = pendingPath[i];
            if (m_selectedPassmarkNodeIds.contains(node.GetId()))
            {
                node.SetPassmark(false);
            }
        }
    }

    // Commit immediately — passmark operations are independent of Apply.
    {
        const auto *data = m_currentTool->getData();
        const VPiece oldDet = data->GetPiece(m_currentTool->getId());
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceOptions(oldDet, m_pendingDet,
                                 VAbstractValApplication::VApp()->getCurrentDocument(),
                                 m_currentTool->getId()));
        m_pendingDet = data->GetPiece(m_currentTool->getId());
    }

    CancelPassmarkSelection();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPieceWidget::OnManageMainPath()
{
    if (!m_currentTool)
    {
        return;
    }

    const auto *data = m_currentTool->getData();
    const VPiece piece = data->GetPiece(m_currentTool->getId());

    auto *dialog = new DialogMainPath(data, this);
    dialog->SetNodes(piece.GetPath().GetNodes());

    if (dialog->exec() == QDialog::Accepted)
    {
        const QVector<VPieceNode> nodes = dialog->GetNodes();
        m_pendingDet.GetPath().SetNodes(nodes);

        const VPiece oldDet = data->GetPiece(m_currentTool->getId());
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceOptions(oldDet, m_pendingDet,
                                 VAbstractValApplication::VApp()->getCurrentDocument(),
                                 m_currentTool->getId()));
        m_pendingDet = data->GetPiece(m_currentTool->getId());
    }

    dialog->deleteLater();
}
