/************************************************************************
 **
 **  @file   osymbollibrary.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The symbols of the installation: the marks a legend is written
 **          with, and the words each is written as.
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

#include "osymbollibrary.h"

#include "osymbolshapes.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "vmisc/def.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QSaveFile>
#include <QUuid>

namespace
{
const QString kTagRoot = QStringLiteral("symbols");
const QString kTagLanguages = QStringLiteral("languages");
const QString kTagLanguage = QStringLiteral("language");
const QString kTagSymbol = QStringLiteral("symbol");
const QString kTagMark = QStringLiteral("mark");
const QString kTagText = QStringLiteral("text");
const QString kAttrCode = QStringLiteral("code");
const QString kAttrShape = QStringLiteral("shape");
const QString kAttrLocale = QStringLiteral("locale");
const QString kAttrPermanent = QStringLiteral("permanent");
const QString kAttrId = QStringLiteral("id");
const QString kValueFalse = QStringLiteral("false");

//---------------------------------------------------------------------------------------------------------------------
/// The two marks every pattern carries, said in every language the application knows.
auto SeededLibrary() -> OSymbolLibrary::Library
{
    OSymbolLibrary::Library library;
    library.languages = {DefaultUiLanguage()};

    const auto AddSymbol = [&library](OSymbolShapes::Family family, const char *source)
    {
        OSymbolShapes::Shape shape;
        shape.family = family;

        OSymbolLibrary::Entry entry;
        entry.shapes.append(shape);

        for (const QString &locale : AvailableUiLanguages())
        {
            // **The words are the application's own**, taken from the translation that already
            // carries them rather than written out here: a French installation seeds "Droit-fil"
            // because that is what the application calls it, and the day a translation is corrected
            // this follows it. A language without a translation falls back to the source words,
            // which is what TranslateToLocale answers for English.
            entry.texts.insert(locale, TranslateToLocale(locale, "DialogSeamAllowance", source));
        }

        library.entries.append(entry);
    };

    // The two no pattern is ever drawn without: a legend that had to be built before it could be
    // used would ask for work nobody has to do.
    AddSymbol(OSymbolShapes::Family::Grainline, "Grainline");
    AddSymbol(OSymbolShapes::Family::FoldLine, "Fold line");

    // A seeded symbol is an entry like any other, and a pattern may choose it — so it is given its
    // id here rather than at the first save, which would leave the very first session unable to
    // refer to the two symbols the library starts with.
    OSymbolLibrary::GiveIds(library);

    return library;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolLibrary::GiveIds(Library &library) -> bool
{
    bool filled = false;
    for (Entry &entry : library.entries)
    {
        if (entry.id.isEmpty())
        {
            // A uuid without its braces: the same shape the cartouche's clients are named with, so
            // an id in this application looks like an id wherever it is met.
            entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
            filled = true;
        }
    }
    return filled;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolLibrary::Directory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathSymbols();
}

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolLibrary::MetaPath() -> QString
{
    return Directory() + QLatin1Char('/') + QStringLiteral("symbols.xml");
}

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolLibrary::Load() -> Library
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
        // error worth a word: the two marks are handed back anyway and the page works.
        const Library seeded = SeededLibrary();
        Save(seeded);
        return seeded;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        // A malformed index costs the library and nothing else. The page opens empty rather than
        // refusing to open, and a symbol can be added again.
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

    for (QDomElement el = root.firstChildElement(kTagSymbol); !el.isNull();
         el = el.nextSiblingElement(kTagSymbol))
    {
        Entry entry;

        // An id the library does not carry is not an error: it was written before symbols had one,
        // and it is filled in below rather than refused here — the file belongs to the user, and a
        // symbol the application stopped reading would be a symbol that disappeared.
        entry.id = el.attribute(kAttrId);

        // Absent means permanent: an index written before a symbol could wait to be asked is one
        // whose symbols were all meant to be shown, and reading them as "not chosen" would empty
        // every legend written so far.
        entry.permanent = el.attribute(kAttrPermanent) != kValueFalse;

        for (QDomElement mark = el.firstChildElement(kTagMark); !mark.isNull();
             mark = mark.nextSiblingElement(kTagMark))
        {
            if (const std::optional<OSymbolShapes::Shape> shape =
                    OSymbolShapes::FromKey(mark.attribute(kAttrShape));
                shape.has_value())
            {
                entry.shapes.append(*shape);
            }
        }

        if (entry.shapes.isEmpty())
        {
            // An index written before a symbol could stand for several marks carries its one on the
            // entry itself. It is read all the same: the file belongs to the user, and a symbol that
            // lost its mark would be a symbol that disappeared.
            if (const std::optional<OSymbolShapes::Shape> shape =
                    OSymbolShapes::FromKey(el.attribute(kAttrShape));
                shape.has_value())
            {
                entry.shapes.append(*shape);
            }
        }

        if (entry.shapes.isEmpty())
        {
            // An entry that names no mark has nothing to draw and nothing to show: it is left out
            // rather than shown as a blank line the user could neither read nor repair.
            continue;
        }

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

    // **A library written before ids existed is given them, and the file is written back.** Without
    // this, every entry would be handed a fresh id each time the library was read, and a pattern
    // that had chosen one would be pointing at an id that no longer names anything — the choice
    // would survive in the file and mean nothing. The write is the one the first launch already
    // does, and its failure is tolerated the same way: the library still works, it simply cannot be
    // referred to until its folder can be written.
    if (GiveIds(library))
    {
        Save(library);
    }

    return library;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSymbolLibrary::Save(const Library &library, QString *error) -> bool
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
        QDomElement el = doc.createElement(kTagSymbol);
        el.setAttribute(kAttrId, entry.id);
        el.setAttribute(kAttrPermanent, entry.permanent ? QStringLiteral("true") : kValueFalse);

        for (const OSymbolShapes::Shape &shape : entry.shapes)
        {
            QDomElement mark = doc.createElement(kTagMark);
            mark.setAttribute(kAttrShape, OSymbolShapes::Key(shape));
            el.appendChild(mark);
        }

        for (auto text = entry.texts.constBegin(); text != entry.texts.constEnd(); ++text)
        {
            QDomElement textElement = doc.createElement(kTagText);
            textElement.setAttribute(kAttrLocale, text.key());
            textElement.appendChild(doc.createTextNode(text.value()));
            el.appendChild(textElement);
        }

        root.appendChild(el);
    }

    // The folder is the application's to keep, not the user's to remember — as with the logos, a
    // library that refused its first symbol because its folder had been deleted would be a dead end.
    if (!QDir().mkpath(Directory()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot create the symbols folder.");
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
