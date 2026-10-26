/************************************************************************
 **
 **  @file   preferencesfolderspage.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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

#include "preferencesfolderspage.h"

#include "../../core/oexporttemplatestorage.h"
#include "../../core/opiecelibrary.h"
#include "../../core/opatternlibrary.h"
#include "../../widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// How many files the folder holds, at any depth. Directories are not counted: the question asked is
/// about files, and a folder can legitimately hold a few empty ones.
auto FileCount(const QString &path) -> int
{
    QDirIterator it(path, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    int count = 0;
    while (it.hasNext())
    {
        it.next();
        ++count;
    }
    return count;
}

//---------------------------------------------------------------------------------------------------------------------
/// True when one of the two paths is inside the other. Copying a folder into its own subtree would
/// walk the files it is creating.
auto IsNested(const QString &a, const QString &b) -> bool
{
    const QString left = QDir::cleanPath(a);
    const QString right = QDir::cleanPath(b);
    return right.startsWith(left + QLatin1Char('/')) || left.startsWith(right + QLatin1Char('/'));
}

//---------------------------------------------------------------------------------------------------------------------
auto SubDirectories(const QString &path) -> QStringList
{
    QStringList dirs;
    QDirIterator it(path, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        dirs << it.next();
    }
    // Deepest first, so a folder is empty by the time it is removed.
    std::sort(dirs.begin(), dirs.end(),
              [](const QString &first, const QString &second) { return first.size() > second.size(); });
    return dirs;
}

//---------------------------------------------------------------------------------------------------------------------
/// Copy every file of `from` into `to`, then delete the copies from `from`.
///
/// **Nothing is removed before it has been copied**, and a file whose name already exists in the
/// destination is left alone (reported): a library folder is the user's work, and a silent overwrite
/// is the one mistake that cannot be undone. The source folder is removed only when it ends up empty.
auto MoveTree(const QString &from, const QString &to, QStringList *failed) -> int
{
    if (not QDir(from).exists())
    {
        return 0;
    }

    if (not QDir().mkpath(to))
    {
        *failed << to;
        return 0;
    }

    QStringList files;
    QDirIterator it(from, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        files << it.next();
    }

    QDir const source(from);
    QDir const target(to);
    QStringList copied;

    for (const QString &file : files)
    {
        const QString relative = source.relativeFilePath(file);
        const QString destination = target.filePath(relative);

        if (QFile::exists(destination) || not QDir().mkpath(QFileInfo(destination).absolutePath())
            || not QFile::copy(file, destination))
        {
            *failed << relative;
            continue;
        }

        copied << file;
    }

    for (const QString &file : copied)
    {
        QFile::remove(file);
    }

    const QStringList dirs = SubDirectories(from);
    for (const QString &dir : dirs)
    {
        QDir().rmdir(dir); // Only when empty.
    }
    QDir().rmdir(from);

    return static_cast<int>(copied.size());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
PreferencesFoldersPage::PreferencesFoldersPage(QWidget *parent)
  : QWidget(parent)
{
    BuildFolders();
    BuildRows();
    RetranslateUi();
    Reload();
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesFoldersPage::~PreferencesFoldersPage() = default;

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesFoldersPage::Apply() -> QStringList
{
    for (int i = 0; i < m_folders.size(); ++i)
    {
        Folder &folder = m_folders[i];
        const QString previous = folder.get();
        const bool move = folder.moveContents;
        const QString wanted = folder.pending;

        folder.moveContents = false;
        folder.pending.clear();

        if (wanted.isEmpty() || wanted == previous)
        {
            continue;
        }

        folder.set(wanted);

        // The files come after the setting, never before: a move that fails has already been
        // reported, and the path itself is what the application remembers.
        if (move)
        {
            QStringList failed;
            MoveTree(previous, wanted, &failed);
            if (not failed.isEmpty())
            {
                QMessageBox::warning(
                    this, tr("Files left behind"),
                    tr("%n file(s) could not be moved to « %1 » and stay in the old folder.", "",
                           static_cast<int>(failed.size()))
                        .arg(QDir::toNativeSeparators(wanted)),
                    QMessageBox::Ok);
            }
        }

        // Then the folder is brought back to the state the application expects from it: created,
        // seeded with its default template, or with whatever it had loaded dropped. None of it
        // needs a restart, so nothing is reported back to the dialog.
        if (folder.afterChange)
        {
            folder.afterChange();
        }
    }

    Reload();
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::BuildFolders()
{
    // Every folder under ~/Ostara that the application reads or writes. Order follows the maker,
    // not the alphabet: what is used daily first, the resources of the cartouche after.
    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Measurements"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage",
                                              "Measurements you build and reuse in your patterns."),
                            QStringLiteral("Measurements"),
                            []() -> QString
                            { return VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathMeasurementLibrary(path); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Pieces"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Pieces kept to be reused in other patterns."),
                            QStringLiteral("Pieces"),
                            []() -> QString { return VAbstractApplication::VApp()->Settings()->GetPathPieces(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathPieces(path); },
                            []() { OPieceLibrary::EnsureDirectory(); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Patterns"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Where Open and Save start."),
                            QStringLiteral("Patterns"),
                            []() -> QString { return VAbstractApplication::VApp()->Settings()->GetPathPatterns(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathPatterns(path); },
                            []() { OPatternLibrary::EnsureDirectory(); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Collections"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Collections of pieces."),
                            QStringLiteral("Collections"),
                            []() -> QString { return VAbstractApplication::VApp()->Settings()->GetPathCollections(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathCollections(path); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Clients"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Clients a cartouche can name."),
                            QStringLiteral("Clients"),
                            []() -> QString { return VAbstractApplication::VApp()->Settings()->GetPathClients(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathClients(path); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Seam allowances"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Seam allowance values kept for reuse."),
                            QStringLiteral("SeamAllowances"),
                            []() -> QString
                            { return VAbstractApplication::VApp()->Settings()->GetPathSeamAllowances(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathSeamAllowances(path); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Symbols"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Symbols to place on a piece."),
                            QStringLiteral("Symbols"),
                            []() -> QString { return VAbstractApplication::VApp()->Settings()->GetPathSymbols(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathSymbols(path); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Annotation styles"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Styles of the pattern annotations."),
                            QStringLiteral("AnnotationStyles"),
                            []() -> QString
                            { return VAbstractApplication::VApp()->Settings()->GetPathAnnotationStyles(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathAnnotationStyles(path); },
                            []() { OPieceCartoucheStorage::InvalidateAnnotationStyles(); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Cartouche templates"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Cartouche templates (.oct)."),
                            QStringLiteral("CartoucheTemplates"),
                            []() -> QString
                            { return VAbstractApplication::VApp()->Settings()->GetPathCartoucheTemplates(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathCartoucheTemplates(path); },
                            []() { OPieceCartoucheStorage::EnsureDefaultTemplate(); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "PDF templates"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Templates of the exported PDF."),
                            QStringLiteral("PDFTemplate"),
                            []() -> QString
                            { return VAbstractApplication::VApp()->Settings()->GetPathPdfTemplates(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathPdfTemplates(path); },
                            []() { OExportTemplateStorage::EnsureDefaultTemplate(); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Cover page templates"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Cover pages of the exports."),
                            QStringLiteral("CoverPageTemplate"),
                            []() -> QString
                            { return VAbstractApplication::VApp()->Settings()->GetPathCoverPageTemplates(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathCoverPageTemplates(path); }});

    m_folders.append(Folder{QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Logos"),
                            QT_TRANSLATE_NOOP("PreferencesFoldersPage", "Logos to place in the cartouche."),
                            QStringLiteral("Logos"),
                            []() -> QString { return VAbstractApplication::VApp()->Settings()->GetPathLogos(); },
                            [](const QString &path)
                            { VAbstractApplication::VApp()->Settings()->SetPathLogos(path); }});
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::BuildRows()
{
    auto *layout = new QVBoxLayout(this);

    m_group = new QGroupBox(this);
    auto *grid = new QGridLayout(m_group);
    grid->setColumnStretch(1, 1);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(6);

    for (int i = 0; i < m_folders.size(); ++i)
    {
        Folder &folder = m_folders[i];

        folder.nameLabel = new QLabel(m_group);

        folder.pathEdit = new QLineEdit(m_group);
        folder.pathEdit->setReadOnly(true);
        folder.pathEdit->setCursorPosition(0);
        folder.pathEdit->installEventFilter(this);

        folder.changeButton = new QToolButton(m_group);
        folder.changeButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
        folder.changeButton->setAutoRaise(true);
        connect(folder.changeButton, &QToolButton::clicked, this, [this, i]() { Choose(i); });

        grid->addWidget(folder.nameLabel, i, 0);
        grid->addWidget(folder.pathEdit, i, 1);
        grid->addWidget(folder.changeButton, i, 2);
    }

    layout->addWidget(m_group);

    auto *bottom = new QHBoxLayout;
    m_defaultsButton = new QPushButton(this);
    connect(m_defaultsButton, &QPushButton::clicked, this, &PreferencesFoldersPage::RestoreDefaults);
    bottom->addWidget(m_defaultsButton);
    bottom->addStretch();
    layout->addLayout(bottom);
    layout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::Reload()
{
    for (Folder &folder : m_folders)
    {
        folder.pending.clear();
        folder.moveContents = false;

        const QString path = folder.get();
        folder.pathEdit->setText(DisplayPath(path));
        folder.pathEdit->setCursorPosition(0);
        folder.pathEdit->setToolTip(QDir::toNativeSeparators(path));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::Choose(int row)
{
    Folder &folder = m_folders[row];
    const QString current = folder.pending.isEmpty() ? folder.get() : folder.pending;

    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Choose the « %1 » folder").arg(folder.nameLabel->text()),
                                                            current, QFileDialog::ShowDirsOnly);
    if (chosen.isEmpty())
    {
        return;
    }

    const QString path = QDir::cleanPath(chosen);
    if (path == current)
    {
        return;
    }

    RowChanged(row, path);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::RowChanged(int row, const QString &path)
{
    Folder &folder = m_folders[row];
    const QString previous = folder.get();

    folder.moveContents = false;
    folder.pending = path;

    folder.pathEdit->setText(DisplayPath(path));
    folder.pathEdit->setCursorPosition(0);
    folder.pathEdit->setToolTip(QDir::toNativeSeparators(path));

    if (IsNested(previous, path))
    {
        QMessageBox::warning(this, tr("Folder inside the other"),
                             tr("The two folders are inside one another. Choose a folder that does not "
                                "contain « %1 », nor is contained by it.")
                                 .arg(QDir::toNativeSeparators(previous)),
                             QMessageBox::Ok);
        return;
    }

    const int files = FileCount(previous);
    if (files == 0)
    {
        return;
    }

    QMessageBox box(QMessageBox::Question, tr("Move the files"),
                    tr("The « %1 » folder holds %n file(s). Move them into the new folder?", "", files)
                        .arg(folder.nameLabel->text()),
                    QMessageBox::NoButton, this);
    box.setInformativeText(tr("From %1\nto %2").arg(QDir::toNativeSeparators(previous), QDir::toNativeSeparators(path)));
    box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::Yes);

    folder.moveContents = (box.exec() == QMessageBox::Yes);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::RestoreDefaults()
{
    for (int i = 0; i < m_folders.size(); ++i)
    {
        const QString path = DefaultPath(m_folders.at(i));
        if (path != m_folders.at(i).get())
        {
            RowChanged(i, path);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::OpenInFinder(int row)
{
    const Folder &folder = m_folders.at(row);
    const QString path = folder.pending.isEmpty() ? folder.get() : folder.pending;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesFoldersPage::DefaultPath(const Folder &folder) const -> QString
{
    return QDir::homePath() + QLatin1Char('/') + QStringLiteral("Ostara") + QLatin1Char('/') + folder.name;
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesFoldersPage::DisplayPath(const QString &path) const -> QString
{
    const QString home = QDir::homePath();

    if (path == home)
    {
        return QStringLiteral("~");
    }

    if (path.startsWith(home + QLatin1Char('/')))
    {
        return QStringLiteral("~") + path.mid(home.size());
    }

    return QDir::toNativeSeparators(path);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::RetranslateUi()
{
    m_group->setTitle(tr("Folders"));
    m_defaultsButton->setText(tr("Restore defaults"));

    for (Folder &folder : m_folders)
    {
        folder.nameLabel->setText(QCoreApplication::translate("PreferencesFoldersPage", folder.label));
        folder.nameLabel->setToolTip(QCoreApplication::translate("PreferencesFoldersPage", folder.hint));
        folder.changeButton->setText(tr("Change…"));
        folder.changeButton->setToolTip(tr("Choose another folder"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesFoldersPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUi();
    }

    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
bool PreferencesFoldersPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick)
    {
        for (int i = 0; i < m_folders.size(); ++i)
        {
            if (watched == m_folders.at(i).pathEdit)
            {
                OpenInFinder(i);
                return true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}
