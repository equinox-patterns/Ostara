/************************************************************************
 **
 **  @file   mainwindow_graded_helpers.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: item roles and predicates shared by the Graded-mode TUs.
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

#ifndef MAINWINDOW_GRADED_HELPERS_H
#define MAINWINDOW_GRADED_HELPERS_H

#include <QGraphicsItem>
#include <QVariant>
#include <QtGlobal>
#include <QtNumeric>

class QColor;
class QGraphicsScene;
class VContainer;

/// Item roles tagging every graded item so alignment / offsets / drag can recover the owning
/// piece id, whether the item is the reference (layer 0) of its piece, and the position base
/// (mx/my) the item was created at — used to make re-applies idempotent. Shared across the
/// Graded-mode translation units (mainwindow_graded_*.cpp). The per-child-type markers
/// (interactive grainline / info-block / annotation) stay file-local to their own TU.
constexpr int kGradedPieceIdRole     = Qt::UserRole + 900;
constexpr int kGradedIsRefRole       = Qt::UserRole + 901;
constexpr int kGradedBasePosRole     = Qt::UserRole + 902;
constexpr int kGradedHostGroupIdRole = Qt::UserRole + 903; // invisible group host (holds group children)
/// Size axis label ("38", "40", ...) of the layer a silhouette root belongs to — set at capture so a
/// later re-apply pass can resolve "size → style" without knowing the active range (Graded Size Styles).
constexpr int kGradedSizeRole        = Qt::UserRole + 907;
/// Native (pre-style) QPen of a trace item the first time a size style overrides its dash — lets a
/// later "no style / gate off" pass restore the original pen (mirror line dash, internal paths...).
constexpr int kGradedBasePenRole     = Qt::UserRole + 908;
/// Local X (pattern frame) of the unit's forced alignment point for the size this item renders.
/// Captured while the container still holds that size's measurements, because the alignment pass
/// runs after the live combination has been restored. 0 is a legal X, so "carries a point" must be
/// tested with QVariant::isValid(), never isNull().
constexpr int kGradedAlignPointXRole = Qt::UserRole + 909;
/// Piece id carried by the "Member of <group>" mark hung above a grouped reference member.
constexpr int kGradedMemberMarkRole  = Qt::UserRole + 911;
/// Marks the invisible deck a reference piece hangs its own children on (see GradedDecorHost).
constexpr int kGradedDecorHostRole   = Qt::UserRole + 912;
/**
 * Piece id carried by a MEMBER VIEW — the editable copy a grouped member stands on in Graded.
 *
 * A group's stack is its own geometry: it is what the group draws and what it prints, and it must
 * not move when a member is pulled aside to be worked on. So a grouped piece has two carriers on the
 * reference layer: its stack item, which keeps the piece id and everything that goes with it
 * (alignment, offsets, export), and this view — a second render of the same piece, movable, which
 * carries the member's children and its "Member of" mark, exactly like the member standing in the
 * Details workshop. The view is positioned from the stack (stack position + the range's own offset
 * for that member), so it never takes part in alignment: only where the member is being edited.
 */
constexpr int kGradedMemberViewRole  = Qt::UserRole + 913;
/// Position Details gives a member (its mx/my when the canvas was captured): the place its own
/// graded stack stands, beside the group — carried by every member view.
constexpr int kGradedMemberBaseRole  = Qt::UserRole + 914;

/**
 * Deck height of a reference piece's own children, relative to the piece item.
 *
 * Two decks overlap on the reference layer: the children a group carries for its whole stack (on
 * the group host, placed one step above the reference pieces) and the children a piece carries for
 * itself. Both must be reachable — the member's, because a size range re-places them, the group's,
 * because they are what it prints. A piece's deck therefore sits one step above the group's host,
 * so the two stacks interleave by one: at equal child z, a piece's child wins; where the group's
 * child is alone, it takes the click. Without a deck, a child would inherit its class z (1..10001)
 * and lose to the host the moment that z was reset by OInfoBlockItem / OAnnotationItem activation.
 */
