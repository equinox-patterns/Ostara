/************************************************************************
 **
 **  @file   osavepatterndialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  Where a pattern is written: in the library, in a folder of it, or somewhere else entirely.
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

#include "osavepatterndialog.h"

#include "opatternfolderdialog.h"

#include "core/opatternlibrary.h"
#include "vtheme.h"
#include "widgets/olibraryicondelegate.h"
#include "widgets/olibrarytree.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
const int kIconSize = 20;
const int kTreeMinimumHeight = 180;

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OSavePatternDialog::OSavePatternDialog(const QString &libraryDirectory, const QString &proposedName,
                                       const QString &currentPath, QWidget *parent)
    : QDialog(parent), m_library(libraryDirectory), m_currentPath(currentPath)
{
    SetupUI();

    m_nameEdit->setText(proposedName);
    // The extension is not the reader's to type: the base name is what she is being asked to choose, and it is
    // selected so that typing replaces it.
    m_nameEdit->setSelection(0, static_cast<int>(QFileInfo(proposedName).completeBaseName().size()));

    PopulateTree();
    ShowDestination();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::SetupUI()
{
    setMinimumSize(520, 460);

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    // --- The name, first: it is the only thing that has to be typed ---
    auto *nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);
    m_nameLabel = new QLabel();
    nameRow->addWidget(m_nameLabel);
    m_nameEdit = new QLineEdit();
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { ShowDestination(); });
    nameRow->addWidget(m_nameEdit, 1);
    layout->addLayout(nameRow);

    // --- The library, offered ---
    m_libraryCheck = new QCheckBox();
    m_libraryCheck->setChecked(true); // the library is where a pattern is kept
    connect(m_libraryCheck, &QCheckBox::toggled, this, &OSavePatternDialog::OnLibraryToggled);
    layout->addWidget(m_libraryCheck);

    m_treeGroup = new QGroupBox();
    auto *treeLayout = new QVBoxLayout(m_treeGroup);

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
    m_tree->setMinimumHeight(kTreeMinimumHeight);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this]() { ShowDestination(); });
    treeLayout->addWidget(m_tree, 1);

    auto *treeButtons = new QHBoxLayout();
    m_createFolderButton = new QPushButton();
    connect(m_createFolderButton, &QPushButton::clicked, this, &OSavePatternDialog::OnCreateFolder);
    treeButtons->addWidget(m_createFolderButton);
    treeButtons->addStretch(1);
    treeLayout->addLayout(treeButtons);

    layout->addWidget(m_treeGroup, 1);

    // --- Where it will land, in plain words: a tree and a name do not say a path ---
    m_destinationLabel = new QLabel();
    m_destinationLabel->setWordWrap(true);
    m_destinationLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    layout->addWidget(m_destinationLabel);

    // --- The way out, and the two endings ---
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);

    m_otherFolderButton = new QPushButton();
    connect(m_otherFolderButton, &QPushButton::clicked, this, [this]() {
        m_destination = Destination::Elsewhere;
        accept();
    });
    buttonRow->addWidget(m_otherFolderButton);

    buttonRow->addStretch(1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    m_saveButton = m_buttonBox->addButton(QString(), QDialogButtonBox::AcceptRole);
    connect(m_saveButton, &QPushButton::clicked, this, &OSavePatternDialog::OnAccept);
    buttonRow->addWidget(m_buttonBox);

    layout->addLayout(buttonRow);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::Retranslate()
{
    setWindowTitle(tr("Save the pattern"));

    m_nameLabel->setText(tr("Name:"));
    m_nameEdit->setPlaceholderText(tr("File name"));

    m_libraryCheck->setText(tr("Add to the pattern library"));

    m_treeGroup->setTitle(tr("Pattern library"));
    m_createFolderButton->setText(tr("Create a folder"));

    m_otherFolderButton->setText(tr("Save to another folder…"));

    if (m_saveButton != nullptr)
    {
        m_saveButton->setText(tr("Save"));
    }

    ShowDestination();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePatternDialog::ChosenDestination() const -> Destination
{
    return m_destination;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePatternDialog::LibraryPath() const -> QString
{
    return TargetPath();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::PopulateTree()
{
    // Rebuilding the tree is not the reader choosing anything: the signals are cut, and the selection is put
    // back afterwards.
    const QSignalBlocker blocker(m_tree);

    OLibraryTree::Fill(m_tree, m_library, true, tr("The library's own folder"),
                       OLibraryIcon(QStringLiteral("folder-open")));

    // The folder the pattern already lives in, when it lives in the library: that is the one she means.
    QString wanted = m_library;
    if (not m_currentPath.isEmpty())
    {
        const QString folder = QFileInfo(m_currentPath).absolutePath();
        if (folder == m_library || folder.startsWith(m_library + QDir::separator()))
        {
            wanted = folder;
        }
    }

    OLibraryTree::Select(m_tree, wanted);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePatternDialog::TargetFolder() const -> QString
{
    const QString chosen = OLibraryTree::SelectedPath(m_tree);
    return chosen.isEmpty() ? m_library : chosen;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePatternDialog::CleanName(const QString &name) -> QString
{
    // One extension, and only one: what she typed may carry the one she knows, or the legacy one.
    QString base = name.trimmed();
    while (base.endsWith(QStringLiteral(".ostara"), Qt::CaseInsensitive)
           || base.endsWith(QStringLiteral(".val"), Qt::CaseInsensitive))
    {
        base.chop(base.endsWith(QStringLiteral(".ostara"), Qt::CaseInsensitive) ? 7 : 4);
    }

    // A name is a name, not a path: the separators go, and the folder is what the tree is for.
    base.remove(QLatin1Char('/'));
    base.remove(QLatin1Char('\\'));
    return base.trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePatternDialog::TargetPath() const -> QString
{
    const QString base = CleanName(m_nameEdit->text());
    if (base.isEmpty())
    {
        return {};
    }

    return QDir(TargetFolder()).absoluteFilePath(base + QLatin1Char('.') + OPatternLibrary::Extension());
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::ShowDestination()
{
    if (not m_libraryCheck->isChecked())
    {
        m_destinationLabel->setText(tr("The folder will be asked for when you save."));
        return;
    }

    const QString path = TargetPath();
    if (path.isEmpty())
    {
        m_destinationLabel->setText(tr("Give the file a name."));
        return;
    }

    m_destinationLabel->setText(tr("Will be saved in: %1").arg(QDir::toNativeSeparators(path)));
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::OnLibraryToggled(bool checked)
{
    m_treeGroup->setEnabled(checked);
    ShowDestination();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::OnCreateFolder()
{
    // The new folder goes inside the one being pointed at, like the library window's own gesture — and it becomes
    // the destination, since a folder is made to be saved into.
    OPatternFolderDialog dialog(TargetFolder(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    PopulateTree();

    const QString created = dialog.CreatedPath();
    if (not created.isEmpty())
    {
        const QSignalBlocker blocker(m_tree);
        OLibraryTree::Select(m_tree, created);
    }

    ShowDestination();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePatternDialog::OnAccept()
{
    if (not m_libraryCheck->isChecked())
    {
        // Unticking the library puts the gesture back where it was: the system asks for the folder.
        m_destination = Destination::Elsewhere;
        accept();
        return;
    }

    if (CleanName(m_nameEdit->text()).isEmpty())
    {
        QMessageBox::information(this, tr("Save the pattern"), tr("Give the file a name."));
        m_nameEdit->setFocus();
        return;
    }

    const QString path = TargetPath();
    if (QFileInfo::exists(path))
    {
        const QMessageBox::StandardButton answer
            = QMessageBox::question(this, tr("Save the pattern"),
                                    tr("\"%1\" already exists in this folder.\n\nReplace it?")
                                        .arg(QFileInfo(path).fileName()),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return; // She said no: the name is hers to change, and the dialog stays for that.
        }
    }

    m_destination = Destination::Library;
    accept();
}
