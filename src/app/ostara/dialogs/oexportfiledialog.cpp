/************************************************************************
 **
 **  @file   oexportfiledialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the shared export destination dialog.
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

#include "oexportfiledialog.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
OExportFileDialog::OExportFileDialog(const QString &outputName, const QString &directory,
                                     const QString &suffix, QWidget *parent)
  : QDialog(parent),
    m_suffix(suffix)
{
    setModal(true);
    setMinimumWidth(520);

    m_grid = new QGridLayout(this);

    m_nameLabel = new QLabel(this);
    m_nameEdit = new QLineEdit(outputName, this);

    m_pathLabel = new QLabel(this);
    m_pathEdit = new QLineEdit(directory, this);
    m_browseButton = new QPushButton(this);
    connect(m_browseButton, &QPushButton::clicked, this, &OExportFileDialog::OnBrowse);

    m_grid->addWidget(m_nameLabel, 0, 0);
    m_grid->addWidget(m_nameEdit, 0, 1, 1, 2);
    m_grid->addWidget(m_pathLabel, 1, 0);
    m_grid->addWidget(m_pathEdit, 1, 1);
    m_grid->addWidget(m_browseButton, 1, 2);

    m_buttonBox = new QDialogButtonBox(this);
    auto *exportButton = m_buttonBox->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
    exportButton->setAutoDefault(false);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once every button is in: the box lays its buttons out as they arrive
    // and can hand the default to the one added after Export — Return would then cancel
    // the export the user just described.
    exportButton->setDefault(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OExportFileDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_grid->addWidget(m_buttonBox, m_optionRow, 0, 1, 3);

    Retranslate();
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FileName() const -> QString
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
    {
        return name;
    }

    if (!m_suffix.isEmpty() && !name.endsWith(m_suffix, Qt::CaseInsensitive))
    {
        name += m_suffix;
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::Directory() const -> QString
{
    return m_pathEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::FullPath() const -> QString
{
    return QDir(Directory()).filePath(FileName());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::AddOption(QWidget *option)
{
    m_grid->addWidget(option, m_optionRow, 0, 1, 3);
    ++m_optionRow;

    // Les boutons suivent l'option, et c'est ce qui fait qu'un dialogue dérivé n'a pas à savoir où
    // ils vivent : il ajoute, le squelette replace.
    m_grid->removeWidget(m_buttonBox);
    m_grid->addWidget(m_buttonBox, m_optionRow, 0, 1, 3);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFileDialog::ConfirmOverwrite(QWidget *parent, const QString &filePath, const QString &title) -> bool
{
    if (!QFileInfo::exists(filePath))
    {
        return true;
    }

    const auto answer = QMessageBox::question(parent, title.isEmpty() ? tr("Export") : title,
                                              tr("%1 already exists. Replace it?")
                                                  .arg(QFileInfo(filePath).fileName()),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer == QMessageBox::Yes;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::OnBrowse()
{
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Folder"), Directory());
    if (!chosen.isEmpty())
    {
        m_pathEdit->setText(chosen);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::OnAccept()
{
    if (FileName().isEmpty())
    {
        QMessageBox::warning(this, tr("Export"), tr("The file needs a name."));
        m_nameEdit->setFocus();
        return;
    }

    const QFileInfo folder(Directory());
    if (!folder.isDir())
    {
        QMessageBox::warning(this, tr("Export"), tr("That folder does not exist."));
        m_pathEdit->setFocus();
        return;
    }

    const QString path = FullPath();
    if (!ConfirmOverwrite(this, path))
    {
        return;
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::Retranslate()
{
    // « Le plan », sans dire lequel : c'est le titre du squelette, et un dialogue dérivé nomme son
    // propre format dans le sien.
    setWindowTitle(tr("Export the plan"));
    m_nameLabel->setText(tr("File name:"));
    m_pathLabel->setText(tr("Folder:"));
    m_browseButton->setText(tr("Browse..."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFileDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
