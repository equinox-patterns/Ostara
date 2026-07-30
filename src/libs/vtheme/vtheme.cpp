/************************************************************************
 **
 **  @file   vtheme.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   17 7, 2023
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2023 Valentina project
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
#include "vtheme.h"
#include "vcommonsettings.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>
#include <QtDebug>
#include <QtGlobal>
#include <QtSvg/QSvgRenderer>

#include "vabstractapplication.h"
#include "vapplicationstyle.h"
#include "vscenestylesheet.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "compatibility.h"
#endif

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
void ActivateCustomLightTheme()
{
    if (QFile f(QStringLiteral(":/light/stylesheet.qss")); !f.exists())
    {
        qDebug() << "Unable to set stylesheet, file not found\n";
    }
    else
    {
        if (f.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream ts(&f);
            qApp->setStyleSheet(ts.readAll()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        }
        else
        {
            qDebug() << "Unable to set stylesheet, unable to open file\n";
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ActivateCustomDarkTheme()
{
    if (QFile f(QStringLiteral(":/dark/stylesheet.qss")); !f.exists())
    {
        qDebug() << "Unable to set stylesheet, file not found\n";
    }
    else
    {
        if (f.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream ts(&f);
            qApp->setStyleSheet(ts.readAll()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        }
        else
        {
            qDebug() << "Unable to set stylesheet, unable to open file\n";
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto GetResourceName(const QString &root, const QString &iconName, bool dark) -> QString
{
    return QStringLiteral(":/%1/%2/%3").arg(root, dark ? "dark"_L1 : "light"_L1, iconName);
}

//---------------------------------------------------------------------------------------------------------------------
auto GetPixmapResource(const QString &root, const QString &iconName, bool dark) -> QPixmap
{
    QString const resourceName = GetResourceName(root, iconName, dark);
    auto pixmap = QPixmap(resourceName);
    Q_ASSERT(!pixmap.isNull());
    return pixmap;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::Instance() -> VTheme *
{
    static VTheme *instance = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    if (instance == nullptr)
    {
        instance = new VTheme();
    }

    return instance;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::ColorSheme() -> VColorSheme
{
    VThemeMode const themeMode = VAbstractApplication::VApp()->Settings()->GetThemeMode();
    return (themeMode == VThemeMode::Dark) ? VColorSheme::Dark : VColorSheme::Light;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::DefaultThemeName() -> QString
{
    VColorSheme const colorScheme = ColorSheme();
    QString const themePrefix = (colorScheme == VColorSheme::Light ? QStringLiteral("Light") : QStringLiteral("Dark"));
    return QStringLiteral("Eleven-%1").arg(themePrefix);
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::InitApplicationStyle()
{
    if (QStyle *style = QStyleFactory::create(QStringLiteral("fusion")); style != nullptr)
    {
        style = new VApplicationStyle(style);
        QApplication::setStyle(style);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::SetIconTheme()
{
    QIcon::setThemeSearchPaths({u":/icons"_s});
    QIcon::setThemeName(DefaultThemeName());
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::SetToAutoTheme() const
{
    qApp->setStyleSheet(QString()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::InitThemeMode()
{
    VThemeMode const themeMode = VAbstractApplication::VApp()->Settings()->GetThemeMode();

    if (themeMode == VThemeMode::Light)
    {
        ActivateCustomLightTheme();
    }
    else
    {
        ActivateCustomDarkTheme();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::ThemeStylesheet() -> QString
{
    VThemeMode const themeMode = VAbstractApplication::VApp()->Settings()->GetThemeMode();
    return (themeMode == VThemeMode::Light) ? QStringLiteral("light") : QStringLiteral("dark");
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::ResetColorScheme()
{
    // No-op: System theme mode has been removed. Theme is controlled entirely by user preference.
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::ResetThemeSettings() const
{
    InitApplicationStyle();
    qApp->setStyleSheet(QString()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
    SetIconTheme();
    InitThemeMode();
    VSceneStylesheet::ResetStyles();

    if (auto *s = qApp->style())
    {
        qApp->setPalette(s->standardPalette());
        s->polish(qApp);
    }

    emit Instance()->ThemeSettingsChanged();

    ApplyThemeToWindows();

    for (auto *w : QApplication::topLevelWidgets())
    {
        QEvent event(QEvent::ThemeChange);
        QCoreApplication::sendEvent(w, &event);
        w->update();
    }

    for (auto *combo : qApp->findChildren<QComboBox *>())
    {
        if (auto *view = combo->view(); view != nullptr)
        {
            if (auto *s = view->style())
            {
                s->polish(view);
            }
            view->update();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VTheme::ApplyThemeToWindows() const
{
    for (auto *w : QApplication::topLevelWidgets())
    {
        if (auto *mw = qobject_cast<QMainWindow *>(w))
        {
            for (auto *dock : mw->findChildren<QDockWidget *>())
            {
                // Force Qt to use widget rendering instead of native QStyle rendering
                // so that the QSS background property is applied to the dock body.
                // Without a custom title bar, the native QStyle ignores QSS background
                // on docked QDockWidgets (macOS-specific behavior).
                if (dock->titleBarWidget() == nullptr)
                {
                    auto *bar = new QWidget();
                    bar->setObjectName(QStringLiteral("dockTitleBar"));
                    auto *layout = new QHBoxLayout(bar);
                    layout->setContentsMargins(4, 2, 4, 2);
                    auto *label = new QLabel(dock->windowTitle());
                    label->setObjectName(QStringLiteral("dockTitleLabel"));
                    layout->addWidget(label);
                    layout->addStretch();
                    dock->setTitleBarWidget(bar);
                }

                dock->setAutoFillBackground(true);
                dock->update();
            }

            if (auto *central = mw->centralWidget())
            {
                central->setContentsMargins(0, 0, 0, 0);
                if (auto *lay = central->layout())
                {
                    lay->setContentsMargins(0, 0, 0, 0);
                    lay->setSpacing(0);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::GetFallbackThemeIcon(const QString &iconName, QSize iconSize) -> QIcon
{
    const auto filePath = QStringLiteral(":/icons/%1/%2.svg").arg(DefaultThemeName(), iconName);

    QIcon icon;
    icon.addFile(filePath, iconSize, QIcon::Normal, QIcon::On);
    iconSize *= 2;
    icon.addFile(filePath, iconSize, QIcon::Normal, QIcon::On);
    return icon;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::GetIconResource(const QString &root, const QString &iconName) -> QIcon
{
    QIcon icon;
    bool const dark = (ColorSheme() == VColorSheme::Dark);
    QPixmap pixmap = ::GetPixmapResource(root, iconName, dark);
    icon.addPixmap(pixmap);
    if (dark)
    {
        // automatic disabled icon is no good for dark
        // paint transparent black to get disabled look
        QPainter p(&pixmap);
        p.fillRect(pixmap.rect(), QColor(48, 47, 47, 128));
        icon.addPixmap(pixmap, QIcon::Disabled);
    }
    return icon;
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::GetPixmapResource(const QString &root, const QString &iconName) -> QPixmap
{
    bool const dark = (ColorSheme() == VColorSheme::Dark);
    return ::GetPixmapResource(root, iconName, dark);
}

//---------------------------------------------------------------------------------------------------------------------
auto VTheme::GetResourceName(const QString &root, const QString &iconName) -> QString
{
    return ::GetResourceName(root, iconName, ColorSheme() == VColorSheme::Dark);
}

//---------------------------------------------------------------------------------------------------------------------
VTheme::VTheme(QObject *parent)
  : QObject(parent)
{
}
