/************************************************************************
 **
 **  @file   oexportformat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Export mode — the tokens the formats are stored under.
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

#include "oexportformat.h"

#include <QLatin1String>

namespace
{
// Les jetons sont écrits ici une fois, et lus ici une fois. Ce sont des CLÉS de fichier : les renommer
// rendrait illisibles tous les favoris déjà enregistrés.
constexpr auto kPrint = "print";
constexpr auto kCutting = "cutting";
constexpr auto kPlotter = "plotter";
constexpr auto kProjector = "projector";
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportFormat::Token(Kind kind) -> QString
{
    switch (kind)
    {
        case Kind::CuttingPlan:
            return QString::fromLatin1(kCutting);
        case Kind::Plotter:
            return QString::fromLatin1(kPlotter);
        case Kind::Projector:
            return QString::fromLatin1(kProjector);
        case Kind::Print:
            break;
    }

    return QString::fromLatin1(kPrint);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFormat::FromToken(const QString &token, Kind fallback) -> Kind
{
    if (token.isEmpty())
    {
        return fallback;
    }

    if (token == QLatin1String(kCutting))
    {
        return Kind::CuttingPlan;
    }
    if (token == QLatin1String(kPlotter))
    {
        return Kind::Plotter;
    }
    if (token == QLatin1String(kProjector))
    {
        return Kind::Projector;
    }
    if (token == QLatin1String(kPrint))
    {
        return Kind::Print;
    }

    return fallback;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFormat::FromLegacyFlags(bool cuttingPlan, bool plotterPlan) -> Kind
{
    // L'ordre des deux tests est celui de la lecture d'alors : le plotter l'emportait, parce que c'est
    // lui qui avait été ajouté en dernier et que rien n'interdisait les deux à la fois.
    if (plotterPlan)
    {
        return Kind::Plotter;
    }
    if (cuttingPlan)
    {
        return Kind::CuttingPlan;
    }

    return Kind::Print;
}
