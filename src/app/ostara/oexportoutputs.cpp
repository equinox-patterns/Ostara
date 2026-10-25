/************************************************************************
 **
 **  @file   oexportoutputs.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Export mode — producing a generation, keeping it, and showing it.
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainwindow_helpers.h"
#include "core/ogradedsizeranges.h"
#include "core/vapplication.h"
#include "core/oexportcuttingguard.h"
#include "core/oexportfavorites.h"
#include "core/oexportoutput.h"
#include "core/olengthunits.h"
#include "core/oexporttemplatestorage.h"
#include "dialogs/oexportprintmodel.h"
#include "dialogs/owaitdialog.h"
#include "oexportalignment.h"
#include "oexportartboards.h"
#include "oexportcanvas.h"
#include "oexportcoverpage.h"
#include "oexportcuttingfile.h"
#include "oexportfolder.h"
#include "oexportpdf.h"
#include "oexportplanimage.h"
#include "oexportplanimagewriter.h"
#include "oexportprojector.h"
#include "oexportplotterlayout.h"
#include "oexportplottersvg.h"
#include "oexportsheets.h"

#include "dialogs/oexportcuttingfiledialog.h"
#include "dialogs/oexportimagedialog.h"
#include "dialogs/oexportfiledialog.h"
#include "dialogs/oexportimagelistdialog.h"
#include "dialogs/oexportpdfdialog.h"

#include "oexportgeometry.h"
#include "oexportnesting.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetoutputs.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QDir>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QScopeGuard>
#include <QStringList>
#include <QThreadPool>
#include <QtConcurrent>

#include <atomic>
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

namespace
{
/// One nesting at a time, and never on the global pool: it is shared with the rest
/// of the application, and the documented trap of this codebase is a nested
/// submission that deadlocks the pool.
constexpr int kExportNestingThreads = 1;

/// L'écart entre le plan de tissu et celui de la doublure, en PIXELS — la scène est en pixels à
/// `PrintDPI`, comme tout le reste.
///
/// **Constante, et pas une fraction de la laize** : elle sépare deux nappes, elle n'appartient à
/// aucune des deux, et un écart proportionnel donnerait deux écarts différents pour deux plans de même
/// largeur — donc deux dessins qui ne se comparent plus. Un pouce : de quoi voir d'un coup d'œil qu'il
/// y a deux plans, sans qu'on lise quelque chose entre eux.
constexpr qreal kLiningGapPx = 96.0;



/// @brief Où le plan de DOUBLURE se pose dans la scène : à droite du plan de tissu, séparé par la
///        bande vide ci-dessus.
///
/// Le tissu occupe `(0, 0, binWidthPx, usedHeightPx)` — l'origine de la scène est la sienne depuis
/// toujours — et la doublure vient après lui. Lus sur le rectangle RÉELLEMENT dessiné et non sur le
/// bac : sur un run tuilé le plan est le papier, et deux plans qui se chevaucheraient ne se liraient
/// pas plus mal qu'ils ne se lisent bien.
auto LiningOrigin(const QRectF &fabricRect) -> QPointF
{
    return {fabricRect.right() + kLiningGapPx, fabricRect.top()};
}

/// @brief A duration said out loud: "2 minutes and 35 seconds".
///
/// Voiced rather than summed — "155 seconds" is the same figure and not the same sentence — and
/// each unit takes its plural from the count in front of it. Qt's %n would ask a translation for
/// that plural, and these strings carry none: the source has to read right on its own, so the two
/// forms are chosen here and only the bare words are handed to tr(). They are handed to
/// MainWindow's context rather than a local one, so that lupdate files them beside the label they
/// end up in.
///
/// A part that is zero is left out, because nobody says "2 minutes and 0 second", and a run under
/// a minute is seconds on their own: "0 minutes and 45 seconds" is a figure the reader has to take
/// apart before it means anything.
auto GenerationTimeText(qint64 milliseconds) -> QString
{
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    const auto counted = [](qint64 count, const QString &unit) {
        return QStringLiteral("%1 %2").arg(count).arg(count == 1 ? unit : unit + QLatin1Char('s'));
    };

    QStringList parts;
    if (hours > 0)
    {
        parts << counted(hours, MainWindow::tr("hour"));
    }
    if (minutes > 0)
    {
        parts << counted(minutes, MainWindow::tr("minute"));
    }
    if (seconds > 0)
    {
        parts << counted(seconds, MainWindow::tr("second"));
    }

    // Only a clock that has not ticked yet reaches this, but the label must not be left ending on
    // a separator because of it.
    if (parts.isEmpty())
    {
        parts << counted(0, MainWindow::tr("second"));
    }

    // The last part is the one spoken with "and": "1 hour, 5 minutes and 12 seconds".
    const QString last = parts.takeLast();
    return parts.isEmpty() ? last
                           : MainWindow::tr("%1 and %2").arg(parts.join(QStringLiteral(", ")), last);
}

/// @brief Ce qu'un run a donné, dit dans les deux systèmes d'unités.
///
/// Un plan de coupe sert à **acheter** du tissu, et ses deux lectrices n'achètent pas de la même
/// façon : une Européenne lit la laize en centimètres et le métrage en centimètres, une Américaine lit
/// la laize en pouces — « du 54 » — et demande son métrage en yards. Le run n'a qu'une unité à lui ;
/// la barre les dit toutes les deux, parce qu'elle est le seul endroit où le plan se traduit en une
/// course à faire.
///
/// **Les deux chiffres ne viennent pas du même endroit, et c'est voulu.** La laize est celle qui a été
/// SAISIE (`output.documentWidthMm`, en millimètres, **jamais** divisée par le pli) : c'est le tissu
/// acheté, et personne n'achète 70 pour une laize de 140. Le métrage vient du rectangle réellement
/// dessiné — la lecture unique du plan, déjà faite pour le canvas, l'image et le PDF.
///
/// La note de pli **ouvre la ligne**, sans parenthèses, et dans la même unité que la laize qui suit.
/// Sur un tissu plié, le premier chiffre que l'œil rencontre doit dire ce que le canvas montre
/// vraiment — une demi-laize — sinon « 140 » décrit un canvas qui en fait 70. Les deux chiffres
/// restent voisins et dans la même unité, donc le rapport « moitié » se vérifie d'un coup d'œil.
///
/// Les runs des autres onglets gardent leur forme : sur un run tuilé la « largeur » est celle du
/// PAPIER, et « Laize » n'y voudrait rien dire.
///
/// Le temps de génération ferme les deux formes, et sur **tous** les runs : un rouleau et une feuille
/// d'A0 se génèrent aussi, et un chiffre qui n'apparaîtrait que sous une grille manquerait exactement
/// là où le coût se voit le moins. Le temps est celui du run — voir `generationMs` — pour que
/// sélectionner une autre ligne du dock, ou annuler une correction, ramène le temps qui l'a produite.
auto ExportResultText(const OExportOutput &output, const OExportDrawing &drawing) -> QString
{
    if (output.format == OExportFormat::Kind::Plotter)
    {
        // Un plan plotter n'a ni laize ni pages : il a un **tapis**, et un **nombre de morceaux** —
        // le chiffre que tout l'onglet cherche à faire descendre. Le tapis ouvre la ligne, parce
        // que c'est lui la contrainte ; il est écrit dans l'unité où l'utilisatrice l'a saisi, et
        // le canevas en centimètres, comme partout ailleurs dans le mode.
        //
        // Le compte des morceaux se lit dans les COUCHES et non dans le résultat : c'est la
        // géométrie qui porte les morceaux, et c'est elle qui dit combien la machine en découpera.
        int parts = 0;
        for (const OExportSizeLayer &layer : std::as_const(output.layers))
        {
            parts += layer.pieces.size();
        }

        const qreal canvasWidthMm = FromPixel(output.result.binWidthPx, Unit::Mm);
        const qreal canvasHeightMm = FromPixel(output.result.usedHeightPx, Unit::Mm);

        QString label = MainWindow::tr("Mat %1 × %2")
                            .arg(OLengthUnits::Format(output.matWidthMm, output.matUnit),
                                 OLengthUnits::Format(output.matHeightMm, output.matUnit));

        label += MainWindow::tr(" · %n piece(s)", nullptr, parts);

        // Le nombre de TAPIS, à côté du nombre de morceaux : c'est le chiffre que le rangement fait
        // descendre, et il ne se déduit pas du canevas — les tapis se rangent en rectangle proche du
        // carré, donc la dernière rangée peut être incomplète.
        label += MainWindow::tr(" · %n mat(s)", nullptr, output.result.matCount);

        label += QStringLiteral(" · ")
                 + MainWindow::tr("Canvas %1 × %2")
                       .arg(OLengthUnits::Format(canvasWidthMm, Unit::Cm),
                            OLengthUnits::Format(canvasHeightMm, Unit::Cm));

        return label + MainWindow::tr(" · %1").arg(GenerationTimeText(output.generationMs));
    }

    if (output.format != OExportFormat::Kind::CuttingPlan)
    {
        // ⚠️ **Le test porte sur le FORMAT, surtout PAS sur « pas d'artboards »** : un rouleau
        // d'impression n'a pas d'artboards lui non plus, et la forme neuve y paraîtrait juste par
        // accident — en appelant « laize » la largeur d'un rouleau de papier.
        //
        // On a tiled run what is shown is the PAPER, not the nesting: the plan's extent reaches the
        // bottom of its last row whether or not the pieces do, so the label gives the sheets' own
        // extent — and, for the count, the sheets that will actually be printed. A blank sheet is not
        // written, and a count that promised it would be one the user could not reconcile with the
        // file. The run's height is nowhere on screen, and a figure nobody can see is a figure nobody
        // can check.
        const Unit unit = output.unit;
        const double width = FromPixel(drawing.drawnRect.width(), unit);
        const double height = FromPixel(drawing.drawnRect.height(), unit);

        QString label = MainWindow::tr("%1 × %2 %3").arg(QString::number(width, 'f', 1),
                                                         QString::number(height, 'f', 1),
                                                         UnitsToStr(unit, true));

        if (drawing.artboards.valid)
        {
            // **The cover page's own sheet is a page, and it is said so.** It is printed like the
            // others — it is the first page of the file — but it is not a cell of the grid, so the
            // plan's own count does not hold it. Only the ExtraSheet seat adds one; the two other
            // seats put the page on paper the plan already pays for.
            const bool extraSheet = drawing.cover.seat == OExportCoverPage::Seat::ExtraSheet;
            label += extraSheet
                         ? MainWindow::tr(" · %n pages (cover page included)", nullptr,
                                          drawing.sheets.count + 1)
                         : MainWindow::tr(" · %n pages", nullptr, drawing.sheets.count);
        }

        return label + MainWindow::tr(" · %1").arg(GenerationTimeText(output.generationMs));
    }

    // « Fabric width » et « length » plutôt que « laize » et « métrage » : la chaîne source d'Ostara
    // est en anglais, et la traduction française lui donnera ses mots du métier. La même chaîne sert
    // aux deux systèmes — seul le nombre change de langue.
    //
    // ⚠️ **`QStringLiteral`, jamais `QLatin1String`, pour ce séparateur.** Le point médian est
    // `U+00B7`, deux octets en UTF-8 (`C2 B7`), et `QLatin1String` lit chaque octet comme un
    // caractère Latin-1 : le `C2` devenait un `Â` à l'écran, entre le pli et la longueur. Tout
    // littéral non ASCII de ce fichier passe par `QStringLiteral`.
    //
    // Les pouces en fraction et le métrage en yards : deux arrondis différents et tous deux voulus —
    // le pouce au seizième le plus proche, le yard au huitième SUPÉRIEUR, parce qu'un métrage
    // s'achète. Voir OLengthUnits. **Et un arrondi PAR MATIÈRE** : la doublure a son propre métrage,
    // et une paire n'additionne pas ses yards — on n'achète pas deux étoffes sur un seul chiffre.
    // La note de pli, elle, ouvre la moitié qu'elle concerne, dans la même unité que la laize qui
    // suit — 70 puis 140 — pour que le rapport « moitié » se vérifie sans conversion mentale.
    const auto figuresOf = [](qreal widthMm, qreal lengthMm, bool folded) -> QString
    {
        QString text;

        if (folded)
        {
            text += MainWindow::tr("Canvas: half fabric width %1")
                        .arg(OLengthUnits::Format(widthMm / 2.0, Unit::Cm))
                    + QStringLiteral(" · ");
        }

        text += MainWindow::tr("Fabric width %1 · length %2")
                    .arg(OLengthUnits::Format(widthMm, Unit::Cm),
                         OLengthUnits::Format(lengthMm, Unit::Cm));

        text += QStringLiteral(" · ")
                + MainWindow::tr("Fabric width %1 · length %2")
                      .arg(OLengthUnits::Format(widthMm, Unit::Inch), OLengthUnits::Yards(lengthMm));
        return text;
    };

    // **Un seul plan : la ligne d'hier, mot pour mot.** Ni le nom de la matière, ni le gras, ni un
    // séparateur de plus — un plan de tissu seul se lit exactement comme il se lisait avant la paire.
    if (not output.liningPlan)
    {
        return figuresOf(output.documentWidthMm, FromPixel(drawing.drawnRect.height(), Unit::Mm),
                         output.foldedFabric)
               + MainWindow::tr(" · %1").arg(GenerationTimeText(output.generationMs));
    }

    // ── La paire : le tissu, puis la doublure, et le temps une fois pour les deux ────────────────
    //
    // **Le nom de la matière ouvre chaque moitié, et il est en gras.** Une ligne qui donnerait deux
    // laizes et deux métrages sans dire laquelle est laquelle serait un rébus ; c'est le nom qui fait
    // la phrase, et c'est lui qu'on cherche des yeux quand on lit la seconde moitié.
    //
    // Les deux moitiés sont séparées par un « / » et non par le point médian des chiffres : le nom de
    // la matière est un titre, pas un chiffre de plus, et le point médian le rangerait avec eux.
    //
    // Le temps ferme la ligne, **une seule fois** : c'est celui du run, et c'est bien lui qu'on a
    // attendu — les deux plans se sont générés l'un après l'autre.
    const QString fabric =
        QStringLiteral("<b>%1</b> : ").arg(MainWindow::tr("Fabric"))
        + figuresOf(output.documentWidthMm, FromPixel(drawing.drawnRect.height(), Unit::Mm),
                    output.foldedFabric);

    // Le métrage de la doublure est celui de SON plan, lu dans son propre résultat : c'est le bac de
    // la doublure, et il n'a rien à voir avec celui du tissu.
    const QString lining =
        QStringLiteral("<b>%1</b> : ").arg(MainWindow::tr("Lining"))
        + figuresOf(output.documentLiningWidthMm,
                    FromPixel(output.liningResult.usedHeightPx, Unit::Mm), output.foldedFabric);

    return fabric + QStringLiteral(" / ") + lining
           + MainWindow::tr(" · %1").arg(GenerationTimeText(output.generationMs));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RunExportNesting resolve the range, nest it on the imposed width, and hand the
 *        result back. It draws nothing: the caller files the generation and shows it.
 *
 * **Un run peut décrire DEUX plans.** Un vêtement doublé se taille deux fois — le tissu et la
 * doublure — et les deux nappes se lisent côte à côte : le run fait donc deux passes, l'une sur la
 * matière du tissu, l'autre sur celle de la doublure, et rend un seul résultat qui les porte toutes
 * les deux. Ce qui les sépare n'est pas un réglage : c'est le **jeu de pièces**, chacune lue dans sa
 * propre quantité de coupe (voir OExportCuttingGuard::Material). Tout le reste — sens, pli, espaces,
 * membres de groupe — leur est commun, à la laize près, que chacune a la sienne.
 *
 * @return the generation, or nothing when the run failed.
 * @param  error when given, the reason the run stopped is written there — and, in console mode,
 *         the message box that would have carried it is not opened: a batch has nobody to show a
 *         box to, and it is the reason that goes in its table.
 */
