/************************************************************************
 **
 **  @file   tst_incrementtables.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  The two increment tables: a row is looked for **in its own table**, never in the document.
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

#ifndef TST_INCREMENTTABLES_H
#define TST_INCREMENTTABLES_H

#include <QObject>

class TST_IncrementTables : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit TST_IncrementTables(QObject *parent = nullptr);
    ~TST_IncrementTables() override = default;

private slots:
    /// `IncrementRow` answers the row of the table it is given, even when the name lives in both.
    void IncrementRowIsReadInItsOwnTable();
    /// `FindIncrementInTable` - the lookup that replaced the scan of the document.
    void FindIncrementInTableIsReadInItsOwnTable();

private:
    Q_DISABLE_COPY_MOVE(TST_IncrementTables) // NOLINT
};

#endif // TST_INCREMENTTABLES_H
