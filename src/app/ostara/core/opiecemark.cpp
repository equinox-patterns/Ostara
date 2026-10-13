/************************************************************************
 **
 **  @file   opiecemark.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The `__P<n>` a piece's names carry when they would have taken the place of names the pattern
 **          already holds: which names it touches, and what they become.
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

#include "opiecemark.h"

#include "../vgeometry/vabstractcurve.h"
#include "../vpatterndb/vcontainer.h"
#include "ifcdef.h"
#include "undocommands/orewriteformulas.h"
#include "undocommands/renametoken.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

#include <algorithm>
#include <utility>

namespace
{
const QString kSchema = QStringLiteral("://schema/ostara/v1.0.0.xsd");

/// The mark itself. Two underscores, because a name of the schema may hold one: the derived `Line_A_B` is a
/// token of its own, and a mark that could be read as part of a pair would make the two indistinguishable.
const QString kMark = QStringLiteral("__P");

// ── The vocabulary of a declared name ───────────────────────────────────────────────────────────────────────

/**
 * @brief What a declared name is used for once it is written, which is what decides how a formula citing it is
 * rewritten.
 *
 * The four are not four kinds of name: they are four ways a token is built on one. A label is found **inside** a
 * token (`Line_A_B`, `SplPath_A_B`, `A14LSubCurve__c1`), an alias is a whole head the container answers to
 * (`SplPath_devant`, `Angle1SplPath_S1LSubCurve`), a role is the head of the two names the container builds for
 * a line (`Line_<role>`, `AngleLine_<role>`), and a piece is the head of the two areas it builds from the name of
 * a `<detail>` (`PieceArea_<name>`, `PieceSeamLineArea_<name>`, `VContainer::FillPiecesAreas`).
 */
enum class Kind : quint8
{
    Name,
    Alias,
    Role,
    Piece
};

/// One place a name is written, as `parent/element/@attribute`.
struct Slot
{
    QString key{};
    Kind kind{Kind::Name};
    bool list{false}; ///< The attribute holds several names, space separated.
};

/// The slots the schema does not type `nameSuffix` but that hold a name all the same.
///
/// The schema types the **alias family** that way — `point/@alias1`, `@alias2`, the six `curve*Name*`, the six
/// `curve*Alias*`, `source/item/@name` and `@alias`, `operation/@suffix`, the `@alias` of an arc, of an elliptical
/// arc and of a spline, and `lineNames/line/@name` — and every one of them is a name of an object, so the family
/// is read rather than written here. What it types otherwise is the **declared** name: a point's own name and the
/// three a three-point tool adds (`shortName`), the technical name of a spline, and the points an operation places
/// (`string`). `spline/@suffix` is typed `string` too and is deliberately absent: it is a fragment the reader
/// composes into a name once and writes back, so no object ever answers to it. Two of them are not labels but
/// names of their own: the name of a `<detail>` is the **name of a piece**, out of which the container builds the
/// two areas of that piece, and the name of an `<offset>` of a graduated curve is a **variable the reader
/// creates** (`VIncrement`). Neither is an increment of the table — an increment is never marked, the first
/// import wins — so nothing else keeps their names apart from the ones the pattern already holds.
auto HandSlots() -> const QVector<Slot> &
{
    static const QVector<Slot> table{
        {QStringLiteral("calculation/point/@name"), Kind::Name, false},
        {QStringLiteral("calculation/point/@name1"), Kind::Name, false},
        {QStringLiteral("calculation/point/@name2"), Kind::Name, false},
        {QStringLiteral("calculation/point/@name3"), Kind::Name, false},
        {QStringLiteral("calculation/spline/@name"), Kind::Alias, false},
        {QStringLiteral("pleatDisplay/item/@name"), Kind::Name, false},
        {QStringLiteral("rotationDisplay/item/@name"), Kind::Name, false},
        {QStringLiteral("flounceSpiral/item/@name"), Kind::Name, false},
        {QStringLiteral("flounceSpiral/item/@alias"), Kind::Alias, false},
        {QStringLiteral("details/detail/@name"), Kind::Piece, false},
        {QStringLiteral("spline/offset/@name"), Kind::Name, false},
    };
    return table;
}

/// The pair of points a role joins: the one slot that holds **several** names in one attribute. Its components
/// are labels of the block, so they are rewritten like any other label — the role itself is a `Name`.
const QString kRolePair = QStringLiteral("lineNames/line/@points");

