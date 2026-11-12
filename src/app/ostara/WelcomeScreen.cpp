/************************************************************************
 **
 **  @file   WelcomeScreen.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/08/2026
 **
 **  @brief  Welcome screen with two sections: Patterns, Measurements.
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

#include "WelcomeScreen.h"

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
// How many entries a section's recent list may show. A history is a shortcut, not an archive: past this the
// wanted file is no longer in sight, and the section grows past the screen. The settings keep more
// (VAbstractMainWindow::MaxRecentFilesStored); the screen shows the most recent ones, and never more than this.
constexpr int kMaxRecentRows = 20;
} // namespace

// ===== Helper to create a section frame =====

static auto CreateSection(QWidget *parent, const QString &titleText,
                          const QString &newText, const QString &openText) -> WelcomeScreen::Section
{
    WelcomeScreen::Section s;

    s.frame = new QFrame(parent);
    s.frame->setObjectName(QStringLiteral("welcomeSection"));
    auto *sectionLayout = new QVBoxLayout(s.frame);
    sectionLayout->setContentsMargins(16, 12, 16, 12);
    sectionLayout->setSpacing(8);

    // Title — styled via QSS (#welcomeSectionTitle)
    s.title = new QLabel(titleText);
    s.title->setObjectName(QStringLiteral("welcomeSectionTitle"));
    sectionLayout->addWidget(s.title);

    // New button — styled via QSS (#welcomeNewBtn)
    s.newBtn = new QPushButton(newText);
    s.newBtn->setObjectName(QStringLiteral("welcomeNewBtn"));
    sectionLayout->addWidget(s.newBtn);

    // Open button — styled via QSS (#welcomeOpenBtn)
    s.openBtn = new QPushButton(openText);
    s.openBtn->setObjectName(QStringLiteral("welcomeOpenBtn"));
    sectionLayout->addWidget(s.openBtn);

    // Spacing above separator
    sectionLayout->addSpacing(16);

    // Separator (1px, 80% width) — centered via HBoxLayout with stretch
    auto *sepLayout = new QHBoxLayout();
    sepLayout->addStretch(1);
    auto *separator = new QFrame();
    separator->setObjectName(QStringLiteral("welcomeSeparator"));
    separator->setFixedHeight(1);
    sepLayout->addWidget(separator, 8); // 80% = 8/(1+8+1)
    sepLayout->addStretch(1);
    sectionLayout->addLayout(sepLayout);

    // Spacing below separator
    sectionLayout->addSpacing(16);

    // Recent list label — styled via QSS (#welcomeRecentLabel)
    s.recentLabel = new QLabel(QObject::tr("Recent:"));
    s.recentLabel->setObjectName(QStringLiteral("welcomeRecentLabel"));
    sectionLayout->addWidget(s.recentLabel);

    // Recent list — styled via QSS (#welcomeRecentList)
    s.recentList = new QListWidget();
    s.recentList->setObjectName(QStringLiteral("welcomeRecentList"));
    s.recentList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sectionLayout->addWidget(s.recentList, 1);

    sectionLayout->addStretch();

    return s;
}

// ===== Constructor =====

WelcomeScreen::WelcomeScreen(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("welcomeScreen"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 32, 40, 32);
    mainLayout->setSpacing(16);

    CreateHeader();
    CreateSections();
}

void WelcomeScreen::CreateHeader()
{
    auto *mainLayout = qobject_cast<QVBoxLayout *>(layout());

    // Title — styled via QSS (#welcomeTitle)
    m_titleLabel = new QLabel(QStringLiteral("Ostara"));
    m_titleLabel->setObjectName(QStringLiteral("welcomeTitle"));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_titleLabel);

    // Version — styled via QSS (#welcomeVersion)
    QString version = QApplication::applicationVersion();
    if (version.isEmpty())
    {
        version = QStringLiteral("1.0");
    }
    m_versionLabel = new QLabel(version);
    m_versionLabel->setObjectName(QStringLiteral("welcomeVersion"));
    m_versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_versionLabel);

    // Subtitle — styled via QSS (#welcomeSubtitle)
    m_subtitleLabel = new QLabel(tr("Sewing pattern design application"));
    m_subtitleLabel->setObjectName(QStringLiteral("welcomeSubtitle"));
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_subtitleLabel);

    mainLayout->addSpacing(16);
}

void WelcomeScreen::CreateSections()
{
    auto *mainLayout = qobject_cast<QVBoxLayout *>(layout());

    auto *sectionsLayout = new QHBoxLayout();
    sectionsLayout->setSpacing(20);

    // Center the two columns: side stretches take 17.5% each,
    // each section takes 32.5%. Ratio = 7:13:20px:13:7.
    sectionsLayout->addStretch(7);

    // --- Patterns section ---
    m_patternsSection = CreateSection(this,
        tr("Patterns"),
        tr("+ New pattern"),
        tr("Open pattern..."));

    QObject::connect(m_patternsSection.newBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::NewPatternClicked);
    QObject::connect(m_patternsSection.openBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::OpenPatternClicked);
    QObject::connect(m_patternsSection.recentList, &QListWidget::itemDoubleClicked,
                     this, [this](QListWidgetItem *item) {
                         emit OpenRecentPattern(item->data(Qt::UserRole).toString());
                     });
    InstallRecentContextMenu(m_patternsSection.recentList, true);

    sectionsLayout->addWidget(m_patternsSection.frame, 13);

    // --- Measurements section ---
    m_measurementsSection = CreateSection(this,
        tr("Measurement tables"),
        tr("+ New table"),
        tr("Open table..."));

    QObject::connect(m_measurementsSection.newBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::NewMeasurementClicked);
    QObject::connect(m_measurementsSection.openBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::OpenMeasurementClicked);
    QObject::connect(m_measurementsSection.recentList, &QListWidget::itemDoubleClicked,
                     this, [this](QListWidgetItem *item) {
                         emit OpenRecentMeasurement(item->data(Qt::UserRole).toString());
                     });
    InstallRecentContextMenu(m_measurementsSection.recentList, false);

    sectionsLayout->addWidget(m_measurementsSection.frame, 13);

    sectionsLayout->addStretch(7);

    mainLayout->addLayout(sectionsLayout, 1);

}

void WelcomeScreen::InstallRecentContextMenu(QListWidget *list, const bool patternList)
{
    list->setContextMenuPolicy(Qt::CustomContextMenu);

    QObject::connect(list, &QListWidget::customContextMenuRequested, this,
                     [this, list, patternList](const QPoint &pos)
                     {
                         // The signal reports the widget's own coordinates; the entries live in the viewport.
                         const QPoint viewportPos = list->viewport()->mapFrom(list, pos);

                         QListWidgetItem *item = list->itemAt(viewportPos);
                         if (item == nullptr)
                         {
                             return; // Empty space: there is no entry to take out.
                         }

                         const QString path = item->data(Qt::UserRole).toString();

                         QMenu menu(this);
                         QAction *const removeAction = menu.addAction(tr("Remove from the list"));
                         if (menu.exec(list->viewport()->mapToGlobal(viewportPos)) != removeAction)
                         {
                             return;
                         }

                         // Removing is not deleting: the file stays where it is, and opening it again puts it
                         // back at the head of the list. Only the history forgets it.
                         if (patternList)
                         {
                             emit RemoveRecentPattern(path);
                         }
                         else
                         {
                             emit RemoveRecentMeasurement(path);
                         }
                     });
}

void WelcomeScreen::RetranslateUI()
{
    m_subtitleLabel->setText(tr("Sewing pattern design application"));

    m_patternsSection.recentLabel->setText(QObject::tr("Recent:"));
    m_patternsSection.title->setText(tr("Patterns"));
    m_patternsSection.newBtn->setText(tr("+ New pattern"));
    m_patternsSection.openBtn->setText(tr("Open pattern..."));

    m_measurementsSection.recentLabel->setText(QObject::tr("Recent:"));
    m_measurementsSection.title->setText(tr("Measurement tables"));
    m_measurementsSection.newBtn->setText(tr("+ New table"));
    m_measurementsSection.openBtn->setText(tr("Open table..."));
}

void WelcomeScreen::UpdateRecentItems(const QStringList &patternPaths, const QStringList &measurementPaths)
{
    // Patterns — only .ostara files (exclude .val)
    m_patternsSection.recentList->clear();
    for (const QString &path : patternPaths)
    {
        if (m_patternsSection.recentList->count() >= kMaxRecentRows)
        {
            break; // The limit is applied where the rows are made, so no refresh can ever show more.
        }
        if (!path.endsWith(QStringLiteral(".ostara"), Qt::CaseInsensitive))
        {
            continue;
        }
        QFileInfo fi(path);
        // Patterns are always .ostara — show the file name without the extension.
        auto *item = new QListWidgetItem(fi.completeBaseName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        m_patternsSection.recentList->addItem(item);
    }

    // Measurements
    m_measurementsSection.recentList->clear();
    for (const QString &path : measurementPaths)
    {
        if (m_measurementsSection.recentList->count() >= kMaxRecentRows)
        {
            break;
        }
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(fi.fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        m_measurementsSection.recentList->addItem(item);
    }

    // Fit the rows to the room each section has, right away and again once the layout has settled: the
    // viewport height is only known then, but the entries must never be visible above their limit meanwhile.
    TrimListToFit(m_patternsSection.recentList);
    TrimListToFit(m_measurementsSection.recentList);

    QTimer::singleShot(0, this, [this]()
    {
        TrimListToFit(m_patternsSection.recentList);
        TrimListToFit(m_measurementsSection.recentList);
    });
}

void WelcomeScreen::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_patternsSection.recentList != nullptr)
    {
        TrimListToFit(m_patternsSection.recentList);
    }
    if (m_measurementsSection.recentList != nullptr)
    {
        TrimListToFit(m_measurementsSection.recentList);
    }
}

void WelcomeScreen::TrimListToFit(QListWidget *list)
{
    // The section's room sets how many entries it can show, never more than the limit. The limit is the
    // starting point, not a measurement: a viewport that cannot be measured (a hidden screen, a list just
    // rebuilt) used to leave every entry in place, and the list then grew past what the history is for.
    int maxVisible = kMaxRecentRows;

    const int itemHeight = list->sizeHintForRow(0);
    const int viewportHeight = list->viewport()->height();
    if (itemHeight > 0 && viewportHeight > 0)
    {
        maxVisible = std::min(maxVisible, viewportHeight / itemHeight);
    }

    maxVisible = std::max(maxVisible, 1); // A window too short for one row scrolls; it does not empty the list.

    while (list->count() > maxVisible)
    {
        delete list->takeItem(list->count() - 1);
    }
}
