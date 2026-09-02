/************************************************************************
 **
 **  @file   otreewidget.h
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

#ifndef OTREEWIDGET_H
#define OTREEWIDGET_H

#include <QColor>
#include <QStyledItemDelegate>
#include <QTreeWidget>

class QEvent;
class QMouseEvent;

// In-layout state (check/x) stored on each tree item so the delegate can pick the icon variant
// to draw on a selected/hovered row (the light-glyph dark set).
constexpr int kTreeInLayoutRole = Qt::UserRole + 2;

/**
 * @brief The OTreeWidgetDelegate class — swaps the in-layout check/x icon to the dark set
 *        (light glyph) when the row is selected or hovered, so the icon stays visible on the
 *        accent background. Qt's item view only switches the QIcon mode for the Selected state,
 *        not for hover, so this is handled here.
 */
class OTreeWidgetDelegate : public QStyledItemDelegate
{
    Q_OBJECT // NOLINT

public:
    explicit OTreeWidgetDelegate(QObject *parent = nullptr);

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;
};

/**
 * @brief The OTreeWidget class — a QTreeWidget whose branch column draws the Breeze-style
 *        breadcrumb guides (vertical spine + ├ / └ corners) with QPainter instead of SVG
 *        images, and the expand/collapse carets from the Ostara icon library (open chevrons).
 *        Used by the Details dock pieces tree.
 */
class OTreeWidget : public QTreeWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OTreeWidget(QWidget *parent = nullptr);

protected:
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;
    auto edit(const QModelIndex &index, EditTrigger trigger, QEvent *event = nullptr) -> bool override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    auto GuideColor() const -> QColor;
    QModelIndex m_hoveredIndex;
};

#endif // OTREEWIDGET_H
