/************************************************************************
 **
 **  @file   tst_oomtconverter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  La fixture est le fichier qui échouait : `GOST_man_ru.vst`, format 0.4.2, copié de la collection de
 **          Valentina. Avant le correctif, `Convert()` rendait un résultat vide (« No dimensions found ») ; il doit
 **          maintenant rendre un document, ses deux axes (hauteur et taille) et ses mesures.
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

#include "tst_oomtconverter.h"

#include "testvapplication.h"
#include "vmainbase.h"
#include "xml/oomtconverter.h"
#include "xml/oomtdocument.h"

#include <QtTest>

//---------------------------------------------------------------------------------------------------------------------
TST_OomtConverter::TST_OomtConverter(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_OomtConverter::LegacyVstIsUpgradedBeforeParsing()
{
    Q_INIT_RESOURCE(schema);

    const QString path = QStringLiteral(OSTARA_TEST_FIXTURES_DIR "/legacy_vst_0_4_2.vst");
    QVERIFY2(QFile::exists(path), qUtf8Printable(path));

    OOmtConverter converter;
    const OOmtConversionResult result = converter.Convert(path);

    QVERIFY2(result.document != nullptr, "Un .vst 0.4.2 doit être mis à niveau puis converti.");
    QCOMPARE(result.document->GetSizeGroupCount(), 2); // hauteur et taille
    QCOMPARE(result.document->GetMCount(), 3);
    QVERIFY(result.unresolvedMeasurements.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);
    VXercesGuard xercesGuard;

    TST_OomtConverter tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_oomtconverter.moc"
