/************************************************************************
 **
 **  @file   opiecelibrarydialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The pieces saved in the library, sorted into folders: what each of them asks for, what the reader
 **          wrote about it, and the gestures that manage them — insert one, move one, copy one out, delete one.
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

#include "opiecelibrarydialog.h"

#include "olibraryfolderdialog.h"

#include "FactoryWidgets.h"
#include "vtheme.h"
#include "widgets/olibraryicondelegate.h"
#include "widgets/olibrarytree.h"

#include <QApplication>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
const int kNameColumn = 0;
const int kNeedsColumn = 1;
const int kUnitColumn = 2;
const int kModifiedColumn = 3;
const int kTransferColumn = 4;
const int kExportColumn = 5;
const int kDeleteColumn = 6;
const int kColumnCount = 7;

const int kActionColumnWidth = 44;
const int kIconSize = 20;
const int kRowHeight = 36;
const int kTreeWidth = 240;
const int kSideWidth = 320;

/// A moment after the typing stops: the list is rebuilt rather than filtered in place, and a rebuild per letter
/// would flicker under the hand.
const int kSearchDelay = 300;

/// The same, for the note: a file written per letter would be a file validated per letter, and a `.opiece` is a
/// whole document.
const int kNotesCommitDelay = 600;

const QString kDateLayout = QStringLiteral("dd/MM/yyyy HH:mm");
const QString kDash = QStringLiteral("-");
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPieceLibraryDialog::OPieceLibraryDialog(const QString &directory, QWidget *parent)
    : QDialog(parent), m_directory(directory)
{
    SetupUI();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::SetupUI()
{
    setMinimumSize(980, 560);
    resize(1180, 680);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // --- Search ---
    auto *searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    m_searchLabel = new QLabel();
    searchRow->addWidget(m_searchLabel);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setClearButtonEnabled(true);
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_searchEdit->addAction(QIcon(OLibraryIconPath(QStringLiteral("magnifying-glass"), dark)),
                            QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OPieceLibraryDialog::OnSearchChanged);
    searchRow->addWidget(m_searchEdit, 1);

    mainLayout->addLayout(searchRow);

    // --- Creating a folder is a gesture of the library, not of a row: it lives under the search ---
    auto *folderRow = new QHBoxLayout();
    folderRow->setSpacing(8);

    m_createFolderButton = new QPushButton();
    connect(m_createFolderButton, &QPushButton::clicked, this, &OPieceLibraryDialog::OnCreateFolder);
    folderRow->addWidget(m_createFolderButton);
    folderRow->addStretch(1);

    mainLayout->addLayout(folderRow);

    // --- The folders, the pieces of the folder being read, and what the chosen piece says ---
    auto *splitter = new QSplitter(Qt::Horizontal);

    m_tree = new QTreeWidget();
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setUniformRowHeights(true);
    m_tree->setIconSize(QSize(kIconSize, kIconSize));
    // The name the theme's branch carets are drawn for: the same one the patterns' tree wears.
    m_tree->setObjectName(QStringLiteral("libraryTree"));
    m_tree->setItemDelegate(new OLibraryRowDelegate(m_tree));
    m_tree->setMinimumWidth(140);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &OPieceLibraryDialog::OnFolderSelected);
    splitter->addWidget(m_tree);

    m_table = new QTableWidget();
    m_table->setColumnCount(kColumnCount);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    m_table->horizontalHeader()->setSectionResizeMode(kNameColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kNeedsColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(kUnitColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kModifiedColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kTransferColumn, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(kExportColumn, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(kDeleteColumn, QHeaderView::Fixed);
    m_table->setColumnWidth(kTransferColumn, kActionColumnWidth);
    m_table->setColumnWidth(kExportColumn, kActionColumnWidth);
    m_table->setColumnWidth(kDeleteColumn, kActionColumnWidth);
    m_table->setItemDelegateForColumn(kTransferColumn, new OLibraryIconDelegate(m_table));
    m_table->setItemDelegateForColumn(kExportColumn, new OLibraryIconDelegate(m_table));
    m_table->setItemDelegateForColumn(kDeleteColumn, new OLibraryIconDelegate(m_table));
    m_table->setMinimumWidth(560);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &OPieceLibraryDialog::ShowDetails);
    connect(m_table, &QTableWidget::cellClicked, this, &OPieceLibraryDialog::OnCellClicked);
    splitter->addWidget(m_table);

    auto *side = new QWidget();
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(8);

    // The state of the list, when it has nothing to show: an empty list beside two empty panels reads like a
    // window that is broken, and saying which of the three empties it is is the whole difference.
    m_stateLabel = new QLabel();
    m_stateLabel->setWordWrap(true);
    m_stateLabel->setAlignment(Qt::AlignCenter);
    sideLayout->addWidget(m_stateLabel);

    m_needsGroup = new QGroupBox();
    auto *needsLayout = new QVBoxLayout(m_needsGroup);

    m_needsLabel = new QLabel();
    m_needsLabel->setObjectName(QStringLiteral("descLabel"));
    m_needsLabel->setWordWrap(true);
    m_needsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_needsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);

    // A piece can read a lot — six values of another piece and fifteen measurements — and the list must be
    // readable rather than clipped: it scrolls inside its group.
    auto *needsScroll = new QScrollArea();
    needsScroll->setWidgetResizable(true);
    needsScroll->setFrameShape(QFrame::NoFrame);
    needsScroll->setWidget(m_needsLabel);
    needsLayout->addWidget(needsScroll);
    sideLayout->addWidget(m_needsGroup, 1);

    m_notesGroup = new QGroupBox();
    auto *notesLayout = new QVBoxLayout(m_notesGroup);

    m_notesEdit = new QPlainTextEdit();
    m_notesEdit->setTabChangesFocus(true);
    connect(m_notesEdit, &QPlainTextEdit::textChanged, this, &OPieceLibraryDialog::OnNotesChanged);
    notesLayout->addWidget(m_notesEdit);
    sideLayout->addWidget(m_notesGroup, 1);

    splitter->addWidget(side);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({kTreeWidth, 640, kSideWidth});

    mainLayout->addWidget(splitter, 1);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(kSearchDelay);
    connect(m_searchTimer, &QTimer::timeout, this, &OPieceLibraryDialog::OnSearchTimeout);

    m_notesTimer = new QTimer(this);
    m_notesTimer->setSingleShot(true);
    m_notesTimer->setInterval(kNotesCommitDelay);
    connect(m_notesTimer, &QTimer::timeout, this, &OPieceLibraryDialog::CommitNotes);

    // --- The gestures, and the two that manage the library are on the left ---
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);

    m_openFolderButton = new QPushButton();
    connect(m_openFolderButton, &QPushButton::clicked, this, &OPieceLibraryDialog::OnOpenFolder);
    buttonRow->addWidget(m_openFolderButton);

    m_importButton = new QPushButton();
    connect(m_importButton, &QPushButton::clicked, this, &OPieceLibraryDialog::OnImportPiece);
    buttonRow->addWidget(m_importButton);

    buttonRow->addStretch(1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // Inserting is what the window is for: the button is the gesture, and it stays down while the chosen piece
    // cannot be read — nothing is offered that would fail afterwards.
    m_insertButton = m_buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    connect(m_insertButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonRow->addWidget(m_buttonBox);

    mainLayout->addLayout(buttonRow);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibraryDialog::SelectedPath() const -> QString
{
    return m_selectedPath;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::Retranslate()
{
    setWindowTitle(tr("Piece library"));

    m_searchLabel->setText(tr("Search:"));
    m_searchEdit->setPlaceholderText(tr("Search pieces…"));
    m_createFolderButton->setText(tr("Create a folder"));

    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Needs"), tr("Unit"), tr("Modified"), QString(), QString(),
                                        QString()});
    m_insertButton->setText(tr("Insert"));
    m_openFolderButton->setText(tr("Open folder"));
    m_importButton->setText(tr("Import a piece"));

    m_needsGroup->setTitle(tr("Needs"));
    m_notesGroup->setTitle(tr("Notes"));
    m_notesEdit->setPlaceholderText(tr("What you want to remember about this piece…"));

    // The tree is rebuilt rather than re-said: its folder lines are file names, and the only word in it is the
    // icon. Doing it here keeps a language change from leaving a window whose tree was never filled.
    PopulateTree();

    // The panels belong to the selection: re-saying what the selected piece reads and what was written about it,
    // in the language the window is now speaking.
    ShowDetails();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::done(int result)
{
    // What the reader is typing is written before the window goes: a pending save is not a reason to lose a note.
    CommitPendingNotes();

    QDialog::done(result);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::CommitPendingNotes()
{
    if (m_notesTimer != nullptr && m_notesTimer->isActive())
    {
        m_notesTimer->stop();
        CommitNotes();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::Refresh()
{
    // The list is about to be rebuilt: a note still waiting is written first, while the row it belongs to still
    // means the piece it was typed for.
    CommitPendingNotes();

    // Every piece is read once, folders walked: the search then reads this list instead of the disk, and typing
    // a letter never re-reads a library.
    m_all = OPieceLibrary::AllSummaries(m_directory);

    PopulateTree();
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::PopulateTree()
{
    // Rebuilding the tree clears the selection, and that would read as "the reader chose the library's own
    // folder": the signals are cut for the length of the rebuild, and the folder is put back before the list is
    // read.
    const QSignalBlocker blocker(m_tree);

    // The library's own folder is the first line, like everywhere else this tree is shown: it is what the window
    // is standing in when no folder is chosen, and the only line to click to come back up from a subfolder.
    OLibraryTree::Fill(m_tree, m_directory, tr("The library's own folder"),
                       OLibraryIcon(QStringLiteral("folder-open")));

    // The window always stands on a line — the chosen folder, or the library's own when none is chosen. A list
    // whose folder no line points at would leave the reader guessing where the pieces come from.
    if (not OLibraryTree::Select(m_tree, ActiveFolder()))
    {
        // The chosen folder is gone: the library's own folder takes over rather than the window insisting on a
        // place that no longer exists.
        m_activeFolder.clear();
        OLibraryTree::Select(m_tree, m_directory);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibraryDialog::ActiveFolder() const -> QString
{
    return m_activeFolder.isEmpty() ? m_directory : m_activeFolder;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnFolderSelected()
{
    m_activeFolder = OLibraryTree::SelectedPath(m_tree);

    // Two gestures say what the list shows: the folder chosen in the tree, and the search. The last one speaks —
    // choosing a folder drops the search, without which the click would look like it did nothing at all. The
    // timer is stopped after the clearing, so the folder is read once and not once more 300 ms later.
    if (not m_searchEdit->text().isEmpty())
    {
        m_searchEdit->clear();
        m_searchTimer->stop();
    }

    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnSearchChanged(const QString &text)
{
    Q_UNUSED(text)
    m_searchTimer->start();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnSearchTimeout()
{
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnCreateFolder()
{
    OLibraryFolderDialog dialog(ActiveFolder(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    // The folder is made in the folder being read, and becomes the folder being read: a folder created and not
    // shown would leave the reader wondering whether it exists at all.
    const QString created = dialog.CreatedPath();

    PopulateTree();
    OLibraryTree::Select(m_tree, created); // the selection tells OnFolderSelected to read it
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::Populate()
{
    // A note still being typed belongs to the piece it was typed for, and the rows are about to mean other
    // pieces: it is written while the old list still says which one it was.
    CommitPendingNotes();

    const QString needle = m_searchEdit->text().trimmed();

    // Two states, and the tree has to say which one is on: while the search is showing the whole library, no
    // folder is the one being read — leaving one highlighted would have the tree claim what the list denies.
    // The signals are cut, because clearing a selection is not the reader choosing the library's own folder.
    {
        const QSignalBlocker blocker(m_tree);
        if (needle.isEmpty())
        {
            OLibraryTree::Select(m_tree, ActiveFolder()); // the choice comes back where it was
        }
        else
        {
            m_tree->setCurrentItem(nullptr);
        }
    }

    // The list is built from the one reading the window already did: the folder's own pieces, or every piece
    // whose name, needs or unit matches. A piece can be in a folder the reader is not looking at, and that is
    // exactly what the search is asking for.
    const QString folder = ActiveFolder();
    m_summaries.clear();
    m_summaries.reserve(m_all.size());
    for (const OPieceLibrary::Summary &summary : m_all)
    {
        if (needle.isEmpty())
        {
            if (QFileInfo(summary.path).absolutePath() == folder)
            {
                m_summaries.append(summary);
            }
            continue;
        }

        if (summary.name.contains(needle, Qt::CaseInsensitive)
            || OPieceLibrary::NeedsLine(summary).contains(needle, Qt::CaseInsensitive)
            || summary.notes.contains(needle, Qt::CaseInsensitive)
            || summary.unit.contains(needle, Qt::CaseInsensitive))
        {
            m_summaries.append(summary);
        }
    }

    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(m_summaries.size()));

    for (int row = 0; row < m_summaries.size(); ++row)
    {
        const OPieceLibrary::Summary &summary = m_summaries.at(row);

        auto *nameItem = new QTableWidgetItem(summary.name);
        nameItem->setData(Qt::UserRole, summary.path);
        if (not summary.error.isEmpty())
        {
            nameItem->setToolTip(summary.error);
        }
        else if (not needle.isEmpty())
        {
            // The search took it from somewhere else: saying where is the difference between a result and a
            // mystery.
            nameItem->setToolTip(QFileInfo(summary.path).absolutePath());
        }
        m_table->setItem(row, kNameColumn, nameItem);

        auto *needsItem = new QTableWidgetItem(OPieceLibrary::NeedsLine(summary));
        needsItem->setToolTip(OPieceLibrary::Details(summary));
        m_table->setItem(row, kNeedsColumn, needsItem);

        m_table->setItem(row, kUnitColumn, new QTableWidgetItem(summary.unit));

        const QString modified = summary.modified.isValid() ? summary.modified.toString(kDateLayout) : kDash;
        m_table->setItem(row, kModifiedColumn, new QTableWidgetItem(modified));

        // The three gestures of the row, as icons — the same pair the patterns library offers.
        auto *transferItem = new QTableWidgetItem(OLibraryIcon(QStringLiteral("file-arrow-up")), QString());
        transferItem->setToolTip(tr("Transfer to another folder"));
        m_table->setItem(row, kTransferColumn, transferItem);

        auto *exportItem = new QTableWidgetItem(OLibraryIcon(QStringLiteral("floppy-disk")), QString());
        exportItem->setToolTip(tr("Export"));
        m_table->setItem(row, kExportColumn, exportItem);

        auto *deleteItem = new QTableWidgetItem(OLibraryIcon(QStringLiteral("trash")), QString());
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(row, kDeleteColumn, deleteItem);
    }

    // A piece is shown selected from the start: two empty panels beside a full list would make the window look
    // like it has nothing to say.
    if (m_table->rowCount() > 0)
    {
        m_table->selectRow(0);
    }

    ShowDetails();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibraryDialog::SelectedRow() const -> int
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_summaries.size())
    {
        return -1;
    }

    return row;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::FollowSelection()
{
    const int row = SelectedRow();

    m_selectedPath = row >= 0 ? m_summaries.at(row).path : QString();
    // A file the library cannot read is shown, and said, and never offered: inserting it could only fail, and the
    // one who reads the message would have to guess why.
    m_insertButton->setEnabled(row >= 0 && m_summaries.at(row).error.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::ShowDetails()
{
    if (m_needsLabel == nullptr)
    {
        return;
    }

    // One entry point for the selection: what the two panels say and what the Insert button offers are three faces
    // of the same choice, and all of them follow every path that changes it — a click, a search, a refresh.
    FollowSelection();

    const int row = SelectedRow();

    // The note on screen belongs to the piece that **was** selected: it is written before the field is given the
    // note of another piece — or it would be lost, and written under a name that is not its own.
    if (m_notesTimer->isActive())
    {
        m_notesTimer->stop();
        CommitNotes();
    }

    m_loadingNotes = true;
    m_notesEdit->setPlainText(row >= 0 ? m_summaries.at(row).notes : QString());
    m_loadingNotes = false;
    m_notesRow = row;

    if (row < 0)
    {
        // No choice yet — which is also what an empty library looks like, what a folder holding nothing looks
        // like, and what a search that matches nothing looks like. Saying which of the three it is is the
        // difference between a window that answers and a window that looks broken.
        if (m_all.isEmpty())
        {
            m_stateLabel->setText(tr("No piece is saved in the library yet."));
        }
        else if (not m_searchEdit->text().trimmed().isEmpty())
        {
            m_stateLabel->setText(tr("No piece matches the search."));
        }
        else
        {
            m_stateLabel->setText(tr("This folder holds no piece."));
        }
        m_stateLabel->setVisible(true);

        m_needsLabel->clear();
        m_notesEdit->setEnabled(false);
        m_notesEdit->setPlaceholderText(QString());
        return;
    }

    m_stateLabel->clear();
    m_stateLabel->setVisible(false);

    m_needsLabel->setText(OPieceLibrary::Details(m_summaries.at(row)));

    // A file nobody can read has no note to write: offering the field would invite the reader to replace a file
    // the library refuses to open.
    const bool readable = m_summaries.at(row).error.isEmpty();
    m_notesEdit->setEnabled(readable);
    m_notesEdit->setPlaceholderText(readable ? tr("What you want to remember about this piece…") : QString());
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnNotesChanged()
{
    if (m_loadingNotes || not m_notesEdit->isEnabled())
    {
        return;
    }

    m_notesTimer->start();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::CommitNotes()
{
    if (m_notesRow < 0 || m_notesRow >= m_summaries.size())
    {
        return;
    }

    SaveNotes(m_notesRow);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::SaveNotes(int row)
{
    const QString notes = m_notesEdit->toPlainText();
    if (notes == m_summaries.at(row).notes)
    {
        return; // Nothing to write: a file rewritten for nothing is a file, and a date, changed for nothing.
    }

    const QString path = m_summaries.at(row).path;

    QString error;
    if (not OPieceLibrary::SetNotes(path, notes, &error))
    {
        QMessageBox::warning(this, tr("Piece library"), error);
        return;
    }

    // The summary is what the window shows, so it is what is told: the file has just said the same thing. The
    // list the search reads holds the same piece, and would otherwise keep the old note.
    m_summaries[row].notes = notes;
    for (OPieceLibrary::Summary &summary : m_all)
    {
        if (summary.path == path)
        {
            summary.notes = notes;
            break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnCellClicked(int row, int column)
{
    if (row < 0 || row >= m_summaries.size())
    {
        return;
    }

    switch (column)
    {
        case kTransferColumn:
            TransferPiece(row);
            break;
        case kExportColumn:
            ExportPiece(row);
            break;
        case kDeleteColumn:
            DeletePiece(row);
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Le choix de la destination : un combo, et rien d'autre à décider.
 *
 * Écrit ici plutôt que par QInputDialog : le combo doit venir de `MakeComboBox()`, la fabrique de la maison — son
 * panneau déroulant est une QListView que la feuille de style habille, là où le popup natif de macOS l'ignore.
 * Et l'index choisi est lu directement, au lieu du libellé retrouvé par `indexOf` : deux dossiers du même nom à
 * deux niveaux donnaient la même chaîne, donc le mauvais dossier.
 */
