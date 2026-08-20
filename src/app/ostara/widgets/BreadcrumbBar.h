/************************************************************************
 **
 **  @file   BreadcrumbBar.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/08/2026
 **
 **  @brief  Breadcrumb bar shown above the canvas while navigating an attachment.
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
#ifndef BREADCRUMBBAR_H
#define BREADCRUMBBAR_H

#include <QWidget>

class QLabel;
class QPushButton;

class BreadcrumbBar final : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit BreadcrumbBar(QWidget *parent = nullptr);
    ~BreadcrumbBar() override;

    void Update(const QString &groupName, const QString &objectName);

signals:
    void canvasClicked();
    void groupClicked();

private:
    Q_DISABLE_COPY_MOVE(BreadcrumbBar) // NOLINT
    QPushButton *m_canvasButton{nullptr};
    QPushButton *m_groupButton{nullptr};
    QLabel *m_groupSeparator{nullptr};
    QLabel *m_objectSeparator{nullptr};
    QLabel *m_objectLabel{nullptr};
};

#endif // BREADCRUMBBAR_H
