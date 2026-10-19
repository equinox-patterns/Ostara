/************************************************************************
 **
 **  @file   opiecemark.h
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

#ifndef OPIECEMARK_H
#define OPIECEMARK_H

#include <QDomElement>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

class VContainer;

/**
 * @brief The names a piece declares, and the mark that keeps them apart from the ones the pattern already holds.
 *
 * Two blocks cut from two patterns can agree on a name — `Em`, `Pro`, `Line_Em_Pro` — and a container holds one
 * object per name: the second piece would take the place of the first, silently. So a piece entering a pattern
 * whose names are already there is **marked**, and the mark is a suffix on **every** name it declares:
 * `Em__P1`. All of them or none: a piece half marked would have two namespaces and no coherence.
 *
 * `__P<n>` is a **mark, not a name**. It says which file the piece entered, so it is written when the piece
 * enters a file and **removed when the piece is saved back to the library** — a `.opiece` never carries one,
 * and the same piece imported twice into two files is marked from scratch each time. The number is drawn from
 * the file (`HighestNumber`, plus one), never from a counter of the session: once written, it never moves.
 *
 * The names are the ones the schema and the code agree on, and nothing else — a layer's label, a dialog's item,
 * a piece's own name are not names any object answers to. Because a formula cites an object by a **token built
 * on its name** (`Line_Em_Pro`, `AngleLine_Angle180`, `SplPath_devant`, `A14LSubCurve__c1`), renaming a name
 * means rewriting those tokens too, and this is one operation, never two.
 *
 * The caller owes the order: the block is marked **before** it is inserted, and the mark is removed **after**
 * the piece's own line roles have been matched on its names — a role is matched by the pair of points it joins,
 * and those points are marked at that moment.
 */
