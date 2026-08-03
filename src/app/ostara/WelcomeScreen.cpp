/************************************************************************
 **
 **  @file   WelcomeScreen.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/08/2026
 **
 **  @brief  Welcome screen with three sections: Patterns, Measurements, Favorites.
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

#include <QApplication>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

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

    // Recent list label — styled via QSS (#welcomeRecentLabel)
    auto *recentLabel = new QLabel(QObject::tr("Récents :"));
    recentLabel->setObjectName(QStringLiteral("welcomeRecentLabel"));
    sectionLayout->addWidget(recentLabel);

    // Recent list — styled via QSS (#welcomeRecentList)
    s.recentList = new QListWidget();
    s.recentList->setObjectName(QStringLiteral("welcomeRecentList"));
    s.recentList->setMaximumHeight(120);
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
    m_subtitleLabel = new QLabel(tr("Application de création de patrons de couture"));
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

    // --- Patterns section ---
    m_patternsSection = CreateSection(this,
        tr("Patrons"),
        tr("+ Nouveau patron"),
        tr("Ouvrir un patron..."));

    QObject::connect(m_patternsSection.newBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::NewPatternClicked);
    QObject::connect(m_patternsSection.openBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::OpenPatternClicked);
    QObject::connect(m_patternsSection.recentList, &QListWidget::itemDoubleClicked,
                     this, [this](QListWidgetItem *item) {
                         emit OpenRecentPattern(item->data(Qt::UserRole).toString());
                     });

    sectionsLayout->addWidget(m_patternsSection.frame, 1);

    // --- Measurements section ---
    m_measurementsSection = CreateSection(this,
        tr("Tableaux de mesures"),
        tr("+ Nouveau tableau"),
        tr("Ouvrir un tableau..."));

    QObject::connect(m_measurementsSection.newBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::NewMeasurementClicked);
    QObject::connect(m_measurementsSection.openBtn, &QPushButton::clicked,
                     this, &WelcomeScreen::OpenMeasurementClicked);
    QObject::connect(m_measurementsSection.recentList, &QListWidget::itemDoubleClicked,
                     this, [this](QListWidgetItem *item) {
                         emit OpenRecentMeasurement(item->data(Qt::UserRole).toString());
                     });

    sectionsLayout->addWidget(m_measurementsSection.frame, 1);

    // --- Favorites section (placeholder) ---
    m_favoritesSection = CreateSection(this,
        tr("⭐ Favoris"),
        QString(),
        QString());
    // Hide buttons for favorites — read-only display for now
    m_favoritesSection.newBtn->hide();
    m_favoritesSection.openBtn->hide();

    auto *favPlaceholder = new QLabel(tr("Utilisez l'étoile à côté d'un fichier récent\n"
                                          "pour l'ajouter aux favoris."));
    favPlaceholder->setWordWrap(true);
    favPlaceholder->setObjectName(QStringLiteral("welcomeFavPlaceholder"));
    auto *favLayout = qobject_cast<QVBoxLayout *>(m_favoritesSection.frame->layout());
    favLayout->insertWidget(2, favPlaceholder);

    sectionsLayout->addWidget(m_favoritesSection.frame, 1);

    mainLayout->addLayout(sectionsLayout, 1);

    // Features button — opens style guide window (dev-only)
    auto *featuresBtn = new QPushButton(tr("🧪 Features"));
    featuresBtn->setObjectName(QStringLiteral("welcomeNewBtn"));
    connect(featuresBtn, &QPushButton::clicked,
            this, &WelcomeScreen::FeaturesClicked);
    mainLayout->addWidget(featuresBtn, 0, Qt::AlignCenter);
}

void WelcomeScreen::RetranslateUI()
{
    m_subtitleLabel->setText(tr("Application de création de patrons de couture"));

    m_patternsSection.title->setText(tr("Patrons"));
    m_patternsSection.newBtn->setText(tr("+ Nouveau patron"));
    m_patternsSection.openBtn->setText(tr("Ouvrir un patron..."));

    m_measurementsSection.title->setText(tr("Tableaux de mesures"));
    m_measurementsSection.newBtn->setText(tr("+ Nouveau tableau"));
    m_measurementsSection.openBtn->setText(tr("Ouvrir un tableau..."));

    m_favoritesSection.title->setText(tr("⭐ Favoris"));
}

void WelcomeScreen::UpdateRecentItems(const QStringList &patternPaths, const QStringList &measurementPaths)
{
    // Patterns
    m_patternsSection.recentList->clear();
    for (const QString &path : patternPaths)
    {
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(fi.fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        m_patternsSection.recentList->addItem(item);
    }

    // Measurements
    m_measurementsSection.recentList->clear();
    for (const QString &path : measurementPaths)
    {
        QFileInfo fi(path);
        auto *item = new QListWidgetItem(fi.fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        m_measurementsSection.recentList->addItem(item);
    }
}
