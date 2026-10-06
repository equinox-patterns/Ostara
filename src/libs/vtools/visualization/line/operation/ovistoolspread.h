/************************************************************************
 **
 **  @file   ovistoolspread.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The preview of a slash-and-spread: the operated objects and their spread images.
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

#ifndef OVISITOOLSPREAD_H
#define OVISITOOLSPREAD_H

#include <QMetaObject>
#include <QObject>
#include <QVector>
#include <QtGlobal>

#include "../../../tools/drawTools/operation/ospreadrule.h"
#include "../vmisc/def.h"
#include "visoperation.h"

class OSpreadTransform;

class OVisToolSpread : public VisOperation
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolSpread(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolSpread() override = default;

    void RefreshGeometry() override;

    /** @brief The ordered source ids and the rotations of the operation, so the preview can replay the very
     *  chain the tool will. */
    void SetRotations(const QVector<quint32> &sourceIds, const QVector<OSpreadRotation> &rotations);

    /** @brief Tell the preview that the spread is already in the pattern.
     *
     *  The pattern then draws the objects the operation spread, where they stand, and the trace of their origin laid
     *  over that drawing says nothing more. The images stay: they are what shows what the spread does while its dialog
     *  is open, and a spread being built has nothing in the pattern for them to duplicate. */
    void SetInPattern(bool inPattern);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolSpread)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolSpread) // NOLINT

    QVector<quint32> m_sourceIds{};
    QVector<OSpreadRotation> m_rotations{};

    /** True when the spread is in the pattern: the preview then keeps the images of the operated objects and hides
     *  their origin, which the pattern draws itself (see SetInPattern). */
    bool m_inPattern{false};

    void CreateTransformedObjects(int &iPoint, int &iCurve, const OSpreadTransform &transform);

    /** @brief Draw the lines attached to the spread as they are — their two ends are the reference the user picked —
     *  and keep them out of sight where the pattern draws those lines itself. */
    void DrawAttachedLines();

    template <class Item> auto AddTransformedCurve(quint32 id, int i, const OSpreadTransform &transform) -> int;
};

#endif // OVISITOOLSPREAD_H
