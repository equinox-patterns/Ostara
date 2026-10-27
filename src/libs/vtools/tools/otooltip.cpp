/************************************************************************
 **
 **  @file   otooltip.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/10/2026
 **
 **  @brief  The hover tooltip of a Draw-mode object: what it may name, and how it reads a length.
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
#include "otooltip.h"

#include <QColor>
#include <QString>
#include <QStringList>

#include "../ifc/ifcdef.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vtheme/vscenestylesheet.h"

namespace
{
/** @brief The head the user writes, translated — `Ligne_`, `Arc_`, `Courbe_` — or empty when it is none.
 *
 *  Curves answer one head whatever the file says: `Spl_` and `SplPath_` both read `Curve_`, the head the
 *  formula reader itself writes (`VTranslateVars`, whose French is `Courbe_`). The other heads are only
 *  translated, never renamed.
 */
auto UserHead(const QString &head) -> QString
{
    if (head == QLatin1String(SPL_) || head == splPath_V + QLatin1Char('_'))
    {
        return VAbstractApplication::VApp()->TrVars()->InternalVarToUser(curve_);
    }

    return VAbstractApplication::VApp()->TrVars()->InternalVarToUser(head);
}

/** @brief The part of a name before the technical half — `Spl_dos` in `Spl_dos (Spl_A_B)` — or empty.
 *
 *  A `VGObject::ObjectName()` carries its technical name in parentheses; a name without one has no alias
 *  in it at all, which is exactly what the tooltip must not show.
 */
auto AliasPart(const QString &objectName) -> QString
{
    const qsizetype aliasEnd = objectName.indexOf(QLatin1String(" ("));
    if (aliasEnd < 0)
    {
        return {};
    }

    return objectName.left(aliasEnd);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::DisplayName(const VGObject &object) -> QString
{
    if (object.GetAlias().isEmpty())
    {
        return {};
    }

    return DisplayName(object.ObjectName());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::DisplayName(const QString &objectName) -> QString
{
    const QString alias = AliasPart(objectName);
    if (alias.isEmpty())
    {
        return {};
    }

    const qsizetype headEnd = alias.indexOf(QLatin1Char('_'));
    if (headEnd < 0)
    {
        // An alias the container never headed: it is the user's own word, so it stands as it is.
        return alias;
    }

    const QString head = alias.left(headEnd + 1);
    return UserHead(head) + alias.mid(headEnd + 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::LineName(const QString &roleName) -> QString
{
    if (roleName.isEmpty())
    {
        return {};
    }

    return UserHead(line_) + roleName;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::Measure(qreal pixels) -> QString
{
    return QStringLiteral("%1 %2")
        .arg(RoundForDisplay(VAbstractValApplication::VApp()->fromPixel(pixels)))
        .arg(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::Degrees(qreal angle) -> QString
{
    return QStringLiteral("%1°").arg(RoundForDisplay(angle));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::Table(const QStringList &rows) -> QString
{
    QString html;
    for (const QString &row : rows)
    {
        html += row;
    }

    if (html.isEmpty())
    {
        return {};
    }

    return QStringLiteral("<table>%1</table>").arg(html);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::Row(const QString &label, const QString &value) -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>").arg(label, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::NameRow(const VGObject &object) -> QString
{
    return NameRow(DisplayName(object));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::NameRow(const QString &name) -> QString
{
    if (name.isEmpty())
    {
        return {};
    }

    return QStringLiteral("<tr> <td colspan=\"100%\"><b>%1</b></td> </tr>").arg(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::StrokeRow(const QString &label, const QString &value, VColorRole role) -> QString
{
    const QColor color = VSceneStylesheet::Color(role);
    return QStringLiteral("<tr> <td><span style=\"color:%1\">&#9632;</span> <b>%2:</b> %3</td> </tr>")
        .arg(color.name(), label, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::StrokeNameRow(const QString &label, const VGObject &object, VColorRole role) -> QString
{
    const QString name = DisplayName(object);
    if (name.isEmpty())
    {
        return {};
    }

    return StrokeRow(label, name, role);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTip::Separator() -> QString
{
    return QStringLiteral("<tr> <td colspan=\"100%\"><hr/></td> </tr>");
}