constexpr qreal kGradedDecorDeckZ = 2.0;


/// Piece id carried by an item (or 0 if the item isn't a graded piece item).
inline auto GradedPieceId(const QGraphicsItem *item) -> quint32
{
    if (item == nullptr)
    {
        return 0;
    }
    const QVariant id = item->data(kGradedPieceIdRole); // data() is virtual — call it once
    return id.isNull() ? 0 : id.toUInt();
}

/// True when the item is the reference (topmost, first size) of its graded piece.
inline auto GradedIsReference(const QGraphicsItem *item) -> bool
{
    return item != nullptr && item->data(kGradedIsRefRole).toBool();
}

/// Piece id carried by a member view (0 when the item is not one).
inline auto GradedMemberViewPiece(const QGraphicsItem *item) -> quint32
{
    if (item == nullptr)
    {
        return 0;
    }
    const QVariant id = item->data(kGradedMemberViewRole);
    return id.isNull() ? 0 : id.toUInt();
}

/// Piece id of the deck carrying a reference piece's children — the piece item itself, the member
/// view it is edited on, or the invisible deck those hang their children on (kGradedDecorHostRole).
inline auto GradedDecorOwnerPiece(const QGraphicsItem *item) -> quint32
{
    if (item == nullptr)
    {
        return 0;
    }
    if (!item->data(kGradedDecorHostRole).isNull())
    {
        return GradedDecorOwnerPiece(item->parentItem());
    }
    const quint32 viewPiece = GradedMemberViewPiece(item);
    return viewPiece != 0 ? viewPiece : GradedPieceId(item);
}

/// Local X of the forced alignment point carried by an item, or NaN when it carries none.
inline auto GradedAlignPointX(const QGraphicsItem *item) -> qreal
{
    if (item == nullptr)
    {
        return qQNaN();
    }
    const QVariant x = item->data(kGradedAlignPointXRole);
    return x.isValid() ? x.toDouble() : qQNaN();
}

/// Rebuild the per-group reference decoration (defined in mainwindow_graded_group.cpp): mask what
/// must not be shown (ghost layers), hang each grouped member's "Member of <group>" mark, and
/// (re)anchor each group's invisible host onto the union of its graded reference members.
/// Positions must be final when called.
void RefreshGradedReferenceGroupDecor(VContainer *pattern, QGraphicsScene *scene);

/**
 * @brief The reference-layer member view of @p pieceId, or nullptr when the piece has none.
 *
 * A member has one view per size — its own graded stack, standing where Details puts the member.
 * The reference one is the only one that is edited, and it is the one carrying the member's
 * children and its mark.
 */
auto GradedMemberView(QGraphicsScene *scene, quint32 pieceId) -> QGraphicsItem *;

/**
 * @brief Where a piece's children must be hung: its member view when it has one, else @p fallback
 * (its own reference item — a lone piece is edited where it stands).
 */
inline auto GradedPieceDecorOwner(QGraphicsScene *scene, quint32 pieceId, QGraphicsItem *fallback)
    -> QGraphicsItem *
{
    QGraphicsItem *view = GradedMemberView(scene, pieceId);
    return view != nullptr ? view : fallback;
}

/// The deck a reference piece hangs its own children on (defined in mainwindow_graded_group.cpp):
/// created on first use, in the piece's own frame, above the group's shared decor, painting
/// nothing and taking no mouse. @p pieceItem must be a reference piece item.
auto GradedDecorHost(QGraphicsItem *pieceItem) -> QGraphicsItem *;

/// Re-color the item tree produced by VLayoutPiece::GetItem (piece silhouette roots + their static
/// layout children) with the theme's piece color (VSceneStylesheet PieceColor) — the same color the
/// Details mode paints every piece trace with. Interactive children (info blocks, annotations,
/// grainline) are skipped: they manage their own colors. Defined in mainwindow_graded_build.cpp.
void RecolorGradedItemPens(QGraphicsItem *root);

#endif // MAINWINDOW_GRADED_HELPERS_H
