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
#include "dialogs/dialogaboutapp.h"
#include "vabstractvalapplication.h"
#include "vcommonsettings.h"
#include "vtheme.h"
#include "vvalentinasettings.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QGuiApplication>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
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

    // ── macOS Application Menu ──────────────────────────────────────
    // On macOS, the Application menu items (About, Preferences, Quit)
    // use the text of QActions with matching MenuRoles. Without these,
    // Qt falls back to its own default English strings.
    //
    // Per Qt docs, a macOS app with multiple windows must use a SINGLE
    // shared QMenuBar with NO parent (new QMenuBar(nullptr)). We do NOT
    // call setMenuBar() — the bar is standalone. It is explicitly deleted
    // in closeEvent so it doesn't interfere with MainWindow's menu bar.

    auto *mb = new QMenuBar(nullptr);
    auto *hiddenMenu = mb->addMenu(QString());

    auto *aboutQtAction = new QAction(this);
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    aboutQtAction->setText(
        QCoreApplication::translate("MainWindow", "About &Qt"));
    connect(aboutQtAction, &QAction::triggered, this, [this]() {
        QMessageBox::aboutQt(this);
    });
    hiddenMenu->addAction(aboutQtAction);

    auto *aboutAction = new QAction(this);
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setText(
        QCoreApplication::translate("MainWindow", "&About Valentina"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        auto *dlg = new DialogAboutApp(this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    });
    hiddenMenu->addAction(aboutAction);

    auto *prefsAction = new QAction(this);
    prefsAction->setMenuRole(QAction::PreferencesRole);
    // Dev-only: toggle between light and dark theme.
    // Will be removed before release.
    VThemeMode const currentMode =
        VAbstractApplication::VApp()->Settings()->GetThemeMode();
    prefsAction->setText(currentMode == VThemeMode::Light
        ? tr("Mode sombre")
        : tr("Mode clair"));
    prefsAction->setShortcut(QKeySequence::Preferences);
    connect(prefsAction, &QAction::triggered, this, [prefsAction]() {
        VCommonSettings *s = VAbstractApplication::VApp()->Settings();
        VThemeMode const newMode =
            (s->GetThemeMode() == VThemeMode::Light) ? VThemeMode::Dark : VThemeMode::Light;
        s->SetThemeMode(newMode);
        VTheme::InitThemeMode();
        if (auto *style = qApp->style())
        {
            qApp->setPalette(style->standardPalette());
            style->polish(qApp);
        }
        prefsAction->setText(newMode == VThemeMode::Light
            ? tr("Mode sombre")
            : tr("Mode clair"));
    });
    hiddenMenu->addAction(prefsAction);

    auto *quitAction = new QAction(this);
    quitAction->setMenuRole(QAction::QuitRole);
    quitAction->setText(
        QCoreApplication::translate("MainWindow", "E&xit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);
    hiddenMenu->addAction(quitAction);

    // Keep a raw pointer to delete in closeEvent — the QMenuBar has no
    // QObject parent so it won't be auto-deleted.
    m_appMenuBar = mb;
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

    // The standalone QMenuBar has no QObject parent — delete it now so it
    // doesn't interfere with MainWindow's menu bar after we close.
    delete m_appMenuBar;
    m_appMenuBar = nullptr;

    QMainWindow::closeEvent(event);
}
