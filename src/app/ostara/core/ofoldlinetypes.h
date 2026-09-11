/************************************************************************
 **
 **  @file   ofoldlinetypes.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/09/2026
 **
 **  @brief  The fold-line types Ostara offers, shared by the piece and group Properties Browsers
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

#ifndef OFOLDLINETYPES_H
#define OFOLDLINETYPES_H

#include "../vmisc/def.h"

#include <QCoreApplication>
#include <QString>
#include <QVector>

// One entry of the fold-line type list. "text" is a QT_TRANSLATE_NOOP literal: the context is shared
// by every widget that builds these buttons, so translate it with OFoldLineTypeText() rather than
// with the calling class's own tr().
struct OFoldLineTypeItem
{
    FoldLineType type;
    const char *text;
};

// The types Ostara offers, in presentation order.
auto OFoldLineTypeItems() -> const QVector<OFoldLineTypeItem> &;

// The label of @a item, in the shared translation context.
inline auto OFoldLineTypeText(const OFoldLineTypeItem &item) -> QString
{
    return QCoreApplication::translate("ODetailsFoldLineWidget", item.text);
}

#endif // OFOLDLINETYPES_H