auto OPieceLibraryDialog::AskTransferFolder(const QString &pieceName, const QStringList &labels,
                                            const QStringList &paths) -> QString
{
    if (paths.isEmpty())
    {
        return {};
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Transfer \"%1\"").arg(pieceName));

    auto *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(10);

    layout->addWidget(new QLabel(tr("Move to:")));

    auto *combo = MakeComboBox(&dialog);
    combo->addItems(labels);
    combo->setCurrentIndex(0);
    layout->addWidget(combo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    if (QPushButton *ok = buttons->button(QDialogButtonBox::Ok); ok != nullptr)
    {
        ok->setText(tr("Transfer"));
    }
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
    {
        return {};
    }

    const int chosen = combo->currentIndex();
    return chosen >= 0 && chosen < paths.size() ? paths.at(chosen) : QString();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::TransferPiece(int row)
{
    const OPieceLibrary::Summary &summary = m_summaries.at(row);

    // The destinations are the library's own folders: a transfer is a move **inside** the library, and Export is
    // the gesture that takes a piece somewhere else on the computer.
    QStringList labels{tr("The library's own folder")};
    QStringList paths{m_directory};
    for (const OPieceLibrary::Folder &folder : OPieceLibrary::Folders(m_directory))
    {
        labels.append(folder.label);
        paths.append(folder.path);
    }

    const QString targetDirectory = AskTransferFolder(summary.name, labels, paths);
    if (targetDirectory.isEmpty())
    {
        return; // A cancelled choice is not a failure: nothing was asked.
    }

    bool overwrite = false;
    const QString target = OPieceLibrary::TargetPath(summary.path, targetDirectory);
    if (QFileInfo::exists(target))
    {
        const QMessageBox::StandardButton answer
            = QMessageBox::question(this, tr("Transfer"),
                                    tr("\"%1\" already exists in that folder.\n\nReplace it?")
                                        .arg(QFileInfo(target).fileName()),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return;
        }
        overwrite = true;
    }

    QString error;
    if (not OPieceLibrary::Transfer(summary.path, targetDirectory, overwrite, &error))
    {
        QMessageBox::warning(this, tr("Transfer"), error);
        return;
    }

    // The moved piece is no longer in the folder that was read: the list follows, and the selection lands on the
    // line that took its place rather than on the first one.
    Refresh();

    const int nearest = qMin(row, m_table->rowCount() - 1);
    if (nearest >= 0)
    {
        m_table->selectRow(nearest);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::ExportPiece(int row)
{
    const OPieceLibrary::Summary &summary = m_summaries.at(row);
    const QString extension = OPieceLibrary::Extension();

    QString target = QFileDialog::getSaveFileName(
        this, tr("Export \"%1\"").arg(summary.name),
        QDir::homePath() + QDir::separator() + summary.name + QLatin1Char('.') + extension,
        tr("Ostara piece (*.%1);;All files (*)").arg(extension));

    if (target.isEmpty())
    {
        return; // A cancelled save is not a failure: nothing was asked.
    }

    // A piece out of the library is still a piece: the extension is added rather than left to be noticed later,
    // in a folder where nothing would open it.
    if (QFileInfo(target).suffix().isEmpty())
    {
        target += QLatin1Char('.') + extension;
    }

    QString error;
    if (not OPieceLibrary::Export(summary.path, target, &error))
    {
        QMessageBox::warning(this, tr("Export error"), error);
        return;
    }

    QMessageBox::information(this, tr("Export successful"), tr("Piece exported to:\n%1").arg(target));
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::DeletePiece(int row)
{
    const OPieceLibrary::Summary &summary = m_summaries.at(row);

    const QMessageBox::StandardButton answer
        = QMessageBox::question(this, tr("Confirm deletion"),
                                tr("Are you sure you want to delete \"%1\"?\n\nThis action cannot be undone.")
                                    .arg(summary.name),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QString error;
    if (not OPieceLibrary::Remove(summary.path, &error))
    {
        QMessageBox::warning(this, tr("Error"), error);
        return;
    }

    // The list is reread, and the selection lands on the piece that took the place of the one that is gone — not
    // on the first line, which would look like a jump to the top.
    Refresh();

    const int nearest = qMin(row, m_table->rowCount() - 1);
    if (nearest >= 0)
    {
        m_table->selectRow(nearest);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnOpenFolder()
{
    // What the list is showing, opened where the system keeps its files — the library's own folder while the
    // search is looking everywhere at once, since the results no longer belong to one folder.
    const QString folder = m_searchEdit->text().trimmed().isEmpty() ? ActiveFolder() : m_directory;

    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibraryDialog::FreeImportPath(const QString &directory, const QString &baseName) -> QString
{
    const QString extension = OPieceLibrary::Extension();
    const QDir library(directory);

    QString name = baseName;
    while (true)
    {
        const QString path = library.filePath(name + QLatin1Char('.') + extension);
        if (not QFileInfo::exists(path))
        {
            return path;
        }

        // The library already holds a piece under that name, and the two are not the same piece: the reader names
        // the one entering, rather than the one already there losing its place.
        bool accepted = false;
        const QString asked = QInputDialog::getText(
            this, tr("Import a piece"),
            tr("The library already holds a piece named \"%1\".\n\nName the piece entering:").arg(name),
            QLineEdit::Normal, name, &accepted);

        if (not accepted)
        {
            return {};
        }

        const QString trimmed = asked.trimmed();
        if (trimmed.isEmpty() || trimmed.contains(QLatin1Char('/')))
        {
            QMessageBox::warning(this, tr("Import a piece"), tr("A piece name cannot be empty nor hold a slash."));
            continue;
        }

        name = trimmed;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnImportPiece()
{
    const QString extension = OPieceLibrary::Extension();

    const QString source = QFileDialog::getOpenFileName(this, tr("Import a piece"), QDir::homePath(),
                                                        tr("Ostara piece (*.%1);;All files (*)").arg(extension));
    if (source.isEmpty())
    {
        return; // A cancelled choice is not a failure: nothing was asked.
    }

    // The piece enters the folder being read, not the library's own folder: what the reader is looking at is
    // where she expects to find it again.
    const QString target = FreeImportPath(ActiveFolder(), QFileInfo(source).completeBaseName());
    if (target.isEmpty())
    {
        return;
    }

    QString error;
    if (not OPieceLibrary::Import(source, target, &error))
    {
        QMessageBox::warning(this, tr("Import a piece"), error);
        return;
    }

    // The piece landed in the folder being read, which the search was not showing: the search goes, so that
    // what was just imported is seen rather than looked for.
    if (not m_searchEdit->text().isEmpty())
    {
        m_searchEdit->clear();
        m_searchTimer->stop();
    }

    Refresh();

    for (int row = 0; row < m_summaries.size(); ++row)
    {
        if (m_summaries.at(row).path == target)
        {
            m_table->selectRow(row);
            break;
        }
    }
}
