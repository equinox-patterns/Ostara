/************************************************************************
 **
 **  @file   tst_pieceenvelope.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  L'enveloppe d'une pièce : `<version>`, `<unit>`, les deux tables d'incréments, `<dependencies>`,
 **          `<measurements>`, `<lineNames>`, `<draw>` — et **dans cet ordre**, parce que `<piece>` est une
 **          `<xs:sequence>` : une section écrite hors de son rang ne fait pas une pièce qui se lit mal, elle
 **          fait une pièce qui ne s'écrit pas du tout.
 **
 **          Les sections sont demandées **dans le désordre**, exprès : c'est `Section()` qui les range, et un
 **          essai qui les demanderait dans l'ordre ne prouverait rien.
 **
 **          Le troisième essai écrit l'enveloppe dans `/tmp/ostara-test/` et **l'y laisse** : c'est le fichier
 **          que le schéma vérifie de l'extérieur, avec
 **          `xmllint --noout --schema src/libs/ifc/schema/opiece/v1.0.0.xsd /tmp/ostara-test/enveloppe-piece.opiece`.
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

#include "tst_pieceenvelope.h"

#include "opiecedocument.h"
#include "vmisc/testvapplication.h"

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace
{
/// L'ordre des sections de `<piece>`, tel que l'`<xs:sequence>` de `opiece/v1.0.0.xsd` le donne.
const QStringList kSchemaOrder{QStringLiteral("version"),
                               QStringLiteral("unit"),
                               QStringLiteral("increments"),
                               QStringLiteral("incrementGroups"),
                               QStringLiteral("previewCalculations"),
                               QStringLiteral("previewCalculationGroups"),
                               QStringLiteral("dependencies"),
                               QStringLiteral("measurements"),
                               QStringLiteral("lineNames"),
                               QStringLiteral("draw")};

/// Où l'essai écrit l'enveloppe, pour que le schéma puisse la relire de l'extérieur.
const QString kEnvelopePath = QStringLiteral("/tmp/ostara-test/enveloppe-piece.opiece");

const QString kLibraryId = QStringLiteral("{2dbbd77a-123f-4cd0-b56e-3cccf0ef1679}");

/// Un morceau de document à copier. Il vient d'un document **à lui** : `importNode` importe d'un autre document.
auto Fragment(const QString &xml) -> QDomElement
{
    QDomDocument source;
    source.setContent(xml);
    return source.documentElement();
}

/// L'enveloppe complète, demandée dans le désordre : chaque section doit trouver son rang toute seule.
auto FillEnvelope(OPieceDocument &piece) -> void
{
    piece.Create(QStringLiteral("cm"));

    piece.SetMeasurements({QStringLiteral("@TT"), QStringLiteral("@TP"), QStringLiteral("@TP")});

    piece.SetDependencies({OPieceDocument::Dependency{QStringLiteral("Curve_dos"), QStringLiteral("Buste"),
                                                      kLibraryId, QStringLiteral("Buste")}});

    piece.SetDraw(Fragment(QStringLiteral("<draw><line first=\"1\" second=\"2\"/></draw>")),
                  QStringLiteral("Buste"), kLibraryId);

    piece.SetIncrements(Fragment(QStringLiteral("<increments><increment name=\"#Cou\" formula=\"1\"/>"
                                               "</increments>")),
                        {}, Fragment(QStringLiteral("<previewCalculations><increment name=\"#Z1\" formula=\"2\"/>"
                                                    "</previewCalculations>")),
                        {});

    piece.SetLineNames(Fragment(QStringLiteral("<lineNames><line points=\"A B\" name=\"RepereDos\"/>"
                                               "</lineNames>")));
}

/// Les sections de l'enveloppe, dans l'ordre où elles ont été écrites.
auto SectionsOf(const OPieceDocument &piece) -> QStringList
{
    QStringList tags;

    const QDomElement root = piece.documentElement();
    for (QDomElement child = root.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        tags.append(child.tagName());
    }

    return tags;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_PieceEnvelope::TST_PieceEnvelope(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::SectionsAreWrittenInTheSchemaOrder()
{
    OPieceDocument piece;
    FillEnvelope(piece);

    // Le désordre des appels ne doit rien laisser passer : chaque section est à son rang, et `<measurements>`
    // entre `<dependencies>` et `<lineNames>`.
    QCOMPARE(SectionsOf(piece),
             QStringList({QStringLiteral("version"), QStringLiteral("unit"), QStringLiteral("increments"),
                          QStringLiteral("previewCalculations"), QStringLiteral("dependencies"),
                          QStringLiteral("measurements"), QStringLiteral("lineNames"), QStringLiteral("draw")}));

    // Et le rang de chacune est bien celui du schéma, sans trou ni doublon.
    int previous = -1;
    for (const QString &tag : SectionsOf(piece))
    {
        const qsizetype rank = kSchemaOrder.indexOf(tag);
        QVERIFY2(rank >= 0, qPrintable(QStringLiteral("section inconnue du schéma : %1").arg(tag)));
        QVERIFY2(rank > previous, qPrintable(QStringLiteral("section hors de son rang : %1").arg(tag)));
        previous = static_cast<int>(rank);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::MeasurementsAreNamedOnceAndInOrder()
{
    OPieceDocument piece;
    piece.Create(QStringLiteral("cm"));

    // Le même nom demandé deux fois, et dans le désordre : la pièce n'en garde qu'un, et les range.
    piece.SetMeasurements({QStringLiteral("@TT"), QStringLiteral("@TP"), QStringLiteral("@TP")});

    QCOMPARE(piece.Measurements(), QStringList({QStringLiteral("@TP"), QStringLiteral("@TT")}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::EmptySectionsLeaveTheDocument()
{
    // Une pièce qui ne lit rien n'écrit pas la section du tout : la même règle pour les mesures, les dépendances
    // et les noms de lignes — un setter qui laisse ce que l'appelant vient de vider ferait dire au document le
    // contraire du modèle.
    OPieceDocument empty;
    empty.Create(QStringLiteral("cm"));
    empty.SetMeasurements({QStringLiteral("@TP")});
    empty.SetMeasurements({});
    empty.SetDependencies({OPieceDocument::Dependency{QStringLiteral("Curve_dos"), QStringLiteral("Buste"),
                                                      kLibraryId, QStringLiteral("Buste")}});
    empty.SetDependencies({});

    QVERIFY2(empty.documentElement().firstChildElement(QStringLiteral("measurements")).isNull(),
             "les mesures remplacées par rien laissent la section dehors");
    QVERIFY2(empty.documentElement().firstChildElement(QStringLiteral("dependencies")).isNull(),
             "les dépendances remplacées par rien laissent la section dehors");
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::TheWrittenEnvelopeIsLeftForTheSchemaToRead()
{
    OPieceDocument piece;
    FillEnvelope(piece);

    QVERIFY2(QDir().mkpath(QFileInfo(kEnvelopePath).absolutePath()), "le dossier d'essai doit se créer");

    QFile file(kEnvelopePath);
    QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text), "le fichier doit s'ouvrir");

    QTextStream stream(&file);
    stream << piece.toString(4);
    file.close();

    QVERIFY2(QFileInfo(kEnvelopePath).size() > 0, "l'enveloppe écrite ne doit pas être vide");
    qInfo("enveloppe écrite : %s", qUtf8Printable(kEnvelopePath));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);
    TST_PieceEnvelope tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_pieceenvelope.moc"
