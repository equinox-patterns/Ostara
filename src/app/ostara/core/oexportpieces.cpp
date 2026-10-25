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
    // Version 5 — **le décor appartient à UNE taille.** Chaque `<piece>` porte les enfants de SA
    // taille, tels que Graded les construit pour elle : le droit-fil, le pli et sa légende, le
    // cartouche et les annotations, tous calculés à cette taille-là. Les versions précédentes
    // recopiaient un seul jeu — celui de la référence — dans chaque `<piece>` : un lecteur qui
    // dessinait une autre taille recevait le décor d'une autre pièce, et c'est ce que ce numéro
    // annonce. `<pieces reference="…">` nomme la taille dont le décor sert de repli, parce qu'un
    // décor manquant doit se lire dans le fichier et non dans l'ordre des éléments.
    //
    // Les précédentes, pour mémoire : la 4 donne à chaque membre d'un groupe ses propres enfants (la
    // 3 les lui refusait et un membre seul portait le décor du groupe) ; la 3 ajoute le contour DESSINÉ
    // (crans) et l'offset d'alignement à chaque pièce ; la 2 n'avait ni l'un ni l'autre, donc un lecteur
    // ne pouvait superposer les tailles qu'en devinant un décalage qu'il n'avait jamais reçu ; la 1
    // n'avait aucun décor. **Un lecteur qui ne trouve pas ce qu'il demande le dit** — il ne lit jamais
    // la version la plus proche et ne la dessine.
    // ── Version 6 : UN décor, celui de la référence, pour toutes les tailles ─────────────────────
    //
    // La 5 écrivait un décor par taille, résolu sur la pièce de cette taille. Le canvas, lui, n'en
    // dessine qu'un — sur sa couche de référence — et superpose les tailles : le cartouche occupe
    // donc UNE place, et sa fraction dans la pièce VARIE quand la pièce grandit (mesuré sur le SVG
    // graded : 0.853 à la 32, 0.752 à la 46 pour le droit-fil de « Manche »). La 5 gardait la
    // fraction constante, ce qui décalait tout de 5 cm à la 46.
    //
    // Le lecteur sait quoi faire de la 6 : il applique l'écart d'alignement à la PIÈCE et pas au
    // décor, ce qui laisse le décor à sa place. Il ne devine rien : la version le dit.
    pieces.setAttribute(QStringLiteral("version"), QStringLiteral("6"));
    if (!layers.isEmpty())
    {
        pieces.setAttribute(QStringLiteral("reference"),
                            !labels.isEmpty() ? labels.first() : layers.first().label);
    }

    // The decor is looked up by the owner's ID, never by its name: two owners may share a name — the
    // visible layer hands out unique ids for exactly that reason — and a name-keyed table would give
    // one piece another's cartouche in silence. Two tables, because a piece and a group are two kinds
    // of owner with two different id spaces, and only the group has members to stand in for.
    //
    // ⚠️ **And by LAYER, for a piece.** The decor of a size is that size's, and a table keyed by the
    // owner alone would hand the reference's to every other size — which is exactly the defect
    // version 5 exists to end.
    QHash<QPair<int, quint32>, const OGradedSvg::LayoutObject *> pieceDecorByLayerAndId;
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
            pieceDecorByLayerAndId.insert({object.layerIndex, object.ownerId}, &object);
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
            // ── La ligne de droit-fil ──────────────────────────────────────────────────────────
            //
            // The ANGLE below is what a plan READS; the line is what a plan DRAWS. Written in the
            // same frame as the outline — mapped, so a piece that carries a mirror has its line
            // where the piece actually is — because a segment in any other frame would sit beside
            // the piece the moment the plan lays it somewhere else.
            //
            // ⚠️ **The line is read FIRST, because the angle is taken FROM it.** `PieceDirection`
            // answers on the piece's STORED grainline, which no matrix has touched; the line written
            // here is mapped. Mirroring the stored angle afterwards therefore described a piece that
            // the file does not draw: on « Parementure manche » (the one piece of the sample whose
            // grainline is not at 90°) the payload carried `grain="135.000"` beside
            // `grainLine` at **45°** and a drawn grainline at **45°** — its own mirror image, and
            // two facts out of three agreed. A plan that pinned the piece on the angle would have
            // turned it by 90° across the grain. One reading, one source: when the piece has a line,
            // the angle IS that line's direction.
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
            const QLineF mappedGrainline =
                grainline.isNull() ? QLineF() : geometry.piece.GetMatrix().map(grainline);

            auto direction = geometry.direction;
            if (!direction.known)
            {
                direction = OExportCuttingDirection::PieceDirection(geometry.piece);
                if (direction.known && geometry.mirrored)
                {
                    // A reflection about a vertical axis sends an angle `a` to `180 − a`, and it
                    // lives in the matrix — which leaves the stored grainline untouched. Only the
                    // piece with NO grainline needs it: the one that has a line takes its angle from
                    // the mapped line, mirror included, a few lines below.
                    direction = OExportCuttingDirection::Mirrored(direction);
                }
            }
            if (!mappedGrainline.isNull() || direction.known)
            {
                if (mappedGrainline.isNull())
                {
                    piece.setAttribute(QStringLiteral("grain"), Number(direction.angle));
                }
                else
                {
                    // An axis, not a direction: its two ends have no order, so half a turn is the
                    // same orientation and the plan must not see `-135` where the pattern means `45`.
                    qreal alignment = std::fmod(mappedGrainline.angle(), 180.0);
                    if (alignment < 0.0)
                    {
                        alignment += 180.0;
                    }
                    piece.setAttribute(QStringLiteral("grain"), Number(alignment));
                }
                // The arrow type is the whole of « the same way round »: it says whether the piece
                // has one axis or two, so it says what that option is allowed to keep.
                if (direction.known)
                {
                    piece.setAttribute(QStringLiteral("arrows"),
                                        QString::number(static_cast<int>(direction.arrows)));
                }
            }
            if (!mappedGrainline.isNull())
            {
                piece.setAttribute(QStringLiteral("grainLine"), LineData(mappedGrainline));
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

            // ── Le décor de cette taille ────────────────────────────────────────────────────────
            //
            // The very ink the visible decor layer draws — same serializer, other frame — in THIS
            // size's own record. Une taille, un dessin : c'est ce que la version 5 énonce, et c'est
            // ce qui fait qu'un plan de coupe de la 46 ne porte plus le décor de la 32.
            //
            // **Un membre de groupe porte les SIENS, désormais.**
            //
            // Le canvas les construit pour lui sans les dessiner (`OExportCanvasContent::payloadDecor`),
            // et le `<group>` plus bas porte en plus le jeu commun. C'est le LECTEUR qui choisit : le
            // groupe s'il dessine le groupe entier, le membre s'il n'en affiche qu'un. Sans les deux, un
            // membre seul recevait le cartouche et la droit-fil du groupe, dans le repère du groupe.
            // ── UN décor, celui de la RÉFÉRENCE, pour toutes les tailles ─────────────────────────
            //
            // ⚠️ **C'est la règle de Graded, et la seule qui reproduise ce qu'on voit.** Le canvas
            // dessine le décor UNE fois — ses enfants sont masqués sur toutes les couches sauf la
            // référence — et les tailles sont superposées : un cartouche occupe donc UNE place, la
            // même pour tout le monde. Un décor résolu PAR TAILLE, comme le faisait la version 5,
            // suivait la fraction de la pièce : mesuré contre le SVG graded, « Manche » passait de
            // 0.796 à 0.805 là où le canvas est à 0.957, et l'écart atteignait 5 cm à la taille 46.
            //
            // Ce que le lecteur reçoit est donc le dessin de la référence, écrit dans le repère des
            // `outline` : il applique l'écart d'alignement aux deux, et ne recalcule plus rien.
            const auto referenceEntry = pieceDecorByLayerAndId.constFind({0, geometry.id});
            const OGradedSvg::LayoutObject *pieceDecor = nullptr;
            if (referenceEntry != pieceDecorByLayerAndId.cend()
                && referenceEntry.value() != nullptr)
            {
                pieceDecor = referenceEntry.value();
            }
            if (pieceDecor != nullptr)
            {
                // ── Le décor de CETTE taille ───────────────────────────────────────────────────
                //
                // ⚠️ **Chaque taille porte le sien, et c'est ce que la version 5 déclare.** Graded
                // ne projette pas un décor d'une taille sur une autre : il construit les enfants de
                // chaque pièce À la taille de sa couche — droit-fil, pli et sa légende, cartouche,
                // annotations — puis masque ceux des couches qui ne sont pas la référence. Le payload
                // recopiait, lui, le jeu de la référence dans tous les `<piece>` : un lecteur qui
                // dessinait une autre taille posait donc le décor d'une autre pièce. Mesuré côté
                // Rabnest, l'écart allait jusqu'à 227 px en x et 42 px en y sur les pièces sans pli,
                // et il suivait la gradation — aucune translation ne pouvait le corriger.
                //
                // Le bloc est écrit tel qu'il a été résolu POUR CETTE TAILLE, dans le même repère que
                // son `outline` : le lecteur applique l'écart d'alignement aux deux, et ne recalcule
                // rien d'autre.
                const QDomElement ink = OGradedSvg::OwnerDecorFor(doc, *pieceDecor);
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
    //
    // ⚠️ **UN bloc de groupe, et il appartient à la RÉFÉRENCE** — c'est un dessin, pas une taille :
    // celui de la sortie qui reproduit le document tel qu'il se lit, où toutes les tailles sont
    // superposées et où un cartouche suffit. Le lecteur du plan de coupe ne s'en sert d'ailleurs
    // jamais : là, chaque membre est une pièce à poser, et il porte SON décor.
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
