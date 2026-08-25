/************************************************************************
 **
 **  @file   OPieceCartoucheCatalogue.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Registry of the cartouche section types (the catalogue, spec §3).
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

#ifndef OPIECECARTOUCHECATALOGUE_H
#define OPIECECARTOUCHECATALOGUE_H

#include "OPieceCartoucheSection.h"

/**
 * @brief Static registry of the cartouche section types.
 *
 * The catalogue is complete (spec §3) — every type carries a data source; the
 * editor only *reads* the data for the preview. Adding a section type later
 * means registering it here (Open/Closed): the editor, the storage and the
 * preview do not change.
 */
class OPieceCartoucheCatalogue
{
public:
    /// All registered section types, in catalogue order.
    static auto Sections() -> const QList<OPieceCartoucheSection> &;

    /// Lookup by id — returns an invalid (empty) section when unknown.
    static auto Section(const QString &id) -> const OPieceCartoucheSection &;

    /// Drop the cached catalogue so the next access rebuilds it (language change).
    static void InvalidateCache();

private:
    static auto Build() -> QList<OPieceCartoucheSection>;
};

#endif // OPIECECARTOUCHECATALOGUE_H
