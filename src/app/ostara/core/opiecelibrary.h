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

#include <QDateTime>
#include <QList>
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
/**
 * @brief What a piece of the library says about itself, read without its geometry.
 *
 * Enough to show it in a list and to say what it will ask for: a piece that reads a value it does not
 * declare needs another piece, and the library is where that other piece is looked for.
 */
struct Summary
{
    QString path{};      ///< the file, as the library holds it.
    QString name{};      ///< the name the piece wears — its file's name when the file could not be read.
    QString unit{};      ///< the unit of the pattern it was cut from (`mm`, `cm`, `inch`).
    QString notes{};     ///< what the reader wrote about the piece; empty when she wrote nothing.
    QList<OPieceDocument::Dependency> dependencies{}; ///< what it reads without declaring it.
    QStringList measurements{}; ///< the measurements its formulas read, as its own block names them.
    QDateTime modified{};///< when the file was last written.
    QString error{};     ///< why the file could not be read; empty when it could.
};

/** @brief The extension of a saved piece, without its dot. */
auto Extension() -> QString;

/** @brief The folder pieces are saved in, from settings (GetPathPieces). */
auto DefaultDirectory() -> QString;

/**
 * @brief Makes the library's own folder, and does nothing when it is already there.
 *
 * Called at the first launch, before any piece was ever saved: the window that shows the library would otherwise
 * be showing the absence of a folder rather than an empty library. Never fatal — a folder made here is a comfort,
 * and a missing one is made again at the first write.
 */
void EnsureDirectory();

/** @brief Every piece in @p dirPath, as absolute paths sorted by name — empty if there is none. */
auto ScanDirectory(const QString &dirPath) -> QStringList;

/// @brief A folder of the library, named the way a reader reads it: `Dupont/Hiver`, and where it lives.
struct Folder
{
    QString label{}; ///< the path relative to the library's own folder, written with slashes.
    QString path{};  ///< the folder itself, absolute.
};

/**
 * @brief Every piece in @p dirPath, read and summarised, in the order the scan gives.
 *
 * A file that cannot be read is **kept in the list**, with its reason: a piece that disappears from the
 * library without a word is worse than one that says it is unreadable — the reader would look for a
 * mistake of her own.
 */
auto Summaries(const QString &dirPath) -> QList<Summary>;

/**
 * @brief Every piece under @p rootPath, sub-folders walked, judged as the folder's list judges it.
 *
 * The same summaries as Summaries, one level deeper: the search of the window reads the whole library at once,
 * and the dialog that makes a pattern from a piece has to be able to offer a piece whatever folder it is in.
 */
auto AllSummaries(const QString &rootPath) -> QList<Summary>;

/**
 * @brief The sub-folders of @p rootPath, recursively, each with its readable label, sorted — root left out.
 *
 * The root is not a line of its own: it is where a reader already is when nothing is chosen, and naming it in
 * the list would make two ways of saying "here".
 */
auto Folders(const QString &rootPath) -> QList<Folder>;

/**
 * @brief Makes @p name under @p parentPath, refusing an empty name, a separator, or a name already taken.
 *
 * @param error the refusal, translated — a folder the reader asked for and did not get has to say why.
 */
auto CreateFolder(const QString &parentPath, const QString &name, QString *error) -> bool;

/// @brief Where the piece of @p sourcePath lands when moved into @p targetDirectory.
auto TargetPath(const QString &sourcePath, const QString &targetDirectory) -> QString;

/// @brief Moves the piece into @p targetDirectory — replacing what is there when @p overwrite says so.
auto Transfer(const QString &sourcePath, const QString &targetDirectory, bool overwrite, QString *error) -> bool;

/// @brief Copies the piece to @p targetPath, replacing what is there — never the file itself.
auto Export(const QString &sourcePath, const QString &targetPath, QString *error) -> bool;

/// @brief Removes the piece from the library, and says so when the file refuses.
auto Remove(const QString &filePath, QString *error) -> bool;

/**
 * @brief What @p summary reads elsewhere, named for a list — `Buste, Manche`.
 *
 * The **pieces**, not the tokens: a piece that enters a pattern asks for other pieces, and that is what a
 * reader looks for. A file that cannot be read has nothing to say about what it needs, and says so with a dash.
 *
 * Written once, here, beside the summary it describes: the library window and the dialog that imports both show
 * this line, and two authors would end up saying it two ways.
 */
auto Needs(const Summary &summary) -> QString;

/**
 * @brief The whole line a table cell holds: the pieces it needs, and how many measurements it reads.
 *
 * `Needs` is a **fragment** and says nothing when there is nothing to say — in the dialog that imports, it is one
 * word inside a sentence. A table cell cannot be empty: an empty cell reads like a piece nobody looked at. So the
 * opening line is written once, here, beside the summary: `Buste, Manche, 4 measurements`, or `Nothing`.
 */
auto NeedsLine(const Summary &summary) -> QString;

/**
 * @brief The same, told at length: what it reads elsewhere — one line per value, and who declares it — then the
 * measurements its formulas read.
 *
 * The case nothing declares is **shown rather than hidden** — it is exactly the one the import refuses, and it
 * is better seen in the library than met as a refusal. The measurements are named too: a piece enters a pattern
 * whose table has to define them, and this is where that can be seen before the refusal.
 */
auto Details(const Summary &summary) -> QString;

/** @brief Reads @p filePath into @p piece, which must not hold anything worth keeping. */
auto Load(const QString &filePath, OPieceDocument *piece, QString *error) -> bool;

/** @brief Writes @p piece to @p filePath, creating the folder, validated before anything is committed. */
auto Save(OPieceDocument *piece, const QString &filePath, QString *error) -> bool;

/**
 * @brief Writes @p notes on the piece held in @p filePath, and saves it back.
 *
 * The note belongs to the piece, so it is written **in the library file** and read again from there: nothing keeps
 * a second copy that could disagree with it. An empty text removes the attribute rather than writing an empty one.
 */
auto SetNotes(const QString &filePath, const QString &notes, QString *error) -> bool;

/**
 * @brief Copies the piece of @p sourcePath into @p targetPath, after reading it **and validating it**.
 *
 * A file that does not open, whose root is not a piece, or that the piece schema refuses is refused with its
 * reason rather than landing in the library as a line the window would then have to explain. The bar is the one
 * the library writes its own pieces with, so what enters is what it would have written. What is copied is then
 * the file itself — byte for byte — because a piece is its own document: the file keeps the name the reader gave
 * it, and the piece keeps the name it was written under, exactly as Export does.
 */
auto Import(const QString &sourcePath, const QString &targetPath, QString *error) -> bool;
} // namespace OPieceLibrary

#endif // OPIECELIBRARY_H
