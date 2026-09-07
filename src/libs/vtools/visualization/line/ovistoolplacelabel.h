/************************************************************************
 **
 **  @file   ovistoolplacelabel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Overlay magenta (VisSupportColor) des points retenus pendant la
 **          collecte multi-points de l'outil Place Label (mode Draw).
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

#ifndef OVISTOOLPLACELABEL_H
#define OVISTOOLPLACELABEL_H

#include <QVector>

#include "visline.h"

/**
 * @brief OVisToolPlaceLabel pose un marqueur magenta par point d'ancre retenu.
 *
 * Remplaçante multi-points de VisToolSpecialPoint pour la dialog Place Label :
 * pendant la collecte, chaque clic (dé)selectionne un point ; cette vis surligne
 * en VisSupportColor (couleur du hover) tous les points retenus d'un coup.
 * Les marqueurs sont des enfants de la vis : ils disparaissent avec elle
 * (nettoyage automatique quand la dialog ferme ou que la scène est reconstruite).
 */
class OVisToolPlaceLabel : public VisLine
{
    Q_OBJECT // NOLINT
public:
    explicit OVisToolPlaceLabel(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolPlaceLabel() override;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    /** @brief Remplace la liste des ancres retenues et rafraîchit l'aperçu. */
    void SetPointIds(const QVector<quint32> &ids);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolPlaceLabel)
    };

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OVisToolPlaceLabel) // NOLINT

    QVector<quint32> m_pointIds{};        /**< Ancres de construction retenues. */
    QVector<VScaledEllipse *> m_markers{}; /**< Un marqueur par ancre retenue. */
};

#endif // OVISTOOLPLACELABEL_H
