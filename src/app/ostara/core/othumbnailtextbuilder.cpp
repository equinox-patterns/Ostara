/************************************************************************
 **
 **  @file   othumbnailtextbuilder.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25 September, 2026
 **
 **  @brief  What the piece miniature says (see othumbnailtextbuilder.h).
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

#include "othumbnailtextbuilder.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../widgets/InfoBlock/OPieceCartoucheTemplate.h"
#include "olengthunits.h"
#include "xml/vabstractpattern.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QHash>

namespace
{
/// @brief How long a template's languages are trusted before the file is looked at again.
///
/// There is no version to watch, and the templates are files on disk: without a cache the builder
/// would re-scan the template folder on every geometry refresh — that is, on every frame of a drag.
/// Without a limit an edit in the editor would never reach the miniature. A second is the compromise:
/// a storm of refreshes reads nothing, and a change appears by the time the user looks at it.
constexpr qint64 kTemplateCacheMs = 1000;

struct CachedLanguages
{
    qint64 stamp{0};
    QStringList languages;
};

/// @brief The languages the miniature is written in.
///
/// **The cartouche template of the piece, not the interface.** A page is printed once and read in
/// as many languages as its template was given, and the miniature is a statement of that same page:
/// it must not end up in a language the cartouche beside it is not written in. The template is read
/// even when the piece's cartouche is switched off — the languages are a property of the template,
/// not of whether the block is shown.
///
/// With no template to read, the interface's own language is the fallback: never a language the user
/// does not read.
auto LanguagesFor(const IPieceInfo &owner, VAbstractPattern *doc) -> QStringList
{
    static QHash<QString, CachedLanguages> cache;

    QString templateName = owner.GetInfoBlockData().IsSet() ? owner.GetInfoBlockData().Template() : QString();
    if (templateName.isEmpty() && doc != nullptr)
    {
        templateName = doc->GetCartoucheTemplate();
    }
    if (templateName.isEmpty())
    {
        templateName = VAbstractValApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    }
    if (templateName.isEmpty())
    {
        templateName = QStringLiteral("Default");
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (const auto it = cache.constFind(templateName);
        it != cache.constEnd() && now - it->stamp < kTemplateCacheMs)
    {
        return it->languages;
    }

    QStringList languages;
    const QStringList paths = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    for (const QString &path : paths)
    {
        if (QFileInfo(path).completeBaseName() != templateName)
        {
            continue;
        }

        OPieceCartoucheTemplate tpl;
        QString error;
        if (OPieceCartoucheStorage::Load(path, &tpl, &error))
        {
            languages = tpl.Languages();
        }
        break;
    }

    if (languages.isEmpty())
    {
        languages = QStringList{DefaultUiLanguage()};
    }

    cache.insert(templateName, CachedLanguages{now, languages});
    return languages;
}

/// @brief One source string, in one language. An empty locale is the application's own language —
/// the same convention the cartouche resolver reads its words by.
auto Translate(const QString &locale, const char *source) -> QString
{
    if (locale.isEmpty())
    {
        return QCoreApplication::translate("OTemplateSection", source);
    }
    return TranslateToLocale(locale, "OTemplateSection", source);
}

/// @brief One text per stored value, in the same order — so that the nth value and the nth text are
/// the same margin.
auto Values(const OThumbnailData &data, bool centimeters, bool inches) -> QStringList
{
    QStringList texts;
    texts.reserve(data.Labels().size());

    for (const OThumbnailLabel &label : data.Labels())
    {
        texts.append(OThumbnailTextBuilder::ValueText(label.mm, centimeters, inches));
    }

    return texts;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailTextBuilder::ValueText(qreal mm, bool centimeters, bool inches) -> QString
{
    if (centimeters && inches)
    {
        // The unit is named once, in the legend, so the pair stands in parentheses: "1 (3/8)".
        return QStringLiteral("%1 (%2)").arg(OLengthUnits::Value(mm, Unit::Cm),
                                             OLengthUnits::Value(mm, Unit::Inch));
    }
    if (centimeters)
    {
        return OLengthUnits::Value(mm, Unit::Cm);
    }
    if (inches)
    {
        return OLengthUnits::Value(mm, Unit::Inch);
    }

    // No unit asked for: no value, and no legend either. An empty word is not a word, and the block
    // says nothing rather than saying something in a unit nobody chose.
    return QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailTextBuilder::Legend(const QStringList &languages, bool centimeters, bool inches) -> QString
{
    QStringList lines;
    for (const QString &locale : languages)
    {
        if (centimeters && inches)
        {
            lines.append(Translate(locale, QT_TRANSLATE_NOOP("OTemplateSection",
                                                            "Seam allowances in centimeters (in inches)")));
        }
        else if (centimeters)
        {
            lines.append(Translate(locale, QT_TRANSLATE_NOOP("OTemplateSection",
                                                            "Seam allowances in centimeters")));
        }
        else if (inches)
        {
            lines.append(Translate(locale, QT_TRANSLATE_NOOP("OTemplateSection", "Seam allowances in inches")));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailTextBuilder::Build(const VPiece &piece, const OThumbnailData &thumbnail,
                                  VAbstractPattern *doc) -> OThumbnailTexts
{
    const bool centimeters = thumbnail.ShowCentimeters();
    const bool inches = thumbnail.ShowInches();

    OThumbnailTexts texts;
    texts.values = Values(thumbnail, centimeters, inches);

    // **A legend with nothing to name is not a legend.** Values the user has not asked to print
    // leave nothing above the piece, and a sentence about the units of no values would be a caption
    // for an empty space.
    if ((centimeters || inches) && not thumbnail.Labels().isEmpty())
    {
        texts.legend = Legend(LanguagesFor(piece, doc), centimeters, inches);
    }
    return texts;
}
