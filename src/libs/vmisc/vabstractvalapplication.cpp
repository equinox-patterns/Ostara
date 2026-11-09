/************************************************************************
 **
 **  @file   vabstractvalapplication.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 10, 2020
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2020 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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
#include "vabstractvalapplication.h"

#include <QMainWindow>
#include <QStatusBar>
#include "customevents.h"
#include "vvalentinasettings.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTranslator>
#include <QWidget>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// The file QSettings writes for that organization and that application, inside a given folder.
[[maybe_unused]] auto SettingsFileIn(const QString &dir) -> QString
{
    return dir + QLatin1Char('/') + QCoreApplication::organizationName() + QLatin1Char('/')
           + QCoreApplication::applicationName() + QLatin1String(".ini");
}

//---------------------------------------------------------------------------------------------------------------------
// True when Qt can really write the settings file in that folder. QSaveFile is the atomic write
// QSettings itself relies on: if it fails here, the settings write will fail too, and silently.
[[maybe_unused]] auto CanPersistSettings(const QString &dir) -> bool
{
    const QString file = SettingsFileIn(dir);
    if (dir.isEmpty() || !QDir().mkpath(QFileInfo(file).absolutePath()))
    {
        return false;
    }

    QSaveFile probe(file + QLatin1String(".probe"));
    if (!probe.open(QIODevice::WriteOnly))
    {
        return false;
    }

    probe.write("probe");
    const bool written = probe.commit();
    QFile::remove(file + QLatin1String(".probe"));
    return written;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VAbstractValApplication::VAbstractValApplication(int &argc, char **argv)
  : VAbstractApplication(argc, argv)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractValApplication::toPixel(double val) const -> double
{
    return ToPixel(val, m_patternUnits);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractValApplication::fromPixel(double pix) const -> double
{
    return FromPixel(pix, m_patternUnits);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractValApplication::PostWarningMessage(const QString &message, QtMsgType severity) const
{
    QApplication::postEvent(mainWindow,
                            new WarningMessageEvent(VAbstractValApplication::ClearMessage(message), severity));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief OpenSettings get acsses to application settings.
 *
 * Because we can create object in constructor we open file separately.
 */
void VAbstractValApplication::OpenSettings()
{
#if defined(Q_OS_WIN)
    // Settings live in Documents, as they do in Valentina. That folder can refuse a new file
    // (OneDrive holding it in the cloud, a network share, a protected medium): QSettings would then
    // fail silently, and the application would lose its language, its theme and its recent files on
    // every start. Probe the folder before settling in it, and move to the application's own data
    // when it cannot take the file.
    const QString documentsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString appDataDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    QString settingsDir = documentsDir;

    if (!appDataDir.isEmpty() && QFile::exists(SettingsFileIn(appDataDir)))
    {
        // A fallback already happened once: stay there. Going back to Documents the day it accepts
        // writes again would move the settings to another file from one start to the next.
        settingsDir = appDataDir;
    }
    else if (documentsDir.isEmpty() || !CanPersistSettings(documentsDir))
    {
        qWarning() << "Settings folder" << documentsDir << "cannot take the settings file; using" << appDataDir
                   << "instead.";

        // Take the previous file along: changing folder must not cost the reader her settings.
        if (!appDataDir.isEmpty())
        {
            const QString legacy = SettingsFileIn(documentsDir);
            const QString target = SettingsFileIn(appDataDir);
            if (QFile::exists(legacy) && !QFile::exists(target)
                && QDir().mkpath(QFileInfo(target).absolutePath()))
            {
                QFile::copy(legacy, target);
            }
        }

        settingsDir = appDataDir;
    }

    if (!settingsDir.isEmpty())
    {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDir);
    }
#endif
    settings = new VValentinaSettings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(),
                                      QCoreApplication::applicationName(), this);
    settings->RemoveOversizedValues();
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractValApplication::ValentinaSettings() -> VValentinaSettings *
{
    SCASSERT(settings != nullptr)

    return qobject_cast<VValentinaSettings *>(settings);
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
void VAbstractValApplication::ShowStatusMessage(const QString &message, int timeoutMs)
{
    auto *main = qobject_cast<QMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
    if (main == nullptr || main->statusBar() == nullptr)
    {
        return;
    }
    main->statusBar()->showMessage(message, timeoutMs);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractValApplication::VApp() -> VAbstractValApplication *
{
    return qobject_cast<VAbstractValApplication *>(QCoreApplication::instance());
}
