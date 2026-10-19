/************************************************************************
 **
 **  @file   oblockconfigwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Blocks sub-tab of the Information dock tab (see oblockconfigwidget.h).
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

#include "oblockconfigwidget.h"

#include "../widgets/InfoBlock/OInfoBlockResolver.h"
#include "../widgets/InfoBlock/OPieceCartoucheCatalogue.h"
#include "../widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../widgets/InfoBlock/OPieceCartoucheTemplate.h"

#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "otranslation.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace
{
constexpr int kMaxBlocks = 11;
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
OBlockConfigWidget::OBlockConfigWidget(QWidget *parent) : QWidget(parent)
{
    // Same content style as the other dock tabs: group box sections, labels
    // above the fields. Side margins on the tab content: the top section and
    // the group box are no longer glued to the tab edges.
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 0);
    layout->setSpacing(16);  // clear gap between the template row and the group box

    // The checkbox and template row align with the group box frame (the group's
    // side margin is suppressed below, so both sit at x = 0).
    auto *topSection = new QVBoxLayout();
    topSection->setContentsMargins(0, 0, 0, 0);
    topSection->setSpacing(6);

    // ⚠️ **Ces trois textes passent par le poseur** (`otranslation.h`). La classe a bien un
    // `RetranslateUI()` complet, mais la page de groupe en crée une **seconde instance** à laquelle
    // personne ne pense à l'appeler : c'est celle-là qui gardait « Split into multiple blocks » en
    // anglais dans un mode Details français. Avec le poseur, les deux instances sont couvertes par
    // le moteur, sans que l'appelant ait quoi que ce soit à savoir.
    m_activeCheck = new QCheckBox(this);
    OTranslation::SetText(this, m_activeCheck, QT_TRANSLATE_NOOP("OBlockConfigWidget", "Activate information block"));
    topSection->addWidget(m_activeCheck);

    auto *templateRow = new QHBoxLayout();
    templateRow->setContentsMargins(0, 0, 0, 0);
    templateRow->setSpacing(6);
    m_templateLabel = new QLabel(this);
    OTranslation::SetText(this, m_templateLabel, QT_TRANSLATE_NOOP("OBlockConfigWidget", "Template:"));
    m_templateCombo = MakeComboBox(this);
    m_templateCombo->setEnabled(false);
    templateRow->addWidget(m_templateLabel);
    templateRow->addWidget(m_templateCombo);
    templateRow->addStretch();
    topSection->addLayout(templateRow);

    layout->addLayout(topSection);

    // No inner scroll area: the dispatch list lives directly in the group box so
    // it shares the group's $DockBody background (a QScrollArea viewport would be
    // forced to $DockTitle by ApplyThemeToWindows — SubtiliteCode §13).
    m_dispatchGroup = new QGroupBox(this);
    OTranslation::SetTitle(this, m_dispatchGroup, QT_TRANSLATE_NOOP("OBlockConfigWidget", "Split into multiple blocks"));
    // Less side margin than the theme default (8px) for this group only.
    m_dispatchGroup->setStyleSheet(QStringLiteral("QGroupBox { margin-left: 0px; margin-right: 0px; }"));
    auto *groupLayout = new QVBoxLayout(m_dispatchGroup);
    groupLayout->setSpacing(6);
    m_dispatchGroup->setEnabled(false);
    m_listLayout = new QFormLayout();
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(4);
    groupLayout->addLayout(m_listLayout);
    groupLayout->addStretch();
    layout->addWidget(m_dispatchGroup, 1);

    connect(m_activeCheck, &QCheckBox::toggled, this, [this](bool on) {
        m_templateCombo->setEnabled(on);
        m_dispatchGroup->setEnabled(on);
        if (m_loading)
        {
            return;
        }
        Commit();
    });
    connect(m_templateCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        if (m_loading)
        {
            return;
        }
        const QHash<QString, int> oldDispatch = ReadDispatch();
        m_loading = true;
        RebuildInfoList(m_templateCombo->currentData().toString(), oldDispatch);
        m_loading = false;
        Commit();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::SetModifyPending(std::function<void(std::function<void(IPieceInfo &)>)> callback)
{
    m_modifyPending = std::move(callback);
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::Show(const IPieceInfo &detail, const VContainer *data)
{
    m_loading = true;

    VAbstractPattern *doc = VAbstractValApplication::VApp()->getCurrentDocument();
    const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveConfig(detail, doc, data);

    m_activeCheck->setChecked(state.active);
    m_templateCombo->setEnabled(state.active);
    m_dispatchGroup->setEnabled(state.active);

    m_templatePaths = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    m_templateCombo->blockSignals(true);
    m_templateCombo->clear();
    QStringList baseNames;
    for (const QString &path : m_templatePaths)
    {
        const QString base = QFileInfo(path).completeBaseName();
        baseNames.append(base);
        m_templateCombo->addItem(base, base);
    }
    const QString defaultName = VAbstractValApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    int preferredIndex = OPieceCartoucheStorage::PreferredTemplateIndex(baseNames, state.templateName, defaultName);
    if (!state.templateName.isEmpty() && !baseNames.contains(state.templateName))
    {
        // The template file is gone (deleted/renamed): keep it selectable so an
        // edit never silently overwrites the piece's effective template.
        m_templateCombo->addItem(state.templateName, state.templateName);
        preferredIndex = m_templateCombo->count() - 1;
    }
    m_templateCombo->setCurrentIndex(preferredIndex);
    m_templateCombo->blockSignals(false);

    // Build the dispatch rows from the template actually shown in the combo (which falls
    // back to the user's preferred template), not from state.templateName — that one is
    // empty when the seed is unset, leaving "Repartir en plusieurs blocs" empty until the
    // user manually toggles the cartouche.
    const QString effectiveTemplate = m_templateCombo->currentData().toString();
    RebuildInfoList(effectiveTemplate, state.dispatch);

    m_lastTemplateName = effectiveTemplate;
    m_lastDispatch = state.dispatch;
    m_hasState = true;

    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::Clear()
{
    m_loading = true;
    m_activeCheck->setChecked(false);
    m_templateCombo->blockSignals(true);
    m_templateCombo->clear();
    m_templateCombo->blockSignals(false);
    m_dispatchGroup->setEnabled(false);
    ClearRows();
    m_hasState = false;
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::RetranslateUI()
{
    m_activeCheck->setText(tr("Activate information block"));
    m_templateLabel->setText(tr("Template:"));
    m_dispatchGroup->setTitle(tr("Split into multiple blocks"));
    if (m_hasState)
    {
        m_loading = true;
        RebuildInfoList(m_lastTemplateName, m_lastDispatch);
        m_loading = false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::ClearRows()
{
    while (m_listLayout->rowCount() > 0)
    {
        m_listLayout->removeRow(0);  // removes and deletes the row widgets
    }
    m_rows.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::RebuildInfoList(const QString &templateName, const QHash<QString, int> &dispatch)
{
    ClearRows();
    if (templateName.isEmpty())
    {
        return;
    }

    OPieceCartoucheTemplate tpl;
    for (const QString &path : m_templatePaths)
    {
        if (QFileInfo(path).completeBaseName() == templateName)
        {
            QString error;
            if (OPieceCartoucheStorage::Load(path, &tpl, &error))
            {
                break;
            }
        }
    }
    if (tpl.IsEmpty())
    {
        return;
    }

    for (int i = 0; i < tpl.SectionCount(); ++i)
    {
        const QString typeId = tpl.SectionTypeId(i);
        const OTemplateSection section = OPieceCartoucheCatalogue::Section(typeId);
        const QString displayName = section.IsValid() ? section.DisplayName() : typeId;

        auto *label = new QLabel(displayName, this);
        auto *combo = MakeComboBox(this);
        // First entry: the information is shown nowhere. It is a real choice, stored as block 0,
        // so a piece no longer needs a dedicated template just to leave one line out.
        combo->addItem(tr("Don't show"), 0);
        for (int block = 1; block <= kMaxBlocks; ++block)
        {
            combo->addItem(tr("Block %1").arg(block), block);
        }
        const int currentBlock = dispatch.value(typeId, 1);
        const int currentIndex = combo->findData(currentBlock);
        combo->setCurrentIndex(currentIndex >= 0 ? currentIndex : combo->findData(1));
        connect(combo, &QComboBox::currentIndexChanged, this, [this](int) {
            if (!m_loading)
            {
                Commit();
            }
        });

        m_listLayout->addRow(label, combo);
        m_rows.append({typeId, combo});
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OBlockConfigWidget::ReadDispatch() const -> QHash<QString, int>
{
    QHash<QString, int> dispatch;
    for (const auto &row : m_rows)
    {
        dispatch.insert(row.first, row.second->currentData().toInt());
    }
    return dispatch;
}

//---------------------------------------------------------------------------------------------------------------------
auto OBlockConfigWidget::BuildData(const IPieceInfo &det) const -> OInfoBlockData
{
    // Start from what the owner already carries. A fresh object would drop the stored block
    // geometry and the anchors with it: changing the cartouche template, or ticking "active",
    // would silently send every block back to its default placement.
    OInfoBlockData data = det.GetInfoBlockData();
    data.SetSet(true);
    data.SetActive(m_activeCheck->isChecked());
    data.SetTemplate(m_templateCombo->currentData().toString());
    data.SetDispatch(ReadDispatch());
    return data;
}

//---------------------------------------------------------------------------------------------------------------------
void OBlockConfigWidget::Commit()
{
    if (m_loading || m_modifyPending == nullptr)
    {
        return;
    }
    m_modifyPending([this](IPieceInfo &det) { det.SetInfoBlockData(BuildData(det)); });
}
