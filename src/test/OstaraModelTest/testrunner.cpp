/************************************************************************
 **
 **  @file   testrunner.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/10/2026
 **
 **  @brief  Le `main` commun des essais du modèle. Un exécutable par `tst_*.cpp` : le fichier de test est
 **          choisi par deux macros de compilation (OSTARA_TEST_HEADER / OSTARA_TEST_CLASS), ce qui évite de
 **          recopier le montage ci-dessous trente-deux fois.
 **
 **          Ce montage n'est pas décoratif : `Q_INIT_RESOURCE(schema)` rend les schémas XSD lisibles, Xerces
 **          doit être initialisé avant tout parseur, et `TestVApplication` fournit le `QApplication` dont
 **          `QPrinter` a besoin. Sans lui, les essais qui touchent au XML ou à l'impression tombent avant
 **          d'avoir commencé.
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

#include <QtTest>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QScopeGuard>
#include <xercesc/util/PlatformUtils.hpp>
#endif

#include "vmisc/testvapplication.h"

#include OSTARA_TEST_HEADER

//---------------------------------------------------------------------------------------------------------------------
#ifdef OSTARA_TEST_DATA
// Défini dans testdata_init.cpp : une bibliothèque statique dont personne ne cite rien se fait jeter par
// l'éditeur de liens, ses ressources avec elle.
void InitOstaraTestData();
#endif

//---------------------------------------------------------------------------------------------------------------------
auto main(int argc, char **argv) -> int
{
    Q_INIT_RESOURCE(schema); // NOLINT

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
    auto terminate = qScopeGuard([]() { XERCES_CPP_NAMESPACE::XMLPlatformUtils::Terminate(); });
#endif

    TestVApplication const app(argc, argv); // Pour QPrinter

#ifdef OSTARA_TEST_DATA
    InitOstaraTestData();
#endif

    OSTARA_TEST_CLASS test;
    return QTest::qExec(&test, argc, argv);
}