auto MainWindow::RunExportNesting(const OExportRunRequest &request, const OGradedSizeRanges::Context &context,
                                  QString *error)
    -> std::optional<OExportOutput>
{
    // Started before the run does anything, so that the figure reported covers the whole wait and
    // not only the engine: what the user spent is the time in front of the dialog.
    QElapsedTimer generationTimer;
    generationTimer.start();

    // **Les miniatures se taisent pendant toute la génération.**
    //
    // `GradationChanged` réarme ce minuteur à CHAQUE taille, et l'export ouvre une boucle
    // d'événements pendant le nesting : il peut donc y partir, relancer `RefreshThumbnailSnapshots`
    // — qui re-parse le document à la taille de référence, avec `ClearVariables(Measurement)` et un
    // `LiteParseTree(FullLiteParse)` — et changer les mesures sous les pieds de la marche en cours.
    //
    // Mesuré : deux exécutions identiques du même métrage, même binaire, donnaient 33 puis 30
    // tailles « ok », trois tailles basculant entre « 138,2 cm » et « document trop étroit ».
    //
    // La reprise réarme une fois, à la fin : une rafale de tailles se replie en un seul aller-retour,
    // ce que ce minuteur existe pour faire.
    m_thumbnailSnapshotsSuspended = true;
    const auto resumeThumbnails = qScopeGuard([this]
    {
        m_thumbnailSnapshotsSuspended = false;
        ScheduleThumbnailSnapshots();
    });

    // Every length of the request is stored in millimetres and the engine consumes pixels:
    // the one conversion of the run happens here, once. The unit the user typed in plays no
    // part in it — it says how the figures are READ, not what they mean, which is why the
    // same run gives the same plan whatever unit it was entered in.
    // **The run's way out, declared before anything can read it.** Held by a shared pointer and not
    // by this window: the engine is handed a lambda that reads it and runs on a worker thread, and
    // the dialog that sets it is destroyed before the result is read. Created here, ahead of the
    // params that carry the question, so that nothing is handed over that does not yet exist.
    auto abandoned = std::make_shared<std::atomic<bool>>(false);

    OExportNestingParams params;
    // ⚠️ **La largeur du bac n'est PAS posée ici.** Chaque plan a la sienne — le tissu sa laize, la
    // doublure la sienne — donc elle s'écrit dans la passe qui la concerne, et nulle part ailleurs.
    // Tout le reste de ces paramètres est commun aux deux plans, et c'est la règle du run : la
    // doublure reprend les réglages du tissu à l'identique.
    //
    // **Le tissu plié divise la laize par deux, et c'est là-bas que ça se dit** — le seul endroit où
    // une longueur de la requête devient une longueur du run. Le tissu plié en deux dans le sens de
    // la laize donne un bac de 70 pour une laize de 140, et tout le reste suit sans rien savoir :
    // le bac, le canvas, la page du PDF et l'image lisent tous `binWidthPx`.
    // La laize ENTRÉE, elle, ne bouge pas : c'est le tissu acheté, et le nom de la sortie le dit
    // encore (`Cut-140`).
    params.marginPx = ToPixel(request.marginMm, Unit::Mm);
    params.gapPx = ToPixel(request.gapMm, Unit::Mm);
    // The grid follows the same rule as every other length: converted once, here.
    // A run that is not tiled carries a zero height — the roll and A0 — and the
    // engine then has no grid, so no adjustment pass.
    params.artboardCount    = request.columns;
    params.artboardHeightPx = ToPixel(request.frameHeightMm, Unit::Mm);

    // Et le tissu plié, qui ne se dit pas seulement ici : c'est le nesting qui en tire la rotation
    // d'une pièce au pli et son ancrage au bord. Voir OExportNestingParams::foldedFabric.
    params.foldedFabric = request.foldedFabric;

    // **The block the cover page reserves — and only when its format keeps it in a sheet.** The
    // seat is the format's own answer (OExportCoverPage), read from the token the run was asked
    // for: a roll reserves the block at its corner, A3 and Tabloid too, and the three formats
    // that give the page a sheet of its own reserve NOTHING here — their nesting is the one they
    // always had.
    //
    // Left empty for "No cover page" as well, which is the same test one step earlier: an empty
    // name is the single representation the whole feature reads.
    //
    // Converted here, like every other length, and from millimetres: the block is stated in
    // millimetres because that is the unit a format is stated in.
    if (!request.coverPageTemplateName.isEmpty())
    {
        // Already in pixels, and read with the RUN's margin: the block is the page's own rectangle,
        // off the plan's edges by the figure the user set — the same figure the Bin below is built
        // with, so the block and the nesting agree about where the paper's edges are.
        params.coverZonePx = OExportCoverPage::BlockPx(
            OExportCoverPage::SeatOf(FindPrintModel(request.printModelToken)),
            ToPixel(request.marginMm, Unit::Mm));
    }

    // Asked on the worker thread, so it reads an atomic and nothing else: no widget, no document,
    // nothing a thread that is not the GUI thread may touch.
    params.abandoned = [abandoned]() { return abandoned->load(); };

    // What separates the two runs. False leaves the engine turning every piece freely, which is
    // what a print run wants and what it has always done.
    // Le moteur ne demande qu'une chose : les pièces suivent-elles leur droit-fil ? Un plan de
    // coupe et un projecteur, oui ; une impression, non.
    params.grainlinePinned = OExportFormat::IsCutting(request.format);

    // ── Le sens unique est une RÈGLE du projecteur, pas un réglage ───────────────────────────────
    //
    // Un plan projeté se lit d'un coup d'œil sur le tissu : deux pièces dont l'une est couchée tête
    // en bas ne se lisent pas, et c'est exactement ce que « toutes les pièces dans le même sens »
    // demande. Le plan de coupe, lui, en fait une case à cocher — il a ses raisons, et le projecteur
    // n'en a pas.
    //
    // ⚠️ « Sens unique » retire un SENS, pas un axe : une pièce à droit-fil « Double » garde ses deux
    // axes (c'est ce que dit `CuttingRules::AllowedAngles`), elle n'est simplement jamais retournée.
    params.oneWayLayout = request.oneWayLayout || OExportFormat::IsProjector(request.format);

    // **Closing this dialog is how the run is given up.** The engine has no abort hook of its own,
    // so one is handed to it below: the cross sets a flag, the engine reads it between its
    // iterations, and the run stops where it stands. There is no Cancel button because there is
    // nothing left to cancel — what a cross means is exactly this, and it now means it.
    // ⚠️ En mode console il n'y a personne pour cliquer la croix, et une fenêtre qui s'ouvrirait 252
    // fois n'annoncerait rien à personne. Le drapeau reste faux : l'abandon n'existe alors que par
    // le processus — et le CSV écrit au fur et à mesure fait qu'on n'y perd rien.
    std::unique_ptr<OWaitDialog> waitDialog;
    if (VApplication::IsGUIMode())
    {
        waitDialog = std::unique_ptr<OWaitDialog>(new OWaitDialog(tr("Nesting. Please, wait."), this));

        // The cross on the dialog is the request; what the engine reads is the flag it was handed.
        connect(waitDialog.get(), &QDialog::rejected, this, [abandoned]() { abandoned->store(true); });

        waitDialog->show();
        QCoreApplication::processEvents();
    }

    // ── Ce que la marche des tailles abîme, et qu'il faut remettre ─────────────────────────────
    //
    // Marcher les tailles re-parse le document, et un parseur qui réécrit une formule fait
    // `modified = true` puis `emit patternChanged(false)`. Ce signal est branché en direct sur
    // `MainWindow::PatternChangesWereSaved`, qui marque la fenêtre modifiée, la mise en page périmée
    // et les sorties périmées — **alors que l'utilisatrice n'a rien touché**. Elle lisait donc « tu as
    // modifié la pièce » après un simple run.
    //
    // Le mode Graded a la parade depuis toujours, et c'est elle qu'on reprend ici : un `QSignalBlocker`
    // sur le document pendant toute la traversée — le signal n'atteint jamais le slot, donc les trois
    // effets n'ont pas lieu — plus une sauvegarde de `IsModified()` pour la seule chose qui a
    // réellement changé. ⚠️ Restaurer le drapeau SANS le bloqueur ne suffirait pas : la fenêtre et les
    // drapeaux périmés auraient déjà été touchés.
    const qreal savedA = m_currentDimensionA;
    const qreal savedB = m_currentDimensionB;
    const qreal savedC = m_currentDimensionC;
    const bool wasModified = doc->IsModified();
    const bool wasWindowModified = isWindowModified();

    // The only way the export puts the document on a size: the one Details uses for its
    // own drop-downs. Reading the measurements here instead — which is what this code
    // used to do — leaves the pieces on the previous size, and every layer of the range
    // then comes back with the same geometry.
    //
    // The document itself is deliberately left alone: the live combination is what puts
    // it on a size, and GradationChanged() reads that — the Graded path resolves a whole
    // range the same way, without writing anything. Writing it here added a `dimensionX`
    // attribute to documents that do not carry one, and the schema refuses any value
    // below 1 (dimesionValue): such a document could no longer be saved at all.
    // ⚠️ **Sans état : le vrai déplacement est celui de la passe**, qui remet la taille trouvée à la
    // fin de SA traversée — voir `applyPassSize`, dans `runOnePlan`. Une seconde passe doit en effet
    // repartir du document tel qu'il a été trouvé, pas de là où la première l'a laissé.
    const auto applySize = [this](qreal a, qreal b, qreal c)
    {
        m_currentDimensionA = a;
        m_currentDimensionB = b;
        m_currentDimensionC = c;
        GradationChanged();
    };

    // The range is the authority on where the decor of the reference layer goes: it holds
    // an override for every owner the user positioned in Graded, and the mode reads it
    // without ever writing to it. A run that names no range reads nothing here — every piece keeps
    // the geometry it carries — and a cutting plan is told which range covers its size further down.
    //
    // The generation takes a COPY of it: a generation is frozen, so editing or deleting
    // the live range must not move what was already nested.
    //
    // ⚠️ Resolved HERE, before the sizes are walked, and not where it used to be: the frozen decor
    // below is resolved on the FIRST layer, while the document is still on that layer's size, and it
    // needs the range at that moment.
    OSizerange exportRange;
    const OSizerange *range = nullptr;
    if (const QHash<QString, OSizerange> *ranges = pattern->SizeRanges(); ranges != nullptr)
    {
        const auto it = ranges->constFind(request.rangeName);
        if (it != ranges->cend())
        {
            exportRange = it.value();
            range = &exportRange;
        }
    }

    // The decor of a nameless run — a plotter, a single-size print — follows the PIECE, and nothing
    // else. It was briefly made to borrow a size range's Graded placements, on the reasoning that the
    // user arranges them once for all sizes; that is true of the drawing but not of its SEAT, which is
    // a fixed point of the document. Applied to another size's piece it put every grain line somewhere
    // else — the plan came out with its lines scattered. The SEAT is now carried over (the reference
    // seats below), so the question is no longer "can it be done" but "who decides".
    //
    // **Un plan de coupe, lui, ne nomme aucune plage et n'en est pas moins soumis à la règle** : si la
    // taille qu'il coupe appartient à une plage, c'est le placement du mode Graded qui gagne, et hors
    // de toute plage c'est Details qui répond. Il ne peut pas la trouver ici — c'est `runOnePlan` qui
    // connaît la taille — donc il reçoit `decorRange` et le remplace par la plage qui la couvre.
    const OSizerange *decorRange = range;

    // **Où chaque axe se tient pour CE run** — le seul endroit qui le sait, puisque c'est ici que
    // les bases sont appliquées. Une taille unique les nomme toutes ; une plage ne nomme que celles
    // qui ne défilent pas, l'axe gradué étant celui que la page liste. La page de garde et le
    // cartouche lisent cette réponse au lieu du tableau de mesures, qui ne connaît que son ancrage.
    QMap<QString, QString> runAxes;

    // ── The decor of the reference layer, frozen at generation ────────────────────────────────
    //
    // A piece's cartouche and annotations do not live in the piece: they live in the DOCUMENT, where
    // their position is re-materialised every time it is put on a size, and where a rotation is a
    // formula evaluated against the live variables. Drawing them from the document therefore draws
    // the size it is on at the moment of the DRAWING — outlines frozen, children alive, and the two
    // drift apart the moment a run generated at one size is looked at from another. They are resolved
    // ONCE, here, and travel in the output beside the frozen geometry.
    //
    // ⚠️ **Only the reference layer carries decor** — the first one; the canvas strips every other.
    // And the resolution must happen DURING the walk, on that first layer: past the walk the range
    // path has already put the document back on the size it was found on.
    //
    // La MINIATURE est du voyage, résolue dans le même appel : c'est le même dessin
    // gelé, la même fenêtre, et un plan de coupe n'en porte aucune — `request.format` le dit à
    // `ResolvePieceDecor`, qui ne l'enregistre alors pas du tout.
    // Le décor des GROUPES de la couche de référence, résolu au même moment et pour la même raison —
    // voir OExportGroupDecor. Rangé à part parce que le résultat du nesting n'existe pas encore : il
    // naîtra de l'appel au moteur, plus bas. **Une collection PAR PASSE** : chacun des deux plans a
    // ses groupes, et le décor de l'un ne dit rien de l'autre — voir `freezeGroupDecor`.

    // **L'unité du droit-fil est celle du PATRON, jamais celle du run.** La longueur d'une ligne de
    // droit-fil est un réglage du DOCUMENT — « 10 cm » — et c'est dans l'unité du patron qu'elle est
    // écrite : `OGraded::ResolveGrainline` documente son paramètre comme « pattern unit, for the
    // design length », et le canvas vivant lit bien celle-là (`oexportcanvas.cpp:1311`). Le run n'a
    // rien à y voir, et il donnait le change tant que la ligne de coupe le forçait au centimètre.
    // Depuis que l'onglet laisse saisir la laize en pouces, passer `request.unit` serait un facteur
    // 2,54 sur la ligne GELÉE — qui l'emporte sur la vivante.
    const Unit *patternUnit = pattern != nullptr ? pattern->GetPatternUnit() : nullptr;
    const Unit grainlineUnit = patternUnit != nullptr ? *patternUnit : Unit::Mm;

    // **The size the cartouche must state, handed over with the layer it belongs to.** A block that
    // says « Taille : 40, Stature : 176 » reads the measurement table, and the table only knows its
    // reference row; the size this run was asked for lives here, in the bases just applied. Without
    // it the frozen decor carried the reference values into every exported page.
    // ── La place de référence de chaque pièce, pour projeter le Graded sur la taille demandée ──
    //
    // The Graded override is a point of the document, arranged where every size is drawn superposed.
    // A single-size run draws one piece, on its own geometry, so that point has to be carried over —
    // and that is done by the same relative position inside the piece, measured on the range's
    // REFERENCE size. This is that measurement: the pieces' extents with the document put on the
    // range's first size, taken once, before the run's own size is applied.
    QHash<quint32, QRectF> referenceSeats;

    const auto seatOf = [](const VLayoutPiece &piece) -> QRectF {
        const QVector<VLayoutPoint> points = piece.GetMappedExternalContourPoints();
        if (points.isEmpty())
        {
            return {};
        }
        qreal lowX = points.first().x();
        qreal highX = lowX;
        qreal lowY = points.first().y();
        qreal highY = lowY;
        for (const VLayoutPoint &point : points)
        {
            lowX = std::min(lowX, point.x());
            highX = std::max(highX, point.x());
            lowY = std::min(lowY, point.y());
            highY = std::max(highY, point.y());
        }
        return QRectF(QPointF(lowX, lowY), QPointF(highX, highY));
    };

    // **La plage est un PARAMÈTRE, et non plus une capture.** Un run qui nomme sa plage la donne
    // telle quelle ; un plan de coupe, lui, n'en nomme aucune et doit pourtant savoir laquelle couvre
    // la taille qu'il coupe — c'est `runOnePlan` qui la trouve, parce que c'est lui qui connaît la
    // taille. Deux plans, deux réponses possibles.
    // ── Le décor d'une PIÈCE, à la taille de sa couche ────────────────────────────────────────────
    //
    // ⚠️ **Par couche, et pas seulement sur la référence.** Graded ne projette pas les enfants d'une
    // taille sur une autre : il construit un item par (pièce × taille), chacun avec SES enfants
    // recalculés à cette taille, puis il masque le droit-fil et le pli des couches qui ne sont pas la
    // référence. Le payload, lui, portait un seul jeu recopié dans chaque `<piece>` : un lecteur qui
    // dessine une taille autre que la référence recevait le décor d'une autre pièce — mesuré côté
    // Rabnest, jusqu'à 227 px d'écart en x et 42 px en y sur les pièces sans pli.
    //
    // Résolu ICI parce que c'est le seul endroit où le document est sur cette taille : le résolveur
    // lit `pattern->DataPieces()`, donc il ne vaut que pour la taille courante.
    // ⚠️ **`ownerRange` est la plage de CETTE passe, et elle est un paramètre.** Un plan de coupe ne
    // nomme aucune plage : c'est `RangeCoveringSize` qui trouve celle qui couvre sa taille, et c'est
    // d'elle que viennent les positions que l'utilisatrice a posées en Graded (droit-fil, bloc,
    // annotation, miniature). La lire ici plutôt que dans la variable `range` de la fonction est ce
    // qui distingue les deux — et l'oublier laissait un plan de coupe sans les placements de sa plage.
    const auto freezePieceDecor = [&](OExportSizeLayer &layer,
                                      const QMap<QString, QString> &sizeAxes,
                                      const OSizerange *ownerRange) {
        for (OExportPieceGeometry &piece : layer.pieces)
        {
            piece.decor =
                ResolvePieceDecor(piece, ownerRange, pattern, doc, request.format, sizeAxes);

            // The grainline is the same story read from the same document — and it is frozen HERE,
            // in the same breath, because it is the same defect: it stayed invisible only for as long
            // as a single-size run left the document on the run's own size. It follows the piece, so
            // it is frozen per size exactly like the decor.
            piece.grainline = ResolvePieceGrainline(piece, ownerRange, grainlineUnit, pattern,
                                                    request.format, referenceSeats.value(piece.id));
        }
    };

    // ── Le décor d'un GROUPE, sur la RÉFÉRENCE seule ──────────────────────────────────────────────
    //
    // Un groupe n'est résolu qu'UNE fois, sur l'ensemble de ses membres : c'est leur union qui donne
    // son cadre, et c'est d'elle que vient le repli d'un enfant jamais placé. Et c'est la référence
    // qui le porte, parce que le groupe est un DESSIN — celui de la sortie qui reproduit le document
    // tel qu'il se lit, taille par taille superposées — et que ce dessin n'a qu'un cartouche.
    const auto freezeGroupDecor = [&](OExportSizeLayer &layer,
                                      QHash<quint32, OExportGroupDecor> &decor,
                                      const QMap<QString, QString> &sizeAxes,
                                      const OSizerange *range) {
        QHash<quint32, QVector<OExportPieceGeometry>> members;
        for (const OExportPieceGeometry &piece : layer.pieces)
        {
            if (piece.groupId != 0)
            {
                members[piece.groupId].append(piece);
            }
        }

        for (auto it = members.cbegin(); it != members.cend(); ++it)
        {
            decor.insert(it.key(),
                         ResolveGroupDecor(it.value(), it.key(), range, grainlineUnit, pattern,
                                           doc, request.format, sizeAxes));
        }
    };

    // A table without axes — an .otu, or a .vit — describes a single size. There is no
    // size to put the document on, and no reason to try: the Layout mode, which has
    // nested a single size since long before this one existed, never touches the document
    // either. Its measurements resolve to what they already hold whatever the bases say —
    // an .otu's shifts are zero and its base is zero — so the size path would re-read the
    // whole table and re-parse the pattern only to arrive where it started.
    const bool hasAxes = !m_m.isNull() && !m_m->Dimensions().isEmpty();

    // ⚠️ The document's signals are cut for the whole walk. The parsers still run and still set
    // `modified` — nothing can stop that — but `patternChanged` never reaches
    // `PatternChangesWereSaved`, so the window, the layout and the outputs are never marked. That is
    // the bug. Restoring `IsModified()` alone would NOT have been enough: those three marks would
    // already have been made.
    //
    // `blockSignals` rather than a scoped blocker because a `QSignalBlocker` cannot be closed early:
    // it would have to live to the end of the function, and the nesting below — which talks to a
    // worker thread and drives a wait dialog — has no business running with the document mute.
    const bool wasSignalsBlocked = doc->signalsBlocked();
    doc->blockSignals(true);

    // ⚠️ **Le dessin de référence des miniatures est gelé AVANT la marche des tailles** — la même
    // précaution que l'export Graded (`mainwindow_graded_export.cpp`), et au même moment du raisonnement :
    // `freezePieceDecor` résout les miniatures de chaque couche, et `ThumbnailReferenceContent()`
    // rend la géométrie **VIVANTE** tant que rien n'a été capturé. La marche commence sur la première
    // taille du range, qui n'est pas forcément l'ancre du patron : la miniature serait alors dessinée à
    // cette taille-là. Gelé, le cache donne aux trois exports — Details, Graded, Export — le même dessin.
    //
    // ⚠️ **Sous le bloqueur, jamais avant.** Le service fait un aller-retour de parsing complet du
    // document ; laissé émettre, il marquerait la fenêtre modifiée, la mise en page périmée et les
    // sorties périmées AVANT que ce run n'ait pris les trois drapeaux qu'il doit restaurer — et il les
    // restaurerait donc déjà sales. C'est le défaut que le bloc au-dessus existe pour éteindre.
    //
    // Le minuteur qui aurait fait ce gel est arrêté au passage : sa capture est désormais faite, et son
    // aller-retour n'a plus rien à apprendre à personne — le laisser partir pendant le nesting ci-dessous
    // ferait re-parser le document pour rien.
    //
    // Ni un plan de coupe ni un run plotter n'en porte : rien à geler, et donc rien à leur donner.
    // Seuls l'impression et le projecteur portent des miniatures : le plan de coupe n'en veut pas,
    // et le plotter dessine le patron tel quel, sans cartouche.
    if (OExportFormat::CarriesDecor(request.format)
        and request.format != OExportFormat::Kind::Plotter)
    {
        if (m_thumbnailSnapshots != nullptr)
        {
            m_thumbnailSnapshots->stop();
        }
        RefreshThumbnailSnapshots(/*freeze*/ true);
    }

    // ── Une passe = un plan ───────────────────────────────────────────────────────────────────────
    //
    // **Le même code sert le tissu et la doublure**, et ce qui les sépare tient en deux choses : la
    // matière — donc le jeu de pièces, chacune lue dans sa propre quantité de coupe — et la laize. Tout
    // le reste est commun, et c'est voulu : la doublure reprend les réglages du tissu à l'identique.
    //
    // La géométrie est refaite à chaque passe : elle porte la matière et la largeur, et rien de tout
    // cela ne se réutilise d'un plan à l'autre.
    struct PlanPass
    {
        QVector<OExportSizeLayer> layers{};
        OExportNestingResult result{};
        QHash<quint32, OExportGroupDecor> groupDecor{};

        /// **Ce que la passe a RÉELLEMENT posé**, quand ce n'est pas ce que la requête demandait.
        ///
        /// Un projecteur cherche sa largeur et son espacement : la requête dit « 10 cm », le plan
        /// retenu peut en avoir posé 5, et sa largeur n'était dans aucune requête. La génération
        /// enregistre ce qui s'est passé — c'est ce que la barre d'état et la recette lisent — et
        /// zéro veut dire « rien de particulier », donc les valeurs de la requête.
        int widthMm{0};
        qreal gapMm{0.0};
    };

    /// Le sort d'une passe : son plan, ou la raison pour laquelle il n'y en a pas. **Aucun message
    /// n'est dit ici** — ils le sont par l'appelant, une fois le document rendu et ses signaux
    /// rétablis, parce qu'une boîte ouverte d'ici s'ouvrirait sur un document muet et resté sur la
    /// taille du run. Voir `reportStop`.
    struct PassResult
    {
        enum class Stop
        {
            None,      ///< le plan est là
            Empty,     ///< la matière n'a donné aucune pièce à poser
            Abandoned, ///< l'utilisatrice a fermé la fenêtre d'attente
            Failed,    ///< le moteur a refusé, et a dit pourquoi
        };

        std::optional<PlanPass> plan{};
        Stop stop{Stop::None};
        OExportCuttingGuard::Material material{OExportCuttingGuard::Material::Fabric};
        QString error{};      ///< ce que le moteur a refusé, mot pour mot
        QString diagnostic{}; ///< ce que la géométrie a vu, quand elle n'a rien gardé
    };

    // The annealing is pure computation over its inputs — it runs off the GUI thread. **Un seul pool
    // pour les deux passes** : elles se suivent, jamais ensemble, et le thread unique est ce qui rend
    // le run reproductible.
    QThreadPool pool;
    pool.setMaxThreadCount(kExportNestingThreads);

    const auto runOnePlan = [&](OExportCuttingGuard::Material material, qreal widthMm) -> PassResult
    {
        PassResult pass;
        pass.material = material;

        // **La laize de CE plan, et le plié** — le seul endroit du run où une longueur de la requête
        // devient une longueur du moteur. Le plié divise la laize par deux, et la doublure le suit :
        // c'est un réglage du run, commun aux deux plans, donc ses deux bacs sont pliés.
        OExportNestingParams planParams = params;
        planParams.binWidthPx = ToPixel(request.foldedFabric ? widthMm / 2.0 : widthMm, Unit::Mm);

        // Resolving the geometry re-parses the document once per size of the range, so it
        // stays on the GUI thread: VLayoutPiece::Create reads the application settings, and
        // the container it reads is the one the scenes are built from.
        OExportGeometry geometry(pattern, doc, m_m);
        geometry.SetFormat(request.format);
        // **La matière, et c'est tout ce que le jeu de pièces a de plus** : chaque plan lit la sienne.
        geometry.SetCuttingMaterial(material);
        // Et si le tissu est plié : c'est de là que la pièce au pli garde son pli au lieu d'être
        // dépliée, et qu'une pièce coupée deux fois n'est posée qu'une.
        geometry.SetFoldedFabric(request.foldedFabric);

        // Un plotter domestique découpe sur des **tapis** : ses pièces ne sont plus des pièces mais des
        // **morceaux**, découpés aux bords du tapis pendant que la géométrie est collectée. C'est de là
        // que vient « un objet par morceau » — la géométrie le porte, et l'écriture n'a rien à découper.
        //
        // La conversion mm → px est la même qu'ailleurs, et au même endroit que les autres : une seule
        // fois, à l'entrée du run.
        if (request.format == OExportFormat::Kind::Plotter)
        {
            geometry.SetCuttingMat(ToPixel(request.matWidthMm, Unit::Mm),
                                   ToPixel(request.matHeightMm, Unit::Mm));

            // Les membres de groupe que l'utilisatrice a décochés. Ce n'est PAS un filtre de pièces : un
            // plotter décompose les groupes, donc les cases ne gardent pas un membre, elles en écartent
            // un. Vide — le cas de toutes les cases cochées — n'écarte rien et laisse le run tel qu'il a
            // toujours été.
            geometry.SetPieceExclusion(request.excludedPieces);
        }

        // A cutting plan decomposes the groups: it keeps the member the user chose for each and drops
        // the others, so a group is not unioned into a single footprint. Empty means every piece —
        // which is what a print run asks for, and what a pattern with no group asks for anyway.
        //
        // **Le filtre est celui de la MATIÈRE** : une pièce taillée en doublure seulement n'est pas du
        // plan de tissu, et le membre retenu pour un groupe peut différer d'une matière à l'autre — donc
        // les deux passes ne gardent pas forcément le même.
        if (not request.groupMemberChoices.isEmpty())
        {
            const QVector<quint32> kept =
                OExportCuttingGuard::PiecesToCut(request.groupMemberChoices, pattern, material);
            geometry.SetPieceFilter(QSet<quint32>(kept.cbegin(), kept.cend()));
        }

        // ── La plage qui gouverne CETTE taille, quand le run n'en nomme aucune ───────────────────
        //
        // Un plan de coupe ne nomme pas de plage : il coupe UNE taille, et c'est tout. Mais une taille
        // peut appartenir à une plage, et là la règle est celle du reste de l'application — **le
        // placement du mode Graded gagne, et hors de toute plage c'est Details qui répond**.
        //
        // Ce que ça déplace ici est exactement ce que ce mode montre d'une pièce : son droit-fil (§4.2).
        // Le membre choisi porte le sien, et le mode Graded le place plage par plage ; sans plage, le
        // plan garde la ligne de Details, celle que la pièce transporte.
        //
        // ⚠️ **La copie est locale à la passe**, comme `exportRange` l'est au run : une génération est
        // gelée, donc retoucher la plage vivante ne doit pas déplacer un plan déjà calculé.
        OSizerange coveringRange;
        const OSizerange *planRange = decorRange;
        if (OExportFormat::IsCutting(request.format) && planRange == nullptr && pattern != nullptr)
        {
            const auto orFloor = [](qreal chosen, qreal fromDocument)
            { return chosen > 0.0 ? chosen : (fromDocument > 0.0 ? fromDocument : 1.0); };

            const QString covering = geometry.RangeCoveringSize(orFloor(request.baseA, savedA),
                                                                orFloor(request.baseB, savedB),
                                                                orFloor(request.baseC, savedC));
            const QHash<QString, OSizerange> *ranges = pattern->SizeRanges();
            if (ranges != nullptr && !covering.isEmpty())
            {
                const auto it = ranges->constFind(covering);
                if (it != ranges->cend())
                {
                    coveringRange = it.value();
                    planRange = &coveringRange;
                }
            }
        }

        // Le décor des groupes de CETTE passe, et les couches de CETTE passe : deux plans, deux jeux.
        QHash<quint32, OExportGroupDecor> planGroupDecor;
        QVector<OExportSizeLayer> planLayers;

        // **Le vrai déplacement est celui de la passe.** Elle remet le document sur la taille qu'elle a
        // trouvée en sortant, sans quoi la passe suivante partirait de la taille du run précédent.
        bool passMoved = false;
        const auto applyPassSize = [&](qreal a, qreal b, qreal c)
        {
            applySize(a, b, c);
            passMoved = true;
        };

        if (request.singleSize)
        {
            if (!hasAxes)
            {
                // Already on its only size: the geometry is read as it stands.
                planLayers = geometry.CollectCurrentSize(tr("Single size"));

                // ⚠️ Un plan de coupe peut ne garder AUCUNE pièce — toutes celles qu'on lui a données
                // avaient un cut vide pour cette matière — et une marche vide n'a pas de couche de
                // référence à geler. Le cas est signalé par l'appelant : `planLayers.last()` sur une liste
                // vide n'est pas une erreur qu'on veut découvrir en release.
                if (not planLayers.isEmpty())
                {
                    // Aucun axe à nommer : une table taille unique n'a qu'une taille, et la valeur
                    // de référence du cartouche est déjà la sienne.
                    freezePieceDecor(planLayers.last(), {}, planRange);
                    freezeGroupDecor(planLayers.last(), planGroupDecor, {}, planRange);
                }
            }
            else
            {
                // The document is put on the requested size for the run, and given back afterwards like
                // the range path — see the restore below.
                //
                // An axis the measurement table does not carry has no size to offer, and the
                // dialog says so with -1 — a value shaped like a size without being one, handed
                // to the pieces as if it were. The document's own value stands instead, and the
                // axis's first size when the document has none either: 1 is the only base the
                // schema accepts as a floor. The range path below guards its bases the same way.
                const auto baseOr = [](qreal fromDialog, qreal fromDocument)
                { return fromDialog > 0.0 ? fromDialog : (fromDocument > 0.0 ? fromDocument : 1.0); };

                const qreal baseOfA = baseOr(request.baseA, savedA);
                const qreal baseOfB = baseOr(request.baseB, savedB);
                const qreal baseOfC = baseOr(request.baseC, savedC);

                // The range's reference size, walked once for the seats. `applySize`, not
                // `applyPassSize`: this move is not the pass's own, and the restore below must not
                // think it owes the document anything.
                //
                // ⚠️ **Une place ne sert qu'à une chose : reprojeter un droit-fil de la plage sur la
                // taille du run.** Un plan de coupe n'a que ce décor-là (§4.2) : sans override de
                // droit-fil dans la plage, cette marche ne porterait rien, et il n'y a pas de raison
                // de re-parser tout le patron pour elle — un lot de métrage en compte des centaines.
                const bool seatMatters =
                    planRange == nullptr || !OExportFormat::IsCutting(request.format)
                    || std::any_of(planRange->pieceContent.cbegin(), planRange->pieceContent.cend(),
                                   [](const OSizerange::PieceContent &content)
                                   { return content.grainline.valid; });

                if (planRange != nullptr && !planRange->name.isEmpty() && seatMatters)
                {
                    const QVector<OExportGeometry::OExportSizeRef> rangeSizes =
                        geometry.RangeSizes(planRange->name);
                    if (!rangeSizes.isEmpty())
                    {
                        const OExportGeometry::OExportFixedBases pins =
                            geometry.RangeFixedBases(planRange->name);
                        qreal seatA = pins.a > 0.0 ? pins.a : savedA;
                        qreal seatB = pins.b > 0.0 ? pins.b : savedB;
                        qreal seatC = pins.c > 0.0 ? pins.c : savedC;
                        const OExportGeometry::OExportSizeRef first = rangeSizes.first();
                        if (first.axisIndex == 0)
                        {
                            seatA = first.base;
                        }
                        else if (first.axisIndex == 1)
                        {
                            seatB = first.base;
                        }
                        else
                        {
                            seatC = first.base;
                        }

                        applySize(seatA, seatB, seatC);
                        const QVector<OExportSizeLayer> seats =
                            geometry.CollectCurrentSize(QStringLiteral("reference seat"));
                        for (const OExportSizeLayer &one : seats)
                        {
                            for (const OExportPieceGeometry &geometryOne : one.pieces)
                            {
                                referenceSeats.insert(geometryOne.id, seatOf(geometryOne.piece));
                            }
                        }
                    }
                }

                applyPassSize(baseOfA, baseOfB, baseOfC);
                planLayers = geometry.CollectCurrentSize(
                    QStringLiteral("A=%1 B=%2 C=%3").arg(baseOfA).arg(baseOfB).arg(baseOfC));

                // Rien à geler quand la marche n'a gardé aucune pièce — même raison que ci-dessus.
                if (not planLayers.isEmpty())
                {
                    const QMap<QString, QString> axes = geometry.AxisLabels(baseOfA, baseOfB, baseOfC);
                    runAxes = axes;
                    freezePieceDecor(planLayers.last(), axes, planRange);
                    freezeGroupDecor(planLayers.last(), planGroupDecor, axes, planRange);
                }
            }
        }
        else
        {
            const QVector<OExportGeometry::OExportSizeRef> sizes = geometry.RangeSizes(request.rangeName);

            // The range pins the axes it does not grade, and the pieces must be resolved with
            // those values. The document's own ones send an axis it does not carry back to the
            // measurement file's base, and every piece built from a formula — which is all of
            // them — then comes out at a geometry the range does not name. The graded capture
            // feeds its pieces this exact way. An axis the range pins nothing at keeps the
            // document's value.
            const OExportGeometry::OExportFixedBases fixed = geometry.RangeFixedBases(request.rangeName);
            const qreal fixedA = fixed.a > 0.0 ? fixed.a : savedA;
            const qreal fixedB = fixed.b > 0.0 ? fixed.b : savedB;
            const qreal fixedC = fixed.c > 0.0 ? fixed.c : savedC;

            for (const OExportGeometry::OExportSizeRef &size : sizes)
            {
                // **The three values this size stands at**, written once so that the geometry and
                // the cartouche are fed the same triple — the graded axis takes the size, the other
                // two keep what the range pins.
                qreal appliedA = fixedA;
                qreal appliedB = fixedB;
                qreal appliedC = fixedC;
                switch (size.axisIndex)
                {
                    case 0:
                        appliedA = size.base;
                        break;
                    case 1:
                        appliedB = size.base;
                        break;
                    default:
                        appliedC = size.base;
                        break;
                }
                applyPassSize(appliedA, appliedB, appliedC);

                planLayers += geometry.CollectCurrentSize(size.label);

                // ── Le décor de CETTE taille, résolu pendant qu'elle est sur le document ────────
                //
                // ⚠️ **Le décor suit la pièce, taille par taille.** Graded ne projette pas un décor
                // d'une taille sur une autre : il construit les enfants de chaque pièce À la taille de
                // la couche, puis masque ceux des couches qui ne sont pas la référence. Un seul jeu
                // recopié dans toutes les `<piece>` donnait donc, pour chaque taille, le décor d'une
                // autre — mesuré côté Rabnest jusqu'à 227 px d'écart en x.
                //
                // Le résolveur lit le document (`pattern->DataPieces()`), donc il ne vaut que pour la
                // taille où le document est : c'est ICI, entre `applyPassSize` et la couche suivante,
                // et nulle part ailleurs.
                freezePieceDecor(planLayers.last(),
                                 geometry.AxisLabels(appliedA, appliedB, appliedC), planRange);

                // Le GROUPE, lui, reste un dessin unique : il n'est gelé que sur la référence.
                if (planLayers.size() == 1)
                {
                    freezeGroupDecor(planLayers.last(), planGroupDecor,
                                     geometry.AxisLabels(appliedA, appliedB, appliedC), planRange);

                    // **La page, elle, ne nomme que les axes qui ne défilent pas.** L'axe gradué
                    // n'a pas UNE valeur dans une plage : c'est la liste des tailles qui le dit. On
                    // l'écarte donc du contexte de la page (base nulle = axe non nommé), sans quoi
                    // elle annoncerait une taille comme si elle était la seule.
                    runAxes = geometry.AxisLabels(size.axisIndex == 0 ? 0.0 : appliedA,
                                                  size.axisIndex == 1 ? 0.0 : appliedB,
                                                  size.axisIndex == 2 ? 0.0 : appliedC);
                }
            }

            // The document was not put back here: the restore is the PASS's own, just below — it
            // borrows the document, so it gives it back, and the next pass then finds it where this
            // one found it.
        }

        // ── Rendre le document tel qu'on l'a trouvé ───────────────────────────────────────────────
        //
        // The document was only borrowed to walk: put it back where it was found, **whatever path ran**.
        // The single-size path used to keep it — "the user asked for this size" — and that is what left
        // Details rendering the export's size, since the Details scene is built from the live combination
        // and its size drop-downs read it too. It no longer has to stay: the reference layer's decor is
        // frozen into the output, so nothing that gets displayed needs the document on the run's size.
        //
        // ⚠️ **Dans la passe, et sous le bloqueur.** La seconde passe doit repartir du document tel
        // qu'il a été trouvé, donc la remise en place ne peut pas attendre la fin du run ; et faite ici
        // elle n'émet rien, ce qui est exactement ce qu'on veut d'un document qu'on n'a fait que
        // traverser. Les trois drapeaux (fenêtre, mise en page, sorties), eux, sont restaurés une fois
        // pour les deux passes — voir l'appelant.
        if (passMoved)
        {
            applySize(savedA, savedB, savedC);
        }

        // Each size sits where the model puts it, and a piece pinned at its fold does not
        // drift like a piece growing from its centre. Unioning them into one footprint only
        // means something once they line up — which is what Graded does before it draws,
        // and what the nesting needs for the same reason.
        // Le projecteur CENTRE chaque pièce sur elle-même : une pièce au pli est rangée sur le côté
        // qu'elle occupe pliée, et dépliée elle pendrait d'un côté — l'empreinte s'allonge et la
        // projection n'est plus centrée. Un plan projeté n'a pas de pli à border (voir
        // OExportAlignment::Apply).
        OExportAlignment::Apply(planLayers, pattern, OExportFormat::IsProjector(request.format));

        int pieceCount = 0;
        for (const OExportSizeLayer &layer : planLayers)
        {
            pieceCount += layer.pieces.size();
        }

        if (pieceCount == 0)
        {
            // **Le message est dit par l'appelant**, une fois le document rendu et ses signaux
            // rétablis : une boîte ouverte d'ici s'ouvrirait sur un document muet, resté sur la taille
            // du run, et rien ne le rattraperait plus.
            pass.stop = PassResult::Stop::Empty;
            pass.diagnostic = geometry.LastDiagnostic();
            waitDialog.reset();
            return pass;
        }

        // ── Le projecteur cherche sa largeur ──────────────────────────────────────────────────────
        //
        // Le moteur EXIGE une largeur, et un projecteur n'a pas de laize : sa largeur est cherchée,
        // palier par palier, jusqu'au premier dont la page tient sous le plafond. C'est fait ici
        // plutôt qu'à l'appelant parce que c'est ici que les pièces résolues et alignées existent —
        // la recherche ne fait tourner que le MOTEUR, jamais la géométrie : une, deux, trois passes
        // sur les mêmes empreintes.
        if (OExportFormat::IsProjector(request.format))
        {
            OExportProjector::Limits limits;
            limits.gapMm = request.gapMm;
            limits.pageMaxMm = 5000.0;

            QFuture<OExportProjector::Outcome> future = QtConcurrent::run(
                &pool, &OExportProjector::Nest, planLayers, planParams, limits);

            QFutureWatcher<OExportProjector::Outcome> watcher;
            QEventLoop fitLoop;
            connect(&watcher, &QFutureWatcher<OExportProjector::Outcome>::finished, &fitLoop,
                    &QEventLoop::quit);
            watcher.setFuture(future);
            fitLoop.exec();

            const OExportProjector::Outcome outcome = watcher.result();
            waitDialog.reset();

            if (outcome.abandoned)
            {
                pass.stop = PassResult::Stop::Abandoned;
                return pass;
            }

            if (!outcome.ok)
            {
                // **Le refus, et il est chiffré.** Aucun fichier ne sera écrit : au-delà du plafond
                // le PDF sortirait rétréci en silence, et c'est exactement le fichier qui a l'air
                // juste. Le message dit quoi changer, et par combien.
                pass.stop = PassResult::Stop::Failed;
                pass.error = outcome.error;
                return pass;
            }

            pass.plan = PlanPass{std::move(planLayers), outcome.result, std::move(planGroupDecor),
                                 outcome.widthMm, outcome.gapMm};
            return pass;
        }

        // Un run plotter ne parle PAS au moteur de nesting, et ce n'est pas un raccourci : un vrai
        // nesting déplacerait et tournerait les morceaux, et le puzzle ne se recollerait plus. Sa mise
        // en page est arithmétique — les morceaux empilés par blocs — et c'est tout ce qu'un plotter
        // domestique demande, puisque c'est son propre logiciel qui re-neste ensuite sur ses tapis.
        //
        // Le même endroit, le même thread, le même dialogue d'attente, le même report d'erreur : seule
        // la fonction qui calcule change.
        QFuture<OExportNestingResult> future =
            request.format == OExportFormat::Kind::Plotter
                ? QtConcurrent::run(&pool, &OExportPlotterLayout::Run, planLayers,
                                    OExportPlotter::Mat{ToPixel(request.matWidthMm, Unit::Mm),
                                                        ToPixel(request.matHeightMm, Unit::Mm)})
                : QtConcurrent::run(&pool, &OExportNesting::Run, planLayers, planParams);

        QFutureWatcher<OExportNestingResult> watcher;
        QEventLoop loop;
        connect(&watcher, &QFutureWatcher<OExportNestingResult>::finished, &loop, &QEventLoop::quit);
        watcher.setFuture(future);
        loop.exec();

        const OExportNestingResult result = future.result();
        waitDialog.reset();

        if (result.abandoned)
        {
            // **Nothing is said about it.** The user closed the dialog to stop the run; a message
            // telling them it did not finish would be the application arguing with a decision that was
            // theirs to make. Nothing is filed, nothing is changed, and the mode stays where it is.
            pass.stop = PassResult::Stop::Abandoned;
            return pass;
        }

        if (!result.ok)
        {
            pass.stop = PassResult::Stop::Failed;
            pass.error = result.error;
            return pass;
        }

        pass.plan = PlanPass{std::move(planLayers), result, std::move(planGroupDecor)};
        return pass;
    };

    // ── Les plans du run, l'un après l'autre ──────────────────────────────────────────────────────
    //
    // **Le tissu d'abord, et il garde sa place.** `layers` et `result` restent le plan de tissu, ce que
    // tout l'existant lit — l'écriture DXF / PLT / HPGL, le PDF, l'image — et la doublure se range à
    // côté d'eux. L'ordre n'est pas décoratif : c'est lui qui dit lequel des deux est le plan
    // principal, et donc ce qu'un export qui n'a pas encore appris la paire écrit.
    PassResult fabric = runOnePlan(OExportCuttingGuard::Material::Fabric, request.documentWidthMm);

    // La doublure ne se demande que si le tissu a abouti : une paire dont la première nappe a échoué
    // n'est pas une paire, et lancer la seconde pour la jeter serait du temps perdu à l'utilisatrice.
    PassResult lining;
    if (fabric.stop == PassResult::Stop::None and request.liningPlan)
    {
        lining = runOnePlan(OExportCuttingGuard::Material::Lining, request.documentLiningWidthMm);
    }

    // Le document est rendu avant tout message : les signaux comme on les a trouvés, puis les trois
    // drapeaux que la traversée a pu salir. Le faire ICI, une fois, couvre les deux passes et tous les
    // chemins de sortie — y compris ceux qui échouent.
    doc->blockSignals(wasSignalsBlocked);
    doc->SetModified(wasModified);
    setWindowModified(wasWindowModified);
    // Un run interrompu ne dit rien ; un run que le moteur a refusé dit ce que le moteur a dit ; une
    // passe vide dit pourquoi elle l'est. Les trois sortent d'ici — et dans les trois cas rien n'est
    // classé, rien n'est changé, et le mode reste où il est.
    const auto reportStop = [&](const PassResult &stopped)
    {
        if (stopped.stop == PassResult::Stop::Abandoned)
        {
            return;
        }

        if (stopped.stop == PassResult::Stop::Failed)
        {
            // ⚠️ Le motif sort par le paramètre AVANT la boîte : en console il n'y a pas de boîte, et
            // c'est ce texte qui va dans la colonne `statut` du tableau.
            if (error != nullptr)
            {
                *error = stopped.error;
            }

            if (VApplication::IsGUIMode())
            {
                QMessageBox::warning(this, tr("Export mode"), stopped.error, QMessageBox::Ok, QMessageBox::Ok);
            }
            return;
        }

        QString message = tr("This size range carries no piece to nest.");
        if (OExportFormat::IsCutting(request.format))
        {
            // Un plan de coupe ne prend que les pièces dont la section Cut de SA matière est
            // renseignée : c'est la seule raison possible que la géométrie ne montre pas, donc elle est
            // dite — sans affirmer que c'est celle-là. Le mot suit la matière, parce que la doublure a
            // son propre jeu de pièces et peut donc être vide toute seule.
            message += QStringLiteral("\n\n")
                       + tr("A cutting plan only takes the pieces whose %1 cut quantity is filled in "
                            "(Details mode, Dock Options, Piece, Information).")
                             .arg(stopped.material == OExportCuttingGuard::Material::Lining
                                      ? tr("lining")
                                      : tr("fabric"));
        }

        if (error != nullptr)
        {
            *error = message + QStringLiteral("\n\n") + stopped.diagnostic;
        }

        if (VApplication::IsGUIMode())
        {
            QMessageBox::warning(this, tr("Export mode"), message + QStringLiteral("\n\n") + stopped.diagnostic,
                                 QMessageBox::Ok, QMessageBox::Ok);
        }
    };

    if (fabric.stop != PassResult::Stop::None)
    {
        reportStop(fabric);
        return std::nullopt;
    }

    if (lining.stop != PassResult::Stop::None)
    {
        reportStop(lining);
        return std::nullopt;
    }

    // The range is resolved ABOVE, before the sizes are walked: the frozen decor needs it while the
    // document is still on the reference layer's size.

    OExportOutput output;
    // Left nameless on purpose: the name comes from the entry dialog, which this function
    // never sees, and the caller sets it before filing the generation. Nothing downstream
    // reads it here.
    output.name.clear();
    output.measurementCanonical = context.canonical;
    output.sizeAxis = context.axis;
    output.range = exportRange;
    output.hasRange = (range != nullptr);
    output.rangeName = request.rangeName;
    output.documentWidthMm = request.documentWidthMm;

    // Le projecteur n'a pas de laize : ce qu'il a trouvé, c'est un palier de largeur et un espacement
    // qui a pu descendre au plancher. La génération garde **ce qui a été posé**, jamais ce qui avait
    // été demandé — sinon la barre d'état et la recette décriraient un autre plan.
    if (OExportFormat::IsProjector(request.format) && fabric.plan.has_value())
    {
        output.documentWidthMm = fabric.plan->widthMm;
    }

    // What this run was asked for. Kept so the run can be asked again — the Refresh entry
    // replays this exact request, and these values are the part of it nothing else records.
    output.singleSize = request.singleSize;
    output.baseA = request.baseA;
    output.baseB = request.baseB;
    output.baseC = request.baseC;
    output.marginMm = request.marginMm;
    output.gapMm = request.gapMm;
    if (OExportFormat::IsProjector(request.format) && fabric.plan.has_value()
        && fabric.plan->gapMm > 0.0)
    {
        output.gapMm = fabric.plan->gapMm;
    }
    output.unit = request.unit;
    output.printModelToken = request.printModelToken;
    output.columns = request.columns;
    output.frameHeightMm = request.frameHeightMm;
    output.marksTemplateName = request.marksTemplateName;
    output.coverPageTemplateName = request.coverPageTemplateName;
    // **What the run actually generated, not what the request hoped for.** The request's own map is
    // empty in every current path; the values the walk applied are the truth, and the page de garde
    // reads them. Request kept as a fallback for a generation replayed from a favourite.
    output.pinnedAxes = runAxes.isEmpty() ? request.pinnedAxes : runAxes;
    output.groupMemberChoices = request.groupMemberChoices;
    output.format = request.format;
    output.oneWayLayout = params.oneWayLayout;
    output.foldedFabric = request.foldedFabric;
    output.excludedPieces = request.excludedPieces;
    output.matWidthMm = request.matWidthMm;
    output.matHeightMm = request.matHeightMm;
    output.matUnit = request.matUnit;

    // **Le plan de tissu, là où il a toujours été** — `layers` et `result` — et la doublure à côté.
    // Cet ordre est le contrat : tout ce qui n'a pas appris la paire continue de lire le tissu, à
    // commencer par l'écriture DXF / PLT / HPGL, le PDF et l'image.
    output.layers = std::move(fabric.plan->layers);
    output.result = std::move(fabric.plan->result);

    // Le décor des groupes de la couche de référence, résolu pendant la marche des tailles. Il rejoint
    // le résultat ici, et pas ailleurs : c'est la seule chose que le canvas reçoit qui appartienne à
    // UNE génération — une pièce a sa place dans les couches, un groupe n'en a pas.
    output.result.groupDecor = std::move(fabric.plan->groupDecor);

    // ── La doublure, quand le run l'a demandée ────────────────────────────────────────────────────
    //
    // Les deux drapeaux sont posés ENSEMBLE : `liningPlan` dit à la lecture qu'il y a une seconde
    // nappe, et `documentLiningWidthMm` dit sur quelle laize elle s'est coupée. Sans le drapeau, la
    // largeur n'est pas un plan ; sans la largeur, le nom d'un plan sans bac.
    output.liningPlan = lining.plan.has_value();
    output.documentLiningWidthMm = output.liningPlan ? request.documentLiningWidthMm : 0.0;

    if (lining.plan.has_value())
    {
        output.liningLayers = std::move(lining.plan->layers);
        output.liningResult = std::move(lining.plan->result);
        output.liningResult.groupDecor = std::move(lining.plan->groupDecor);
    }

    output.generationMs = generationTimer.elapsed();
    return output;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GenerateNamedExport nest a run, file it under its name, and show it.
 *
 * The one way a generation enters the dock: the entry dialog's own Generate button and
 * every line of its list to generate come through here, so that a run asked for one by one
 * and a run asked for as part of a batch are the same run.
 */
auto MainWindow::GenerateNamedExport(const OExportRunRequest &request, const QString &name,
                                     const OGradedSizeRanges::Context &context) -> bool
{
    // **The window must not be deleted while this runs.** A generation waits on a loop of its own —
    // the nesting engine has no abort hook, so that loop outlives the dialog announcing it — and a
    // deferred deletion is performed by whichever loop is running when control returns to one. A
    // close asked for here would therefore have been carried out by the run's own loop, deleting
    // this window while the run was still working through it, which is a method of it. The close is
    // remembered instead, and granted once the count is back to zero: see MainWindow::closeEvent.
    BeginWorkInFlight();
    const auto workGuard = qScopeGuard([this]() { EndWorkInFlight(); });

    std::optional<OExportOutput> output = RunExportNesting(request, context);
    if (not output.has_value())
    {
        // The failure has already been reported. Nothing is filed and nothing is taken
        // away: the mode stays, and a line that failed keeps its line in the list.
        return false;
    }

    // As given, never uniquified here: the name was reserved where it was derived — against
    // the generations the dock lists and against the lines of the list to generate — and it
    // is the name the user read on the line she asked for. A second uniqueness pass could
    // only rename the generation away from what she agreed to.
    output->name = name;

    const quint32 id = m_exportOutputs.Append(std::move(*output));

    // Raised AFTER the run, never inside it: even if resolving the geometry emitted
    // patternChanged, the state that comes out of a successful generation is "up to
    // date" — otherwise the warning would greet the user on every re-entry, and
    // "nothing changed, show the last generation" would never be true twice.
    m_exportOutputs.SetStale(false);

    RefreshOutputsDock();

    if (const OExportOutput *stored = m_exportOutputs.Find(id))
    {
        ShowExportOutput(*stored, /*zoomFit*/ true);
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RenameExportOutput write the new name from the dock into the store and refresh.
 *
 * The dock already rejected empty and conflicting names before emitting renameRequested —
 * this is the write-through, no validation. The id may be unknown (the store lost the row
 * to a deletion between emit and slot) — in that case the dock is refreshed so its row
 * disappears, and the canvas stays on whatever was shown.
 */
void MainWindow::RenameExportOutput(quint32 id, const QString &newName)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        // The row is gone: a deletion landed between the dock's emit and our slot. Re-pull
        // so the dock stops showing a ghost, and the canvas is left alone — the active one
        // may already point at a real row.
        RefreshOutputsDock();
        return;
    }

    if (newName == existing->name)
    {
        // Defensive: the dock's OnNameEdited trims and compares before emitting, but a
        // trimmed name that happens to equal the stored one is a no-op — still refresh so
        // any concurrent state (active id, visibility filter) reads through.
        RefreshOutputsDock();
        return;
    }

    const QString oldName = existing->name; // read BEFORE the write: the favorite is keyed by it
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
    auto *settings = VAbstractApplication::VApp()->Settings();

    // Renaming a generation that is a favorite carries the favorite with it. Left behind, the
    // star would empty itself on this very row while the favorite still existed under the old
    // name — a dock that says "not a favorite" about a favorite.
    if (!context.canonical.isEmpty()
        && OExportFavorites::IsFavorite(settings, context.canonical, context.axis, oldName))
    {
        if (OExportFavorites::IsFavorite(settings, context.canonical, context.axis, newName))
        {
            // Two favorites under one name are one favorite, so the move is refused — and the
            // dock is re-popped straight away, which is not cosmetic: OnNameEdited has already
            // written the refused text into the cell AND into kPreviousNameRole. Without this
            // refresh the next edit would compare the new name to itself, find them equal, and
            // never emit again — renaming would stop working, silently, for good.
            QMessageBox::warning(this, tr("Export mode"), tr("A favorite already has this name. Choose another name."),
                                 QMessageBox::Ok, QMessageBox::Ok);
            RefreshOutputsDock();
            return;
        }

        // Read, then remove, then store: RequestFromFavorite after the removal would read a
        // recipe that is no longer there.
        const OExportRunRequest recipe =
            OExportFavorites::RequestFromFavorite(settings, context.canonical, context.axis, oldName);
        OExportFavorites::RemoveFavorite(settings, context.canonical, context.axis, oldName);
        OExportFavorites::StoreFavorite(settings, context.canonical, context.axis, newName, recipe);
    }

    m_exportOutputs.SetName(id, newName);
    RefreshOutputsDock();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToggleExportFavorite(quint32 id)
{
    const OExportOutput *output = m_exportOutputs.Find(id);
    if (output == nullptr)
    {
        // The row is gone between the emit and this slot — nothing to key a favorite on.
        RefreshOutputsDock();
        return;
    }

    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
    if (context.canonical.isEmpty())
    {
        // No measurements table, so no key: a favorite stored here would be shared with
        // nobody. The star is drawn disabled for that reason — this is the same door, shut
        // once more for the case where the table was unloaded after the dock was filled.
        return;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const QString name = output->name;

    if (OExportFavorites::IsFavorite(settings, context.canonical, context.axis, name))
    {
        OExportFavorites::RemoveFavorite(settings, context.canonical, context.axis, name);
    }
    else
    {
        // The recipe of the run as it stands — what a Refresh would replay — minus the two
        // fields that speak of this pattern's pieces: those are asked again, once per batch,
        // in the pre-flight dialog (see OExportFavorite).
        OExportFavorites::StoreFavorite(settings, context.canonical, context.axis, name, RequestOf(*output));
    }

    // `output` is not used below: the refresh rebuilds the dock, and the store may have
    // moved under it.
    RefreshOutputsDock();
}

/**
 * @brief SetActiveExportOutput show the generation @p id in the canvas.
 *
 * This is the whole point of keeping a generation: the click recomputes nothing. No
 * nesting, no geometry resolution — the canvas is handed what the generation already
 * carries, exactly as on the first display.
 *
 * The result is framed like the first display, because each generation is a different
 * bin: keeping the previous zoom would leave a larger result half off-screen. The
 * already-active row is left alone, so re-clicking it cannot throw away a zoom the user
 * set herself.
 */
void MainWindow::SetActiveExportOutput(quint32 id)
{
    const OExportOutput *output = m_exportOutputs.Find(id);
    if (output == nullptr || m_exportOutputs.ActiveId() == id)
    {
        return;
    }

    m_exportOutputs.SetActiveId(id);
    ShowExportOutput(*output, /*zoomFit*/ true);
    RefreshOutputsDock();
}

/**
 * @brief Redraws the shown generation with what has just been edited under it.
 *
 * **TWO things are drawn under a plan, and this slot serves both**: the template of assembly marks,
 * saved from the editor, and the cover page, written from its dock. Neither moves a piece, so both
 * are handed the same way.
 *
 * The nesting is NOT re-run. Nothing about the plan changed — the decor is drawn under it — so the
 * generation is handed back to the canvas exactly as it was, the same redraw the manual correction
 * asks for. That keeps the zoom the user set and spares a run of several minutes for a frame that
 * moved by a point, or for a comma in a free text.
 *
 * Nothing happens when no generation is shown, which is the common case: the editor is a window of
 * its own, and a user may edit templates before nesting anything. And nothing happens in another
 * draw mode — ShowExportOutput guards that itself, because its scene is not the one on screen then.
 */
void MainWindow::RefreshExportDrawing()
{
    const OExportOutput *output = m_exportOutputs.Find(m_exportOutputs.ActiveId());
    if (output == nullptr)
    {
        return;
    }

    ShowExportOutput(*output, /*zoomFit*/ false);
}

/**
 * @brief RefreshExportOutput re-run the nesting of the generation @p id, and keep the row.
 *
 * This is the entry the staleness warning points at (§3.1 of the spec): the user decides
 * *whether* to regenerate and *which* one, and nothing here happens on its own. The run is
 * replayed with what the generation recorded when it was made — a generation carries what
 * it takes to reproduce itself. The context, though, is re-resolved rather than replayed:
 * the row is listed because it matches the current one, so the current one is the right
 * one.
 *
 * The identity and the name survive — the result is new, the row is not. The refreshed
 * generation is also the one that gets shown, in every case: the run takes minutes, and
 * leaving the canvas on another result would make the click look like it did nothing.
 *
 * Nothing here is undoable, like the rest of the dock: a generation is session-scoped and
 * never part of the document, so it has no place on the pattern's undo stack.
 */
void MainWindow::RefreshExportOutput(quint32 id)
{
    // The fields are read into the call before the run starts, so the pointer going stale
    // during it — the run shows a dialog and pumps events — is not a hazard.
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        return; // the row is gone; there is nothing to regenerate
    }

    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);

    std::optional<OExportOutput> output = RunExportNesting(RequestOf(*existing), context);
    if (not output.has_value())
    {
        return; // the failure has already been reported, and the old result is still there
    }

    m_exportOutputs.Replace(id, std::move(*output));

    // The history is dropped, and it has to be: every command in it names a placement by its
    // index, and the run above has just written a whole new set of them. The indexes still
    // exist, they just no longer mean the same thing — an undo would move a piece to where a
    // DIFFERENT result had put it, silently, which is the one failure mode worse than losing
    // the history. A stack cannot be pruned command by command without breaking the order the
    // redo side depends on, so it goes entirely.
    m_exportCorrection->Reset();

    // Same rule as a fresh generation: what comes out of a successful run is up to date.
    m_exportOutputs.SetStale(false);

    m_exportOutputs.SetActiveId(id);
    RefreshOutputsDock();

    if (const OExportOutput *refreshed = m_exportOutputs.Find(id))
    {
        ShowExportOutput(*refreshed, /*zoomFit*/ true);
    }
}

/**
 * @brief DeleteExportOutput drop the generation @p id, once the user has confirmed.
 *
 * A generation is session-scoped — it is never written to the .ostara (§6.1 of the spec) —
 * so this removes nothing from the document and **Undo cannot bring it back**. That is not
 * a shortcoming to hide: the confirmation says it in as many words, because the user of
 * this mode will reach for Ctrl+Z and must not be surprised.
 *
 * The guard lives here rather than in the dock so that every caller is covered — the
 * confirmation is part of what deleting means, not a property of the button that happens
 * to call it today.
 */
void MainWindow::DeleteExportOutput(quint32 id)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        return;
    }

    const QString name = existing->name;

    if (QMessageBox::question(this, tr("Export mode"),
                              tr("Are you sure you want to delete \"%1\"?\n\n"
                                 "This action cannot be undone.")
                                  .arg(name),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
        != QMessageBox::Yes)
    {
        return; // the row stays, and the canvas is untouched
    }

    m_exportOutputs.Remove(id);

    // The canvas follows the list. ActiveVisibleOutput() answers exactly this question —
    // the active generation when the filter still lists it, otherwise the last one it
    // keeps — so the deletion lands on the rule the mode already uses instead of a second
    // one written here.
    if (const OExportOutput *next = ActiveVisibleOutput())
    {
        m_exportOutputs.SetActiveId(next->id);
        ShowExportOutput(*next, /*zoomFit*/ true);
    }
    else
    {
        // Nothing left to show. The scene is emptied the way a pattern load empties it, and
        // the scene rect is recomputed — otherwise the view keeps framing a result that is
        // no longer there. The status label said the size of what was displayed, so it goes
        // with it.
        //
        // The correction is told before the clear, because this branch reaches neither of the
        // two places that would normally tell it: no ShowExportOutput runs, so no
        // OnSceneAboutToBeRebuilt, and clear() destroys the holder straight out from under it.
        // The id 0 is not the generation being deleted — it is "nothing is shown", which is
        // exactly the situation, and the correction drops the selection on that answer.
        if (m_exportCorrection != nullptr)
        {
            m_exportCorrection->OnSceneAboutToBeRebuilt(0);
        }

        m_sceneExport->clear();
        VMainGraphicsView::NewSceneRect(m_sceneExport, ui->view);
        m_statusLabel->clear();
    }

    RefreshOutputsDock();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportOutputToPdf(quint32 id)
{
    // Un plan de coupe a **son** fichier, et pas un PDF : DXF, PLT ou HPGL, à 100 %, avec les
    // options de l'export du mode Details moins l'échelle (CuttingPlan §24). Le même bouton du dock
    // l'ouvre — le dock reste le seul endroit qui décide ce qu'un run produit, sans une colonne de
    // plus ni un menu.
    if (const OExportOutput *cutting = m_exportOutputs.Find(id);
        cutting != nullptr && cutting->format == OExportFormat::Kind::CuttingPlan)
    {
        ExportOutputToCuttingFile(id);
        return;
    }

    // Un run plotter ne sort pas en PDF : son fichier est un SVG structuré, **un objet par morceau,
    // pas un mot**. Le même bouton ouvre le dialogue de son type de run — c'est ainsi que le dock
    // reste le seul endroit qui décide ce qu'un run produit, sans une colonne de plus ni un menu.
    if (const OExportOutput *plotter = m_exportOutputs.Find(id);
        plotter != nullptr && plotter->format == OExportFormat::Kind::Plotter)
    {
        ExportOutputToPlotterSvg(id);
        return;
    }

    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        return;
    }

    // Où il s'ouvre : le dossier de la gamme quand elle en a un — les autres fichiers de ce plan y
    // sont déjà — sinon le dernier dossier d'export, sinon Documents. La règle est celle
    // d'`ExportFolder`, et les trois exports la partagent.
    OExportPdfDialog dialog(existing->name, ExportFolder(*existing), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    // The very template the canvas draws with, resolved by the very same call — so the paper
    // cannot carry marks the screen did not show, nor miss ones it did.
    OExportTemplate marks;
    bool framed = false;
    const bool hasMarks = ExportMarksOf(*existing, marks, framed);

    // The very cover page the canvas draws with, resolved by the very same call against the live
    // document. Everything the page states comes from the RUN — the sizes of a range, the values
    // the frozen axes hold — so what is delivered is what the paper carries and not what the
    // template demonstrated; and what belongs to the pattern is re-read here, which is what makes
    // an edit need no new nesting. An empty one is a state and not an error: "No cover page" is
    // one of the answers the print widget offers.
    const OExportCoverPage::Drawing cover = OExportCoverPage::Of(*existing, doc);

    // The dialog's one choice, straight through: whether the layout is written as a layer the
    // reader can switch off. It opens on no, and no is the answer that cannot lose anything. The
    // cover page is not that choice and never was: it goes out of every layer whatever this says.
    // ── Le dessin du mode Graded, écrit par SON chemin ─────────────────────────────────────────
    //
    // ⚠️ **C'est le fichier du mode Graded qui part dans le PDF, pas le dessin de ce run.** Le canvas
    // d'export dessine le PLAN — pièces nestées et tournées — et refabrique les enfants par une règle
    // qui n'est pas celle de l'écran : mesuré, le droit-fil de « Manche » sortait à 0.796 de la pièce
    // là où le mode Graded est à 0.853. Le mode Graded écrit les pièces à plat et UN décor, celui
    // qu'on voit : c'est ce document-là que la cliente doit retrouver dans Rabnest.
    //
    // Écrit dans un dossier temporaire qui vit jusqu'à la fin de l'appel — le PDF le lit avant.
    QTemporaryDir gradedDir;
    QString gradedDrawing;
    if (existing->hasRange && !existing->rangeName.isEmpty() && gradedDir.isValid())
    {
        QString written;
        QString gradedError;
        // La copie du payload : même image, texte en CONTOURS — celui qui part dans le PDF.
        QString payloadCopy = gradedDir.filePath(QStringLiteral("graded-payload.svg"));
        if (ExportGradedRangeTo(existing->rangeName, gradedDir.path(), false, &written, &gradedError,
                                &payloadCopy)
            && !payloadCopy.isEmpty())
        {
            gradedDrawing = payloadCopy;
        }
    }

    if (!OExportPdf::Write(*existing, dialog.FullPath(), pattern, doc,
                           hasMarks ? &marks : nullptr, framed, dialog.AddLayoutLayer(),
                           cover.IsEmpty() ? nullptr : &cover, dialog.CarriesPayload(),
                           gradedDrawing))
    {
        QMessageBox::warning(this, tr("Export mode"),
                             tr("\"%1\" could not be exported.").arg(existing->name));
        return;
    }

    // Le dossier vient d'être choisi : c'est celui que le prochain export proposera.
    RememberExportFolder(dialog.Directory());

    QMessageBox::information(this, tr("Export mode"),
                             tr("\"%1\" was exported to\n%2").arg(existing->name, dialog.FullPath()));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportOutputToPlotterSvg(quint32 id)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr || !existing->result.ok)
    {
        return;
    }

    // Où il s'ouvre : le dossier de la gamme quand elle en a un, sinon le dernier dossier d'export,
    // sinon Documents — la règle d'`ExportFolder`, celle du PDF.
    //
    // Un seul dialogue, et rien à y demander : ni format — c'est un SVG — ni taille, ni résolution.
    // Les trois casseraient le plan, et sans le dire : les lignes de coupe cesseraient d'être à des
    // multiples **exacts** du tapis, et le plotter couperait au mauvais endroit.
    //
    // **Le même dialogue que l'export d'un print**, et non la boîte du système : le nom s'y propose
    // au lieu de se taper, le dossier est celui du run, et un fichier qui existe déjà se demande
    // avant d'être écrasé — au lieu de l'être sans un mot. Le suffixe est la seule chose qui change,
    // et c'est le seul endroit où il est écrit.
    OExportFileDialog dialog(existing->name, ExportFolder(*existing), QStringLiteral(".svg"), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        // Elle a fermé le dialogue : rien à dire, et rien à faire.
        return;
    }

    if (!OExportPlotterSvg::Write(dialog.FullPath(), *existing, pattern, doc))
    {
        QMessageBox::warning(this, tr("Export mode"),
                             tr("\"%1\" could not be exported.").arg(existing->name));
        return;
    }

    // Le dossier vient d'être choisi : c'est celui que le prochain export proposera.
    RememberExportFolder(dialog.Directory());

    QMessageBox::information(this, tr("Export mode"),
                             tr("\"%1\" was exported to\n%2").arg(existing->name, dialog.FullPath()));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportOutputToCuttingFile(quint32 id)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr || !existing->result.ok)
    {
        return;
    }

    // Où il s'ouvre : le dossier de la gamme quand elle en a un, sinon le dernier dossier d'export,
    // sinon Documents — la règle d'`ExportFolder`, celle du PDF et du plotter.
    //
    // Ce qu'on y demande est ce que le mode Details demande pour les mêmes moteurs : le format, son
    // sous-type, et les options — **sauf l'échelle**. Un plan de coupe sort à 100 %, toujours : il
    // part chez une machine, et une échelle réduite produirait un plan qui a l'air juste et qui
    // coupe faux (`CuttingPlan.md` §24.2).
    OExportCuttingFileDialog dialog(*existing, ExportFolder(*existing), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        // Elle a fermé le dialogue : rien à dire, et rien à faire.
        return;
    }

    // ── Un fichier par plan ───────────────────────────────────────────────────────────────────────
    //
    // **Ce que le dialogue a nommé, et dans son ordre.** Un plan de tissu seul donne un fichier, comme
    // avant la doublure ; une paire en donne deux, chacun avec sa matière et son nom. On écrit ce que
    // les lignes disent, sans recompter les nappes ici : le dialogue est le seul à savoir laquelle est
    // laquelle.
    QStringList written;
    QStringList failed;
    for (const OExportCuttingFile::Target &target : dialog.Targets())
    {
        QString why;
        if (OExportCuttingFile::Write(target.path, *existing, dialog.Options(), target.material, &why))
        {
            written.append(target.path);
        }
        else
        {
            // Le chemin, et POURQUOI quand l'écrivain le sait : un plan généré avant la géométrie sans
            // marges se régénère, et une boîte qui ne dit que « ça n'a pas marché » laisse chercher.
            failed.append(why.isEmpty() ? target.path : target.path + QStringLiteral("\n") + why);
        }
    }

    // Le dossier vient d'être choisi : c'est celui que le prochain export proposera — et il l'est même
    // quand l'un des fichiers a échoué, parce que le dossier, lui, existe.
    RememberExportFolder(dialog.Directory());

    if (not failed.isEmpty())
    {
        // **Ce qui n'a PAS été écrit est nommé.** Sur une paire, dire seulement que « la sortie » a
        // échoué laisserait croire que rien n'est sorti — ou que tout l'est, si le premier fichier est
        // passé. Les deux cas se ressemblent à l'écran, et un seul est vrai.
        QMessageBox::warning(this, tr("Export mode"),
                             tr("\"%1\" could not be exported.").arg(existing->name)
                                 + QStringLiteral("\n\n") + failed.join(QLatin1Char('\n')));
        return;
    }

    QMessageBox::information(
        this, tr("Export mode"),
        tr("\"%1\" was exported to\n%2").arg(existing->name, written.join(QLatin1Char('\n'))));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportActiveOutputToImage()
{
    // A click that arrives after the mode was left must not photograph another mode's scene —
    // the same guard, on the same thing, that ShowExportOutput opens on.
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Export)
    {
        return;
    }

    // What the canvas is showing, read by the rule the mode already has. The scope button speaks
    // for the plan on screen and for no other — a toolbar button has no row to point at.
    const OExportOutput *active = ActiveVisibleOutput();

    const QVector<quint32> ids = VisibleExportOutputIds();
    if (ids.isEmpty())
    {
        return; // nothing the dock lists, so nothing to offer and nothing to write
    }

    // Step 1 opens its size field on the plan of the generation on show — the only figure that is
    // not a guess. Without one, on the first the dock lists: the dialog still has to open on
    // something, and what it carries is a starting point for a field about to be changed.
    const OExportOutput *reference =
        active != nullptr ? active : m_exportOutputs.Find(ids.constFirst());
    if (reference == nullptr)
    {
        return;
    }

    const OExportPlanImage::Plan referencePlan =
        OExportPlanImage::PlanOf(ComputeExportDrawing(*reference).drawnRect);

    // Le dossier de la gamme quand elle en a un — la règle d'`ExportFolder`, celle du PDF : les deux
    // exports d'une même génération atterrissent côte à côte.
    OExportImageDialog options(ExportFolder(*reference), referencePlan, active != nullptr, this);
    if (options.exec() != QDialog::Accepted)
    {
        return;
    }

    // Step 2: which generations, and under which names. One line for one generation is the same
    // question as one line for six, so it is the same dialog.
    QVector<OExportImageListDialog::Row> rows;
    if (options.Chosen() == OExportImageDialog::Scope::Active)
    {
        // The button that answers this is dead when nothing is shown, so the pointer is not
        // expected to be null — reading it anyway costs one line and cannot be a crash.
        if (active != nullptr)
        {
            rows.append({active->id, active->name});
        }
    }
    else
    {
        for (const quint32 id : ids)
        {
            if (const OExportOutput *output = m_exportOutputs.Find(id))
            {
                rows.append({output->id, output->name});
            }
        }
    }

    OExportImageListDialog list(rows, options.Format(), this);
    if (list.exec() != QDialog::Accepted)
    {
        return;
    }

    // The run. Every generation is drawn into **a scene of its own** — the canvas holds the one on
    // show and no other, so a batch cannot borrow it — and a scene built here carries no correction
    // holder: that one is mounted on the displayed scene alone, which is why no image can hold a
    // dashed frame and four handles meant for the hand.
    QStringList written;
    QStringList failed;

    for (const OExportImageListDialog::Row &row : list.Chosen())
    {
        const OExportOutput *output = m_exportOutputs.Find(row.id);
        if (output == nullptr)
        {
            continue;
        }

        const OExportDrawing drawing = ComputeExportDrawing(*output);
        const OExportPlanImage::Plan plan = OExportPlanImage::PlanOf(drawing.drawnRect);

        // An SVG's page IS a physical size, and PrintDPI is what expresses it; a PNG's pixels ARE
        // the size, so its resolution is the user's. One figure for the whole run — the size was
        // asked for once — and each generation's own proportions give it its other side.
        const OExportPlanImage::Target target = OExportPlanImage::SizeFor(
            plan, options.TargetMm(), options.Axis(),
            options.Format() == OExportPlanImageWriter::Format::Svg ? PrintDPI : options.Dpi());

        QGraphicsScene scene;
        DrawExportDrawing(&scene, *output, drawing);

        const QString path = QDir(options.Directory()).filePath(row.name);
        if (OExportPlanImageWriter::Write(path, &scene, plan, target, options.Format(), output->name))
        {
            written.append(row.name);
        }
        else
        {
            // A generation that cannot be written does not stop the others. Its line is said at the
            // end with the rest, which is the only place that knows which generation it was.
            failed.append(row.name);
        }
    }

    // Le dossier où le run vient d'écrire — s'il a écrit quelque chose : c'est celui que le prochain
    // export proposera.
    if (!written.isEmpty())
    {
        RememberExportFolder(options.Directory());
    }

    // ONE dialog for the whole run, whatever its length: a box per file would be a box the user
    // has to dismiss six times to learn one thing.
    QStringList lines;
    if (!written.isEmpty())
    {
        lines << tr("%n file(s) written to %1", "", static_cast<int>(written.size()))
                     .arg(options.Directory());
        lines << QString();
        lines << written;
    }

    if (failed.isEmpty())
    {
        QMessageBox::information(this, tr("Export mode"), lines.join(QLatin1Char('\n')));
        return;
    }

    if (!lines.isEmpty())
    {
        lines << QString();
    }
    lines << tr("%n file(s) could not be written:", "", static_cast<int>(failed.size()));
    lines << failed;

    QMessageBox::warning(this, tr("Export mode"), lines.join(QLatin1Char('\n')));
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::ExportMarksOf(const OExportOutput &output, OExportTemplate &marks, bool &framed) const
    -> bool
{
    // By NAME, and resolved afresh every time — never a copy taken at nesting time. A template
    // is a file the user edits, and a run that had copied its frame and its marks would go on
    // drawing what the file said the day it was nested, while the editor shows something else:
    // the drawing would then be lying about the very thing it is there to show.
    const bool hasMarks = OExportTemplateStorage::Resolve(output.marksTemplateName, &marks);

    // Whether the format carries a frame — the two framed formats, and them alone. Read off the
    // table rather than guessed from the geometry: the frame is a property of the model, and its
    // sheet happens to be the size the frame is.
    const OExportPrintModel *model = FindPrintModel(output.printModelToken);
    framed = model != nullptr && model->hasFrame;

    return hasMarks;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::ComputeExportDrawing(const OExportOutput &output) const -> OExportDrawing
{
    OExportDrawing drawing;

    // How the plan is cut, when the run is tiled — computed from the generation alone, since
    // that is all a kept run has. An invalid grid means one continuous sheet, and the canvas
    // then draws its bin exactly as it always did.
    drawing.artboards = OExportArtboards::Compute(
        output.result.binWidthPx, output.result.usedHeightPx, output.frameHeightMm, output.columns);

    // **The cover page is resolved here, and nowhere else on this path.** The template by name, the
    // document read live, and the values filled FROM THE RUN (§6) — so a page the user edits is the
    // one redrawn, without a nesting being run. The canvas is handed the result: it places and it
    // paints, and it looks nothing up.
    drawing.cover = OExportCoverPage::Of(output, doc);

    // The block, when the format keeps the page in a sheet — the SAME figure the canvas will hand
    // to its own Compute, read from the same rule. The extra sheet is not passed: it is not a cell.
    const QRectF coverReserved =
        (drawing.cover.seat == OExportCoverPage::Seat::InSheet && drawing.artboards.valid)
            ? OExportCoverPage::PaperPx(drawing.cover.seat, drawing.cover.marginPx,
                                        drawing.artboards.planRect, drawing.artboards.cellWidthPx,
                                        drawing.artboards.cellHeightPx)
            : QRectF();

    drawing.sheets =
        OExportSheets::Compute(drawing.artboards, output.result.placements, coverReserved);

    // What the canvas is about to draw, computed ONCE. Three readers want it — the rectangle a
    // hand may not leave, the image an export writes, and the figures of the status label — and
    // three readings of the same thing are three answers waiting to differ.
    drawing.drawnRect =
        drawing.artboards.valid
            ? drawing.artboards.planRect
            : QRectF(0.0, 0.0, output.result.binWidthPx, output.result.usedHeightPx);

    return drawing;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::DrawExportDrawing(QGraphicsScene *scene, const OExportOutput &output,
                                   const OExportDrawing &drawing) const
{
    // The template of assembly marks, resolved here and now rather than stored in the run: the
    // file behind the name can be edited, renamed or deleted between two rebuilds, and what is
    // drawn has to be what the file says today. A name that resolves to nothing falls back to the
    // shipped template, and a library with neither draws no marks at all.
    OExportTemplate marks;
    bool framed = false;
    const bool hasMarks = ExportMarksOf(output, marks, framed);

    const OExportRunFlags flags{output.format, output.foldedFabric, output.matWidthMm,
                                output.matHeightMm, output.result.matCount};

    // Le plan de TISSU ouvre la scène — c'est lui qui la vide, et il part de son origine.
    OExportCanvas::Build(scene, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc, nullptr,
                         drawing.artboards, hasMarks ? &marks : nullptr, framed, &drawing.cover,
                         flags);

    // ── Et la doublure vient à sa DROITE, dans la même scène ─────────────────────────────────────
    //
    // **C'est ici, et nulle part ailleurs, que la paire se pose.** La scène ne se vide qu'une fois —
    // `clearScene` reste au premier plan — et le second s'ajoute au premier à l'endroit qui lui est
    // réservé. Une seule scène pour les deux : c'est ce qui fait qu'on les lit ensemble, qu'un seul
    // zoom les cadre tous les deux, et que la barre de statut parle d'un run et non de deux.
    //
    // **Elle ne porte ni artboards, ni marques, ni page de garde** : un plan de coupe n'en a aucun, et
    // ses propres drapeaux de run le redisent — à commencer par son compte de tapis, qui est zéro.
    if (output.liningPlan)
    {
        OExportPlanSeat seat;
        seat.origin = LiningOrigin(drawing.drawnRect);
        seat.clearScene = false;

        OExportCanvas::Build(
            scene, output.liningLayers, output.liningResult,
            output.hasRange ? &output.range : nullptr, pattern, doc, nullptr, {}, nullptr, false,
            nullptr,
            {output.format, output.foldedFabric, output.matWidthMm, output.matHeightMm,
             output.liningResult.matCount},
            seat);
    }
}

/**
 * @brief ShowExportOutput draw a kept generation in the Export scene.
 *
 * Recomputes nothing — no nesting, and no geometry resolution: it hands the canvas what
 * the generation carries. That is the whole point of keeping one.
 *
 * The decor, though, is still resolved against the live pattern and document
 * (OExportCanvas::Build reads them for the grainlines, the groups, the information block
 * and the per-size trace styles). Placements and geometry are frozen; the decor follows.
 * Whatever is to be frozen must be owned by the generation first.
 */
void MainWindow::ShowExportOutput(const OExportOutput &output, bool zoomFit)
{
    // A click that arrives after the mode was left must not clear another mode's scene.
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Export)
    {
        return;
    }

    // The figures of the drawing and the drawing itself are two calls, so that the correction can
    // slip between them: it has to be told the scene is ABOUT to be rebuilt while the item holding
    // its mouse grab still exists. Both come from the one place that decides what is drawn.
    const OExportDrawing drawing = ComputeExportDrawing(output);

    if (m_exportCorrection != nullptr)
    {
        // **Une PAIRE ne se corrige pas à la main** — le repère d'un item est l'index de son placement
        // dans le résultat, et les deux nappes en ont chacune un : le même index y désigne une pièce
        // dans les deux. Dit ici, une fois, avant que la scène soit refaite : voir
        // OExportCorrection::SetEnabled, qui jette au passage une sélection et un historique devenus
        // faux.
        m_exportCorrection->SetEnabled(not output.liningPlan);

        // The rectangle a hand may not leave is the one the canvas is about to draw: the
        // sheets when the run is tiled — the dead band of the last row included, since it is
        // paper the nesting already paid for — and the bin otherwise. Read from the same grid
        // Build gets, never from a second computation of the same thing.
        m_exportCorrection->SetPlan(drawing.drawnRect);

        // Before the rebuild, which empties the scene: a gesture in flight has to be abandoned
        // while the item holding the mouse grab still exists, and a selection made on another
        // generation has to fall, because a placement index means nothing in a new result.
        m_exportCorrection->OnSceneAboutToBeRebuilt(output.id);
    }

    DrawExportDrawing(m_sceneExport, output, drawing);

    if (m_exportCorrection != nullptr)
    {
        // After the rebuild: the holder is not something Build draws — it must never reach the
        // PDF, which calls the same Build for its own scene — so it is mounted here, on the
        // screen's scene alone, and dressed on the footprint that was selected.
        m_exportCorrection->OnSceneRebuilt(output);
        m_exportCorrection->SetViewScale(ui->view->transform().m11());
    }

    if (zoomFit)
    {
        // Frame the whole nesting, with air around it: a plain "Zoom fit best" would run the
        // plan against all four edges of the viewport, and a plan is read as a whole — its
        // border is not what the user came to look at. The margin is one ZoomOut step past
        // the fit, which is what the mode wants to open on every time.
        ui->view->ZoomFitBestFramed();
    }

    // The status label belongs to what IS shown, not to the run that produced it. Ce qu'elle dit
    // dépend du genre de plan : les deux formes sont côte à côte dans ExportResultText, avec ce qui
    // les justifie — un plan de coupe parle laize et métrage dans les deux systèmes d'unités, un run
    // d'impression dit l'étendue du papier et son nombre de pages.
    m_statusLabel->setText(ExportResultText(output, drawing));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VisibleExportOutputIds the generations of the current (table, axis) context.
 *
 * A generation made under another measurements table is not listed — the same rule the
 * Size Ranges dock lists a range by, asked of the same foyer rather than copied.
 */
auto MainWindow::VisibleExportOutputIds() const -> QVector<quint32>
{
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);

    QVector<quint32> ids;
    const QVector<OExportOutput> &outputs = m_exportOutputs.Outputs();
    ids.reserve(outputs.size());
    for (const OExportOutput &output : outputs)
    {
        if (OGradedSizeRanges::MatchesContext(output.measurementCanonical, output.sizeAxis, context))
        {
            ids.append(output.id);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ActiveVisibleOutput the generation to show: the active one when the filter lists
 *        it, otherwise the last one the filter keeps.
 *
 * The filter can hide the active generation — the measurements table changed under the
 * session — and showing a row the list does not carry would be an invisible selection.
 */
auto MainWindow::ActiveVisibleOutput() const -> const OExportOutput *
{
    const QVector<quint32> visible = VisibleExportOutputIds();
    if (visible.isEmpty())
    {
        return nullptr;
    }

    if (const OExportOutput *active = m_exportOutputs.Find(m_exportOutputs.ActiveId());
        active != nullptr && visible.contains(active->id))
    {
        return active;
    }

    return m_exportOutputs.Find(visible.constLast());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RefreshOutputsDock push the filtered rows to the Outputs dock.
 *
 * The dock never sees the store: the rule lives here, with the only owner of the current
 * context, and the dock only draws what it is handed.
 */
void MainWindow::RefreshOutputsDock()
{
    OWidgetOutputs *widget = m_dockManager != nullptr ? m_dockManager->outputsWidget() : nullptr;
    if (widget == nullptr)
    {
        return;
    }

    // The favorites of the context, read ONCE for the whole list and not once per row: each
    // of those reads opens the settings file, and the Size Ranges dock had to grow the same
    // cache for the same reason.
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
    auto *settings = VAbstractApplication::VApp()->Settings();
    const QSet<QString> favorites = OExportFavorites::Names(settings, context.canonical, context.axis);
    const bool canFavorite = !context.canonical.isEmpty();

    QVector<OExportOutputRow> rows;
    const QVector<quint32> visible = VisibleExportOutputIds();
    rows.reserve(visible.size());
    for (quint32 id : visible)
    {
        if (const OExportOutput *output = m_exportOutputs.Find(id))
        {
            rows.append(OExportOutputRow{output->id, output->name, favorites.contains(output->name),
                                         canFavorite, output->format == OExportFormat::Kind::CuttingPlan});
        }
    }

    widget->SetOutputs(rows);
    widget->UpdateList(m_exportOutputs.ActiveId());
}
