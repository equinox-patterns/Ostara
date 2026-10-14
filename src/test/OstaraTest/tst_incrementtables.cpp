/************************************************************************
 **
 **  @file   tst_incrementtables.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  A name can live in **both** increment tables - `<increments>` and
 **          `<previewCalculations>` - and that is a state the product has to hold:
 **          a row is therefore looked for **in the table it is given**. A scan of the document, on the other
 **          answers the first row in document order, `<increments>` always coming first.
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

#include "tst_incrementtables.h"

#include "ifc/xml/vabstractpattern.h"
#include "vmisc/testvapplication.h"

#include <QtTest>

namespace
{
/// A concrete document, reduced to what is needed: the increment machinery lives in `VAbstractPattern`.
class TestDoc : public VAbstractPattern
{
public:
    explicit TestDoc(QObject *parent = nullptr)
      : VAbstractPattern(parent)
    {
    }

    void CreateEmptyFile() override {}
    auto GenerateLabel(const LabelType &type, const QString &reservedName = QString()) const -> QString override
    {
        Q_UNUSED(type)
        Q_UNUSED(reservedName)
        return {};
    }
    void UpdateToolData(const quint32 &id, VContainer *data) override
    {
        Q_UNUSED(id)
        Q_UNUSED(data)
    }
    void LiteParseTree(const Document &parse) override { Q_UNUSED(parse) }

    // The lookup by table is `protected` - that is what lets `VPattern` use it; the test makes it
    // readable to check what it answers.
    using VAbstractPattern::FindIncrementInTable;
};

/// The same name in **both** tables, with a distinct formula to say which one answered.
const QString kBothTables = QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?>
<pattern>
    <version>1.0.0</version>
    <unit>cm</unit>
    <increments>
        <increment name="#Cou" formula="1"/>
    </increments>
    <previewCalculations>
        <increment name="#Cou" formula="2"/>
    </previewCalculations>
</pattern>)");
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_IncrementTables::TST_IncrementTables(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_IncrementTables::IncrementRowIsReadInItsOwnTable()
{
    TestDoc doc;
    QVERIFY2(doc.setContent(kBothTables.toUtf8()), "the test document must be read");

    const QDomElement increments = doc.IncrementRow(QStringLiteral("#Cou"), false);
    const QDomElement preview = doc.IncrementRow(QStringLiteral("#Cou"), true);

    QVERIFY2(not increments.isNull(), "the row of <increments> must be found");
    QVERIFY2(not preview.isNull(), "the row of <previewCalculations> must be found");
    QVERIFY2(increments != preview, "the two tables hold two distinct rows, not the same one");

    QCOMPARE(increments.attribute(QStringLiteral("formula")), QStringLiteral("1"));
    QCOMPARE(preview.attribute(QStringLiteral("formula")), QStringLiteral("2"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_IncrementTables::FindIncrementInTableIsReadInItsOwnTable()
{
    TestDoc doc;
    QVERIFY2(doc.setContent(kBothTables.toUtf8()), "the test document must be read");

    const QDomElement increments
        = doc.FindIncrementInTable(VAbstractPattern::TagIncrements, QStringLiteral("#Cou"));
    const QDomElement preview
        = doc.FindIncrementInTable(VAbstractPattern::TagPreviewCalculations, QStringLiteral("#Cou"));

    QVERIFY2(not increments.isNull() && not preview.isNull(), "every table must answer its own row");
    QVERIFY2(increments != preview, "a name in both tables: two rows, never the same one");
    QCOMPARE(increments.attribute(QStringLiteral("formula")), QStringLiteral("1"));
    QCOMPARE(preview.attribute(QStringLiteral("formula")), QStringLiteral("2"));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);
    TST_IncrementTables tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_incrementtables.moc"
