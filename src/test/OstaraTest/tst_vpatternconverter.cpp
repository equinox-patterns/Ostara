/************************************************************************
 **
 **  @file   tst_vpatternconverter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Le patron d'essai est celui de l'amont (`legacy_height_line_name.val`, format 0.9.8) : un point
 **          Height H posé sur la ligne A-B, et deux points d'extrémité dont les formules nomment la ligne dans
 **          l'ancien ordre. La conversion doit les réécrire :
 **
 **            `Line_B_H`       → `Line_H_B`
 **            `AngleLine_B_H`  → `fmod(AngleLine_H_B+180;360)`   (l'angle retourné garde sa valeur)
 **
 **          Le remplacement se fait **par jetons**, jamais par `QString::replace` : `Line_A_B` est le préfixe de
 **          `Line_A_Bab`, et une substitution textuelle casserait le second nom.
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

#include "tst_vpatternconverter.h"

#include "vmisc/testvapplication.h"
#include "vmisc/vmainbase.h"
#include "xml/vpatternconverter.h"

#include <QtTest>

#include <QFile>

namespace
{
// Copie fidèle de `src/test/CollectionTest/tst_valentina/legacy_height_line_name.val` (amont, commit cbbb95a72).
auto LegacyHeightLinePattern() -> QByteArray
{
    return QByteArrayLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<pattern labelPrefix="en">
    <!--Pattern created with Valentina v1.0.4.0 (https://smart-pattern.com.ua/).-->
    <version>0.9.8</version>
    <unit>cm</unit>
    <description/>
    <notes/>
    <measurements/>
    <increments/>
    <previewCalculations/>
    <draw name="Piece1">
        <calculation>
            <point id="1" mx="0.132292" my="0.264583" name="A" showLabel="true" type="single" x="0" y="0"/>
            <point id="2" mx="0.132292" my="0.264583" name="B" showLabel="true" type="single" x="10" y="0"/>
            <point id="3" mx="0.132292" my="0.264583" name="C" showLabel="true" type="single" x="5" y="5"/>
            <point basePoint="3" id="4" lineColor="black" mx="0.132292" my="0.264583" name="H" p1Line="1" p2Line="2" showLabel="true" type="height" typeLine="hair"/>
            <point angle="AngleLine_B_H" basePoint="3" id="5" length="Line_B_H" lineColor="black" mx="0.132292" my="0.264583" name="D" showLabel="true" type="endLine" typeLine="hair"/>
            <point angle="AngleLine_H_B" basePoint="3" id="6" length="Line_H_B" lineColor="black" mx="0.132292" my="0.264583" name="E" showLabel="true" type="endLine" typeLine="hair"/>
        </calculation>
        <modeling/>
        <details/>
        <groups/>
    </draw>
</pattern>)");
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_PatternConverter::TST_PatternConverter(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PatternConverter::initTestCase()
{
    // Les schémas vivent dans une ressource compilée dans IFCLib : une bibliothèque statique ne l'enregistre pas
    // d'elle-même, et le convertisseur valide le fichier d'entrée avant de le convertir.
    Q_INIT_RESOURCE(schema);

    QVERIFY2(m_dir.isValid(), "Impossible de créer le dossier temporaire des patrons de test.");
}

//---------------------------------------------------------------------------------------------------------------------
auto TST_PatternConverter::WriteFile(const QString &name, const QByteArray &content) -> QString
{
    const QString path = m_dir.filePath(name);

    QFile file(path);
    if (not file.open(QIODevice::WriteOnly))
    {
        return {};
    }

    file.write(content);
    file.close();

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PatternConverter::HeightLineNamesAreRewritten()
{
    const QString path = WriteFile(QStringLiteral("legacy_height_line_name.val"), LegacyHeightLinePattern());
    QVERIFY(not path.isEmpty());

    VPatternConverter converter(path);
    // `Convert()` rend le chemin du fichier converti, pas son contenu : le convertisseur travaille dans un
    // fichier temporaire et l'application le relit ensuite.
    const QString convertedPath = converter.Convert();
    QVERIFY(not convertedPath.isEmpty());

    QFile converted(convertedPath);
    QVERIFY(converted.open(QIODevice::ReadOnly));
    const QString xml = QString::fromUtf8(converted.readAll());
    QVERIFY(not xml.isEmpty());

    // L'ancien ordre a disparu, le nouveau est là.
    QVERIFY2(not xml.contains(QStringLiteral("Line_B_H")), qUtf8Printable(xml));
    QVERIFY2(xml.contains(QStringLiteral("Line_H_B")), qUtf8Printable(xml));

    // L'angle retourné garde sa valeur : 180° plus l'angle dans le nouvel ordre.
    QVERIFY2(xml.contains(QStringLiteral("fmod(AngleLine_H_B+180;360)")), qUtf8Printable(xml));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);

    // La validation d'un patron passe par Xerces : sans cette initialisation, la bibliothèque tombe sur un
    // gestionnaire de mémoire nul au premier tampon.
    VXercesGuard xercesGuard;

    TST_PatternConverter tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_vpatternconverter.moc"
