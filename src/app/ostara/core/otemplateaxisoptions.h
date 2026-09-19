/************************************************************************
 **
 **  @file   otemplateaxisoptions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   19/09/2026
 **
 **  @brief  The axes an information shows, and the aliases it gives them.
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

#ifndef OTEMPLATEAXISOPTIONS_H
#define OTEMPLATEAXISOPTIONS_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

class QDomDocument;
class QDomElement;

/**
 * @brief The three values an information states when it shows size axes.
 *
 * **Shared by both families, and the same shape in both.** A cartouche states the sizes of the
 * piece it decorates; a cover page states the sizes of the sheet it heads. Neither invents what an
 * axis is — the .omt declares them, and a .omt declares three at most — so both ask the same three
 * questions: *which axes*, *what are they called in each language*, and *does every language get
 * its own line*. Answering them in one place is what makes the two editors the same editor.
 *
 * The keys are named here once, for the same reason the cartouche used to name them in its own
 * header: the control that writes them and the resolver that reads them must not come apart.
 */
namespace OTemplateAxisOptions
{
/// QStringList — the axes the information shows, in the order the table declares them.
inline const QString kAxes = QStringLiteral("axes");

/**
 * QVariantMap — axis name → locale → { "name": QString, "values": QVariantList }.
 *
 * **The axis is the outer key, and that is what a single-axis version lacked.** An alias belongs
 * to one axis: the same word can be the name of no two of them, and a value's place in its own
 * axis is what its alias is indexed by. A file written before an information could show more than
 * one axis stores the inner map alone; reading it back is the section reader's business, and it
 * produces this shape with the one axis the file named.
 */
inline const QString kRenames = QStringLiteral("axisRenames");

/// bool — write every display language on its own line, rather than all of them on one.
inline const QString kOneLine = QStringLiteral("axisOneLine");

/**
 * @brief The key a file written before @ref kAxes existed names its single axis under.
 *
 * Named here rather than spelled out where it is read, because three places read it — the control,
 * the serialisation and both resolvers — and a key that three readers spell by hand is a key that
 * one of them will eventually spell wrong. It is never written: saving a section turns it into the
 * list it always meant, and the older key disappears from the file by itself.
 */
inline const QString kLegacyAxis = QStringLiteral("axis");
} // namespace OTemplateAxisOptions

/**
 * @brief The aliases one display language gives one axis, and its values.
 *
 * An alias is an *alias*: an empty entry means « keep the original », so the table is never
 * modified by a display choice — a template that renames « Stature » to « Height » in English
 * leaves the .omt saying « Stature ».
 */
struct OAxisRename
{
    QString name;        ///< alias of the axis name; empty = keep the table's.
    QStringList values;  ///< aliases of the axis values, in table order; empty entry = keep the table's.
};

/// One axis, as the table declares it: the name to write, and the values it holds.
struct OAxisChoice
{
    QString name;
    QStringList values;
};

/// The aliases of one axis, by display language.
using OAxisRenames = QMap<QString, OAxisRename>;

/// Read one axis's aliases out of the option as it is stored.
Q_REQUIRED_RESULT auto ParseAxisRenames(const QVariant &var) -> OAxisRenames;

/// Write one axis's aliases back into the shape the option stores.
Q_REQUIRED_RESULT auto MakeAxisRenames(const OAxisRenames &renames) -> QVariant;

/**
 * @brief Read the whole option: what each axis is called, in each language.
 *
 * **Two shapes live in this key, and the older one is read rather than converted.** A file written
 * when an information could show one axis holds a single locale-keyed map, the axis being the one
 * that file named under its own key — that is `legacyAxis`, and an empty one means the file never
 * named an axis at all. Files written since hold the axis-keyed map described above. Reading both
 * is what lets a template written yesterday open today with its axis ticked and its aliases intact;
 * saving it writes the newer shape, so the older one disappears by itself.
 */
Q_REQUIRED_RESULT auto ParseAxisRenamesByAxis(const QVariant &var, const QString &legacyAxis)
    -> QMap<QString, OAxisRenames>;

/// Write the whole option back, axis by axis.
Q_REQUIRED_RESULT auto MakeAxisRenamesByAxis(const QMap<QString, OAxisRenames> &renames) -> QVariant;

/**
 * @brief Write one information's axes into its element: which, how named, on how many lines.
 *
 * **Extracted from the cartouche, which was the only family to have it.** A cover page states the
 * same three things about the same table, and a second copy of this writer would be a second place
 * for the file format to drift — which is exactly what happened to the logo rasteriser before it
 * was extracted. Both families call this from their own `AxisRename` branch.
 */
void WriteAxisOptions(QDomDocument &doc, QDomElement &sectionEl, const QHash<QString, QVariant> &options);

/**
 * @brief Read those three things back into one information's options.
 *
 * Reads the older single-axis shape as well: a section that names one axis under its own key, and
 * alias elements carrying no axis, describe the same thing as a list of one. It is not converted on
 * the way in — it is kept under the key it was written with, so that every reader finds the same
 * shape it always found — and saving writes the newer one.
 */
void ReadAxisOptions(const QDomElement &sectionEl, QHash<QString, QVariant> &options);

#endif // OTEMPLATEAXISOPTIONS_H
