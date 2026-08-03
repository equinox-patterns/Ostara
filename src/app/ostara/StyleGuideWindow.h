/************************************************************************
 **
 **  @file   StyleGuideWindow.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/08/2026
 **
 **  @brief  Modeless style-guide window showing every Ostara widget in
 **          every state. Used to validate the global QSS.
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

#ifndef STYLEGUIDEWINDOW_H
#define STYLEGUIDEWINDOW_H

#include <QMainWindow>

class StyleGuideWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit StyleGuideWindow(QWidget *parent = nullptr);
    ~StyleGuideWindow() override = default;

private:
    void BuildUI();
};

#endif // STYLEGUIDEWINDOW_H
