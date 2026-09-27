/************************************************************************
 **
 **  @file   ovistoolrectangle.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  preview of a rectangle, with its two lengths told apart
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

#ifndef OVISTOOLRECTANGLE_H
#define OVISTOOLRECTANGLE_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"
#include "visline.h"

class VScaledLine;

/**
 * @brief The preview of a rectangle, while it is being described in the window.
 *
 * The rectangle is drawn whole, but not all of it in the same colour: the top side is the width and the right
 * side is the height, and they carry the same two colours as the pastilles that name them in the window. That
 * is the whole point — the user sees which length the green dot and the red dot are about before the rectangle
 * exists.
 */
class OVisToolRectangle : public VisLine
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolRectangle(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolRectangle() override = default;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    void SetBasePointId(quint32 value);
    void SetFormulaWidth(const QString &expression);
    void SetFormulaHeight(const QString &expression);

    /** @brief The type and the colour the user chose for the rectangle. The two lengths keep their own two
     *  colours — that is what the pastilles name — but they follow the chosen style, so that what the preview
     *  shows is what the rectangle will look like. */
    void SetTypeLine(const QString &typeLine);
    void SetLineColor(const QString &color);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolRectangle)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolRectangle) // NOLINT

    quint32 m_basePointId{NULL_ID};
    qreal m_width{0.0};
    qreal m_height{0.0};
    QString m_typeLine{TypeLineLine};
    QString m_lineColor{ColorBlack};

    VScaledEllipse *m_pointA{nullptr};
    VScaledEllipse *m_pointB{nullptr};
    VScaledEllipse *m_pointC{nullptr};
    VScaledEllipse *m_pointD{nullptr};

    VScaledLine *m_lineWidth{nullptr};  // A→B, the width
    VScaledLine *m_lineHeight{nullptr}; // B→C, the height
    VScaledLine *m_lineCD{nullptr};
    VScaledLine *m_lineDA{nullptr};

    /** @brief Put the chosen style and the four colours on the four sides. */
    void ApplyPens();
};

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolRectangle::SetBasePointId(quint32 value)
{
    m_basePointId = value;
}

#endif // OVISTOOLRECTANGLE_H
