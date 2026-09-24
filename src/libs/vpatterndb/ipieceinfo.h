/************************************************************************
 **
 **  @file   ipieceinfo.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Common "piece-like" contract shared by VPiece and OPieceGroup.
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

#ifndef IPIECEINFO_H
#define IPIECEINFO_H

#include <QString>

#include "oannotationdata.h"
#include "oinfoblockdata.h"
#include "othumbnaildata.h"

/**
 * @brief The IPieceInfo interface — the shared "piece-like" contract.
 *
 * Implemented by VPiece and OPieceGroup so that the Information widgets of the
 * Details dock (number, cut, free text, info blocks, annotations) can be driven
 * identically for a piece and for a group.
 */
class IPieceInfo
{
public:
    virtual ~IPieceInfo() = default;

    virtual auto GetName() const -> QString = 0;

    virtual auto GetPieceNumber() const -> uint = 0;
    virtual void SetPieceNumber(uint number) = 0;

    virtual auto GetCutFabricQty() const -> int = 0;
    virtual void SetCutFabricQty(int qty) = 0;
    virtual auto IsCutFabricOnFold() const -> bool = 0;
    virtual void SetCutFabricOnFold(bool on) = 0;

    virtual auto GetCutInterfacingQty() const -> int = 0;
    virtual void SetCutInterfacingQty(int qty) = 0;
    virtual auto IsCutInterfacingOnFold() const -> bool = 0;
    virtual void SetCutInterfacingOnFold(bool on) = 0;

    virtual auto GetCutLiningQty() const -> int = 0;
    virtual void SetCutLiningQty(int qty) = 0;
    virtual auto IsCutLiningOnFold() const -> bool = 0;
    virtual void SetCutLiningOnFold(bool on) = 0;

    virtual auto GetFreeText() const -> QString = 0;
    virtual void SetFreeText(const QString &text) = 0;

    virtual auto GetInfoBlockData() const -> const OInfoBlockData & = 0;
    virtual auto GetInfoBlockData() -> OInfoBlockData & = 0;
    virtual void SetInfoBlockData(const OInfoBlockData &data) = 0;

    virtual auto GetAnnotationData() const -> const OAnnotationData & = 0;
    virtual auto GetAnnotationData() -> OAnnotationData & = 0;
    virtual void SetAnnotationData(const OAnnotationData &data) = 0;

    virtual auto GetThumbnailData() const -> const OThumbnailData & = 0;
    virtual auto GetThumbnailData() -> OThumbnailData & = 0;
    virtual void SetThumbnailData(const OThumbnailData &data) = 0;
};

#endif // IPIECEINFO_H
