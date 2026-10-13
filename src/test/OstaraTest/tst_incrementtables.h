/************************************************************************
 **
 **  @file   tst_incrementtables.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  Les deux tables d'incréments : une ligne se cherche **dans sa table**, jamais dans le document.
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
    /// `IncrementRow` rend la ligne de la table qu'on lui nomme, même quand le nom vit dans les deux.
    void IncrementRowIsReadInItsOwnTable();
    /// `FindIncrementInTable` — la recherche qui a remplacé le balayage du document.
    void FindIncrementInTableIsReadInItsOwnTable();

private:
    Q_DISABLE_COPY_MOVE(TST_IncrementTables) // NOLINT
};

#endif // TST_INCREMENTTABLES_H
