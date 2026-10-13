/************************************************************************
 **
 **  @file   tst_incrementtables.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/10/2026
 **
 **  @brief  Un nom peut vivre dans **les deux** tables d'incréments — `<increments>` (« Évolutions ») et
 **          `<previewCalculations>` (« Calculs préliminaires ») — et c'est un état que le produit doit savoir
 **          tenir : une ligne se cherche donc **dans la table qu'on lui nomme**. Un balayage du document, lui,
 **          répond la première ligne en ordre document, `<increments>` venant toujours en premier.
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
/// Un document concret, réduit au nécessaire : la machinerie des incréments vit dans `VAbstractPattern`.
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

    // La recherche par table est `protected` — c'est ce qui permet à `VPattern` de s'en servir ; l'essai la rend
    // lisible pour vérifier ce qu'elle répond.
    using VAbstractPattern::FindIncrementInTable;
};

/// Le même nom dans **les deux** tables, avec une formule distincte pour dire laquelle a répondu.
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
    QVERIFY2(doc.setContent(kBothTables.toUtf8()), "le document d'essai doit se lire");

    const QDomElement increments = doc.IncrementRow(QStringLiteral("#Cou"), false);
    const QDomElement preview = doc.IncrementRow(QStringLiteral("#Cou"), true);

    QVERIFY2(not increments.isNull(), "la ligne de <increments> doit être trouvée");
    QVERIFY2(not preview.isNull(), "la ligne de <previewCalculations> doit être trouvée");
    QVERIFY2(increments != preview, "les deux tables portent deux lignes distinctes, pas la même");

    QCOMPARE(increments.attribute(QStringLiteral("formula")), QStringLiteral("1"));
    QCOMPARE(preview.attribute(QStringLiteral("formula")), QStringLiteral("2"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_IncrementTables::FindIncrementInTableIsReadInItsOwnTable()
{
    TestDoc doc;
    QVERIFY2(doc.setContent(kBothTables.toUtf8()), "le document d'essai doit se lire");

    const QDomElement increments
        = doc.FindIncrementInTable(VAbstractPattern::TagIncrements, QStringLiteral("#Cou"));
    const QDomElement preview
        = doc.FindIncrementInTable(VAbstractPattern::TagPreviewCalculations, QStringLiteral("#Cou"));

    QVERIFY2(not increments.isNull() && not preview.isNull(), "chaque table doit répondre sa ligne");
    QVERIFY2(increments != preview, "un nom dans les deux tables : deux lignes, jamais la même");
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
