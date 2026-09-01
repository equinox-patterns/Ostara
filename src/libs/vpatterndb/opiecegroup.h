/************************************************************************
 **
 **  @file   opiecegroup.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  OPieceGroup — a group of pieces sharing a common point.
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

#ifndef OPIECEGROUP_H
#define OPIECEGROUP_H

#include <QHash>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../vmisc/typedef.h"
#include "floatItemData/vgrainlinedata.h"
#include "ipieceinfo.h"
#include "oannotationdata.h"
#include "oinfoblockdata.h"

/**
 * @brief The OPieceGroup struct — a group of pieces sharing a common point.
 *
 * Header-only, like Val Fork's VPieceGroup, but as an Ostara creation the group
 * carries its own Information fields directly (number, cut, free text, info
 * blocks, annotations) instead of delegating to a VCustomLabelData.
 */
struct OPieceGroup : public IPieceInfo
{
    quint32 id{NULL_ID};                     // Unique auto-generated ID
    QString name{};                          // User-defined group name
    QVector<quint32> pieceIds{};             // Member piece IDs (min 2)
    QHash<quint32, QPointF> memberOffsets{}; // Offset per member (scene coords): refNode - memberNode

    int alignH{2}; // Graded alignment horizontal: 1=left, 2=center, 3=right
    int alignV{2}; // Graded alignment vertical: 1=top, 2=center, 3=bottom

    // Children — same types as VPiece. No VCustomLabelData: the group holds its fields directly.
    VGrainlineData m_glGrainline{};
    OInfoBlockData m_infoBlocks{};
    OAnnotationData m_annotations{};
    uint m_pieceNumber{0};
    int m_cutFabricQty{0};
    bool m_cutFabricOnFold{false};
    int m_cutInterfacingQty{0};
    bool m_cutInterfacingOnFold{false};
    int m_cutLiningQty{0};
    bool m_cutLiningOnFold{false};
    QString m_freeText{};
    // Fold formules only — the mirror-line geometry lives on the member piece that carries it.
    QString m_formulaFoldHeight{};
    QString m_formulaFoldWidth{};
    QString m_formulaFoldCenter{};
    bool m_manualFoldHeight{false};
    bool m_manualFoldWidth{false};
    bool m_manualFoldCenter{false};

    auto GetName() const -> QString override { return name; }

    auto GetPieceNumber() const -> uint override { return m_pieceNumber; }
    void SetPieceNumber(uint number) override { m_pieceNumber = number; }

    auto GetCutFabricQty() const -> int override { return m_cutFabricQty; }
    void SetCutFabricQty(int qty) override { m_cutFabricQty = qty; }
    auto IsCutFabricOnFold() const -> bool override { return m_cutFabricOnFold; }
    void SetCutFabricOnFold(bool on) override { m_cutFabricOnFold = on; }

    auto GetCutInterfacingQty() const -> int override { return m_cutInterfacingQty; }
    void SetCutInterfacingQty(int qty) override { m_cutInterfacingQty = qty; }
    auto IsCutInterfacingOnFold() const -> bool override { return m_cutInterfacingOnFold; }
    void SetCutInterfacingOnFold(bool on) override { m_cutInterfacingOnFold = on; }

    auto GetCutLiningQty() const -> int override { return m_cutLiningQty; }
    void SetCutLiningQty(int qty) override { m_cutLiningQty = qty; }
    auto IsCutLiningOnFold() const -> bool override { return m_cutLiningOnFold; }
    void SetCutLiningOnFold(bool on) override { m_cutLiningOnFold = on; }

    auto GetFreeText() const -> QString override { return m_freeText; }
    void SetFreeText(const QString &text) override { m_freeText = text; }

    auto GetInfoBlockData() const -> const OInfoBlockData & override { return m_infoBlocks; }
    auto GetInfoBlockData() -> OInfoBlockData & override { return m_infoBlocks; }
    void SetInfoBlockData(const OInfoBlockData &data) override { m_infoBlocks = data; }

    auto GetAnnotationData() const -> const OAnnotationData & override { return m_annotations; }
    auto GetAnnotationData() -> OAnnotationData & override { return m_annotations; }
    void SetAnnotationData(const OAnnotationData &data) override { m_annotations = data; }
};

#endif // OPIECEGROUP_H
