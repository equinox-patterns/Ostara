/************************************************************************
 **
 **  @file   opatternlibrarydialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The library of patterns: the folders they are sorted into, the patterns of the chosen folder, and
 **          what the selected one says about itself.
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

#include "opatternlibrarydialog.h"

#include "opatternfolderdialog.h"

#include "core/olibraryscanner.h"
#include "FactoryWidgets.h"
#include "vabstractvalapplication.h"
#include "vtheme.h"
#include "widgets/olibraryicondelegate.h"
#include "widgets/olibrarytree.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
const int kNameColumn = 0;
const int kModifiedColumn = 1;
const int kCollectionColumn = 2;
const int kClientColumn = 3;
const int kTransferColumn = 4;
const int kExportColumn = 5;
const int kDeleteColumn = 6;
const int kColumnCount = 7;

const int kActionColumnWidth = 44;
const int kIconSize = 20;
const int kRowHeight = 36;

const int kTreeWidth = 240;
const int kSideWidth = 320;

// What the three panes will not go below. The list's floor is the sum of its seven columns (four of text, three
// of icons): under it, the gestures of the row would be out of sight — and they are what the window is for.
const int kTreeMinimumWidth = 140;
const int kTableMinimumWidth = 560;
const int kSideMinimumWidth = 240;

/// The search reads every `.ostara` header of the library: it is run a moment after the typing stops, never on
/// every keystroke — a library is not a text field.
const int kSearchDelay = 300;

const QString kDateLayout = QStringLiteral("dd/MM/yyyy HH:mm");

/// What a panel shows for a field the pattern leaves empty. An empty panel reads like a window that did not
/// look; a dash reads like an answer.
const QString kDash = QStringLiteral("-");

/// @brief Every folder of the library, as a label the reader reads (`Dupont/Hiver`) and the path it stands for —
/// one list, built once, for the two things a transfer needs to say.
void CollectFolders(const QString &root, const QString &prefix, QStringList *labels, QStringList *paths)
{
    const QStringList folders = OLibraryScanner::Directories(root);
    for (const QString &folder : folders)
    {
        const QString name = QFileInfo(folder).fileName();
        const QString label = prefix.isEmpty() ? name : prefix + QLatin1Char('/') + name;

        labels->append(label);
        paths->append(folder);

        CollectFolders(folder, label, labels, paths);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPatternLibraryDialog::OPatternLibraryDialog(const QString &directory, QWidget *parent)
    : QDialog(parent), m_directory(directory)
{
    SetupUI();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::SetupUI()
{
    setMinimumSize(980, 560);
    resize(1180, 680);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // --- The search, then the folder's own name ---
    auto *searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    m_searchLabel = new QLabel();
    searchRow->addWidget(m_searchLabel);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setClearButtonEnabled(true);
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_searchEdit->addAction(QIcon(OLibraryIconPath(QStringLiteral("magnifying-glass"), dark)),
                            QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OPatternLibraryDialog::OnSearchChanged);
    searchRow->addWidget(m_searchEdit, 1);

    mainLayout->addLayout(searchRow);

    auto *createRow = new QHBoxLayout();
    m_createFolderButton = new QPushButton();
    connect(m_createFolderButton, &QPushButton::clicked, this, &OPatternLibraryDialog::OnCreateFolder);
    createRow->addWidget(m_createFolderButton);
    createRow->addStretch(1);
    mainLayout->addLayout(createRow);

    // --- The tree, the list of the chosen folder, and what the selected pattern says ---
    auto *splitter = new QSplitter(Qt::Horizontal);

    m_tree = new QTreeWidget();
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setUniformRowHeights(true);
    m_tree->setIconSize(QSize(kIconSize, kIconSize));
    // Le même nom pour les deux arbres : le thème leur donne nos carets (template.qss.in), et le délégué de ligne
    // passe l'icône du dossier en variante claire quand la ligne est choisie ou survolée.
    m_tree->setObjectName(QStringLiteral("libraryTree"));
    m_tree->setItemDelegate(new OLibraryRowDelegate(m_tree));
    // The tree can be made narrow, but not to nothing: the folders are what the window is read through.
    m_tree->setMinimumWidth(kTreeMinimumWidth);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, &OPatternLibraryDialog::OnFolderSelected);
    splitter->addWidget(m_tree);

    m_table = new QTableWidget();
    m_table->setColumnCount(kColumnCount);
    // The list keeps room for its seven columns: squeezed further, the three gestures of the row would leave the
    // view, and an action nobody can see is an action nobody has.
    m_table->setMinimumWidth(kTableMinimumWidth);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    m_table->horizontalHeader()->setSectionResizeMode(kNameColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(kModifiedColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kCollectionColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kClientColumn, QHeaderView::ResizeToContents);
    for (int column = kTransferColumn; column < kColumnCount; ++column)
    {
        m_table->horizontalHeader()->setSectionResizeMode(column, QHeaderView::Fixed);
        m_table->setColumnWidth(column, kActionColumnWidth);
        m_table->setItemDelegateForColumn(column, new OLibraryIconDelegate(m_table));
    }
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &OPatternLibraryDialog::ShowDetails);
    connect(m_table, &QTableWidget::cellClicked, this, &OPatternLibraryDialog::OnCellClicked);
    splitter->addWidget(m_table);

    auto *side = new QWidget();
    side->setMinimumWidth(kSideMinimumWidth);
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(8);

    // The one place that answers when there is nothing to show: an empty folder, a search that matched nothing,
    // and a library nobody has filled yet do not look the same, and the window says which one it is.
    m_stateLabel = new QLabel();
    m_stateLabel->setWordWrap(true);
    m_stateLabel->setAlignment(Qt::AlignCenter);
    sideLayout->addWidget(m_stateLabel);

    m_fileNameLabel = new QLabel();
    m_fileNameLabel->setObjectName(QStringLiteral("descLabel"));
    m_fileNameLabel->setWordWrap(true);
    // Centré, comme le titre des groupes et le message d'état du panneau : le nom du fichier est le titre de ce
    // que la sélection décrit, et un titre resté à gauche sous des titres centrés se lit comme un oubli.
    m_fileNameLabel->setAlignment(Qt::AlignCenter);
    m_fileNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    sideLayout->addWidget(m_fileNameLabel);

    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *scrollBody = new QWidget();
    auto *scrollLayout = new QVBoxLayout(scrollBody);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(8);

    const auto addTextGroup = [&scrollLayout](const QString &title, QLabel **label) {
        auto *group = new QGroupBox(title);
        auto *groupLayout = new QVBoxLayout(group);
        *label = new QLabel();
        (*label)->setWordWrap(true);
        (*label)->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        (*label)->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        groupLayout->addWidget(*label);
        scrollLayout->addWidget(group);
        return group;
    };

    m_descriptionGroup = addTextGroup(QString(), &m_descriptionLabel);
    m_notesGroup = addTextGroup(QString(), &m_notesLabel);
    m_fabricGroup = addTextGroup(QString(), &m_fabricLabel);
    scrollLayout->addStretch(1);
    scroll->setWidget(scrollBody);
    sideLayout->addWidget(scroll, 1);

    splitter->addWidget(side);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({kTreeWidth, 640, kSideWidth});

    mainLayout->addWidget(splitter, 1);

    // --- What is left: the folder, where the system keeps it, and the way out ---
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);

    m_openFolderButton = new QPushButton();
    connect(m_openFolderButton, &QPushButton::clicked, this, &OPatternLibraryDialog::OnOpenFolder);
    buttonRow->addWidget(m_openFolderButton);

    buttonRow->addStretch(1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    buttonRow->addWidget(m_buttonBox);

    mainLayout->addLayout(buttonRow);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(kSearchDelay);
    connect(m_searchTimer, &QTimer::timeout, this, &OPatternLibraryDialog::OnSearchTimeout);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::Retranslate()
{
    setWindowTitle(tr("Pattern library"));

    m_searchLabel->setText(tr("Search:"));
    m_searchEdit->setPlaceholderText(tr("Search the patterns of every folder…"));

    m_createFolderButton->setText(tr("Create a folder"));

    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Modified"), tr("Collection"), tr("Client"), QString(),
                                        QString(), QString()});

    m_descriptionGroup->setTitle(tr("Description"));
    m_notesGroup->setTitle(tr("Notes"));
    m_fabricGroup->setTitle(tr("Fabric info"));

    m_openFolderButton->setText(tr("Open folder"));

    // The tree holds one word, the name of the library's own folder, and the folders are file names that no
    // language changes: it is rebuilt rather than re-said, so a language change never leaves a window whose tree
    // still speaks the old one.
    PopulateTree();

    // The panels belong to the selection: re-saying what the selected pattern holds, in the language the window
    // is now speaking.
    ShowDetails();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::Refresh()
{
    PopulateTree();
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::PopulateTree()
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
    // whose folder no line points at would leave the reader guessing where the patterns come from.
    if (not OLibraryTree::Select(m_tree, ActiveFolder()))
    {
        // The chosen folder is gone: the library's own folder takes over rather than the window insisting on a
        // place that no longer exists.
        m_activeFolder.clear();
        OLibraryTree::Select(m_tree, m_directory);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibraryDialog::ActiveFolder() const -> QString
{
    return m_activeFolder.isEmpty() ? m_directory : m_activeFolder;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::OnFolderSelected()
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
void OPatternLibraryDialog::OnSearchChanged(const QString &text)
{
    Q_UNUSED(text)
    m_searchTimer->start();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::OnSearchTimeout()
{
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::Populate()
{
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

    if (needle.isEmpty())
    {
        // The chosen folder, and only it: the list is what the tree is pointing at.
        m_entries = OPatternLibrary::Entries(ActiveFolder());
    }
    else
    {
        // A pattern can be in a folder the reader is not looking at, and that is exactly what she is asking for:
        // the search is the one gesture that walks the whole library.
        m_entries = OPatternLibrary::Search(m_directory, needle);
    }

    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(m_entries.size()));

    for (int row = 0; row < m_entries.size(); ++row)
    {
        const OPatternLibrary::Entry &entry = m_entries.at(row);

        auto *nameItem = new QTableWidgetItem(OPatternLibrary::DisplayName(entry));
        nameItem->setData(Qt::UserRole, entry.path);
        if (not entry.error.isEmpty())
        {
            nameItem->setToolTip(entry.error);
        }
        else if (not needle.isEmpty())
        {
            // The search took it from somewhere else: saying where is the difference between a result and a
            // mystery.
            nameItem->setToolTip(QFileInfo(entry.path).absolutePath());
        }
        m_table->setItem(row, kNameColumn, nameItem);

        const QString modified = entry.modified.isValid() ? entry.modified.toString(kDateLayout) : kDash;
        m_table->setItem(row, kModifiedColumn, new QTableWidgetItem(modified));
        m_table->setItem(row, kCollectionColumn, new QTableWidgetItem(entry.collection));
        m_table->setItem(row, kClientColumn, new QTableWidgetItem(entry.customer));

        // The three gestures of the row, as icons — the same pair the piece library offers.
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

    // A pattern is shown selected from the start: an empty panel beside a full list would make the window look
    // like it has nothing to say.
    if (m_table->rowCount() > 0)
    {
        m_table->selectRow(0);
    }

    ShowDetails();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibraryDialog::SelectedRow() const -> int
{
    return m_table->currentRow();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::ShowDetails()
{
    const int row = SelectedRow();
    const bool searching = not m_searchEdit->text().trimmed().isEmpty();

    if (row < 0 || row >= m_entries.size())
    {
        m_fileNameLabel->clear();
        m_descriptionLabel->clear();
        m_notesLabel->clear();
        m_fabricLabel->clear();

        // Which of the three empty states it is: the difference between a window that answers and a window that
        // looks broken.
        if (m_entries.isEmpty())
        {
            m_stateLabel->setText(searching ? tr("No pattern matches the search.")
                                            : tr("This folder holds no pattern."));
        }
        else
        {
            m_stateLabel->clear();
        }

        m_stateLabel->setVisible(not m_stateLabel->text().isEmpty());
        return;
    }

    m_stateLabel->clear();
    m_stateLabel->setVisible(false);

    const OPatternLibrary::Entry &entry = m_entries.at(row);

    m_fileNameLabel->setText(entry.fileName);
    m_fileNameLabel->setToolTip(entry.path);

    m_descriptionLabel->setText(entry.description.isEmpty() ? kDash : entry.description);
    m_notesLabel->setText(entry.notes.isEmpty() ? kDash : entry.notes);
    m_fabricLabel->setText(entry.fabricInfo.isEmpty() ? kDash : entry.fabricInfo);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::OnCellClicked(int row, int column)
{
    if (row < 0 || row >= m_entries.size())
    {
        return;
    }

    switch (column)
    {
        case kTransferColumn:
            TransferEntry(row);
            break;
        case kExportColumn:
            ExportEntry(row);
            break;
        case kDeleteColumn:
            DeleteEntry(row);
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
auto OPatternLibraryDialog::AskTransferFolder(const QString &patternName, const QStringList &labels,
                                              const QStringList &paths) -> QString
{
    if (paths.isEmpty())
    {
        return {};
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Transfer \"%1\"").arg(patternName));

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
void OPatternLibraryDialog::TransferEntry(int row)
{
    const OPatternLibrary::Entry &entry = m_entries.at(row);

    // The destinations are the library's own folders: a transfer is a move **inside** the library, and Export is
    // the gesture that takes a pattern somewhere else on the computer.
    QStringList labels{tr("The library's own folder")};
    QStringList paths{m_directory};
    CollectFolders(m_directory, QString(), &labels, &paths);

    const QString targetDirectory = AskTransferFolder(OPatternLibrary::DisplayName(entry), labels, paths);
    if (targetDirectory.isEmpty())
    {
        return; // A cancelled choice is not a failure: nothing was asked.
    }

    bool overwrite = false;
    const QString target = OPatternLibrary::TargetPath(entry.path, targetDirectory);
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
    if (not OPatternLibrary::Transfer(entry.path, targetDirectory, overwrite, &error))
    {
        QMessageBox::warning(this, tr("Transfer"), error);
        return;
    }

    // The moved pattern is no longer in the folder that was read: the list follows, and the selection lands on
    // the line that took its place rather than on the first one.
    Refresh();

    const int nearest = qMin(row, m_table->rowCount() - 1);
    if (nearest >= 0)
    {
        m_table->selectRow(nearest);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::ExportEntry(int row)
{
    const OPatternLibrary::Entry &entry = m_entries.at(row);
    const QString extension = OPatternLibrary::Extension();

    QString target
        = QFileDialog::getSaveFileName(this, tr("Export \"%1\"").arg(OPatternLibrary::DisplayName(entry)),
                                       QDir::homePath() + QDir::separator() + entry.fileName,
                                       tr("Pattern files (*.%1);;All files (*)").arg(extension));

    if (target.isEmpty())
    {
        return; // A cancelled save is not a failure: nothing was asked.
    }

    // A pattern out of the library is still a pattern: the extension is added rather than left to be noticed
    // later, in a folder where nothing would open it.
    if (QFileInfo(target).suffix().isEmpty())
    {
        target += QLatin1Char('.') + extension;
    }

    QString error;
    if (not OPatternLibrary::Export(entry.path, target, &error))
    {
        QMessageBox::warning(this, tr("Export error"), error);
        return;
    }

    QMessageBox::information(this, tr("Export successful"), tr("Pattern exported to:\n%1").arg(target));
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::DeleteEntry(int row)
{
    const OPatternLibrary::Entry &entry = m_entries.at(row);

    // The pattern that is open in Ostara is the one file this window must not touch: deleting it under the
    // editor's feet would leave the application working on a file that is gone.
    const QString openPattern = VAbstractValApplication::VApp()->GetPatternPath();
    if (not openPattern.isEmpty() && SameFile(openPattern, entry.path))
    {
        QMessageBox::warning(this, tr("Delete"),
                             tr("\"%1\" is the pattern that is open in Ostara.\n\nClose it before deleting it.")
                                 .arg(OPatternLibrary::DisplayName(entry)));
        return;
    }

    const QMessageBox::StandardButton answer
        = QMessageBox::question(this, tr("Confirm deletion"),
                                tr("Delete \"%1\"?\n\n%2\n\nThis action cannot be undone.")
                                    .arg(OPatternLibrary::DisplayName(entry), entry.fileName),
                                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QString error;
    if (not OPatternLibrary::Remove(entry.path, &error))
    {
        QMessageBox::warning(this, tr("Delete"), error);
        return;
    }

    Refresh();

    const int nearest = qMin(row, m_table->rowCount() - 1);
    if (nearest >= 0)
    {
        m_table->selectRow(nearest);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::OnOpenFolder()
{
    // What the list is showing, opened where the system keeps its files — the library's own folder while the
    // search is looking everywhere at once, since the results no longer belong to one folder.
    const QString folder = m_searchEdit->text().trimmed().isEmpty() ? ActiveFolder() : m_directory;

    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibraryDialog::SameFile(const QString &left, const QString &right) -> bool
{
    const QString leftCanonical = QFileInfo(left).canonicalFilePath();
    const QString rightCanonical = QFileInfo(right).canonicalFilePath();
    if (not leftCanonical.isEmpty() && not rightCanonical.isEmpty())
    {
        return leftCanonical == rightCanonical;
    }

    // One of the two does not exist as the file system spells it — the comparison falls back to the paths as
    // they were written, which is still an answer rather than an excuse.
    return QDir::cleanPath(QFileInfo(left).absoluteFilePath())
           == QDir::cleanPath(QFileInfo(right).absoluteFilePath());
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibraryDialog::OnCreateFolder()
{
    OPatternFolderDialog dialog(ActiveFolder(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    Refresh();

    // A folder is made to be filled: it is the one the window stands in now, and its emptiness is said rather
    // than shown as a list that looks broken.
    if (not dialog.CreatedPath().isEmpty())
    {
        OLibraryTree::Select(m_tree, dialog.CreatedPath());
    }
}
