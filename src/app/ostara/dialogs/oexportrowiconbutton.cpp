/************************************************************************
 **
 **  @file   oexportrowiconbutton.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation du bouton-icône de ligne.
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

#include "oexportrowiconbutton.h"

#include "oexportrowlistwidget.h"

#include "../vtheme/vtheme.h"

#include <QEnterEvent>
#include <QEvent>
#include <QIcon>

namespace
{

/// Le chemin d'une variante : `:/icons/<thème>/<nom>_<thème>.svg`.
auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportRowIconButton::OExportRowIconButton(const QString &baseName, QWidget *parent)
  : QPushButton(parent),
    m_baseName(baseName)
{
    // Jamais le bouton par défaut du dialogue : `:default` porte le MÊME fond accent que `:hover`,
    // donc une icône clair sur fond sombre — le défaut reproduirait le défaut qu'on corrige.
    setAutoDefault(false);
    setFixedWidth(kExportRowButtonWidth);

    // Un changement de thème ne passe par aucun de nos événements : on le suit à la source.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &OExportRowIconButton::Refresh);

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowIconButton::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    Refresh();
    QPushButton::enterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowIconButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    Refresh();
    QPushButton::leaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowIconButton::changeEvent(QEvent *event)
{
    // Un bouton désactivé pendant qu'il est survolé ne reçoit pas de leaveEvent : il garderait
    // sinon l'icône du survol sous un fond qui n'est plus celui du survol.
    if (event->type() == QEvent::EnabledChange)
    {
        Refresh();
    }

    QPushButton::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowIconButton::Refresh()
{
    const bool darkTheme = (VTheme::ColorSheme() == VColorSheme::Dark);
    const bool darkIcon = (m_hovered && isEnabled()) || darkTheme;

    setIcon(QIcon(IconResPath(m_baseName, darkIcon)));
}
