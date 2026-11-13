/************************************************************************
 **
 **  @file   tst_patternversions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  The versions of the pattern, as the dock and the information
 **          block read them.
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

#include "tst_patternversions.h"

#include "opatternversions.h"

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QSharedPointer>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// Minimal concrete document: these questions are about the DOM and about who wears what, not about a pattern that
// can be drawn.
class TestDoc final : public VAbstractPattern
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
};

//---------------------------------------------------------------------------------------------------------------------
// A pattern with three versions: the entry every piece belongs to, then A and B. Two pieces, one wearing A, the
// other wearing nothing — which is the entry every piece belongs to.
auto VersionedDocument() -> QByteArray
{
    return QByteArrayLiteral("<pattern><version>1.2.0</version><unit>cm</unit>"
                             "<patternVersions active=\"true\">"
                             "<patternVersion key=\"1\" name=\"All versions\"/>"
                             "<patternVersion key=\"2\" name=\"Version A\"/>"
                             "<patternVersion key=\"3\" name=\"Version B\"/>"
                             "</patternVersions>"
                             "<draw name=\"B\"><calculation/><modeling/><details>"
                             "<detail id=\"10\" name=\"P1\" patternVersion=\"2\"><nodes/></detail>"
                             "<detail id=\"11\" name=\"P2\"><nodes/></detail>"
                             "</details></draw></pattern>");
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_PatternVersions::TST_PatternVersions(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_PatternVersions::GroupShowsItsMembersVersions()
{
    TestDoc doc;
    const QDomDocument::ParseResult parsed = doc.setContent(VersionedDocument());
    QVERIFY2(parsed, qUtf8Printable(parsed.errorMessage));

    const Unit unit = Unit::Cm;
    QSharedPointer<VContainer> data(new VContainer(nullptr, &unit, VContainer::UniqueNamespace()));

    VPiece pieceA;
    pieceA.SetPatternVersion(2);
    VPiece pieceB;
    pieceB.SetPatternVersion(3);
    VPiece pieceAll; // told nothing: the entry every piece belongs to
    data->DataPieces()->insert(10, pieceA);
    data->DataPieces()->insert(11, pieceB);
    data->DataPieces()->insert(12, pieceAll);

    // Two members, two versions: the group is both of them.
    OPieceGroup group;
    group.pieceIds = {10, 11};
    QCOMPARE(OPatternVersions::Ids(doc, data.data(), group), (QList<quint32>{2, 3}));
    QCOMPARE(OPatternVersions::Join(OPatternVersions::Names(doc, OPatternVersions::Ids(doc, data.data(), group))),
             u"Version A + Version B"_s);

    // "All versions" is not absorbed by the union: a group is the versions of its members, all of them, and the
    // order is the pattern's, so the entry that covers everything stands first.
    group.pieceIds = {12, 10};
    QCOMPARE(OPatternVersions::Join(OPatternVersions::Names(doc, OPatternVersions::Ids(doc, data.data(), group))),
             u"All versions + Version A"_s);

    // A piece wears one version, and only one.
    QCOMPARE(OPatternVersions::Ids(doc, data.data(), pieceA), (QList<quint32>{2}));

    // Who wears what, read from the file itself: this is the count the version dialog refuses a deletion on.
    QCOMPARE(OPatternVersions::PieceIdsUsing(doc, 2), (QList<quint32>{10}));
    QCOMPARE(OPatternVersions::PieceIdsUsing(doc, 1), (QList<quint32>{11}));
    QCOMPARE(OPatternVersions::PieceIdsUsing(doc, 3), (QList<quint32>{}));
}
