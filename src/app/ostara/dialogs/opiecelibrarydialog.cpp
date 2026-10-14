/************************************************************************
 **
 **  @file   opiecelibrarydialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The pieces saved in the library: what each of them asks for, what the reader wrote about it, and the
 **          four gestures that manage them — insert one, copy one out, take one in, or delete it.
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

#include "vtheme.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFile>
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
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
const int kNameColumn = 0;
const int kNeedsColumn = 1;
const int kUnitColumn = 2;
const int kModifiedColumn = 3;
const int kExportColumn = 4;
const int kDeleteColumn = 5;
const int kColumnCount = 6;

const int kActionColumnWidth = 44;
const int kIconSize = 20;
const int kRowHeight = 36;

/// A moment after the typing stops, never on every keystroke: a file written per letter would be a file validated
/// per letter, and a `.opiece` is a whole document.
const int kNotesCommitDelay = 600;

auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg").arg(dark ? QStringLiteral("dark") : QStringLiteral("light"),
                                                     baseName);
}

/// @brief The two variants of an action icon, read once: the light one on a plain row, the dark one on a selected
/// row — the same pair the measurement tables window paints.
auto ActionIcon(const QString &baseName) -> QIcon
{
    QIcon icon;
    icon.addFile(IconResPath(baseName, false), QSize(), QIcon::Normal);
    icon.addFile(IconResPath(baseName, true), QSize(), QIcon::Selected);
    return icon;
}

/**
 * @brief The icons of the two action columns, painted centred.
 *
 * An item's own icon would sit at the left of its cell and keep one variant for every state; here the icon is
 * centred, and the row's selection is what picks the variant — the delegate holds the pair, so there is no second
 * place where the two files are named.
 */
class ActionIconDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        const QVariant decoration = index.data(Qt::DecorationRole);
        if (not decoration.isValid())
        {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        // The row's background belongs to the view: it is drawn as it would be, without text or icon, and the
        // action's icon is then painted on top of it.
        QStyleOptionViewItem background = option;
        initStyleOption(&background, index);
        background.text.clear();
        background.icon = QIcon();

        const QWidget *widget = background.widget;
        QStyle *style = widget != nullptr ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &background, painter, widget);

        const QIcon icon = decoration.value<QIcon>();
        const bool selected = (option.state & QStyle::State_Selected) != 0;

        QRect target(QPoint(), QSize(kIconSize, kIconSize));
        target.moveCenter(option.rect.center());
        icon.paint(painter, target, Qt::AlignCenter, selected ? QIcon::Selected : QIcon::Normal);
    }
};

