/************************************************************************
 **
 **  @file   tst_expressionwalkers.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Un patron minimal dont **chaque** attribut-formule est renseigné avec une valeur reconnaissable, puis
 **          `ListExpressions()` doit les rendre tous. Les attributs testés sont ceux que le correctif `ca638aacf` a
 **          ajoutés aux walkers :
 **
 **            `<operation rotationAngle>`        (l'angle de rotation de l'outil Move)
 **            `<point visible>`                  (la visibilité d'un point / d'un place label)
 **            `<path visible>`                   (la visibilité d'un chemin de pièce)
 **            `<mirrorLine height|width|center>` (les trois formules de la ligne de pli)
 **            `<node passmarkWidth|passmarkAngleFormula|passmarkVisibilityFormula>`
 **
 **          Les `<data>` / `<patternInfo>` d'upstream n'existent pas chez nous (remplacés par le cartouche et les
 **          annotations) et la ligne de pli porte les mêmes trois formules.
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

#include "tst_expressionwalkers.h"

#include "vcontainer.h"
#include "vmisc/testvapplication.h"
#include "xml/vabstractpattern.h"

#include <QtTest>

namespace
{
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
};

auto MinimalPattern() -> QByteArray
{
    return QByteArrayLiteral(R"(
        <pattern>
            <draw name="Block A">
                <calculation>
                    <operation id="5" type="moveOperation" angle="0" length="0" rotationAngle="#rotationAngle"/>
                </calculation>
                <modeling>
                    <point id="6" type="placeLabel" visible="#placeLabelVisible"/>
                    <path id="7" visible="#pathVisible"/>
                </modeling>
                <details>
                    <detail id="9">
                        <mirrorLine height="#foldHeight" width="#foldWidth" center="#foldCenter"/>
                        <annotations>
                            <annotation rotation="#annotationRotation"><text>x</text></annotation>
                        </annotations>
                        <nodes>
                            <node idObject="1" type="NodePoint" passmarkWidth="#passmarkWidth"
                                  passmarkAngleFormula="#passmarkAngle"
                                  passmarkVisibilityFormula="#passmarkVisibility"/>
                        </nodes>
                    </detail>
                </details>
            </draw>
        </pattern>)");
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_ExpressionWalkers::TST_ExpressionWalkers(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_ExpressionWalkers::EveryFormulaAttributeIsListed()
{
    TestDoc doc;
    QVERIFY(doc.setContent(MinimalPattern()));

    QStringList formulas;
    for (const auto &field : doc.ListExpressions())
    {
        formulas.append(field.expression);
    }

    const QStringList expected{QStringLiteral("#rotationAngle"),
                               QStringLiteral("#placeLabelVisible"),
                               QStringLiteral("#pathVisible"),
                               QStringLiteral("#foldHeight"),
                               QStringLiteral("#foldWidth"),
                               QStringLiteral("#foldCenter"),
                               QStringLiteral("#annotationRotation"),
                               QStringLiteral("#passmarkWidth"),
                               QStringLiteral("#passmarkAngle"),
                               QStringLiteral("#passmarkVisibility")};

    QStringList missing;
    for (const auto &formula : expected)
    {
        if (not formulas.contains(formula))
        {
            missing.append(formula);
        }
    }

    QVERIFY2(missing.isEmpty(), qUtf8Printable(QStringLiteral("Absents de ListExpressions : %1")
                                                   .arg(missing.join(QStringLiteral(", ")))));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);

    TST_ExpressionWalkers tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_expressionwalkers.moc"
