/************************************************************************
 **
 **  @file   odetailsfoldlinewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Foldline editor widget for the Details mode options dock
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

#include "odetailsfoldlinewidget.h"

#include "../vmisc/def.h"
#include "../vmisc/typedef.h"
#include "../vpatterndb/vpiecenode.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
const struct
{
    FoldLineType type;
    const char *text;
} kFoldLineTypeItems[] = {
    {FoldLineType::TwoArrows,          QT_TRANSLATE_NOOP("ODetailsFoldLineWidget", "No text")},
    {FoldLineType::TwoArrowsTextAbove, QT_TRANSLATE_NOOP("ODetailsFoldLineWidget", "Text above")},
    {FoldLineType::TwoArrowsTextUnder, QT_TRANSLATE_NOOP("ODetailsFoldLineWidget", "Text below")},
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsFoldLineWidget::ODetailsFoldLineWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *foldGroup = new QGroupBox(tr("Foldline"));
    auto *foldLayout = new QVBoxLayout(foldGroup);

    m_defineBtn = new QPushButton(tr("Define foldline"));
    foldLayout->addWidget(m_defineBtn);

    m_deactivateBtn = new QPushButton(tr("Deactivate foldline"));
    m_deactivateBtn->setVisible(false);
    foldLayout->addWidget(m_deactivateBtn);

    m_statusLabel = new QLabel();
    foldLayout->addWidget(m_statusLabel);

    m_showFullPieceCheck = new QCheckBox(tr("Show unfolded piece"));
    foldLayout->addWidget(m_showFullPieceCheck);

    m_showMirrorLineCheck = new QCheckBox(tr("Show symmetry line"));
    foldLayout->addWidget(m_showMirrorLineCheck);

    // Type buttons
    auto *typeLayout = new QHBoxLayout();
    m_typeGroup = new QButtonGroup(this);
    m_typeGroup->setExclusive(true);

    for (const auto &item : kFoldLineTypeItems)
    {
        auto *btn = new QPushButton(tr(item.text));
        btn->setCheckable(true);
        btn->setProperty("foldLineType", static_cast<int>(item.type));
        m_typeGroup->addButton(btn, static_cast<int>(item.type));
        typeLayout->addWidget(btn);
    }
    foldLayout->addLayout(typeLayout);

    // Text
    m_textEdit = new QLineEdit();
    m_textEdit->setPlaceholderText(tr("Cut on fold"));
    foldLayout->addWidget(m_textEdit);

    mainLayout->addWidget(foldGroup);

    ConnectControls();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::ConnectControls()
{
    connect(m_defineBtn, &QPushButton::clicked, this, [this]()
    {
        switch (m_selectState)
        {
            case FoldLineSelectState::Idle:
                m_selectState = FoldLineSelectState::WaitingFirstPoint;
                m_firstPoint = 0;
                m_defineBtn->setText(tr("Cancel"));
                emit ShowStatusTip(tr("Select the first point of the foldline — respect clockwise direction"));
                break;
            case FoldLineSelectState::WaitingFirstPoint:
                m_selectState = FoldLineSelectState::Idle;
                m_firstPoint = 0;
                m_defineBtn->setText(tr("Define foldline"));
                emit ShowStatusTip(QString());
                break;
            case FoldLineSelectState::WaitingSecondPoint:
                m_selectState = FoldLineSelectState::WaitingFirstPoint;
                m_firstPoint = 0;
                emit ShowStatusTip(tr("Select the first point of the foldline — respect clockwise direction"));
                break;
        }
    });

    connect(m_deactivateBtn, &QPushButton::clicked, this, [this]()
    {
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([](VPiece &det)
        {
            det.SetMirrorLineStartPoint(NULL_ID);
            det.SetMirrorLineEndPoint(NULL_ID);
            det.SetShowMirrorLine(false);
            det.SetShowFullPiece(false);
        });

        const bool wasUpdating = m_updating;
        m_updating = true;
        m_deactivateBtn->setVisible(false);
        m_showFullPieceCheck->setChecked(false);
        m_showMirrorLineCheck->setChecked(false);
        m_showMirrorLineCheck->setEnabled(false);
        m_updating = wasUpdating;
        UpdateFoldLineStatus(false);
    });

    connect(m_showFullPieceCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        const bool wasUpdating = m_updating;
        m_updating = true;
        m_showMirrorLineCheck->setEnabled(checked);
        m_showMirrorLineCheck->setChecked(checked);
        m_updating = wasUpdating;

        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([checked](VPiece &det)
        {
            det.SetShowFullPiece(checked);
            if (checked)
            {
                det.SetShowMirrorLine(true);
            }
        });
    });

    connect(m_showMirrorLineCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating)
        {
            return;
        }
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([checked](VPiece &det) { det.SetShowMirrorLine(checked); });
    });

    connect(m_typeGroup, QOverload<QAbstractButton *, bool>::of(&QButtonGroup::buttonToggled),
            this, [this](QAbstractButton *button, bool checked)
    {
        if (!checked || m_updating)
        {
            return;
        }
        const auto lineType = static_cast<FoldLineType>(button->property("foldLineType").toInt());
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([lineType](VPiece &det) { det.SetFoldLineType(lineType); });
    });

    connect(m_textEdit, &QLineEdit::editingFinished, this, [this]()
    {
        if (m_updating)
        {
            return;
        }
        if (!m_modifyPending)
        {
            return;
        }
        const QString label = m_textEdit->text();
        m_modifyPending([label](VPiece &det) { det.SetFoldLineLabel(label); });
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::Show(const VPiece &det)
{
    m_path = det.GetPath();

    const bool hasMirrorPoints = det.GetMirrorLineStartPoint() != NULL_ID
                              && det.GetMirrorLineEndPoint() != NULL_ID;
    const bool foldValid = hasMirrorPoints
                        && ValidateMirrorLinePoints(det.GetMirrorLineStartPoint(), det.GetMirrorLineEndPoint());

    m_updating = true;

    UpdateFoldLineStatus(foldValid);
    m_deactivateBtn->setVisible(foldValid);

    m_showFullPieceCheck->setChecked(det.IsShowFullPiece());
    m_showMirrorLineCheck->setChecked(det.IsShowMirrorLine());
    m_showMirrorLineCheck->setEnabled(det.IsShowFullPiece());

    if (auto *btn = m_typeGroup->button(static_cast<int>(det.GetFoldLineType())))
    {
        btn->setChecked(true);
    }

    m_textEdit->setText(det.GetFoldLineLabel());

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::Clear()
{
    m_path = VPiecePath{};
    m_selectState = FoldLineSelectState::Idle;
    m_firstPoint = 0;
    m_defineBtn->setText(tr("Define foldline"));
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify)
{
    m_modifyPending = std::move(modify);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsFoldLineWidget::IsSelectingPoints() const -> bool
{
    return m_selectState != FoldLineSelectState::Idle;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::OnNodeSelected(quint32 nodeId)
{
    if (m_selectState == FoldLineSelectState::Idle)
    {
        return;
    }

    // Validate the node belongs to the current piece.
    if (m_path.indexOfNode(nodeId) == -1)
    {
        m_selectState = FoldLineSelectState::Idle;
        m_firstPoint = 0;
        m_defineBtn->setText(tr("Define foldline"));
        m_statusLabel->setText(QStringLiteral("<span style='color:#D2691E;'>⚠ %1</span>")
                                   .arg(tr("Error — wrong piece. Start over.")));
        emit ShowStatusTip(QString());
        return;
    }

    if (m_selectState == FoldLineSelectState::WaitingFirstPoint)
    {
        m_firstPoint = nodeId;
        m_selectState = FoldLineSelectState::WaitingSecondPoint;
        emit ShowStatusTip(tr("Select the second point of the foldline — respect clockwise direction"));
        return;
    }

    // WaitingSecondPoint
    const quint32 startPoint = m_firstPoint;
    const quint32 endPoint = nodeId;

    if (!ValidateMirrorLinePoints(startPoint, endPoint))
    {
        m_selectState = FoldLineSelectState::Idle;
        m_firstPoint = 0;
        m_defineBtn->setText(tr("Define foldline"));
        m_statusLabel->setText(QStringLiteral("<span style='color:#D2691E;'>⚠ %1</span>")
                                   .arg(tr("Error — invalid points. Start over.")));
        emit ShowStatusTip(QString());
        return;
    }

    m_selectState = FoldLineSelectState::Idle;
    m_firstPoint = 0;
    m_defineBtn->setText(tr("Define foldline"));
    emit ShowStatusTip(QString());

    if (!m_modifyPending)
    {
        return;
    }
    m_modifyPending([startPoint, endPoint](VPiece &det)
    {
        det.SetMirrorLineStartPoint(startPoint);
        det.SetMirrorLineEndPoint(endPoint);
        det.SetShowMirrorLine(true);
        det.SetShowFullPiece(false);
        det.SetFoldLineType(FoldLineType::TwoArrowsTextAbove);
    });

    const bool wasUpdating = m_updating;
    m_updating = true;
    UpdateFoldLineStatus(true);
    m_deactivateBtn->setVisible(true);
    m_showFullPieceCheck->setChecked(false);
    m_showMirrorLineCheck->setChecked(true);
    m_showMirrorLineCheck->setEnabled(false);
    m_updating = wasUpdating;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::CancelSelection()
{
    m_selectState = FoldLineSelectState::Idle;
    m_firstPoint = 0;
    m_defineBtn->setText(tr("Define foldline"));
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineWidget::UpdateFoldLineStatus(bool defined)
{
    if (defined)
    {
        m_statusLabel->setText(QStringLiteral("<span style='color:#228B22;'>✓ %1</span>").arg(tr("Foldline defined")));
    }
    else
    {
        m_statusLabel->setText(QStringLiteral("<span style='color:#DC143C;'>✗ %1</span>").arg(tr("Foldline not defined")));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsFoldLineWidget::ValidateMirrorLinePoints(quint32 startPoint, quint32 endPoint) const -> bool
{
    if (startPoint == NULL_ID || endPoint == NULL_ID || startPoint == endPoint)
    {
        return false;
    }

    const int startIdx = m_path.indexOfNode(startPoint);
    const int endIdx = m_path.indexOfNode(endPoint);
    if (startIdx == -1 || endIdx == -1)
    {
        return false;
    }

    if (m_path.at(startIdx).IsExcluded() || m_path.at(endIdx).IsExcluded())
    {
        return false;
    }

    // Walk clockwise from startIdx (wrapping around the closed loop) to check
    // that endPoint is reachable without crossing excluded nodes.
    const auto nodeCount = m_path.CountNodes();
    for (int i = 1; i < nodeCount; ++i)
    {
        const auto idx = (startIdx + i) % nodeCount;
        const VPieceNode &node = m_path.at(idx);
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
