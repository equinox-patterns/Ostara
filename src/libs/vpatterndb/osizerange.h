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
                           && qFuzzyCompare(l.geom.fontScale, r.geom.fontScale)
                           && l.geom.anchor == r.geom.anchor));
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
                           && qFuzzyCompare(l.geom.fontScale, r.geom.fontScale)
                           && l.geom.anchor == r.geom.anchor));
        }
    };

    /**
     * @brief Optional per-range miniature override (a move/resize done in Graded).
     *
     * The miniature's drawing never changes with the range — it is the piece at the reference size
     * of the pattern — so only its frame is recorded here. No entry (or valid == false) means the
     * frame is the piece's own one, exactly what Details shows.
     */
    struct ThumbnailOverride
    {
        bool valid = false; // an override exists → it wins over the Details geometry
        QPointF pos{};      // parent-local px
        qreal width = 0.0;  // px (0 = auto-size, the Details rule)
        qreal height = 0.0;

        friend bool operator==(const ThumbnailOverride &l, const ThumbnailOverride &r)
        {
            return l.valid == r.valid
                   && (!l.valid
                       || (l.pos == r.pos && qFuzzyCompare(l.width, r.width)
                           && qFuzzyCompare(l.height, r.height)));
        }
    };

    /** @brief Content recorded for one piece inside this range. */
    struct PieceContent
    {
        qreal dx = 0.0; // piece drag offset (all sizes of the piece)
        qreal dy = 0.0;
        GrainlineOverride grainline;              // this piece's own grainline — a group member
                                                  // keeps hers, the group carries a second one
        QHash<int, BlockOverride> blocks;         // block index 1..11 → override (only edited ones)
        QHash<int, AnnotationOverride> annotations; // annotation list index → override (0-based)
        ThumbnailOverride thumbnail;              // this piece's own miniature — a group member keeps
                                                  // hers, the group carries one per member as well
        QString annotationsFp;                    // fingerprint of the annotation list that the
                                                  // overrides were made against (empty = none)

        friend bool operator==(const PieceContent &l, const PieceContent &r)
        {
            return qFuzzyCompare(l.dx, r.dx) && qFuzzyCompare(l.dy, r.dy) && l.grainline == r.grainline
                   && l.blocks == r.blocks && l.annotations == r.annotations && l.thumbnail == r.thumbnail
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
        /** The frame of each member's miniature, by piece id: a group carries no miniature of its own,
         *  it carries one per member — and a range records the frame of every one of them. */
        QHash<quint32, ThumbnailOverride> memberThumbnails;
        QString annotationsFp;                    // fingerprint of the annotation list (see above)

        friend bool operator==(const GroupContent &l, const GroupContent &r)
        {
            return qFuzzyCompare(l.dx, r.dx) && qFuzzyCompare(l.dy, r.dy) && l.grainline == r.grainline
                   && l.blocks == r.blocks && l.annotations == r.annotations
                   && l.memberThumbnails == r.memberThumbnails && l.annotationsFp == r.annotationsFp;
        }
    };

    QString name;                          // unique within the pattern (the key)
    QString measurement;                   // canonical path of the linked measurements table
    QString sizeAxis;                      // name of the axis being graded
    QString minValue;                      // first size of the sub-range (axis CSV label)
    QString maxValue;                      // last size of the sub-range (axis CSV label)
    QMap<QString, QString> axisValues;     // other axes -> frozen value label

    // Export settings of the graded SVG export (dialog step 3, "Export" group).
    // exportFileName / exportLayoutLayerName are stored only when the user overrides
    // the autogenerated file name / the default layout-layer name (empty otherwise →
    // recomputed at export). Path and scales are stored as chosen on the range.
    QString exportPath;                    // destination folder (default = software export path)
    QString exportFileName;                // override of the autogenerated file name (empty = autogen)
    QString exportLayoutLayerName;         // override of the default layout-layer name (empty = default)
    qreal exportXScale{1.0};               // horizontal scale as a fraction (1.0 = 100 %)
    qreal exportYScale{1.0};               // vertical scale as a fraction (1.0 = 100 %)

    QHash<quint32, PieceContent> pieceContent; // by piece id (quint32, NULL_ID = 0)
    QHash<quint32, GroupContent> groupContent; // by group id

    /** @brief Whether the range covers a valid [min..max] sub-range. */
    auto IsValid() const -> bool
    {
        return !name.isEmpty() && !measurement.isEmpty() && !sizeAxis.isEmpty() && !minValue.isEmpty()
               && !maxValue.isEmpty();
    }

    /**
     * @brief Ce que vaut CHAQUE axe à la taille de référence de la plage, nommé comme le tableau
     *        de mesures nomme ses axes.
     *
     * ⚠️ **Sans cette table, un cartouche ne peut pas suivre sa plage.** `OInfoBlockResolver`
     * choisit la valeur affichée de chaque axe — et de là, la colonne du tableau qu'il lit — par
     * `sizeAxes.value(axis, table->AnchorValue(axis))` (`OInfoBlockResolver.cpp:285`) puis
     * `table->MeasurementValueAt(name, sizeAxes)` (`:341`). Ce paramètre a une valeur par défaut
     * vide, et tout retombe alors sur l'ANCRE du tableau, c'est-à-dire la combinaison de
     * référence : une plage en stature 160 imprimait 168, la stature restée active en Details.
     * Le conteneur de mesures n'y entre pas du tout — les valeurs viennent du tableau.
     *
     * La référence est le PREMIER de la plage : c'est la taille que le payload nomme
     * (`<pieces reference="…">`) et la seule dont Graded construit les enfants.
     */
    auto ReferenceAxes() const -> QMap<QString, QString>
    {
        QMap<QString, QString> axes = axisValues;
        if (!sizeAxis.isEmpty() && !minValue.isEmpty())
        {
            axes.insert(sizeAxis, minValue);
        }
        return axes;
    }
};

#endif // OSIZERANGE_H
