/************************************************************************
 **
 **  @file   oselectallspinbox.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  QDoubleSpinBox that selects all text on focus.
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

#ifndef OSELECTALLSPINBOX_H
#define OSELECTALLSPINBOX_H

#include <QDoubleSpinBox>
#include <QTimer>

class OSelectAllSpinBox : public QDoubleSpinBox
{
public:
    explicit OSelectAllSpinBox(QWidget *parent = nullptr)
        : QDoubleSpinBox(parent)
    {
    }

protected:
    void focusInEvent(QFocusEvent *e) override
    {
        QDoubleSpinBox::focusInEvent(e);
        QTimer::singleShot(0, this, &QDoubleSpinBox::selectAll);
    }

private:
    OSelectAllSpinBox(const OSelectAllSpinBox &) = delete;
    OSelectAllSpinBox &operator=(const OSelectAllSpinBox &) = delete;
};

#endif // OSELECTALLSPINBOX_H
