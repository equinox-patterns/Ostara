/************************************************************************
 **
 **  @file   opatternopendialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  Open a pattern: from the pattern library, or from anywhere on the computer.
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

#include "opatternopendialog.h"

#include "FactoryWidgets.h"
#include "vmisc/vabstractvalapplication.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

namespace
{
const int kListMinimumHeight = 220;
const int kDialogMinimumWidth = 560;
const int kDialogMinimumHeight = 520;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OPatternOpenDialog::OPatternOpenDialog(const QString &libraryDirectory, QWidget *parent)
    : QDialog(parent), m_library(libraryDirectory)
{
    SetupUI();
    PopulateFolders();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::SetupUI()
{
    setMinimumSize(kDialogMinimumWidth, kDialogMinimumHeight);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);

    // --- La bibliothèque : un dossier, puis ses patrons ---
    m_libraryGroup = new QGroupBox();
    auto *libraryLayout = new QVBoxLayout(m_libraryGroup);

    auto *folderRow = new QHBoxLayout();
    m_folderLabel = new QLabel();
    folderRow->addWidget(m_folderLabel);
    m_folderCombo = MakeComboBox();
    m_folderCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    connect(m_folderCombo, &QComboBox::currentIndexChanged, this, &OPatternOpenDialog::OnFolderChanged);
    folderRow->addWidget(m_folderCombo, 1);
    libraryLayout->addLayout(folderRow);

    auto *searchRow = new QHBoxLayout();
    m_searchLabel = new QLabel();
    searchRow->addWidget(m_searchLabel);
    m_searchEdit = new QLineEdit();
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OPatternOpenDialog::OnSearchChanged);
    searchRow->addWidget(m_searchEdit, 1);
    libraryLayout->addLayout(searchRow);

    m_list = new QListWidget();
    m_list->setAlternatingRowColors(true);
    m_list->setMinimumHeight(kListMinimumHeight);
    m_list->setUniformItemSizes(true);
    connect(m_list, &QListWidget::itemActivated, this, &OPatternOpenDialog::OnItemActivated);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &OPatternOpenDialog::OnItemActivated);
    libraryLayout->addWidget(m_list, 1);

    m_hint = new QLabel();
    m_hint->setObjectName(QStringLiteral("descLabel"));
    m_hint->setWordWrap(true);
    libraryLayout->addWidget(m_hint);

    mainLayout->addWidget(m_libraryGroup, 1);

    // --- Ailleurs sur l'ordinateur ---
    m_otherGroup = new QGroupBox();
    auto *otherLayout = new QVBoxLayout(m_otherGroup);

    m_browseButton = new QPushButton();
    m_browseButton->setMinimumHeight(40);
    // Pas l'action première : le thème peint en accent le bouton par défaut, et « Parcourir » ne doit pas avoir
    // l'air d'être le chemin à prendre. Entrée, elle, ouvre le patron choisi dans la liste (itemActivated).
    m_browseButton->setAutoDefault(false);
    connect(m_browseButton, &QPushButton::clicked, this, &OPatternOpenDialog::OnBrowse);
    otherLayout->addWidget(m_browseButton);

    m_browseHint = new QLabel();
    m_browseHint->setObjectName(QStringLiteral("descLabel"));
    m_browseHint->setWordWrap(true);
    otherLayout->addWidget(m_browseHint);

    mainLayout->addWidget(m_otherGroup);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    // Aucun bouton par défaut : l'action de cette dialogue est **dans la liste** (Entrée y ouvre le patron
    // choisi), et le thème peint en accent le bouton par défaut — « Annuler » n'est pas ce qu'on vient faire.
    if (QPushButton *cancel = m_buttonBox->button(QDialogButtonBox::Cancel); cancel != nullptr)
    {
        cancel->setAutoDefault(false);
    }
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::Retranslate()
{
    setWindowTitle(tr("Open a pattern"));

    m_libraryGroup->setTitle(tr("Pattern library"));
    m_folderLabel->setText(tr("Folder:"));
    m_searchLabel->setText(tr("Search:"));
    m_searchEdit->setPlaceholderText(tr("Search this folder's patterns"));
    if (m_folderCombo->count() > 0)
    {
        m_folderCombo->setItemText(0, tr("The library's own folder"));
    }

    m_otherGroup->setTitle(tr("Another folder"));
    m_browseButton->setText(tr("Browse files…"));
    m_browseHint->setText(tr("Open a .ostara file from any folder on the computer."));

    ShowEmptyState();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::PopulateFolders()
{
    const QSignalBlocker blocker(m_folderCombo);

    m_folderCombo->clear();
    m_folderCombo->addItem(QString(), m_library); // le dossier de la bibliothèque

    const QList<OPatternLibrary::Folder> folders = OPatternLibrary::Folders(m_library);
    for (const OPatternLibrary::Folder &folder : folders)
    {
        m_folderCombo->addItem(folder.label, folder.path);
    }

    m_folderCombo->setItemText(0, tr("The library's own folder"));

    // Le dossier où un patron vit déjà est le premier proposé ? Non : la bibliothèque elle-même. C'est le point
    // d'entrée le plus fréquent, et la recherche traverse le dossier choisi, pas la bibliothèque entière.
    m_folderCombo->setCurrentIndex(0);
    PopulateList();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternOpenDialog::CurrentFolder() const -> QString
{
    const QString chosen = m_folderCombo->currentData().toString();
    return chosen.isEmpty() ? m_library : chosen;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::OnFolderChanged()
{
    PopulateList();
    m_list->setFocus();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::PopulateList()
{
    const QSignalBlocker blocker(m_list);

    m_list->clear();
    m_entries = OPatternLibrary::Entries(CurrentFolder());

    for (const OPatternLibrary::Entry &entry : m_entries)
    {
        const QString text = entry.modified.isValid()
                                 ? QStringLiteral("%1 — %2").arg(OPatternLibrary::DisplayName(entry),
                                                                 entry.modified.toString(QStringLiteral("dd/MM/yyyy")))
                                 : OPatternLibrary::DisplayName(entry);

        auto *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, entry.path);
        // Le nom du fichier en infobulle : deux patrons peuvent porter le même nom, et le fichier les sépare.
        item->setToolTip(entry.fileName);
        m_list->addItem(item);
    }

    OnSearchChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::OnSearchChanged()
{
    const QString needle = m_searchEdit->text().trimmed();
    for (int row = 0; row < m_list->count(); ++row)
    {
        QListWidgetItem *item = m_list->item(row);
        const QString name = item->text();
        const QString fileName = item->toolTip();
        item->setHidden(not needle.isEmpty() && not name.contains(needle, Qt::CaseInsensitive)
                        && not fileName.contains(needle, Qt::CaseInsensitive));
    }

    ShowEmptyState();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::ShowEmptyState()
{
    if (m_hint == nullptr || m_list == nullptr)
    {
        return;
    }

    int visible = 0;
    for (int row = 0; row < m_list->count(); ++row)
    {
        if (not m_list->item(row)->isHidden())
        {
            ++visible;
        }
    }

    if (m_list->count() == 0)
    {
        m_hint->setText(tr("This folder holds no pattern."));
    }
    else if (visible == 0)
    {
        m_hint->setText(tr("No pattern matches the search."));
    }
    else
    {
        m_hint->setText(tr("Double-click a pattern to open it."));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::OnItemActivated(QListWidgetItem *item)
{
    if (item == nullptr || item->isHidden())
    {
        return;
    }

    m_selectedPath = item->data(Qt::UserRole).toString();
    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternOpenDialog::OnBrowse()
{
    const QString filter(tr("Pattern files") + QStringLiteral(" (*.ostara *.val)"));
    const QString path
        = QFileDialog::getOpenFileName(this, tr("Open a pattern"), CurrentFolder(), filter, nullptr,
                                       VAbstractApplication::VApp()->NativeFileDialog());
    if (path.isEmpty())
    {
        return;
    }

    m_selectedPath = path;
    accept();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternOpenDialog::SelectedPath() const -> QString
{
    return m_selectedPath;
}
