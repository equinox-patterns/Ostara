/************************************************************************
 **
 **  @file   ogroupmembershipmark.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 October, 2026
 **
 **  @brief  The mark a group member carries: a dashed frame and "Member of <group>".
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

#ifndef OGROUPMEMBERSHIPMARK_H
#define OGROUPMEMBERSHIPMARK_H

#include "vmisc/def.h"

#include <QGraphicsItem>
#include <QRectF>
#include <QString>

class QPainter;

/**
 * @brief "This piece is not the one in use — the group is."
 *
 * A member of a piece group is a real piece: its margins, its notches and its children are edited
 * on it, yet it is the group that counts, exports and prints. The mark says so — a grey dashed
 * frame around the piece and the name of the group it belongs to, above its top-left corner.
 *
 * The drawing lives here, once, because two very different carriers show it: the Details tool
 * paints it itself (VToolSeamAllowance::PaintGroupMembershipMark), the Graded canvas has no tool
 * to paint it — its pieces are rendered by VLayoutPiece — so it hangs a OGroupMembershipMarkItem
 * above the piece instead. One rule, two carriers.
 */
namespace OGroupMark
{
/// @brief The caption: the group's name, or a generic wording while it has none.
auto Caption(const QString &groupName) -> QString;

/// @brief The caption's pixel size, so a bounding rect and the drawing agree.
auto FontPixelSize(const Unit &unit) -> qreal;

/**
 * @brief Draws the mark for @p rect — in the caller's own coordinates.
 *
 * @param rect    the piece's bounding rect, in the frame the painter is set in
 * @param caption the text to draw above the top-left corner (see Caption)
 * @param unit    pattern unit, which sizes the caption
 */
void Paint(QPainter *painter, const QRectF &rect, const QString &caption, const Unit &unit);
} // namespace OGroupMark

/**
 * @brief The membership mark as a graphics item, for a canvas whose pieces are not tools.
 *
 * Paints nothing itself but the mark: the frame sits on the rect it was given, exactly where the
 * Details tool draws it, and it takes no mouse, no focus and no selection — the piece under it
 * keeps every gesture.
 */
class OGroupMembershipMarkItem : public QGraphicsItem
{
    Q_DISABLE_COPY_MOVE(OGroupMembershipMarkItem) // NOLINT

public:
    explicit OGroupMembershipMarkItem(QGraphicsItem *parent = nullptr);

    /// @brief The piece's rect (parent frame) and the caption to draw above it.
    void SetMark(const QRectF &rect, const QString &caption, const Unit &unit);

    auto boundingRect() const -> QRectF override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QRectF m_rect{};
    QString m_caption{};
    Unit m_unit{Unit::Cm};
};

#endif // OGROUPMEMBERSHIPMARK_H
