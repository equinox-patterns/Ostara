/************************************************************************
 **
 **  @file   WelcomeWindow.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/08/2026
 **
 **  @brief  Welcome window — lightweight QMainWindow shell hosting WelcomeScreen.
 **          Forwards all WelcomeScreen signals so main.cpp (the controller)
 **          can orchestrate navigation.
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

#ifndef WELCOMEWINDOW_H
#define WELCOMEWINDOW_H

#include <QMainWindow>

class QAction;
class QCloseEvent;
class QMenuBar;
class WelcomeScreen;

class WelcomeWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit WelcomeWindow(QWidget *parent = nullptr);
    ~WelcomeWindow() override = default;

    /// Populate the recent-file lists shown in each section of the welcome screen.
    void UpdateRecentItems(const QStringList &patternPaths,
                           const QStringList &measurementPaths);

signals:
    void NewPatternRequested();
    void OpenPatternRequested();
    void NewMeasurementRequested();
    void OpenMeasurementRequested();
    void OpenRecentPatternRequested(const QString &path);
    void OpenRecentMeasurementRequested(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    WelcomeScreen *m_welcomeScreen{nullptr};
    QMenuBar *m_appMenuBar{nullptr}; // standalone, no parent — deleted in closeEvent
};

#endif // WELCOMEWINDOW_H
