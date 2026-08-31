/************************************************************************
 **
 **  @file   oinfoblockcontent.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Cross-layer contract between the app (which resolves the real
 **          content of an information block) and the libs piece tool
 **          (which renders it). Keeps libs free of any app dependency.
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

#ifndef OINFOBLOCKCONTENT_H
#define OINFOBLOCKCONTENT_H

#include <QHash>
#include <QList>
#include <functional>

#include "../../vpatterndb/vpiece.h"
#include "../../vwidgets/ocartouchepainter.h"

class VAbstractPattern;
class VContainer;

/**
 * @brief Effective per-piece information block state plus the resolved
 * lines of every existing block, produced by the app for the canvas.
 *
 * "Effective" means the seed-vs-override resolution is already applied:
 * an unset piece follows the pattern-level seed, a set piece carries its
 * explicit values. A block exists iff at least one section is dispatched
 * to it.
 */
struct OInfoBlockEffectiveState
{
    bool active = false;
    QString templateName;                             // Effective template base name.
    QHash<QString, int> dispatch;                     // Section type -> block 1..11.
    QHash<int, QList<OCartoucheRenderLine>> blockContent; // Existing block -> rendered lines.
};

/**
 * @brief Content builder registered by the app. The piece tool calls it to
 * resolve the block content it cannot resolve itself (layering).
 */
using OInfoBlockContentBuilder = std::function<OInfoBlockEffectiveState(const VPiece &, VAbstractPattern *, const VContainer *)>;

#endif // OINFOBLOCKCONTENT_H
