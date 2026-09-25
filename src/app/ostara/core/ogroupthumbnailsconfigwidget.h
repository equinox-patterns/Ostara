/************************************************************************
 **
 **  @file   ogroupthumbnailsconfigwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26 September, 2026
 **
 **  @brief  Thumbnail sub-tab of the group page (Information tab): one
 **          miniature editor per member piece.
 **
 **          A group carries no miniature of its own — what a miniature
 **          draws is one piece, and a group is several. So this page does
 **          not edit the group: it shows the miniature of every member,
 **          in its own box, and each box commits to that member's piece.
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

#ifndef OGROUPTHUMBNAILSCONFIGWIDGET_H
#define OGROUPTHUMBNAILSCONFIGWIDGET_H

#include <QVector>
#include <QWidget>

#include <functional>

class QVBoxLayout;
class VContainer;
class OThumbnailConfigWidget;
struct OPieceGroup;

/**
 * @brief The group page's Thumbnail sub-tab: the miniature of each member piece.
 *
 * **The group owns no miniature.** Its blocks, annotations and grainline are one set for the whole
 * group, but a miniature draws a piece — so there is one per member, and it belongs to that member.
 * This page is therefore an editor of the members, not of the group: one box per member, titled with
 * the member's name, each one driving that member's own thumbnail data through the piece command.
 */
class OGroupThumbnailsConfigWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    /// Callback that applies a mutation to the group and commits it to the undo stack.
    using GroupCommitter = std::function<void(std::function<void(OPieceGroup &)>)>;

    explicit OGroupThumbnailsConfigWidget(QWidget *parent = nullptr);
    ~OGroupThumbnailsConfigWidget() override = default;

    void SetModifyGroup(GroupCommitter commit);

    /** @brief Shows one editor per member of @p groupId. The boxes are rebuilt only when the
     *  membership changes: a mere refresh must not destroy the widget the user is typing in. */
    void Show(quint32 groupId, const VContainer *data);
    void Clear();
    void RetranslateUI();

private:
    struct MemberBox
    {
        quint32 pieceId{0};
        QWidget *box{nullptr}; // The per-member QGroupBox, titled with the member's name.
        OThumbnailConfigWidget *editor{nullptr};
    };

    void BuildMemberBoxes(const QVector<quint32> &pieceIds);
    void ClearMemberBoxes();
    /// @brief The members the current boxes stand for — what a membership change is detected with.
    auto BoxMemberIds() const -> QVector<quint32>;
    static auto MemberName(quint32 pieceId, const VContainer *data) -> QString;

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OGroupThumbnailsConfigWidget) // NOLINT

    GroupCommitter m_modifyGroup;

    QVBoxLayout *m_boxesLayout{nullptr};
    QVector<MemberBox> m_boxes;
};

#endif // OGROUPTHUMBNAILSCONFIGWIDGET_H
