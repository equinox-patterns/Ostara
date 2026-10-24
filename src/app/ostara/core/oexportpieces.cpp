/************************************************************************
 **
 **  @file   oexportpieces.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/10/2026
 **
 **  @brief  Export mode — the pieces as a cutting plan has to read them.
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

#include "oexportpieces.h"

#include "../ogradedsvgwriter.h"  // OGradedSvg::OwnerDecorFor
#include "../oexportgeometry.h"
#include "../osvgutils.h"  // OSvg::PainterPathToSvgD
#include "../vlayout/vlayoutpiece.h"
#include "../vwidgets/vpiecegrainline.h"
#include "../vpatterndb/vcontainer.h"
#include "oexportcuttingdirection.h"
#include "oexportcuttingguard.h"

#include <QDomDocument>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QHash>
#include <QLineF>
#include <QStringList>

namespace
{
/// Three decimals of a pixel — a thirtieth of a millimetre. The nesting itself rounds vertices at
/// 1e-6 px, so this is the drawing's precision and not the engine's: what a reader rebuilds from
/// here is a shape to plan with, and a plan is placed in metres of cloth.
constexpr int kDecimals = 3;

auto Number(qreal value) -> QString
{
    return QString::number(static_cast<double>(value), 'f', kDecimals);
}

/// A contour as an SVG path, in the pattern's own frame.
auto PathData(const QVector<VLayoutPoint> &points) -> QString
{
    if (points.size() < 3)
    {
        return QString();
    }

    QStringList commands;
    commands.reserve(static_cast<int>(points.size()) + 1);

    bool first = true;
    for (const VLayoutPoint &point : points)
    {
        commands.append(QStringLiteral("%1 %2,%3")
                            .arg(first ? QStringLiteral("M") : QStringLiteral("L"), Number(point.x()),
                                 Number(point.y())));
        first = false;
    }
    commands.append(QStringLiteral("Z"));

    return commands.join(QLatin1Char(' '));
}

/// A line as « x1,y1 x2,y2 ».
auto LineData(const QLineF &line) -> QString
{
    return QStringLiteral("%1,%2 %3,%4")
        .arg(Number(line.x1()), Number(line.y1()), Number(line.x2()), Number(line.y2()));
}

/// A point as « x,y » — the pair an offset is stated with.
auto PointData(const QPointF &point) -> QString
{
    return QStringLiteral("%1,%2").arg(Number(point.x()), Number(point.y()));
}

/// @brief L'encre d'une pièce ENTIÈRE, telle qu'elle se dessine : contour, chemins internes, repères
/// de placement et ligne de miroir, en un seul « d ».
///
/// ⚠️ **Une pièce est un tout, et le payload doit la porter d'un bloc.** Le contour seul ne suffit
/// pas : les chemins internes, les place labels et le miroir sont dessinés par la pièce elle-même
/// (`VLayoutPiece::GetItem`), pas par le décor — et c'est précisément ce que le filtre du décor avait
/// fini par avaler, en les sortant de la pièce pour les mettre dans le tronc commun du fichier. On
/// reprend donc ici la construction d'Ostara, et on garde exactement ce qu'elle dessine comme
/// géométrie : la racine, puis chaque enfant qui n'est pas une information (le droit-fil, le pli et
/// sa légende, qui voyagent avec le décor parce que leur place se projette d'une taille à l'autre).
auto InkOf(const VLayoutPiece &piece) -> QString
{
    QGraphicsItem *item = piece.GetItem(false, true, false);
    if (item == nullptr)
    {
        return {};
    }

    QStringList parts;
    const auto add = [&parts](const QPainterPath &path)
    {
        const QString data = OSvg::PainterPathToSvgD(path);
        if (!data.isEmpty())
        {
            parts.append(data);
        }
    };

    if (const auto *root = qgraphicsitem_cast<QGraphicsPathItem *>(item); root != nullptr)
    {
        add(root->path());
    }
    for (QGraphicsItem *child : item->childItems())
    {
        if (OSvg::IsInfoChild(child))
        {
            continue;
        }
        if (const auto *path = qgraphicsitem_cast<QGraphicsPathItem *>(child); path != nullptr)
        {
            add(path->path());
        }
    }

    delete item;
    return parts.join(QLatin1Char(' '));
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPieces::Of(QDomDocument &doc, const QVector<OExportSizeLayer> &layers,
                       const QStringList &labels, const VContainer *pattern,
                       const QList<OGradedSvg::LayoutObject> &decor) -> QDomElement
{
    QDomElement pieces = doc.createElement(QStringLiteral("pieces"));
    // Version 4 — **chaque membre d'un groupe porte ses propres enfants.** Jusqu'ici son `<piece>`
    // restait sans `<decor>` et le `<group>` portait le seul jeu : un lecteur qui n'affichait qu'un
    // membre lui donnait le cartouche, les annotations et la droit-fil du groupe, dans le repère du
    // groupe. Version 3 dit la même chose SANS les enfants des membres, donc un lecteur de 4 qui
    // recevrait une 3 retomberait sur le groupe — le comportement d'avant, et c'est ce que la version
    // déclare.
    //
    // Les deux précédentes, pour mémoire : la 3 ajoute le contour DESSINÉ (crans) et l'offset
    // d'alignement à chaque pièce ; la 2 n'avait ni l'un ni l'autre, donc un lecteur ne pouvait
    // superposer les tailles qu'en devinant un décalage qu'il n'avait jamais reçu ; la 1 n'avait aucun
    // décor, et la prendre pour un payload dont les tailles n'ont rien à dessiner donnait dix pièces
    // et pas une ligne de droit-fil. **Un lecteur qui ne trouve pas ce qu'il demande le dit** — il ne
    // lit jamais la version la plus proche et ne la dessine.
    pieces.setAttribute(QStringLiteral("version"), QStringLiteral("4"));

    // The decor is looked up by the owner's ID, never by its name: two owners may share a name — the
    // visible layer hands out unique ids for exactly that reason — and a name-keyed table would give
    // one piece another's cartouche in silence. Two tables, because a piece and a group are two kinds
    // of owner with two different id spaces, and only the group has members to stand in for.
    QHash<quint32, const OGradedSvg::LayoutObject *> pieceDecorById;
    QHash<quint32, const OGradedSvg::LayoutObject *> groupDecorById;
    for (const OGradedSvg::LayoutObject &object : decor)
    {
        if (object.ownerId == 0)
        {
            continue;
        }
        if (object.isGroup)
        {
            groupDecorById.insert(object.ownerId, &object);
        }
        else
        {
            pieceDecorById.insert(object.ownerId, &object);
        }
    }

    for (int index = 0; index < layers.size(); ++index)
    {
        const OExportSizeLayer &layer = layers.at(index);
        // The name the drawing gave this layer, which is how a reader finds the piece again. The
        // layer's own label is the smaller answer and is only a fallback.
        const QString label = index < labels.size() ? labels.at(index) : layer.label;

        for (const OExportPieceGeometry &geometry : layer.pieces)
        {
            QDomElement piece = doc.createElement(QStringLiteral("piece"));

            // ⚠️ The key, and not the name alone: it is what the drawing's own `<g id="…">` carries,
            // and the only thing that ties this element to that group — a name repeats across size
            // layers, and a group's name repeats across groups.
            piece.setAttribute(QStringLiteral("key"), geometry.key);
            piece.setAttribute(QStringLiteral("name"), geometry.name);
            piece.setAttribute(QStringLiteral("size"), label);
            piece.setAttribute(QStringLiteral("id"), QString::number(geometry.id));
            if (geometry.groupId != 0)
            {
                piece.setAttribute(QStringLiteral("group"), QString::number(geometry.groupId));
            }

            // ── La forme de la pièce ────────────────────────────────────────────────────────────
            //
            // ⚠️ **In the PATTERN's frame, and not where the drawing put it.** A plan places the
            // pieces again, so a contour already moved onto a sheet is a plan already made — and,
            // worse, a contour turned by a placement no longer faces the grain angle written below
            // it. This is the same reading the nesting makes, of the same getter, so a reader that
            // rebuilds the plan starts from exactly the shape the engine was given.
            const QString outline = PathData(geometry.piece.GetMappedExternalContourPoints());
            if (!outline.isEmpty())
            {
                piece.setAttribute(QStringLiteral("outline"), outline);
            }

            // ── L'encre du contour ──────────────────────────────────────────────────────────────
            //
            // ⚠️ **Le contour DESSINÉ, qui n'est pas celui d'`outline`.** L'outline est la forme
            // extérieure SANS les crans, et elle doit le rester : un cran est une entaille, donc
            // l'union d'un contour entaillé donne une empreinte plus PETITE que la pièce qu'on coupe.
            // Mais c'est le contour AVEC les crans que le canvas dessine, pour la raison inverse —
            // un cran n'est pas un ornement, c'est le repère de montage, et une pièce qu'on ne peut
            // pas monter est une pièce ratée. Les deux formes sont donc écrites : celle qu'on nest,
            // celle qu'on voit. Sans la seconde, un lecteur ne peut dessiner que la moitié du
            // dessin, et rien ne le lui dit.
            const QString ink = InkOf(geometry.piece);
            // Written even when it comes out empty — a piece whose main path is hidden and that
            // carries no seam allowance draws nothing at all, and « nothing » is a fact the reader
            // has to be told. Left out, the same piece would be indistinguishable from a payload
            // that never carried the ink.
            piece.setAttribute(QStringLiteral("outlineInk"), ink);

            // ── L'écart d'alignement ────────────────────────────────────────────────────────────
            //
            // ⚠️ **Une traduction, et rien d'autre.** Les tailles d'une pièce ne se superposent pas
            // dans le patron — l'une grandit depuis son centre, l'autre est épinglée à son pli — et
            // c'est cet écart, calculé une fois pour toutes par `OExportAlignment`, qui les fait
            // coïncider. Le contour ci-dessus est donc écrit DÉCALÉ PAR RIEN, et l'écart à part :
            // un lecteur qui l'ignore n'obtient pas une empreinte un peu fausse, il obtient une
            // empreinte trop grande — vingt-neuf feuilles là où le patron en demande vingt, mesuré.
            // Toujours écrit, même nul : « aligné de zéro » est un fait, et une valeur absente
            // obligerait le lecteur à connaître une valeur par défaut.
            piece.setAttribute(QStringLiteral("alignDelta"), PointData(geometry.alignDelta));

            // ── La direction ────────────────────────────────────────────────────────────────────
            //
            // Resolved here when the generation did not keep one: a print run never asks for a
            // direction, so the field is left empty and the piece still HAS one. Reading it here
            // rather than at nesting time costs one pure function, and it is the same reading the
            // cutting plan makes — `PieceDirection`, never a second opinion on the fold.
            auto direction = geometry.direction;
            if (!direction.known)
            {
                direction = OExportCuttingDirection::PieceDirection(geometry.piece);
                if (direction.known && geometry.mirrored)
                {
                    // A reflection about a vertical axis sends an angle `a` to `180 − a`, and it
                    // lives in the matrix — which leaves the stored grainline untouched.
                    direction = OExportCuttingDirection::Mirrored(direction);
                }
            }
            if (direction.known)
            {
                piece.setAttribute(QStringLiteral("grain"), Number(direction.angle));
                // The arrow type is the whole of « the same way round »: it says whether the piece
                // has one axis or two, so it says what that option is allowed to keep.
                piece.setAttribute(QStringLiteral("arrows"),
                                    QString::number(static_cast<int>(direction.arrows)));
            }

            // ── La ligne de droit-fil ──────────────────────────────────────────────────────────
            //
            // The ANGLE above is what a plan READS; the line is what a plan DRAWS. Written in the
            // same frame as the outline — mapped, so a piece that carries a mirror has its line
            // where the piece actually is — because a segment in any other frame would sit beside
            // the piece the moment the plan lays it somewhere else.
            QLineF grainline;
            if (geometry.grainline.resolved)
            {
                // The frozen line wins: it was resolved while the document was still on the size
                // that matters, and the live piece would answer for whatever size it is on now.
                grainline = geometry.grainline.mainLine;
            }
            else if (geometry.piece.GetGrainline().IsEnabled())
            {
                grainline = geometry.piece.GetGrainline().GetMainLine();
            }
            if (!grainline.isNull())
            {
                piece.setAttribute(QStringLiteral("grainLine"),
                                    LineData(geometry.piece.GetMatrix().map(grainline)));
            }

            // ── Le pli ──────────────────────────────────────────────────────────────────────────
            const QLineF fold = OExportCuttingDirection::MappedFoldLine(geometry.piece);
            if (!fold.isNull())
            {
                piece.setAttribute(QStringLiteral("onFold"), QStringLiteral("yes"));
                // The axis, and never the pair of arrows the drawing carries: this is the edge that
                // goes on the fold of the cloth.
                piece.setAttribute(QStringLiteral("foldAxis"), LineData(fold));

                const bool storedUnfolded = geometry.piece.IsShowFullPiece();
                piece.setAttribute(QStringLiteral("stored"), storedUnfolded
                                                                  ? QStringLiteral("unfolded")
                                                                  : QStringLiteral("folded"));

                // ⚠️ **The form the drawing does not carry.** A half piece is a closed contour, so
                // nothing tells it apart from a whole one — and a plan laid from the wrong one is
                // half a garment that looks finished. Flipping the piece's own flag and reading its
                // contour again is the mirror-and-union, done here, once: a reader never does it.
                VLayoutPiece other = geometry.piece;
                other.SetShowFullPiece(!storedUnfolded);
                const QString outline = PathData(other.GetMappedExternalContourPoints());
                if (!outline.isEmpty())
                {
                    piece.setAttribute(QStringLiteral("outlineAlt"), outline);
                }
                // Et son encre : un plan de coupe DESSINE la forme qu'il pose, et il la pose dans
                // l'autre forme dès que le tissu le demande — la moitié sur du tissu plié, la pièce
                // entière sur du tissu à plat. Sans cette encre, le plan n'aurait que le contour nu :
                // pas de crans, pas de ligne de couture, et une pièce qu'on ne peut pas monter.
                piece.setAttribute(QStringLiteral("outlineInkAlt"), InkOf(other));
            }

            // ── Les quantités ───────────────────────────────────────────────────────────────────
            //
            // Read on the piece, then on its group, and 0 means « not cut in that cloth at all » —
            // a figure the cartouche holds and the drawing never does. Left out when the pattern is
            // not at hand: a missing figure is a fact, a guessed one is not.
            if (pattern != nullptr && geometry.id != 0)
            {
                piece.setAttribute(
                    QStringLiteral("cutFabric"),
                    QString::number(OExportCuttingGuard::CutQuantity(
                        geometry.id, pattern, OExportCuttingGuard::Material::Fabric)));
                piece.setAttribute(
                    QStringLiteral("cutLining"),
                    QString::number(OExportCuttingGuard::CutQuantity(
                        geometry.id, pattern, OExportCuttingGuard::Material::Lining)));
            }

            // ── Le décor, recopié ───────────────────────────────────────────────────────────────
            //
            // The very ink the visible decor layer draws — same serializer, other frame — copied
            // into this record. Ten sizes give ten copies of ONE resolved drawing, and that is
            // deliberate: the copy is what lets a reader find the children of the size it was asked
            // for and place them without computing anything.
            //
            // **Un membre de groupe porte les SIENS, désormais.**
            //
            // Le canvas les construit pour lui sans les dessiner (`OExportCanvasContent::payloadDecor`),
            // et le `<group>` plus bas porte en plus le jeu commun. C'est le LECTEUR qui choisit : le
            // groupe s'il dessine le groupe entier, le membre s'il n'en affiche qu'un. Sans les deux, un
            // membre seul recevait le cartouche et la droit-fil du groupe, dans le repère du groupe.
            const auto entry = pieceDecorById.constFind(geometry.id);
            if (entry != pieceDecorById.cend() && entry.value() != nullptr)
            {
                // ── Le bloc est le RENDU DE RÉFÉRENCE, et il ne bouge pas d'une taille à l'autre ──
                //
                // ⚠️ **Aucune projection.** Le décor d'un size range est placé UNE fois, en Graded,
                // de façon à valoir pour n'importe quelle taille de la plage ; seule la plus petite
                // l'affiche, et les autres en portent une copie IDENTIQUE. C'est le contrat du
                // payload (`000_Rabnest.md:131` : « géométrie des pièces, par taille, et rendu de
                // référence […] tout ce que Rabnest ne recalcule pas sort identique — pas de
                // dérive »), et c'est ce que Rabnest fait : il pose le bloc avec la transformation
                // de la pièce, sans rien recalculer.
                //
                // ⚠️ **Ce qui était là décalait le bloc par la différence des CENTRES** entre la
                // boîte de la taille de référence et celle de la taille courante (`CentreSeat`), pour
                // « suivre la pièce ». Cette différence n'est pas la croissance de la pièce : mesuré
                // sur un vrai run, la plus petite taille était juste et les suivantes glissaient, un
                // peu plus à chacune, jusqu'à poser le cartouche hors de la pièce. Le pli avait même
                // son propre rattrapage (`FoldSeatShift`) pour ne pas suivre ce décalage — deux
                // mécanismes pour compenser une translation qui n'a pas lieu d'être.
                //
                // Le bloc est donc écrit tel qu'il a été résolu : mêmes octets pour toutes les tailles.
                const QDomElement ink = OGradedSvg::OwnerDecorFor(doc, *entry.value());
                if (!ink.isNull())
                {
                    piece.appendChild(ink);
                }
            }

            pieces.appendChild(piece);
        }
    }

    // ── Le décor des groupes ────────────────────────────────────────────────────────────────────
    //
    // Written beside the pieces, once per group, because a grouped member draws nothing of its own:
    // the group hosts the grain line, the fold and the cartouche for all of its members. A reader
    // reads a member's own `<decor>` when it has one and the group's otherwise — the same rule the
    // drawing applies, stated once here so nobody has to re-derive it.
    if (pattern != nullptr)
    {
        if (const auto *groups = pattern->DataPieceGroups(); groups != nullptr)
        {
            for (auto it = groups->cbegin(); it != groups->cend(); ++it)
            {
                const auto entry = groupDecorById.constFind(it.key());
                if (entry == groupDecorById.cend() || entry.value() == nullptr)
                {
                    continue;
                }
                const QDomElement ink = OGradedSvg::OwnerDecorFor(doc, *entry.value());
                if (ink.isNull())
                {
                    continue;
                }
                QDomElement group = doc.createElement(QStringLiteral("group"));
                group.setAttribute(QStringLiteral("id"), QString::number(it.key()));
                group.appendChild(ink);
                pieces.appendChild(group);
            }
        }
    }

    return pieces;
}
