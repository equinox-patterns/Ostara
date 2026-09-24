/************************************************************************
 **
 **  @file   oexportfavoriterowwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of one line of a favorites list.
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

#include "oexportfavoriterowwidget.h"

#include "oexportrowiconbutton.h"
#include "oexportrowlistwidget.h"

#include <QCheckBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>

//---------------------------------------------------------------------------------------------------------------------
OExportFavoriteRowWidget::OExportFavoriteRowWidget(const QString &name, QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_check = new QCheckBox(this);
    // Ticked, and it is the whole point: a favorite was kept to be run, so running it is what
    // the batch does unless the user says otherwise.
    m_check->setChecked(true);
    layout->addWidget(m_check);

    m_nameLabel = new QLabel(name, this);
    layout->addWidget(m_nameLabel, 1);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setMinimumWidth(kExportRowStateWidth); // the same column as the Working list
    layout->addWidget(m_stateLabel);

    // Le bouton porte lui-même la règle des icônes de ligne : variante du thème au repos, `_dark`
    // au survol — le fond de survol est l'accent, donc sombre dans les deux thèmes.
    m_starButton = new OExportRowIconButton(QStringLiteral("star-fill"), this);
    layout->addWidget(m_starButton);

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoriteRowWidget::Name() const -> QString
{
    return m_nameLabel->text();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoriteRowWidget::SetState(OExportRunState state)
{
    m_state = state;
    m_stateLabel->setText(OExportRunStateText(m_state));
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoriteRowWidget::CheckBox() const -> QCheckBox *
{
    return m_check;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoriteRowWidget::StarButton() const -> QPushButton *
{
    return m_starButton;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoriteRowWidget::Retranslate()
{
    m_stateLabel->setText(OExportRunStateText(m_state));
    m_starButton->setToolTip(tr("Remove from favorites"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoriteRowWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
