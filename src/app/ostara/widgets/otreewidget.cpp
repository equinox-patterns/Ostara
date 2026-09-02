/************************************************************************
 **
 **  @file   otreewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   02/09/2026
 **
 **  @brief  QTreeWidget with hand-painted breadcrumb branch guides.
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

#include "otreewidget.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QEvent>
#include <QIcon>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QStyleOptionViewItem>
#include <QVector>

//---------------------------------------------------------------------------------------------------------------------
OTreeWidget::OTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    // The in-layout check/x icons must switch to the light-glyph dark set on selected/hovered
    // rows (Qt only switches the icon mode for the Selected state, not for hover).
    setItemDelegate(new OTreeWidgetDelegate(this));
}

//---------------------------------------------------------------------------------------------------------------------
OTreeWidgetDelegate::OTreeWidgetDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

//---------------------------------------------------------------------------------------------------------------------
void OTreeWidgetDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    if (index.column() != 0 || option->icon.isNull())
    {
        return;
    }
    // On a selected/hovered row the background is the accent: swap the check/x icon to the dark
    // set (light glyph #e5e5e5) so it stays visible.
    if (option->state & (QStyle::State_Selected | QStyle::State_MouseOver))
    {
        const QString name = index.data(kTreeInLayoutRole).toBool() ? QStringLiteral("check") : QStringLiteral("x");
        option->icon = QIcon(QStringLiteral(":/icons/dark/%1_dark.svg").arg(name));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OTreeWidget::mouseMoveEvent(QMouseEvent *event)
{
    // Normalize to the branch column: indexAt() returns the column under the cursor (often the
    // name column), while drawBranches() compares against the column-0 index.
    const QModelIndex hovered = indexAt(event->pos()).siblingAtColumn(0);
    if (hovered != m_hoveredIndex)
    {
        m_hoveredIndex = hovered;
        viewport()->update(); // repaint so the branch guides switch back/forth on hover
    }
    QTreeWidget::mouseMoveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OTreeWidget::leaveEvent(QEvent *event)
{
    if (m_hoveredIndex.isValid())
    {
        m_hoveredIndex = QModelIndex();
        viewport()->update();
    }
    QTreeWidget::leaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto OTreeWidget::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) -> bool
{
    // Only the last (name) column is editable — the in-layout column holds just the check / x
    // icon and must never open a text editor (a double-click there would otherwise show an
    // empty input with a blinking cursor).
    if (index.column() < model()->columnCount() - 1)
    {
        return false;
    }
    return QTreeWidget::edit(index, trigger, event);
}

//---------------------------------------------------------------------------------------------------------------------
void OTreeWidget::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    const int indent = indentation();
    if (indent <= 0)
    {
        QTreeWidget::drawBranches(painter, rect, index);
        return;
    }

    // Breadcrumb branch guides (Breeze look, painted — no SVG): a vertical "spine" per level
    // with ├ / └ corners at leaf rows, and the library carets (open chevrons) on expandable
    // rows. The geometry mirrors QTreeView::drawBranches: the innermost cell is the rightmost
    // `indent`-wide cell of the branch column, each outer level one cell to the left.
    QVector<QModelIndex> ancestors;
    for (QModelIndex parent = index.parent(); parent.isValid(); parent = parent.parent())
    {
        ancestors.prepend(parent);
    }
    const int level = static_cast<int>(ancestors.size()); // 0 = top-level item

    const int rowTop = rect.top();
    const int rowBottom = rect.bottom() + 1;
    const int midY = rowTop + (rowBottom - rowTop) / 2;
    const int innerX = rect.right() + 1 - indent; // x of the innermost (own) cell

    // The caret matches the item icon size (the check / x column), capped by the cell width.
    const int caretSize = qBound(14, iconSize().width(), indent);
    const int caretHalf = caretSize / 2;

    // On a selected or hovered row the background is the accent: the guides and the carets must
    // switch to the light-glyph dark set (#e5e5e5) to stay visible.
    const bool onAccent = selectionModel()->isSelected(index) || index == m_hoveredIndex;
    const bool dark = VAbstractApplication::VApp()->Settings()->GetThemeMode() == VThemeMode::Dark;
    const QString caretTheme = (dark || onAccent) ? QStringLiteral("dark") : QStringLiteral("light");

    QPen pen(onAccent ? QColor(0xe5, 0xe5, 0xe5) : GuideColor(), 1.0);
    painter->save();
    painter->setPen(pen);

    for (int k = 0; k <= level; ++k)
    {
        const int cellX = innerX - k * indent;
        const int cx = cellX + indent / 2;

        // Does the item at this level have a following sibling (the spine continues below)?
        bool moreSiblings = false;
        if (k == 0)
        {
            moreSiblings = index.siblingAtRow(index.row() + 1).isValid();
        }
        else
        {
            const QModelIndex ancestor = ancestors.at(level - k);
            moreSiblings = ancestor.siblingAtRow(ancestor.row() + 1).isValid();
        }

        if (k == 0)
        {
            if (model()->hasChildren(index))
            {
                // Expandable node — the spine is CUT around the caret so the open chevron never
                // has a line running through it; the line continues above and below.
                if (moreSiblings)
                {
                    painter->drawLine(cx, rowTop, cx, midY - caretHalf);
                    painter->drawLine(cx, midY + caretHalf, cx, rowBottom);
                }
                // Caret — the library icon (open chevron `>` / `v`), centered in the cell,
                // at the same size as the check / x icons. Never a filled triangle.
                const QString name = isExpanded(index) ? QStringLiteral("caret-down") : QStringLiteral("caret-right");
                const QIcon caret(QStringLiteral(":/icons/%1/%2_%1.svg").arg(caretTheme, name));
                const QRect caretRect(cellX + (indent - caretSize) / 2, rowTop + (rowBottom - rowTop - caretSize) / 2,
                                      caretSize, caretSize);
                caret.paint(painter, caretRect, Qt::AlignCenter, QIcon::Normal);
            }
            else
            {
                // Leaf corner — the "L": the spine plus the horizontal arm.
                // ├ (a following sibling exists): full spine; └ (last child): spine up to the
                // arm only, mirroring the Breeze branch_more / branch_end shapes.
                if (moreSiblings)
                {
                    painter->drawLine(cx, rowTop, cx, rowBottom);
                }
                else
                {
                    painter->drawLine(cx, rowTop, cx, midY);
                }
                painter->drawLine(cx, midY, cellX + indent, midY);
            }
        }
        else if (moreSiblings)
        {
            // Ancestor level — plain vertical spine (│).
            painter->drawLine(cx, rowTop, cx, rowBottom);
        }
    }

    painter->restore();
}

//---------------------------------------------------------------------------------------------------------------------
auto OTreeWidget::GuideColor() const -> QColor
{
    // Theme-aware, subtle: the text color at low alpha reads as a light guide line in both themes.
    const QColor text = palette().color(QPalette::Text);
    return QColor(text.red(), text.green(), text.blue(), 80);
}
