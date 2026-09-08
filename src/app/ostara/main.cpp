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
#include "../vpatterndb/oannotationdata.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtools/tools/vtoolseamallowance.h"

#include <QHash>
#include "core/vapplication.h"
#include "mainwindow.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"
#include "widgets/InfoBlock/OPieceCartoucheStorage.h"

#include "vabstractapplication.h"
#include "vvalentinasettings.h"
#include "oclickselectallfilter.h"

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
    InitializeCrashpad(QStringLiteral(VER_PRODUCTNAME_STR), QStringLiteral(VER_INTERNALNAME_STR));
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

    // Register the app-side information block content builder so the piece tools
    // (libs) can resolve real block content without a libs -> app dependency.
    VToolSeamAllowance::SetInfoBlockContentBuilder(
        [](const VPiece &piece, VAbstractPattern *doc, const VContainer *data) -> OInfoBlockEffectiveState {
            return OInfoBlockResolver::ResolveEffectiveState(piece, doc, data);
        });

    // Register the app-side annotation style resolver so the piece tools (libs)
    // can resolve the render style of an annotation without a libs -> app dependency.
    VToolSeamAllowance::SetAnnotationStyleResolver(
        [](const QString &name) -> OAnnotationRenderStyle {
            // Cache the library — the resolver runs per annotation on every
            // layout refresh; only a successful save bumps the version.
            static quint64 version = 0;
            static bool loaded = false;
            static QHash<QString, OAnnotationRenderStyle> cache;
            const quint64 current = OPieceCartoucheStorage::AnnotationStylesVersion();
            if (!loaded || current != version)
            {
                cache.clear();
                for (const OPieceCartoucheStorage::AnnotationStyleEntry &entry :
                     OPieceCartoucheStorage::LoadAnnotationStyles())
                {
                    OAnnotationRenderStyle style;
                    style.fontFamily = entry.fontFamily;
                    style.fontWeight = entry.fontWeight;
                    style.fontSize = entry.fontSize;
                    style.backgroundColor = entry.backgroundColor;
                    style.textColor = entry.textColor;
                    cache.insert(entry.name, style);
                }
                version = current;
                loaded = true;
            }
            return cache.value(name);  // Default / unknown → default font, transparent, black
        });

    // Select all text on a single click in every QLineEdit / QTextEdit / QPlainTextEdit.
    OClickSelectAllFilter::InstallOnApplication();

    VAbstractApplication::LogScreenInfo();

    QT_REQUIRE_VERSION(argc, argv, "5.15.0") // clazy:exclude=qstring-arg,qstring-allocations NOLINT

    VApplication::setDesktopFileName(QStringLiteral("com.ostara.ostara"));

    // --- Single-window bootstrap ---
    // MainWindow now hosts WelcomeScreen internally via a QStackedWidget.
    // No separate WelcomeWindow — navigation is handled by ShowWelcomeScreen()/ShowEditor().

    auto *mw = new MainWindow();
    mw->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(mw, &QObject::destroyed, []() { QApplication::quit(); });

    VAbstractValApplication::VApp()->setMainWindow(mw);

#if !defined(Q_OS_MAC)
    VApplication::setWindowIcon(QIcon(":/icon/64x64/icon64x64.png"));
#endif // !defined(Q_OS_MAC)

    if (VApplication::IsGUIMode())
    {
        // First launch of the installed software: seed the default cartouche template
        // (idempotent — skipped when the templates folder already holds a template).
        OPieceCartoucheStorage::EnsureDefaultTemplate();

        mw->show();

        // Auto-updater
        if (VAbstractApplication::VApp()->Settings()->IsAutomaticallyCheckUpdates())
        {
            FvUpdater::sharedUpdater()->SetFeedURL(FvUpdater::CurrentFeedURL());
            FvUpdater::sharedUpdater()->SetMainWindow(mw);
            FvUpdater::sharedUpdater()->CheckForUpdatesSilent();
        }

        // Process command-line args (crash restore, file opens)
        QTimer::singleShot(0, mw, &MainWindow::ProcessCMD);
    }
    else
    {
        mw->ProcessCMD(); // console/export path
    }

    return VApplication::exec();
}
