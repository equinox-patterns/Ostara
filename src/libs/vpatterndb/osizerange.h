/************************************************************************
 **
 **  @file   osizerange.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  A "size range" of the Graded mode: definition (size axis, sub-range,
 **          frozen values of the other axes) plus the per-piece/group content
 **          (offsets and child overrides) recorded while working in Graded.
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

#ifndef OSIZERANGE_H
#define OSIZERANGE_H

#include "oannotationdata.h"
#include "oinfoblockdata.h"

#include <QHash>
#include <QMap>
#include <QPointF>
#include <QString>
#include <QtGlobal>

/**
 * @brief One size range of the Graded mode.
 *
 * A range is autonomous: its definition (which axis of the measurements table is
 * the graded one, the [min..max] sub-range and the frozen value of every other
 * axis) and its content (offsets / child overrides per piece and per group) live
 * together. Ranges are persisted in the pattern (.ostara) under <sizeRanges>.
 */
class OSizerange
{
public:
    /** @brief Optional per-range grainline override (move/resize done in Graded). */
    struct GrainlineOverride
    {
        bool valid = false;     // an override exists → it wins over the Details base geometry
        QPointF pos{};          // parent-local px
        qreal lengthPx = 0.0;   // length in px

        friend bool operator==(const GrainlineOverride &l, const GrainlineOverride &r)
        {
            return l.valid == r.valid && l.pos == r.pos && qFuzzyCompare(l.lengthPx, r.lengthPx);
        }
    };

    /** @brief Optional per-range info-block override (a move/resize done in Graded). */
    struct BlockOverride
    {
        bool valid = false;     // an override exists → wins over the Details base geometry
        OInfoBlockGeom geom{};  // pos + fontScale (width/height are re-derived from content)

        friend bool operator==(const BlockOverride &l, const BlockOverride &r)
        {
            return l.valid == r.valid
                   && (!l.valid
                       || (l.geom.pos == r.geom.pos && qFuzzyCompare(l.geom.width, r.geom.width)
                           && qFuzzyCompare(l.geom.height, r.geom.height)
                           && qFuzzyCompare(l.geom.rotation, r.geom.rotation)
                           && qFuzzyCompare(l.geom.fontScale, r.geom.fontScale)));
        }
    };

    /** @brief Optional per-range annotation override (a move/resize done in Graded). */
    struct AnnotationOverride
    {
        bool valid = false;     // an override exists → wins over the Details base geometry
        OAnnotationGeom geom{}; // pos + boxWidth/boxHeight + fontScale (rotation is NOT stored —
                                // it stays fixed from the base annotation's rotation formula)

        friend bool operator==(const AnnotationOverride &l, const AnnotationOverride &r)
        {
            return l.valid == r.valid
                   && (!l.valid
                       || (l.geom.pos == r.geom.pos
                           && qFuzzyCompare(l.geom.boxWidth, r.geom.boxWidth)
                           && qFuzzyCompare(l.geom.boxHeight, r.geom.boxHeight)
                           && qFuzzyCompare(l.geom.fontScale, r.geom.fontScale)));
        }
    };

    /** @brief Content recorded for one piece inside this range. */
    struct PieceContent
    {
        qreal dx = 0.0; // piece drag offset (all sizes of the piece)
        qreal dy = 0.0;
        GrainlineOverride grainline;              // only meaningful for a lone (non-grouped) piece
        QHash<int, BlockOverride> blocks;         // block index 1..11 → override (only edited ones)
        QHash<int, AnnotationOverride> annotations; // annotation list index → override (0-based)
        QString annotationsFp;                    // fingerprint of the annotation list that the
                                                  // overrides were made against (empty = none)

        friend bool operator==(const PieceContent &l, const PieceContent &r)
        {
            return qFuzzyCompare(l.dx, r.dx) && qFuzzyCompare(l.dy, r.dy) && l.grainline == r.grainline
                   && l.blocks == r.blocks && l.annotations == r.annotations
                   && l.annotationsFp == r.annotationsFp;
        }
    };

    /** @brief Content recorded for one group inside this range. */
    struct GroupContent
    {
        qreal dx = 0.0; // group drag offset (rigid body)
        qreal dy = 0.0;
        GrainlineOverride grainline;              // the group's shared grainline
        QHash<int, BlockOverride> blocks;         // block index 1..11 → override
        QHash<int, AnnotationOverride> annotations; // annotation list index → override (0-based)
        QString annotationsFp;                    // fingerprint of the annotation list (see above)

        friend bool operator==(const GroupContent &l, const GroupContent &r)
        {
            return qFuzzyCompare(l.dx, r.dx) && qFuzzyCompare(l.dy, r.dy) && l.grainline == r.grainline
                   && l.blocks == r.blocks && l.annotations == r.annotations
                   && l.annotationsFp == r.annotationsFp;
        }
    };

    QString name;                          // unique within the pattern (the key)
    QString measurement;                   // canonical path of the linked measurements table
    QString sizeAxis;                      // name of the axis being graded
    QString minValue;                      // first size of the sub-range (axis CSV label)
    QString maxValue;                      // last size of the sub-range (axis CSV label)
    QMap<QString, QString> axisValues;     // other axes -> frozen value label

    QHash<quint32, PieceContent> pieceContent; // by piece id (quint32, NULL_ID = 0)
    QHash<quint32, GroupContent> groupContent; // by group id

    /** @brief Whether the range covers a valid [min..max] sub-range. */
    auto IsValid() const -> bool
    {
        return !name.isEmpty() && !measurement.isEmpty() && !sizeAxis.isEmpty() && !minValue.isEmpty()
               && !maxValue.isEmpty();
    }
};

#endif // OSIZERANGE_H
