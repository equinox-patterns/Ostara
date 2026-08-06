/************************************************************************
 **
 **  @file   oomtconverter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Converts .vst (Valentina multi-size) to .omt (Ostara multi-size).
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

#ifndef OOMTCONVERTER_H
#define OOMTCONVERTER_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>

#include "vmisc/def.h"
#include "omeasurementtypes.h"

class OOmtDocument;

/// Result of the .vst → .omt conversion.
struct OOmtConversionResult
{
    /// The converted document (never null on success).
    std::unique_ptr<OOmtDocument> document;

    /// List of measurement names that couldn't be fully reverse-engineered
    /// (non-separable multi-axis corrections). Empty if everything converted cleanly.
    QStringList unresolvedMeasurements;
};

/// Converts a .vst (Valentina) file to .omt (Ostara) format.
///
/// The converter reads the .vst dimensions and measurements, then
/// reverse-engineers the increment structure:
///
/// Case 1 (no corrections) — shifts are used as-is → linear per axis.
/// Case 2/3 (with corrections, 1-3 axes) — for each axis, fix other axes
///   at reference, compute successive differences, group equal consecutive
///   diffs into RangeGroups.
///
/// A validation pass recomputes all cells from the recipe and compares
/// with the original values. Mismatches are reported as unresolved.
class OOmtConverter
{
public:
    OOmtConverter() = default;
    ~OOmtConverter() = default;

    /// Convert a .vst file to .omt.
    /// @param vstPath  Path to the .vst file.
    /// @return Conversion result with document and optional warnings.
    auto Convert(const QString &vstPath) -> OOmtConversionResult;

private:
    /// Dimension descriptor extracted from .vst
    struct DimInfo
    {
        QString name;          // e.g. "Stature", "Taille"
        qreal baseValue{0};    // reference value
        qreal minValue{0};
        qreal maxValue{0};
        qreal step{0};
        QStringList values;    // all values from min to max by step
    };

    /// Raw measurement data extracted from .vst
    struct MeasurementData
    {
        QString name;          // e.g. "@TP"
        QString fullName;
        QString description;
        qreal base{0};         // anchor value at reference
        qreal shiftA{0};
        qreal shiftB{0};
        qreal shiftC{0};
        bool specialUnits{false};
        /// Map: coordinateKey ("160;32") → correction value
        QMap<QString, qreal> corrections;
    };

    /// Parse dimensions from the .vst DOM
    auto ParseDimensions(const class QDomElement &root) -> QVector<DimInfo>;

    /// Parse measurements from the .vst DOM
    auto ParseMeasurements(const class QDomElement &root) -> QVector<MeasurementData>;

    /// Compute the absolute value for a measurement at a given coordinate.
    /// Formula: base + kA×shiftA + kB×shiftB + kC×shiftC + correction
    auto ComputeValue(const MeasurementData &md, const QVector<qreal> &coords,
                      const QVector<DimInfo> &dims) const -> qreal;

    /// Build a coordinate key string e.g. "160;32"
    static auto CoordKey(const QVector<qreal> &coords) -> QString;

    /// Reverse-engineer increments for one measurement on one axis.
    /// Returns a GroupIncrement (linear or range-based).
    /// @param md             Measurement data
    /// @param dims           All dimensions
    /// @param axisIndex      Which axis to reverse-engineer (0, 1, or 2)
    /// @param referenceCoord The reference coordinate (all axes at base)
    /// @param outRanges      If non-linear, ranges are appended here (caller names them)
    /// @return The GroupIncrement for this axis
    auto ReverseIncrements(const MeasurementData &md,
                           const QVector<DimInfo> &dimensions,
                           int axisIndex,
                           const QVector<qreal> &referenceCoord,
                           QVector<RangeGroup> &outRanges) const -> GroupIncrement;
};

#endif // OOMTCONVERTER_H
