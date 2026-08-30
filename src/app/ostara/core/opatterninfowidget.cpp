/************************************************************************
 **
 **  @file   opatterninfowidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   30/08/2026
 **
 **  @brief  Pattern information page — implementation.
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

#include "opatterninfowidget.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../widgets/InfoBlock/OPieceCartoucheStorage.h"

#include <algorithm>

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPatternInfoWidget::OPatternInfoWidget(QWidget *parent)
  : QWidget(parent)
{
    m_patternNameEdit = new QLineEdit(this);
    m_clientCombo = MakeComboBox(this);
    m_collectionCombo = MakeComboBox(this);
    m_fabricInfoEdit = new QPlainTextEdit(this);

    auto *groupBox = new QGroupBox(tr("Pattern"), this);
    auto *groupLayout = new QVBoxLayout(groupBox);
    groupLayout->setSpacing(6);

    m_patternNameLabel = new QLabel(tr("Pattern name:"), groupBox);
    groupLayout->addWidget(m_patternNameLabel);
    groupLayout->addWidget(m_patternNameEdit);

    m_clientLabel = new QLabel(tr("Client:"), groupBox);
    groupLayout->addWidget(m_clientLabel);
    groupLayout->addWidget(m_clientCombo);

    m_collectionLabel = new QLabel(tr("Collection:"), groupBox);
    groupLayout->addWidget(m_collectionLabel);
    groupLayout->addWidget(m_collectionCombo);

    m_fabricLabel = new QLabel(tr("Fabric info:"), groupBox);
    groupLayout->addWidget(m_fabricLabel);

    // Fabric info — grows with its content (3 visible lines minimum).
    m_fabricInfoEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const auto minFabricHeight = [this]() {
        return m_fabricInfoEdit->fontMetrics().lineSpacing() * 3 + m_fabricInfoEdit->frameWidth() * 2
               + static_cast<int>(m_fabricInfoEdit->document()->documentMargin()) * 2;
    };
    m_fabricInfoEdit->setFixedHeight(minFabricHeight());
    connect(m_fabricInfoEdit, &QPlainTextEdit::textChanged, this, [this, minFabricHeight]() {
        const int h = static_cast<int>(m_fabricInfoEdit->document()->size().height())
                      + m_fabricInfoEdit->frameWidth() * 2;
        m_fabricInfoEdit->setFixedHeight((std::max)(h, minFabricHeight()));
        ScheduleCommit();
    });
    groupLayout->addWidget(m_fabricInfoEdit);

    groupLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(groupBox);

    m_commitTimer = new QTimer(this);
    m_commitTimer->setSingleShot(true);
    m_commitTimer->setInterval(600);

    connect(m_patternNameEdit, &QLineEdit::editingFinished, this, &OPatternInfoWidget::CommitPatternName);
    connect(m_clientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OPatternInfoWidget::CommitClient);
    connect(m_collectionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OPatternInfoWidget::CommitCollection);
    connect(m_commitTimer, &QTimer::timeout, this, &OPatternInfoWidget::CommitDelayed);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternInfoWidget::CurrentDocument() const -> VAbstractPattern *
{
    auto *app = VAbstractValApplication::VApp();
    return app != nullptr ? app->getCurrentDocument() : nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::Refresh()
{
    QPointer<VAbstractPattern> doc(CurrentDocument());
    if (doc != m_doc)
    {
        if (m_doc)
        {
            disconnect(m_doc, &VAbstractPattern::FullUpdateFromFile, this, &OPatternInfoWidget::OnFullUpdate);
        }
        m_doc = doc;
        if (m_doc)
        {
            connect(m_doc, &VAbstractPattern::FullUpdateFromFile, this, &OPatternInfoWidget::OnFullUpdate);
        }
        // New document bound: reload the libraries and repopulate from it.
        OnFullUpdate();
    }
    else
    {
        // Same document: cancel any pending commit and repopulate from cache.
        m_commitTimer->stop();
        Populate();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::OnFullUpdate()
{
    m_clients = OPieceCartoucheStorage::LoadClients();
    const bool ascending = VAbstractValApplication::VApp()->Settings()->GetCollectionsSortAscending();
    m_collections = OPieceCartoucheStorage::LoadCollections(ascending);
    m_commitTimer->stop();
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::Populate()
{
    m_loading = true;

    const VAbstractPattern *doc = CurrentDocument();
    m_patternNameEdit->setText(doc != nullptr ? doc->GetPatternName() : QString());

    // Client — from the cached library; "No client" as an explicit option. An
    // unnamed client is not selectable. A customer already set on the document
    // but absent from the library is kept selectable so it is never dropped.
    m_clientCombo->blockSignals(true);
    m_clientCombo->clear();
    m_clientCombo->addItem(tr("No client"), QString());
    const QString currentClient = doc != nullptr ? doc->GetCustomer() : QString();
    int clientIndex = 0; // "No client"
    for (const OPieceCartoucheStorage::ClientEntry &client : m_clients)
    {
        if (client.name.isEmpty())
        {
            continue;
        }
        m_clientCombo->addItem(client.name, client.name);
        if (client.name == currentClient)
        {
            clientIndex = m_clientCombo->count() - 1;
        }
    }
    if (!currentClient.isEmpty() && clientIndex == 0)
    {
        m_clientCombo->addItem(currentClient, currentClient);
        clientIndex = m_clientCombo->count() - 1;
    }
    m_clientCombo->setCurrentIndex(clientIndex);
    m_clientCombo->blockSignals(false);

    // Collection — from the cached library, in the user's chosen year order.
    m_collectionCombo->blockSignals(true);
    m_collectionCombo->clear();
    m_collectionCombo->addItem(tr("No collection"), QString());
    const QString currentCollection = doc != nullptr ? doc->GetCollection() : QString();
    int collectionIndex = 0; // "No collection"
    for (const OPieceCartoucheStorage::CollectionEntry &collection : m_collections)
    {
        if (collection.name.isEmpty())
        {
            continue;
        }
        m_collectionCombo->addItem(collection.name, collection.name);
        if (collection.name == currentCollection)
        {
            collectionIndex = m_collectionCombo->count() - 1;
        }
    }
    if (!currentCollection.isEmpty() && collectionIndex == 0)
    {
        m_collectionCombo->addItem(currentCollection, currentCollection);
        collectionIndex = m_collectionCombo->count() - 1;
    }
    m_collectionCombo->setCurrentIndex(collectionIndex);
    m_collectionCombo->blockSignals(false);

    m_fabricInfoEdit->setPlainText(doc != nullptr ? doc->GetFabricInfo() : QString());

    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::RetranslateUI()
{
    if (m_patternNameLabel)
    {
        m_patternNameLabel->setText(tr("Pattern name:"));
    }
    if (m_clientLabel)
    {
        m_clientLabel->setText(tr("Client:"));
    }
    if (m_collectionLabel)
    {
        m_collectionLabel->setText(tr("Collection:"));
    }
    if (m_fabricLabel)
    {
        m_fabricLabel->setText(tr("Fabric info:"));
    }
    if (m_clientCombo)
    {
        m_clientCombo->setItemText(0, tr("No client"));
        m_clientCombo->setItemData(0, QString());
    }
    if (m_collectionCombo)
    {
        m_collectionCombo->setItemText(0, tr("No collection"));
        m_collectionCombo->setItemData(0, QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitPatternName()
{
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr || doc != m_doc)
    {
        return;
    }
    const QString name = m_patternNameEdit->text();
    if (name != doc->GetPatternName())
    {
        doc->SetPatternName(name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitClient(int index)
{
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr || doc != m_doc)
    {
        return;
    }
    const QString client = m_clientCombo->itemData(index).toString();
    if (client != doc->GetCustomer())
    {
        doc->SetCustomer(client);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitCollection(int index)
{
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr || doc != m_doc)
    {
        return;
    }
    const QString collection = m_collectionCombo->itemData(index).toString();
    if (collection != doc->GetCollection())
    {
        doc->SetCollection(collection);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::ScheduleCommit()
{
    if (m_loading)
    {
        return;
    }
    m_commitTimer->start();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitDelayed()
{
    m_commitTimer->stop();
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr || doc != m_doc)
    {
        return;
    }
    const QString fabricInfo = m_fabricInfoEdit->toPlainText();
    if (fabricInfo != doc->GetFabricInfo())
    {
        doc->SetFabricInfo(fabricInfo);
    }
}
