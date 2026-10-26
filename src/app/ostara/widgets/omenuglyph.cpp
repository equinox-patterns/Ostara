/************************************************************************
 **
 **  @file   omenuglyph.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Les glyphes d'un menu : l'icône du thème au repos, sa variante
 **          claire au survol.
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

#include "omenuglyph.h"

#include "vtheme.h"

#include <QAction>
#include <QHash>
#include <QMenu>
#include <QVariant>

#include <memory>

namespace
{
/// Le nom de base de l'icône d'une action de menu standard, d'après le nom que Qt lui a donné.
/// Une chaîne vide quand l'action n'est pas des nôtres — et alors on n'y touche pas.
auto GlyphForObjectName(const QString &objectName) -> QString
{
    // Qt nomme ces actions dans qlineedit.cpp et qwidgettextcontrol.cpp. Toutes portent le préfixe
    // `edit-` sauf « Select All », nommée `select-all` alors que l'icône qu'elle demande est
    // `edit-select-all` : les deux formes sont dans la table, à leur place.
    static const QHash<QString, QString> glyphs{
        {QStringLiteral("edit-undo"), QStringLiteral("arrow-bend-up-left")},
        {QStringLiteral("edit-redo"), QStringLiteral("arrow-bend-up-right")},
        {QStringLiteral("edit-cut"), QStringLiteral("scissors")},
        {QStringLiteral("edit-copy"), QStringLiteral("copy")},
        {QStringLiteral("edit-paste"), QStringLiteral("copy-fill")},
        {QStringLiteral("edit-delete"), QStringLiteral("trash")},
        {QStringLiteral("select-all"), QStringLiteral("selection-all")},
    };

    return glyphs.value(objectName);
}

/// L'entrée survolée, et celle qu'il faut remettre au repos quand le pointeur la quitte.
struct HoverState
{
    QAction *lastHovered{nullptr};
};

void ApplyHover(const std::shared_ptr<HoverState> &state, QAction *action)
{
    if (action == state->lastHovered)
    {
        return;
    }

    if (state->lastHovered != nullptr)
    {
        const QVariant rest = state->lastHovered->property("restIcon");
        if (rest.isValid())
        {
            state->lastHovered->setIcon(rest.value<QIcon>());
        }
    }

    state->lastHovered = action;

    if (action != nullptr)
    {
        const QVariant hover = action->property("hoverIcon");
        if (hover.isValid())
        {
            action->setIcon(hover.value<QIcon>());
        }
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OMenuGlyph::SetGlyph(QAction *action, const QString &baseName)
{
    if (action == nullptr || baseName.isEmpty())
    {
        return;
    }

    const QIcon rest = VTheme::ThemedIcon(baseName);
    // La variante du survol est toujours celle du jeu sombre, quel que soit le thème : la ligne
    // survolée a un fond accent dans les deux. En thème sombre les deux icônes sont identiques, et
    // l'échange ne se voit pas — ce qui est exactement le comportement voulu.
    const QIcon hover(QStringLiteral(":/icons/dark/%1_dark.svg").arg(baseName));

    action->setIcon(rest);
    action->setProperty("restIcon", QVariant::fromValue(rest));
    action->setProperty("hoverIcon", QVariant::fromValue(hover));
}

//---------------------------------------------------------------------------------------------------------------------
void OMenuGlyph::InstallHover(QMenu *menu, QObject *context)
{
    if (menu == nullptr || context == nullptr || menu->property("glyphHoverInstalled").toBool())
    {
        return;
    }

    menu->setProperty("glyphHoverInstalled", true);

    auto state = std::make_shared<HoverState>();
    QObject::connect(menu, &QMenu::hovered, context, [state](QAction *action) { ApplyHover(state, action); });
    QObject::connect(menu, &QMenu::aboutToHide, context, [state]() { ApplyHover(state, nullptr); });
}

//---------------------------------------------------------------------------------------------------------------------
void OMenuGlyph::StyleStandardMenu(QMenu *menu)
{
    if (menu == nullptr)
    {
        return;
    }

    bool styled = false;
    const QList<QAction *> actions = menu->actions();
    for (QAction *action : actions)
    {
        const QString baseName = GlyphForObjectName(action->objectName());
        if (baseName.isEmpty())
        {
            continue;
        }

        SetGlyph(action, baseName);
        styled = true;
    }

    if (styled)
    {
        InstallHover(menu, menu);
    }
}
