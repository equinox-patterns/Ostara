/************************************************************************
 **
 **  @file   ODockTitleBar.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/08/2026
 **
 **  @brief  Title bar for a docked panel: title label and the float/close buttons.
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

// Stub — will be replaced with SVG icons
#ifndef ODOCKTITLEBAR_H
#define ODOCKTITLEBAR_H

#include <QWidget>

class QLabel;
class QPushButton;

class ODockTitleBar : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)

public:
    explicit ODockTitleBar(const QString &title, QWidget *parent = nullptr);

    void setCollapsed(bool collapsed);
    bool isCollapsed() const { return m_collapsed; }

    void setFloating(bool floating);

    void setTitle(const QString &title);

signals:
    void collapseToggled(bool collapsed);
    void detachRequested();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void updateCollapseIcon();

    QLabel *m_titleLabel = nullptr;
    QPushButton *m_collapseBtn = nullptr;
    QPushButton *m_detachBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
    bool m_collapsed = false;
};

#endif // ODOCKTITLEBAR_H
