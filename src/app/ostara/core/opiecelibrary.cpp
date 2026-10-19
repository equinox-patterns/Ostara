/************************************************************************
 **
 **  @file   opiecelibrary.cpp
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

#include "opiecelibrary.h"

#include "olibraryscanner.h"
#include "olibrarystorage.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>

#include <algorithm>

namespace
{
const QString kExtension = QStringLiteral("opiece");
const QString kFilePattern = QStringLiteral("*.opiece");
const QString kRootTag = QStringLiteral("piece");

/// @brief One file, read and summarised — the file name is the fallback name, and not a lesser one.
auto ReadSummary(const QString &filePath) -> OPieceLibrary::Summary
{
    const QFileInfo info(filePath);

    OPieceLibrary::Summary summary;
    summary.path = filePath;
    // It is what the reader will recognise when the file cannot be read, and what she will go and look at.
    summary.name = info.completeBaseName();
    summary.modified = info.lastModified();

    OPieceDocument piece;
    QString error;
    if (OPieceLibrary::Load(filePath, &piece, &error))
    {
        summary.name = piece.PieceName();
        summary.unit = piece.Unit();
        summary.notes = piece.Notes();
        summary.dependencies = piece.Dependencies();
        summary.measurements = piece.Measurements();
    }
    else
    {
        // A file that cannot be read is kept in the list, with its reason: a piece that disappears from the
        // library without a word is worse than one that says it is unreadable.
        summary.error = error;
    }

    return summary;
}

auto Collect(const QStringList &files) -> QList<OPieceLibrary::Summary>
{
    QList<OPieceLibrary::Summary> summaries;
    summaries.reserve(files.size());
    for (const QString &filePath : files)
    {
        summaries.append(ReadSummary(filePath));
    }

    return summaries;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Extension() -> QString
{
    return kExtension;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathPieces();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibrary::EnsureDirectory()
{
    const QString directory = DefaultDirectory();
    if (directory.isEmpty())
    {
        return;
    }

    QDir().mkpath(directory);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::ScanDirectory(const QString &dirPath) -> QStringList
{
    return OLibraryScanner::Files(dirPath, kFilePattern, false);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Summaries(const QString &dirPath) -> QList<Summary>
{
    return Collect(ScanDirectory(dirPath));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::AllSummaries(const QString &rootPath) -> QList<Summary>
{
    QList<Summary> summaries = Collect(OLibraryScanner::Files(rootPath, kFilePattern, true));

    // Read in the order the tree gives; shown in the order a reader looks a name up, which is not the same thing
    // once several folders hold pieces: two pieces of the same name are told apart by their path.
    std::sort(summaries.begin(), summaries.end(), [](const Summary &left, const Summary &right) {
        const int byName = left.name.compare(right.name, Qt::CaseInsensitive);
        return byName != 0 ? byName < 0 : left.path < right.path;
    });

    return summaries;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Folders(const QString &rootPath) -> QList<Folder>
{
    QList<Folder> folders;
    const QList<OLibraryScanner::Folder> scanned = OLibraryScanner::Folders(rootPath);
    folders.reserve(scanned.size());
    for (const OLibraryScanner::Folder &folder : scanned)
    {
        folders.append({folder.label, folder.path});
    }

    return folders;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::CreateDirectory(const QString &parentPath, const QString &name, QString *error) -> bool
{
    return OLibraryStorage::CreateDirectory(parentPath, name, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::TargetPath(const QString &sourcePath, const QString &targetDirectory) -> QString
{
    return OLibraryStorage::TargetPath(sourcePath, targetDirectory);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Transfer(const QString &sourcePath, const QString &targetDirectory, bool overwrite,
                             QString *error) -> bool
{
    return OLibraryStorage::Transfer(sourcePath, targetDirectory, overwrite, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Export(const QString &sourcePath, const QString &targetPath, QString *error) -> bool
{
    return OLibraryStorage::Export(sourcePath, targetPath, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Remove(const QString &filePath, QString *error) -> bool
{
    return OLibraryStorage::Remove(filePath, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Load(const QString &filePath, OPieceDocument *piece, QString *error) -> bool
{
    if (piece == nullptr)
    {
        return false;
    }

    QFile file(filePath);
    if (not file.open(QIODevice::ReadOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }

    const QDomDocument::ParseResult result = piece->setContent(&file);
    if (not result)
    {
        if (error != nullptr)
        {
            *error = result.errorMessage;
        }
        return false;
    }

    if (piece->documentElement().tagName() != kRootTag)
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OPieceLibrary", "Not a piece file.");
        }
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Save(OPieceDocument *piece, const QString &filePath, QString *error) -> bool
{
    if (piece == nullptr)
    {
        return false;
    }

    // A folder that was deleted, or a path typed with one in it, must not fail with a raw
    // "No such file or directory".
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QString err;
    const bool ok = piece->SaveDocument(filePath, err);
    if (not ok && error != nullptr)
    {
        *error = err;
    }
    return ok;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Needs(const Summary &summary) -> QString
{
    if (not summary.error.isEmpty())
    {
        // A file that cannot be read has nothing to say about what it needs, and the dash says exactly that.
        return QStringLiteral("-");
    }

    QStringList pieces;
    for (const OPieceDocument::Dependency &dependency : summary.dependencies)
    {
        pieces.append(dependency.origin.isEmpty() ? dependency.token : dependency.origin);
    }
    pieces.removeDuplicates();

    return pieces.join(QStringLiteral(", "));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::NeedsLine(const Summary &summary) -> QString
{
    if (not summary.error.isEmpty())
    {
        // A file that cannot be read has nothing to say about what it needs, and the dash says exactly that.
        return QStringLiteral("-");
    }

    QStringList parts;
    if (const QString needs = Needs(summary); not needs.isEmpty())
    {
        parts.append(needs);
    }

    // What it reads from the table counts too, and it counts as a **number**: a piece can read fifteen
    // measurements, and fifteen names would push everything else out of the cell. The names are in the details.
    if (not summary.measurements.isEmpty())
    {
        parts.append(QCoreApplication::translate("OPieceLibrary", "%n measurement(s)", nullptr,
                                                 static_cast<int>(summary.measurements.size())));
    }

    // Nothing at all is said rather than left blank: an empty cell reads like a piece nobody looked at.
    return parts.isEmpty() ? QCoreApplication::translate("OPieceLibrary", "Nothing")
                           : parts.join(QStringLiteral(", "));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Details(const Summary &summary) -> QString
{
    if (not summary.error.isEmpty())
    {
        return QCoreApplication::translate("OPieceLibrary", "This piece could not be read: %1").arg(summary.error);
    }

    QStringList lines{QCoreApplication::translate("OPieceLibrary", "From other pieces:")};
    if (summary.dependencies.isEmpty())
    {
        lines.append(QStringLiteral("    ")
                     + QCoreApplication::translate("OPieceLibrary",
                                                   "Nothing: everything it reads, it declares."));
    }
    else
    {
        for (const OPieceDocument::Dependency &dependency : summary.dependencies)
        {
            // The value, and then who declares it. When nothing does, the value is shown alone rather than hidden:
            // it is exactly the case the import will refuse, and it is better seen here than there.
            lines.append(dependency.origin.isEmpty()
                             ? QStringLiteral("    ")
                                   + QCoreApplication::translate("OPieceLibrary", "%1 - no piece declares it")
                                         .arg(dependency.token)
                             : QStringLiteral("    ") + dependency.token + QStringLiteral(" ← ")
                                   + dependency.origin);
        }
    }

    // And the table: a piece enters a pattern whose measurements have to define these, and this is where that is
    // seen before the refusal rather than as one.
    lines.append(QString());
    lines.append(QCoreApplication::translate("OPieceLibrary", "Measurements it reads:"));
    if (summary.measurements.isEmpty())
    {
        lines.append(QStringLiteral("    ") + QCoreApplication::translate("OPieceLibrary", "None."));
    }
    else
    {
        for (const QString &measurement : summary.measurements)
        {
            lines.append(QStringLiteral("    ") + measurement);
        }
    }

    return lines.join(QLatin1Char('\n'));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::SetNotes(const QString &filePath, const QString &notes, QString *error) -> bool
{
    OPieceDocument piece;
    if (not Load(filePath, &piece, error))
    {
        return false;
    }

    piece.SetNotes(notes);
    return Save(&piece, filePath, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibrary::Import(const QString &sourcePath, const QString &targetPath, QString *error) -> bool
{
    // Read first, copy after: a file that does not open, whose root is not a piece, or that the piece schema
    // refuses, must not land in the library — it would be a line the window then has to explain on every visit.
    QFile source(sourcePath);
    if (not source.open(QIODevice::ReadOnly))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OPieceLibrary", "This file is not a piece: %1")
                         .arg(source.errorString());
        }
        return false;
    }
    const QByteArray data = source.readAll();
    source.close();

    OPieceDocument piece;
    QString reason;
    if (not Load(sourcePath, &piece, &reason))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OPieceLibrary", "This file is not a piece: %1").arg(reason);
        }
        return false;
    }

    // And the schema, because a file can be well-formed XML and still not be a piece: this is the same bar the
    // library writes its own pieces with, so what enters is what it would have written.
    if (not VDomDocument::ValidateXMLData(piece.SaveSchema(), data, sourcePath, reason))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OPieceLibrary", "This piece does not match the piece format: %1")
                         .arg(reason);
        }
        return false;
    }

    QDir().mkpath(QFileInfo(targetPath).absolutePath());

    // The file keeps the name the reader gave it, and the piece keeps the name it was written under — the same
    // rule as Export. What binds the two is the identity inside the file, never a file name.
    if (not QFile::copy(sourcePath, targetPath))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OPieceLibrary", "The piece could not be copied to the library.");
        }
        return false;
    }

    return true;
}
