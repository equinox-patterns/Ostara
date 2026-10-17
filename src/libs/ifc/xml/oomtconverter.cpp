/************************************************************************
 **
 **  @file   oomtconverter.cpp
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

#include "oomtconverter.h"
#include "oomtdocument.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMap>

#include <memory>

#include "vexception.h"
#include "vvstconverter.h"

Q_LOGGING_CATEGORY(vOmtConvert, "v.omt.convert")

using GroupIncMap = QMap<QString, GroupIncrement>;

namespace {

// ===== Constants for .vst XML tags =====
const QString VstTagVST = QStringLiteral("vst");
const QString VstTagUnit = QStringLiteral("unit");
const QString VstTagDimensions = QStringLiteral("dimensions");
const QString VstTagDimension = QStringLiteral("dimension");
const QString VstTagBodyMeasurements = QStringLiteral("body-measurements");
const QString VstTagM = QStringLiteral("m");
const QString VstTagCorrections = QStringLiteral("corrections");
const QString VstTagCorrection = QStringLiteral("correction");

const QString VstAttrBase = QStringLiteral("base");
const QString VstAttrMin = QStringLiteral("min");
const QString VstAttrMax = QStringLiteral("max");
const QString VstAttrStep = QStringLiteral("step");
const QString VstAttrType = QStringLiteral("type");
const QString VstAttrName = QStringLiteral("name");
const QString VstAttrFullName = QStringLiteral("full_name");
const QString VstAttrDescription = QStringLiteral("description");
const QString VstAttrShiftA = QStringLiteral("shiftA");
const QString VstAttrShiftB = QStringLiteral("shiftB");
const QString VstAttrShiftC = QStringLiteral("shiftC");
const QString VstAttrSpecialUnits = QStringLiteral("specialUnits");
const QString VstAttrCoordinates = QStringLiteral("coordinates");
const QString VstAttrCorrection = QStringLiteral("correction");

// ===== Helpers =====

/// Generate all values from min to max by step
auto GenerateValues(qreal minVal, qreal maxVal, qreal stepVal) -> QStringList
{
    QStringList result;
    if (stepVal <= 0)
    {
        return result;
    }
    for (qreal v = minVal; v <= maxVal + 0.001; v += stepVal)
    {
        result.append(QString::number(v, 'f', 0));
    }
    return result;
}

/// Get a dimension name from its type attribute
auto DimensionNameFromType(const QString &type) -> QString
{
    if (type == QStringLiteral("x"))
    {
        return QLatin1String("Stature");
    }
    if (type == QStringLiteral("y"))
    {
        return QLatin1String("Taille");
    }
    if (type == QStringLiteral("w"))
    {
        return QLatin1String("Tour de taille");
    }
    if (type == QStringLiteral("z"))
    {
        return QLatin1String("Tour de hanches");
    }
    return type;
}

/// Parse a semicolon-separated coordinate string like "160;32" into doubles
} // anonymous namespace

// ===== Public =====

auto OOmtConverter::Convert(const QString &vstPath) -> OOmtConversionResult
{
    OOmtConversionResult result;

    // A .vst written before 0.5.0 carries no <dimensions> element — only single base values for size and height —
    // and its measures use size_increase/height_increase instead of shiftA/B/C. Upgrade it first, exactly as the
    // pattern loader does; read as is, there is simply nothing to find.
    // Kept alive for the whole function: Convert() hands back a temporary file that the converter owns and
    // deletes with itself.
    std::unique_ptr<VVSTConverter> converter;
    QString source = vstPath;
    try
    {
        converter = std::make_unique<VVSTConverter>(vstPath);
        source = converter->Convert();
    }
    catch (const VException &e)
    {
        qCWarning(vOmtConvert) << "Cannot upgrade .vst file:" << e.ErrorMessage();
        return result;
    }

    // --- Load .vst XML ---
    QFile file(source);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qCWarning(vOmtConvert) << "Cannot open .vst file:" << vstPath;
        return result;
    }

    QDomDocument vstDoc;
    const QDomDocument::ParseResult parseResult = vstDoc.setContent(&file);
    if (!parseResult)
    {
        qCWarning(vOmtConvert) << "XML parse error at line" << parseResult.errorLine
                                << ":" << parseResult.errorMessage;
        return result;
    }
    file.close();

    QDomElement root = vstDoc.documentElement();
    if (root.tagName() != VstTagVST)
    {
        qCWarning(vOmtConvert) << "Not a .vst file, root tag:" << root.tagName();
        return result;
    }

    // --- Parse .vst data ---
    QVector<DimInfo> dimensions = ParseDimensions(root);
    if (dimensions.isEmpty())
    {
        qCWarning(vOmtConvert) << "No dimensions found in .vst file";
        return result;
    }

    QVector<MeasurementData> measurements = ParseMeasurements(root);
    if (measurements.isEmpty())
    {
        qCWarning(vOmtConvert) << "No measurements found in .vst file";
        return result;
    }

    // --- Determine unit ---
    QDomElement unitEl = root.firstChildElement(VstTagUnit);
    QString unitStr = unitEl.text().trimmed();
    Unit unit = StrToUnits(unitStr);

    // --- Build OOmtDocument ---
    auto doc = std::make_unique<OOmtDocument>();
    doc->CreateEmptyFile(unit);

    // Table name from file name
    QString tableName = QFileInfo(vstPath).completeBaseName();
    doc->SetTableName(tableName);

    // --- Add size groups and anchors ---
    for (const DimInfo &dim : dimensions)
    {
        doc->AddSizeGroup(dim.name, dim.values);
        doc->SetAnchorBinding(dim.name, QString::number(dim.baseValue, 'f', 0));
    }

    // --- Add measurements ---
    // Reference coordinate for reverse-engineering (all axes at base)
    QVector<qreal> referenceCoord;
    for (const DimInfo &dim : dimensions)
    {
        referenceCoord.append(dim.baseValue);
    }

    for (const MeasurementData &md : measurements)
    {
        if (md.name.isEmpty())
        {
            continue;
        }

        GroupIncMap increments;

        // Determine if this measurement uses corrections
        bool hasCorrections = !md.corrections.isEmpty();

        for (int axisIdx = 0; axisIdx < dimensions.size(); ++axisIdx)
        {
            GroupIncrement gi;

            if (!hasCorrections)
            {
                // Case 1: no corrections — use shifts directly as linear
                gi.isLinear = true;
                if (axisIdx == 0)
                {
                    gi.linearValue = md.shiftA;
                }
                else if (axisIdx == 1)
                {
                    gi.linearValue = md.shiftB;
                }
                else
                {
                    gi.linearValue = md.shiftC;
                }
            }
            else
            {
                // Case 2/3: reverse-engineer from computed values
                // Each axis gets its own RangeGroup(s), stored under the correct axis name
                QVector<RangeGroup> axisRanges;
                gi = ReverseIncrements(md, dimensions, axisIdx, referenceCoord, axisRanges);

                if (!gi.isLinear && gi.rangeGroupName.isEmpty() && !axisRanges.isEmpty())
                {
                    // ReverseIncrements set gi.isLinear=false but may not have set
                    // rangeGroupName if it found a single group (edge case).
                    // Take the first range group name.
                    gi.rangeGroupName = axisRanges.first().name;
                }

                // Generate unique names and register under the correct axis
                const QString axisName = dimensions.at(axisIdx).name;
                for (int rgIdx = 0; rgIdx < axisRanges.size(); ++rgIdx)
                {
                    RangeGroup rg = axisRanges.at(rgIdx);
                    QString uniqueName = QStringLiteral("%1_%2_G%3")
                                             .arg(md.name.mid(1))     // remove @ prefix
                                             .arg(axisName)
                                             .arg(rgIdx + 1);
                    rg.name = uniqueName;
                    doc->AddRangeGroup(axisName, rg);

                    // Update the GroupIncrement to reference the unique name
                    if (rgIdx == 0)
                    {
                        gi.rangeGroupName = uniqueName;
                    }
                }
            }

            increments.insert(dimensions.at(axisIdx).name, gi);
        }

        // Compute the anchor value at the reference coordinate
        qreal anchorValue = ComputeValue(md, referenceCoord, dimensions);

        doc->AddM(md.name, md.fullName, md.description, anchorValue, increments,
                  QStringLiteral("measurement"), md.specialUnits);
    }

    // --- Compute grid ---
    doc->ComputeGrid();

    result.document = std::move(doc);
    return result;
}

// ===== Private: Parse helpers =====

auto OOmtConverter::ParseDimensions(const QDomElement &root) -> QVector<DimInfo>
{
    QVector<DimInfo> result;
    QDomElement dimsEl = root.firstChildElement(VstTagDimensions);
    if (dimsEl.isNull())
    {
        return result;
    }

    QDomNodeList dimList = dimsEl.elementsByTagName(VstTagDimension);
    for (int i = 0; i < dimList.count(); ++i)
    {
        QDomElement dimEl = dimList.at(i).toElement();
        DimInfo info;
        // Use the user's custom name if set, otherwise fall back to the default
        const QString customName = dimEl.attribute(QStringLiteral("customName"));
        info.name = customName.isEmpty()
            ? DimensionNameFromType(dimEl.attribute(VstAttrType))
            : customName;
        info.baseValue = dimEl.attribute(VstAttrBase).toDouble();
        info.minValue = dimEl.attribute(VstAttrMin).toDouble();
        info.maxValue = dimEl.attribute(VstAttrMax).toDouble();
        info.step = dimEl.attribute(VstAttrStep).toDouble();
        info.values = GenerateValues(info.minValue, info.maxValue, info.step);
        result.append(info);
    }

    return result;
}

auto OOmtConverter::ParseMeasurements(const QDomElement &root) -> QVector<MeasurementData>
{
    QVector<MeasurementData> result;
    QDomElement bmEl = root.firstChildElement(VstTagBodyMeasurements);
    if (bmEl.isNull())
    {
        return result;
    }

    QDomNodeList mList = bmEl.elementsByTagName(VstTagM);
    for (int i = 0; i < mList.count(); ++i)
    {
        QDomElement mEl = mList.at(i).toElement();
        MeasurementData md;
        md.name = mEl.attribute(AttrName);
        md.fullName = mEl.attribute(VstAttrFullName);
        md.description = mEl.attribute(VstAttrDescription);
        md.base = mEl.attribute(VstAttrBase).toDouble();
        md.shiftA = mEl.attribute(VstAttrShiftA, QStringLiteral("0")).toDouble();
        md.shiftB = mEl.attribute(VstAttrShiftB, QStringLiteral("0")).toDouble();
        md.shiftC = mEl.attribute(VstAttrShiftC, QStringLiteral("0")).toDouble();
        md.specialUnits = (mEl.attribute(VstAttrSpecialUnits, QStringLiteral("false")) == QStringLiteral("true"));

        // Parse corrections
        QDomElement correctionsEl = mEl.firstChildElement(VstTagCorrections);
        if (!correctionsEl.isNull())
        {
            QDomNodeList corrList = correctionsEl.elementsByTagName(VstTagCorrection);
            for (int c = 0; c < corrList.count(); ++c)
            {
                QDomElement corrEl = corrList.at(c).toElement();
                QString coord = corrEl.attribute(VstAttrCoordinates);
                qreal corrVal = corrEl.attribute(VstAttrCorrection, QStringLiteral("0")).toDouble();
                md.corrections.insert(coord, corrVal);
            }
        }

        result.append(md);
    }

    return result;
}

// ===== Private: Value computation =====

auto OOmtConverter::ComputeValue(const MeasurementData &md, const QVector<qreal> &coords,
                                  const QVector<DimInfo> &dims) const -> qreal
{
    qreal value = md.base;

    // Apply per-axis shifts
    for (int i = 0; i < coords.size() && i < dims.size(); ++i)
    {
        qreal k = 0;
        if (dims.at(i).step > 0)
        {
            k = (coords.at(i) - dims.at(i).baseValue) / dims.at(i).step;
        }

        qreal shift = 0;
        if (i == 0)
        {
            shift = md.shiftA;
        }
        else if (i == 1)
        {
            shift = md.shiftB;
        }
        else
        {
            shift = md.shiftC;
        }

        value += k * shift;
    }

    // Apply correction if any
    QString key = CoordKey(coords);
    if (md.corrections.contains(key))
    {
        value += md.corrections.value(key);
    }

    return value;
}

auto OOmtConverter::CoordKey(const QVector<qreal> &coords) -> QString
{
    QStringList parts;
    for (qreal v : coords)
    {
        parts.append(QString::number(v, 'f', 0));
    }
    return parts.join(QLatin1Char(';'));
}

// ===== Private: Reverse-engineering =====

auto OOmtConverter::ReverseIncrements(const MeasurementData &md,
                                       const QVector<DimInfo> &dimensions,
                                       int axisIndex,
                                       const QVector<qreal> &referenceCoord,
                                       QVector<RangeGroup> &outRanges) const -> GroupIncrement
{
    GroupIncrement result;
    const DimInfo &axis = dimensions.at(axisIndex);

    if (axis.values.size() < 2)
    {
        result.isLinear = true;
        result.linearValue = 0.0;
        return result;
    }

    // Build a coordinate with all axes fixed to reference except the target axis
    QVector<qreal> coord = referenceCoord;

    // Compute differences for each transition on this axis
    QVector<qreal> diffs;
    for (int v = 0; v < axis.values.size() - 1; ++v)
    {
        coord[axisIndex] = axis.values.at(v).toDouble();
        qreal val1 = ComputeValue(md, coord, dimensions);

        coord[axisIndex] = axis.values.at(v + 1).toDouble();
        qreal val2 = ComputeValue(md, coord, dimensions);

        diffs.append(val2 - val1);
    }

    // Check if all diffs are the same → linear
    bool allSame = true;
    qreal firstDiff = diffs.first();
    for (qreal d : diffs)
    {
        if (qAbs(d - firstDiff) > 0.001)
        {
            allSame = false;
            break;
        }
    }

    if (allSame)
    {
        result.isLinear = true;
        result.linearValue = firstDiff;
        return result;
    }

    // Non-linear: group consecutive equal diffs into ranges
    result.isLinear = false;
    result.rangeGroupName = QStringLiteral("Groupe %1").arg(outRanges.size() + 1);

    RangeGroup rg;
    rg.name = result.rangeGroupName;

    QString currentFrom = axis.values.at(0);
    qreal currentDiff = diffs.at(0);
    QVector<qreal> rangeValues;
    rangeValues.append(currentDiff);

    for (int v = 1; v < diffs.size(); ++v)
    {
        if (qAbs(diffs.at(v) - currentDiff) < 0.001)
        {
            // Same increment, extend the current range
            // Update "to" value
        }
        else
        {
            // Different increment → close current range, start new one
            RangeDef rd;
            rd.fromValue = currentFrom;
            rd.toValue = axis.values.at(v); // v is the index of the first value with the new diff
            rg.ranges.append(rd);

            currentFrom = axis.values.at(v);
            currentDiff = diffs.at(v);
            rangeValues.append(currentDiff);
        }
    }

    // Close the last range
    RangeDef rd;
    rd.fromValue = currentFrom;
    rd.toValue = axis.values.last();
    rg.ranges.append(rd);

    outRanges.append(rg);
    result.rangeValues = rangeValues;

    return result;
}
