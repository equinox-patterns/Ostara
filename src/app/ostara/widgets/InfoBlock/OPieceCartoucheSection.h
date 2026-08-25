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
#include <QString>
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
    Axis,         ///< QString — size-axis name from the loaded table
    MeasureName,  ///< QString — measurement full name from the loaded table
    Separator,    ///< QString — separator between name and value ("", ":", "-", "/", "·")
    Logo,         ///< QString — logo file name from the logo library
    LogoSize      ///< double — logo size in cm
};

/**
 * @brief Declarative description of one option of a section type.
 */
struct OPieceCartoucheOptionField
{
    QString key;
    OPieceCartoucheOptionKind kind{OPieceCartoucheOptionKind::FontFamily};
    QString label;
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
    OPieceCartoucheSection(const QString &id, const QString &displayName, const QString &description,
                           const QList<OPieceCartoucheOptionField> &fields);

    /// True when the section carries a non-empty id (i.e. it exists in the catalogue).
    auto IsValid() const -> bool;

    auto Id() const -> QString;
    auto DisplayName() const -> QString;
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
    QString m_displayName;
    QString m_description;
    QList<OPieceCartoucheOptionField> m_fields;
};

#endif // OPIECECARTOUCHESECTION_H
