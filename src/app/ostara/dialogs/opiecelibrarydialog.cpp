/************************************************************************
 **
 **  @file   opiecelibrarydialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The pieces saved in the library, and what each of them will ask for.
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

#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace
{
const int kNameColumn = 0;
const int kNeedsColumn = 1;
const int kUnitColumn = 2;
const int kModifiedColumn = 3;

auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg").arg(dark ? QStringLiteral("dark") : QStringLiteral("light"),
                                                     baseName);
}

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
    setMinimumSize(720, 460);
    resize(860, 560);

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

    // --- The list ---
    m_table = new QTableWidget();
    m_table->setColumnCount(4);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(kNameColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kNeedsColumn, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(kUnitColumn, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(kModifiedColumn, QHeaderView::ResizeToContents);
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &OPieceLibraryDialog::ShowDetails);
    mainLayout->addWidget(m_table, 1);

    // --- What the selected piece needs ---
    m_detailsLabel = new QLabel();
    m_detailsLabel->setObjectName(QStringLiteral("descLabel"));
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_detailsLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_detailsLabel->setMinimumHeight(96);
    mainLayout->addWidget(m_detailsLabel);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // Inserting is what the window is for: the button is the gesture, and it stays down while the chosen piece
    // cannot be read — nothing is offered that would fail afterwards.
    m_insertButton = m_buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    connect(m_insertButton, &QPushButton::clicked, this, &QDialog::accept);
    mainLayout->addWidget(m_buttonBox);

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

    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Needs"), tr("Unit"), tr("Modified")});
    m_insertButton->setText(tr("Insert"));

    // The details area belongs to the selection: re-saying what the selected piece needs, in the language
    // the window is now speaking.
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
void OPieceLibraryDialog::Refresh()
{
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

        auto *needsItem = new QTableWidgetItem(OPieceLibrary::Needs(summary));
        needsItem->setToolTip(OPieceLibrary::Details(summary));
        m_table->setItem(row, kNeedsColumn, needsItem);

        m_table->setItem(row, kUnitColumn, new QTableWidgetItem(summary.unit));

        const QString modified = summary.modified.isValid() ? summary.modified.toString(kDateLayout)
                                                           : QStringLiteral("—");
        m_table->setItem(row, kModifiedColumn, new QTableWidgetItem(modified));
    }

    // A piece is shown selected from the start: an empty details area under a full list would make the
    // window look like it has nothing to say.
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

    // The details below describe the selected piece: when the search hides it, the choice moves to what is
    // still visible rather than leaving the area talking about something the list no longer shows.
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
void OPieceLibraryDialog::FollowSelection()
{
    const int row = m_table->currentRow();
    const bool chosen = row >= 0 && row < m_summaries.size() && not m_table->isRowHidden(row);

    m_selectedPath = chosen ? m_summaries.at(row).path : QString();
    // A file the library cannot read is shown, and said, and never offered: inserting it could only fail, and the
    // one who reads the message would have to guess why.
    m_insertButton->setEnabled(chosen && m_summaries.at(row).error.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceLibraryDialog::ShowDetails()
{
    if (m_detailsLabel == nullptr)
    {
        return;
    }

    // One entry point for the selection: what the details say and what the Insert button offers are two faces of
    // the same choice, and both follow every path that changes it — a click, a search, a refresh.
    FollowSelection();

    const int row = m_table->currentRow();
    if (row < 0 || row >= m_summaries.size() || m_table->isRowHidden(row))
    {
        // No choice yet — which is also what an empty library looks like, and what a search that matches
        // nothing looks like. Saying which of the three it is the difference between a window that answers
        // and a window that looks broken.
        if (m_summaries.isEmpty())
        {
            m_detailsLabel->setText(tr("No piece is saved in the library yet."));
        }
        else if (not m_searchEdit->text().trimmed().isEmpty())
        {
            m_detailsLabel->setText(tr("No piece matches the search."));
        }
        else
        {
            m_detailsLabel->setText(QString());
        }
        return;
    }

    m_detailsLabel->setText(OPieceLibrary::Details(m_summaries.at(row)));
}

