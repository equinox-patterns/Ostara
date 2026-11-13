/************************************************************************
 **
 **  @file   tst_savednotification.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  La boîte qui dit qu'un enregistrement a eu lieu, et ce qu'elle dit.
 **
 **          Un enregistrement qui ne dit rien ressemble exactement à un enregistrement qui a échoué :
 **          ces cas fixent qu'une boîte apparaît, qu'elle est modale, et qu'elle nomme ce qui a été
 **          écrit. Le texte est vérifié en anglais, la traduction étant celle des `.ts`.
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

#include "tst_savednotification.h"

#include "osavednotification.h"
#include "testvapplication.h"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>
#include <QtTest>

namespace
{
/** What the box said, read from inside its own event loop — the only moment it exists. */
struct Captured
{
    bool found{false};
    bool modal{false};
    bool informative{false};
    QString text;
};

//---------------------------------------------------------------------------------------------------------------------
// Runs while the box is up: reads it, then dismisses it so the call under test returns.
void ReadAndDismiss(Captured *captured)
{
    auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
    if (box == nullptr)
    {
        return;
    }

    captured->found = true;
    captured->modal = box->isModal();
    captured->informative = box->icon() == QMessageBox::Information;
    captured->text = box->text();
    box->accept();
}

//---------------------------------------------------------------------------------------------------------------------
// Calls @p show, which is expected to open a modal box and block until it is dismissed.
auto Capture(const std::function<void(QWidget *)> &show) -> Captured
{
    Captured captured;
    QWidget parent;
    QTimer::singleShot(0, [&captured]() { ReadAndDismiss(&captured); });
    // A box that never appeared leaves nothing to close: the call returns on its own, and the case
    // fails on `found`. This one only closes a box whose first read was too early.
    QTimer::singleShot(2000, []() {
        if (QWidget *box = QApplication::activeModalWidget())
        {
            box->close();
        }
    });
    show(&parent);
    return captured;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_SavedNotification::TST_SavedNotification(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SavedNotification::TheTemplateBoxSaysSo()
{
    const Captured captured = Capture([](QWidget *parent) { OSavedNotification::Template(parent); });

    QVERIFY2(captured.found, "a box must appear after a save");
    QVERIFY2(captured.modal, "the box answers the click, so it must be modal");
    QVERIFY2(captured.informative, "a save that worked is news, not a warning");
    // The window title is not checked: macOS ignores it (and the guidelines ask for that), so it
    // reads empty here whatever the code sets.
    QCOMPARE(captured.text, QStringLiteral("The template has been saved."));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SavedNotification::TheMeasurementsBoxSaysSo()
{
    const Captured captured = Capture([](QWidget *parent) { OSavedNotification::Measurements(parent); });

    QVERIFY2(captured.found, "a box must appear after a save");
    QVERIFY2(captured.modal, "the box answers the click, so it must be modal");
    QVERIFY2(captured.informative, "a save that worked is news, not a warning");
    QCOMPARE(captured.text, QStringLiteral("The measurements table has been saved."));
}

//---------------------------------------------------------------------------------------------------------------------
auto main(int argc, char **argv) -> int
{
    TestVApplication const app(argc, argv);
    TST_SavedNotification test;
    return QTest::qExec(&test, argc, argv);
}
