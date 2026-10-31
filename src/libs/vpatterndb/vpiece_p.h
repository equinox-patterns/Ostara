/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 24/09/2026.
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
// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#ifndef VPIECE_P_H
#define VPIECE_P_H

#include <QSharedData>
#include <QVector>

#include "../vmisc/def.h"
#include "../vmisc/typedef.h"
#include "floatItemData/vgrainlinedata.h"
#include "oannotationdata.h"
#include "oinfoblockdata.h"
#include "othumbnaildata.h"
#include "vpiecepath.h"

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")
QT_WARNING_DISABLE_GCC("-Wnon-virtual-dtor")

class VPieceData : public QSharedData
{
public:
    explicit VPieceData(PiecePathType type);
    VPieceData(const VPieceData &detail) = default;
    ~VPieceData() = default;

    /** @brief nodes list detail nodes. */
    VPiecePath m_path; // NOLINT(misc-non-private-member-variables-in-classes)

    bool m_inLayout{true}; // NOLINT(misc-non-private-member-variables-in-classes)
    /** @brief Left out of the piece total (it still goes to the layout and the export). */
    bool m_excludedFromTotal{false}; // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_united{false};            // NOLINT(misc-non-private-member-variables-in-classes)

    QVector<CustomSARecord> m_customSARecords{}; // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<quint32> m_internalPaths{};          // NOLINT(misc-non-private-member-variables-in-classes)
    QVector<quint32> m_placeLabels{};            // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief SA custom margin group node IDs (sorted). */
    QVector<QVector<quint32>> m_saMarginGroupNodeIds{}; // NOLINT(misc-non-private-member-variables-in-classes)
    /** @brief Names for SA custom margin groups (parallel to m_saMarginGroupNodeIds). */
    QVector<QString> m_saMarginGroupNames{}; // NOLINT(misc-non-private-member-variables-in-classes)
    /** @brief Color indices for SA custom margin groups (parallel to m_saMarginGroupNodeIds). */
    QVector<int> m_saMarginGroupColorIndices{}; // NOLINT(misc-non-private-member-variables-in-classes)
    /** @brief Show-on-canvas states for SA custom margin groups (parallel to m_saMarginGroupNodeIds). */
    QVector<bool> m_saMarginGroupShowStates{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief m_glGrainline grainline geometry object*/
    VGrainlineData m_glGrainline{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Per-piece information block state (cartouche). */
    OInfoBlockData m_infoBlocks{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Per-piece floating annotations. */
    OAnnotationData m_annotations{}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Per-piece thumbnail state (piece miniature). */
    OThumbnailData m_thumbnail{}; // NOLINT(misc-non-private-member-variables-in-classes)

    QString m_formulaWidth{'0'}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Graded alignment: horizontal (1=left, 2=center, 3=right). */
    int m_gradedAlignH{2}; // NOLINT(misc-non-private-member-variables-in-classes)
    /** @brief Graded alignment: vertical (1=top, 2=center, 3=bottom). */
    int m_gradedAlignV{2}; // NOLINT(misc-non-private-member-variables-in-classes)

    /**
     * @brief Forced graded alignment: main path node whose X every size must share (NULL_ID = none).
     *
     * When set, the node's X is the same for all sizes and the horizontal alignment is overridden;
     * the vertical alignment keeps working. Stored as the node id of this piece's own copy.
     */
    quint32 m_gradedAlignPointId{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Piece number displayed on the cartouche (0 = none). */
    uint m_pieceNumber{0}; // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Cut quantities and fold flags per fabric layer (cartouche). */
    int m_cutFabricQty{0};       // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_cutFabricOnFold{false};       // NOLINT(misc-non-private-member-variables-in-classes)
    int m_cutInterfacingQty{0};  // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_cutInterfacingOnFold{false};  // NOLINT(misc-non-private-member-variables-in-classes)
    int m_cutLiningQty{0};       // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_cutLiningOnFold{false};       // NOLINT(misc-non-private-member-variables-in-classes)

    /** @brief Free text shown on the cartouche (piece level). */
    QString m_freeText{}; // NOLINT(misc-non-private-member-variables-in-classes)

    quint32 m_mirrorLineStartPoint{NULL_ID}; // NOLINT(misc-non-private-member-variables-in-classes)
    quint32 m_mirrorLineEndPoint{NULL_ID};   // NOLINT(misc-non-private-member-variables-in-classes)

    bool m_manualFoldHeight{false}; // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_manualFoldWidth{false};  // NOLINT(misc-non-private-member-variables-in-classes)
    bool m_manualFoldCenter{false}; // NOLINT(misc-non-private-member-variables-in-classes)

    QString m_formulaFoldHeight{}; // NOLINT(misc-non-private-member-variables-in-classes)
    QString m_formulaFoldWidth{};  // NOLINT(misc-non-private-member-variables-in-classes)
    QString m_formulaFoldCenter{}; // NOLINT(misc-non-private-member-variables-in-classes)

private:
    Q_DISABLE_ASSIGN_MOVE(VPieceData) // NOLINT
};

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
inline VPieceData::VPieceData(PiecePathType type)
  : m_path(type)
{
}

#endif // VPIECE_P_H
