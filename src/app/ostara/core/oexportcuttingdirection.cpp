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

#include <QLineF>
#include <QTransform>
#include <QtNumeric>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// An angle in [0, 360), against 0: the engine compares angles modulo a full turn, and a negative
// one would only ever be normalised there anyway.
auto Normalized(qreal degrees) -> double
{
    double angle = std::fmod(static_cast<double>(degrees), 360.0);
    if (angle < 0.0)
    {
        angle += 360.0;
    }
    return angle;
}
} // namespace

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
    return fold;
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
    const QLineF fold = MappedFoldLine(piece);
    if (fold.isNull())
    {
        return false;
    }

    // Les deux rotations qui mettent le pli vertical — celles que `AllowedAngles` rend déjà pour un
    // pli. Le signe est celui du chantier : `QLineF::angle()` tourne dans un sens, `rotatePath` dans
    // l'autre, d'où le `− 90` et non le `+ 90`.
    qreal alignment = std::fmod(fold.angle(), 180.0);
    if (alignment < 0.0)
    {
        alignment += 180.0;
    }
    const double base = alignment - 90.0;

    // ⚠️ **Laquelle des deux ? Celle qui ne retourne pas la pièce.** Elles diffèrent d'un
    // demi-tour : l'une laisse la pièce dans son sens, l'autre la couche tête en bas — et les deux
    // sont posables, la seconde simplement à l'envers. Un demi-tour, c'est un cosinus négatif :
    // `rotatePath` retranche l'angle de l'angle écran, donc la rotation qui ne retourne rien est
    // celle dont le cosinus est positif. C'est le même choix que `g − 90` pour une grainline.
    const double candidates[2] = {base, base + 180.0};
    double chosen = candidates[0];
    for (const double candidate : candidates)
    {
        if (std::cos(candidate * M_PI / 180.0) > 0.0)
        {
            chosen = candidate;
            break;
        }
    }

    // Le contour que le moteur recevra, dans le même repère que le pli : c'est LUI que le pli doit
    // border, pas la pièce d'un autre repère.
    const QVector<VLayoutPoint> contour = piece.GetMappedExternalContourPoints();

    // Un dixième de millième de pixel. La rotation est une transformation de coordonnées, et deux
    // points que le calcul sépare de ça ne sont pas séparés : c'est du bruit, pas une pente.
    constexpr qreal kTolerance = 1e-4;

    QTransform turn;
    turn.rotate(chosen); // Qt tourne dans le sens de l'écran, comme `rotatePath`

    const QLineF turned = turn.map(fold);
    if (std::abs(turned.dx()) <= kTolerance)
    {
        // Le corps tombe d'un côté du pli — ou de l'autre. Le pli doit être le bord extrême de la
        // pièce dans ce sens-là, sinon il n'est pas un bord de sa propre pièce et rien ne le rendra
        // posable : le dire est le travail de l'appelant.
        const qreal edge = turned.x1();
        bool bodyOnTheRight = true;
        bool bodyOnTheLeft = true;
        for (const VLayoutPoint &point : contour)
        {
            const qreal x = turn.map(QPointF(point.x(), point.y())).x();
            bodyOnTheRight = bodyOnTheRight && (x >= edge - kTolerance);
            bodyOnTheLeft = bodyOnTheLeft && (x <= edge + kTolerance);
            if (!bodyOnTheRight && !bodyOnTheLeft)
            {
                break;
            }
        }

        if (bodyOnTheRight != bodyOnTheLeft)
        {
            *angle = Normalized(chosen);
            // Le corps à gauche veut un pli sur le bord DROIT pour l'accueillir, et réciproquement.
            *foldOnTheRight = bodyOnTheLeft;
            return true;
        }
    }

    return false;
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
    // plan that looked finished. Found by Sophie, who saw a single piece facing the wrong way and
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
        // départ l'est. Trouvé par Sophie : une pièce au pli sortait à l'envers, et ajouter une
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
    mirrored.angle = Normalized(180.0 - static_cast<double>(direction.angle));
    return mirrored; // the arrows and the "known" flag do not depend on which way round it is
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingDirection::AllowedAngles(const Direction &direction, bool oneWay) -> QVector<double>
{
    if (not direction.known)
    {
        return {};
    }

    // ⚠️ THE sign of this whole feature. See the header: Qt's angle() is counter-clockwise on
    // screen, the engine's rotatePath() is clockwise. Laying the direction along the fabric's
    // length takes `g - 90`; `g + 90` is that same orientation turned a half-turn.
    const double base = static_cast<double>(direction.angle) - 90.0;

    // Ostara only ever writes two of the twelve values — the dock offers "Simple"
    // (twoWaysUpDown) and "Double" (fourWays), and reads anything else as Simple. The ten others
    // exist in imported .val files and are read here the way the dock reads them: as two ways.
    // They deserve their own reading the day one of them is actually reachable.
    if (direction.arrows == GrainlineArrowDirection::fourWays)
    {
        // The cross — TWO axes: droit-fil, and contre-fil. Both are legitimate, so the one-way
        // option keeps them both and takes one sense of each. Sophie, on what the cross means.
        if (oneWay)
        {
            return {Normalized(base), Normalized(base + 90.0)};
        }

        return {Normalized(base), Normalized(base + 90.0), Normalized(base + 180.0), Normalized(base + 270.0)};
    }

    // One axis only, so one-way leaves a single orientation: the piece is laid the way it was
    // drawn, and no copy of it is turned a half-turn.
    if (oneWay)
    {
        return {Normalized(base)};
    }

    return {Normalized(base), Normalized(base + 180.0)};
}
