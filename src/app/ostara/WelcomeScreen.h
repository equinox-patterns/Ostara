/************************************************************************
 **
 **  @file   WelcomeScreen.h
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

#ifndef WELCOMESCREEN_H
#define WELCOMESCREEN_H

#include <QWidget>

class QFrame;
class QLabel;
class QListWidget;
class QPushButton;
class QResizeEvent;

class WelcomeScreen : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeScreen(QWidget *parent = nullptr);
    ~WelcomeScreen() override = default;

    void RetranslateUI();

    /// Update the recent file lists displayed in each section.
    void UpdateRecentItems(const QStringList &patternPaths, const QStringList &measurementPaths);

    struct Section
    {
        QFrame *frame{nullptr};
        QLabel *title{nullptr};
        QPushButton *newBtn{nullptr};
        QPushButton *openBtn{nullptr};
        QListWidget *recentList{nullptr};
    };

signals:
    void NewPatternClicked();
    void OpenPatternClicked();
    void NewMeasurementClicked();
    void OpenMeasurementClicked();
    void OpenRecentPattern(const QString &path);
    void OpenRecentMeasurement(const QString &path);
    void FeaturesClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void CreateHeader();
    void CreateSections();
    void TrimListToFit(QListWidget *list);

    // Header
    QLabel *m_titleLabel{nullptr};
    QLabel *m_versionLabel{nullptr};
    QLabel *m_subtitleLabel{nullptr};

    // Sections
    Section m_patternsSection;
    Section m_measurementsSection;
};

#endif // WELCOMESCREEN_H
