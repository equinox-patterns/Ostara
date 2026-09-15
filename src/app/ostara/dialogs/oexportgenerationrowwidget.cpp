/************************************************************************
 **
 **  @file   oexportgenerationrowwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of one row of the list to generate.
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

#include "oexportgenerationrowwidget.h"

#include "../vtheme/vtheme.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>

namespace
{
/// Wide enough for the longest of the four states, so that the names beside them stay in
/// one column instead of stepping in and out as the batch walks the list.
constexpr int kStateColumnWidth = 70;

constexpr int kDeleteButtonWidth = 30;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportGenerationRowWidget::OExportGenerationRowWidget(const QString &name, QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_nameLabel = new QLabel(name, this);
    layout->addWidget(m_nameLabel, 1);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setMinimumWidth(kStateColumnWidth);
    layout->addWidget(m_stateLabel);

    m_deleteButton = new QPushButton(this);
    // An icon-only button must never become the dialog's default button (it would
    // take the :default style — same look as :hover in the theme).
    m_deleteButton->setAutoDefault(false);
    m_deleteButton->setFixedWidth(kDeleteButtonWidth);
    m_deleteButton->setToolTip(tr("Remove this run from the list"));
    RefreshDeleteIcon();
    layout->addWidget(m_deleteButton);

    // Reload the themed trash icon when the theme changes (Light/Dark toggle).
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &OExportGenerationRowWidget::RefreshDeleteIcon);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGenerationRowWidget::StateText(OExportRunState state) -> QString
{
    switch (state)
    {
        case OExportRunState::Running:
            return tr("Running");
        case OExportRunState::Done:
            return tr("Done");
        case OExportRunState::Failed:
            return tr("Failed");
        case OExportRunState::Pending:
        default:
            return tr("Waiting");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationRowWidget::SetState(OExportRunState state)
{
    m_state = state;
    m_stateLabel->setText(StateText(m_state));
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGenerationRowWidget::DeleteButton() const -> QPushButton *
{
    return m_deleteButton;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationRowWidget::RefreshDeleteIcon()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_deleteButton->setIcon(QIcon(QStringLiteral(":/icons/%1/trash_%1.svg")
                                      .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"))));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationRowWidget::Retranslate()
{
    m_stateLabel->setText(StateText(m_state));
    m_deleteButton->setToolTip(tr("Remove this run from the list"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationRowWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
