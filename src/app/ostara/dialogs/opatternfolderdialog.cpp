/************************************************************************
 **
 **  @file   opatternfolderdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  A folder made in the pattern library — and, when asked, the client or the collection of the same name.
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

#include "opatternfolderdialog.h"

#include "core/opatternlibrary.h"
#include "widgets/InfoBlock/OPieceCartoucheStorage.h"

#include <QCheckBox>
#include <QDate>
#include <QDialogButtonBox>
#include <QDir>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPatternFolderDialog::OPatternFolderDialog(const QString &parentPath, QWidget *parent)
    : QDialog(parent), m_parentPath(parentPath)
{
    SetupUI();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternFolderDialog::CreatedPath() const -> QString
{
    return m_createdPath;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::SetupUI()
{
    setMinimumWidth(480);

    auto *layout = new QVBoxLayout(this);

    auto *nameForm = new QFormLayout();
    m_nameLabel = new QLabel();
    m_nameEdit = new QLineEdit();
    nameForm->addRow(m_nameLabel, m_nameEdit);
    layout->addLayout(nameForm);

    // --- The client bridge ---
    m_addClientCheck = new QCheckBox();
    connect(m_addClientCheck, &QCheckBox::toggled, this, &OPatternFolderDialog::OnAddToClientToggled);
    layout->addWidget(m_addClientCheck);

    m_clientGroup = new QGroupBox();
    auto *clientForm = new QFormLayout(m_clientGroup);
    m_clientMailEdit = new QLineEdit();
    m_clientMailLabel = new QLabel();
    clientForm->addRow(m_clientMailLabel, m_clientMailEdit);
    m_clientPhoneEdit = new QLineEdit();
    m_clientPhoneLabel = new QLabel();
    clientForm->addRow(m_clientPhoneLabel, m_clientPhoneEdit);
    m_clientInfoEdit = new QPlainTextEdit();
    m_clientInfoEdit->setFixedHeight(70);
    m_clientInfoLabel = new QLabel();
    clientForm->addRow(m_clientInfoLabel, m_clientInfoEdit);
    layout->addWidget(m_clientGroup);

    // --- The collection bridge ---
    m_addCollectionCheck = new QCheckBox();
    connect(m_addCollectionCheck, &QCheckBox::toggled, this, &OPatternFolderDialog::OnAddToCollectionToggled);
    layout->addWidget(m_addCollectionCheck);

    m_collectionGroup = new QGroupBox();
    auto *collectionForm = new QFormLayout(m_collectionGroup);
    m_collectionYearSpin = new QSpinBox();
    m_collectionYearSpin->setRange(1900, 2100);
    m_collectionYearSpin->setValue(QDate::currentDate().year());
    m_collectionYearLabel = new QLabel();
    collectionForm->addRow(m_collectionYearLabel, m_collectionYearSpin);
    m_collectionPeriodEdit = new QLineEdit();
    m_collectionPeriodLabel = new QLabel();
    collectionForm->addRow(m_collectionPeriodLabel, m_collectionPeriodEdit);
    m_collectionDescriptionEdit = new QPlainTextEdit();
    m_collectionDescriptionEdit->setFixedHeight(70);
    m_collectionDescriptionLabel = new QLabel();
    collectionForm->addRow(m_collectionDescriptionLabel, m_collectionDescriptionEdit);
    layout->addWidget(m_collectionGroup);

    layout->addStretch(1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &OPatternFolderDialog::OnAccept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    Retranslate();

    // Nothing of the two bridges is shown until it is asked for: the dialog's own question is the folder's name.
    ShowClientFields(false);
    ShowCollectionFields(false);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::Retranslate()
{
    setWindowTitle(tr("New folder"));

    m_nameLabel->setText(tr("Name:"));
    m_nameEdit->setPlaceholderText(tr("Folder name"));

    m_addClientCheck->setText(tr("Create the client of the same name"));
    m_clientGroup->setTitle(tr("Client"));
    m_clientMailLabel->setText(tr("Email:"));
    m_clientMailEdit->setPlaceholderText(tr("Email"));
    m_clientPhoneLabel->setText(tr("Phone:"));
    m_clientPhoneEdit->setPlaceholderText(tr("Phone"));
    m_clientInfoLabel->setText(tr("Information:"));
    m_clientInfoEdit->setPlaceholderText(tr("Information"));

    m_addCollectionCheck->setText(tr("Create the collection of the same name"));
    m_collectionGroup->setTitle(tr("Collection"));
    m_collectionYearLabel->setText(tr("Year:"));
    m_collectionPeriodLabel->setText(tr("Period:"));
    m_collectionPeriodEdit->setPlaceholderText(tr("Period"));
    m_collectionDescriptionLabel->setText(tr("Description:"));
    m_collectionDescriptionEdit->setPlaceholderText(tr("Description"));

    if (QPushButton *ok = m_buttonBox->button(QDialogButtonBox::Ok); ok != nullptr)
    {
        ok->setText(tr("Create"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::ShowClientFields(bool visible)
{
    m_clientGroup->setVisible(visible);
    adjustSize();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::ShowCollectionFields(bool visible)
{
    m_collectionGroup->setVisible(visible);
    adjustSize();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::OnAddToClientToggled(bool checked)
{
    ShowClientFields(checked);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::OnAddToCollectionToggled(bool checked)
{
    ShowCollectionFields(checked);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternFolderDialog::ClientNamed(const QString &name) -> QString
{
    const QList<OPieceCartoucheStorage::ClientEntry> clients = OPieceCartoucheStorage::LoadClients();
    for (const OPieceCartoucheStorage::ClientEntry &client : clients)
    {
        // Two names a combo cannot tell apart are one name here: the comparison ignores the case, and an entry
        // left unnamed is nobody.
        if (not client.name.trimmed().isEmpty() && client.name.trimmed().compare(name, Qt::CaseInsensitive) == 0)
        {
            return client.id;
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternFolderDialog::CollectionNamed(const QString &name) -> QString
{
    const QList<OPieceCartoucheStorage::CollectionEntry> collections = OPieceCartoucheStorage::LoadCollections(false);
    for (const OPieceCartoucheStorage::CollectionEntry &collection : collections)
    {
        if (not collection.name.trimmed().isEmpty()
            && collection.name.trimmed().compare(name, Qt::CaseInsensitive) == 0)
        {
            return collection.id;
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternFolderDialog::OnAccept()
{
    const QString name = m_nameEdit->text().trimmed();

    QString error;
    if (not OPatternLibrary::ValidateChildName(m_parentPath, name, &error))
    {
        QMessageBox::warning(this, tr("New folder"), error);
        m_nameEdit->setFocus();
        return;
    }

    const bool wantsClient = m_addClientCheck->isChecked();
    const bool wantsCollection = m_addCollectionCheck->isChecked();

    // The two bridges are settled **before** anything is written: a name already taken on the other side is a
    // question, and a refusal means the name changes — never a second entry nobody could tell from the first.
    if (wantsClient && not ClientNamed(name).isEmpty())
    {
        const QMessageBox::StandardButton answer
            = QMessageBox::question(this, tr("New folder"),
                                    tr("A client called \"%1\" already exists in the Information block.\n\n"
                                       "Link the folder to it?")
                                        .arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::Yes)
        {
            QMessageBox::information(this, tr("New folder"),
                                     tr("Rename the folder or the client, so that the two can be told apart."));
            m_nameEdit->setFocus();
            return;
        }
    }

    if (wantsCollection && not CollectionNamed(name).isEmpty())
    {
        const QMessageBox::StandardButton answer
            = QMessageBox::question(this, tr("New folder"),
                                    tr("A collection called \"%1\" already exists in the Information block.\n\n"
                                       "Link the folder to it?")
                                        .arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::Yes)
        {
            QMessageBox::information(this, tr("New folder"),
                                     tr("Rename the folder or the collection, so that the two can be told apart."));
            m_nameEdit->setFocus();
            return;
        }
    }

    if (not OPatternLibrary::CreateFolder(m_parentPath, name, &error))
    {
        QMessageBox::warning(this, tr("New folder"), error);
        return;
    }

    m_createdPath = QDir(m_parentPath).absoluteFilePath(name);

    // A folder made and an entry not: the folder is what was asked for first, so it stays and the failure is
    // said out loud rather than undone.
    if (wantsClient && ClientNamed(name).isEmpty())
    {
        const QString id = OPieceCartoucheStorage::AddClient(&error);
        if (id.isEmpty()
            || not OPieceCartoucheStorage::UpdateClientMeta(id, name, m_clientMailEdit->text().trimmed(),
                                                            m_clientPhoneEdit->text().trimmed(),
                                                            m_clientInfoEdit->toPlainText().trimmed(), &error))
        {
            QMessageBox::warning(this, tr("New folder"),
                                 tr("The folder was created, but the client could not be: %1").arg(error));
        }
    }

    if (wantsCollection && CollectionNamed(name).isEmpty())
    {
        const QString id = OPieceCartoucheStorage::AddCollection(&error);
        if (id.isEmpty()
            || not OPieceCartoucheStorage::UpdateCollectionMeta(id, name, m_collectionYearSpin->value(),
                                                               m_collectionPeriodEdit->text().trimmed(),
                                                               m_collectionDescriptionEdit->toPlainText().trimmed(),
                                                               &error))
        {
            QMessageBox::warning(this, tr("New folder"),
                                 tr("The folder was created, but the collection could not be: %1").arg(error));
        }
    }

    accept();
}
