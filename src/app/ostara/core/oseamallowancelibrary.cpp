/************************************************************************
 **
 **  @file   oseamallowancelibrary.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  The names a pattern's margins are written with on a cover page.
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

#include "oseamallowancelibrary.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QSaveFile>

namespace
{
const QString kTagRoot = QStringLiteral("seamAllowances");
const QString kTagLanguages = QStringLiteral("languages");
const QString kTagLanguage = QStringLiteral("language");
const QString kAttrCode = QStringLiteral("code");

const QString kTagMargin = QStringLiteral("margin");
const QString kTagText = QStringLiteral("text");
const QString kAttrLocale = QStringLiteral("locale");
const QString kAttrDefault = QStringLiteral("default");
const QString kAttrComparison = QStringLiteral("comparison");
const QString kAttrValue = QStringLiteral("value");

const QString kValueFalse = QStringLiteral("false");

/// The two ways a width can answer a condition, as they are written in the file. A stable token and
/// not a sign: `>=` in an attribute is punctuation an editor is free to rewrite, and what the file
/// means must not depend on that.
const QString kComparisonEqual = QStringLiteral("equal");
const QString kComparisonAtLeast = QStringLiteral("atLeast");

//---------------------------------------------------------------------------------------------------------------------
auto ComparisonToKey(OSeamAllowanceLibrary::Comparison comparison) -> QString
{
    return comparison == OSeamAllowanceLibrary::Comparison::AtLeast ? kComparisonAtLeast
                                                                   : kComparisonEqual;
}

//---------------------------------------------------------------------------------------------------------------------
auto ComparisonFromKey(const QString &key) -> OSeamAllowanceLibrary::Comparison
{
    return key == kComparisonAtLeast ? OSeamAllowanceLibrary::Comparison::AtLeast
                                     : OSeamAllowanceLibrary::Comparison::Equal;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The library as it stands the first time somebody needs one.
 *
 * **One entry, and it is the default.** Nothing else can be seeded: what a French seam is, or where
 * a hem starts, is the pattern maker's vocabulary and not the application's, and an application that
 * guessed would be putting words in a page the user then has to take out.
 *
 * The words are the application's own — borrowed from where the default seam allowance is set, and
 * translated into every language the application knows, so that the seeded name reads in the user's
 * language on the day it is written rather than after a translation pass.
 */
