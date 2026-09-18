/************************************************************************
 **
 **  @file   ocoverpagebandeditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  One band of a cover page, as the editor shows it and acts on it.
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

#include "ocoverpagebandeditor.h"

#include "core/ocoverpagecatalogue.h"

#include <QAction>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QPushButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OCoverPageBandEditor::OCoverPageBandEditor(OCoverPageTemplate::Placement placement, QWidget *parent)
  : QWidget(parent), m_placement(placement)
{
    auto *bandLayout = new QVBoxLayout(this);
    bandLayout->setContentsMargins(0, 0, 0, 0);
    bandLayout->setSpacing(2);

    // The band's name and the caret that folds it away, on one row above the frame. Folded, the
    // four bands are four names — which is what a template is judged on; unfolded, one of them
    // shows the list it holds. The caret comes before the name because what it opens is what lies
    // underneath.
    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    m_toggleBtn = new QPushButton(header);
    m_toggleBtn->setObjectName(QStringLiteral("coverPageBandToggle"));
    m_toggleBtn->setFlat(true);
    m_toggleBtn->setCheckable(true);
    m_toggleBtn->setCursor(Qt::ArrowCursor);
    // The caret is drawn by the stylesheet, from the button's checked state — no icon is set here,
    // and none is needed for the caret to follow the theme.
    headerLayout->addWidget(m_toggleBtn);

    m_nameLabel = new QLabel(header);
    m_nameLabel->setObjectName(QStringLiteral("coverPageBandName"));
    headerLayout->addWidget(m_nameLabel);
    headerLayout->addStretch(1);

    bandLayout->addWidget(header);

    // The frame carries the contour and nothing else: its title is left empty, the band being
    // named above it, on the row that folds it.
    m_box = new QGroupBox(this);
    auto *boxLayout = new QVBoxLayout(m_box);
    boxLayout->setSpacing(2);
    bandLayout->addWidget(m_box);

    connect(m_toggleBtn, &QPushButton::toggled, this, [this](bool expanded) { m_box->setVisible(expanded); });
    m_box->setVisible(false);

    // Every band carries the whole line: its own add button with its own menu, its own arrows and
    // its own bin. Nothing is shared, so nothing has to ask which band it is acting on.
    auto *bar = new QHBoxLayout();
    bar->setContentsMargins(0, 0, 0, 0);
    bar->setSpacing(4);

    m_addBtn = new QPushButton(m_box);
    m_addMenu = new QMenu(m_addBtn);
    m_addBtn->setMenu(m_addMenu);
    bar->addWidget(m_addBtn);

    // The three below draw their icons from the stylesheet, by object name — see the rules for
    // coverPageBand* in the theme. None of them is given a setIcon(), and none of them needs one.
    m_upBtn = new QPushButton(m_box);
    m_upBtn->setObjectName(QStringLiteral("coverPageBandUpBtn"));
    connect(m_upBtn, &QPushButton::clicked, this, &OCoverPageBandEditor::MoveUpRequested);
    bar->addWidget(m_upBtn);

    m_downBtn = new QPushButton(m_box);
    m_downBtn->setObjectName(QStringLiteral("coverPageBandDownBtn"));
    connect(m_downBtn, &QPushButton::clicked, this, &OCoverPageBandEditor::MoveDownRequested);
    bar->addWidget(m_downBtn);

    m_removeBtn = new QPushButton(m_box);
    m_removeBtn->setObjectName(QStringLiteral("coverPageBandTrashBtn"));
    // A bin that would remove nothing is drawn greyed out, which is what the stylesheet's
    // :disabled rule is for.
    m_removeBtn->setEnabled(false);
    connect(m_removeBtn, &QPushButton::clicked, this, &OCoverPageBandEditor::RemoveRequested);
    bar->addWidget(m_removeBtn);

    bar->addStretch(1);
    boxLayout->addLayout(bar);

    // The list holds the band's informations in the order they are laid out, and it is the only
    // thing that decides that order — nothing re-sorts it, so a row's index is the rank the
    // information has within its band.
    m_list = new QListWidget(m_box);
    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!m_updatingSelection)
        {
            emit RowSelected(row);
        }
    });
    boxLayout->addWidget(m_list);

    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandEditor::RetranslateUI()
{
    m_addBtn->setText(tr("+ Add information"));
    m_upBtn->setToolTip(tr("Move up"));
    m_downBtn->setToolTip(tr("Move down"));
    m_removeBtn->setToolTip(tr("Remove information"));
    m_nameLabel->setText(Name());

    // The catalogue resolves a label when it builds the entry and keeps it, so a language change
    // reaches this menu only through a rebuild — the editor drops the cache before calling here.
    RebuildAddMenu();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandEditor::RebuildAddMenu()
{
    m_addMenu->clear();
    for (const OTemplateSection &section : OCoverPageCatalogue::Sections())
    {
        QAction *action = m_addMenu->addAction(section.DisplayName());
        action->setToolTip(section.Description());
        connect(action, &QAction::triggered, this, [this, id = section.Id()]() { emit AddRequested(id); });
    }
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandEditor::Name() const -> QString
{
    switch (m_placement)
    {
        case OCoverPageTemplate::Placement::Top:
            return tr("Top");
        case OCoverPageTemplate::Placement::MiddleLeft:
            return tr("Middle left");
        case OCoverPageTemplate::Placement::MiddleRight:
            return tr("Middle right");
        case OCoverPageTemplate::Placement::Bottom:
            return tr("Bottom");
    }
    return QString();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandEditor::SetInformations(const QStringList &typeIds)
{
    m_list->clear();
    for (const QString &typeId : typeIds)
    {
        const OTemplateSection &type = OCoverPageCatalogue::Section(typeId);
        m_list->addItem(type.IsValid() ? type.DisplayName() : typeId);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandEditor::SetCurrentRow(int row)
{
    m_updatingSelection = true;
    m_list->setCurrentRow(row);
    m_updatingSelection = false;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandEditor::CurrentRow() const -> int
{
    return m_list->currentRow();
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandEditor::Count() const -> int
{
    return m_list->count();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandEditor::SetRemoveEnabled(bool enabled)
{
    m_removeBtn->setEnabled(enabled);
}
