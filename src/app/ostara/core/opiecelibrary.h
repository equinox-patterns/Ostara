/************************************************************************
 **
 **  @file   opiecelibrary.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  Where pieces are saved, and how a `.opiece` file is written and read back.
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

#ifndef OPIECELIBRARY_H
#define OPIECELIBRARY_H

#include "opiecedocument.h"

#include <QString>
#include <QStringList>

/**
 * @brief The library of saved pieces: one `.opiece` file per piece, in a per-user folder.
 *
 * The cartouche's model, one format further on: files under a folder of the user's home, enumerated by
 * scanning it, each one XML validated against its schema and committed atomically — so a piece is either
 * the one that was saved or the one that was there before, never half of each.
 *
 * The **file name** is the piece's name as the reader typed it; the piece's **identity** is inside the
 * file. The two are deliberately not the same thing: renaming a piece in the library is renaming a file,
 * and it must not break the pieces already imported from it.
 */
namespace OPieceLibrary
{
/** @brief The extension of a saved piece, without its dot. */
auto Extension() -> QString;

/** @brief The folder pieces are saved in, from settings (GetPathPieces). */
auto DefaultDirectory() -> QString;

/** @brief Every piece in @p dirPath, as absolute paths sorted by name — empty if there is none. */
auto ScanDirectory(const QString &dirPath) -> QStringList;

/** @brief Reads @p filePath into @p piece, which must not hold anything worth keeping. */
auto Load(const QString &filePath, OPieceDocument *piece, QString *error) -> bool;

/** @brief Writes @p piece to @p filePath, creating the folder, validated before anything is committed. */
auto Save(OPieceDocument *piece, const QString &filePath, QString *error) -> bool;
} // namespace OPieceLibrary

#endif // OPIECELIBRARY_H
