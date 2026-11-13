/************************************************************************
 **
 **  @file   tst_layergroupchooser.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  La ligne « dans quel groupe de calques ? » : ce qu'elle accepte comme réponse.
 **
 **          Trois réponses sont valides : « aucun groupe », un groupe qui existe, et un nom
 **          neuf — ce dernier seulement une fois tapé. L'entrée « Nouveau groupe… » seule ne
 **          nomme rien, et c'est le seul cas que `IsValid()` doit refuser : c'est l'inverse qui
 **          était écrit, et le OK restait grisé pendant qu'on tapait le nom.
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

#include "tst_layergroupchooser.h"

#include "testvapplication.h"
#include "typedef.h"
#include "vtools/dialogs/widgets/olayergroupchooser.h"

#include <QComboBox>
#include <QLineEdit>
#include <QSignalSpy>
#include <QtTest>

namespace
{
// One group of the piece, as the dock hands it over: an id and the name the reader sees.
constexpr quint32 kExistingGroup = 5;
const QString kExistingName = QStringLiteral("Construction");

//---------------------------------------------------------------------------------------------------------------------
auto MakeChooser() -> OLayerGroupChooser *
{
    auto *chooser = new OLayerGroupChooser(nullptr);
    chooser->SetGroups({{kExistingGroup, kExistingName}});
    return chooser;
}

//---------------------------------------------------------------------------------------------------------------------
auto NameField(OLayerGroupChooser &chooser) -> QLineEdit *
{
    return chooser.findChild<QLineEdit *>();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_LayerGroupChooser::TST_LayerGroupChooser(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::NoGroupIsAnAnswer()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetGroup(NULL_ID);

    QCOMPARE(chooser->Group(), NULL_ID);
    QVERIFY(chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::TheNewGroupEntryAloneIsNoAnswer()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetGroup(OLayerGroupChooser::kCreateGroup);

    // The entry is chosen, the field is showing and empty: nothing is named yet.
    QLineEdit *field = NameField(*chooser);
    QVERIFY(field != nullptr);
    QVERIFY(!field->isHidden());
    QVERIFY(chooser->NewGroupName().isEmpty());

    QCOMPARE(chooser->Group(), OLayerGroupChooser::kCreateGroup);
    QVERIFY(!chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::ANamedNewGroupIsAnAnswer()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetGroup(OLayerGroupChooser::kCreateGroup);

    QLineEdit *field = NameField(*chooser);
    QVERIFY(field != nullptr);
    field->setText(kExistingName + QStringLiteral(" 2"));

    // The name is simplified on the way out — leading and trailing spaces go, inner runs collapse to one space:
    // a stray space is not a second group.
    QCOMPARE(chooser->NewGroupName(), kExistingName + QStringLiteral(" 2"));
    QCOMPARE(chooser->Group(), OLayerGroupChooser::kCreateGroup);
    QVERIFY(chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::ANamedNewGroupIsNotAnExistingOne()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetGroup(OLayerGroupChooser::kCreateGroup);

    QLineEdit *field = NameField(*chooser);
    QVERIFY(field != nullptr);
    field->setText(QStringLiteral("Pli"));

    // A name no group wears is a new group, never a join: the caller creates it, and the layer is written with it.
    QCOMPARE(chooser->Group(), OLayerGroupChooser::kCreateGroup);
    QVERIFY(chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::ANameAnExistingGroupWearsJoinsIt()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetGroup(OLayerGroupChooser::kCreateGroup);

    QLineEdit *field = NameField(*chooser);
    QVERIFY(field != nullptr);
    // Typed in another case: two groups the reader cannot tell apart would be two rows saying the same thing.
    field->setText(kExistingName.toLower());

    QCOMPARE(chooser->Group(), kExistingGroup);
    QVERIFY(chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::TypingRechecksTheOkButton()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetGroup(OLayerGroupChooser::kCreateGroup);

    // The dialog's OK hangs on this signal alone (DialogGroup connects it to CheckState). Without it the button
    // keeps the state it had before the name was typed, which is what "OK reste grisé" looked like.
    QSignalSpy const spy(chooser.data(), &OLayerGroupChooser::Changed);
    QLineEdit *field = NameField(*chooser);
    QVERIFY(field != nullptr);
    field->setText(QStringLiteral("Pli"));

    QCOMPARE(spy.count(), 1);
    QVERIFY(chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LayerGroupChooser::TheEntryIsOnlyOfferedWhenCreatingIsAllowed()
{
    QScopedPointer<OLayerGroupChooser> const chooser(MakeChooser());
    chooser->SetCreatable(false);

    // A dialog that only picks an existing group (a transfer, for one) must not offer to name a new one.
    auto *combo = chooser->findChild<QComboBox *>();
    QVERIFY(combo != nullptr);
    QCOMPARE(combo->findData(OLayerGroupChooser::kCreateGroup), -1);
    QVERIFY(chooser->IsValid());
}

//---------------------------------------------------------------------------------------------------------------------
// Les dialogues lisent leurs préférences par `VApp()` : l'application de test les construit, comme le fait
// l'inventaire des dialogues.
auto main(int argc, char **argv) -> int
{
    TestVApplication const app(argc, argv);
    TST_LayerGroupChooser test;
    return QTest::qExec(&test, argc, argv);
}
