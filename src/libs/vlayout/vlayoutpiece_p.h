/************************************************************************
 **
 **  @file   vlayoutdetail_p.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 1, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#ifndef VLAYOUTDETAIL_P_H
#define VLAYOUTDETAIL_P_H

#include <QPointF>
#include <QSharedData>
#include <QTransform>
#include <QVector>

#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vlayoutplacelabel.h"
#include "../vpatterndb/floatItemData/floatitemdef.h"
#include "../vwidgets/vpiecegrainline.h"
#include "vlayoutpiecepath.h"
#include "vlayoutpoint.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

class VLayoutPieceData : public QSharedData
{
public:
    VLayoutPieceData() = default;
    VLayoutPieceData(const VLayoutPieceData &detail) = default;
    ~VLayoutPieceData() = default;


    /** @brief contour list of contour points. */
    QVector<VLayoutPoint> m_contour{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief seamAllowance list of seam allowance points. */
    QVector<VLayoutPoint> m_seamAllowance{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief layoutAllowance list of layout allowance points. */
    QVector<QPointF> m_layoutAllowance{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief passmarks list of passmakrs. */
    QVector<VLayoutPassmark> m_passmarks{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_internalPaths list of internal paths. */
    QVector<VLayoutPiecePath> m_internalPaths{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief matrix transformation matrix*/
    QTransform m_matrix{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief layoutWidth value layout allowance width in pixels. */
    qreal m_layoutWidth{0}; // NOLINT(misc-non-private-member-variables-in-classes)

    bool m_verticallyFlipped{false};   // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_horizontallyFlipped{false}; // NOLINT(misc-non-private-member-variables-in-classes)

    VPieceGrainline m_grainline{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_placeLabels list of place labels. */
    QVector<VLayoutPlaceLabel> m_placeLabels{}; // NOLINT(misc-non-private-member-variables-in-classes)

    qint64 m_square{0}; // NOLINT(misc-non-private-member-variables-in-classes)

    quint16 m_quantity{1}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_id keep id of original piece. */
    vidtype m_id{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes)

    QString m_gradationId{}; // NOLINT(misc-non-private-member-variables-in-classes)

    qreal m_xScale{1.0}; // NOLINT(misc-non-private-member-variables-in-classes)
    qreal m_yScale{1.0}; // NOLINT(misc-non-private-member-variables-in-classes)

    QLineF m_seamMirrorLine{};          // NOLINT(misc-non-private-member-variables-in-classes)
    QLineF m_seamAllowanceMirrorLine{}; // NOLINT(misc-non-private-member-variables-in-classes)

    qreal m_foldLineHeight{0};   // NOLINT(misc-non-private-member-variables-in-classes)
    qreal m_foldLineWidth{0};    // NOLINT(misc-non-private-member-variables-in-classes)
    qreal m_foldLineCenter{0.5}; // NOLINT(misc-non-private-member-variables-in-classes)

    QFont m_foldLineOutlineFont{};     // NOLINT (misc-non-private-member-variables-in-classes)

    /** @brief m_showFoldLineLabel whether this drawing shows the fold-line caption ("Cut on fold").
     * Deliberately out of the data stream: it is a decision of ONE drawing (a cutting plan asks it
     * away), never a property of the piece that a save would carry from one run to the next. */
    bool m_showFoldLineLabel{true}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_foldLineCaption the text the fold-line caption draws on this piece: its own, or
     * the generic one resolved in the cartouche's languages when the piece was built. Deliberately
     * out of the data stream, like m_showFoldLineLabel: it is what the application could say at one
     * moment, not a property of the piece. An empty value falls back to the piece's own label. */
    QString m_foldLineCaption{}; // NOLINT(misc-non-private-member-variables-in-classes)

private:
    Q_DISABLE_ASSIGN_MOVE(VLayoutPieceData) // NOLINT
};

QT_WARNING_POP
#endif // VLAYOUTDETAIL_P_H
