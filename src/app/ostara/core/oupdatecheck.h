/************************************************************************
 **
 **  @file   oupdatecheck.h
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

#ifndef OUPDATECHECK_H
#define OUPDATECHECK_H

class QWidget;

/// Ce qu'une application dit à son utilisatrice quand une version plus récente existe.
///
/// **Elle publie un numéro, et rien d'autre.** Le fichier contient une seule ligne — « 1.2.0 » —,
/// sans phrase, sans lien public, sans binaire : la phrase est une constante de l'application, et
/// c'est **elle** qui est traduite. Sophie n'écrit donc jamais de texte ; elle remplace un numéro.
///
/// ⚠️ **Un fichier par application.** Ostara et Rabnest ne sortent pas ensemble, et un numéro
/// commun annoncerait à une cliente une version qui n'existe pas pour elle.
///
/// ⚠️ **Silencieux quand le fichier est injoignable** : hors ligne, derrière un proxy, ou le fichier
/// oublié. Une application ne réclame pas l'attention pour une nouvelle qu'elle n'a pas pu lire.
namespace OUpdateCheck
{
/// Lit le numéro du site et, s'il dépasse celui de cette application, montre la boîte.
///
/// @param parent la fenêtre qui porte la boîte — et qui la voit rester au premier plan
void Check(QWidget *parent);
}  // namespace OUpdateCheck

#endif  // OUPDATECHECK_H
