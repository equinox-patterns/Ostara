/************************************************************************
 **
 **  @file   vtheme.h
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
#ifndef VTHEME_H
#define VTHEME_H

#include <QHash>
#include <QObject>
#include <QString>

enum class VColorSheme : std::uint8_t
{
    Light,
    Dark
};

class QStyle;
class QWidget;

class VTheme : public QObject
{
    Q_OBJECT // NOLINT

public:
    ~VTheme() override = default;

    static auto Instance() -> VTheme *;

    void SetToAutoTheme() const;
    void ResetThemeSettings();
    void ApplyThemeToWindows();

public:
    static auto GetIconResource(const QString &root, const QString &iconName) -> QIcon;
    static auto GetPixmapResource(const QString &root, const QString &iconName) -> QPixmap;
    static auto GetResourceName(const QString &root, const QString &iconName) -> QString;

    /// The icon of the active theme, from its base name: `:/icons/{light,dark}/<baseName>_{light,dark}.svg`.
    /// The one place that knows how a themed icon is named — the idiom was copied by hand in six files.
    static auto ThemedIcon(const QString &baseName) -> QIcon;

    static auto ColorSheme() -> VColorSheme;
    static auto DefaultThemeName() -> QString;
    static void InitApplicationStyle();
    static void InitFonts();
    static void SetIconTheme();
    static void InitThemeMode();
    static auto ThemeStylesheet() -> QString;

public slots:
    void ResetColorScheme();

signals:
    void ThemeSettingsChanged();

protected:
    auto eventFilter(QObject *obj, QEvent *event) -> bool override;

private:
    Q_DISABLE_COPY_MOVE(VTheme) // NOLINT

    explicit VTheme(QObject *parent = nullptr);

    QHash<QObject *, QColor> m_viewportColors;
};

#endif // VTHEME_H
