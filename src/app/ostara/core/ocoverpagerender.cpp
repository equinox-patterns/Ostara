/************************************************************************
 **
 **  @file   ocoverpagerender.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  A cover page, resolved and painted — the ONE pipeline.
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

#include "ocoverpagerender.h"

#include "ocoverpagecatalogue.h"
#include "ocoverpagelayout.h"

#include "vwidgets/ocartouchepainter.h"

#include <QColor>
#include <QPainter>

namespace
{
/// Les quatre bandes de la page, dans l'ordre où elles se lisent.
///
/// Écrit ici plutôt que demandé au modèle : ce sont les quatre emplacements de
/// `OCoverPageTemplate::Placement`, et une page les a tous — vides ou non.
constexpr OCoverPageTemplate::Placement kPlacements[] = {
    OCoverPageTemplate::Placement::Top,
    OCoverPageTemplate::Placement::MiddleLeft,
    OCoverPageTemplate::Placement::MiddleRight,
    OCoverPageTemplate::Placement::Bottom,
};

/// **Rien ne dépasse de la page.** Un texte plus large que la bande où il se tient est replié sur
/// autant de lignes qu'il en faut — une page a des bords, et du contenu qui les franchit n'est plus
/// du contenu. Un logo et une forme se mesurent eux-mêmes plutôt que la page, et sont laissés
/// tranquilles.
auto FittedToWidth(const QVector<OCartoucheRenderLine> &drawn, qreal width)
    -> QVector<OCartoucheRenderLine>
{
    QVector<OCartoucheRenderLine> fitted;
    for (const OCartoucheRenderLine &line : drawn)
    {
        if (line.kind != OCartoucheRenderLine::Kind::Text)
        {
            fitted.append(line);
            continue;
        }

        // **Ce que les marques prennent n'est pas de la place que les mots peuvent avoir.** Une
        // ligne de légende, ce sont ses marques et son mot ; replier le mot sur toute la bande le
        // laisserait courir sous elles.
        const qreal marksWidth = line.marks.IsEmpty()
                                     ? 0.0
                                     : line.marks.size.width() + OCartouchePainter::kMarksGap;
        QVector<OCartoucheRenderLine> folded = OCartouchePainter::TextLinesFitting(
            line.text, line.font, line.lineSpacing, width - marksWidth);

        // **L'échantillon appartient à la taille, pas à la ligne où son mot a atterri.** Une taille
        // dont le nom a pris deux lignes porte son tracé une fois, après la dernière. Les marques
        // sont la même sorte de chose : elles tiennent pour le symbole, pas pour la ligne sur
        // laquelle le mot s'est cassé.
        if (line.trace.HasTrace() && !folded.isEmpty())
        {
            folded.last().trace = line.trace;
        }
        if (!line.marks.IsEmpty() && !folded.isEmpty())
        {
            folded.last().marks = line.marks;
        }

        fitted.append(folded);
    }
    return fitted;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OCoverPageRender::Bands(const OCoverPageTemplate &tpl, const DataForSection &data,
                             const QFont &baseFont, const OSizeTable *table, bool substituteEmpty)
    -> QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>>
{
    QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> bands;

    for (const OCoverPageTemplate::Placement placement : kPlacements)
    {
        QVector<OCartoucheRenderLine> lines;

        for (const OCoverPageTemplate::Section &section : tpl.Sections(placement))
        {
            // Ce qu'une section lit est la réponse de l'APPELANT, et elle est demandée PAR SECTION :
            // les axes qu'une information montre vivent dans ses propres options.
            const OCoverPageResolver::PatternData values = data(section);

            QVector<OCartoucheRenderLine> drawn =
                OCoverPageResolver::Resolve(section, values, baseFont, tpl.languages, table);

            if (drawn.isEmpty() && substituteEmpty)
            {
                // Rien à dessiner — un logo que personne n'a choisi, un texte laissé à remplir, un
                // nom que le patron n'a pas donné. La page elle-même ne dessine rien là ; ici
                // l'information se tient pour elle-même sous son propre nom, comme la preview du
                // bloc d'informations montre ses sections. **Comportement de preview, et de
                // preview seule** — voir le §6 de la nomenclature.
                const OTemplateSection &type = OCoverPageCatalogue::Section(section.typeId);
                const QFont font = OCartouchePainter::SectionFont(section.options, baseFont);
                const int lineSpacing =
                    section.options.value(QStringLiteral("lineSpacing"), 0).toInt();
                drawn.append(OCartouchePainter::TextLine(
                    type.IsValid() ? type.DisplayName() : section.typeId, font, lineSpacing));
            }

            lines.append(FittedToWidth(drawn, OCoverPageLayout::ContentWidth(placement)));
        }

        bands.insert(placement, lines);
    }

    return bands;
}

//---------------------------------------------------------------------------------------------------------------------
void OCoverPageRender::Paint(
    QPainter &painter, const QRectF &page,
    const QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> &bands, qreal scale)
{
    // L'encre de la page pour ses mots. **Les lignes ne portent pas de couleur — sauf les
    // échantillons de tracé**, dessinés dans la couleur de la taille qu'ils représentent : cette
    // couleur est toute la raison d'être de l'échantillon, et le peintre l'emprunte puis la rend
    // autour de chacun.
    painter.setPen(QColor());

    const QMap<OCoverPageTemplate::Placement, QRectF> cells = OCoverPageLayout::Place(bands);
    for (auto cell = cells.constBegin(); cell != cells.constEnd(); ++cell)
    {
        const QVector<OCartoucheRenderLine> lines = bands.value(cell.key());
        if (lines.isEmpty())
        {
            continue;
        }

        // Les rectangles sortent en coordonnées de PAGE, origine à son coin haut-gauche : c'est
        // l'échelle appliquée ici qui les amène à ce qui est affiché, jamais une mise en page
        // refaite à une autre taille.
        const QRectF target(page.left() + cell.value().left() * scale,
                            page.top() + cell.value().top() * scale,
                            cell.value().width() * scale,
                            cell.value().height() * scale);

        // ⚠️ **Une échelle de 1 est la page imprimée, et passe les lignes telles quelles.**
        // `ScaledLines(lines, 1.0)` n'est PAS l'identité : elle arrondit chaque taille de police en
        // pixels, donc une page dessinée à 1 différerait de la même page dessinée à 0,999 — un
        // écart d'un pixel, silencieux, entre la preview et le papier.
        if (qFuzzyCompare(scale, 1.0))
        {
            OCartouchePainter::Paint(painter, target, lines, 1.0);
            continue;
        }

        // Les polices sont amenées à l'échelle avant que les lignes soient posées : toute la page
        // est montrée à une fraction de sa taille, donc tout y est dessiné à cette fraction — les
        // mots autant qu'un logo, et autant que la géométrie d'un carré test. Peindre le texte à sa
        // taille d'impression dans une page réduite le rendrait aussi trop grand que la page est
        // réduite, et un carré de cinq centimètres serait tracé autour d'une étiquette qui n'y
        // tient plus.
        OCartouchePainter::Paint(painter, target,
                                 OCartouchePainter::ScaledLines(lines, scale), scale);
    }
}