namespace OPieceMark
{
/// The number a name's mark carries — the **last** one, since a name can carry several — and 0 when the name
/// carries none. A name that merely ends in `__P` and digits is read as marked: the mark has no other shape.
auto Number(const QString &name) -> int;

/// @p name with every mark removed: `Em__P1` is `Em`, and a name that carries none is itself.
auto Unmarked(const QString &name) -> QString;

/// @p name, marked: `Em` becomes `Em__P1`. A number below 1 marks nothing.
auto Marked(const QString &name, int number) -> QString;

/**
 * @brief The slots that declare one name, as `parent/element/@attribute` — read from the pattern schema.
 *
 * The **parent** is part of the slot because the same element name does not mean the same thing everywhere:
 * `item` under `source` is a point the operation places and is named; `item` under `rotations` is a line of a
 * dialog, which no object ever answers to. A slot the schema types `nameSuffix` is an alias — the whole family,
 * read rather than written by hand — and what it types otherwise is written here, because only the code knows
 * that a point's own name (`shortName`) and the technical name of a spline (`string`) are names of objects too.
 */
auto NameSlots() -> const QSet<QString> &;

/// The slots that declare **several** names in one attribute, space separated: the pair of points a line's role
/// joins. The one slot of the schema that is not a name of its own.
auto NameListSlots() -> const QSet<QString> &;

/// Every name @p root declares, in the order they are met, without repetition.
auto DeclaredNames(const QDomElement &root) -> QStringList;

/**
 * @brief De quoi un alias est le nom — la seule chose qu'une fenêtre a besoin de dire pour savoir de quoi on parle.
 */
enum class AliasOf : quint8
{
    Line,          ///< le rôle d'une ligne : `Line_<rôle>`, `AngleLine_<rôle>`.
    Curve,         ///< une spline, un chemin de spline, le segment qu'une coupe produit, un volant.
    Arc,           ///< un arc.
    EllipticalArc, ///< un arc elliptique.
};

/**
 * @brief One **alias** a piece carries: what it is called, what kind of object wears it, and where it is written.
 *
 * An alias is a **key the container answers to**, not a label: the container builds a head on it — `Spl_dos` for
 * the alias of a spline, `SplPath_Em_R1` for the name of a segment, `Line_Angle180` and `AngleLine_Angle180` for
 * the name of a line's role — and overwrites in silence a key that is already there. Two objects that agree on
 * one alias are **one** row, because a rename is indexed by the name, never by the object.
 *
 * An **alias** is a name that was given; the names the code composes out of the objects' own names are read
 * beside it and never asked about — see Aliases().
 */
struct Alias
{
    QString name{};       ///< la valeur écrite dans le fichier — le suffixe ; c'est elle que la fenêtre montre.
    AliasOf of{AliasOf::Curve}; ///< le genre d'objet qui la porte : `Ligne`, `Courbe`, `Arc`, `Arc elliptique`.
    QStringList writtenIn{};    ///< les slots du schéma qui l'écrivent, `parent/element/@attribute`.
    QStringList owners{};       ///< les objets qui la portent, nommés comme l'utilisateur les connaît (`R1`).
};

/// Every alias @p root carries, in the order they are met, **one row per distinct value** — whether a formula
/// cites it or not, and whether or not the object is used anywhere: what a piece entering a pattern decides is
/// the name it will wear, not what reads it.
///
/// An alias is a name **somebody gave**, never one the code **composed** out of the objects it names: `dos` for a
/// spline, `devant` for another, `Angle180` for a line whose pair of points is `A1 A3` — as opposed to `Em_R1`,
/// `A14LSubCurve` and `A11_A16__o2`, which are the names of segments and of a spline the tools built out of the
/// objects' own names, and which say nothing a dialog could ask about.
auto Aliases(const QDomElement &root) -> QVector<Alias>;

/**
 * @brief The line-role pairs @p root's block really joins, written `A1_A3`, in both orders.
 *
 * A role in `<lineNames>` is just a pair of point names: an old file can carry one whose line was deleted while
 * both points stayed. This is the piece's own answer — the block's tools, read as the tools read them: a `<line>`
 * joins its two points, and a point tool joins its base (or first, or second) point to the point it places. The
 * import dialog uses it to keep a line whose line exists and to drop the names of the lines that do not.
 */
auto BorneLineRoles(const QDomElement &root) -> QSet<QString>;

/**
 * @brief The names @p root declares that the code **composed**, and that a formula therefore may not cite.
 *
 * A name that was **given** — an `alias`, the role of a line once it is more than the pair it joins, the name of
 * a point, the name of a `<detail>` — can carry an identity, and follow its object from one file to another. A
 * name the code **composed** cannot: the segments a cut names (`curveName1`, `curve1Name2`…), the technical name
 * of a spline, the name an operation gives the point it places, its `suffix`, and the **derived** name of a line,
 * `A1_A3`, which nobody writes down and which the container answers all the same.
 */
auto ComposedNames(const QDomElement &root) -> QSet<QString>;

/**
 * @brief The token of @p expression that cites one of @p composed, empty when it cites none.
 *
 * The **longest** name wins, because that is the one the container reads when two of them end the same token —
 * a curve named `Em` and one named `A1_Em`.
 */
auto ComposedCited(const QString &expression, const QSet<QString> &composed) -> QString;

/// @brief L'endroit qui écrit un nom : l'élément lui-même, et le slot du schéma qui le porte.
struct Carrier
{
    QDomElement element{}; ///< l'élément à écrire — le `<point>` d'un segment, la `<spline>` d'un nom.
    QString slot{};        ///< le slot, `parent/element/@attribute`.
};

/// L'élément qui écrit chaque nom déclaré par @p root, dans l'ordre du fichier. Le premier gagne : deux objets
/// du même nom n'en font qu'un pour le conteneur, et c'est lui que la dialog nommerait.
auto Carriers(const QDomElement &root) -> QHash<QString, Carrier>;

/// De quoi un nom écrit dans @p slot est le nom : `Ligne` pour un rôle, `Arc` et `Arc elliptique` pour ce que le
/// schéma nomme ainsi, et `Courbe` pour tout le reste — le segment d'une coupe, un chemin de spline, un volant.
auto ObjectOf(const QString &slot) -> AliasOf;

/**
 * @brief Écrire `Curve_` là où une formule cite une courbe par `Spl_` ou `SplPath_`.
 *
 * Les trois têtes mènent au **même** objet : `Spl_dos`, `SplPath_dos` et `Curve_dos` sont la même spline. Le
 * produit n'en garde qu'une — celle que le lecteur de formules écrit lui-même — et le fichier se nettoie à
 * l'ouverture, sinon chacun corrige à la main des jetons que le patron sait réécrire.
 *
 * Seules les têtes **entières** sont touchées : `C2LengthSplPath_R1_Eb` et `Angle2SplPath_dos` portent leur
 * `SplPath_` au milieu d'une tête composée, qui n'est pas ce que l'utilisateur écrit.
 *
 * @return le nombre de jetons réécrits — zéro quand le fichier était déjà propre.
 */
auto NormaliseCurveHeads(const QDomElement &root) -> int;

/**
 * @brief Écrire l'alias là où une formule cite encore le **nom composé** de l'objet — à l'ouverture du fichier.
 *
 * Un alias est prioritaire : dès qu'un objet en porte un, c'est lui qu'une formule doit employer. Les fichiers
 * écrits avant que le renommage ne répare les citations gardent pourtant le nom que le patron compose sur les
 * points (`Courbe_R3_Ef`), et personne ne va les corriger à la main.
 *
 * La règle est celle du fichier, pour **tout** fichier : le **nom** d'un objet est écrit à côté de son alias —
 * `curveName2` à côté de `alias2`, `curve1Name1` à côté de `curve1Alias1`, `name` à côté de l'`alias` d'une
 * courbe ou d'un `item` — et c'est ce couple qui dit quoi réécrire. Le rôle d'une ligne se lit de même, la paire
 * de points à côté du rôle dans `<lineNames>`.
 *
 * Une **courbe entière** (spline, chemin, arc, volant) n'écrit son nom nulle part : le patron le compose sur ses
 * points à la lecture. Ce couple-là ne peut donc être lu qu'**après** la lecture, avec @p data — la seconde passe —
 * et son résultat demande une relecture pour que les outils le voient.
 *
 * @param data le conteneur du patron, quand la lecture est faite. Nul à la première passe.
 * @return le nombre de jetons réécrits — zéro quand le fichier était déjà à jour.
 */
auto UseAliases(const QDomElement &root, const VContainer *data = nullptr) -> int;

/// Whether @p taken already answers to @p name — the name itself, or the same name under one of the heads the
/// container builds on it (`Spl_devant` for the alias `devant`). The one reading of « déjà pris » in the product.
auto AnswersTo(const QSet<QString> &taken, const QString &name) -> bool;

/// The highest mark @p root carries, 0 when it carries none. The file's own answer to "which number next".
auto HighestNumber(const QDomElement &root) -> int;

/// True when the pattern the piece is entering already answers to one of the names @p root declares — the name
/// itself, or the same name under one of the heads the container builds on it (`Spl_devant` for the alias
/// `devant`, the area of a piece for a `<detail>`). Wider than "holds that name", because an alias of the pattern
/// is overwritten in silence by an alias of the piece, and a name is not what the pattern answers to.
auto Collides(const QDomElement &root, const QSet<QString> &taken) -> bool;

/// What a rename did: the names of the table that were really met, and the places rewritten.
struct Report
{
    int names = 0;   ///< Distinct names of the table the piece declares.
    int written = 0; ///< Name slots rewritten, one count per value.
};

/**
 * @brief The table that marks @p root with @p number: every name it declares, and the name it takes.
 *
 * **All or nothing, per piece.** The table is empty when @p root declares none of the names in @p taken: a
 * piece that collides with nothing keeps its names exactly as they are, and part of a fresh file it is the
 * first `Buste` that arrives as `Em`, never as `Em__P1`. It never marks a name per name.
 */
auto TableFor(const QDomElement &root, const QSet<QString> &taken, int number) -> QHash<QString, QString>;

/**
 * @brief Applies @p renamed to every name @p root declares, **and to the formulas that cite them**.
 *
 * A name is cited by tokens built on it, and the token is what the calculator reads: a name renamed without its
 * tokens leaves the piece pointing at objects that no longer exist. The two passes are therefore one, and the
 * same call serves both directions — the mark a piece takes when it enters a file, and the mark it loses when
 * it is saved.
 *
 * @param roleNames the roles of the piece @p root belongs to, when @p root is its `<draw>` alone: the names of
 *                  the roles are written in the piece's own `<lineNames>`, which a block does not carry, so the
 *                  walk meets no role slot — and `Line_<role>` would keep pointing at the pattern's own roles.
 */
auto Rename(QDomElement &root, const QHash<QString, QString> &renamed, const QStringList &roleNames = {}) -> Report;

/// Removes the mark of @p root: the names, and the formulas that cite them.
auto Unmark(QDomElement &root) -> Report;
} // namespace OPieceMark

#endif // OPIECEMARK_H
