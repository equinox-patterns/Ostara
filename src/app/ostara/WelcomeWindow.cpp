/************************************************************************
 **
 **  @file   WelcomeWindow.cpp
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

#include "WelcomeWindow.h"
#include "WelcomeScreen.h"
#include "vabstractvalapplication.h"
#include "vvalentinasettings.h"

#include <QCloseEvent>
#include <QGuiApplication>
#include <QScreen>

WelcomeWindow::WelcomeWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Ostara"));

    m_welcomeScreen = new WelcomeScreen(this);
    setCentralWidget(m_welcomeScreen);

    // Restore geometry from the shared key (written by MainWindow on last close,
    // or by WelcomeWindow's own closeEvent). Must happen AFTER setCentralWidget()
    // because restoreGeometry can change window state (fullscreen, minimized, etc.)
    // which Qt queries on internal widgets. On first launch, use "faux plein écran".
    if (VAbstractValApplication *va = VAbstractValApplication::VApp())
    {
        if (VValentinaSettings *s = va->ValentinaSettings())
        {
            const QByteArray geom = s->value(QStringLiteral("geometry")).toByteArray();
            if (!geom.isEmpty())
            {
                restoreGeometry(geom);
            }
            else
            {
                // First launch: ~90% width, ~85% height of primary screen
                if (QScreen *screen = QGuiApplication::primaryScreen())
                {
                    const QRect avail = screen->availableGeometry();
                    resize(qRound(avail.width() * 0.90), qRound(avail.height() * 0.85));
                }
            }
        }
    }

    // Forward all WelcomeScreen signals → WelcomeWindow signals
    // main.cpp (the controller) connects these to navigation lambdas
    connect(m_welcomeScreen, &WelcomeScreen::NewPatternClicked,
            this, &WelcomeWindow::NewPatternRequested);
    connect(m_welcomeScreen, &WelcomeScreen::OpenPatternClicked,
            this, &WelcomeWindow::OpenPatternRequested);
    connect(m_welcomeScreen, &WelcomeScreen::NewMeasurementClicked,
            this, &WelcomeWindow::NewMeasurementRequested);
    connect(m_welcomeScreen, &WelcomeScreen::OpenMeasurementClicked,
            this, &WelcomeWindow::OpenMeasurementRequested);
    connect(m_welcomeScreen, &WelcomeScreen::OpenRecentPattern,
            this, &WelcomeWindow::OpenRecentPatternRequested);
    connect(m_welcomeScreen, &WelcomeScreen::OpenRecentMeasurement,
            this, &WelcomeWindow::OpenRecentMeasurementRequested);
}

//---------------------------------------------------------------------------------------------------------------------
void WelcomeWindow::UpdateRecentItems(const QStringList &patternPaths,
                                       const QStringList &measurementPaths)
{
    m_welcomeScreen->UpdateRecentItems(patternPaths, measurementPaths);
}

//---------------------------------------------------------------------------------------------------------------------
void WelcomeWindow::closeEvent(QCloseEvent *event)
{
    // Save geometry to the SHARED key so MainWindow inherits the same size/position.
    if (VAbstractValApplication *va = VAbstractValApplication::VApp())
    {
        if (VValentinaSettings *s = va->ValentinaSettings())
        {
            s->setValue(QStringLiteral("geometry"), saveGeometry());
            s->sync();
        }
    }
    QMainWindow::closeEvent(event);
}
