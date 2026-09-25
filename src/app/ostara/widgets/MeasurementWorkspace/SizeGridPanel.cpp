/************************************************************************
 **
 **  @file   SizeGridPanel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Step 1 of measurement creation — axis group configuration.
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

#include "SizeGridPanel.h"

#include "core/otranslation.h"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QSet>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

// ===== Constructor =====

SizeGridPanel::SizeGridPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Title — centered
    auto *titleLabel = new QLabel();
    OTranslation::SetText(this, titleLabel, QT_TRANSLATE_NOOP("SizeGridPanel", "Configure size groups"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Hint — centered
    auto *hintLabel = new QLabel();
    OTranslation::SetText(this, hintLabel,
                          QT_TRANSLATE_NOOP("SizeGridPanel",
                                            "Each group represents an axis (e.g., Height, Size…). "
                                            "Name it and enter its values to activate it."));
    hintLabel->setWordWrap(true);
    hintLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(hintLabel);

    // Table name — from the dialog, editable here
    auto *nameRow = new QHBoxLayout();
    nameRow->addStretch();
    auto *tableNameLabel = new QLabel();
    OTranslation::SetText(this, tableNameLabel, QT_TRANSLATE_NOOP("SizeGridPanel", "Name:"));
    nameRow->addWidget(tableNameLabel);
    m_tableNameEdit = new QLineEdit();
    OTranslation::SetPlaceholderText(this, m_tableNameEdit, QT_TRANSLATE_NOOP("SizeGridPanel", "Table name"));
    m_tableNameEdit->setMaximumWidth(380);
    nameRow->addWidget(m_tableNameEdit);
    nameRow->addStretch();
    mainLayout->addLayout(nameRow);

    // Three fixed columns — centered, max width to avoid stretching in fullscreen
    auto *columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(16);
    columnsLayout->addStretch();
    // ⚠️ **Le marquage est au site d'appel, et pas seulement dans `SetupColumn`.** `lupdate`
    // n'extrait pas un argument de fonction : le `QT_TRANSLATE_NOOP` posé sur le paramètre `source`
    // porte sur une variable et ne voit rien. Les trois titres seraient passés en `vanished`.
    SetupColumn(0, QT_TRANSLATE_NOOP("SizeGridPanel", "Group 1"));
    SetupColumn(1, QT_TRANSLATE_NOOP("SizeGridPanel", "Group 2"));
    SetupColumn(2, QT_TRANSLATE_NOOP("SizeGridPanel", "Group 3"));
    for (int i = 0; i < NumGroups; ++i)
    {
        m_columns[i].box->setMaximumWidth(380);
        columnsLayout->addWidget(m_columns[i].box);
    }
    columnsLayout->addStretch();
    mainLayout->addLayout(columnsLayout, 1);

    // Validation label
    m_validationLabel = new QLabel();
    m_validationLabel->setObjectName(QStringLiteral("validationLabel"));
    mainLayout->addWidget(m_validationLabel);
}

void SizeGridPanel::SetupColumn(int index, const char *source)
{
    GroupColumn &col = m_columns[index];

    col.box = new QGroupBox();
    OTranslation::SetTitle(this, col.box, source);
    auto *form = new QFormLayout(col.box);
    form->setContentsMargins(16, 12, 16, 16);
    form->setSpacing(8);

    // Name — domain-specific placeholder per group
    col.nameEdit = new QLineEdit();
    switch (index)
    {
    case 0: OTranslation::SetPlaceholderText(this, col.nameEdit, QT_TRANSLATE_NOOP("SizeGridPanel", "e.g., Height")); break;
    case 1: OTranslation::SetPlaceholderText(this, col.nameEdit, QT_TRANSLATE_NOOP("SizeGridPanel", "e.g., Commercial size")); break;
    case 2: OTranslation::SetPlaceholderText(this, col.nameEdit, QT_TRANSLATE_NOOP("SizeGridPanel", "e.g., Cup or Body type")); break;
    }
    connect(col.nameEdit, &QLineEdit::textChanged, this, &SizeGridPanel::OnGroupDataChanged);
    auto *colNameLabel = new QLabel();
    OTranslation::SetText(this, colNameLabel, QT_TRANSLATE_NOOP("SizeGridPanel", "Name:"));
    form->addRow(colNameLabel, col.nameEdit);

    // Values — domain-specific placeholder per group
    col.valuesEdit = new QLineEdit();
    switch (index)
    {
    case 0: OTranslation::SetPlaceholderText(this, col.valuesEdit, QT_TRANSLATE_NOOP("SizeGridPanel", "e.g., 160 cm, 168 cm, 176 cm")); break;
    case 1:
        OTranslation::SetPlaceholderText(
            this, col.valuesEdit,
            QT_TRANSLATE_NOOP("SizeGridPanel", "e.g., 32, 34, 36  or  XS, S, M  or  6 years, 8 years, 10 years"));
        break;
    case 2: OTranslation::SetPlaceholderText(this, col.valuesEdit, QT_TRANSLATE_NOOP("SizeGridPanel", "e.g., A, B, C")); break;
    }
    connect(col.valuesEdit, &QLineEdit::textChanged, this, &SizeGridPanel::OnGroupDataChanged);
    auto *colValuesLabel = new QLabel();
    OTranslation::SetText(this, colValuesLabel, QT_TRANSLATE_NOOP("SizeGridPanel", "Values:"));
    form->addRow(colValuesLabel, col.valuesEdit);

    // Hint below values field
    {
        auto *hint = new QLabel();
        OTranslation::SetText(this, hint, QT_TRANSLATE_NOOP("SizeGridPanel", "Separate each value with a comma"));
        hint->setObjectName(QStringLiteral("fieldHint"));
        form->addRow(hint);
    }

    // Generate button
    col.generateBtn = new QPushButton();
    OTranslation::SetText(this, col.generateBtn, QT_TRANSLATE_NOOP("SizeGridPanel", "Generate"));
    connect(col.generateBtn, &QPushButton::clicked, this, [this, index]() {
        OnGenerateClicked(index);
    });
    form->addRow(col.generateBtn);

    // Spacer between Generate and Base
    {
        auto *spacer = new QWidget();
        spacer->setFixedHeight(6);
        form->addRow(spacer);
    }

    // Base dropdown
    col.baseLabel = new QLabel();
    OTranslation::SetText(this, col.baseLabel, QT_TRANSLATE_NOOP("SizeGridPanel", "Base:"));
    col.baseCombo = new QComboBox();
    col.baseCombo->setView(new QListView()); // bypass native macOS popup that ignores QSS
    col.baseCombo->setEnabled(false); // empty until values are generated
    connect(col.baseCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SizeGridPanel::OnGroupDataChanged);

    auto *baseRow = new QHBoxLayout();
    baseRow->addWidget(col.baseLabel);
    baseRow->addWidget(col.baseCombo, 1);
    form->addRow(baseRow);

    // Hint below Base dropdown
    {
        auto *hint = new QLabel();
        OTranslation::SetText(this, hint, QT_TRANSLATE_NOOP("SizeGridPanel", "Reference size for calculating other sizes"));
        hint->setObjectName(QStringLiteral("fieldHint"));
        form->addRow(hint);
    }

    // Feedback area — shown after "Générer", hidden by default
    // Separator at 80% width
    {
        auto *sepWidget = new QWidget();
        auto *sepLayout = new QHBoxLayout(sepWidget);
        sepLayout->setContentsMargins(0, 0, 0, 0);
        sepLayout->addStretch(1); // 10% left
        col.feedbackSeparator = new QFrame();
        col.feedbackSeparator->setFrameShape(QFrame::HLine);
        col.feedbackSeparator->setFrameShadow(QFrame::Sunken);
        sepLayout->addWidget(col.feedbackSeparator, 8); // 80%
        sepLayout->addStretch(1); // 10% right
        form->addRow(sepWidget);
    }

    col.feedbackSummary = new QLabel();
    col.feedbackSummary->setWordWrap(true);
    col.feedbackSummary->setVisible(false);
    form->addRow(col.feedbackSummary);

    col.feedbackValues = new QLabel();
    col.feedbackValues->setWordWrap(true);
    col.feedbackValues->setVisible(false);
    form->addRow(col.feedbackValues);
}

// ===== Slots =====

void SizeGridPanel::OnGenerateClicked(int index)
{
    if (index < 0 || index >= NumGroups)
    {
        return;
    }

    GroupColumn &col = m_columns[index];
    QStringList vals = ParseValues(col.valuesEdit->text());

    col.baseCombo->clear();
    if (vals.isEmpty())
    {
        col.baseCombo->setEnabled(false);
        col.valuesGenerated = false;
        col.feedbackSeparator->setVisible(false);
        col.feedbackSummary->setVisible(false);
        col.feedbackValues->setVisible(false);
    }
    else
    {
        col.baseCombo->addItems(vals);
        col.baseCombo->setEnabled(true);
        col.valuesGenerated = true;

        // Show feedback
        QString name = col.nameEdit->text().trimmed();
        if (name.isEmpty())
        {
            name = tr("Group %1").arg(index + 1);
        }
        col.feedbackSummary->setText(tr("Group \"%1\" · %2 values").arg(name).arg(vals.size()));
        col.feedbackValues->setText(vals.join(QStringLiteral("\n")));
        col.feedbackSeparator->setVisible(true);
        col.feedbackSummary->setVisible(true);
        col.feedbackValues->setVisible(true);
    }

    emit GroupsChanged();
}

void SizeGridPanel::OnGroupDataChanged()
{
    emit GroupsChanged();
}

// ===== Public API =====

void SizeGridPanel::ClearAll()
{
    for (int i = 0; i < NumGroups; ++i)
    {
        m_columns[i].nameEdit->blockSignals(true);
        m_columns[i].valuesEdit->blockSignals(true);
        m_columns[i].baseCombo->blockSignals(true);

        m_columns[i].nameEdit->clear();
        m_columns[i].valuesEdit->clear();
        m_columns[i].baseCombo->clear();
        m_columns[i].baseCombo->setEnabled(false);
        m_columns[i].valuesGenerated = false;

        m_columns[i].feedbackSeparator->setVisible(false);
        m_columns[i].feedbackSummary->setVisible(false);
        m_columns[i].feedbackValues->setVisible(false);

        m_columns[i].nameEdit->blockSignals(false);
        m_columns[i].valuesEdit->blockSignals(false);
        m_columns[i].baseCombo->blockSignals(false);
    }
}

void SizeGridPanel::LoadFromDocument(const QStringList &groupNames,
                                      const QList<QStringList> &groupValues,
                                      const QMap<QString, QString> &anchorBindings)
{
    ClearAll();

    for (int i = 0; i < groupNames.size() && i < NumGroups; ++i)
    {
        GroupColumn &col = m_columns[i];
        col.nameEdit->setText(groupNames.at(i));
        col.valuesEdit->setText(groupValues.at(i).join(QLatin1String(", ")));

        // Generate values into base combo
        col.baseCombo->clear();
        col.baseCombo->addItems(groupValues.at(i));
        col.baseCombo->setEnabled(true);
        col.valuesGenerated = true;

        // Show feedback
        col.feedbackSummary->setText(tr("Group \"%1\" · %2 values")
                                        .arg(groupNames.at(i))
                                        .arg(groupValues.at(i).size()));
        col.feedbackValues->setText(groupValues.at(i).join(QStringLiteral("\n")));
        col.feedbackSeparator->setVisible(true);
        col.feedbackSummary->setVisible(true);
        col.feedbackValues->setVisible(true);

        // Set anchor value if present
        if (anchorBindings.contains(groupNames.at(i)))
        {
            int idx = col.baseCombo->findText(anchorBindings.value(groupNames.at(i)));
            if (idx >= 0)
            {
                col.baseCombo->setCurrentIndex(idx);
            }
        }
    }
}

auto SizeGridPanel::GetActiveGroupNames() const -> QStringList
{
    QStringList names;
    for (int i = 0; i < NumGroups; ++i)
    {
        QString name = m_columns[i].nameEdit->text().trimmed();
        if (!name.isEmpty())
        {
            names.append(name);
        }
    }
    return names;
}

auto SizeGridPanel::GetGroupValues(const QString &groupName) const -> QStringList
{
    const GroupColumn *col = FindColumnByName(groupName);
    if (!col)
    {
        return {};
    }
    return ParseValues(col->valuesEdit->text());
}

auto SizeGridPanel::GetBaseBinding(const QString &groupName) const -> QString
{
    const GroupColumn *col = FindColumnByName(groupName);
    if (!col)
    {
        return {};
    }
    return col->baseCombo->currentText();
}

auto SizeGridPanel::IsValid() -> bool
{
    QStringList names = GetActiveGroupNames();

    // At least one group with a name
    if (names.isEmpty())
    {
        m_validationLabel->setText(tr("Name at least one group and generate its values."));
        return false;
    }

    // Validate each group that has a name
    for (int i = 0; i < NumGroups; ++i)
    {
        QString name = m_columns[i].nameEdit->text().trimmed();
        if (name.isEmpty())
        {
            continue; // empty column, skip
        }

        // At least 2 values
        QStringList vals = ParseValues(m_columns[i].valuesEdit->text());
        if (vals.size() < 2)
        {
            m_validationLabel->setText(tr("Group \"%1\" must have at least 2 values.")
                                          .arg(name));
            return false;
        }

        // Base dropdown must have a selection
        if (m_columns[i].baseCombo->count() == 0 || m_columns[i].baseCombo->currentText().isEmpty())
        {
            m_validationLabel->setText(tr("Generate values and select a base for group \"%1\".")
                                          .arg(name));
            return false;
        }
    }

    // Check for duplicate names
    QSet<QString> seen;
    for (const QString &n : names)
    {
        if (seen.contains(n))
        {
            m_validationLabel->setText(tr("Group name \"%1\" is used multiple times.").arg(n));
            return false;
        }
        seen.insert(n);
    }

    m_validationLabel->clear();
    return true;
}

// ===== Private helpers =====

auto SizeGridPanel::FindColumnByName(const QString &name) -> GroupColumn *
{
    for (int i = 0; i < NumGroups; ++i)
    {
        if (m_columns[i].nameEdit->text().trimmed() == name)
        {
            return &m_columns[i];
        }
    }
    return nullptr;
}

auto SizeGridPanel::FindColumnByName(const QString &name) const -> const GroupColumn *
{
    for (int i = 0; i < NumGroups; ++i)
    {
        if (m_columns[i].nameEdit->text().trimmed() == name)
        {
            return &m_columns[i];
        }
    }
    return nullptr;
}

auto SizeGridPanel::ParseValues(const QString &text) -> QStringList
{
    QStringList result;
    const QStringList parts = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts)
    {
        QString trimmed = part.trimmed();
        if (!trimmed.isEmpty())
        {
            result.append(trimmed);
        }
    }
    return result;
}

auto SizeGridPanel::GetTableName() const -> QString
{
    return m_tableNameEdit ? m_tableNameEdit->text().trimmed() : QString();
}

void SizeGridPanel::SetTableName(const QString &name)
{
    if (m_tableNameEdit)
    {
        m_tableNameEdit->setText(name);
    }
}
