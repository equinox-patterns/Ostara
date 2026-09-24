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

#include "oexportrowiconbutton.h"
#include "oexportrowlistwidget.h" // kExportRowStateWidth : la colonne d'état des deux sortes de lignes

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

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
    m_stateLabel->setMinimumWidth(kExportRowStateWidth); // the same column as the favorites lists
    layout->addWidget(m_stateLabel);

    // Le bouton porte lui-même la règle des icônes de ligne : variante du thème au repos, `_dark`
    // au survol — le fond de survol est l'accent, donc sombre dans les deux thèmes.
    m_deleteButton = new OExportRowIconButton(QStringLiteral("trash"), this);
    m_deleteButton->setToolTip(tr("Remove this run from the list"));
    layout->addWidget(m_deleteButton);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationRowWidget::SetState(OExportRunState state)
{
    m_state = state;
    m_stateLabel->setText(OExportRunStateText(m_state));
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGenerationRowWidget::DeleteButton() const -> QPushButton *
{
    return m_deleteButton;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationRowWidget::Retranslate()
{
    m_stateLabel->setText(OExportRunStateText(m_state));
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
