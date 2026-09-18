/************************************************************************
 **
 **  @file   otemplatelanguagerow.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The display languages of a template, and the line that edits them.
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

#include "otemplatelanguagerow.h"

#include "vmisc/FactoryWidgets.h"
#include "vmisc/def.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OTemplateLanguageRow::OTemplateLanguageRow(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_label = new QLabel(this);
    m_hint = new QLabel(this);
    m_hint->setObjectName(QStringLiteral("templateLanguageHint"));

    auto *top = new QHBoxLayout();
    top->setContentsMargins(0, 0, 0, 0);
    top->setSpacing(6);
    top->addWidget(m_label);
    top->addWidget(m_hint);
    top->addStretch(1);
    layout->addLayout(top);

    m_combosLayout = new QHBoxLayout();
    m_combosLayout->setContentsMargins(0, 0, 0, 0);
    m_combosLayout->setSpacing(4);
    layout->addLayout(m_combosLayout);

    // A hairline under the row, so that the languages read as belonging to the template and not
    // to whatever is below them.
    auto *separatorBar = new QHBoxLayout();
    separatorBar->setContentsMargins(0, 0, 0, 0);
    separatorBar->addStretch(1);
    auto *separator = new QFrame(this);
    separator->setObjectName(QStringLiteral("templateLanguageSeparator"));
    separator->setFixedHeight(1);
    separatorBar->addWidget(separator, 8); // 80 % of the row: 8 parts against 1 on each side
    separatorBar->addStretch(1);
    layout->addLayout(separatorBar);

    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::SetLanguages(const QStringList &codes)
{
    m_languages = codes;
    // A page written in no language is not a template, and the row would have nothing to show but
    // its add button — which is a state the user could not get out of by choosing.
    if (m_languages.isEmpty())
    {
        m_languages = {DefaultUiLanguage()};
    }
    Rebuild();
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateLanguageRow::Languages() const -> QStringList
{
    return m_languages;
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::SetLabels(const QString &label, const QString &hint)
{
    m_label->setText(label);
    m_hint->setText(hint);
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::RetranslateUI()
{
    if (m_addBtn != nullptr)
    {
        m_addBtn->setText(tr("Add language"));
    }

    // Rebuilt rather than re-worded: the names inside the combos are translated too, and the trash
    // tooltips are built with the buttons.
    Rebuild();
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::Rebuild()
{
    // Rebuilt rather than adjusted: removing a language shifts every one after it and adding one
    // appends, which together make « change what is there » the longer way round — and the row is
    // a handful of widgets.
    m_updating = true;

    while (QLayoutItem *item = m_combosLayout->takeAt(0))
    {
        // Deferred: a change of language is reported from inside a combo, and that combo is one of
        // the widgets being dropped here. Deleting it now would be destroying the sender of the
        // signal still being delivered.
        if (QWidget *widget = item->widget())
        {
            widget->deleteLater();
        }
        delete item;
    }
    m_combos.clear();
    m_addBtn = nullptr;

    const QStringList available = AvailableUiLanguages();

    for (const QString &code : m_languages)
    {
        auto *combo = MakeComboBox(this);
        AddLanguageItems(combo, available);

        // Connected before the index is set, and silenced until the whole row is built: filling a
        // combo in is not the user choosing a language.
        connect(combo, &QComboBox::currentIndexChanged, this,
                &OTemplateLanguageRow::OnLanguageChanged);

        const int index = combo->findData(code);
        if (index >= 0)
        {
            combo->setCurrentIndex(index);
        }

        // A language already chosen in another combo is disabled here: offering it twice would let
        // the same language appear twice on the page.
        if (auto *model = qobject_cast<QStandardItemModel *>(combo->model()))
        {
            for (int i = 0; i < combo->count(); ++i)
            {
                if (i == index)
                {
                    continue;
                }
                if (m_languages.contains(combo->itemData(i).toString()))
                {
                    QStandardItem *entry = model->item(i);
                    entry->setFlags(entry->flags() & ~Qt::ItemIsEnabled);
                }
            }
        }

        m_combos.append(combo);
        m_combosLayout->addWidget(combo);

        auto *removeButton = new QPushButton(this);
        removeButton->setObjectName(QStringLiteral("templateLanguageTrashBtn"));
        removeButton->setToolTip(tr("Remove language"));
        // One display language always survives, so the last bin has nothing to do.
        removeButton->setEnabled(m_languages.size() > 1);
        const QString removed = code;
        connect(removeButton, &QPushButton::clicked, this,
                [this, removed]() { OnRemoveLanguage(removed); });
        m_combosLayout->addWidget(removeButton);
    }

    m_addBtn = new QPushButton(this);
    m_addBtn->setText(tr("Add language"));
    m_addBtn->setEnabled(m_languages.size() < available.size());
    connect(m_addBtn, &QPushButton::clicked, this, &OTemplateLanguageRow::OnAddLanguage);
    m_combosLayout->addWidget(m_addBtn);

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::OnLanguageChanged()
{
    if (m_updating)
    {
        return;
    }

    auto *combo = qobject_cast<QComboBox *>(sender());
    if (combo == nullptr)
    {
        return;
    }

    const int row = static_cast<int>(m_combos.indexOf(combo));
    if (row < 0 || row >= m_languages.size())
    {
        return;
    }

    const QString code = combo->currentData().toString();
    if (code.isEmpty() || code == m_languages.at(row))
    {
        return;
    }

    m_languages[row] = code;
    emit LanguagesChanged(m_languages);
    Rebuild();
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::OnAddLanguage()
{
    // The first language nobody has taken yet, in the application's own order — which keeps the
    // row predictable rather than putting the newest at the end of the list of languages.
    for (const QString &code : AvailableUiLanguages())
    {
        if (!m_languages.contains(code))
        {
            m_languages.append(code);
            emit LanguagesChanged(m_languages);
            Rebuild();
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateLanguageRow::OnRemoveLanguage(const QString &code)
{
    if (m_languages.size() <= 1)
    {
        return;
    }

    m_languages.removeAll(code);
    emit LanguagesChanged(m_languages);
    Rebuild();
}
