/************************************************************************
 **
 **  @file   testdata_init.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/10/2026
 **
 **  @brief  Force les données d'essai dans le binaire. Les ressources d'une bibliothèque statique sont
 **          jetées par l'éditeur de liens si rien ne les cite : cette fonction est le seul point d'entrée
 **          qui les retient, et `testrunner.cpp` l'appelle quand OSTARA_TEST_DATA est défini.
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

#include <QtGlobal>

//---------------------------------------------------------------------------------------------------------------------
void InitOstaraTestData()
{
    Q_INIT_RESOURCE(test_data); // NOLINT
}
