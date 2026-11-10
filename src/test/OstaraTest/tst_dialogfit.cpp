/************************************************************************
 **
 **  @file   tst_dialogfit.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  The two invariants every dialog owes the screen — never taller than the usable area,
 **          never stretched inside — verified on the policy itself, at the sizes where they break.
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

#include "tst_dialogfit.h"

#include "vwidgets/odialogfit.h"
#include "vcontainer.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QScreen>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabWidget>
#include <QCheckBox>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTest>
#include <QVBoxLayout>

namespace
{
// A 1366 × 768 screen, its taskbar and its title bar taken out: the size at which the Export dialog
// was reported with its buttons under the edge.
constexpr int kScreenWidth = 1024;
constexpr int kScreenHeight = 728;
const QRect kSmallScreen(0, 0, kScreenWidth, kScreenHeight);

//---------------------------------------------------------------------------------------------------------------------
/// One page of a form, as tall as `rows` fields make it.
auto MakePage(int rows) -> QWidget *
{
    auto *page = new QWidget;
    auto *form = new QFormLayout(page);
    for (int i = 0; i < rows; ++i)
    {
        form->addRow(new QLabel(QStringLiteral("Row %1:").arg(i), page), new QLineEdit(page));
    }
    return page;
}

//---------------------------------------------------------------------------------------------------------------------
/// A dialog shaped like the ones that broke: pages in a tab widget, a bar of buttons under it.
auto MakeTallTabDialog(int rowsPerPage) -> QDialog *
{
    auto *dialog = new QDialog;
    auto *layout = new QVBoxLayout(dialog);

    auto *tabs = new QTabWidget(dialog);
    for (int i = 0; i < 3; ++i)
    {
        tabs->addTab(MakePage(rowsPerPage), QStringLiteral("Page %1").arg(i));
    }
    layout->addWidget(tabs);
    layout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog));

    return dialog;
}

//---------------------------------------------------------------------------------------------------------------------
/// Whether `widget` sits inside a scroll area — which it must not, when it is the way out of the dialog.
auto InsideAScrollArea(const QWidget *widget) -> bool
{
    for (const QWidget *parent = widget->parentWidget(); parent != nullptr; parent = parent->parentWidget())
    {
        if (qobject_cast<const QScrollArea *>(parent) != nullptr)
        {
            return true;
        }
    }
    return false;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_DialogFit::TST_DialogFit(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ATooTallDialogGetsAScrollAreaAndFits()
{
    const QScopedPointer<QDialog> dialog(MakeTallTabDialog(40));

    // The defect, stated: the content asks for more than the screen gives it.
    QVERIFY2(dialog->minimumSizeHint().height() > kScreenHeight,
             qPrintable(QStringLiteral("the dialog is only %1 px tall").arg(dialog->minimumSizeHint().height())));

    const ODialogFit::Verdict verdict = ODialogFit::Apply(dialog.data(), kSmallScreen);

    QCOMPARE(verdict, ODialogFit::Verdict::Scrolls);
    QVERIFY(dialog->findChild<QScrollArea *>() != nullptr);
    QVERIFY2(dialog->minimumSizeHint().height() <= kScreenHeight,
             qPrintable(QStringLiteral("still %1 px of content").arg(dialog->minimumSizeHint().height())));
    QVERIFY(dialog->maximumHeight() <= kScreenHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::TheButtonsStayInTheFrame()
{
    const QScopedPointer<QDialog> dialog(MakeTallTabDialog(40));
    ODialogFit::Apply(dialog.data(), kSmallScreen);

    auto *buttons = dialog->findChild<QDialogButtonBox *>();
    QVERIFY(buttons != nullptr);
    // The only way out of the dialog cannot scroll away with the content.
    QVERIFY(!InsideAScrollArea(buttons));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::TheDialogIsWrappedOnlyOnce()
{
    const QScopedPointer<QDialog> dialog(MakeTallTabDialog(40));

    ODialogFit::Apply(dialog.data(), kSmallScreen);
    ODialogFit::Apply(dialog.data(), kSmallScreen);
    ODialogFit::Apply(dialog.data(), kSmallScreen);

    QCOMPARE(dialog->findChildren<QScrollArea *>().size(), 1);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ADialogThatFitsIsLeftAlone()
{
    const QScopedPointer<QDialog> dialog(MakeTallTabDialog(2));
    const int needed = dialog->minimumSizeHint().height();
    QVERIFY(needed < kScreenHeight);

    const ODialogFit::Verdict verdict = ODialogFit::Apply(dialog.data(), kSmallScreen);

    QCOMPARE(verdict, ODialogFit::Verdict::Fits);
    QVERIFY(dialog->findChild<QScrollArea *>() == nullptr);
    // Capped, never squeezed: a dialog that fits keeps the height it asks for.
    QVERIFY(dialog->maximumHeight() <= kScreenHeight);
    QVERIFY(dialog->maximumHeight() >= needed);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::AFullWidthRowDoesNotSwallowTheSurplus()
{
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);

    auto *content = new QWidget(&dialog);
    auto *form = new QFormLayout(content);
    for (int i = 0; i < 5; ++i)
    {
        form->addRow(new QLabel(QStringLiteral("Row %1:").arg(i), content), new QLineEdit(content));
    }

    // A stacked widget alone on its row: Qt promotes it to expanding because it owns a layout, and the
    // form then hands it the whole surplus — the empty band the customer saw above "Cover page".
    auto *stack = new QStackedWidget(content);
    stack->addWidget(MakePage(1));
    stack->addWidget(MakePage(1));
    form->addRow(stack);
    for (int i = 0; i < 3; ++i)
    {
        form->addRow(new QLabel(QStringLiteral("Tail %1:").arg(i), content), new QLineEdit(content));
    }
    layout->addWidget(content);
    layout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog));

    ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(stack->sizePolicy().verticalPolicy(), QSizePolicy::Maximum);

    // Give the dialog far more height than its content needs: the row must keep its own height, and
    // the surplus must end up under the last row, not between the rows.
    dialog.resize(400, kScreenHeight);
    dialog.layout()->activate();

    QVERIFY2(stack->height() <= stack->sizeHint().height() + 2,
             qPrintable(QStringLiteral("the row grew to %1 px for a %2 px content")
                            .arg(stack->height())
                            .arg(stack->sizeHint().height())));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ADialogThatCannotScrollIsReported()
{
    QDialog dialog;
    // A grid, not a column: moving its items elsewhere would lose the positions it was written for,
    // so the policy must say so instead of pretending it fixed anything.
    auto *grid = new QGridLayout(&dialog);
    auto *fixed = new QWidget(&dialog);
    fixed->setFixedHeight(900);
    grid->addWidget(fixed, 0, 0);
    grid->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok, &dialog), 1, 0);

    const ODialogFit::Verdict verdict = ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(verdict, ODialogFit::Verdict::Overflows);
    QVERIFY(dialog.findChild<QScrollArea *>() == nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ACustomButtonRowStaysInTheFrame()
{
    // Not every dialog ends with a QDialogButtonBox: here the way out is two plain buttons in a row
    // of their own, and they must stay in the frame just the same.
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(MakePage(40));

    auto *bar = new QHBoxLayout;
    auto *cancel = new QPushButton(QStringLiteral("Cancel"), &dialog);
    bar->addWidget(cancel);
    bar->addWidget(new QPushButton(QStringLiteral("Generate"), &dialog));
    layout->addLayout(bar);

    const ODialogFit::Verdict verdict = ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(verdict, ODialogFit::Verdict::Scrolls);
    QVERIFY(!InsideAScrollArea(cancel));
    QVERIFY(dialog.minimumSizeHint().height() <= kScreenHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::TheExportDialogShapeFitsOnASmallScreen()
{
    // The shape of the Export dialog, row for row: pages in a tab widget, each with a form and a
    // stretch under it, a full-width stacked row inside the form, and a bar of plain buttons under
    // the tabs. This is the dialog the customer saw with its bar under the edge of the screen.
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);

    auto *tabs = new QTabWidget(&dialog);
    QStackedWidget *stack = nullptr;
    for (int page = 0; page < 3; ++page)
    {
        auto *content = new QWidget;
        auto *pageLayout = new QVBoxLayout(content);
        auto *form = new QFormLayout;
        for (int i = 0; i < 25; ++i)
        {
            form->addRow(new QLabel(QStringLiteral("Field %1:").arg(i), content), new QLineEdit(content));
        }
        auto *shape = new QStackedWidget(content);
        shape->addWidget(MakePage(1));
        shape->addWidget(MakePage(2));
        form->addRow(shape); // full-width row: the one that used to swallow the surplus
        if (stack == nullptr)
        {
            stack = shape;
        }
        pageLayout->addLayout(form);
        pageLayout->addStretch();
        tabs->addTab(content, QStringLiteral("Tab %1").arg(page));
    }
    layout->addWidget(tabs);

    auto *bar = new QHBoxLayout;
    bar->addWidget(new QLabel(QStringLiteral("Added"), &dialog), 1);
    auto *cancel = new QPushButton(QStringLiteral("Cancel"), &dialog);
    bar->addWidget(cancel);
    bar->addWidget(new QPushButton(QStringLiteral("Add to list"), &dialog));
    layout->addLayout(bar);

    const ODialogFit::Verdict verdict = ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(verdict, ODialogFit::Verdict::Scrolls);
    QVERIFY2(dialog.minimumSizeHint().height() <= kScreenHeight,
             qPrintable(QStringLiteral("still %1 px").arg(dialog.minimumSizeHint().height())));
    QVERIFY(!InsideAScrollArea(cancel));
    QVERIFY(stack != nullptr && stack->sizePolicy().verticalPolicy() == QSizePolicy::Maximum);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ANoteFieldKeepsGrowing()
{
    // The notes field of a tool dialog is content, not empty space: capping it would not remove a
    // void, it would remove the place to write.
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *notes = new QPlainTextEdit(&dialog);
    form->addRow(notes);
    layout->addLayout(form);

    ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(notes->sizePolicy().verticalPolicy(), QSizePolicy::Expanding);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ATooWideDialogGetsAScrollAreaAndFits()
{
    // Width is the same defect as height: a frame wider than the screen puts whatever sits on its
    // right edge out of reach, and a dialog is not free to be too large in one direction only.
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);
    auto *wide = new QWidget(&dialog);
    wide->setFixedWidth(2000);
    layout->addWidget(wide);

    auto *bar = new QHBoxLayout;
    auto *ok = new QPushButton(QStringLiteral("OK"), &dialog);
    bar->addWidget(ok);
    layout->addLayout(bar);

    const ODialogFit::Verdict verdict = ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(verdict, ODialogFit::Verdict::Scrolls);
    QVERIFY2(dialog.minimumSizeHint().width() <= kScreenWidth,
             qPrintable(QStringLiteral("still %1 px wide").arg(dialog.minimumSizeHint().width())));
    QVERIFY(dialog.maximumWidth() <= kScreenWidth);
    QVERIFY(!InsideAScrollArea(ok));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::TheInventoryNamesEveryDialogItMet()
{
    // The inventory is what the release audit reads: it must name every dialog the policy met, say what
    // it asked for, and give the verdict — the faulty ones included, spelled out.
    {
        const QScopedPointer<QDialog> tall(MakeTallTabDialog(40));
        tall->setObjectName(QStringLiteral("InventoryTallDialog"));
        ODialogFit::Apply(tall.data(), kSmallScreen);
    }
    {
        const QScopedPointer<QDialog> small(MakeTallTabDialog(2));
        small->setObjectName(QStringLiteral("InventorySmallDialog"));
        ODialogFit::Apply(small.data(), kSmallScreen);
    }

    const QStringList report = ODialogFit::InventoryReport();
    const QString joined = report.join(QLatin1Char('\n'));

    QVERIFY2(joined.contains(QStringLiteral("InventoryTallDialog")), qPrintable(joined));
    QVERIFY2(joined.contains(QStringLiteral("InventorySmallDialog")), qPrintable(joined));
    QVERIFY2(joined.contains(QStringLiteral("px asked,")) && joined.contains(QStringLiteral("px available,")),
             qPrintable(joined));
    QVERIFY2(joined.contains(QStringLiteral("scrolls")), qPrintable(joined));
    QVERIFY2(joined.contains(QStringLiteral("fits")), qPrintable(joined));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::AContentThatGrowsAfterTheShowIsFitted()
{
    // The promise that matters most, and the one a one-shot check would miss: a page that fills after the
    // dialog is on screen — one row per measurement axis — must still be re-measured. Here the rows arrive
    // after the show, and the dialog must end up scrollable without anyone asking again.
    ODialogFit::Install(); // idempotent, and what the application does once

    QDialog dialog;
    dialog.setObjectName(QStringLiteral("GrowingDialog"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    layout->addLayout(form);
    layout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog));

    for (int i = 0; i < 3; ++i)
    {
        form->addRow(new QLabel(QStringLiteral("Start %1:").arg(i), &dialog), new QLineEdit(&dialog));
    }

    dialog.show();
    QTest::qWait(50);
    QVERIFY2(dialog.findChild<QScrollArea *>() == nullptr, "a dialog this short has nothing to scroll");

    // The pattern opens its table: thirty rows nobody had counted on when the dialog was shown.
    for (int i = 0; i < 60; ++i)
    {
        form->addRow(new QLabel(QStringLiteral("Late %1:").arg(i), &dialog), new QLineEdit(&dialog));
    }
    QTest::qWait(200);

    const QScreen *screen = dialog.screen();
    QVERIFY(screen != nullptr);
    const int available = screen->availableGeometry().height();

    QVERIFY2(dialog.findChild<QScrollArea *>() != nullptr, "the grown content was not made scrollable");
    QVERIFY2(dialog.height() <= available,
             qPrintable(QStringLiteral("the dialog is %1 px tall on a %2 px screen").arg(dialog.height()).arg(available)));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::AContainerHoldingATableKeepsItsRoom()
{
    // A group box is a plain container. Capped from its own size policy, it would squeeze whatever table
    // it holds, and a table squeezed to three rows cannot scroll back. Content that needs the room wins,
    // and a box that merely holds a stacked form is capped like the row it is.
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;

    auto *boxWithTable = new QGroupBox(QStringLiteral("With table"), &dialog);
    auto *boxLayout = new QVBoxLayout(boxWithTable);
    boxLayout->addWidget(new QTableWidget(20, 3, boxWithTable));
    form->addRow(boxWithTable);

    auto *boxWithStack = new QGroupBox(QStringLiteral("With stack"), &dialog);
    auto *stackLayout = new QVBoxLayout(boxWithStack);
    auto *stack = new QStackedWidget(boxWithStack);
    stack->addWidget(MakePage(1));
    stack->addWidget(MakePage(2));
    stackLayout->addWidget(stack);
    form->addRow(boxWithStack);

    layout->addLayout(form);
    layout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok, &dialog));

    ODialogFit::Apply(&dialog, kSmallScreen);

    QVERIFY2(boxWithTable->sizePolicy().verticalPolicy() != QSizePolicy::Maximum,
             "a container holding a table was capped, and its table with it");
    QCOMPARE(boxWithStack->sizePolicy().verticalPolicy(), QSizePolicy::Maximum);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ALongListDoesNotGrowTheDialog()
{
    // « Une bibliothèque à 200 entrées » — la question qui décide si un dialogue de bibliothèque peut
    // déborder avec le contenu. Une liste qui défile ne pousse pas le cadre : elle défile. Une liste
    // construite en rangées dans un layout, si. Les deux cas sont mesurés ici, côte à côte.
    const auto build = [](int rows) -> QDialog *
    {
        auto *dialog = new QDialog;
        auto *layout = new QVBoxLayout(dialog);
        auto *tree = new QTreeWidget(dialog);
        for (int i = 0; i < rows; ++i)
        {
            tree->addTopLevelItem(new QTreeWidgetItem(QStringList{QStringLiteral("Entry %1").arg(i)}));
        }
        layout->addWidget(tree);
        layout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog));
        return dialog;
    };

    const QScopedPointer<QDialog> small(build(5));
    const QScopedPointer<QDialog> large(build(500));

    const int smallHeight = small->minimumSizeHint().height();
    const int largeHeight = large->minimumSizeHint().height();

    // 495 rows de plus ne doivent pas coûter un pixel de hauteur : c'est la liste qui défile.
    QVERIFY2(largeHeight <= smallHeight + 2,
             qPrintable(QStringLiteral("5 lignes: %1 px, 500 lignes: %2 px").arg(smallHeight).arg(largeHeight)));

    QCOMPARE(ODialogFit::Apply(large.data(), kSmallScreen), ODialogFit::Verdict::Fits);
    QVERIFY(large->minimumSizeHint().height() <= kScreenHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ThePositionChosenByTheReaderIsRespected()
{
    // « Les dialogues ne se déplacent pas » : sur X11 et Wayland, un client qui se replace pendant que
    // l'utilisatrice fait glisser la fenêtre fait annuler le glisser par le gestionnaire de fenêtres.
    // La position choisie n'est donc jamais reprise tant que le cadre est entier.
    QDialog dialog;
    dialog.resize(220, 160);

    const QRect available = dialog.screen()->availableGeometry();
    dialog.move(available.topLeft() + QPoint(60, 50));
    const QPoint chosen = dialog.pos();

    ODialogFit::KeepOnScreen(&dialog);

    QCOMPARE(dialog.pos(), chosen);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ADialogPartlyOffScreenIsBroughtBack()
{
    QDialog dialog;
    dialog.resize(220, 160);

    const QRect available = dialog.screen()->availableGeometry();
    dialog.move(available.right() - 40, available.bottom() - 30); // its right and bottom hang out

    ODialogFit::KeepOnScreen(&dialog);

    const QRect frame = dialog.frameGeometry();
    QVERIFY2(available.contains(frame), qPrintable(QStringLiteral("frame %1,%2 %3×%4 outside %5,%6 %7×%8")
                                                      .arg(frame.x()).arg(frame.y())
                                                      .arg(frame.width()).arg(frame.height())
                                                      .arg(available.x()).arg(available.y())
                                                      .arg(available.width()).arg(available.height())));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ADialogThatLockedItsOwnSizeKeepsIt()
{
    // « Nouveau patron » se verrouille à sa taille : minimum = maximum. Le plafond de l'écran est bien plus
    // grand, et pourtant il ne doit pas relever ce maximum — sinon le verrou tombe et la dialogue se remet
    // à grandir, ce que son auteur a explicitement refusé.
    QDialog dialog;
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(QStringLiteral("A fixed form"), &dialog));
    layout->addWidget(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog));

    dialog.adjustSize();
    const QSize locked = dialog.size();
    dialog.setMinimumSize(locked);
    dialog.setMaximumSize(locked);

    ODialogFit::Apply(&dialog, kSmallScreen);

    QCOMPARE(dialog.maximumHeight(), locked.height());
    QCOMPARE(dialog.maximumWidth(), locked.width());
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
void TST_DialogFit::ACentredDialogIsWholeAndRaisedWhenTheScreenIsShort()
{
    // « Quand elle s'ouvre, elle est trop basse » : une fenêtre centrée sur un écran trop court pend sous le
    // bord. Elle doit être entière — centrée quand elle tient, remontée sinon — et jamais plus grande que la
    // zone utile.
    const QRect available = QGuiApplication::primaryScreen()->availableGeometry();

    QDialog fitting;
    fitting.resize(400, 300);
    ODialogFit::CenterOnScreen(&fitting);
    QVERIFY(available.contains(fitting.frameGeometry()));
    // Centred: the two margins above and below are equal to a pixel or two.
    const int above = fitting.frameGeometry().top() - available.top();
    const int below = available.bottom() - fitting.frameGeometry().bottom();
    QVERIFY2(qAbs(above - below) <= 2, qPrintable(QStringLiteral("above %1, below %2").arg(above).arg(below)));

    QDialog tallerThanTheScreen;
    tallerThanTheScreen.resize(400, available.height() + 400);
    ODialogFit::CenterOnScreen(&tallerThanTheScreen);
    QVERIFY2(tallerThanTheScreen.height() <= available.height(),
             qPrintable(QStringLiteral("still %1 px tall for a %2 px screen")
                            .arg(tallerThanTheScreen.height())
                            .arg(available.height())));
    QVERIFY2(tallerThanTheScreen.frameGeometry().top() >= available.top(),
             "a window taller than the screen must keep its top visible");
}

QTEST_MAIN(TST_DialogFit)
