/************************************************************************
 **
 **  @file   oexportpdfdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the PDF destination dialog.
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

#include "oexportpdfdialog.h"

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

namespace
{
/// What the file is called when the user leaves the name alone.
constexpr auto kPdfSuffix = ".pdf";
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportPdfDialog::OExportPdfDialog(const QString &outputName, const QString &directory, QWidget *parent)
  : QDialog(parent)
{
    setModal(true);
    setMinimumWidth(520);

    auto *layout = new QGridLayout(this);

    m_nameLabel = new QLabel(this);
    m_nameEdit = new QLineEdit(outputName, this);

    m_pathLabel = new QLabel(this);
    m_pathEdit = new QLineEdit(directory, this);
    m_browseButton = new QPushButton(this);
    connect(m_browseButton, &QPushButton::clicked, this, &OExportPdfDialog::OnBrowse);

    layout->addWidget(m_nameLabel, 0, 0);
    layout->addWidget(m_nameEdit, 0, 1, 1, 2);
    layout->addWidget(m_pathLabel, 1, 0);
    layout->addWidget(m_pathEdit, 1, 1);
    layout->addWidget(m_browseButton, 1, 2);

    m_buttonBox = new QDialogButtonBox(this);
    auto *exportButton = m_buttonBox->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
    exportButton->setAutoDefault(false);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once every button is in: the box lays its buttons out as they arrive
    // and can hand the default to the one added after Export — Return would then cancel
    // the export the user just described.
    exportButton->setDefault(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OExportPdfDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(m_buttonBox, 2, 0, 1, 3);

    Retranslate();
    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdfDialog::FileName() const -> QString
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
    {
        return name;
    }

    if (!name.endsWith(QLatin1String(kPdfSuffix), Qt::CaseInsensitive))
    {
        name += QLatin1String(kPdfSuffix);
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdfDialog::Directory() const -> QString
{
    return m_pathEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdfDialog::FullPath() const -> QString
{
    return QDir(Directory()).filePath(FileName());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPdfDialog::OnBrowse()
{
    const QString chosen = QFileDialog::getExistingDirectory(this, tr("Folder"), Directory());
    if (!chosen.isEmpty())
    {
        m_pathEdit->setText(chosen);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPdfDialog::OnAccept()
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
    if (QFileInfo::exists(path))
    {
        const auto answer = QMessageBox::question(this, tr("Export"),
                                                  tr("%1 already exists. Replace it?").arg(FileName()),
                                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
        {
            return;
        }
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPdfDialog::Retranslate()
{
    setWindowTitle(tr("Export the plan as PDF"));
    m_nameLabel->setText(tr("File name:"));
    m_pathLabel->setText(tr("Folder:"));
    m_browseButton->setText(tr("Browse..."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPdfDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