/// What a name typed `nameSuffix` is used for. The attribute says it: an `alias` — and the `curve*Alias*` beside
/// it — is a head a curve answers to, where a `curve*Name*` is a label of the segment built on it. The name a
/// role carries is the one slot the attribute alone does not tell apart, and it is named here.
auto KindOf(const QString &key, const QString &attribute) -> Kind
{
    if (key == QStringLiteral("lineNames/line/@name"))
    {
        return Kind::Role;
    }

    if (key == QStringLiteral("details/detail/@name"))
    {
        return Kind::Piece;
    }

    // A `curveName` is the name of a segment, and the container answers to it under a curve head (`Spl_`,
    // `SplPath_`): it is an alias by nature, even where the schema calls it a name.
    if (attribute.startsWith(QStringLiteral("curve")))
    {
        return Kind::Alias;
    }

    return attribute.contains(QStringLiteral("alias"), Qt::CaseInsensitive) ? Kind::Alias : Kind::Name;
}

/// L'**attribut** qui porte le nom de l'objet dont @p attribute porte l'alias, tel que le fichier l'écrit à côté :
/// `curveName1` pour `alias1`, `curve1Name1` pour `curve1Alias1`. Le couple est la règle du fichier, et elle vaut
/// pour tout fichier : rien à deviner, rien à écrire à la main.
///
/// Vide pour un `alias` seul : il appartient à une courbe entière — dont le nom n'est pas dans le fichier — ou à
/// un point qu'une opération place, et l'alias d'un point n'est pas une tête de courbe. Ces deux-là passent par le
/// conteneur, qui seul sait de quel objet il s'agit.
auto PartnerAttribute(const QDomElement &element, const QString &attribute) -> QString
{
    if (attribute.startsWith(QStringLiteral("alias")))
    {
        // `alias1` n'a pas de `name1` à lui : le nom du segment est celui que la coupe compose, `curveName1`.
        const QString numbered = QStringLiteral("curveName") + attribute.mid(5);
        return element.hasAttribute(numbered) ? numbered : QString();
    }

    const qsizetype at = attribute.indexOf(QStringLiteral("alias"), 0, Qt::CaseInsensitive);
    if (at < 0)
    {
        return {};
    }

    QString partner = attribute;
    partner.replace(at, 5, attribute.at(at).isUpper() ? QStringLiteral("Name") : QStringLiteral("name"));
    return element.hasAttribute(partner) ? partner : QString();
}

/// Le nom qu'une **courbe entière** répond : le patron le compose sur ses points — `P1_P4` pour une spline,
/// `Premier_Dernier` pour un chemin, le centre et son id pour un arc — et le fichier ne l'écrit nulle part. Il
/// n'existe donc qu'après la lecture, dans le conteneur.
///
/// Vide quand l'objet de l'élément n'est pas une courbe : l'alias d'un point ou d'un item n'est pas une tête de
/// courbe, et la passe ne le touche pas.
auto PublishedName(const QDomElement &element, const VContainer *data) -> QString
{
    if (data == nullptr)
    {
        return {};
    }

    const QHash<quint32, QSharedPointer<VGObject>> *objects = data->CalculationGObjects();
    const QString id = element.attribute(QStringLiteral("id"));
    if (objects == nullptr || id.isEmpty())
    {
        return {};
    }

    const QSharedPointer<VAbstractCurve> curve
        = qSharedPointerDynamicCast<VAbstractCurve>(objects->value(id.toUInt()));
    return curve.isNull() ? QString() : curve->HeadlessName();
}

/// The type a declaration carries, without its prefix — the schema writes `xs:nameSuffix`.
auto DeclaredType(const QXmlStreamAttributes &attributes) -> QStringView
{
    const QStringView type = attributes.value(QStringLiteral("type"));
    const qsizetype colon = type.lastIndexOf(QLatin1Char(':'));
    return colon < 0 ? type : type.mid(colon + 1);
}

/**
 * @brief Reads the pattern schema: every attribute it types `nameSuffix`, which is the whole alias family.
 *
 * The key keeps the **parent** of the element, because the same element name does not mean the same thing
 * everywhere: an `item` under `source` is a point an operation places and is named, an `item` under `rotations`
 * is a line of a dialog that no object answers to. Every declaration of the schema sits inside the `<xs:element>`
 * that carries it, which is what lets the walk name a slot by its parent.
 */
auto ReadSchema() -> QHash<QString, Slot>
{
    QHash<QString, Slot> found;
    for (const Slot &slot : HandSlots())
    {
        found.insert(slot.key, slot);
    }

    found.insert(kRolePair, Slot{kRolePair, Kind::Name, true});

    QFile schema(kSchema);
    if (not schema.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "OPieceMark: the pattern schema could not be read:" << kSchema;
        return found;
    }

    QStringList owners;
    QXmlStreamReader reader(&schema);
    while (not reader.atEnd())
    {
        const QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::EndElement)
        {
            if (reader.name() == u"element" && not owners.isEmpty())
            {
                owners.removeLast();
            }
            continue;
        }

        if (token != QXmlStreamReader::StartElement)
        {
            continue;
        }

        if (reader.name() == u"element")
        {
            owners.append(reader.attributes().value(QStringLiteral("name")).toString());
            continue;
        }

        if (reader.name() != u"attribute" || owners.size() < 2)
        {
            continue;
        }

        const QXmlStreamAttributes attributes = reader.attributes();
        const QString name = attributes.value(QStringLiteral("name")).toString();
        if (name.isEmpty() || DeclaredType(attributes) != u"nameSuffix")
        {
            continue;
        }

        const QString key = owners.at(owners.size() - 2) + QStringLiteral("/") + owners.last()
                            + QStringLiteral("/@") + name;
        if (not found.contains(key))
        {
            found.insert(key, Slot{key, KindOf(key, name), false});
        }
    }

    if (reader.hasError())
    {
        qWarning() << "OPieceMark: the pattern schema could not be parsed:" << reader.errorString();
    }

    return found;
}

