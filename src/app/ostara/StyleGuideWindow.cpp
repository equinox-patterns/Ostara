/************************************************************************
 **
 **  @file   StyleGuideWindow.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/08/2026
 **
 **  @brief  Modeless style-guide window showing every Ostara widget in
 **          every state. Used to validate the global QSS.
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

#include "StyleGuideWindow.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "vtheme.h"
#include "vmisc/FactoryWidgets.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

// ===== Helper: section label =====
static QLabel *SectionLabel(const QString &title)
{
    auto *l = new QLabel(title);
    l->setObjectName(QStringLiteral("welcomeSectionTitle"));
    return l;
}

// ===== Helper: widget label =====
static QLabel *WidgetLabel(const QString &name)
{
    auto *l = new QLabel(name);
    l->setObjectName(QStringLiteral("welcomeRecentLabel"));
    return l;
}

// ===== Helper: row of widgets with a label above =====
static QWidget *WidgetRow(const QString &label, std::initializer_list<QWidget *> widgets)
{
    auto *w = new QWidget();
    auto *lay = new QVBoxLayout(w);
    lay->setContentsMargins(0, 4, 0, 4);
    lay->addWidget(WidgetLabel(label));
    auto *row = new QHBoxLayout();
    for (auto *child : widgets)
    {
        row->addWidget(child);
    }
    row->addStretch();
    lay->addLayout(row);
    return w;
}

// ===== Build all sections =====
void StyleGuideWindow::BuildUI()
{
    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Make the viewport transparent so the QScrollArea background (set via global
    // QSS) shows through.  QAbstractScrollArea is poorly supported by QSS — the
    // ::QScrollArea > .QWidget selector is unreliable across platforms.
    scrollArea->viewport()->setStyleSheet(
        QStringLiteral(".QWidget { background: transparent; }"));

    auto *content = new QWidget();
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    mainLayout->setSpacing(12);

    // ── Theme toggle ──
    {
        VThemeMode const currentMode =
            VAbstractApplication::VApp()->Settings()->GetThemeMode();
        auto *toggleBtn = new QPushButton(currentMode == VThemeMode::Light
            ? QStringLiteral("Mode sombre")
            : QStringLiteral("Mode clair"));
        connect(toggleBtn, &QPushButton::clicked, this, [toggleBtn]() {
            VCommonSettings *s = VAbstractApplication::VApp()->Settings();
            VThemeMode const newMode =
                (s->GetThemeMode() == VThemeMode::Light) ? VThemeMode::Dark : VThemeMode::Light;
            s->SetThemeMode(newMode);
            VTheme::InitThemeMode(); // sets both QSS and palette
            if (auto *style = qApp->style())
            {
                style->polish(qApp);
            }
            toggleBtn->setText(newMode == VThemeMode::Light
                ? QStringLiteral("Mode sombre")
                : QStringLiteral("Mode clair"));
        });
        mainLayout->addWidget(toggleBtn, 0, Qt::AlignRight);
    }

    // ── QPushButton ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QPushButton")));

        auto *normal = new QPushButton(QStringLiteral("Normal"));
        auto *disabled = new QPushButton(QStringLiteral("Disabled"));
        disabled->setEnabled(false);
        auto *def = new QPushButton(QStringLiteral("Default"));
        def->setDefault(true);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / disabled / default"), {normal, disabled, def}));
    }

    // ── QToolButton ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QToolButton")));

        auto *normal = new QToolButton();
        normal->setText(QStringLiteral("Normal"));
        auto *checked = new QToolButton();
        checked->setText(QStringLiteral("Checked"));
        checked->setCheckable(true);
        checked->setChecked(true);
        auto *disabled = new QToolButton();
        disabled->setText(QStringLiteral("Disabled"));
        disabled->setEnabled(false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / checked / disabled"), {normal, checked, disabled}));
    }

    // ── QLineEdit ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QLineEdit")));

        auto *normal = new QLineEdit(QStringLiteral("Text input"));
        auto *disabled = new QLineEdit(QStringLiteral("Disabled"));
        disabled->setEnabled(false);
        auto *placeholder = new QLineEdit();
        placeholder->setPlaceholderText(QStringLiteral("Placeholder text"));

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / disabled / placeholder"), {normal, disabled, placeholder}));
    }

    // ── QPlainTextEdit ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QPlainTextEdit")));

        auto *normal = new QPlainTextEdit();
        normal->setPlainText(QStringLiteral("Multi-line\ntext edit."));
        normal->setFixedHeight(60);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {normal}));
    }

    // ── QSpinBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QSpinBox")));

        auto *normal = new QSpinBox();
        normal->setValue(42);
        auto *disabled = new QSpinBox();
        disabled->setValue(42);
        disabled->setEnabled(false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / disabled"), {normal, disabled}));
    }

    // ── QDoubleSpinBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QDoubleSpinBox")));

        auto *normal = new QDoubleSpinBox();
        normal->setValue(3.14);
        auto *disabled = new QDoubleSpinBox();
        disabled->setValue(3.14);
        disabled->setEnabled(false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / disabled"), {normal, disabled}));
    }

    // ── QKeySequenceEdit ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QKeySequenceEdit")));

        auto *normal = new QKeySequenceEdit();

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {normal}));
    }

    // ── QComboBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QComboBox")));

        auto *normal = MakeComboBox();
        normal->addItems({QStringLiteral("Option A"), QStringLiteral("Option B"), QStringLiteral("Option C")});
        auto *disabled = MakeComboBox();
        disabled->addItems({QStringLiteral("Option A"), QStringLiteral("Option B"), QStringLiteral("Option C")});
        disabled->setEnabled(false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / disabled"), {normal, disabled}));
    }

    // ── QFontComboBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QFontComboBox")));

        auto *normal = new QFontComboBox();

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {normal}));
    }

    // ── QCheckBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QCheckBox")));

        auto *unchecked = new QCheckBox(QStringLiteral("Unchecked"));
        auto *checked = new QCheckBox(QStringLiteral("Checked"));
        checked->setChecked(true);
        auto *disabled = new QCheckBox(QStringLiteral("Disabled"));
        disabled->setEnabled(false);
        auto *disabledChecked = new QCheckBox(QStringLiteral("Disabled + checked"));
        disabledChecked->setChecked(true);
        disabledChecked->setEnabled(false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("unchecked / checked / disabled"), {unchecked, checked, disabled, disabledChecked}));
    }

    // ── QRadioButton ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QRadioButton")));

        auto *unchecked = new QRadioButton(QStringLiteral("Unchecked"));
        auto *checked = new QRadioButton(QStringLiteral("Checked"));
        checked->setChecked(true);
        auto *disabled = new QRadioButton(QStringLiteral("Disabled"));
        disabled->setEnabled(false);
        auto *disabledChecked = new QRadioButton(QStringLiteral("Disabled + checked"));
        disabledChecked->setChecked(true);
        disabledChecked->setEnabled(false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("unchecked / checked / disabled"), {unchecked, checked, disabled, disabledChecked}));
    }

    // ── QTabWidget ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QTabWidget")));

        auto *tabs = new QTabWidget();
        auto *tab1 = new QWidget();
        auto *tab1Lay = new QVBoxLayout(tab1);
        tab1Lay->addWidget(new QLabel(QStringLiteral("Content of Tab 1")));
        auto *tab2 = new QWidget();
        auto *tab2Lay = new QVBoxLayout(tab2);
        tab2Lay->addWidget(new QLabel(QStringLiteral("Content of Tab 2")));
        auto *tab3 = new QWidget();
        auto *tab3Lay = new QVBoxLayout(tab3);
        tab3Lay->addWidget(new QLabel(QStringLiteral("Disabled Tab")));
        tabs->addTab(tab1, QStringLiteral("Tab 1 (selected)"));
        tabs->addTab(tab2, QStringLiteral("Tab 2"));
        tabs->addTab(tab3, QStringLiteral("Tab 3 (disabled)"));
        tabs->setTabEnabled(2, false);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal / selected / disabled"), {tabs}));
    }

    // ── QListWidget ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QListWidget")));

        auto *lw = new QListWidget();
        lw->addItem(QStringLiteral("Normal item"));
        lw->setFixedHeight(80);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {lw}));
    }

    // ── QTreeWidget ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QTreeWidget")));

        auto *tw = new QTreeWidget();
        tw->setHeaderLabels({QStringLiteral("Column 1"), QStringLiteral("Column 2")});
        tw->addTopLevelItem(new QTreeWidgetItem({QStringLiteral("Item 1"), QStringLiteral("Value A")}));
        tw->addTopLevelItem(new QTreeWidgetItem({QStringLiteral("Item 2"), QStringLiteral("Value B")}));
        tw->setFixedHeight(90);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {tw}));
    }

    // ── QTableWidget ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QTableWidget (includes QHeaderView)")));

        auto *tab = new QTableWidget(3, 3);
        tab->setHorizontalHeaderLabels({QStringLiteral("Header 1"), QStringLiteral("Header 2"), QStringLiteral("Header 3")});
        tab->setItem(0, 0, new QTableWidgetItem(QStringLiteral("A1")));
        tab->setItem(0, 1, new QTableWidgetItem(QStringLiteral("B1")));
        tab->setItem(1, 0, new QTableWidgetItem(QStringLiteral("A2")));
        tab->setItem(1, 1, new QTableWidgetItem(QStringLiteral("B2")));
        tab->setFixedHeight(110);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {tab}));
    }

    // ── QGroupBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QGroupBox")));

        auto *gb = new QGroupBox(QStringLiteral("Group Title"));
        auto *gbLay = new QVBoxLayout(gb);
        gbLay->addWidget(new QCheckBox(QStringLiteral("Option inside group")));
        gbLay->addWidget(new QPushButton(QStringLiteral("Button inside group")));

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {gb}));
    }

    // ── QScrollBar ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QScrollBar — vertical")));

        auto *vsb = new QScrollBar(Qt::Vertical);
        vsb->setRange(0, 100);
        vsb->setValue(30);
        vsb->setFixedHeight(120);

        mainLayout->addWidget(WidgetRow(QStringLiteral("vertical"), {vsb}));
    }
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QScrollBar — horizontal")));

        auto *hsb = new QScrollBar(Qt::Horizontal);
        hsb->setRange(0, 100);
        hsb->setValue(30);

        mainLayout->addWidget(WidgetRow(QStringLiteral("horizontal"), {hsb}));
    }

    // ── QProgressBar ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QProgressBar")));

        auto *p0 = new QProgressBar();
        p0->setValue(0);
        auto *p50 = new QProgressBar();
        p50->setValue(50);

        mainLayout->addWidget(WidgetRow(QStringLiteral("0% / 50%"), {p0, p50}));
    }

    // ── QSplitter ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QSplitter — horizontal")));

        auto *split = new QSplitter(Qt::Horizontal);
        auto *left = new QFrame();
        left->setFrameStyle(QFrame::Box);
        auto *right = new QFrame();
        right->setFrameStyle(QFrame::Box);
        split->addWidget(left);
        split->addWidget(right);

        mainLayout->addWidget(WidgetRow(QStringLiteral("horizontal"), {split}));
    }

    // ── QLabel ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QLabel")));

        auto *standard = new QLabel(QStringLiteral("QLabel — standard text"));
        auto *centered = new QLabel(QStringLiteral("Centered"));
        centered->setAlignment(Qt::AlignCenter);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {standard, centered}));
    }

    // ── QDateEdit ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QDateEdit")));

        auto *de = new QDateEdit(QDate::currentDate());
        de->setCalendarPopup(true);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {de}));
    }

    // ── QToolBar ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QToolBar")));

        auto *tb = new QToolBar();
        tb->addAction(QStringLiteral("Action 1"));
        tb->addAction(QStringLiteral("Action 2"));
        tb->addSeparator();
        auto *btn = new QToolButton();
        btn->setText(QStringLiteral("ToolBtn"));
        tb->addWidget(btn);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {tb}));
    }

    // ── QMenuBar ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QMenuBar")));

        auto *mb = new QMenuBar();
        auto *menu1 = mb->addMenu(QStringLiteral("Menu A"));
        menu1->addAction(QStringLiteral("Item 1"));
        menu1->addAction(QStringLiteral("Item 2"));
        menu1->addSeparator();
        menu1->addAction(QStringLiteral("Item 3"));
        mb->addMenu(QStringLiteral("Menu B"));

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {mb}));
    }

    // ── QStatusBar ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QStatusBar")));

        auto *sb = new QStatusBar();
        sb->showMessage(QStringLiteral("Status message"));

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {sb}));
    }

    // ── QDockWidget ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QDockWidget")));

        auto *dock = new QDockWidget(QStringLiteral("Dock Title"));
        auto *dockContent = new QWidget();
        auto *dockLay = new QVBoxLayout(dockContent);
        dockLay->addWidget(new QLabel(QStringLiteral("Dock content")));
        dock->setWidget(dockContent);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {dock}));
    }

    // ── QDialogButtonBox ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QDialogButtonBox")));

        auto *dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {dbb}));
    }

    // ── QScrollArea ──
    {
        mainLayout->addWidget(SectionLabel(QStringLiteral("QScrollArea")));

        auto *sa = new QScrollArea();
        auto *saContent = new QLabel(QStringLiteral("Long content inside QScrollArea.\nLine 2\nLine 3\nLine 4"));
        saContent->setMinimumSize(100, 60);
        sa->setWidget(saContent);
        sa->setFixedSize(200, 60);

        mainLayout->addWidget(WidgetRow(QStringLiteral("normal"), {sa}));
    }

    mainLayout->addStretch();

    scrollArea->setWidget(content);
    setCentralWidget(scrollArea);
}

// ===== Constructor =====
StyleGuideWindow::StyleGuideWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setObjectName(QStringLiteral("styleGuideWindow"));
    setWindowTitle(QStringLiteral("Style Guide — Ostara Widgets"));
    resize(960, 720);
    BuildUI();
}
