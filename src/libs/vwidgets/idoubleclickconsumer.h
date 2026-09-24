/************************************************************************
 **
 **  @file   idoubleclickconsumer.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25 September, 2026
 **
 **  @brief  A canvas item that claims the double-click, so that the view
 **          does not also zoom to fit on the same gesture.
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

#ifndef IDOUBLECLICKCONSUMER_H
#define IDOUBLECLICKCONSUMER_H

/**
 * @brief Implemented by an item whose double-click means something of its own.
 *
 * The view zooms to fit on a double-click, and it does so **before** the scene ever offers the
 * event to the item under the cursor — so an item cannot refuse it from its own handler. This is
 * how it asks in advance instead: an item that answers true here takes the double-click, and the
 * canvas stays where it is.
 *
 * A claim is a promise that the item *does* something with the gesture. An item that answers true
 * and handles nothing leaves the user with a double-click that does neither.
 */
class IDoubleClickConsumer
{
public:
    virtual ~IDoubleClickConsumer() = default;

    IDoubleClickConsumer() = default;
    auto operator=(const IDoubleClickConsumer &) -> IDoubleClickConsumer & = default;
    IDoubleClickConsumer(const IDoubleClickConsumer &) = default;

    /// @brief Whether the item takes the double-click instead of the view's zoom to fit.
    virtual auto ConsumesDoubleClick() const -> bool = 0;
};

#endif // IDOUBLECLICKCONSUMER_H
