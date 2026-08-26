/************************************************************************
 **
 **  @file   OPieceCartoucheTable.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/08/2026
 **
 **  @brief  Read-only facade over the pattern's loaded .omt size table.
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

#ifndef OPIECECARTOUCHETABLE_H
#define OPIECECARTOUCHETABLE_H

#include <QString>
#include <QStringList>

#include <memory>

class OOmtDocument;
class OOtuDocument;

/**
 * @brief Read access to the loaded measurement table (.omt or .otu) for the
 * cartouche template editor (Sizes and Measurement sections).
 *
 * The current pattern does not own a measurement document: this facade builds
 * one from the resolved table path on demand and caches it. A multi-size .omt
 * table also provides the Sizes (axis) data; a single-size .otu table only the
 * measurement values. A missing path or an unreadable file are treated as
 * "no table" (the table-driven sections then render nothing, spec §4.3).
 */
class OPieceCartoucheTable
{
public:
    OPieceCartoucheTable();
    ~OPieceCartoucheTable();

    /// Re-resolve the current pattern's measurement table path (.omt or .otu)
    /// and re-parse only when it changed.
    void Load();

    /// True when a multi-size (.omt) table is currently loaded.
    auto HasMultiSizeTable() const -> bool;

    /// True when any measurement table (.omt or .otu) is currently loaded.
    auto HasTable() const -> bool;

    /// The size-group (axis) names of the table, user-defined.
    /// Empty when no multi-size (.omt) table is loaded.
    auto AxisNames() const -> QStringList;

    /// Reference value of an axis, verbatim (user-defined: "168", "168 cm", "X"…).
    /// Empty when no multi-size (.omt) table is loaded.
    auto AnchorValue(const QString &axisName) const -> QString;

    /// All values of an axis, verbatim, in table order (e.g. "160", "168", "176").
    /// Empty when no multi-size (.omt) table is loaded.
    auto AxisValues(const QString &axisName) const -> QStringList;

    /// The measurement formula names (@Name). Works for .omt and .otu tables.
    auto MeasurementNames() const -> QStringList;

    /// Full display name of a measurement (@Name -> full name).
    auto MeasurementFullName(const QString &name) const -> QString;

    /// Resolve a stored full name (or a legacy @Name) back to the @Name key.
    /// Empty when the measurement is not found in the current table.
    auto MeasurementNameFromFullName(const QString &fullName) const -> QString;

    /// Value of a measurement at the reference size (anchor value).
    auto MeasurementValue(const QString &name) const -> double;

    /// Measurement unit of the table, as a display string (cm / mm / inch).
    auto UnitsStr() const -> QString;

    /// Format a numeric value, trimming trailing zeros, up to 2 decimals.
    static auto FormatValue(double value) -> QString;

private:
    void Clear();

    std::unique_ptr<OOmtDocument> m_omtDoc;  // multi-size table (.omt): axes + measurements
    std::unique_ptr<OOtuDocument> m_otuDoc;  // single-size table (.otu): measurements only
    QString m_sourcePath;
};

#endif // OPIECECARTOUCHETABLE_H
