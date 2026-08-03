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
#include <QFontDatabase>
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
/// Sets qApp palette from Ostara design tokens so that QStyle (Fusion) uses our
/// colors for elements not covered by QSS (popup frames, tooltip backgrounds,
/// scrollbar backgrounds, etc.).
void ApplyOstaraPalette(bool dark)
{
    QPalette p;

    if (dark)
    {
        // ── Dark palette (from palette.json dark semantique) ──
        p.setColor(QPalette::Window,          QColor(0x28, 0x28, 0x2C)); // Gray800
        p.setColor(QPalette::WindowText,      QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::Base,            QColor(0x28, 0x28, 0x2C)); // Gray800
        p.setColor(QPalette::AlternateBase,   QColor(0x28, 0x28, 0x2C)); // Gray800
        p.setColor(QPalette::Text,            QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::Button,          QColor(0x40, 0x40, 0x44)); // Gray700
        p.setColor(QPalette::ButtonText,      QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::ToolTipBase,     QColor(0x28, 0x28, 0x2C)); // Gray800
        p.setColor(QPalette::ToolTipText,     QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::Highlight,       QColor(0x98, 0x3E, 0x60)); // Accent
        p.setColor(QPalette::HighlightedText, QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::Light,           QColor(0x54, 0x54, 0x59)); // Gray600
        p.setColor(QPalette::Midlight,        QColor(0x40, 0x40, 0x44)); // Gray700
        p.setColor(QPalette::Mid,             QColor(0x54, 0x54, 0x59)); // Gray600
        p.setColor(QPalette::Dark,            QColor(0x71, 0x71, 0x7A)); // Gray500
        p.setColor(QPalette::Shadow,          QColor(0x1E, 0x1E, 0x22)); // Gray900
    }
    else
    {
        // ── Light palette (from palette.json light semantique) ──
        p.setColor(QPalette::Window,          QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::WindowText,      QColor(0x27, 0x27, 0x2A)); // Gray800
        p.setColor(QPalette::Base,            QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::AlternateBase,   QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::Text,            QColor(0x27, 0x27, 0x2A)); // Gray800
        p.setColor(QPalette::Button,          QColor(0xF4, 0xF4, 0xF5)); // Gray100
        p.setColor(QPalette::ButtonText,      QColor(0x27, 0x27, 0x2A)); // Gray800
        p.setColor(QPalette::ToolTipBase,     QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::ToolTipText,     QColor(0x27, 0x27, 0x2A)); // Gray800
        p.setColor(QPalette::Highlight,       QColor(0x8C, 0x45, 0x60)); // Accent
        p.setColor(QPalette::HighlightedText, QColor(0xFA, 0xFA, 0xFA)); // Gray50
        p.setColor(QPalette::Light,           QColor(0xF4, 0xF4, 0xF5)); // Gray100
        p.setColor(QPalette::Midlight,        QColor(0xE4, 0xE4, 0xE7)); // Gray200
        p.setColor(QPalette::Mid,             QColor(0xD4, 0xD4, 0xD8)); // Gray300
        p.setColor(QPalette::Dark,            QColor(0xA1, 0xA1, 0xAA)); // Gray400
        p.setColor(QPalette::Shadow,          QColor(0x72, 0x72, 0x79)); // Gray500
    }

    qApp->setPalette(p); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
}

//---------------------------------------------------------------------------------------------------------------------
void ActivateCustomLightTheme()
{
    if (QFile f(QStringLiteral(":/ostara/light.qss")); !f.exists())
    {
        qWarning() << "Ostara light theme not found, falling back to Eleven\n";
        if (QFile fallback(QStringLiteral(":/light/stylesheet.qss")); fallback.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream ts(&fallback);
            qApp->setStyleSheet(ts.readAll()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        }
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
            qWarning() << "Unable to open Ostara light theme\n";
        }
    }

    ApplyOstaraPalette(false);
}

//---------------------------------------------------------------------------------------------------------------------
void ActivateCustomDarkTheme()
{
    if (QFile f(QStringLiteral(":/ostara/dark.qss")); !f.exists())
    {
        qWarning() << "Ostara dark theme not found, falling back to Eleven\n";
        if (QFile fallback(QStringLiteral(":/dark/stylesheet.qss")); fallback.open(QFile::ReadOnly | QFile::Text))
        {
            QTextStream ts(&fallback);
            qApp->setStyleSheet(ts.readAll()); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        }
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
            qWarning() << "Unable to open Ostara dark theme\n";
        }
    }

    ApplyOstaraPalette(true);
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
void VTheme::InitFonts()
{
    // ── Inter — UI font (variable weight 100-900) ──
    if (int const id = QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/inter/Inter-VariableFont.ttf"));
        id != -1)
    {
        QStringList const families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty())
        {
            QFont interFont(families.first());
            interFont.setPixelSize(15);
            interFont.setWeight(QFont::Normal); // 400 — variable font, Normal is a safe default
            qApp->setFont(interFont);           // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)
        }
    }
    else
    {
        qWarning() << "VTheme::InitFonts — Failed to load Inter font";
    }

    // ── Phosphor — icon font (regular) ──
    if (int const id = QFontDatabase::addApplicationFont(
            QStringLiteral(":/fonts/phosphor/regular/Phosphor.ttf"));
        id == -1)
    {
        qWarning() << "VTheme::InitFonts — Failed to load Phosphor Regular font";
    }

    // ── Phosphor Fill — icon font (filled variant) ──
    if (int const id = QFontDatabase::addApplicationFont(
            QStringLiteral(":/fonts/phosphor/fill/Phosphor-Fill.ttf"));
        id == -1)
    {
        qWarning() << "VTheme::InitFonts — Failed to load Phosphor Fill font";
    }

    qDebug() << "VTheme::InitFonts — Inter + Phosphor loaded";
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
    InitFonts();
    SetIconTheme();
    InitThemeMode(); // qApp->setStyleSheet() + ApplyOstaraPalette()
    VSceneStylesheet::ResetStyles();

    if (auto *s = qApp->style())
    {
        s->polish(qApp);
    }

    emit Instance()->ThemeSettingsChanged();

    ApplyThemeToWindows();

    // qApp->setStyleSheet() pushes explicit palettes onto matching widgets.
    // After that, qApp->setPalette() has no effect on those widgets — they
    // already have their own palette and no longer inherit from the app.
    // Resetting every widget's palette to an empty one forces inheritance
    // from the application level, which was just updated by ApplyOstaraPalette().
    // See QWidget::setPalette() doc: "pass an empty palette to inherit from parent".
    const auto widgets = qApp->allWidgets();
    for (auto *w : widgets)
    {
        w->setPalette(QPalette());
    }

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
