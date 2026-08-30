/************************************************************************
 **
 **  @file   opiecedialog_patternstep.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Step 1 of the piece tool dialog: pattern-level information.
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

#include "opiecedialog_patternstep.h"

#include "../widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPiecePatternStep::OPiecePatternStep(VAbstractPattern *doc, QWidget *parent)
  : QWidget(parent),
    m_doc(doc)
{
    SCASSERT(m_doc != nullptr)

    auto *mainLayout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_patternNameEdit = new QLineEdit(m_doc->GetPatternName());
    form->addRow(tr("Pattern name"), m_patternNameEdit);

    // Client — from the client library (Information block > Clients); "No client"
    // as an explicit option. An unnamed client is not selectable. A customer
    // already set on the document but absent from the library is kept selectable
    // so accepting the dialog never silently drops it.
    m_clientCombo = MakeComboBox(this);
    m_clientCombo->addItem(tr("No client"), QString());
    const QString currentClient = m_doc->GetCustomer();
    const QList<OPieceCartoucheStorage::ClientEntry> clients = OPieceCartoucheStorage::LoadClients();
    int clientIndex = 0;  // "No client"
    for (const OPieceCartoucheStorage::ClientEntry &client : clients)
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
    form->addRow(tr("Client"), m_clientCombo);

    // Collection — from the collection library (Information block > Collections),
    // shown in the user's chosen year order; "No collection" as an explicit
    // option. An unnamed collection is not selectable. A collection already set
    // on the document but absent from the library is kept selectable so
    // accepting the dialog never silently drops it.
    m_collectionCombo = MakeComboBox(this);
    m_collectionCombo->addItem(tr("No collection"), QString());
    const QString currentCollection = m_doc->GetCollection();
    const bool ascending = VAbstractValApplication::VApp()->Settings()->GetCollectionsSortAscending();
    const QList<OPieceCartoucheStorage::CollectionEntry> collections = OPieceCartoucheStorage::LoadCollections(ascending);
    int collectionIndex = 0;  // "No collection"
    for (const OPieceCartoucheStorage::CollectionEntry &collection : collections)
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
    form->addRow(tr("Collection"), m_collectionCombo);

    // Fabric info — grows with its content (3 visible lines minimum).
    m_fabricInfoEdit = new QPlainTextEdit(m_doc->GetFabricInfo());
    m_fabricInfoEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const auto minFabricHeight = [this]() {
        return m_fabricInfoEdit->fontMetrics().lineSpacing() * 3 + m_fabricInfoEdit->frameWidth() * 2
               + static_cast<int>(m_fabricInfoEdit->document()->documentMargin()) * 2;
    };
    m_fabricInfoEdit->setFixedHeight(minFabricHeight());
    connect(m_fabricInfoEdit->document(), &QTextDocument::contentsChanged, this, [this, minFabricHeight]() {
        const int h = static_cast<int>(m_fabricInfoEdit->document()->size().height())
                      + m_fabricInfoEdit->frameWidth() * 2;
        m_fabricInfoEdit->setFixedHeight((std::max)(h, minFabricHeight()));
    });
    form->addRow(tr("Fabric info"), m_fabricInfoEdit);

    mainLayout->addLayout(form);

    auto *separator = new QFrame;
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(separator);

    // Seam allowances — pattern-level seeds applied to every piece created afterwards.
    auto *seamAllowanceGroup = new QGroupBox(tr("Seam Allowances"));
    auto *saLayout = new QVBoxLayout(seamAllowanceGroup);
    m_activateSACheck = new QCheckBox(tr("Activate seam allowances for all pieces"));
    m_activateSACheck->setChecked(m_doc->GetDefaultActivateSA());
    saLayout->addWidget(m_activateSACheck);
    m_hideMainPathCheck = new QCheckBox(tr("Hide main path for all pieces"));
    m_hideMainPathCheck->setChecked(m_doc->GetDefaultHideMainPath());
    saLayout->addWidget(m_hideMainPathCheck);
    mainLayout->addWidget(seamAllowanceGroup);

    // Information block — display is opted-in per pattern: the checkbox activates
    // the label for every piece created afterwards (same seed semantics as the
    // seam allowance checkboxes), and the combo picks the template. The combo only
    // appears once the box is checked and is pre-filled with the favourite template
    // or the shipped "Default" (the templates directory is never empty).
    OPieceCartoucheStorage::EnsureDefaultTemplate();
    auto *infoBlockGroup = new QGroupBox(tr("Information block"));
    auto *ibLayout = new QVBoxLayout(infoBlockGroup);
    const QString currentTemplate = m_doc->GetCartoucheTemplate();
    m_activateInfoBlockCheck = new QCheckBox(tr("Activate information block for all pieces"));
    m_activateInfoBlockCheck->setChecked(not currentTemplate.isEmpty());
    ibLayout->addWidget(m_activateInfoBlockCheck);
    m_cartoucheCombo = MakeComboBox(this);
    const QStringList templates = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    const QString defaultName = VAbstractValApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    QStringList baseNames;
    for (const QString &path : templates)
    {
        const QString base = QFileInfo(path).completeBaseName();
        baseNames.append(base);
        m_cartoucheCombo->addItem(base, base);
    }
    // The pattern's existing template wins over the favourite; the favourite is
    // only a fallback while no template is set on the pattern yet.
    int preferredIndex = OPieceCartoucheStorage::PreferredTemplateIndex(baseNames, currentTemplate, defaultName);
    if (!currentTemplate.isEmpty() && !baseNames.contains(currentTemplate))
    {
        // The pattern's template file is gone (deleted/renamed): keep it
        // selectable so accepting the dialog never drops the reference.
        m_cartoucheCombo->addItem(currentTemplate, currentTemplate);
        preferredIndex = m_cartoucheCombo->count() - 1;
    }
    m_cartoucheCombo->setCurrentIndex(preferredIndex);
    m_cartoucheCombo->setVisible(m_activateInfoBlockCheck->isChecked());
    connect(m_activateInfoBlockCheck, &QCheckBox::toggled, m_cartoucheCombo, &QWidget::setVisible);
    ibLayout->addWidget(m_cartoucheCombo);
    mainLayout->addWidget(infoBlockGroup);

    mainLayout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetCartouche() const -> QString
{
    // The template name only counts when the user opted in: unchecked means "no
    // information block" and the pattern keeps no cartouche reference.
    return m_activateInfoBlockCheck->isChecked() ? m_cartoucheCombo->currentData().toString() : QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetPatternName() const -> QString
{
    return m_patternNameEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetClient() const -> QString
{
    return m_clientCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetCollection() const -> QString
{
    return m_collectionCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::GetFabricInfo() const -> QString
{
    return m_fabricInfoEdit->toPlainText().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::IsActivateSA() const -> bool
{
    return m_activateSACheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePatternStep::IsHideMainPath() const -> bool
{
    return m_hideMainPathCheck->isChecked();
}