auto AllSlots() -> const QHash<QString, Slot> &
{
    static const QHash<QString, Slot> table = ReadSchema();
    return table;
}

/// The slot @p attribute of @p element is, and the parent the element sits under.
auto SlotOf(const QString &parentTag, const QString &tag, const QString &attribute) -> const Slot *
{
    static const QHash<QString, Slot> &table = AllSlots();
    const auto known = table.constFind(parentTag + QStringLiteral("/") + tag + QStringLiteral("/@") + attribute);
    return known == table.constEnd() ? nullptr : &known.value();
}

// ── Walking the names ────────────────────────────────────────────────────────────────────────────────────────

/// One name met by the walk: its value, the slot it fills, and the element that carries it.
struct Declared
{
    QString value{};
    const Slot *slot{nullptr};
    QDomElement element{};
};

/**
 * @brief Hands every declared name of @p element and its descendants to @p map, which returns what to write in
 * its place; a value it returns unchanged is left alone.
 *
 * @p element is a handle: what the walk writes goes into the document it points at, whether that is the block of
 * a piece or the envelope of a `.opiece`.
 */
void WalkNames(QDomElement element, const QString &parentTag, const std::function<QString(const Declared &)> &map)
{
    if (element.isNull())
    {
        return;
    }

    const QString tag = element.tagName();

    const QDomNamedNodeMap attributes = element.attributes();
    for (int i = 0; i < attributes.count(); ++i)
    {
        QDomAttr attribute = attributes.item(i).toAttr();
        if (attribute.isNull())
        {
            continue;
        }

        const Slot *slot = SlotOf(parentTag, tag, attribute.name());
        if (slot == nullptr)
        {
            continue;
        }

        const QString value = attribute.value();
        if (not slot->list)
        {
            const QString written = map(Declared{value, slot, element});
            if (written != value)
            {
                attribute.setValue(written);
            }
            continue;
        }

        const QStringList names = value.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QStringList written;
        written.reserve(names.size());
        for (const QString &name : names)
        {
            written.append(map(Declared{name, slot, element}));
        }

        const QString joined = written.join(QLatin1Char(' '));
        if (joined != value)
        {
            attribute.setValue(joined);
        }
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        WalkNames(child, tag, map);
    }
}

/// The name of the object that wears @p declared, as the pattern's own dialogs write it: the points a role joins,
/// the name of the point that carries the alias — and nothing when the slot **is** the name (an alias names its
/// own curve, and a `.opiece` gives a spline no other name).
auto OwnerOf(const Declared &declared) -> QString
{
    const QDomElement &element = declared.element;

    if (element.hasAttribute(QStringLiteral("points"))) // une ligne de `<lineNames>`
    {
        return element.attribute(QStringLiteral("points"));
    }

    if (declared.slot != nullptr && declared.slot->key.endsWith(QStringLiteral("/@name")))
    {
        return {};
    }

    return element.attribute(QStringLiteral("name"));
}

/// L'élément d'un slot `parent/element/@attribute` : le dernier mot avant l'attribut.
auto ElementOf(const QString &slot) -> QString
{
    const qsizetype attribute = slot.lastIndexOf(QLatin1Char('@'));
    QString before = attribute < 0 ? slot : slot.left(attribute);
    if (before.endsWith(QLatin1Char('/')))
    {
        before.chop(1);
    }

    return before.section(QLatin1Char('/'), -1);
}

/// De quoi le nom écrit dans @p slot est le nom. Le schéma dit l'élément, et ce que les outils produisent est une
/// courbe : le segment d'une coupe, le chemin d'une spline, le volant d'une opération.
auto ObjectOfSlot(const QString &slotKey) -> OPieceMark::AliasOf
{
    if (slotKey.startsWith(QStringLiteral("lineNames/")))
    {
        return OPieceMark::AliasOf::Line;
    }

    const QString element = ElementOf(slotKey);
    if (element == QStringLiteral("arc"))
    {
        return OPieceMark::AliasOf::Arc;
    }
    if (element == QStringLiteral("elArc"))
    {
        return OPieceMark::AliasOf::EllipticalArc;
    }

    return OPieceMark::AliasOf::Curve;
}