const QString kDateLayout = QStringLiteral("dd/MM/yyyy HH:mm");
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
    setMinimumSize(860, 520);
    resize(1040, 620);

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
    m_searchEdit->addAction(QIcon(IconResPath(QStringLiteral("magnifying-glass"), dark)),
                            QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OPieceLibraryDialog::OnSearchChanged);
    searchRow->addWidget(m_searchEdit, 1);

    mainLayout->addLayout(searchRow);

    // --- The list on the left, what the piece needs and what was written about it on the right ---
    auto *splitter = new QSplitter(Qt::Horizontal);

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
    m_table->horizontalHeader()->setSectionResizeMode(kExportColumn, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(kDeleteColumn, QHeaderView::Fixed);
    m_table->setColumnWidth(kExportColumn, kActionColumnWidth);
    m_table->setColumnWidth(kDeleteColumn, kActionColumnWidth);
    m_table->setItemDelegateForColumn(kExportColumn, new ActionIconDelegate(m_table));
    m_table->setItemDelegateForColumn(kDeleteColumn, new ActionIconDelegate(m_table));
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &OPieceLibraryDialog::ShowDetails);
    connect(m_table, &QTableWidget::cellClicked, this, &OPieceLibraryDialog::OnCellClicked);
    splitter->addWidget(m_table);

    auto *side = new QWidget();
    auto *sideLayout = new QVBoxLayout(side);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(8);

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
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({520, 520}); // Half and half to begin with; where the divider goes is the reader's.

    mainLayout->addWidget(splitter, 1);

    m_notesTimer = new QTimer(this);
    m_notesTimer->setSingleShot(true);
    m_notesTimer->setInterval(kNotesCommitDelay);
    connect(m_notesTimer, &QTimer::timeout, this, &OPieceLibraryDialog::CommitNotes);

    // --- The four gestures, and the two that manage the library are on the left ---
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

    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Needs"), tr("Unit"), tr("Modified"), QString(), QString()});
    m_insertButton->setText(tr("Insert"));
    m_openFolderButton->setText(tr("Open folder"));
    m_importButton->setText(tr("Import a piece"));

    m_needsGroup->setTitle(tr("Needs"));
    m_notesGroup->setTitle(tr("Notes"));
    m_notesEdit->setPlaceholderText(tr("What you want to remember about this piece…"));

    // The two panels belong to the selection: re-saying what the selected piece reads and what was written about
    // it, in the language the window is now speaking.
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
    if (m_notesTimer != nullptr && m_notesTimer->isActive())
    {
        m_notesTimer->stop();
        CommitNotes();
    }

    QDialog::done(result);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::Refresh()
{
    // The list is about to be rebuilt: a note still waiting is written first, while the row it belongs to still
    // means the piece it was typed for.
    if (m_notesTimer != nullptr && m_notesTimer->isActive())
    {
        m_notesTimer->stop();
        CommitNotes();
    }

    m_summaries = OPieceLibrary::Summaries(m_directory);

    Populate();
    Filter(m_searchEdit->text().trimmed());
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::Populate()
{
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
        m_table->setItem(row, kNameColumn, nameItem);

        auto *needsItem = new QTableWidgetItem(OPieceLibrary::NeedsLine(summary));
        needsItem->setToolTip(OPieceLibrary::Details(summary));
        m_table->setItem(row, kNeedsColumn, needsItem);

        m_table->setItem(row, kUnitColumn, new QTableWidgetItem(summary.unit));

        const QString modified = summary.modified.isValid() ? summary.modified.toString(kDateLayout)
                                                           : QStringLiteral("-");
        m_table->setItem(row, kModifiedColumn, new QTableWidgetItem(modified));

        // The two gestures of the row, as icons — the same pair the measurement tables window offers.
        auto *exportItem = new QTableWidgetItem(ActionIcon(QStringLiteral("floppy-disk")), QString());
        exportItem->setToolTip(tr("Export"));
        m_table->setItem(row, kExportColumn, exportItem);

        auto *deleteItem = new QTableWidgetItem(ActionIcon(QStringLiteral("trash")), QString());
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
void OPieceLibraryDialog::Filter(const QString &filter)
{
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        bool matches = filter.isEmpty();
        for (int column = 0; not matches && column < m_table->columnCount(); ++column)
        {
            if (const QTableWidgetItem *item = m_table->item(row, column); item != nullptr)
            {
                matches = item->text().contains(filter, Qt::CaseInsensitive);
            }
        }
        m_table->setRowHidden(row, not matches);
    }

    // The panels describe the selected piece: when the search hides it, the choice moves to what is still visible
    // rather than leaving them talking about something the list no longer shows.
    const int current = m_table->currentRow();
    if (current < 0 || m_table->isRowHidden(current))
    {
        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            if (not m_table->isRowHidden(row))
            {
                m_table->selectRow(row);
                break;
            }
        }
    }

    ShowDetails();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnSearchChanged(const QString &text)
{
    Filter(text.trimmed());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceLibraryDialog::SelectedRow() const -> int
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_summaries.size() || m_table->isRowHidden(row))
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
        // No choice yet — which is also what an empty library looks like, and what a search that matches nothing
        // looks like. Saying which of the three it is the difference between a window that answers and a window
        // that looks broken.
        if (m_summaries.isEmpty())
        {
            m_needsLabel->setText(tr("No piece is saved in the library yet."));
        }
        else if (not m_searchEdit->text().trimmed().isEmpty())
        {
            m_needsLabel->setText(tr("No piece matches the search."));
        }
        else
        {
            m_needsLabel->setText(QString());
        }

        m_notesEdit->setEnabled(false);
        m_notesEdit->setPlaceholderText(QString());
        return;
    }

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

    QString error;
    if (not OPieceLibrary::SetNotes(m_summaries.at(row).path, notes, &error))
    {
        QMessageBox::warning(this, tr("Piece library"), error);
        return;
    }

    // The summary is what the window shows, so it is what is told: the file has just said the same thing.
    m_summaries[row].notes = notes;
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
void OPieceLibraryDialog::ExportPiece(int row)
{
    const OPieceLibrary::Summary &summary = m_summaries.at(row);
    const QString extension = OPieceLibrary::Extension();

    const QString target = QFileDialog::getSaveFileName(
        this, tr("Export \"%1\"").arg(summary.name),
        QDir::homePath() + QDir::separator() + summary.name + QLatin1Char('.') + extension,
        tr("Ostara piece (*.%1);;All files (*)").arg(extension));

    if (target.isEmpty())
    {
        return; // A cancelled save is not a failure: nothing was asked.
    }

    if (QFile::copy(summary.path, target))
    {
        QMessageBox::information(this, tr("Export successful"), tr("Piece exported to:\n%1").arg(target));
    }
    else
    {
        QMessageBox::warning(this, tr("Export error"), tr("Could not copy the piece to the selected destination."));
    }
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

    if (not QFile::remove(summary.path))
    {
        QMessageBox::warning(this, tr("Error"), tr("Could not delete the file."));
        return;
    }

    // The list is reread, and the selection lands on the piece that took the place of the one that is gone — not
    // on the first line, which would look like a jump to the top.
    Refresh();

    const int nearest = qMin(row, m_table->rowCount() - 1);
    if (nearest >= 0 && not m_table->isRowHidden(nearest))
    {
        m_table->selectRow(nearest);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::OnOpenFolder()
{
    // The window is looking at that folder: opening it is what makes a piece findable outside Ostara.
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_directory));
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

    const QString target = FreeImportPath(m_directory, QFileInfo(source).completeBaseName());
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
