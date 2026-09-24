/************************************************************************
 **
 **  @file   oexportpuzzlereference.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The reassembly marks: one shape straddling every cut, so
 **          that the parts of a piece can be put back together.
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

#include "oexportpuzzlereference.h"

#include <QPolygonF>
#include <QRectF>

#include <cmath>

namespace
{
/// Le nombre de côtés du rond. Un repère se REGARDE, il ne se mesure pas : au-delà, on paie des
/// points que personne ne voit. À 32 côtés, un rond d'un centimètre s'écarte de son cercle d'un
/// dixième de millimètre — bien en dessous de ce qu'une plume laisse sur du tissu.
constexpr int kSides = 32;

/// Combien de positions on essaie le long d'une coupe avant de choisir. Assez pour trouver le milieu
/// d'un passage étroit, assez peu pour que le calcul ne se voie pas.
constexpr int kAttempts = 24;

constexpr double kTwoPi = 6.283185307179586;

/// Le répertoire des formes, dans l'ordre où les coupes les reçoivent.
///
/// ⚠️ **L'ordre compte autant que les formes.** C'est lui qui distingue la première coupe de la
/// deuxième, et une forme qui reviendrait sur une coupe voisine dirait la même chose deux fois. Le
/// répertoire reboucle au-delà de la cinquième : une pièce coupée six fois ne se recoud pas à
/// l'aveugle, et son morceau du milieu porte de toute façon deux formes, qui le situent déjà.
enum class Kind
{
    Circle,
    Square,
    Triangle,
    Diamond,
    Cross
};

constexpr int kKindCount = 5;

/// Une forme du répertoire, prête à poser : ses points autour de son centre, et les points sur
/// lesquels on l'interroge — ses sommets, **et le milieu de chacun de ses côtés**.
///
/// ⚠️ Les milieux, parce qu'un sommet seul ne dit pas qu'une forme tient dans la matière : une croix
/// a des creux, et un creux peut sortir de la pièce sans qu'aucun de ses sommets n'en sorte.
struct Mark
{
    QVector<QPointF> offsets{};
    QVector<QPointF> probes{};
};

/// La forme @p kind, autour de son centre, dans une boîte de @p half de demi-côté.
///
/// ⚠️ **Toutes les formes tiennent dans la même boîte**, le rond compris : le rond la touche aux
/// quatre côtés, le carré la remplit, le triangle et le losange s'y inscrivent. Sans ça, un carré
/// paraîtrait plus gros que le rond qu'il remplace — et un repère se reconnaît avant de se mesurer.
///
/// La forme est toujours droite **dans le repère où la coupe est une droite**, et elle y revient par
/// la transformation inverse. C'est ce qui fait qu'une croix se lit comme une croix, et non comme
/// une croix couchée par le quart de tour du tapis.
auto MakeMark(Kind kind, double half) -> Mark
{
    QVector<QPointF> offsets;

    switch (kind)
    {
        case Kind::Circle:
            for (int side = 0; side <= kSides; ++side) // le premier point répété : la forme se referme
            {
                const double angle = kTwoPi * side / kSides;
                offsets.append(QPointF(half * std::cos(angle), half * std::sin(angle)));
            }
            break;
        case Kind::Square:
            offsets = {{-half, -half}, {half, -half}, {half, half}, {-half, half}, {-half, -half}};
            break;
        case Kind::Triangle:
            offsets = {{0.0, -half}, {half, half}, {-half, half}, {0.0, -half}};
            break;
        case Kind::Diamond:
            offsets = {{0.0, -half}, {half, 0.0}, {0.0, half}, {-half, 0.0}, {0.0, -half}};
            break;
        case Kind::Cross:
        {
            // Le bras fait la moitié du demi-côté : une croix plus grasse serait un carré à peine
            // entaillé, et ne se distinguerait plus du carré qu'elle doit remplacer.
            const double arm = half / 2.0;
            offsets = {{-arm, -half},  {arm, -half},  {arm, -arm},   {half, -arm},  {half, arm},
                       {arm, arm},     {arm, half},   {-arm, half},  {-arm, arm},   {-half, arm},
                       {-half, -arm},  {-arm, -arm},  {-arm, -half}};
            break;
        }
    }

    Mark mark;
    mark.offsets = offsets;
    mark.probes = offsets;
    for (int i = 0; i + 1 < offsets.size(); ++i)
    {
        mark.probes.append((offsets.at(i) + offsets.at(i + 1)) / 2.0);
    }

    return mark;
}

/// La forme posée en @p center tombe-t-elle ENTIÈREMENT dans la pièce ?
auto Inside(const QPolygonF &material, const Mark &mark, const QPointF &center) -> bool
{
    for (const QPointF &probe : mark.probes)
    {
        if (!material.containsPoint(center + probe, Qt::OddEvenFill))
        {
            return false;
        }
    }

    return true;
}

/// La forme que reçoit la coupe de rang @p rank : le répertoire dans l'ordre, puis on reboucle.
auto Repertoire(int rank) -> Kind
{
    return static_cast<Kind>(rank % kKindCount);
}

/// Pose une forme au milieu du plus long passage d'une coupe, s'il en existe un.
///
/// Le milieu du plus LONG passage, et pas le milieu de tout : un contour en C traverse une coupe
/// deux fois, et le milieu de l'ensemble tomberait alors dans le vide, entre les deux passages.
void AddMark(const QPolygonF &material, Kind kind, const QPointF &from, const QPointF &to,
             double half, const QTransform &back, QVector<QVector<QPointF>> &marks)
{
    const Mark mark = MakeMark(kind, half);

    QVector<QPointF> centers;
    QVector<bool> holds;
    centers.reserve(kAttempts + 1);
    holds.reserve(kAttempts + 1);

    for (int attempt = 0; attempt <= kAttempts; ++attempt)
    {
        const double ratio = static_cast<double>(attempt) / kAttempts;
        const QPointF center = from + (to - from) * ratio;
        centers.append(center);
        holds.append(Inside(material, mark, center));
    }

    int bestStart = -1;
    int bestLength = 0;

    for (int i = 0; i < holds.size();)
    {
        if (!holds.at(i))
        {
            ++i;
            continue;
        }

        const int start = i;
        while (i < holds.size() && holds.at(i))
        {
            ++i;
        }

        if (i - start > bestLength)
        {
            bestStart = start;
            bestLength = i - start;
        }
    }

    if (bestStart < 0)
    {
        return; // la coupe n'existe pas des deux côtés : une forme n'y dirait rien
    }

    // Le centre est raisonné dans le repère des cellules — celui où les coupes sont des droites — et
    // la forme se pose dans celui de la pièce, où la découpe l'attend. Chaque point y repasse donc,
    // et pas seulement le centre : une forme n'est pas ronde, et un carré posé sans tourner ne serait
    // pas le carré que la coupe a séparé.
    const QPointF center = centers.at(bestStart + (bestLength - 1) / 2);

    QVector<QPointF> points;
    points.reserve(mark.offsets.size());
    for (const QPointF &offset : mark.offsets)
    {
        points.append(back.map(center + offset));
    }

    marks.append(points);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPuzzleReference::Marks(const QVector<QPointF> &outline, const QTransform &turn,
                                   const OExportPlotter::PiecePlan &plan, double sizePx)
    -> QVector<QVector<QPointF>>
{
    QVector<QVector<QPointF>> marks;

    // Une pièce tenue dans une seule cellule n'est pas coupée : il n'y a rien à recoller.
    if (plan.columns * plan.rows <= 1 || outline.size() < 3 || sizePx <= 0.0)
    {
        return marks;
    }

    // Le contour dans le repère où vivent les cellules : c'est là que les coupes sont des droites,
    // et donc là que se demande « la pièce existe-t-elle des deux côtés ? ».
    QPolygonF material;
    material.reserve(outline.size());
    for (const QPointF &point : outline)
    {
        material.append(turn.map(point));
    }

    const double half = sizePx / 2.0;
    const QTransform back = turn.inverted();

    int rank = 0;

    // ── Les coupes VERTICALES, de gauche à droite ───────────────────────────────────────────────
    //
    // ⚠️ Une coupe est la DROITE d'une cellule, pas un segment : toutes les rangées la partagent, et
    // elles reçoivent donc la MÊME forme. Sinon les deux morceaux qui se touchent le long d'une même
    // droite porteraient deux formes différentes — c'est-à-dire aucune information.
    //
    // Une forme s'arrête à un demi-côté des coins de la cellule : sinon elle déborderait dans la
    // cellule du dessus ou du dessous, et une coupe à quatre morceaux autour d'elle la découperait
    // en quatre.
    for (int column = 0; column + 1 < plan.columns; ++column)
    {
        const Kind kind = Repertoire(rank);
        ++rank;

        for (int row = 0; row < plan.rows; ++row)
        {
            const QRectF &cell = plan.cuts.at(row * plan.columns + column);
            AddMark(material, kind, QPointF(cell.right(), cell.top() + half),
                    QPointF(cell.right(), cell.bottom() - half), half, back, marks);
        }
    }

    // ── Puis les HORIZONTALES, de haut en bas ──────────────────────────────────────────────────
    for (int row = 0; row + 1 < plan.rows; ++row)
    {
        const Kind kind = Repertoire(rank);
        ++rank;

        for (int column = 0; column < plan.columns; ++column)
        {
            const QRectF &cell = plan.cuts.at(row * plan.columns + column);
            AddMark(material, kind, QPointF(cell.left() + half, cell.bottom()),
                    QPointF(cell.right() - half, cell.bottom()), half, back, marks);
        }
    }

    return marks;
}
