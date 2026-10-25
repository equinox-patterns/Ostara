/************************************************************************
 **
 **  @file   oexportpdf.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the PDF export of a generation.
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

#include "oexportpdf.h"

#include "oexportartboards.h"
#include "oexportcanvas.h"
#include "oexportcoverpage.h"
#include "oexportcoverpagesvg.h"
#include "oexportsheets.h"
#include "oexportunderlay.h"
#include "ogradedsvgwriter.h"

#include "core/oexportoutput.h"
#include "core/oexportpieces.h"
#include "core/oexportrecipe.h"
#include "core/oexporttemplate.h"

#include "../vmisc/projectversion.h"
#include "../vpatterndb/vcontainer.h"

#include "pdf/SvgToOcgPdf.h"  // Trame — writes the optional content groups

#include <QCoreApplication>
#include <QDomDocument>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QTemporaryDir>
#include <QVector>

namespace
{
/// Puts the cover page INSIDE the payload, as its own element.
///
/// The cover page is drawn outside the plan's layers, so a reader of the payload cannot reconstruct
/// it: the drawing is the only truth there is. Rabnest has to deliver the same first page — a plan
/// delivered without its nomenclature looks perfectly normal and is not — so the page travels with
/// the plan, as a document of its own, and Rabnest writes it back as the file's first sheet.
///
/// The SVG text is carried as TEXT and not as markup: the payload is one XML document, and a second
/// root inside it would be a document that no longer parses.
auto CarryCover(const QString &svgPath, const QString &coverPath) -> bool
{
    QFile coverFile(coverPath);
    if (!coverFile.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QByteArray cover = coverFile.readAll();
    coverFile.close();

    QFile svgFile(svgPath);
    if (!svgFile.open(QIODevice::ReadOnly))
    {
        return false;
    }
    QDomDocument document;
    const bool parsed = static_cast<bool>(document.setContent(svgFile.readAll()));
    svgFile.close();
    if (!parsed)
    {
        return false;
    }

    QDomElement root = document.documentElement();
    QDomElement metadata = root.firstChildElement(QStringLiteral("metadata"));
    if (metadata.isNull())
    {
        metadata = document.createElement(QStringLiteral("metadata"));
        root.insertBefore(metadata, root.firstChild()).isNull();
    }

    QDomElement element = document.createElement(QStringLiteral("exportCover"));
    element.appendChild(document.createTextNode(QString::fromUtf8(cover)));
    metadata.appendChild(element);

    QFile out(svgPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    out.write(document.toByteArray());
    return true;
}


/// @brief The decor layer's name when the range has none of its own.
///
/// Filed under MainWindow's context, and deliberately so: that is where the graded export
/// declares this same string, and therefore where its translation lives — the French UI
/// reads "Mise en page" from there. Asked under a context of our own, the lookup finds
/// nothing and the layer comes out under the English source string, which is precisely what
/// it did. One string and one context, until the two call sites are given a shared home;
/// lupdate merges them anyway, since context, source and disambiguation are all equal.
auto DefaultDecorLayerName() -> QString
{
    return QCoreApplication::translate("MainWindow", "Layout",
                                       "Decor layout layer default name in a graded SVG export");
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdf::Write(const OExportOutput &output, const QString &pdfPath,
                       const VContainer *pattern, VAbstractPattern *doc,
                       const OExportTemplate *marks, bool framed, bool addLayoutLayer,
                       const OExportCoverPage::Drawing *cover, bool rabnestPayload,
                       const QString &gradedDrawing) -> bool
{
    if (pattern == nullptr || doc == nullptr || output.layers.isEmpty() || !output.result.ok)
    {
        qWarning() << "OExportPdf: the generation cannot be reproduced";
        return false;
    }

    // Draw the generation the way the canvas draws it, and take back what was drawn. The
    // scene is a local: everything the writer needs points into it, and it dies with this
    // function, once every SVG has been written.
    QGraphicsScene scene;
    OExportCanvasContent content;
    OExportCanvas::Build(&scene, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc, &content,
                         {}, nullptr, false, nullptr,
                         {output.format, output.foldedFabric, output.matWidthMm,
                          output.matHeightMm, output.result.matCount});

    // The page is the bin when the generation has no sheets to cut — a roll, the A0 sheet,
    // anything the print model left untuiled — and that is the whole of this export, left
    // exactly as it was.
    //
    // When the run IS tiled the plan is cut into its artboards, and each artboard becomes
    // a page. The grid is the SCREEN's: the same Compute() the canvas calls, so that the
    // paper cannot disagree with what is drawn, and frameHeightMm > 0 — the print model's
    // own token — is the only thing that decides it. Nothing is asked of the user, because
    // there is nothing left to ask: she already said what she prints on.
    const OExportArtboards::Grid artboards = OExportArtboards::Compute(
        output.result.binWidthPx, output.result.usedHeightPx, output.frameHeightMm,
        output.columns);

    // Where the cover page stands, read from the FORMAT and nowhere else — the canvas asks the same
    // call the same question, so the two cannot show the page in different places. A generation with
    // no cover page answers Seat::None, and none of what follows runs.
    const OExportCoverPage::Seat coverSeat =
        cover != nullptr ? cover->seat : OExportCoverPage::Seat::None;
    // The margin travels with the page, and with the run's own: a reserved block is placed with it.
    const qreal coverMarginPx = cover != nullptr ? cover->marginPx : 0.0;

    // **The reserved block is ink for the one question OExportSheets asks** — which sheets carry
    // something — and for no other. A plan that happens to keep every piece out of cell (0,0) must
    // still write the sheet its own cover page is drawn on, or the page would be delivered on paper
    // the file does not have. The extra sheet is deliberately NOT passed: it is not a cell, and
    // counting it here would move the plan's own ranks.
    const QRectF reserved =
        (coverSeat == OExportCoverPage::Seat::InSheet && artboards.valid)
            ? OExportCoverPage::PaperPx(coverSeat, coverMarginPx, artboards.planRect,
                                        artboards.cellWidthPx, artboards.cellHeightPx)
            : QRectF();

    // Which of those sheets is worth a page, read from the placements the run produced and any
    // hand has moved since — the generation's own, never a copy of them. The canvas asks the same
    // question of the same three things, so the paper on screen and the paper in the file are one
    // decision: a sheet a hand has emptied is not written, and a piece put back on a dropped one
    // brings its page back with it.
    const OExportSheets::Set sheets =
        OExportSheets::Compute(artboards, output.result.placements, reserved);

    // The cover page's own sheet, for the formats that give it one: **the sheet the file opens
    // on**, and Trame writes it as a page GRAFTED onto the document rather than as a window onto
    // the plan. Only the rectangle crosses — a cover page's sheet holds the cover page and none of
    // the plan, which is what grafting means and what keeps the plan's own frame, whose line runs
    // along that boundary, off it.
    //
    // Read from the SEAT and never from the page's own figure: the seat is the format's property,
    // and asking a second time would be a second place deciding where the page stands.
    const QRectF coverSheet =
        (coverSeat == OExportCoverPage::Seat::ExtraSheet && artboards.valid)
            ? OExportCoverPage::PaperPx(coverSeat, coverMarginPx, artboards.planRect,
                                        artboards.cellWidthPx, artboards.cellHeightPx)
            : QRectF();

    QVector<QRectF> pages;
    if (artboards.valid)
    {
        pages.reserve(sheets.count);
        // Row by row, left to right — the order the sheets come off the printer in and are taped
        // back together in. A sheet with nothing on it is passed over: paper, ink and a hand to
        // tape it are what a page costs, and the nesting's short last row would otherwise be paid
        // for in full. What decides is the content and never the grid — a blank sheet that a hand
        // has since put a piece on is a real sheet again, and is written as one.
        for (int row = 0; row < artboards.rows; ++row)
        {
            for (int col = 0; col < artboards.columns; ++col)
            {
                if (!sheets.IsPresent(row, col))
                {
                    continue;
                }

                pages.append(QRectF(artboards.planRect.left() + col * artboards.cellWidthPx,
                                    artboards.planRect.top() + row * artboards.cellHeightPx,
                                    artboards.cellWidthPx, artboards.cellHeightPx));
            }
        }
    }
    else
    {
        pages.append(QRectF(0.0, 0.0, output.result.binWidthPx, output.result.usedHeightPx));
    }

    // The writer takes the layers largest first and paints them in that order, so the
    // smallest — the reference, the one the decor sits on — ends up on top, both in the
    // document and in the reader's panel. The canvas walks them the other way round,
    // because it draws the reference first. Hence the walk backwards.
    QList<OGradedSvg::SizeLayer> layers;
    layers.reserve(content.layers.size());
    for (int i = static_cast<int>(content.layers.size()) - 1; i >= 0; --i)
    {
        const OExportCanvasContent::Layer &layer = content.layers.at(i);

        OGradedSvg::SizeLayer sizeLayer;
        // The name the Size Styles give the size is the one written; the axis value stands
        // in when they give none.
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

    // A range of one size is a single size: there is nothing to switch between, and a
    // layer panel offering one entry offers no choice.
    //
    // ⚠️ **Le projecteur fait exception, et c'est tout l'inverse** : même en taille unique il est
    // calqué, parce que ses trois calques de fond et de calibration sont trois choses que
    // l'utilisatrice doit pouvoir éteindre. Un document plat n'en porterait aucun — le magenta
    // resterait sous les pièces, mais plus rien ne pourrait le retirer.
    const bool projector = OExportFormat::IsProjector(output.format);
    const bool layered = projector || (output.hasRange && layers.size() > 1);

    // ── Les calques du projecteur : un fond magenta, deux grilles ────────────────────────────────
    //
    // **Calculés ici, jamais dessinés à l'écran** — décision de Sophie : « on ne l'ajoute pas dans le
    // canvas, mais dans l'export PDF ». Le canvas montre le plan, nu.
    //
    // ⚠️ **Pas de fond blanc** : le fond d'un PDF est blanc, un calque blanc ne servirait à rien. Le
    // fichier de référence en porte un ; c'est un reste d'Illustrator, et il a été relevé comme tel.
    //
    // Les noms sont en clair : la traduction attend la validation du format (`Projector.md` §4).
    // ⚠️ **Écrits par le SOCLE** (`io/GradedOverlays.h`) : Rabnest écrit les mêmes calques pour son
    // propre export projecteur, et les valeurs ci-dessous sont les seules qu'Ostara décide.
    GradedOverlays::Set projectOverlays;
    const GradedOverlays::Set *overlayData = nullptr;
    if (projector && layered)
    {
        // Les noms se lisent dans le panneau du lecteur PDF : ils sont donc TRADUITS, comme tout ce
        // que l'utilisatrice voit. Le rôle, lui, ne l'est jamais : `ostara:role` est une clé de
        // fichier, et c'est lui que Trame et Rabnest lisent, pas le nom.
        // `.name()` : la même écriture qu'avant la descente au socle, pour que le SVG reste
        // identique à l'octet près — la casse d'une couleur ne change rien au rendu, mais elle
        // change un fichier validé.
        projectOverlays.background = QColor(QLatin1String("#E6007E")).name();
        projectOverlays.backgroundName =
            QCoreApplication::translate("OExportPdf", "Magenta background");
        projectOverlays.backgroundId = projectOverlays.backgroundName;

        const QString gridCm = QCoreApplication::translate("OExportPdf", "5cm grid");
        projectOverlays.grids.append({gridCm, gridCm, ToPixel(50.0, Unit::Mm)});

        const QString gridIn = QCoreApplication::translate("OExportPdf", "3in grid");
        projectOverlays.grids.append({gridIn, gridIn, ToPixel(76.2, Unit::Mm)});
        overlayData = &projectOverlays;
    }

    // The decor layer's name comes from the range, and from the range as it is NOW: the
    // decor follows the pattern, exactly as the canvas reads it live. A generation carries
    // its own copy of the range, taken when the run started — and that copy can predate a
    // rename, which would have handed out the old name for ever.
    QString layoutLayerName;
    if (output.hasRange)
    {
        if (const auto *ranges = pattern->SizeRanges(); ranges != nullptr)
        {
            layoutLayerName = ranges->value(output.rangeName).exportLayoutLayerName;
        }
        if (layoutLayerName.isEmpty())
        {
            layoutLayerName = output.range.exportLayoutLayerName;
        }
    }
    if (layered && layoutLayerName.isEmpty())
    {
        layoutLayerName = DefaultDecorLayerName();
    }

    QTemporaryDir dir;
    if (!dir.isValid())
    {
        qWarning() << "OExportPdf: no temporary directory";
        return false;
    }

    // ONE SVG, over the whole plan — not one per sheet. The drawing below is the same
    // everywhere, and a sheet has become a matter for the PDF: the plan is written once
    // into a Form XObject and every page references it, offset so that its own artboard
    // lands on the paper. Writing it per sheet was writing the same drawing N times, and
    // rendering it per sheet was rendering it N times.
    //
    // The rectangle is the plan's own, and the plan starts at the origin: that is what
    // keeps the writer's transform the identity and leaves the sheet offset entirely to
    // the PDF. A page is the plan read through a rectangle — and the writer does not crop
    // to what it is handed, it only sizes the root and shifts the content, so the plan
    // being whole here is exactly what each page needs.
    const QRectF imageRect = artboards.valid ? artboards.planRect : pages.first();

    const QString svgPath = dir.filePath(QStringLiteral("generation.svg"));

    // Whether this plan HAS a decor at all — a sheet grid to draw it on and a template to draw it
    // from. A rouleau and the A0 sheet have neither: no sheets are cut, so there is no frame, no
    // numbering and no marks. That absence is a fact of the file and not a default, and it is what
    // the recipe has to state, because it is the one case where the reader must supply its own
    // decor rather than reproduce the one it was given.
    const bool drawsDecor = artboards.valid && marks != nullptr;

    // The recipe goes into that very file, and only when the file is going to be reopened: the SVG
    // just written IS the payload, so a recipe written for a closed PDF would be a few hundred bytes
    // of intent nobody reads. It states the format and the decor — the two things the drawing cannot
    // state, since a frame drawn for A4 is wrong the moment the client asks for A3, while the same
    // recipe draws the right one at any size.
    //
    // Written before the file is read back, because that is the file the payload is cut from.
    QDomDocument recipe;
    QDomElement recipeElement = rabnestPayload
                                    ? OExportRecipe::Of(recipe, output, drawsDecor ? marks : nullptr,
                                                        framed)
                                    : QDomElement();

    // The pieces travel with the recipe, and for the same reason: the recipe says what paper this
    // is, and the drawing says where each piece stands — neither says what may be DONE with a
    // piece. A plan that has to choose a rotation, glue a fold to the edge of the cloth or cut a
    // piece three times cannot read any of that from ink, so it is stated here, once per piece.
    if (!recipeElement.isNull())
    {
        // The names the drawing gives its layers, put back in the generation's own order: `layers`
        // was walked backwards, and a piece names its layer with the words the reader will see.
        QStringList displayLabels;
        displayLabels.reserve(static_cast<int>(layers.size()));
        for (int index = static_cast<int>(layers.size()) - 1; index >= 0; --index)
        {
            displayLabels.append(layers.at(index).label);
        }

        // The decor the drawing resolved — ONCE, on the reference layer — travels with the pieces,
        // copied into every size by OExportPieces. It is the same `content.decor` the visible layer
        // below is written from, so the payload and the paper cannot show two different drawings.
        //
        // **Et les enfants de chaque membre de groupe viennent en plus** — `payloadDecor`, construits
        // par le canvas sans être dessinés. Ils ne vont QUE dans le payload : Rabnest en a besoin pour
        // afficher un membre seul, la couche visible n'en porte aucun. Les fondre dans `content.decor`
        // ajouterait au plan l'encre de chaque membre.
        QList<OGradedSvg::LayoutObject> payloadDecor = content.decor;
        payloadDecor += content.payloadDecor;
        recipeElement.appendChild(
            OExportPieces::Of(recipe, output.layers, displayLabels, pattern, payloadDecor));
    }

    // ── Le DESSIN, sans la métadonnée ────────────────────────────────────────────────────────────
    //
    // ⚠️ **Le payload ne va plus dans le fichier qui est dessiné.** Il y vivait, dans `<metadata>`,
    // et un convertisseur qui rend les enfants de tout élément qu'il ne connaît pas le DESSINAIT :
    // les cartouches, annotations et droit-fils de chaque pièce réapparaissaient une seconde fois,
    // écrits dans le repère de la pièce — donc posés n'importe où sur la feuille. Constaté sur deux
    // exports du même patron : sans l'option Rabnest la page est propre, avec elle elle se remplit
    // de cartouches flottants.
    //
    // Trame sait écarter `<metadata>` ; mais faire reposer un dessin juste sur ce que le
    // convertisseur ignore est un pari, et il a déjà été perdu une fois. Le payload est donc écrit
    // dans SON fichier, et le fichier dessiné n'en porte plus rien du tout.
    if (!OGradedSvg::WriteGradedSvg(svgPath, layers, layoutLayerName, content.decor, imageRect,
                                    QMarginsF(), 1.0, 1.0, QString(), QString(), layered,
                                    QDomElement(), false, overlayData))
    {
        qWarning() << "OExportPdf: the SVG could not be written";
        return false;
    }

    // ── Le PAYLOAD, dans un fichier à part ──────────────────────────────────────────────────────
    //
    // Le même document, métadonnée comprise : c'est ce que Rabnest réouvre. Personne ne le dessine.
    QString payloadSvgPath;
    if (!recipeElement.isNull())
    {
        payloadSvgPath = dir.filePath(QStringLiteral("payload.svg"));

        // ── Le payload EST le dessin du mode Graded, quand on le lui donne ──────────────────────
        //
        // ⚠️ **Pas le dessin de ce run.** Le canvas d'export dessine le PLAN — pièces nestées,
        // tournées — et ses enfants sont refabriqués par une règle qui n'est pas celle de l'écran :
        // mesuré sur « Manche » à la taille 32, le droit-fil sortait à 0.796 de la pièce là où le
        // mode Graded est à 0.853. Le fichier du mode Graded, lui, porte les pièces à plat, non
        // tournées, et UN décor : c'est lui que la cliente doit retrouver.
        bool payloadWritten = false;
        if (!gradedDrawing.isEmpty())
        {
            QFile graded(gradedDrawing);
            if (graded.open(QIODevice::ReadOnly))
            {
                QString document = QString::fromUtf8(graded.readAll());
                // ⚠️ **Après l'ouverture de `<svg>`, jamais après la déclaration XML.** Le premier
                // `>` du fichier est celui de `<?xml ... ?>` : y insérer la métadonnée la plaçait
                // AVANT l'élément racine, donc à côté — un document à deux racines ou une
                // métadonnée que personne ne lit.
                const qsizetype svgTag = document.indexOf(QStringLiteral("<svg"));
                const qsizetype after = svgTag < 0 ? -1 : document.indexOf(QLatin1Char('>'), svgTag) + 1;
                // ⚠️ **La métadonnée s'écrit depuis l'ÉLÉMENT, pas depuis `recipe`.** `OExportRecipe::Of`
                // rend un élément qui appartient à un autre document : `recipe.toString()` est vide,
                // et c'est ce qui a produit un payload sans `<pieces>` du tout.
                QDomDocument metaDoc;
                metaDoc.appendChild(metaDoc.importNode(recipeElement, true));
                const QString metadata = metaDoc.toString(-1);
                if (after > 0 && after < document.size() && !metadata.isEmpty())
                {
                    document.insert(after, QLatin1Char('\n') + metadata + QLatin1Char('\n'));
                    QFile out(payloadSvgPath);
                    if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        out.write(document.toUtf8());
                        payloadWritten = true;
                    }
                }
            }
        }
        if (!payloadWritten
            && !OGradedSvg::WriteGradedSvg(payloadSvgPath, layers, layoutLayerName, content.decor,
                                           imageRect, QMarginsF(), 1.0, 1.0, QString(), QString(),
                                           layered, recipeElement, false, overlayData))
        {
            qWarning() << "OExportPdf: the payload SVG could not be written";
            return false;
        }
    }

    // The sheet's own frame, its numbering and its registration marks go in as a SECOND
    // document, not into the one above. Trame draws that one on every page, under the layers
    // and — the whole point — OUTSIDE every optional content group, so that nothing in the
    // reader's layer panel can switch the paper's own frame off. A sheet whose marks can be
    // turned off is a sheet that can be assembled wrongly, and it is the silent kind of wrong:
    // the print looks right.
    //
    // It is written over the whole plan, in the plan's own frame, and each page shows of it
    // exactly what it shows of the plan — the same crop, by the same translation, so no page
    // identity has to cross into Trame. Drawn sheet by sheet it would be wrong: a seam belongs
    // to the two sheets that share it and must carry ONE line from the plan's anchor, or its
    // dashes restart at every edge and the join shows in print.
    //
    // Only where there is something to draw: a tiled run whose template resolved. A roll and
    // the A0 sheet have no sheets to cut and no frame to carry, and their export is left
    // exactly as it was — no second document, nothing added to the file. `drawsDecor` says the
    // same thing to the recipe, so the file cannot claim a decor it does not carry.
    QString underlayPath;
    if (drawsDecor)
    {
        underlayPath = dir.filePath(QStringLiteral("underlay.svg"));
        if (!OExportUnderlay::Write(underlayPath, svgPath, *marks, artboards, sheets, framed))
        {
            qWarning() << "OExportPdf: the assembly marks could not be written";
            return false;
        }
    }

    // The cover page goes in the same way and for the same reason: a second document, drawn on
    // every page and — the whole point — outside every optional content group, so that nothing in
    // the reader's panel can deliver the plan without its nomenclature. A plan missing its cover
    // page looks perfectly normal, and it is the silent kind of wrong.
    //
    // Its page is its OWN, and that is where it parts company with the marks: a cover page is
    // 19 × 25.9 cm wherever its format puts it, so it is not laid out on the plan's page — it is
    // written in the plan's own coordinates and Trame re-places it against the plan's own frame,
    // which is what lets the page land where the canvas showed it, sheet of its own or not.
    //
    // Written whatever the grid says. A rouleau has no sheet to cut and still carries a cover page,
    // its block at the bin's own corner; the seat, and only the seat, decides where it stands.
    QString coverPath;
    if (cover != nullptr && !cover->IsEmpty())
    {
        coverPath = dir.filePath(QStringLiteral("cover.svg"));
        const QRectF coverPage =
            OExportCoverPage::PagePx(coverSeat, coverMarginPx, artboards.planRect,
                                     artboards.cellWidthPx, artboards.cellHeightPx);
        if (!OExportCoverPageSvg::Write(coverPath, *cover, coverPage))
        {
            qWarning() << "OExportPdf: the cover page could not be written";
            return false;
        }
    }

    // ⚠️ **C'est le PAYLOAD qu'il faut modifier, pas le dessin du run.** Le payload a son fichier à
    // lui depuis `227f8d49` (« le payload sort du dessin ») : `svgPath` est la génération de CE run,
    // que Trame convertit en pages, et `payloadSvgPath` le document que Rabnest réouvre. Laisser ici
    // `svgPath` écrivait la page de garde dans un fichier qui ne quitte pas la fonction — le PDF
    // gardait sa page (Trame la reçoit séparément, en `coverPath`) et le payload n'en portait plus
    // trace, donc Rabnest ne pouvait plus la redessiner : c'est la régression constatée le
    // 25-10-2026, « la page de garde a disparu dans Print ».
    if (rabnestPayload && !coverPath.isEmpty() && !payloadSvgPath.isEmpty()
        && !CarryCover(payloadSvgPath, coverPath))
    {
        qWarning() << "OExportPdf: the cover page could not be carried in the payload";
    }

    // Trame reads the layers back out of the SVG and rebuilds them as optional content —
    // ONE set of layers for the whole document, not one per sheet, so switching a size off
    // hides it on every page at once. What it takes from us besides the file is the page
    // list: the artboards, in the order the sheets come off the printer. A page's size is
    // ours to state now rather than the SVG's to imply, and that is what lets one drawing
    // serve all of them.
    SvgToOcgPdf converter;
    // What the reader shows in its information panel. The title is the file's own name —
    // what this delivery is called. Author and creator both name the application, as
    // Illustrator does: the content and the file come from the same hand. The producer is
    // left empty on purpose, so the module that writes the PDF names itself.
    SvgToOcgPdf::DocumentInfo info;
    info.title = QFileInfo(pdfPath).fileName();
    info.author = QStringLiteral("Ostara %1").arg(AppDisplayVersion());
    info.creator = info.author;

    // The underlay is the fifth argument, and an empty one says there is none: Trame then
    // writes the page stream it has always written, byte for byte the same. The sixth says
    // whether the layout is offered as a layer — the one thing the dialog asks about, and it
    // opens on no. The seventh is the cover page, empty when there is none, and it is drawn
    // outside every layer whatever the sixth says. The eighth is the sheet that page gets when
    // its format gives it one: empty leaves it on the plan's own sheets. The ninth is the
    // payload — what the plan was drawn from, written inside the file and read by nobody here.
    //
    // The SVG just written IS that payload: it is the plan as drawn, before Trame turned it
    // into pages, and it is what Rabnest reopens to offer another format. It lives in a
    // temporary directory that dies with this function, which is why the payload is handed
    // over now rather than later.
    const QString payloadPath = rabnestPayload ? payloadSvgPath : QString();
    if (!converter.convert(svgPath, pages, pdfPath, info, underlayPath, addLayoutLayer, coverPath,
                           coverSheet, payloadPath))
    {
        qWarning() << "OExportPdf: the PDF could not be written";
        return false;
    }

    return true;
}
