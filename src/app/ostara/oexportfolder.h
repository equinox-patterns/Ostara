/************************************************************************
 **
 **  @file   oexportfolder.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Le dossier d'un export — celui qu'un dialogue propose, et celui
 **          qu'un export retient pour le suivant.
 **
 **  @copyright
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

#ifndef OEXPORTFOLDER_H
#define OEXPORTFOLDER_H

#include <QString>

struct OExportOutput;

/// @brief Le dossier que le prochain export d'un run proposera.
///
/// Trois sources, dans cet ordre, et **une seule définition pour tous les exports** — PDF, SVG de
/// plotter, image : le dossier de la GAMME quand le run en a une (les autres fichiers de ce plan y
/// sont déjà), sinon le dernier dossier d'export de l'application, sinon Documents.
///
/// ⚠️ Le réglage de l'application est celui des exports du mode Layout et du dock Size Ranges —
/// `paths/layoutExport`. Une destination d'export est une propriété de **l'utilisatrice**, pas d'un
/// patron : un patron partagé ne doit pas porter le chemin d'une machine, et c'est aussi pourquoi
/// rien de tout cela n'est écrit dans le `.ostara`.
auto ExportFolder(const OExportOutput &output) -> QString;

/// @brief Le dossier que l'**export en lot** proposera.
///
/// Le lot ne passe pas par `ExportFolder` : celui-ci propose d'abord le dossier de la **gamme**, et
/// un lot traverse plusieurs gammes — il n'y en a pas une dont le dossier vaudrait pour toutes. Ce
/// qui reste est donc le dernier dossier d'export de l'application, celui que chaque export
/// individuel vient d'écrire, et Documents quand il a disparu.
auto LastExportFolder() -> QString;

/// @brief Retient le dossier où un export vient d'écrire : c'est celui que le suivant proposera.
///
/// Sans ça, chaque export d'un run repartait de Documents — le dossier de la gamme étant vide pour
/// tout run en taille unique, plotter compris — et l'utilisatrice retapait le même chemin tous les
/// jours.
///
/// Un dossier disparu n'est pas retenu : le proposer au prochain export ne mènerait nulle part, et
/// `ExportFolder` le remplacerait de toute façon.
void RememberExportFolder(const QString &directory);

#endif // OEXPORTFOLDER_H
