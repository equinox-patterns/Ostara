/************************************************************************
 **
 **  @file   tst_foldermove.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  Changing a library folder moves its files, and the move is all or nothing: one file that cannot
 **          arrive undoes every copy already made, and the source keeps everything.
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

#include "tst_foldermove.h"

#include "olibrarystorage.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto WriteFile(const QString &path, const QByteArray &content) -> bool
{
    QFile file(path);
    if (not QDir().mkpath(QFileInfo(path).absolutePath()) || not file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    return file.write(content) == content.size();
}

//---------------------------------------------------------------------------------------------------------------------
auto ReadFile(const QString &path) -> QByteArray
{
    QFile file(path);
    if (not file.open(QIODevice::ReadOnly))
    {
        return {};
    }
    return file.readAll();
}

//---------------------------------------------------------------------------------------------------------------------
/// Every file under @p path, without the prefix, sorted — the shape a folder has, to compare two states.
auto Tree(const QString &path) -> QStringList
{
    QStringList entries;
    if (not QDir(path).exists())
    {
        return entries;
    }

    QDirIterator it(path, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        entries << QDir(path).relativeFilePath(it.next());
    }
    entries.sort();
    return entries;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_FolderMove::TST_FolderMove(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::MovesEverythingIncludingSubfolders()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    const QString target = tmp.filePath(QStringLiteral("target"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "a"));
    QVERIFY(WriteFile(source + QStringLiteral("/sub/b.txt"), "b"));

    QStringList blocked;
    QVERIFY(OLibraryStorage::MoveFolderContents(source, target, &blocked));
    QVERIFY2(blocked.isEmpty(), qPrintable(blocked.join(QStringLiteral(", "))));

    QVERIFY(not QDir(source).exists());
    QCOMPARE(ReadFile(target + QStringLiteral("/a.txt")), QByteArray("a"));
    QCOMPARE(ReadFile(target + QStringLiteral("/sub/b.txt")), QByteArray("b"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::OneFileThatCannotArriveLeavesEverything()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    const QString target = tmp.filePath(QStringLiteral("target"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "from the old folder"));
    QVERIFY(WriteFile(source + QStringLiteral("/b.txt"), "b"));
    // The name is already taken in the destination: the copy refuses to overwrite it.
    QVERIFY(WriteFile(target + QStringLiteral("/a.txt"), "kept"));

    const QStringList sourceBefore = Tree(source);

    QStringList blocked;
    QVERIFY(not OLibraryStorage::MoveFolderContents(source, target, &blocked));
    QVERIFY(not blocked.isEmpty());

    // Nothing left the source, and the destination holds only what it already had.
    QCOMPARE(Tree(source), sourceBefore);
    QCOMPARE(ReadFile(source + QStringLiteral("/a.txt")), QByteArray("from the old folder"));
    QCOMPARE(Tree(target), QStringList{QStringLiteral("a.txt")});
    QCOMPARE(ReadFile(target + QStringLiteral("/a.txt")), QByteArray("kept"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::DestinationThatCannotBeCreatedChangesNothing()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "a"));

    // A file where the destination folder should be: nothing can be created there, and the failure comes
    // before a single byte is copied.
    const QString impossible = tmp.filePath(QStringLiteral("impossible"));
    QVERIFY(WriteFile(impossible, "not a folder"));

    QStringList blocked;
    QVERIFY(not OLibraryStorage::MoveFolderContents(source, impossible, &blocked));
    QVERIFY(not blocked.isEmpty());
    QCOMPARE(Tree(source), QStringList{QStringLiteral("a.txt")});
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::MissingSourceIsNotAFailure()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Nothing to move is not an error: a folder that does not exist already "lives" nowhere else, and
    // refusing here would stop a library from being repointed to an empty folder.
    QStringList blocked;
    QVERIFY(OLibraryStorage::MoveFolderContents(tmp.filePath(QStringLiteral("absent")),
                                                tmp.filePath(QStringLiteral("target")), &blocked));
    QVERIFY(blocked.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::NestedFoldersAreRefused()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "a"));

    // The target lives inside the source: copying there would walk the tree being deleted.
    const QString inside = source + QStringLiteral("/inside");

    QStringList blocked;
    QVERIFY(not OLibraryStorage::MoveFolderContents(source, inside, &blocked));
    QCOMPARE(blocked, QStringList{QDir::toNativeSeparators(inside)});
    QCOMPARE(Tree(source), QStringList{QStringLiteral("a.txt")});
    QVERIFY(not QDir(inside).exists());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::MovingAFolderOntoItselfChangesNothing()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "a"));

    // Two spellings of one folder are one folder: a trailing separator is not a different folder, and
    // refusing would be telling the reader off for a gesture that changes nothing.
    QStringList blocked;
    QVERIFY(OLibraryStorage::MoveFolderContents(source, source + QStringLiteral("/"), &blocked));
    QVERIFY(blocked.isEmpty());
    QCOMPARE(Tree(source), QStringList{QStringLiteral("a.txt")});
    QCOMPARE(ReadFile(source + QStringLiteral("/a.txt")), QByteArray("a"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::AFailedMoveKeepsTheFoldersItFound()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    const QString target = tmp.filePath(QStringLiteral("target"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "from the old folder"));
    // The destination already holds an empty folder and a file whose name is taken: neither belongs to
    // this attempt, so neither may disappear when it fails.
    QVERIFY(QDir().mkpath(target + QStringLiteral("/kept-empty")));
    QVERIFY(WriteFile(target + QStringLiteral("/a.txt"), "kept"));

    QStringList blocked;
    QVERIFY(not OLibraryStorage::MoveFolderContents(source, target, &blocked));
    QVERIFY(not blocked.isEmpty());

    QVERIFY(QDir(target + QStringLiteral("/kept-empty")).exists());
    QCOMPARE(Tree(target), (QStringList{QStringLiteral("a.txt"), QStringLiteral("kept-empty")}));
    QCOMPARE(ReadFile(source + QStringLiteral("/a.txt")), QByteArray("from the old folder"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_FolderMove::ACallerListDoesNotDecideTheMove()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QString source = tmp.filePath(QStringLiteral("source"));
    const QString target = tmp.filePath(QStringLiteral("target"));
    QVERIFY(WriteFile(source + QStringLiteral("/a.txt"), "a"));

    // The list is an output: what a caller left in it says nothing about this move, and a success clears it.
    QStringList blocked{QStringLiteral("stale")};
    QVERIFY(OLibraryStorage::MoveFolderContents(source, target, &blocked));
    QVERIFY(blocked.isEmpty());
    QCOMPARE(ReadFile(target + QStringLiteral("/a.txt")), QByteArray("a"));
}

QTEST_MAIN(TST_FolderMove)
