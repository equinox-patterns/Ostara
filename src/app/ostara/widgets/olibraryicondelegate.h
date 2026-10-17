/************************************************************************
 **
 **  @file   olibraryicondelegate.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The icons a library row wears, and the delegate that paints them centred.
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

#ifndef OLIBRARYICONDELEGATE_H
#define OLIBRARYICONDELEGATE_H

#include <QIcon>
#include <QString>
#include <QStyledItemDelegate>

/**
 * @brief The theme icon of a row, in its two variants: the light one on a plain row, the dark one on a selected
 * one.
 *
 * The path format lives here once: the piece library, the patterns library, the dialog that saves into it and the
 * history of a client all wear the same pair, and three copies of that format would be three places to look for
 * the day the theme moves.
 */
auto OLibraryIconPath(const QString &baseName, bool dark) -> QString;

/// @brief The same, as an icon carrying both variants.
auto OLibraryIcon(const QString &baseName) -> QIcon;

/**
 * @brief The icons of a library's action column, painted centred.
 *
 * An item's own icon would sit at the left of its cell and keep one variant for every state; here the icon is
 * centred, and the row's selection is what picks the variant.
 */
class OLibraryIconDelegate : public QStyledItemDelegate
{
public:
    explicit OLibraryIconDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

/**
 * @brief A row that carries text and an icon, painted with the icon's light variant on a dark row.
 *
 * The view picks the icon variant from the **selection** only; a hovered row is dark too, and the light-variant
 * icon would disappear into it. So the row is painted here, with the icon swapped for its light variant whenever
 * the row is chosen **or** under the mouse.
 */
class OLibraryRowDelegate : public QStyledItemDelegate
{
public:
    explicit OLibraryRowDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // OLIBRARYICONDELEGATE_H
