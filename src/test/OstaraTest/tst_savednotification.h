/************************************************************************
 **
 **  @file   tst_savednotification.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  La boîte qui dit qu'un enregistrement a eu lieu, et ce qu'elle dit.
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

#ifndef TST_SAVEDNOTIFICATION_H
#define TST_SAVEDNOTIFICATION_H

#include <QObject>

class TST_SavedNotification : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_SavedNotification(QObject *parent = nullptr);
    ~TST_SavedNotification() override = default;

private slots:
    void TheTemplateBoxSaysSo();
    void TheMeasurementsBoxSaysSo();
};

#endif // TST_SAVEDNOTIFICATION_H
