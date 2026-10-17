/************************************************************************
 **
 **  @file   olibraryicondelegate.cpp
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

#include "olibraryicondelegate.h"

#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>

namespace
{
const int kIconSize = 20;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryIconPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg").arg(dark ? QStringLiteral("dark") : QStringLiteral("light"),
                                                     baseName);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryIcon(const QString &baseName) -> QIcon
{
    QIcon icon;
    icon.addFile(OLibraryIconPath(baseName, false), QSize(), QIcon::Normal);
    icon.addFile(OLibraryIconPath(baseName, true), QSize(), QIcon::Selected);
    return icon;
}

//---------------------------------------------------------------------------------------------------------------------
OLibraryIconDelegate::OLibraryIconDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void OLibraryIconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    const QVariant decoration = index.data(Qt::DecorationRole);
    if (not decoration.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // The row's background belongs to the view: it is drawn as it would be, without text or icon, and the action's
    // icon is then painted on top of it.
    QStyleOptionViewItem background = option;
    initStyleOption(&background, index);
    background.text.clear();
    background.icon = QIcon();

    const QWidget *widget = background.widget;
    QStyle *style = widget != nullptr ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &background, painter, widget);

    const QIcon icon = decoration.value<QIcon>();
    // Une ligne survolée est aussi foncée qu'une ligne choisie : la variante claire vaut pour les deux.
    const bool dark = (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) != 0;

    QRect target(QPoint(), QSize(kIconSize, kIconSize));
    target.moveCenter(option.rect.center());
    icon.paint(painter, target, Qt::AlignCenter, dark ? QIcon::Selected : QIcon::Normal);
}

//---------------------------------------------------------------------------------------------------------------------
OLibraryRowDelegate::OLibraryRowDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void OLibraryRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                const QModelIndex &index) const
{
    QStyleOptionViewItem viewOption = option;
    initStyleOption(&viewOption, index);

    const bool dark = (option.state & (QStyle::State_Selected | QStyle::State_MouseOver)) != 0;
    if (dark && not viewOption.icon.isNull())
    {
        const QSize size = viewOption.decorationSize.isValid() ? viewOption.decorationSize
                                                               : QSize(kIconSize, kIconSize);
        const QPixmap pixmap = viewOption.icon.pixmap(size, QIcon::Selected);
        if (not pixmap.isNull())
        {
            // Une icône d'une seule variante : la claire, que le style peindra en mode normal.
            viewOption.icon = QIcon(pixmap);
        }
    }

    const QWidget *widget = viewOption.widget;
    QStyle *style = widget != nullptr ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &viewOption, painter, widget);
}
