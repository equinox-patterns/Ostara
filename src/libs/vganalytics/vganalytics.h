/************************************************************************
 **
 **  @file   vganalytics.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26 6, 2023
 **
 **  @brief  Sanctions compliance — GeoIP country check.
 **          Google Analytics tracking removed (Ostara fork).
 **  @copyright
 **  Copyright (C) 2023-2024 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 01/08/2026.
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
#ifndef VGANALYTICS_H
#define VGANALYTICS_H

#include <QObject>
#include <functional>

class VGAnalytics : public QObject
{
    Q_OBJECT // NOLINT

public:
    /// Synchronous country code lookup (blocking network request).
    static auto CountryCode() -> QString;

    /// Asynchronous country code lookup. Calls @p callback with the
    /// two-letter country code, or "Unknown" on error / timeout.
    static void CheckCountryCodeAsync(std::function<void(const QString &)> callback);
};

#endif // VGANALYTICS_H
