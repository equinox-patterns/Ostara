/************************************************************************
 **
 **  @file   oexportrowiconbutton.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Le bouton-icône d'une ligne de liste : variante du thème au repos, variante
 **          `_dark` au survol.
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

#ifndef OEXPORTROWICONBUTTON_H
#define OEXPORTROWICONBUTTON_H

#include <QPushButton>

/**
 * @brief L'icône d'un bouton de ligne : variante du thème au repos, variante `_dark` au survol.
 *
 * **La règle, et sa raison.** `Modifications/Ostara/Interface/references/Icons.md` (« Règle hover
 * Measurement ») dit qu'au survol l'icône utilise la variante `_dark`, quel que soit le thème. Ce
 * n'est pas une convention de goût : le fond de survol d'un `QPushButton` est l'**accent**
 * (`#8C4560` en light, `#983E60` en dark — `share/themes/generated/light.qss` et `dark.qss`), donc
 * sombre dans les DEUX thèmes. Un glyphe clair y disparaît. En dark, l'icône du survol est la même
 * que celle du repos, et c'est normal : elle est déjà `_dark`.
 *
 * **Pourquoi une classe.** Le dock Outputs fait déjà ce calcul à la main, bouton par bouton
 * (`OWidgetOutputs::UpdateButtonIcons` et son `eventFilter`). Les deux sortes de lignes du dialogue
 * de création ont besoin de la même chose, et une règle réécrite à deux endroits finit toujours par
 * diverger de l'un des deux.
 *
 * Le survol est suivi par `enterEvent` / `leaveEvent` plutôt que par un filtre d'événements : le
 * bouton n'a que lui-même à surveiller.
 */
class OExportRowIconButton : public QPushButton
{
    Q_OBJECT

public:
    /// @param baseName le nom du fichier d'icône sans suffixe de thème (« trash », « star-fill »).
    ///                 Les deux variantes sont résolues à chaque rafraîchissement, jamais gardées :
    ///                 c'est ce qui fait suivre un basculement Light/Dark.
    explicit OExportRowIconButton(const QString &baseName, QWidget *parent = nullptr);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void Refresh();

    QString m_baseName{};
    bool m_hovered{false};
};

#endif  // OEXPORTROWICONBUTTON_H
