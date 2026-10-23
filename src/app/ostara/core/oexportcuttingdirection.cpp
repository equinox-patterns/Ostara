/************************************************************************
 **
 **  @file   oexportcuttingdirection.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — which rotations a piece may take, from its direction.
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

#include "oexportcuttingdirection.h"

#include "../vpatterndb/opiecegroup.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vwidgets/vpiecegrainline.h"

#include "nesting/CuttingRules.h"  // Trame — les règles du plan de coupe

#include <QLineF>
#include <QPointF>
#include <QTransform>
#include <QtNumeric>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>


namespace
{
// The export's own direction, in the socle's words. One conversion, at the boundary: the two enums
// carry the same twelve values in the same order, and neither is allowed its own opinion.
auto ToRules(const OExportCuttingDirection::Direction &direction) -> CuttingRules::Direction
{
    CuttingRules::Direction out;
    out.known = direction.known;
    out.angle = static_cast<double>(direction.angle);
    out.arrows = static_cast<CuttingRules::Arrows>(static_cast<int>(direction.arrows));
    return out;
}

/// @brief Le pli RAMENÉ sur l'arête de la pièce qu'il longe, quand il n'y est pas déjà.
///
/// ⚠️ **Le pli n'était pas un bord de sa propre pièce, et c'est un défaut de calcul, pas de dessin.**
/// La ligne vient de `GetSeamAllowanceMirrorLine` — la ligne de miroir de la valeur de couture,
/// calculée sur le RECTANGLE de la pièce — tandis que le contour extérieur sort de l'algorithme
/// d'offset, qui recalcule ses propres coins. Deux calculs indépendants, donc deux résultats : sur
/// « Devant x1 pli » d'un patron réel, l'axe longe l'arête en passant 0,04 px à côté de son sommet,
/// qui se retrouve 0,013 px du mauvais côté. Trois millièmes de millimètre, invisibles à l'écran — et
/// assez pour que tout lecteur refuse la pièce, Ostara comprise.
///
/// Le remède est à la source : on cherche l'arête du contour la plus proche de l'axe et parallèle à
/// lui, et on TRANSLATE l'axe sur sa droite. Ses extrémités ne bougent pas — un pli ne change pas de
/// longueur parce qu'on le recale — et il ne reste qu'un déplacement perpendiculaire du bruit du
/// calcul. Une pièce dont le pli ne longe vraiment aucune arête garde son axe tel quel, et c'est le
/// garde-fou qui le dit.
auto SnapToEdge(const VLayoutPiece &piece, const QLineF &axis) -> QLineF
{
    if (axis.isNull())
    {
        return axis;
    }

    // Le MÊME contour que celui que le moteur reçoit, et dans le même repère que l'axe : celui de la
    // pièce, avant sa matrice. Les comparer dans deux repères serait comparer deux dessins.
    const QVector<VLayoutPoint> points = piece.GetExternalContourPoints();
    if (points.size() < 3)
    {
        return axis;
    }

    constexpr qreal kAngleTolerance = 1e-3;  // degrés — deux droites sont parallèles ou non
    constexpr qreal kDistanceLimit = 1.0;    // px — au-delà, ce n'est pas un pli posé sur un bord

    const qreal axisAngle = axis.angle();
    const qreal radians = axisAngle * M_PI / 180.0;
    const QPointF normal(std::sin(radians), -std::cos(radians));

    qreal bestOffset = 0.0;
    qreal bestDistance = kDistanceLimit;
    for (qsizetype i = 0; i < points.size(); ++i)
    {
        const QPointF a = points.at(i);
        const QPointF b = points.at((i + 1) % points.size());
        if (a == b)
        {
            continue;
        }

        // Parallèle : le sens de parcours du contour n'a pas d'importance, et une arête perpendiculaire
        // au pli n'est pas le pli.
        qreal turn = std::fmod(std::abs(QLineF(a, b).angle() - axisAngle), 180.0);
        if (turn > 90.0)
        {
            turn = 180.0 - turn;
        }
        if (turn > kAngleTolerance)
        {
            continue;
        }

        // La distance SIGNÉE du premier sommet de l'arête à l'axe : c'est de ça qu'il faut décaler.
        const qreal offset = QPointF::dotProduct(a - axis.p1(), normal);
        const qreal distance = std::abs(offset);
        if (distance > bestDistance)
        {
            continue;
        }

        bestDistance = distance;
        bestOffset = offset;
    }

    if (bestDistance >= kDistanceLimit)
    {
        return axis;
    }

    return QLineF(axis.p1() - normal * bestOffset, axis.p2() - normal * bestOffset);
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::FoldLine(const VLayoutPiece &piece) -> QLineF
{
    // ⚠️ L'ordre compte, et il est le même partout : la ligne de MARGE d'abord, celle d'assemblage
    // ensuite. Une pièce sans marge rend la ligne d'assemblage telle quelle, donc les deux
    // répondent la même chose ; et une pièce dont le rayon de l'axe rate le polygone de marge rend
    // une ligne nulle là où l'autre existe — c'est le cas qui a rendu le garde-fou muet.
    QLineF fold = piece.GetSeamAllowanceMirrorLine();
    if (fold.isNull())
    {
        fold = piece.GetSeamMirrorLine();
    }

    // Et le pli devient LE BORD qu'il longe — voir `SnapToEdge`, et le défaut de 0,013 px qu'il
    // répare. Fait ici, une fois : tout ce qui lit un pli dans l'export passe par cette fonction,
    // donc le contrôle, l'angle de rotation, la place du décor et le payload disent tous la même
    // ligne, exactement celle du contour.
    return SnapToEdge(piece, fold);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::MappedFoldLine(const VLayoutPiece &piece) -> QLineF
{
    return piece.GetMatrix().map(FoldLine(piece));
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::FoldAngle(const VLayoutPiece &piece, double *angle, bool *foldOnTheRight)
    -> bool
{
    // The same reading of the fold as everywhere else — `MappedFoldLine`, never the marking's type —
    // and one frame for the axis and the contour: the one the engine will receive.
    const QLineF fold = MappedFoldLine(piece);

    CuttingRules::Line line;
    line.x1 = fold.x1();
    line.y1 = fold.y1();
    line.x2 = fold.x2();
    line.y2 = fold.y2();

    const QVector<VLayoutPoint> points = piece.GetMappedExternalContourPoints();
    std::vector<std::pair<double, double>> contour;
    contour.reserve(static_cast<std::size_t>(points.size()));
    for (const VLayoutPoint &point : points)
    {
        contour.emplace_back(point.x(), point.y());
    }

    return CuttingRules::FoldAngle(contour, line, angle, foldOnTheRight);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::PieceDirection(const VLayoutPiece &piece) -> Direction
{
    // The grainline first: it is the explicit one. ⚠️ IsEnabled says the direction was GIVEN —
    // IsVisible only says whether it is drawn, and a direction that is not shown is still one.
    const VPieceGrainline grainline = piece.GetGrainline();
    if (grainline.IsEnabled())
    {
        return Direction{true, grainline.GetMainLine().angle(), grainline.GetArrowType()};
    }

    // Otherwise the fold: its axis is the grain (§7 — « la foldline a un sens, et c'est le sens du
    // droit-fil »), and it is the same frame as the grainline.
    //
    // ⚠️ **The SAME line the guard reads** — @ref FoldLine, and not a second reading of it. The
    // guard calls a piece directed when its seam-allowance mirror line is not null; reading the
    // seam line here instead — or the other one only when the main path happens to be hidden — left
    // a piece the guard had let through with NO constraint at all. The engine then placed it
    // wherever the annealing liked, and nothing said so: one piece of a plan, laid at random, in a
    // plan that looked finished. Found by hand: a single piece facing the wrong way and
    // fixed it by adding a grainline.
    const QLineF fold = FoldLine(piece);
    if (not fold.isNull())
    {
        // ⚠️ Un pli est un AXE, pas une direction : ses deux points n'ont pas d'ordre, et `angle()`
        // rend donc 0° ou 180° selon celui que le fichier a écrit en premier.
        //
        // Ce n'est PAS anodin, et je l'ai cru à tort : l'ENSEMBLE des rotations permises est bien
        // le même dans les deux cas — `{g−90, g+90}` et `{g+90, g+270}` sont identiques — mais
        // **l'index 0 de la banque ne l'est pas**, et le moteur PART de là. Rien dans sa fonction
        // objectif ne préfère un sens à l'autre : la pièce reste donc retournée si l'angle de
        // départ l'est. Trouvé : une pièce au pli sortait à l'envers, et ajouter une
        // grainline la remettait d'aplomb — la grainline, elle, a un sens qu'elle a dessiné.
        //
        // On ramène donc l'angle du pli sur un demi-tour : les deux ordres donnent la MÊME
        // orientation, la plus proche de celle du patron.
        qreal alignment = std::fmod(fold.angle(), 180.0);
        if (alignment < 0.0)
        {
            alignment += 180.0;
        }
        return Direction{true, alignment, GrainlineArrowDirection::twoWaysUpDown};
    }

    return Direction{};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::GroupDirection(const OPieceGroup &group) -> Direction
{
    const VGrainlineData &grainline = group.m_glGrainline;
    if (not (grainline.IsEnabled() && grainline.IsVisible()))
    {
        return Direction{};
    }

    // A group's rotation is a LITERAL, unlike a piece's, which is a formula evaluated against the
    // measurement table (OGraded::GrainlineRotation draws that line — and it is the only one).
    bool ok = false;
    const qreal degrees = grainline.GetRotation().toDouble(&ok);
    if (not ok)
    {
        return Direction{};
    }

    return Direction{true, degrees, grainline.GetArrowType()};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::Mirrored(const Direction &direction) -> Direction
{
    Direction mirrored = direction;
    mirrored.angle = CuttingRules::Mirrored(ToRules(direction)).angle;
    return mirrored; // the arrows and the "known" flag do not depend on which way round it is
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::AllowedAngles(const Direction &direction, bool oneWay) -> QVector<double>
{
    // **The rule lives in the socle, and only there.** A second application that needs a plan would
    // otherwise copy this arithmetic — and a sign written twice is a sign that can be right in one
    // place and wrong in the other, with nothing on screen to show which one is which.
    const std::vector<double> angles = CuttingRules::AllowedAngles(ToRules(direction), oneWay);

    QVector<double> out;
    out.reserve(static_cast<int>(angles.size()));
    for (const double angle : angles)
    {
        out.append(angle);
    }
    return out;
}
