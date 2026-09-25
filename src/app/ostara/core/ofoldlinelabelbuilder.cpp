/************************************************************************
 **
 **  @file   ofoldlinelabelbuilder.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  La légende générique d'une foldline, dans les langues du
 **          cartouche de la pièce.
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
#include "ofoldlinelabelbuilder.h"

#include <QCoreApplication>
#include <QStringList>

#include "../vmisc/def.h"
#include "othumbnailtextbuilder.h"
#include "otemplateseparatoroptions.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// Le texte générique d'une foldline, dans une langue. Contexte et source sont ceux de
/// `VFoldLine::DefaultFoldLineLabel()` : c'est la même phrase, et une seule entrée de catalogue la
/// porte — deux copies finiraient par se traduire différemment.
auto Translate(const QString &locale) -> QString
{
    static const char *const kContext = "FoldLine";
    static const char *const kSource = QT_TRANSLATE_NOOP("FoldLine", "Cut On Fold");

    if (locale.isEmpty())
    {
        return QCoreApplication::translate(kContext, kSource);
    }
    return TranslateToLocale(locale, kContext, kSource);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OFoldLineLabelBuilder::Caption(const VPiece &piece, VAbstractPattern *doc) -> QString
{
    const QStringList languages = OThumbnailTextBuilder::Languages(piece, doc);

    QStringList parts;
    parts.reserve(languages.size());
    for (const QString &locale : languages)
    {
        const QString text = Translate(locale);
        if (!text.isEmpty())
        {
            // Une langue que le catalogue ne sait pas dire ne laisse pas un trou dans la ligne : le
            // lecteur voit une langue de moins, jamais un séparateur qui ne sépare rien.
            parts.append(text);
        }
    }

    return parts.join(OTemplateSeparatorOptions::kLanguageJoin);
}
