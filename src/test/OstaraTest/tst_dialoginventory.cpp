/************************************************************************
 **
 **  @file   tst_dialoginventory.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  L'inventaire mesuré : la hauteur que chaque dialogue du dépôt réclame réellement,
 **          comparée à ce qu'un écran de 1024×768 et un de 1366×768 laissent.
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

#include "tst_dialoginventory.h"

#include "dialoginventory_cases.h"
#include "vwidgets/odialogfit.h"

#include <QDialog>
#include "vmisc/testvapplication.h"

#include <QTest>

namespace
{
// The screens the objective names, taskbar and title bar taken out.
const QRect kSmallScreen(0, 0, 1024, 728);
const QRect kLaptopScreen(0, 0, 1366, 728);
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_DialogInventory::TST_DialogInventory(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogInventory::EveryDialogFitsTheScreensOfTheObjective()
{
    QStringList report;
    QStringList overflowing;

    QVector<DialogCase> cases = AllDialogCases();
    cases += AllCodeBuiltCases();
    for (const DialogCase &testCase : cases)
    {
        // A `.ui` is set up ON the holder; a dialog built in code is a child of it. Both are measured
        // the same way, on the widget that carries the layout.
        QDialog holder;
        holder.setObjectName(QLatin1String(testCase.name));
        testCase.setup(holder);
        QDialog *dialog = holder.layout() != nullptr ? &holder : holder.findChild<QDialog *>();
        if (dialog == nullptr)
        {
            overflowing << QStringLiteral("%1 (not built)").arg(QLatin1String(testCase.name));
            continue;
        }
        dialog->setObjectName(QLatin1String(testCase.name));

        const int needed = dialog->minimumSizeHint().height();
        const int neededW = dialog->minimumSizeHint().width();
        const ODialogFit::Verdict small = ODialogFit::Apply(dialog, kSmallScreen);
        const ODialogFit::Verdict laptop = ODialogFit::Apply(dialog, kLaptopScreen);

        report << QStringLiteral("%1: %2×%3 px asked%6 — 1024×768: %4, 1366×768: %5")
                      .arg(QLatin1String(testCase.name))
                      .arg(neededW)
                      .arg(needed)
                      .arg(small == ODialogFit::Verdict::Overflows ? QStringLiteral("OVERFLOWS")
                                                                   : QStringLiteral("fits"))
                      .arg(laptop == ODialogFit::Verdict::Overflows ? QStringLiteral("OVERFLOWS")
                                                                    : QStringLiteral("fits"))
                      .arg(needed > kSmallScreen.height() ? QStringLiteral("  ← dépasse l'écran sans la règle")
                                                          : QString());

        if (small == ODialogFit::Verdict::Overflows || laptop == ODialogFit::Verdict::Overflows)
        {
            overflowing << QStringLiteral("%1 (%2 px)").arg(QLatin1String(testCase.name)).arg(needed);
        }
    }

    qInfo("%s", qPrintable(QStringLiteral("\n%1 dialogues mesurées\n%2")
                               .arg(cases.size())
                               .arg(report.join(QLatin1Char('\n')))));

    QVERIFY2(overflowing.isEmpty(),
             qPrintable(QStringLiteral("cannot fit: ") + overflowing.join(QStringLiteral(", "))));
}

//---------------------------------------------------------------------------------------------------------------------
// Un `QApplication` nu ne suffit pas : les dialogues lisent leurs préférences par `VApp()`, qui rend un
// pointeur nul sur une application ordinaire, et la première qui l'appelle tue le processus. C'est donc
// l'application de test qui les construit.
auto main(int argc, char **argv) -> int
{
    TestVApplication const app(argc, argv);
    TST_DialogInventory test;
    return QTest::qExec(&test, argc, argv);
}