/// Whether the role @p declared carries is more than the pair of points it joins — the one name of the schema
/// whose default is composed rather than written: `A1_A3` is the line between `A1` and `A3`, `ProfondeurEmmanchure`
/// is a name somebody gave.
auto RoleGiven(const Declared &declared) -> bool
{
    const QStringList pair = declared.element.attribute(QStringLiteral("points"))
                                 .split(QLatin1Char(' '), Qt::SkipEmptyParts);
    return declared.value != pair.join(QLatin1Char('_'));
}

/// Whether the name @p declared carries is one somebody **gave as an alias** — what a dialog can ask about, and
/// what `Aliases()` is made of. A point's name and a `<detail>`'s name are given too, but they are not aliases:
/// see Composed(), which tells the names a formula may cite from the ones it may not.
auto Given(const Declared &declared) -> bool
{
    const Slot *slot = declared.slot;
    if (slot == nullptr)
    {
        return false;
    }

    const QString attribute = slot->key.section(QLatin1Char('@'), 1, 1);
    if (attribute.contains(QStringLiteral("alias"), Qt::CaseInsensitive))
    {
        return true;
    }

    return slot->kind == Kind::Role && RoleGiven(declared);
}

/// Whether the name @p declared carries is one the code **composed** — a name nobody gave, which therefore
/// carries no identity and cannot follow its object into a library piece. Everything else — an alias, the role of
/// a line that was named, the name of a point, the name of a `<detail>`, the name of a point an operation places,
/// the offset of a graduated curve — was given, and is cited as it stands.
auto Composed(const Declared &declared) -> bool
{
    const Slot *slot = declared.slot;
    if (slot == nullptr)
    {
        return false;
    }

    if (Given(declared) || slot->kind == Kind::Piece)
    {
        return false;
    }

    if (slot->kind == Kind::Role)
    {
        return true; // un rôle qui n'est que la paire qu'il joint : le conteneur le compose
    }

    static const QSet<QString> composedSlots{
        QStringLiteral("calculation/spline/@name"),
        QStringLiteral("source/item/@name"),
        QStringLiteral("operation/@suffix"),
    };
    if (composedSlots.contains(slot->key))
    {
        return true;
    }

    // Les noms des segments d'une coupe — `curveName1`, `curve1Name2`… — sont bâtis sur les points qu'elle joint.
    const QString attribute = slot->key.section(QLatin1Char('@'), 1, 1);
    return attribute.startsWith(QStringLiteral("curve")) && attribute.contains(QStringLiteral("Name"));
}

/// The names a point answers to: its own, and the three a three-point tool adds.
void CollectPointNames(const QDomElement &element, const QString &parentTag, QStringList &names)
{
    const QString tag = element.tagName();

    if (tag == QStringLiteral("point") && parentTag == QStringLiteral("calculation"))
    {
        static const QStringList attributes{QStringLiteral("name"), QStringLiteral("name1"), QStringLiteral("name2"),
                                            QStringLiteral("name3")};
        for (const QString &attribute : attributes)
        {
            const QString value = element.attribute(attribute);
            if (not value.isEmpty() && not names.contains(value))
            {
                names.append(value);
            }
        }
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        CollectPointNames(child, tag, names);
    }
}



// ── The mark ─────────────────────────────────────────────────────────────────────────────────────────────────

/// `__P<n>` at the very end of a name. A name may carry several marks — a piece cut from a marked piece — and
/// they are read from the last one backwards.
auto MarkPattern() -> const QRegularExpression &
{
    static const QRegularExpression pattern(QStringLiteral("__P(\\d+)$"));
    return pattern;
}

/// What the container builds on a name of its own: a line answers to `Line_<role>` and `AngleLine_<role>`, the
/// area of a piece to `PieceArea_<name>` and `PieceSeamLineArea_<name>`. The roles and the pieces are not curves,
/// so `ReplaceTokenAlias` does not know them.
auto HeadsOf(Kind kind) -> const QStringList &
{
    static const QStringList roles{line_, angleLine_};
    static const QStringList pieces{QStringLiteral("PieceArea_"), QStringLiteral("PieceSeamLineArea_")};
    static const QStringList none{};
    return kind == Kind::Role ? roles : (kind == Kind::Piece ? pieces : none);
}

/// The mark of a name, hidden behind a control character no name of the schema may hold.
///
/// A label is written **inside** a token, and a pass that rewrites a token once goes on with what it wrote: with
/// the mark left as it is, a name that is the tail of a mark — `P1` ends `__P1`, and the first mark the piece
/// takes is `__P1` — is found inside the mark the pass has just written, and marked in its turn
/// (`Line_Y__P1_Y1__P1__P1`). The mark a pass inserts is therefore hidden while the rest of the pass runs.
const QChar kHidden(0x1F);