auto SeededLibrary() -> OSeamAllowanceLibrary::Library
{
    OSeamAllowanceLibrary::Library library;
    library.languages = {DefaultUiLanguage()};

    OSeamAllowanceLibrary::Entry entry;
    entry.byDefault = true;
    for (const QString &locale : AvailableUiLanguages())
    {
        entry.texts.insert(locale, TranslateToLocale(locale, "PreferencesPatternPage", "Seam allowance"));
    }

    library.entries.append(entry);
    return library;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::Steps(qreal mm) -> qint64
{
    return qRound64(mm * kComparisonSteps);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::Matches(const Entry &entry, qreal mm) -> bool
{
    // **Integers, not doubles.** Both sides are counted in hundredths of a millimetre, so the
    // comparison is between two whole numbers of them and cannot come out differently on two runs
    // that were handed the same length by two different routes.
    const qint64 value = Steps(mm);
    const qint64 limit = Steps(entry.condition.mm);

    switch (entry.condition.comparison)
    {
        case Comparison::Equal:
            return value == limit;
        case Comparison::AtLeast:
            return value >= limit;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::Rounded(qreal mm) -> qreal
{
    return static_cast<qreal>(Steps(mm)) / kComparisonSteps;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::DefaultIndex(const Library &library) -> int
{
    for (int i = 0; i < library.entries.size(); ++i)
    {
        if (library.entries.at(i).byDefault)
        {
            return i;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::Directory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathSeamAllowances();
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::MetaPath() -> QString
{
    return Directory() + QLatin1Char('/') + QStringLiteral("seamAllowances.xml");
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::Load() -> Library
{
    // A library always has a language, even before it is read: the page would have nothing to write
    // in otherwise, and every path below has to hand back something the user can work with.
    Library library;
    library.languages = {DefaultUiLanguage()};

    QFile file(MetaPath());
    if (!file.open(QIODevice::ReadOnly))
    {
        // Nothing on disk means nothing has ever been added, which is the first launch and the only
        // time the library is written for the user. A library that could not be written is not an
        // error worth a word: the default margin is handed back anyway and the page works.
        const Library seeded = SeededLibrary();
        Save(seeded);
        return seeded;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        // A malformed index costs the library and nothing else. The page opens empty rather than
        // refusing to open, and a margin can be added again.
        return library;
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != kTagRoot)
    {
        return library;
    }

    QStringList languages;
    const QDomElement languagesElement = root.firstChildElement(kTagLanguages);
    for (QDomElement el = languagesElement.firstChildElement(kTagLanguage); !el.isNull();
         el = el.nextSiblingElement(kTagLanguage))
    {
        const QString code = el.attribute(kAttrCode);
        if (!code.isEmpty() && !languages.contains(code))
        {
            languages.append(code);
        }
    }
    if (!languages.isEmpty())
    {
        library.languages = languages;
    }

    for (QDomElement el = root.firstChildElement(kTagMargin); !el.isNull();
         el = el.nextSiblingElement(kTagMargin))
    {
        Entry entry;
        entry.byDefault = el.attribute(kAttrDefault) == QStringLiteral("true");

        // The condition is read whatever the entry is: an entry that was the default yesterday and
        // is not today keeps the width it was given, rather than losing it to a flag.
        entry.condition.comparison = ComparisonFromKey(el.attribute(kAttrComparison));
        entry.condition.mm = el.attribute(kAttrValue).toDouble();

        for (QDomElement text = el.firstChildElement(kTagText); !text.isNull();
             text = text.nextSiblingElement(kTagText))
        {
            const QString locale = text.attribute(kAttrLocale);
            if (!locale.isEmpty())
            {
                entry.texts.insert(locale, text.text());
            }
        }

        library.entries.append(entry);
    }

    return library;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceLibrary::Save(const Library &library, QString *error) -> bool
{
    QDomDocument doc;
    QDomElement root = doc.createElement(kTagRoot);
    doc.appendChild(root);

    QDomElement languages = doc.createElement(kTagLanguages);
    for (const QString &code : library.languages)
    {
        QDomElement el = doc.createElement(kTagLanguage);
        el.setAttribute(kAttrCode, code);
        languages.appendChild(el);
    }
    root.appendChild(languages);

    for (const Entry &entry : library.entries)
    {
        QDomElement el = doc.createElement(kTagMargin);
        el.setAttribute(kAttrDefault, entry.byDefault ? QStringLiteral("true") : kValueFalse);
        el.setAttribute(kAttrComparison, ComparisonToKey(entry.condition.comparison));
        el.setAttribute(kAttrValue, QString::number(entry.condition.mm, 'f', 2));

        for (auto text = entry.texts.constBegin(); text != entry.texts.constEnd(); ++text)
        {
            QDomElement textElement = doc.createElement(kTagText);
            textElement.setAttribute(kAttrLocale, text.key());
            textElement.appendChild(doc.createTextNode(text.value()));
            el.appendChild(textElement);
        }

        root.appendChild(el);
    }

    // The folder is the application's to keep, not the user's to remember — as with the symbols, a
    // library that refused its first margin because its folder had been deleted would be a dead end.
    if (!QDir().mkpath(Directory()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot create the seam allowances folder.");
        }
        return false;
    }

    QSaveFile file(MetaPath());
    if (!file.open(QIODevice::WriteOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }

    file.write(doc.toByteArray(2));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }

    return true;
}
