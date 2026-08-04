/************************************************************************
 **
 **  @file   macwindowtheme.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/08/2026
 **
 **  @brief  macOS native window appearance helper — sets NSApp.appearance
 **          to match the active Qt theme (Light = Aqua, Dark = DarkAqua).
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
#ifndef MACWINDOWTHEME_H
#define MACWINDOWTHEME_H

#include <QtGlobal>

using WId = quintptr;

/// Sets NSApp.appearance to Aqua (light) or DarkAqua (dark).
/// No-op on non-macOS platforms.
void SetMacWindowAppearance(WId winId, bool dark);

#endif // MACWINDOWTHEME_H
