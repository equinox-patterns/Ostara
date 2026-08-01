/************************************************************************
 **
 **  @file   main.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "../fervor/fvupdater.h"
#include "../vpatterndb/vpiecenode.h"
#include "core/vapplication.h"
#include "mainwindow.h"
#include "WelcomeWindow.h"

#include <QFileDialog>
#include "vabstractapplication.h"

#include <QMessageBox> // For QT_REQUIRE_VERSION
#include <QTimer>

#ifdef Q_OS_WIN
#include <clocale>
#endif

#ifdef CRASH_REPORTING
#include "crashhandler.h"
#include "version.h"
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QDir>
#if defined(SHARED_ICU_DATA)
#include <unicode/putil.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;
#endif
#endif

#include "vmainbase.h"

//---------------------------------------------------------------------------------------------------------------------
auto main(int argc, char *argv[]) -> int
{
#ifdef Q_OS_WIN
    std::setlocale(LC_ALL, ".UTF8");
#endif

    Q_INIT_RESOURCE(cursor);     // NOLINT
    Q_INIT_RESOURCE(toolcursor); // NOLINT
    Q_INIT_RESOURCE(fonts);      // NOLINT
    Q_INIT_RESOURCE(icon);       // NOLINT
    Q_INIT_RESOURCE(schema);     // NOLINT
    Q_INIT_RESOURCE(flags);      // NOLINT
    Q_INIT_RESOURCE(icons);      // NOLINT
    Q_INIT_RESOURCE(toolicon);   // NOLINT
    Q_INIT_RESOURCE(breeze);     // NOLINT
    Q_INIT_RESOURCE(scenestyle); // NOLINT
    Q_INIT_RESOURCE(win_light_theme); // NOLINT
    Q_INIT_RESOURCE(win_dark_theme);  // NOLINT
    Q_INIT_RESOURCE(ostara_theme);    // NOLINT

#ifdef CRASH_REPORTING
    InitializeCrashpad(QStringLiteral(VER_PRODUCTNAME_STR));
#endif

#if defined(Q_OS_WIN)
    VAbstractApplication::WinAttachConsole();
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#if defined(SHARED_ICU_DATA)
#if defined(Q_OS_MACOS)
    const QString icuDataPath = QCoreApplication::applicationDirPath() + "/../Resources/icu"_L1;
    u_setDataDirectory(icuDataPath.toUtf8().constData());
#endif
#endif // defined(SHARED_ICU_DATA)
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

    // Need to internally move a node inside a piece main path
    REGISTER_META_TYPE_STREAM_OPERATORS(VPieceNode);
    // Need to internally move a node inside a custom seam allowance path
    REGISTER_META_TYPE_STREAM_OPERATORS(CustomSARecord);

#ifndef Q_OS_MAC // supports natively
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    VAbstractApplication::InitHighDpiScaling(argc, argv);
#endif
#endif // Q_OS_MAC

#ifdef Q_OS_MAC
#if MACOS_LAYER_BACKING_AFFECTED
    MacosEnableLayerBacking();
#endif // MACOS_LAYER_BACKING_AFFECTED
#endif // Q_OS_MAC

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const VXercesGuard xercesGuard;
#endif

    VApplication app(argc, argv);
    app.InitOptions();

    VAbstractApplication::LogScreenInfo();

    QT_REQUIRE_VERSION(argc, argv, "5.15.0") // clazy:exclude=qstring-arg,qstring-allocations NOLINT

    VApplication::setDesktopFileName(QStringLiteral("com.ostara.ostara"));

    // === MVC Controller — main.cpp orchestrates which view is shown ===

    auto createMainWindow = [](const QString &filePath = {}) {
        auto *mw = new MainWindow();
        mw->setAttribute(Qt::WA_DeleteOnClose);
        // When MainWindow closes → quit the app (no return to Welcome)
        QObject::connect(mw, &QObject::destroyed, []() { QApplication::quit(); });
        mw->show();
        if (!filePath.isEmpty())
        {
            mw->LoadPattern(filePath);
        }
        return mw;
    };

    auto showWelcome = [&]() {
        auto *welcome = new WelcomeWindow();
        welcome->setAttribute(Qt::WA_DeleteOnClose);

        // "Nouveau patron"
        QObject::connect(welcome, &WelcomeWindow::NewPatternRequested, [welcome, createMainWindow]() {
            createMainWindow();
            welcome->close();
        });

        // "Ouvrir un patron..."
        QObject::connect(welcome, &WelcomeWindow::OpenPatternRequested, [welcome, createMainWindow]() {
            const QString filter = QObject::tr("Fichiers patron (*.val *.ostara)");
            const QString path = QFileDialog::getOpenFileName(welcome, QObject::tr("Ouvrir un patron"),
                                                              QDir::homePath(), filter);
            if (path.isEmpty())
            {
                return;
            }
            createMainWindow(path);
            welcome->close();
        });

        // Double-clic sur un récent patron
        QObject::connect(welcome, &WelcomeWindow::OpenRecentPatternRequested,
                         [welcome, createMainWindow](const QString &path) {
            if (path.isEmpty())
            {
                return;
            }
            createMainWindow(path);
            welcome->close();
        });

        // Measurement — placeholder (pas encore implémenté)
        auto showMeasurementPlaceholder = [welcome]() {
            QMessageBox::information(welcome,
                QObject::tr("Fonctionnalité à venir"),
                QObject::tr("L'éditeur de tableaux de mesures sera disponible "
                            "dans une prochaine mise à jour."));
        };
        QObject::connect(welcome, &WelcomeWindow::NewMeasurementRequested,
                         welcome, showMeasurementPlaceholder);
        QObject::connect(welcome, &WelcomeWindow::OpenMeasurementRequested,
                         welcome, showMeasurementPlaceholder);
        QObject::connect(welcome, &WelcomeWindow::OpenRecentMeasurementRequested,
                         welcome, [showMeasurementPlaceholder](const QString &) {
            showMeasurementPlaceholder();
        });

        // Auto-updater
        if (VApplication::IsGUIMode() && VAbstractApplication::VApp()->Settings()->IsAutomaticallyCheckUpdates())
        {
            FvUpdater::sharedUpdater()->SetFeedURL(FvUpdater::CurrentFeedURL());
            FvUpdater::sharedUpdater()->SetMainWindow(welcome);
            FvUpdater::sharedUpdater()->CheckForUpdatesSilent();
        }

        welcome->show();
    };

    QApplication::setQuitOnLastWindowClosed(false);
#if !defined(Q_OS_MAC)
    VApplication::setWindowIcon(QIcon(":/icon/64x64/icon64x64.png"));
#endif // !defined(Q_OS_MAC)

    // === Go ===
    QTimer::singleShot(0, showWelcome);

    return VApplication::exec();
}
