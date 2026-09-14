/************************************************************************
 **
 **  @file   ooutputsstale.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  The single-button warning shown on entering the Export mode when
 **          the listed generations were made before the last modification of
 **          the pattern.
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

#ifndef OOUTPUTSSTALE_H
#define OOUTPUTSSTALE_H

#include <QCoreApplication>

class QWidget;

/**
 * @brief The warning of the Export mode, and nothing else.
 *
 * General on purpose: it does not point at any row, and it never relaunches a nesting.
 * It tells the user that modifying the pattern does not propagate to the generations she
 * already made; what she does about it is hers to decide. It is shown on entering the
 * mode, and only there.
 *
 * Its single button makes it impossible to miss — that is the point — and the box under
 * it lets the user say she has understood, once and for all.
 */
class OOutputsStale
{
    Q_DECLARE_TR_FUNCTIONS(OOutputsStale)

public:
    /// @brief Whether the warning is still asked for (the "do not show again" box was not ticked).
    static auto ShouldWarn() -> bool;

    /// @brief Show the warning; forget it for good when its box was ticked.
    static void Show(QWidget *parent);
};

#endif // OOUTPUTSSTALE_H
