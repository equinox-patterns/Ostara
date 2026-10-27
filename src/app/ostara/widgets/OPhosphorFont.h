/************************************************************************
 **
 **  @file   OPhosphorFont.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/08/2026
 **
 **  @brief  Phosphor icon font helper. Stub: to be replaced with SVG icons.
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

// Stub — Phosphor font functionality will be replaced with SVG icons
#ifndef OPHOSPHORFONT_H
#define OPHOSPHORFONT_H
#include <QChar>
#include <QFont>
struct OPhosphorFont
{
    enum Style { Regular, Fill, Bold, Duotone };
    static auto font(Style style = Regular, int pixelSize = 16) -> QFont { Q_UNUSED(style); QFont f; f.setPixelSize(pixelSize); return f; }
    static auto codepoint(const QString &name) -> QChar { Q_UNUSED(name); return QChar(0x25A1); }
};
#endif
