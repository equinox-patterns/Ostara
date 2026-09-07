/************************************************************************
 **
 **  @file   onodeplacelabel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Clickable place-label point of a Details piece
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

#ifndef ONODEPLACELABEL_H
#define ONODEPLACELABEL_H

#include <QGraphicsItem>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "../vmisc/typedef.h"
#include "../vwidgets/vscenepoint.h"

class VPlaceLabelItem;

/**
 * @brief The ONodePlaceLabel class an interactive per-place-label handle drawn on a Details
 * piece. It reuses the standard "real point" visuals of VScenePoint so a place label reads and
 * behaves like the piece's own node points (outline circle at rest, name + leader line, hover
 * highlight) while staying a lightweight child of the piece — it is NOT a data tool and NOT a
 * seam-allowance node. Clicking it selects the owning piece (the main window routes the scene
 * press); it is not movable and holds no editing state of its own.
 *
 * Ported behaviour from Val Fork (VNodePlaceLabel), re-expressed on VScenePoint so the look
 * matches the piece's real points instead of re-implementing the paint.
 */
class ONodePlaceLabel final : public VScenePoint
{
public:
    explicit ONodePlaceLabel(QGraphicsItem *parent = nullptr);
    ~ONodePlaceLabel() override = default;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Tool::PlaceLabel)
    };

    /** Re-aims the handle at label @a labelId and refreshes geometry/name from the current
     *  place-label item. */
    void UpdateFromLabel(quint32 labelId, const VPlaceLabelItem &label);

    auto getId() const -> quint32 { return m_id; }

protected:
    auto itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant override;

private:
    Q_DISABLE_COPY_MOVE(ONodePlaceLabel) // NOLINT

    quint32 m_id{NULL_ID};
};

#endif // ONODEPLACELABEL_H
