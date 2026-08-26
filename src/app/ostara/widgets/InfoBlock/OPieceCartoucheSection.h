/************************************************************************
 **
 **  @file   OPieceCartoucheSection.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  One section type of the cartouche catalogue (catalogue entry).
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

#ifndef OPIECECARTOUCHESECTION_H
#define OPIECECARTOUCHESECTION_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVariant>

/**
 * @brief Kind of an option field — drives both serialization and the generated
 * editing control in the Information block editor.
 *
 * The editor is data-driven: it reads the fields of the selected section type
 * and builds one control per field. Adding a new kind means adding an entry
 * here plus one control builder in the editor — nothing else changes.
 */
enum class OPieceCartoucheOptionKind
{
    FontFamily,   ///< QString — font family
    FontWeight,   ///< int — QFont::Weight (0 = default)
    FontSize,     ///< int — font size in px (0 = default)
    LineSpacing,  ///< int — extra vertical space in px with the next line (0 = default, negative = tighter)
    Axis,         ///< QString — size-axis name from the loaded table
    MeasureName,  ///< QString — measurement full name from the loaded table
    Separator,    ///< QString — separator between name and value ("", ":", "-", "/", "·")
    Logo,         ///< QString — logo file name from the logo library
    LogoSize,     ///< double — logo size in cm
    Cut,          ///< composite — 3 tabs (fabric/interfacing/lining), qty + on fold
    Multilingual, ///< bool — write this information once per block language
    AxisRename    ///< composite — per-language display aliases of an axis + its values
};

/**
 * @brief Option keys of the Cut section — one section covers the three fabrics
 * (fabric / interfacing / lining); each filled tab becomes one label line.
 */
namespace OPieceCartoucheCut
{
inline const QString kFabricQty = QStringLiteral("fabricQty");
inline const QString kFabricOnFold = QStringLiteral("fabricOnFold");
inline const QString kInterfacingQty = QStringLiteral("interfacingQty");
inline const QString kInterfacingOnFold = QStringLiteral("interfacingOnFold");
inline const QString kLiningQty = QStringLiteral("liningQty");
inline const QString kLiningOnFold = QStringLiteral("liningOnFold");

inline const QStringList kQtyKeys = {kFabricQty, kInterfacingQty, kLiningQty};
inline const QStringList kFoldKeys = {kFabricOnFold, kInterfacingOnFold, kLiningOnFold};
inline const QStringList kAllKeys = {kFabricQty, kFabricOnFold, kInterfacingQty, kInterfacingOnFold,
                                     kLiningQty, kLiningOnFold};
}  // namespace OPieceCartoucheCut

/**
 * @brief Option keys of the axis-rename composite: per-language display aliases
 * of an axis and its values (never "alias" in the UI).
 */
namespace OPieceCartoucheAxisRename
{
/// QVariantMap: locale → { "name": QString, "values": QVariantList }. Empty
/// entries mean "keep the original table name" (no alias).
inline const QString kRenames = QStringLiteral("axisRenames");
/// bool — render every block language on its own line (default false).
inline const QString kOneLine = QStringLiteral("axisOneLine");
}  // namespace OPieceCartoucheAxisRename

/// Per-language display aliases of an axis and its values (empty = keep the
/// original table name).
struct OAxisRename
{
    QString name;        ///< alias of the axis name
    QStringList values;  ///< aliases of the axis values, in table order
};

/// Read the axis-rename option (a QVariantMap) into a locale-keyed map.
Q_REQUIRED_RESULT auto ParseAxisRenames(const QVariant &var) -> QMap<QString, OAxisRename>;

/// Serialize a locale-keyed map into the QVariant the option stores.
Q_REQUIRED_RESULT auto MakeAxisRenames(const QMap<QString, OAxisRename> &renames) -> QVariant;

/**
 * @brief Declarative description of one option of a section type.
 */
struct OPieceCartoucheOptionField
{
    QString key;
    OPieceCartoucheOptionKind kind{OPieceCartoucheOptionKind::FontFamily};
    QString label;
    QStringList values{};     ///< candidate values for the editor (e.g. separators); empty = kind default
    QVariant defaultValue{};  ///< per-field default value; invalid = kind default
};

/**
 * @brief One section type of the cartouche catalogue.
 *
 * A catalogue entry carries its identity (id, display name, description) and its
 * declarative option schema. The template model stores section *instances* which
 * reference a type by id and hold concrete option values.
 */
class OPieceCartoucheSection
{
public:
    OPieceCartoucheSection() = default;
    OPieceCartoucheSection(const QString &id, const QString &sourceName, const QString &displayName,
                           const QString &description, const QList<OPieceCartoucheOptionField> &fields);

    /// True when the section carries a non-empty id (i.e. it exists in the catalogue).
    auto IsValid() const -> bool;

    auto Id() const -> QString;
    auto DisplayName() const -> QString;

    /// The untranslated (English) name — lets the preview render the block in
    /// another display language through the app's own translation catalogues.
    auto SourceName() const -> QString;

    auto Description() const -> QString;
    auto Fields() const -> const QList<OPieceCartoucheOptionField> &;

    /// True when the option schema contains the given key.
    auto HasField(const QString &key) const -> bool;

    /// Default value for one option key (per its kind).
    auto DefaultValue(const QString &key) const -> QVariant;

    /// Map of every option key → its default value.
    auto DefaultOptions() const -> QHash<QString, QVariant>;

private:
    QString m_id;
    QString m_sourceName;
    QString m_displayName;
    QString m_description;
    QList<OPieceCartoucheOptionField> m_fields;
};

#endif // OPIECECARTOUCHESECTION_H
