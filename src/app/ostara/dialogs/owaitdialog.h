/************************************************************************
 **
 **  @file   owaitdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Minimal modal "please wait" dialog without a progress bar.
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

#ifndef OWAITDIALOG_H
#define OWAITDIALOG_H

#include <QDialog>

class QLabel;

/**
 * @brief Application-modal wait dialog without a progress bar: a single static message.
 *        No buttons — the caller closes/destroys it once the blocking work is done.
 */
class OWaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OWaitDialog(const QString &text, QWidget *parent = nullptr);
};

#endif // OWAITDIALOG_H
