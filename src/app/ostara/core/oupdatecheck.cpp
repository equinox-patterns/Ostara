/************************************************************************
 **
 **  @file   oupdatecheck.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  L'annonce d'une nouvelle version : une ligne sur le site, lue une fois.
 **
 **  @copyright
 **
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

#include "oupdatecheck.h"

#include "../vmisc/projectversion.h"

#include "checkablemessagebox.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace
{
/// Le fichier d'Ostara. Une ligne, le numéro — et **son** fichier : Rabnest a le sien.
constexpr auto kVersionUrl = "https://sophiedenys.com/ostara.txt";

/// Le lien du message, et son texte. **La racine du site, jamais un fichier.** Sophie livre par son
/// propre canal : un lien public de binaire serait une porte ouverte à côté de sa boutique.
constexpr auto kSiteUrl = "https://sophiedenys.com";

/// Le temps qu'on accorde au réseau. Au-delà, on se tait : c'est une politesse, pas un service.
constexpr int kTimeoutMs = 5000;

/// Vrai quand @p remote est un numéro de version **plus grand** que @p local.
///
/// Comparaison terme à terme, numériquement : « 1.10.0 » est plus récent que « 1.9.2 », ce qu'une
/// comparaison de textes lirait à l'envers. Un fichier qui ne porte pas un numéro ne dit rien —
/// il vaut mieux se taire que d'annoncer n'importe quoi.
auto IsNewer(const QString &remote, const QString &local) -> bool
{
    const QStringList remoteParts = remote.split(QLatin1Char('.'));
    const QStringList localParts = local.split(QLatin1Char('.'));

    for (int index = 0; index < remoteParts.size(); ++index)
    {
        bool ok = false;
        const int remotePart = remoteParts.at(index).toInt(&ok);
        if (!ok)
        {
            return false;
        }

        const int localPart = index < localParts.size() ? localParts.at(index).toInt() : 0;
        if (remotePart != localPart)
        {
            return remotePart > localPart;
        }
    }

    return false;
}

/// Le texte de la boîte : la phrase, et le lien **en clair** dessous.
///
/// ⚠️ Le lien est écrit ici et **jamais dans la traduction** : une adresse recopiée dans dix-sept
/// catalogues est une adresse qui finira fausse dans l'un d'eux.
auto Message(const QString &version) -> QString
{
    const QString link = QStringLiteral("<a href=\"%1\">%2</a>")
                             .arg(QLatin1String(kSiteUrl),
                                  QUrl(QLatin1String(kSiteUrl)).host());

    return QCoreApplication::translate("OUpdateCheck", "Version %1 is available.\n\n%2")
        .arg(version, link);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void OUpdateCheck::Check(QWidget *parent)
{
    auto *manager = new QNetworkAccessManager();
    QNetworkRequest request{QUrl(QLatin1String(kVersionUrl))};
    // Le site redirige vers https://sophiedenys.com et l'inverse selon la page ; sans cette règle
    // une redirection rendrait la réponse vide, et l'application se tairait pour rien.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager->get(request);

    // ⚠️ **Un hors-ligne qui ne répond jamais laisserait le gestionnaire en vie.** Le délai ferme la
    // requête, la réponse revient en erreur, et le silence est le même que pour un fichier absent.
    auto *timeout = new QTimer(reply);
    timeout->setSingleShot(true);
    QObject::connect(timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeout->start(kTimeoutMs);

    QObject::connect(reply, &QNetworkReply::finished, reply,
                     [reply, manager, parent] {
                         const QNetworkReply::NetworkError error = reply->error();
                         const QString body = QString::fromUtf8(reply->readAll());

                         reply->deleteLater();
                         manager->deleteLater();

                         if (error != QNetworkReply::NoError)
                         {
                             return;
                         }

                         // La première ligne, et rien qu'elle : le fichier tient sur une ligne, et
                         // un éditeur qui en ajoute une ne doit pas changer la réponse.
                         const QString remote =
                             body.section(QLatin1Char('\n'), 0, 0).trimmed();
                         if (!IsNewer(remote, AppDisplayVersion()))
                         {
                             return;
                         }

                         // Une boîte PAR VERSION : la clé porte le numéro, donc une version plus
                         // récente repose la question et la même ne la repose jamais.
                         Utils::CheckableMessageBox::doNotShowAgainInformation(
                             parent, QCoreApplication::translate("OUpdateCheck", "Update"),
                             Message(remote), VAbstractApplication::VApp()->Settings(),
                             QStringLiteral("update/ostara-%1").arg(remote),
                             QDialogButtonBox::Ok);
                     });
}
