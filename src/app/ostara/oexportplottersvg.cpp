/************************************************************************
 **
 **  @file   oexportplottersvg.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The file a domestic plotter cuts: one object per part, no bac,
 **          no grid, and not one word.
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

#include "oexportplottersvg.h"

#include "oexportcanvas.h"
#include "ogradedsvgwriter.h"

#include <QGraphicsScene>
#include <QList>
#include <QMarginsF>
#include <QRectF>

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterSvg::Write(const QString &path, const OExportOutput &output,
                              const VContainer *pattern, VAbstractPattern *doc) -> bool
{
    if (path.isEmpty() || !output.result.ok)
    {
        return false;
    }

    // ── Dessiner, puis reprendre ce qui a été dessiné ────────────────────────────────────────────
    //
    // La même scène jetable que le chemin PDF, et pour la même raison : c'est `Build` qui décide de
    // ce qu'un run montre, et il n'y a **qu'un seul endroit** qui en décide. Ce que l'écriture reçoit
    // ensuite est le CONTENU — et c'est ce qui fait que la scène peut porter le bac et la grille sans
    // que le fichier les porte.
    //
    // Ni grille, ni repères, ni cadre, ni page de garde : les quatre se coupent en les nommant, et
    // aucun n'a de sens sur un tapis de découpe.
    QGraphicsScene scene;
    OExportCanvasContent content;
    OExportCanvas::Build(&scene, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc, &content,
                         {}, nullptr, false, nullptr,
                         {output.format, output.foldedFabric, output.matWidthMm,
                          output.matHeightMm, output.result.matCount});

    if (content.layers.isEmpty())
    {
        return false;
    }

    // Le contenu en couches d'écriture, la référence en dernier pour qu'elle finisse au-dessus.
    QList<OGradedSvg::SizeLayer> layers;
    layers.reserve(content.layers.size());
    for (int i = static_cast<int>(content.layers.size()) - 1; i >= 0; --i)
    {
        const OExportCanvasContent::Layer &layer = content.layers.at(i);

        OGradedSvg::SizeLayer sizeLayer;
        sizeLayer.label = layer.displayName.isEmpty() ? layer.label : layer.displayName;

        for (const OExportCanvasContent::Object &object : layer.objects)
        {
            OGradedSvg::GradedObject entry;
            entry.name = object.name;
            entry.isGroup = object.isGroup;

            if (object.isGroup)
            {
                for (const auto &member : object.members)
                {
                    entry.members.append({member.first, member.second});
                }
            }
            else
            {
                entry.loneItem = object.loneItem;
            }

            sizeLayer.objects.append(entry);
        }

        layers.append(sizeLayer);
    }

    // ── La taille du plan ────────────────────────────────────────────────────────────────────────
    //
    // La grille des tapis, en pixels. L'écrivain en fait des millimètres tout seul (96 dpi), et
    // c'est ce qui fait qu'une ligne de coupe tombe sur un multiple **exact** du tapis dans la vraie
    // vie. C'est aussi pourquoi aucune taille cible ni aucune résolution ne sont offertes : les
    // offrir casserait cela, et couperait au mauvais endroit sans le dire.
    const QRectF imageRect(0, 0, output.result.binWidthPx, output.result.usedHeightPx);

    // ── Le décor : rien du tout ──────────────────────────────────────────────────────────────────
    //
    // ⚠️ **Vide, et surtout pas `content.decor`.**
    //
    // Un fichier de découpe n'a rien à porter de texte : ni cartouche, ni annotation, ni « Cut On
    // Fold ». Les morceaux n'en ont aucun de toute façon — ils n'ont plus d'identité de pièce du
    // modèle, donc plus de cartouche à résoudre — mais un décor écrit reste un décor écrit.
    //
    // Et la grainline d'un morceau n'est pas perdue pour autant : elle est un tracé **interne** de
    // sa pièce, écrit avec elle. Passer le décor la ferait écrire DEUX fois — une fois avec la
    // pièce, une fois dans la couche de décor — et c'est exactement le piège que la revue avait
    // signalé sur les tracés internes.
    const QList<OGradedSvg::LayoutObject> noDecor;

    // `layered = false` : un run plotter est en taille unique, il n'y a rien entre quoi basculer —
    // et c'est aussi ce qui n'écrit aucun `<title>` de couche.
    return OGradedSvg::WriteGradedSvg(path, layers, QString(), noDecor, imageRect, QMarginsF(), 1.0,
                                      1.0, QString(), QString(), false);
}