auto HideMark(const QString &text) -> QString
{
    static const QRegularExpression mark(QStringLiteral("__P(\\d+)"));
    QString hidden = text;
    hidden.replace(mark, QStringLiteral("_%1\\1%1").arg(kHidden));
    return hidden;
}

auto ShowMark(const QString &text) -> QString
{
    static const QRegularExpression hidden(QStringLiteral("_%1(\\d+)%1").arg(kHidden));
    QString shown = text;
    shown.replace(hidden, QStringLiteral("__P\\1"));
    return shown;
}

/// @p oldName under the heads of @p kind, and the same name under them.
auto ReplaceHeadedToken(const QString &token, Kind kind, const QString &oldName, const QString &newName) -> QString
{
    for (const QString &head : HeadsOf(kind))
    {
        const QString before = head + oldName;
        if (token == before)
        {
            return head + newName;
        }

        // What is built on a name carries it and then its own part; the remainder has to begin with the
        // separator, so a role named `Angle` never reaches into the tokens of one named `Angle180`.
        if (token.startsWith(before) && token.size() > before.size() && token.at(before.size()) == u'_')
        {
            return head + newName + token.mid(before.size());
        }
    }

    return token;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Number(const QString &name) -> int
{
    int number = 0;
    QString rest = name;
    for (QRegularExpressionMatch match = MarkPattern().match(rest); match.hasMatch();
         match = MarkPattern().match(rest))
    {
        number = match.captured(1).toInt();
        rest.chop(match.captured(0).size());
    }

    return number;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Unmarked(const QString &name) -> QString
{
    QString bare = name;
    for (QRegularExpressionMatch match = MarkPattern().match(bare); match.hasMatch();
         match = MarkPattern().match(bare))
    {
        bare.chop(match.captured(0).size());
    }

    return bare;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Marked(const QString &name, int number) -> QString
{
    return number < 1 ? name : name + kMark + QString::number(number);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::NameSlots() -> const QSet<QString> &
{
    static const QSet<QString> keys = []() -> QSet<QString>
    {
        QSet<QString> keys;
        for (auto it = AllSlots().constBegin(); it != AllSlots().constEnd(); ++it)
        {
            if (not it.value().list)
            {
                keys.insert(it.key());
            }
        }
        return keys;
    }();

    return keys;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::NameListSlots() -> const QSet<QString> &
{
    static const QSet<QString> keys = []() -> QSet<QString>
    {
        QSet<QString> keys;
        for (auto it = AllSlots().constBegin(); it != AllSlots().constEnd(); ++it)
        {
            if (it.value().list)
            {
                keys.insert(it.key());
            }
        }
        return keys;
    }();

    return keys;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::DeclaredNames(const QDomElement &root) -> QStringList
{
    QStringList names;
    QSet<QString> seen;

    QDomElement element = root;
    WalkNames(element, QString(),
              [&names, &seen](const Declared &declared) -> QString
              {
                  if (not declared.value.isEmpty() && not seen.contains(declared.value))
                  {
                      seen.insert(declared.value);
                      names.append(declared.value);
                  }
                  return declared.value;
              });

    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Aliases(const QDomElement &root) -> QVector<Alias>
{
    QVector<Alias> aliases;
    QHash<QString, int> rowOf; // la valeur de l'alias → sa ligne

    QDomElement element = root;
    WalkNames(element, QString(),
              [&aliases, &rowOf](const Declared &declared) -> QString
              {
                  const Slot *slot = declared.slot;
                  const QString &value = declared.value;

                  // Un alias, et rien d'autre : le nom qu'on a **donné** à un objet — jamais celui que le code a
                  // composé à partir des noms qu'il joint (`A1_A3`, `Em_R1`, `A11_A16__o2`). Les noms d'objet,
                  // eux, la marque les départage sans rien demander à personne.
                  if (value.isEmpty() || not Given(declared))
                  {
                      return value;
                  }

                  const QString owner = OwnerOf(declared);

                  // Une ligne par alias, jamais par objet : deux objets qui portent le même nom n'en font qu'un,
                  // et une réécriture les touche tous les deux.
                  if (const auto known = rowOf.constFind(value); known != rowOf.constEnd())
                  {
                      Alias &row = aliases[known.value()];
                      if (not owner.isEmpty() && not row.owners.contains(owner))
                      {
                          row.owners.append(owner);
                      }
                      if (not row.writtenIn.contains(slot->key))
                      {
                          row.writtenIn.append(slot->key);
                      }
                      return value;
                  }

                  Alias row;
                  row.name = value;
                  row.of = ObjectOfSlot(slot->key);
                  row.writtenIn.append(slot->key);
                  if (not owner.isEmpty())
                  {
                      row.owners.append(owner);
                  }

                  rowOf.insert(value, static_cast<int>(aliases.size()));
                  aliases.append(row);
                  return value;
              });

    return aliases;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::ComposedNames(const QDomElement &root) -> QSet<QString>
{
    QSet<QString> composed;
    QSet<QString> given;

    QDomElement element = root;
    WalkNames(element, QString(),
              [&composed, &given](const Declared &declared) -> QString
              {
                  if (declared.value.isEmpty())
                  {
                      return declared.value;
                  }

                  if (Composed(declared))
                  {
                      composed.insert(declared.value);
                  }
                  else
                  {
                      given.insert(declared.value);
                  }
                  return declared.value;
              });

    // Les noms **dérivés** d'une ligne : deux noms de points joints, que personne n'écrit dans le fichier et que
    // le conteneur répond pourtant (`Line_A_B`, `AngleLine_A_B`). Sauf quand ce nom-là a été donné à autre chose.
    QStringList points;
    CollectPointNames(root, QString(), points);
    for (const QString &first : points)
    {
        for (const QString &second : points)
        {
            const QString derived = first + QLatin1Char('_') + second;
            if (not given.contains(derived))
            {
                composed.insert(derived);
            }
        }
    }

    return composed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::ComposedCited(const QString &expression, const QSet<QString> &composed) -> QString
{
    if (expression.isEmpty() || composed.isEmpty())
    {
        return {};
    }

    // Ce que le calculateur lit comme un nom : un jeton, et les noms qu'il porte sont ses fins de `_`. Le plus
    // long d'abord, parce que c'est celui que le conteneur lit quand deux noms finissent le même jeton.
    static const QRegularExpression identifier(QStringLiteral("[A-Za-z_@#][A-Za-z0-9_@#]*"));

    auto tokens = identifier.globalMatch(expression);
    while (tokens.hasNext())
    {
        QString candidate = tokens.next().captured();
        while (not candidate.isEmpty())
        {
            if (composed.contains(candidate))
            {
                return candidate;
            }

            const qsizetype cut = candidate.indexOf(QLatin1Char('_'));
            if (cut < 0)
            {
                break;
            }
            candidate = candidate.mid(cut + 1);
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Carriers(const QDomElement &root) -> QHash<QString, Carrier>
{
    QHash<QString, Carrier> carriers;

    QDomElement element = root;
    WalkNames(element, QString(),
              [&carriers](const Declared &declared) -> QString
              {
                  if (not declared.value.isEmpty() && not carriers.contains(declared.value))
                  {
                      carriers.insert(declared.value, Carrier{declared.element, declared.slot->key});
                  }
                  return declared.value;
              });

    return carriers;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::ObjectOf(const QString &slot) -> AliasOf
{
    return ObjectOfSlot(slot);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::NormaliseCurveHeads(const QDomElement &root) -> int
{
    int written = 0;

    // Les têtes d'une courbe : `Spl_`, `SplPath_` — seules, ou derrière l'un des six préfixes que la dialog
    // assistant formule emploie déjà en `Courbe` : la longueur d'une courbe (`C1Length`, `C2Length`), son angle
    // (`Angle1`, `Angle2`), la distance d'un point de contrôle (`Handle1`, `Handle2`). Le préfixe est ce qui dit
    // que le `Spl_` qui suit est une tête et non un morceau de nom donné par quelqu'un.
    // Les six préfixes des trois familles que la dialog assistant formule écrit déjà en `Courbe` : la longueur
    // d'une courbe (`C1Length`, `C2Length`), son angle (`Angle1`, `Angle2`), la distance d'un point de contrôle
    // (`Handle1`, `Handle2`) — décidé par Sophie le 13-10-2026. Mesuré pour les angles et les poignées
    // (`Angle2Curve_dos`, `Handle1Curve_dos`) ; les longueurs ne sont pas employées dans ses fichiers, mais la
    // règle les couvre comme les autres.
    static const QStringList prefixes{QString(),           QStringLiteral("C1Length"), QStringLiteral("C2Length"),
                                      QStringLiteral("Angle1"), QStringLiteral("Angle2"), QStringLiteral("Handle1"),
                                      QStringLiteral("Handle2")};

    QDomElement element = root;
    RewriteFormulas(element,
                    [&written](const QString &token) -> QString
                    {
                        for (const QString &prefix : prefixes)
                        {
                            if (not token.startsWith(prefix))
                            {
                                continue;
                            }

                            const QString rest = token.mid(prefix.size());
                            for (const QString &head : {QStringLiteral("SplPath_"), QStringLiteral("Spl_")})
                            {
                                if (rest.startsWith(head))
                                {
                                    ++written;
                                    return prefix + QStringLiteral("Curve_") + rest.mid(head.size());
                                }
                            }
                        }
                        return token;
                    });

    return written;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::HighestNumber(const QDomElement &root) -> int
{
    int highest = 0;
    for (const QString &name : DeclaredNames(root))
    {
        highest = std::max(highest, Number(name));
    }

    return highest;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::UseAliases(const QDomElement &root, const VContainer *data) -> int
{
    if (root.isNull())
    {
        return 0;
    }

    // Ce qu'une formule doit écrire quand l'objet porte un alias : le nom qu'elle emploie encore, et l'alias qui
    // prend sa place. Le nom est celui que le fichier écrit à côté de l'alias ; à défaut — une courbe entière — il
    // vient du conteneur, et la passe tourne alors après la lecture.
    QVector<QPair<QString, QString>> curves;
    QVector<QPair<QString, QString>> roles;

    QDomElement element = root;
    WalkNames(element, QString(),
              [&curves, &roles, data](const Declared &declared) -> QString
              {
                  const QString &key = declared.slot->key;

                  // Le rôle d'une ligne : la paire de points est ce qu'une formule écrit tant qu'il n'y a pas de
                  // rôle. `VChangeLineRoleName` la réécrit déjà au geste ; ici, c'est le fichier au repos.
                  if (key == kRolePair)
                  {
                      const QStringList points
                          = declared.element.attribute(QStringLiteral("points")).split(QLatin1Char(' '), Qt::SkipEmptyParts);
                      const QString role = declared.element.attribute(QStringLiteral("name"));
                      if (points.size() == 2 && not role.isEmpty())
                      {
                          const QPair<QString, QString> pair{points.at(0) + QLatin1Char('_') + points.at(1), role};
                          if (pair.first != pair.second && not roles.contains(pair))
                          {
                              roles.append(pair);
                          }
                      }

                      return declared.value; // Lecture seule : la passe ne touche pas aux noms déclarés.
                  }

                  if (declared.value.isEmpty())
                  {
                      return declared.value;
                  }

                  const QString attribute = key.section(QLatin1Char('@'), -1);
                  const QString partner = PartnerAttribute(declared.element, attribute);
                  // La **valeur** de l'attribut, pas son nom : c'est elle qu'une formule écrit après la tête.
                  QString name = partner.isEmpty() ? QString() : declared.element.attribute(partner);
                  if (name.isEmpty())
                  {
                      name = PublishedName(declared.element, data);
                  }

                  if (name.isEmpty() || name == declared.value)
                  {
                      return declared.value;
                  }

                  const QPair<QString, QString> pair{name, declared.value};
                  if (not curves.contains(pair))
                  {
                      curves.append(pair);
                  }

                  return declared.value;
              });

    if (curves.isEmpty() && roles.isEmpty())
    {
        return 0;
    }

    int written = 0;
    QDomElement envelope = root;
    RewriteFormulas(envelope,
                    [&curves, &roles, &written](const QString &token) -> QString
                    {
                        // La plus longue d'abord : deux noms peuvent finir le même jeton — `Em` et `Em_R1` — et
                        // c'est le nom entier que le conteneur lit.
                        for (const auto &[name, alias] : curves)
                        {
                            const QString mapped = ReplaceTokenAlias(token, CurveAliasType::All, name, alias);
                            if (mapped != token)
                            {
                                ++written;
                                return mapped;
                            }
                        }

                        for (const auto &[pair, role] : roles)
                        {
                            if (token == line_ + pair || token == angleLine_ + pair)
                            {
                                ++written;
                                return token.startsWith(angleLine_) ? angleLine_ + role : line_ + role;
                            }
                        }

                        return token;
                    });

    return written;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::AnswersTo(const QSet<QString> &taken, const QString &name) -> bool
{
    if (taken.isEmpty() || name.isEmpty())
    {
        return false;
    }

    // The pattern answers to a name **in the forms it built on it**, and a collision is what the container would
    // overwrite: the name itself under a point, or the same name under a head — `Spl_devant` for an alias, the
    // area of a piece for a `<detail>`, `Line_Em_Pro` for the label `Pro` of a line the pattern already holds.
    // Only the tail is looked for, which is what the heads have in common and all this needs to know about them.
    if (taken.contains(name))
    {
        return true;
    }

    const QString tail = QStringLiteral("_") + name;
    for (auto it = taken.constBegin(); it != taken.constEnd(); ++it)
    {
        if ((*it).endsWith(tail))
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Collides(const QDomElement &root, const QSet<QString> &taken) -> bool
{
    if (taken.isEmpty())
    {
        return false;
    }

    for (const QString &name : DeclaredNames(root))
    {
        if (AnswersTo(taken, name))
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::TableFor(const QDomElement &root, const QSet<QString> &taken, int number) -> QHash<QString, QString>
{
    QHash<QString, QString> renamed;
    if (root.isNull() || number < 1 || not Collides(root, taken))
    {
        return renamed;
    }

    // All or nothing: every name the piece declares is marked, not only the ones that were taken.
    for (const QString &name : DeclaredNames(root))
    {
        renamed.insert(name, Marked(name, number));
    }

    return renamed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Rename(QDomElement &root, const QHash<QString, QString> &renamed, const QStringList &roleNames)
    -> Report
{
    Report report;
    if (root.isNull() || renamed.isEmpty())
    {
        return report;
    }

    // A name is a token of its own, a label inside a token built on it, an alias or the name of a role or of a
    // piece: what the token becomes is decided by which of them it is, and the walk is what tells them apart.
    QVector<QPair<QString, QString>> labels;
    QVector<QPair<QString, QString>> aliases;
    QVector<QPair<QString, QString>> roles;
    QVector<QPair<QString, QString>> pieces;

    const auto Now = [&renamed, &report, &labels, &aliases, &roles,
                      &pieces](const Declared &declared) -> QString
    {
        const QString &value = declared.value;
        const auto known = renamed.constFind(value);
        if (known == renamed.constEnd())
        {
            return value;
        }

        ++report.written;
        switch (declared.slot->kind)
        {
            case Kind::Alias:
                aliases.append({value, known.value()});
                break;
            case Kind::Role:
                roles.append({value, known.value()});
                break;
            case Kind::Piece:
                pieces.append({value, known.value()});
                break;
            case Kind::Name:
                labels.append({value, known.value()});
                break;
        }

        return known.value();
    };

    QDomElement element = root;
    WalkNames(element, QString(), Now);

    // A block does not carry its piece's `<lineNames>`: the walk then meets no role slot, and the tokens a formula
    // builds on the role — `Line_<role>`, `AngleLine_<role>` — would keep reading the roles of the pattern. The
    // caller knows which names are roles, because it is holding the piece, so it says them.
    for (const QString &roleName : roleNames)
    {
        const auto known = renamed.constFind(roleName);
        if (known == renamed.constEnd() || known.value() == roleName)
        {
            continue;
        }

        const bool met = std::any_of(roles.cbegin(), roles.cend(),
                                     [&roleName](const QPair<QString, QString> &pair)
                                     { return pair.first == roleName; });
        if (not met)
        {
            roles.append({roleName, known.value()});
        }
    }

    report.names = static_cast<int>(labels.size() + aliases.size() + roles.size() + pieces.size());

    if (labels.isEmpty() && aliases.isEmpty() && roles.isEmpty() && pieces.isEmpty())
    {
        return report;
    }

    // The longest alias first: two of them can head the same token — a curve named `Em` and one named `Em_R1` —
    // and the whole name is the one the container answers to.
    std::sort(aliases.begin(), aliases.end(),
              [](const QPair<QString, QString> &left, const QPair<QString, QString> &right)
              { return left.first.size() > right.first.size(); });

    // The tokens, and the forms a formula can cite a name by. A name of the table read as it stands comes first —
    // the shortest answer, and the only one for an alias cited whole.
    const auto Token = [&renamed, &labels, &aliases, &roles, &pieces](const QString &token) -> QString
    {
        if (const auto whole = renamed.constFind(token); whole != renamed.constEnd())
        {
            return whole.value();
        }

        // What the container answers to by a **head** is read before the labels are, because a token built on a
        // label can read like one built on an alias: `SplPath_S1LSubCurve` is the segment curve `S1LSubCurve` and
        // also the path between the points `S1` and `LSubCurve`. The head that exists is the one to follow.
        for (const auto &[old, now] : aliases)
        {
            const QString mapped = ReplaceTokenAlias(token, CurveAliasType::All, old, now);
            if (mapped != token)
            {
                return mapped;
            }
        }
        for (const auto &[old, now] : roles)
        {
            const QString mapped = ReplaceHeadedToken(token, Kind::Role, old, now);
            if (mapped != token)
            {
                return mapped;
            }
        }
        for (const auto &[old, now] : pieces)
        {
            const QString mapped = ReplaceHeadedToken(token, Kind::Piece, old, now);
            if (mapped != token)
            {
                return mapped;
            }
        }

        // A label is found **inside** the token, and a pair of them needs both passes: `Line_Em_Pro` is rewritten
        // once for each of its two names.
        QString mapped = token;
        for (const auto &[old, now] : labels)
        {
            mapped = ReplaceTokenLabel(mapped, old, HideMark(now));
        }

        return ShowMark(mapped);
    };

    QDomElement envelope = root;
    RewriteFormulas(envelope, Token);

    return report;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceMark::Unmark(QDomElement &root) -> Report
{
    if (root.isNull())
    {
        return {};
    }

    // `__P<n>` is a mark, not a name: what is written into a `.opiece` is the name the piece had before it entered
    // any file. A name that carries several marks loses them all, so a piece that travelled keeps one namespace.
    QHash<QString, QString> renamed;
    for (const QString &name : DeclaredNames(root))
    {
        if (Number(name) > 0)
        {
            renamed.insert(name, Unmarked(name));
        }
    }

    return Rename(root, renamed);
}
