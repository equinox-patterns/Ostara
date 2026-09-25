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
#include "otranslation.h"

#include <algorithm>

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
// Debounce delay for the fabric-info text edit — one commit per pause, not one
// per keystroke (Valentina anti-pattern: one undo command per keypress).
constexpr int kCommitDebounceMs = 600;
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
OPatternInfoWidget::OPatternInfoWidget(QWidget *parent)
  : QWidget(parent)
{
    m_patternNameEdit = new QLineEdit(this);
    m_clientCombo = MakeComboBox(this);
    m_collectionCombo = MakeComboBox(this);
    m_fabricInfoEdit = new QPlainTextEdit(this);

    // ⚠️ **Le titre passe par le poseur, il n'est plus écrit à la main.** Trois titres de ce widget
    // — celui-ci, « Seam Allowances » et « Information block » — étaient construits avec `tr()` dans
    // le constructeur et jamais repris : `RetranslateUI()` ne pouvait pas les atteindre, faute de
    // référence, et c'est ce « Pattern » qui restait en anglais dans le dock Options en mode
    // Details. Le poseur garde la chaîne source sur le widget, et le moteur la rejoue à chaque
    // bascule de langue — sans que cette classe ait à s'en occuper.
    m_patternGroup = new QGroupBox(this);
    OTranslation::SetTitle(this, m_patternGroup, "Pattern");
    auto *groupBox = m_patternGroup;
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

    // Seam allowances — pattern-level seeds applied to every piece created
    // afterwards (same semantics as the dialog step 1).
    m_seamAllowanceGroup = new QGroupBox(this);
    OTranslation::SetTitle(this, m_seamAllowanceGroup, "Seam Allowances");
    auto *seamAllowanceGroup = m_seamAllowanceGroup;
    auto *saLayout = new QVBoxLayout(seamAllowanceGroup);
    saLayout->setSpacing(6);

    // The scope of these seeds is stated once, under the group title, instead of repeating
    // "for all pieces" on every box.
    m_saHint = new QLabel(tr("Applies to all pieces, including those created afterwards. "
                             "Not retroactive: existing pieces are left unchanged."),
                          seamAllowanceGroup);
    m_saHint->setObjectName(QStringLiteral("patternGroupHint"));
    m_saHint->setWordWrap(true);
    saLayout->addWidget(m_saHint);

    m_activateSACheck = new QCheckBox(tr("Activate seam allowances"), seamAllowanceGroup);
    saLayout->addWidget(m_activateSACheck);
    m_hideMainPathCheck = new QCheckBox(tr("Hide main path"), seamAllowanceGroup);
    saLayout->addWidget(m_hideMainPathCheck);
    m_secondPassmarkCheck = new QCheckBox(tr("Show the second passmark on seam line"), seamAllowanceGroup);
    saLayout->addWidget(m_secondPassmarkCheck);
    m_sewLineOnDrawingCheck = new QCheckBox(tr("Export the sew line with the drawings (DXF-AAMA)"),
                                            seamAllowanceGroup);
    saLayout->addWidget(m_sewLineOnDrawingCheck);
    layout->addWidget(seamAllowanceGroup);

    // Information block — opted in per pattern; the combo is revealed once the
    // box is checked and picks the template (the pattern's template wins over
    // the settings favourite). Same seed semantics as the seam allowances.
    OPieceCartoucheStorage::EnsureDefaultTemplate();
    m_infoBlockGroup = new QGroupBox(this);
    OTranslation::SetTitle(this, m_infoBlockGroup, "Information block");
    auto *infoBlockGroup = m_infoBlockGroup;
    auto *ibLayout = new QVBoxLayout(infoBlockGroup);
    ibLayout->setSpacing(6);

    // Same scope statement as the seam allowances group above (identical seeding semantics).
    m_infoBlockHint = new QLabel(tr("Applies to all pieces, including those created afterwards. "
                                    "Not retroactive: existing pieces are left unchanged."),
                                 infoBlockGroup);
    m_infoBlockHint->setObjectName(QStringLiteral("patternGroupHint"));
    m_infoBlockHint->setWordWrap(true);
    ibLayout->addWidget(m_infoBlockHint);

    m_activateInfoBlockCheck = new QCheckBox(tr("Activate information block"), infoBlockGroup);
    ibLayout->addWidget(m_activateInfoBlockCheck);
    m_cartoucheCombo = MakeComboBox(infoBlockGroup);
    m_cartoucheCombo->setVisible(false);
    ibLayout->addWidget(m_cartoucheCombo);
    layout->addWidget(infoBlockGroup);

    layout->addStretch();

    m_commitTimer = new QTimer(this);
    m_commitTimer->setSingleShot(true);
    m_commitTimer->setInterval(kCommitDebounceMs);

    connect(m_patternNameEdit, &QLineEdit::editingFinished, this, &OPatternInfoWidget::CommitPatternName);
    connect(m_clientCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OPatternInfoWidget::CommitClient);
    connect(m_collectionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OPatternInfoWidget::CommitCollection);
    connect(m_activateSACheck, &QCheckBox::toggled, this, &OPatternInfoWidget::CommitActivateSA);
    connect(m_hideMainPathCheck, &QCheckBox::toggled, this, &OPatternInfoWidget::CommitHideMainPath);
    connect(m_secondPassmarkCheck, &QCheckBox::toggled, this,
            &OPatternInfoWidget::CommitSecondPassmarkOnSeamLine);
    connect(m_sewLineOnDrawingCheck, &QCheckBox::toggled, this, &OPatternInfoWidget::CommitSewLineOnDrawing);
    connect(m_activateInfoBlockCheck, &QCheckBox::toggled, this, &OPatternInfoWidget::CommitCartoucheActivation);
    connect(m_cartoucheCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OPatternInfoWidget::CommitCartoucheTemplate);
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
    // Stop any pending commit first: the document is about to change, and a
    // stale delayed commit must not write the previous document's text into it.
    m_commitTimer->stop();
    m_clients = OPieceCartoucheStorage::LoadClients();
    const bool ascending = VAbstractValApplication::VApp()->Settings()->GetCollectionsSortAscending();
    m_collections = OPieceCartoucheStorage::LoadCollections(ascending);
    m_templates = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
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

    // Seam allowance / cartouche seeds — pattern-level, read at piece creation
    // only. Existing pieces are never rewritten.
    m_activateSACheck->setChecked(doc != nullptr && doc->GetDefaultActivateSA());
    m_hideMainPathCheck->setChecked(doc != nullptr && doc->GetDefaultHideMainPath());
    m_secondPassmarkCheck->setChecked(doc != nullptr && doc->GetDefaultSecondPassmarkOnSeamLine());
    m_sewLineOnDrawingCheck->setChecked(doc != nullptr && doc->GetDefaultSewLineOnDrawing());

    const QString currentTemplate = doc != nullptr ? doc->GetCartoucheTemplate() : QString();
    m_activateInfoBlockCheck->setChecked(!currentTemplate.isEmpty());

    // Template combo — the pattern's template wins over the settings favourite
    // (shared helper, identical selection in the dialog step 1).
    m_cartoucheCombo->blockSignals(true);
    m_cartoucheCombo->clear();
    const QString defaultName = VAbstractValApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    QStringList baseNames;
    for (const QString &path : m_templates)
    {
        const QString base = QFileInfo(path).completeBaseName();
        baseNames.append(base);
        m_cartoucheCombo->addItem(base, base);
    }
    int preferredIndex = OPieceCartoucheStorage::PreferredTemplateIndex(baseNames, currentTemplate, defaultName);
    if (!currentTemplate.isEmpty() && !baseNames.contains(currentTemplate))
    {
        // The pattern's template file is gone (deleted/renamed): keep it
        // selectable so an activation toggle never silently overwrites it.
        m_cartoucheCombo->addItem(currentTemplate, currentTemplate);
        preferredIndex = m_cartoucheCombo->count() - 1;
    }
    m_cartoucheCombo->setCurrentIndex(preferredIndex);
    m_cartoucheCombo->blockSignals(false);
    m_cartoucheCombo->setVisible(!currentTemplate.isEmpty());

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
    if (m_saHint)
    {
        m_saHint->setText(tr("Applies to all pieces, including those created afterwards. "
                             "Not retroactive: existing pieces are left unchanged."));
    }
    if (m_activateSACheck)
    {
        m_activateSACheck->setText(tr("Activate seam allowances"));
    }
    if (m_hideMainPathCheck)
    {
        m_hideMainPathCheck->setText(tr("Hide main path"));
    }
    if (m_secondPassmarkCheck)
    {
        m_secondPassmarkCheck->setText(tr("Show the second passmark on seam line"));
    }
    if (m_sewLineOnDrawingCheck)
    {
        m_sewLineOnDrawingCheck->setText(tr("Export the sew line with the drawings (DXF-AAMA)"));
    }
    if (m_infoBlockHint)
    {
        m_infoBlockHint->setText(tr("Applies to all pieces, including those created afterwards. "
                                    "Not retroactive: existing pieces are left unchanged."));
    }
    if (m_activateInfoBlockCheck)
    {
        m_activateInfoBlockCheck->setText(tr("Activate information block"));
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
        doc->CheckLayout();
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
        doc->CheckLayout();
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
        doc->CheckLayout();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitActivateSA(bool on)
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
    if (on != doc->GetDefaultActivateSA())
    {
        doc->SetDefaultActivateSA(on);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitHideMainPath(bool on)
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
    if (on != doc->GetDefaultHideMainPath())
    {
        doc->SetDefaultHideMainPath(on);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitSewLineOnDrawing(bool on)
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
    if (on != doc->GetDefaultSewLineOnDrawing())
    {
        doc->SetDefaultSewLineOnDrawing(on);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitSecondPassmarkOnSeamLine(bool on)
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
    if (on != doc->GetDefaultSecondPassmarkOnSeamLine())
    {
        doc->SetDefaultSecondPassmarkOnSeamLine(on);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitCartoucheActivation(bool on)
{
    // The combo is revealed exactly when the block is activated — including
    // while Populate() runs, so the two states never drift apart.
    m_cartoucheCombo->setVisible(on);
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr || doc != m_doc)
    {
        return;
    }
    if (on)
    {
        // Nothing selectable (degraded templates folder) — do not write an empty
        // cartouche reference on a bare activation.
        if (m_cartoucheCombo->currentIndex() < 0)
        {
            return;
        }
        const QString name = m_cartoucheCombo->itemData(m_cartoucheCombo->currentIndex()).toString();
        if (name != doc->GetCartoucheTemplate())
        {
            doc->SetCartoucheTemplate(name);
        }
    }
    else if (!doc->GetCartoucheTemplate().isEmpty())
    {
        // Deactivating removes the pattern-level cartouche reference.
        doc->SetCartoucheTemplate(QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternInfoWidget::CommitCartoucheTemplate(int index)
{
    if (m_loading || !m_activateInfoBlockCheck->isChecked())
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr || doc != m_doc)
    {
        return;
    }
    const QString name = m_cartoucheCombo->itemData(index).toString();
    if (name != doc->GetCartoucheTemplate())
    {
        doc->SetCartoucheTemplate(name);
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
        doc->CheckLayout();
    }
}
