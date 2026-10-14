/************************************************************************
 **
 **  @file   tst_pieceenvelope.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  The envelope of a piece: `<version>`, `<unit>`, the two increment tables, `<dependencies>`,
 **          `<measurements>`, `<lineNames>`, `<draw>` - and **in that order**, because `<piece>` is an
 **          `<xs:sequence>`: a section written out of its rank does not make a piece that reads badly, it
 **          makes a piece that cannot be written at all.
 **
 **          The sections are asked for **in the wrong order**, on purpose: `Section()` is what places them, and a
 **          test that asked for them in order would prove nothing.
 **
 **          The third test writes the envelope to `/tmp/ostara-test/` and **leaves it there**: it is the file
 **          the schema checks from the outside, with
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
/// The order of the sections of `<piece>`, as the `<xs:sequence>` of `opiece/v1.0.0.xsd` gives it.
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

/// Where the test writes the envelope, so the schema can read it back from the outside.
const QString kEnvelopePath = QStringLiteral("/tmp/ostara-test/enveloppe-piece.opiece");

const QString kLibraryId = QStringLiteral("{2dbbd77a-123f-4cd0-b56e-3cccf0ef1679}");

/// A document fragment to copy. It comes from a document **of its own**: `importNode` imports from another one.
auto Fragment(const QString &xml) -> QDomElement
{
    QDomDocument source;
    source.setContent(xml);
    return source.documentElement();
}

/// The whole envelope, asked for in the wrong order: every section has to find its rank by itself.
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

    // And what the reader wrote about the piece: the note lives on the block, so it travels with it. The text is
    // **multiline** on purpose: that is the shape an XML attribute has to carry.
    piece.SetNotes(QStringLiteral("Buste de base\nà retoucher"));
}

/// The sections of the envelope, in the order they were written.
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

    // The wrong order of the calls must let nothing slip: every section is at its rank, and `<measurements>`
    // entre `<dependencies>` et `<lineNames>`.
    QCOMPARE(SectionsOf(piece),
             QStringList({QStringLiteral("version"), QStringLiteral("unit"), QStringLiteral("increments"),
                          QStringLiteral("previewCalculations"), QStringLiteral("dependencies"),
                          QStringLiteral("measurements"), QStringLiteral("lineNames"), QStringLiteral("draw")}));

    // And the rank of each one is the schema's, with no gap and no duplicate.
    int previous = -1;
    for (const QString &tag : SectionsOf(piece))
    {
        const qsizetype rank = kSchemaOrder.indexOf(tag);
        QVERIFY2(rank >= 0, qPrintable(QStringLiteral("section unknown to the schema: %1").arg(tag)));
        QVERIFY2(rank > previous, qPrintable(QStringLiteral("section out of its rank: %1").arg(tag)));
        previous = static_cast<int>(rank);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::MeasurementsAreNamedOnceAndInOrder()
{
    OPieceDocument piece;
    piece.Create(QStringLiteral("cm"));

    // The same name asked for twice, and out of order: the piece keeps only one, and sorts them.
    piece.SetMeasurements({QStringLiteral("@TT"), QStringLiteral("@TP"), QStringLiteral("@TP")});

    QCOMPARE(piece.Measurements(), QStringList({QStringLiteral("@TP"), QStringLiteral("@TT")}));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::NotesAreCarriedByTheBlock()
{
    OPieceDocument piece;
    piece.Create(QStringLiteral("cm"));
    piece.SetDraw(Fragment(QStringLiteral("<draw/>")), QStringLiteral("Buste"), kLibraryId);

    // The note is an attribute **of the block**: that is what makes it travel into the piece and come back with it.
    piece.SetNotes(QStringLiteral("Buste de base\nà retoucher"));
    QCOMPARE(piece.Notes(), QStringLiteral("Buste de base\nà retoucher"));
    QCOMPARE(piece.DrawElement().attribute(QStringLiteral("notes")), QStringLiteral("Buste de base\nà retoucher"));

    // And empty means absent: a piece nobody said anything about carries no empty attribute.
    piece.SetNotes(QString());
    QCOMPARE(piece.Notes(), QString());
    QVERIFY2(not piece.DrawElement().hasAttribute(QStringLiteral("notes")),
             "a note emptied leaves the attribute out");
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::EmptySectionsLeaveTheDocument()
{
    // A piece that reads nothing does not write the section at all: the same rule for measurements, dependencies
    // and line names - a setter that kept what the caller just emptied would make the document say the
    // opposite of the model.
    OPieceDocument empty;
    empty.Create(QStringLiteral("cm"));
    empty.SetMeasurements({QStringLiteral("@TP")});
    empty.SetMeasurements({});
    empty.SetDependencies({OPieceDocument::Dependency{QStringLiteral("Curve_dos"), QStringLiteral("Buste"),
                                                      kLibraryId, QStringLiteral("Buste")}});
    empty.SetDependencies({});

    QVERIFY2(empty.documentElement().firstChildElement(QStringLiteral("measurements")).isNull(),
             "measurements replaced by nothing leave the section out");
    QVERIFY2(empty.documentElement().firstChildElement(QStringLiteral("dependencies")).isNull(),
             "dependencies replaced by nothing leave the section out");
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PieceEnvelope::TheWrittenEnvelopeIsLeftForTheSchemaToRead()
{
    OPieceDocument piece;
    FillEnvelope(piece);

    QVERIFY2(QDir().mkpath(QFileInfo(kEnvelopePath).absolutePath()), "the test folder must be created");

    QFile file(kEnvelopePath);
    QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text), "the file must open");

    QTextStream stream(&file);
    stream << piece.toString(4);
    file.close();

    QVERIFY2(QFileInfo(kEnvelopePath).size() > 0, "the written envelope must not be empty");
    qInfo("envelope written: %s", qUtf8Printable(kEnvelopePath));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);
    TST_PieceEnvelope tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_pieceenvelope.moc"
