/************************************************************************
 **
 **  @file   vganalytics.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26 6, 2023
 **
 **  @brief  Sanctions compliance — GeoIP country check.
 **          Google Analytics tracking removed (Ostara fork).
 **  @copyright
 **  Copyright (C) 2023-2024 Valentina project
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
#include "vganalytics.h"

#include <atomic>
#include <functional>
#include <memory>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

using namespace std::chrono_literals;

//---------------------------------------------------------------------------------------------------------------------
auto VGAnalytics::CountryCode() -> QString
{
    QNetworkAccessManager manager;
    QNetworkRequest const request(QUrl(QStringLiteral("https://api.country.is")));
    QNetworkReply *reply = manager.get(request);

    QTimer timer;
    timer.setSingleShot(true);
    timer.start(5s);

    QEventLoop eventLoop;

    QObject::connect(&timer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);

    eventLoop.exec();

    auto country = QStringLiteral("Unknown");

    if (timer.isActive())
    {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray const responseData = reply->readAll();
            QJsonParseError error;
            QJsonDocument const jsonDoc = QJsonDocument::fromJson(responseData, &error);

            if (error.error == QJsonParseError::NoError && jsonDoc.isObject())
            {
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj.contains(QStringLiteral("country")))
                {
                    country = jsonObj[QStringLiteral("country")].toString().toLower();
                }
            }
        }
    }
    else
    {
        reply->abort();
    }

    reply->deleteLater();

    return country;
}

//---------------------------------------------------------------------------------------------------------------------
void VGAnalytics::CheckCountryCodeAsync(std::function<void(const QString &)> callback)
{
    struct RequestState
    {
        QNetworkAccessManager *manager = nullptr;
        QNetworkReply *reply = nullptr;
        QTimer *timer = nullptr;
        std::function<void(const QString &)> callback{};
        std::atomic_bool done{false};
    };

    auto state = std::make_shared<RequestState>();
    state->manager = new QNetworkAccessManager;
    state->callback = std::move(callback);

    QNetworkRequest const request(QUrl(QStringLiteral("https://api.country.is")));
    state->reply = state->manager->get(request);

    state->timer = new QTimer;
    state->timer->setSingleShot(true);

    auto finish = [state](const QString &country) -> void
    {
        if (bool expected = false; !state->done.compare_exchange_strong(expected, true))
        {
            return;
        }

        state->timer->stop();
        state->timer->deleteLater();
        state->reply->deleteLater();
        state->manager->deleteLater();
        state->callback(country);
    };

    QObject::connect(state->timer,
                     &QTimer::timeout,
                     [state, finish]() -> void
                     {
                         state->reply->abort();
                         finish(QStringLiteral("Unknown"));
                     });

    QObject::connect(state->reply,
                     &QNetworkReply::finished,
                     [state, finish]() -> void
                     {
                         auto country = QStringLiteral("Unknown");
                         if (state->reply->error() == QNetworkReply::NoError)
                         {
                             const QByteArray responseData = state->reply->readAll();
                             QJsonParseError error;
                             if (const QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData, &error);
                                 error.error == QJsonParseError::NoError && jsonDoc.isObject())
                             {
                                 if (const QJsonObject jsonObj = jsonDoc.object();
                                     jsonObj.contains(QStringLiteral("country")))
                                 {
                                     country = jsonObj[QStringLiteral("country")].toString().toLower();
                                 }
                             }
                         }
                         finish(country);
                     });

    // Abort the pending request on app shutdown so the SSL thread can clean up before
    // QApplication destructs.
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, state->reply,
                     [state]() { state->reply->abort(); });

    state->timer->start(5s);
}
