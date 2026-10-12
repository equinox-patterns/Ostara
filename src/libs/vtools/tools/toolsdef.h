/************************************************************************
 **
 **  @file   toolsdef.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 11, 2020
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2020 Valentina project
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
#ifndef TOOLSDEF_H
#define TOOLSDEF_H

#include <QMetaType>
#include <QString>
#include <QUuid>

#include "../ifc/ifcdef.h"

class VGObject;
class VContainer;
class VPieceNode;

struct VRawGraduatedCurveOffset
{
    QString name{};
    QString formula{};
    QString description{};
};

struct SourceItem
{
    quint32 id{NULL_ID};
    QString name{};
    QString penStyle{TypeLineDefault};
    QString color{ColorDefault};
    QUuid recordId{QUuid::createUuid()};
};

Q_DECLARE_METATYPE(SourceItem)
Q_DECLARE_TYPEINFO(SourceItem, Q_MOVABLE_TYPE); // NOLINT

/** @brief Where a point stands in the ordered selection an operation was given: -1 when it is not there at all.
 *
 *  One answer for the pleats, the slash-and-spread and the batch move: the three ask the very same question of the
 *  very same list, and the whole rule is built on it (which objects travel, which side of a line is folded). */
inline auto SourceIndexOf(const QVector<SourceItem> &source, quint32 pointId) -> int
{
    for (int index = 0; index < source.size(); ++index)
    {
        if (source.at(index).id == pointId)
        {
            return index;
        }
    }

    return -1;
}

struct DestinationItem
{
    quint32 id{NULL_ID};
    qreal mx{1};
    qreal my{1};
    bool showLabel{false};
    QUuid recordId{};
};

Q_DECLARE_METATYPE(DestinationItem)
Q_DECLARE_TYPEINFO(DestinationItem, Q_MOVABLE_TYPE); // NOLINT

/**
 * @brief A straight segment a tool draws, described as the pair of points it registered in the container.
 *
 * @a point1 and @a point2 are given in the order the tool registered them through VContainer::AddLine — the
 * order the Line_ and AngleLine_ variables of the pattern are keyed on. The pen is carried per segment even
 * though every tool met so far uses one pen for all of its segments: a tool that draws two of them
 * differently must not have to change this interface.
 */
struct VDrawnSegment
{
    quint32 point1{NULL_ID};
    quint32 point2{NULL_ID};
    QString typeLine{TypeLineDefault};
    QString lineColor{ColorDefault};
};

Q_DECLARE_TYPEINFO(VDrawnSegment, Q_MOVABLE_TYPE); // NOLINT

enum class SegmentLabel : quint8
{
    Segment1,
    Segment2,
    Segment3,
    Segment4
};

// Default label position
constexpr qreal labelMX = ToPixel(4, Unit::Mm);
constexpr qreal labelMY = ToPixel(6.5, Unit::Mm);

auto SourceToObjects(const QVector<SourceItem> &source) -> QVector<quint32>;
auto NodesToObjects(const QVector<VPieceNode> &nodes) -> QVector<quint32>;

void FillDefSourceNames(QVector<SourceItem> &source, const VContainer *data, const QString &suffix);

/**
 * @brief Les suffixes qui nomment les copies — un seul endroit.
 *
 * Une copie s'appelle d'après son objet d'origine, plus le suffixe de l'outil qui l'a fabriquée. Le suffixe
 * commence par `__` et se termine par la marque de l'outil ; les six outils historiques y ajoutent l'indice de
 * génération (`__r1`, `__r2`), ce dont FillDefSourceNames se charge, les outils Ostara le portent dans leur
 * suffixe (`__pli2`, `__o3`).
 *
 * La casse distingue des outils différents : `__r` la rotation simple et `__R` le slash-and-spread, `__m` le
 * move simple et `__M` le move batch.
 *
 * Le pli porte `__mp` — `m` de move, `p` de pleat : c'est le move du pli. Il portait `__m` comme le move
 * simple, si bien qu'un move et un pli copiant le même point réclamaient le même nom. Le changement ne touche
 * aucun patron existant : un nom de copie est **écrit** dans `<source><item name="…">` et relu, et les outils
 * respectent un nom déjà présent — un vieux pli garde `C7__m3`, seuls les nouveaux reçoivent `__mp3`.
 */
namespace CopySuffix
{
inline const QString Rotation = QStringLiteral("__r");
inline const QString CloseDart = QStringLiteral("__c");
inline const QString Transfert = QStringLiteral("__t");
inline const QString FlipByLine = QStringLiteral("__fl");
inline const QString FlipByAxis = QStringLiteral("__fa");
inline const QString Move = QStringLiteral("__m");
inline const QString Pleat = QStringLiteral("__mp");
inline const QString PleatMark = QStringLiteral("__pli");
inline const QString PleatNotch = QStringLiteral("__cran");
inline const QString Spread = QStringLiteral("__R");
inline const QString MoveBatch = QStringLiteral("__M");
inline const QString DartCap = QStringLiteral("__cap");
inline const QString Notch = QStringLiteral("__cran");
inline const QString OffsetCurve = QStringLiteral("__o");
inline const QString SubCurveLeft = QStringLiteral("__ls");
inline const QString SubCurveRight = QStringLiteral("__rs");
} // namespace CopySuffix

/// Le nom de base d'une copie : celui de son origine — le nom d'un point, le « headless » d'une courbe.
auto CopyNameBase(const VContainer *data, quint32 sourceId) -> QString;

/// Ce nom plus le suffixe de l'outil, sans vérifier le patron : la marque seule, l'indice restant à l'appelant.
auto DefaultCopyName(const VContainer *data, quint32 sourceId, const QString &suffix) -> QString;

/// Ce nom plus le suffixe, rendu unique dans le patron — la règle complète.
auto CopyName(const VContainer *data, const QString &base, const QString &suffix) -> QString;

/// La règle complète, à partir de l'id de l'objet d'origine.
auto CopyNameOfSource(const VContainer *data, quint32 sourceId, const QString &suffix) -> QString;

auto GetDefSourceName(quint32 id, const VContainer *data, const QString &suffix, const QSet<QString> &occupiedNames)
    -> QString;

auto GetSourceItemName(const QString &name, quint32 id, const VContainer *data) -> QString;


auto ConfirmDeletion() -> int;

#endif // TOOLSDEF_H
