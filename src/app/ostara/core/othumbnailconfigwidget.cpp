/************************************************************************
 **
 **  @file   othumbnailconfigwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Thumbnail sub-tab of the Information dock tab (see
 **          othumbnailconfigwidget.h).
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

#include "othumbnailconfigwidget.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OThumbnailConfigWidget::OThumbnailConfigWidget(QWidget *parent) : QWidget(parent)
{
    // Same content style as the other sub-tabs: side margins on the tab content so
    // the section is not glued to the tab edges.
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 0);
    layout->setSpacing(16);

    m_activeCheck = new QCheckBox(tr("Activate thumbnail"), this);
    layout->addWidget(m_activeCheck);

    // No inner scroll area and no intermediate raw QWidget: the group box brings the
    // $DockBody background itself (SubtiliteCode §13/§14).
    m_infoGroup = new QGroupBox(tr("Piece thumbnail"), this);
    // Less side margin than the theme default (8px) for this group only.
    m_infoGroup->setStyleSheet(QStringLiteral("QGroupBox { margin-left: 0px; margin-right: 0px; }"));
    auto *groupLayout = new QVBoxLayout(m_infoGroup);
    groupLayout->setSpacing(6);
    m_hintLabel = new QLabel(m_infoGroup);
    m_hintLabel->setWordWrap(true);
    groupLayout->addWidget(m_hintLabel);
    groupLayout->addStretch();
    layout->addWidget(m_infoGroup, 1);

    connect(m_activeCheck, &QCheckBox::toggled, this, [this](bool) {
        if (m_loading)
        {
            return;
        }
        Commit();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::SetModifyPending(std::function<void(std::function<void(IPieceInfo &)>)> callback)
{
    m_modifyPending = std::move(callback);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::Show(const IPieceInfo &detail, const VContainer *data)
{
    Q_UNUSED(data)

    m_loading = true;
    m_activeCheck->setChecked(detail.GetThumbnailData().IsActive());
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::Clear()
{
    m_loading = true;
    m_activeCheck->setChecked(false);
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::RetranslateUI()
{
    m_activeCheck->setText(tr("Activate thumbnail"));
    m_infoGroup->setTitle(tr("Piece thumbnail"));
    m_hintLabel->setText(tr("The thumbnail draws the piece with its seam allowances, its notches and its internal "
                            "paths, at the reference size of each axis — so it keeps showing the same size when the "
                            "active size changes. Move and resize it directly on the canvas."));
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailConfigWidget::BuildData(const IPieceInfo &det) const -> OThumbnailData
{
    // Never start from a fresh object: the stored geometry must survive an activation change.
    OThumbnailData data = det.GetThumbnailData();
    data.SetActive(m_activeCheck->isChecked());
    data.SetSet(true);
    return data;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::Commit()
{
    if (m_loading || m_modifyPending == nullptr)
    {
        return;
    }
    m_modifyPending([this](IPieceInfo &det) { det.SetThumbnailData(BuildData(det)); });
}
