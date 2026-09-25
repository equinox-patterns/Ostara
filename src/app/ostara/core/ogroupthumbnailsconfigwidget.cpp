/************************************************************************
 **
 **  @file   ogroupthumbnailsconfigwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26 September, 2026
 **
 **  @brief  Thumbnail sub-tab of the group page (Information tab): one
 **          miniature editor per member piece.
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

#include "ogroupthumbnailsconfigwidget.h"

#include "othumbnailconfigwidget.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QGroupBox>
#include <QVBoxLayout>

#include <utility>

//---------------------------------------------------------------------------------------------------------------------
OGroupThumbnailsConfigWidget::OGroupThumbnailsConfigWidget(QWidget *parent) : QWidget(parent)
{
    m_boxesLayout = new QVBoxLayout(this);
    // Same content style as the other sub-tabs: side margins on the tab content so the sections are
    // not glued to the tab edges.
    m_boxesLayout->setContentsMargins(8, 8, 8, 0);
    m_boxesLayout->setSpacing(16);
    m_boxesLayout->addStretch(1);
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupThumbnailsConfigWidget::SetModifyGroup(GroupCommitter commit)
{
    m_modifyGroup = std::move(commit);
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupThumbnailsConfigWidget::Show(quint32 groupId, const VContainer *data)
{
    if (data == nullptr)
    {
        Clear();
        return;
    }

    const auto *groups = data->DataPieceGroups();
    if (groups == nullptr || !groups->contains(groupId))
    {
        Clear();
        return;
    }

    const QVector<quint32> pieceIds = groups->value(groupId).pieceIds;
    if (pieceIds != BoxMemberIds())
    {
        BuildMemberBoxes(pieceIds);
    }

    // The state is re-read on every show — an undo, a name change or a size change must land here —
    // while the boxes themselves survive it.
    const OPieceGroup group = groups->value(groupId);
    for (const MemberBox &member : std::as_const(m_boxes))
    {
        const QString name = MemberName(member.pieceId, data);
        if (auto *box = qobject_cast<QGroupBox *>(member.box); box != nullptr)
        {
            box->setTitle(name);
        }
        // The group's own miniature of that member — never the piece's, which stays masked while the
        // piece is linked and is found untouched after an unlink.
        member.editor->Show(group.MemberThumbnail(member.pieceId), data, member.pieceId, name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupThumbnailsConfigWidget::Clear()
{
    ClearMemberBoxes();
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupThumbnailsConfigWidget::RetranslateUI()
{
    for (const MemberBox &member : std::as_const(m_boxes))
    {
        member.editor->RetranslateUI();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupThumbnailsConfigWidget::BuildMemberBoxes(const QVector<quint32> &pieceIds)
{
    ClearMemberBoxes();

    for (quint32 pieceId : pieceIds)
    {
        // One box per member: the member's name is the subject, and the editor inside it names that
        // same member in its own labels — there is no group-level miniature to speak of.
        auto *box = new QGroupBox(this);
        auto *boxLayout = new QVBoxLayout(box);
        boxLayout->setContentsMargins(8, 8, 8, 8);
        boxLayout->setSpacing(6);

        auto *editor = new OThumbnailConfigWidget(box);
        editor->SetModifyPending([this, pieceId](std::function<void(OThumbnailData &)> func)
        {
            if (m_modifyGroup == nullptr)
            {
                return; // nothing can be committed: the controls stay inert, as on the piece page
            }
            m_modifyGroup([pieceId, &func](OPieceGroup &group)
            {
                OThumbnailData thumbnail = group.MemberThumbnail(pieceId);
                func(thumbnail);
                group.SetMemberThumbnail(pieceId, thumbnail);
            });
        });
        editor->RetranslateUI();
        boxLayout->addWidget(editor);

        // Inserted before the trailing stretch, so the boxes stay packed at the top.
        m_boxesLayout->insertWidget(m_boxesLayout->count() - 1, box);

        MemberBox member;
        member.pieceId = pieceId;
        member.box = box;
        member.editor = editor;
        m_boxes.append(member);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupThumbnailsConfigWidget::ClearMemberBoxes()
{
    for (const MemberBox &member : std::as_const(m_boxes))
    {
        delete member.box; // takes the editor with it
    }
    m_boxes.clear();
}

//---------------------------------------------------------------------------------------------------------------------
auto OGroupThumbnailsConfigWidget::BoxMemberIds() const -> QVector<quint32>
{
    QVector<quint32> ids;
    ids.reserve(m_boxes.size());
    for (const MemberBox &member : m_boxes)
    {
        ids.append(member.pieceId);
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGroupThumbnailsConfigWidget::MemberName(quint32 pieceId, const VContainer *data) -> QString
{
    // The same fallback the rest of the group page uses: a piece with no name still has to be
    // designatable in the box that edits it. The container is checked first — GetPiece() *throws* on
    // an id it does not hold (Bugs.md, 27-08), and a group can name a piece that is already gone.
    const auto *pieces = data->DataPieces();
    if (pieces == nullptr || not pieces->contains(pieceId))
    {
        return tr("Unnamed");
    }
    const VPiece piece = data->GetPiece(pieceId);
    return piece.GetName().isEmpty() ? tr("Unnamed") : piece.GetName();
}
