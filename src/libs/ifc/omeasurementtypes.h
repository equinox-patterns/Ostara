/************************************************************************
 **
 **  @file   omeasurementtypes.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Shared data types for measurements.
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

#ifndef OMEASUREMENTTYPES_H
#define OMEASUREMENTTYPES_H

#include <QDebug>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

/// A single size range (from/to) within a RangeGroup.
struct RangeDef
{
    QString fromValue;  // e.g. "32"
    QString toValue;    // e.g. "36"
};

/// A named group of size ranges covering all values of an axis group.
struct RangeGroup
{
    QString name;             // e.g. "Standard"
    QVector<RangeDef> ranges; // ordered, must cover all values
};

/// Increment definition for one axis group within a measurement.
struct GroupIncrement
{
    bool isLinear = true;
    double linearValue = 0.0;          // used when isLinear
    QString rangeGroupName;            // name of RangeGroup, used when !isLinear
    QVector<double> rangeValues;       // one increment per range in the RangeGroup
};

/// Find the increment for a transition whose source is sourceValue.
/// @param direction  +1 = going UP (to next value), -1 = going DOWN (to previous value)
///
/// Cascade rule:
///   - Going DOWN: always uses the increment of the range containing sourceValue
///   - Going UP from the LAST value of a range: uses the NEXT range's increment
///   - Going UP from any other value: uses its own range's increment
inline double FindIncrementForTransition(const GroupIncrement &gi, const QString &sourceValue,
                                          const QStringList &allValues,
                                          const QVector<RangeDef> &ranges,
                                          int direction)
{
    if (gi.isLinear)
    {
        return gi.linearValue;
    }
    int srcIdx = static_cast<int>(allValues.indexOf(sourceValue));
    if (srcIdx < 0)
    {
        qWarning() << "FindIncrementForTransition: sourceValue" << sourceValue
                    << "not found in allValues" << allValues;
        return 0.0;
    }
    for (int i = 0; i < static_cast<int>(ranges.size()) && i < static_cast<int>(gi.rangeValues.size()); ++i)
    {
        const RangeDef &rd = ranges.at(i);
        int fromIdx = static_cast<int>(allValues.indexOf(rd.fromValue));
        int toIdx = static_cast<int>(allValues.indexOf(rd.toValue));
        if (fromIdx >= 0 && toIdx >= 0 && srcIdx >= fromIdx && srcIdx <= toIdx)
        {
            if (direction > 0 && srcIdx == toIdx && i + 1 < gi.rangeValues.size())
            {
                // Going UP from the LAST value of a range → use NEXT range's increment
                return gi.rangeValues.at(i + 1);
            }
            return gi.rangeValues.at(i);
        }
    }
    qWarning() << "FindIncrementForTransition: sourceValue" << sourceValue
                << "not covered by any range - ranges may be stale after grid modification";
    return 0.0;
}

/// Data record for one measurement row.
struct MeasurementRecord
{
    QString name;              // e.g. "@TP"
    QString fullName;          // "Tour de poitrine"
    QString description;
    double anchorValue = 0.0;
    QMap<QString, GroupIncrement> increments; // group name → definition
    bool isSeparator = false;
    bool isCollapsed = false;  // only meaningful when isSeparator == true
};

#endif // OMEASUREMENTTYPES_H
