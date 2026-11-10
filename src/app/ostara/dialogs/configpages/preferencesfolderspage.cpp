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
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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
#include "../../core/olibrarystorage.h"
#include "../../widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QEvent>
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

        // The move comes first, and it is all or nothing. A setting that has already switched cannot
        // be taken back, and a library left behind while the application looks elsewhere is a state
        // no interface can repair — so a refused move writes nothing at all.
        if (move)
        {
            QStringList blocked;
            if (not OLibraryStorage::MoveFolderContents(previous, wanted, &blocked))
            {
                QMessageBox::warning(
                    this, tr("Nothing was moved"),
                    tr("The « %1 » folder could not be moved to « %2 »: %3 could not be copied. "
                       "Nothing changed — the folder stays where it is, with all its files.")
                        .arg(folder.nameLabel->text(), QDir::toNativeSeparators(wanted), blocked.constFirst()),
                    QMessageBox::Ok);
                continue; // the setting keeps the old path
            }
        }

        folder.set(wanted);

        // The setting is read back: QSettings::sync() says nothing, and a refused write (folder that only
        // allows reading, quota, another program holding the ini) would otherwise leave the files moved and
        // the library still pointing at the old path — the very "emptied library" this work removes. The
        // files are already in the new folder at this point, so the message has to say where they are.
        if (QDir::cleanPath(folder.get()) != QDir::cleanPath(wanted))
        {
            QMessageBox::warning(
                this, tr("The change could not be saved"),
                tr("The « %1 » folder was moved, but Ostara could not remember the new path: %2. The files "
                   "are in the new folder; copy the path above into the preferences again, or check that "
                   "the settings file can be written.")
                    .arg(folder.nameLabel->text(), QDir::toNativeSeparators(wanted)),
                QMessageBox::Ok);
            continue;
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

    // Back to the folder the library already has: there is nothing to ask, nothing to remember, and above
    // all nothing to refuse — two spellings of one folder are not nested inside each other. Reachable by
    // choosing another folder, then changing one's mind and picking the original again.
    if (QDir::cleanPath(previous) == QDir::cleanPath(path))
    {
        folder.moveContents = false;
        folder.pending.clear();
        folder.pathEdit->setText(DisplayPath(previous));
        folder.pathEdit->setCursorPosition(0);
        folder.pathEdit->setToolTip(QDir::toNativeSeparators(previous));
        return;
    }

    // The guard comes BEFORE anything is remembered. It used to warn and return with the forbidden
    // path already stored, so Apply then moved the library into the folder it had just refused.
    if (OLibraryStorage::IsNested(previous, path))
    {
        folder.moveContents = false;
        folder.pending.clear();
        folder.pathEdit->setText(DisplayPath(previous));
        folder.pathEdit->setCursorPosition(0);
        folder.pathEdit->setToolTip(QDir::toNativeSeparators(previous));

        QMessageBox::warning(this, tr("Folder inside the other"),
                             tr("The two folders are inside one another. Choose a folder that does not "
                                "contain « %1 », nor is contained by it.")
                                 .arg(QDir::toNativeSeparators(previous)),
                             QMessageBox::Ok);
        return;
    }

    folder.moveContents = false;
    folder.pending = path;

    folder.pathEdit->setText(DisplayPath(path));
    folder.pathEdit->setCursorPosition(0);
    folder.pathEdit->setToolTip(QDir::toNativeSeparators(path));

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
