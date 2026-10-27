/************************************************************************
 **
 **  @file   PhosphorIconButton.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/08/2026
 **
 **  @brief  Push button drawing a Phosphor glyph. Stub: to be replaced with SVG icons.
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

// Stub — will be replaced with SVG icons
#ifndef PHOSPHORICONBUTTON_H
#define PHOSPHORICONBUTTON_H
#include <QPushButton>
class PhosphorIconButton : public QPushButton
{
public:
    explicit PhosphorIconButton(const QString &iconName, QWidget *parent = nullptr) : QPushButton(parent) { Q_UNUSED(iconName); setText(QStringLiteral("[icon]")); }
    void setHoverStyle(int style) { Q_UNUSED(style); }
};
#endif
