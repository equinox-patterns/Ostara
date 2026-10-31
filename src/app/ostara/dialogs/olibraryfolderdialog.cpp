/************************************************************************
 **
 **  @file   olibraryfolderdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  A folder made in a library, named and created the same way everywhere.
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

#include "olibraryfolderdialog.h"

#include "core/olibrarystorage.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OLibraryFolderDialog::OLibraryFolderDialog(const QString &parentPath, QWidget *parent)
    : QDialog(parent), m_parentPath(parentPath)
{
    SetupUI();
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryFolderDialog::CreatedPath() const -> QString
{
    return m_createdPath;
}

//---------------------------------------------------------------------------------------------------------------------
void OLibraryFolderDialog::SetupUI()
{
    setMinimumWidth(420);

    auto *layout = new QVBoxLayout(this);

    auto *nameForm = new QFormLayout();
    m_nameLabel = new QLabel();
    m_nameEdit = new QLineEdit();
    nameForm->addRow(m_nameLabel, m_nameEdit);
    layout->addLayout(nameForm);

    layout->addStretch(1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OLibraryFolderDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    Retranslate();
    m_nameEdit->setFocus();
}

//---------------------------------------------------------------------------------------------------------------------
void OLibraryFolderDialog::Retranslate()
{
    setWindowTitle(tr("New folder"));

    m_nameLabel->setText(tr("Name:"));
    m_nameEdit->setPlaceholderText(tr("Folder name"));

    if (QPushButton *ok = m_buttonBox->button(QDialogButtonBox::Ok); ok != nullptr)
    {
        ok->setText(tr("Create"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OLibraryFolderDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OLibraryFolderDialog::OnAccept()
{
    const QString name = m_nameEdit->text().trimmed();

    QString error;
    if (not OLibraryStorage::CreateFolder(m_parentPath, name, &error))
    {
        QMessageBox::warning(this, tr("New folder"), error);
        m_nameEdit->setFocus();
        return;
    }

    // The folder exists from here on: the caller is told exactly which one, so it never recomputes a name that
    // could have been trimmed on the way.
    m_createdPath = QDir(m_parentPath).absoluteFilePath(name);
    accept();
}
