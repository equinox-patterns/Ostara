/************************************************************************
 **
 **  @file   OAxisRenameEditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Per-language display aliases of a size axis and its values (Sizes section).
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

#include "OAxisRenameEditor.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QTabWidget>
#include <QVBoxLayout>

OAxisRenameEditor::OAxisRenameEditor(QWidget *parent) : QWidget(parent)
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    m_root->setSpacing(4);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setWordWrap(true);
    m_root->addWidget(m_titleLabel);

    m_tabs = new QTabWidget(this);
    m_root->addWidget(m_tabs);

    m_oneLineCheck = new QCheckBox(tr("Write each language on its own line"), this);
    connect(m_oneLineCheck, &QCheckBox::toggled, this, [this](bool on) { emit OneLineChanged(on); });
    m_root->addWidget(m_oneLineCheck);
}

void OAxisRenameEditor::SetTitle(const QString &title)
{
    m_title = title;
    m_titleLabel->setText(title);
}

void OAxisRenameEditor::SetData(const QMap<QString, OAxisRename> &renames, bool oneLine,
                                const QStringList &languages, const QString &axisName,
                                const QStringList &axisValues)
{
    m_renames = renames;
    m_oneLine = oneLine;
    m_languages = languages;
    m_axisName = axisName;
    m_axisValues = axisValues;
    Rebuild();
}

void OAxisRenameEditor::Rebuild()
{
    while (m_tabs->count() > 0)
    {
        QWidget *page = m_tabs->widget(0);
        m_tabs->removeTab(0);
        delete page;
    }

    m_oneLineCheck->setChecked(m_oneLine);
    m_oneLineCheck->setEnabled(!m_languages.isEmpty());
    m_tabs->setVisible(!m_languages.isEmpty());

    for (const QString &locale : m_languages)
    {
        const OAxisRename rename = m_renames.value(locale);
        auto *page = new QWidget(m_tabs);
        auto *layout = new QVBoxLayout(page);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        // Axis name — pre-filled with the alias or the original table name.
        auto *nameEdit = new QLineEdit(page);
        nameEdit->setText(rename.name.isEmpty() ? m_axisName : rename.name);
        nameEdit->setPlaceholderText(m_axisName);
        connect(nameEdit, &QLineEdit::textEdited, this, [this, locale](const QString &text) {
            OAxisRename r = m_renames.value(locale);
            r.name = (text == m_axisName) ? QString() : text;
            m_renames.insert(locale, r);
            emit RenamesChanged(m_renames);
        });
        layout->addWidget(nameEdit);

        // One field per axis value.
        for (int i = 0; i < m_axisValues.size(); ++i)
        {
            const QString originalValue = m_axisValues.at(i);
            const QString storedValue = (i < rename.values.size()) ? rename.values.at(i) : QString();
            auto *valueEdit = new QLineEdit(page);
            valueEdit->setText(storedValue.isEmpty() ? originalValue : storedValue);
            valueEdit->setPlaceholderText(originalValue);
            connect(valueEdit, &QLineEdit::textEdited, this, [this, locale, i](const QString &text) {
                OAxisRename r = m_renames.value(locale);
                while (r.values.size() <= i)
                {
                    r.values.append(QString());
                }
                const bool sameAsOriginal = (i < m_axisValues.size()) && (text == m_axisValues.at(i));
                r.values[i] = sameAsOriginal ? QString() : text;
                m_renames.insert(locale, r);
                emit RenamesChanged(m_renames);
            });
            layout->addWidget(valueEdit);
        }

        layout->addStretch();
        m_tabs->addTab(page, QLocale(locale).nativeLanguageName());
    }
}
