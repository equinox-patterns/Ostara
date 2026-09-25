/************************************************************************
 **
 **  @file   opatternpropertieswidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Pattern work fields (pattern name, description, notes) shown in
 **          the Draw property block when no tool is selected.
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

#include "opatternpropertieswidget.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractvalapplication.h"

#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTimer>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPatternPropertiesWidget::OPatternPropertiesWidget(QWidget *parent)
  : QWidget(parent)
{
    m_patternNameEdit = new QLineEdit(this);
    m_descriptionEdit = new QPlainTextEdit(this);
    m_notesEdit = new QPlainTextEdit(this);

    auto *groupBox = new QGroupBox(tr("Pattern"), this);
    m_groupBox = groupBox;
    auto *groupLayout = new QVBoxLayout(groupBox);
    groupLayout->setSpacing(6);

    m_nameLabel = new QLabel(tr("Pattern name:"), groupBox);
    groupLayout->addWidget(m_nameLabel);
    groupLayout->addWidget(m_patternNameEdit);

    m_descriptionLabel = new QLabel(tr("Description:"), groupBox);
    groupLayout->addWidget(m_descriptionLabel);
    groupLayout->addWidget(m_descriptionEdit);

    m_notesLabel = new QLabel(tr("Notes:"), groupBox);
    groupLayout->addWidget(m_notesLabel);
    groupLayout->addWidget(m_notesEdit);

    groupLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(groupBox);

    m_commitTimer = new QTimer(this);
    m_commitTimer->setSingleShot(true);
    m_commitTimer->setInterval(600);

    connect(m_patternNameEdit, &QLineEdit::editingFinished, this, &OPatternPropertiesWidget::CommitPatternName);
    connect(m_descriptionEdit, &QPlainTextEdit::textChanged, this, &OPatternPropertiesWidget::ScheduleCommit);
    connect(m_notesEdit, &QPlainTextEdit::textChanged, this, &OPatternPropertiesWidget::ScheduleCommit);
    connect(m_commitTimer, &QTimer::timeout, this, &OPatternPropertiesWidget::CommitDelayed);
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternPropertiesWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternPropertiesWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUI();
    }
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Re-apply the four labels from the translator installed right now.
 *
 * Called by `changeEvent()` and, explicitly, from `VToolOptionsPropertyBrowser::RetranslateUI()` —
 * that browser's own retranslation goes through `RefreshOptions()`, which returns at once when no
 * tool is selected, which is precisely the state this page is shown in.
 */
void OPatternPropertiesWidget::RetranslateUI()
{
    m_groupBox->setTitle(tr("Pattern"));
    m_nameLabel->setText(tr("Pattern name:"));
    m_descriptionLabel->setText(tr("Description:"));
    m_notesLabel->setText(tr("Notes:"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternPropertiesWidget::CurrentDocument() const -> VAbstractPattern *
{
    auto *app = VAbstractValApplication::VApp();
    return app != nullptr ? app->getCurrentDocument() : nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternPropertiesWidget::Refresh()
{
    QPointer<VAbstractPattern> doc(CurrentDocument());
    if (doc != m_doc)
    {
        if (m_doc)
        {
            disconnect(m_doc, nullptr, this, nullptr);
        }
        m_doc = doc;
        if (m_doc)
        {
            connect(m_doc, &VAbstractPattern::FullUpdateFromFile, this, &OPatternPropertiesWidget::Refresh);
        }
    }
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternPropertiesWidget::Populate()
{
    m_loading = true;
    const VAbstractPattern *doc = CurrentDocument();
    m_patternNameEdit->setText(doc != nullptr ? doc->GetPatternName() : QString());
    m_descriptionEdit->setPlainText(doc != nullptr ? doc->GetDescription() : QString());
    m_notesEdit->setPlainText(doc != nullptr ? doc->GetNotes() : QString());
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternPropertiesWidget::CommitPatternName()
{
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr)
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
void OPatternPropertiesWidget::ScheduleCommit()
{
    if (m_loading)
    {
        return;
    }
    m_commitTimer->start();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternPropertiesWidget::CommitDelayed()
{
    m_commitTimer->stop();
    if (m_loading)
    {
        return;
    }
    VAbstractPattern *doc = CurrentDocument();
    if (doc == nullptr)
    {
        return;
    }
    const QString description = m_descriptionEdit->toPlainText();
    if (description != doc->GetDescription())
    {
        doc->SetDescription(description);
    }
    const QString notes = m_notesEdit->toPlainText();
    if (notes != doc->GetNotes())
    {
        doc->SetNotes(notes);
    }
}
