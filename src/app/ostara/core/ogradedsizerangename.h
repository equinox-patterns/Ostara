/************************************************************************
 **
 **  @file   ogradedsizerangename.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The two characters a size range name may never carry, and the field
 **          filter that keeps them out while the user types.
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

#ifndef OGRADEDSIZERANGENAME_H
#define OGRADEDSIZERANGENAME_H

#include <QObject>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

/**
 * @brief A validator that refuses `/` and `:`, the two characters a size range name
 *        can never carry.
 *
 * A range name is a key inside the pattern today, and the token an exported file
 * name will be built from tomorrow. The two path separators are the only characters
 * that break that second use; accents, spaces and dashes go through untouched.
 *
 * The refusal happens at the keystroke: an invalid character never reaches the field,
 * and a paste that carries one is dropped whole and in silence. The silence is
 * deliberate — the end user does not read messages, and a character that simply does
 * not appear is a clearer answer than a dialog.
 *
 * A validator filters what is typed and nothing else: anything that writes the text
 * programmatically walks straight past it. A caller that owns a commit path keeps its
 * own guard there — this only spares the user the refusal.
 */
inline auto MakeSizeRangeNameValidator(QObject *parent = nullptr) -> QRegularExpressionValidator *
{
    // No ^ or $ on purpose: QRegularExpressionValidator anchors the pattern itself in
    // \A and \z, and adding them would kill the Intermediate state that lets a field be
    // typed into one character at a time.
    static const QRegularExpression forbidden(QStringLiteral("[^/:]*"));
    return new QRegularExpressionValidator(forbidden, parent);
}

#endif  // OGRADEDSIZERANGENAME_H
