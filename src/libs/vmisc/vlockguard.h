/************************************************************************
 **
 **  @file   VLockGuard.h
 **  @author Alex Zaharov <alexzkhr@gmail.com>
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   14 9, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef VLOCKGUARD_H
#define VLOCKGUARD_H

#include <QtGlobal>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif /*Q_OS_WIN*/

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QLockFile>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QString>
#include <cstdint>

/*@brief
 * This class creates Guarded object if and only if lock file taken. It keeps shared_ptr to object and lock-file.
 * Can use optional object allocator and deleter.
 *
 * On older Qt lock assumed always taken and compile-time warning is shown.
 *
 */
template <typename Guarded> class VLockGuard
{
public:
    explicit VLockGuard(const QString &lockName, int stale = 0, int timeout = 0);
    ~VLockGuard() = default;

    template <typename Alloc> VLockGuard(const QString &lockName, Alloc a, int stale = 0, int timeout = 0);

    template <typename Alloc, typename Delete>
    VLockGuard(const QString &lockName, Alloc a, Delete d, int stale = 0, int timeout = 0);

    auto GetProtected() const -> const QSharedPointer<Guarded> &;
    auto GetLockError() const -> int;
    auto IsLocked() const -> bool;
    void Unlock();
    auto GetLockFile() const -> QString;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VLockGuard) // NOLINT

    QSharedPointer<Guarded> holder{};
    int lockError{0};
    QString lockFile{};
    QSharedPointer<QLockFile> lock{};

    auto TryLock(const QString &lockName, int stale, int timeout) -> bool;
    auto TryLockFile(const QString &path, int stale, int timeout) -> bool;
    void MarkLockFileHidden() const;

    // Usual location: next to the guarded file, hidden on Unix. Every window of the application
    // looks there first, so it stays the first choice.
    static auto PrimaryLockFile(const QString &lockName) -> QString;
    // Last resort when the guarded file's folder cannot take a new file. Keyed on the canonical path
    // of the guarded file so that every window of this user still designates the same pattern.
    static auto FallbackLockFile(const QString &lockName) -> QString;
};

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> VLockGuard<Guarded>::VLockGuard(const QString &lockName, int stale, int timeout)
{
    if (TryLock(lockName, stale, timeout))
    {
        holder.reset(new Guarded());
    }
}

//---------------------------------------------------------------------------------------------------------------------
// using allocator lambdas seems logically better than supplying pointer, because we will take ownership of allocated
// object
template <typename Guarded>
template <typename Alloc>
VLockGuard<Guarded>::VLockGuard(const QString &lockName, Alloc a, int stale, int timeout)
{
    if (TryLock(lockName, stale, timeout))
    {
        holder.reset(a());
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded>
template <typename Alloc, typename Delete>
VLockGuard<Guarded>::VLockGuard(const QString &lockName, Alloc a, Delete d, int stale, int timeout)
{
    if (TryLock(lockName, stale, timeout))
    {
        holder.reset(a(), d);
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> inline auto VLockGuard<Guarded>::GetProtected() const -> const QSharedPointer<Guarded> &
{
    return holder;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> inline auto VLockGuard<Guarded>::GetLockError() const -> int
{
    return lockError;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> inline auto VLockGuard<Guarded>::IsLocked() const -> bool
{
    return !holder.isNull();
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> inline void VLockGuard<Guarded>::Unlock()
{
    if (IsLocked())
    {
        lock->unlock();
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> auto VLockGuard<Guarded>::GetLockFile() const -> QString
{
    return lockFile;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> auto VLockGuard<Guarded>::TryLock(const QString &lockName, int stale, int timeout) -> bool
{
    if (TryLockFile(PrimaryLockFile(lockName), stale, timeout))
    {
        MarkLockFileHidden();
        return true;
    }

    // A file can be perfectly readable while its folder refuses the creation of a new one: read-only
    // medium, cloud folder in a bad state, network share, folder whose ACLs deny creation... Opening
    // only reads the guarded file, so a lock that cannot be written there must not stop the open.
    // A real conflict is something else and is still reported.
    if (lockError == QLockFile::LockFailedError)
    {
        return false;
    }

    if (TryLockFile(FallbackLockFile(lockName), stale, timeout))
    {
        MarkLockFileHidden();
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded>
auto VLockGuard<Guarded>::TryLockFile(const QString &path, int stale, int timeout) -> bool
{
    lock.reset(new QLockFile(path));

    lock->setStaleLockTime(stale);
    lock->tryLock(timeout);

    if (QLockFile::LockFailedError == lock->error())
    {
        // This happens if a stale lock file exists and another process uses that PID.
        // Try removing the stale file, which will fail if a real process is holding a
        // file-level lock. A false error is more problematic than not locking properly
        // on corner-case systems.
        lock->removeStaleLockFile();
        lock->tryLock(timeout);
    }

    lockError = lock->error();
    if (lockError != QLockFile::NoError)
    {
        lock.reset();
        return false;
    }

    lockFile = path;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> auto VLockGuard<Guarded>::PrimaryLockFile(const QString &lockName) -> QString
{
    QString path = lockName + QLatin1String(".lock");
#if defined(Q_OS_UNIX) || defined(Q_OS_MACOS)
    {
        QFileInfo const info(path);
        path = info.absolutePath() + QLatin1String("/.") + info.fileName();
    }
#endif
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename Guarded> auto VLockGuard<Guarded>::FallbackLockFile(const QString &lockName) -> QString
{
    QFileInfo const info(lockName);

    // The same pattern has to give the same guard file whatever the spelling of the path used to
    // open it (symlink, case, relative form).
    QString target = info.canonicalFilePath();
    if (target.isEmpty())
    {
        target = info.absoluteFilePath();
    }

    const QByteArray digest = QCryptographicHash::hash(target.toUtf8(), QCryptographicHash::Sha256).toHex();

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (tempDir.isEmpty())
    {
        tempDir = QDir::tempPath();
    }

    return QDir(tempDir).filePath(QStringLiteral("ostara-%1.lock").arg(QString::fromLatin1(digest.left(32))));
}

//---------------------------------------------------------------------------------------------------------------------
#if defined(Q_OS_WIN)
template <typename Guarded> inline void VLockGuard<Guarded>::MarkLockFileHidden() const
{
    SetFileAttributesW(lockFile.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
}
#else
template <typename Guarded> inline void VLockGuard<Guarded>::MarkLockFileHidden() const
{
}
#endif

// use pointer and function below to persistent things like class-member, because lock is taken by constructor
// helper functions allow to write shorter creating and setting new lock-pointer

QT_WARNING_PUSH
QT_WARNING_DISABLE_INTEL(1418)

template <typename Guarded>
void VlpCreateLock(QSharedPointer<VLockGuard<Guarded>> &r, const QString &lockName, int stale = 0, int timeout = 0)
{
    r.reset(new VLockGuard<Guarded>(lockName, stale, timeout));
}

template <typename Guarded, typename Alloc>
void VlpCreateLock(QSharedPointer<VLockGuard<Guarded>> &r, const QString &lockName, Alloc a, int stale = 0,
                   int timeout = 0)
{
    r.reset(new VLockGuard<Guarded>(lockName, a, stale, timeout));
}

template <typename Guarded, typename Alloc, typename Del>
void VlpCreateLock(QSharedPointer<VLockGuard<Guarded>> &r, const QString &lockName, Alloc a, Del d, int stale = 0,
                   int timeout = 0)
{
    r.reset(new VLockGuard<Guarded>(lockName, a, d, stale, timeout));
}

QT_WARNING_POP

#endif // VLOCKGUARD_H
