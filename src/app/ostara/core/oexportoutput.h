/************************************************************************
 **
 **  @file   oexportoutput.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  One generation of the Export mode — the result of a nesting run,
 **          kept as it was produced — and the session store that holds them.
 **
 **          Nothing here ever reaches the .ostara: a result filed in the
 **          document would read as pattern data, and therefore as something
 **          that is up to date. Generations live for the session only.
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

#ifndef OEXPORTOUTPUT_H
#define OEXPORTOUTPUT_H

#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>

#include "oexportformat.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"
#include "../vpatterndb/osizerange.h"
#include "../vmisc/def.h"

/**
 * @brief One generation: the outcome of a nesting run, kept as it was produced.
 *
 * It carries everything the canvas needs to draw it again — the resolved geometry
 * (one layer per size of the range) and the placements — plus the identity the dock
 * lists it by. The range is a copy: a generation is frozen, so editing or deleting
 * the live range must not move it.
 *
 * The (measurements table, declared axis) pair is the context it was generated under.
 * The dock lists a generation only when that pair matches the current one, exactly as
 * the Size Ranges dock lists a range only within its own pair.
 */
struct OExportOutput
{
    quint32 id{0};  ///< stable identity — a row index is not one
    QString name{}; ///< what the user named it in the entry dialog; unique in its context

    QString measurementCanonical{}; ///< absolute path of the table it was generated under
    QString sizeAxis{};             ///< declared grading axis at that moment; empty lifts the filter

    OSizerange range{};   ///< copy of the range, as it was when the run started
    bool hasRange{false}; ///< false for a single-size run
    QString rangeName{};  ///< the range name — the label, and the key the size styles resolve by

    /// The width the nesting ran on, in MILLIMETRES. Not in the pattern's unit: the width
    /// is now derived from the print model, which is a standard in millimetres before it is
    /// anything on screen — storing what the user happened to be reading would change what
    /// the stored value means the day she switched units, and every replay with it.
    qreal documentWidthMm{0.0};

    QVector<OExportSizeLayer> layers{}; ///< the pattern resolved for every size
    OExportNestingResult result{};      ///< the placements, and the bin they were placed in

    /// @brief La laize de la DOUBLURE, en millimètres — la seconde matière du run.
    ///
    /// Elle est à `documentWidthMm` ce que la doublure est au tissu : une autre étoffe, une autre
    /// laize, et rien de commun sinon les réglages. **Zéro quand le run n'a pas demandé de paire.**
    qreal documentLiningWidthMm{0.0};

    /// @brief Whether this generation was asked for as a PAIR: the fabric plan, and a lining plan.
    ///
    /// Un plan de coupe peut décrire **deux** plans — le tissu et la doublure — et c'est ce drapeau
    /// qui les fait exister. Ce qui les sépare n'est pas un réglage : c'est le **jeu de pièces**,
    /// chacune lue dans sa propre matière (voir OExportCuttingGuard::Material). Tout le reste est
    /// commun : à la laize près, la doublure reprend les réglages du tissu à l'identique.
    bool liningPlan{false};

    /// Le plan de DOUBLURE : mêmes deux listes, même contrat que `layers` et `result` ci-dessus —
    /// qui restent le plan de TISSU, et que tout l'existant continue de lire.
    ///
    /// ⚠️ **Le plan de tissu reste là où il a toujours été**, dans `layers` et `result` : l'écriture
    /// DXF / PLT / HPGL, le PDF et l'image lisent ces deux membres-là et n'ont pas eu à apprendre la
    /// paire. La doublure vit à côté, et seuls le canvas et la barre de statut la connaissent.
    QVector<OExportSizeLayer> liningLayers{};
    OExportNestingResult liningResult{};

    /// How long the run took, in milliseconds — the figure the status bar reports. Measured from
    /// the first line of the run to its last, so that it covers everything the user waited
    /// through: the geometry resolved size after size, the nesting itself, and the event loop
    /// both are waited on. A figure that left the geometry out would be one she could not
    /// reconcile with her own stopwatch.
    ///
    /// An OUTCOME and not something asked for, which is why it is absent from OExportRunRequest
    /// below: nothing replays it. A Refresh measures its own; a generation merely shown again —
    /// another row selected in the dock, an undo of a manual correction, the mode re-entered —
    /// keeps the one it was made with, and that is the whole reason it is kept here rather than
    /// in the window that draws the label.
    qint64 generationMs{0};

    /// What the run was asked for, kept so that it can be asked again — this is what makes
    /// Refresh possible.
    ///
    /// This is not "the configuration that produced the result", which §3 of the spec
    /// deliberately keeps out: it is the strict minimum to reproduce the run, and the run
    /// cannot be replayed without it. The width and the range name are already above; the
    /// three bases are not recoverable from anywhere else — in the single-size path they
    /// are what puts the document on a size, and the generations carry no other trace of
    /// them.
    bool singleSize{false}; ///< true when the dialog asked for one size, not a whole range
    qreal baseA{0.0};       ///< the dimension bases the dialog pinned (single-size path only)
    qreal baseB{0.0};
    qreal baseC{0.0};

    /// The margin and the gap belong to that same list, and for the same reason: both move
    /// the pieces. A replay that guessed them would draw a plan nobody asked for while
    /// claiming to be refreshing the one on screen.
    /// L'espace gardé depuis les bords, en millimètres — **deux onglets, deux noms, une grandeur**.
    /// L'onglet Print l'appelle sa marge de document ; l'onglet Cutting plan, l'espace depuis les
    /// bords du tissu. Sur un plan de coupe plié, le nesting la répartit par rectangle et **le bord
    /// d'un pli n'en paie rien** — voir `SeatMargins`, dans oexportnesting.cpp.
    qreal marginMm{5.0};
    qreal gapMm{5.0};    ///< the space between two pieces, in millimetres

    /// The unit the run was entered in — the one the dialog's own fields were read and written in,
    /// and, on the print tab, the one the status figures come back in, so that what the user has just
    /// read in the dialog is what returns to her. It changes **nothing** about the result: every
    /// length is stored in millimetres.
    ///
    /// ⚠️ **Une barre de statut de plan de coupe ne s'en sert pas.** Elle dit la laize et le métrage
    /// dans les DEUX systèmes à la fois, quel que soit celui du run — voir `ExportResultText` — parce
    /// qu'un plan de coupe se traduit en une course à faire, et que ses deux lectrices n'achètent pas
    /// leur tissu de la même façon. Le nom de la sortie, lui, reste en centimètres : il est un chemin
    /// de fichier. Sur un plan de coupe, ce champ est donc d'abord la trace de ce qui a été saisi.
    Unit unit{Unit::Cm};

    /// The print model, by token rather than by pointer: the table is a static, but the
    /// token is what the generated name carries and what the model is looked back up by.
    QString printModelToken{};

    /// Sheets side by side. Irrelevant to a roll — which has none — and kept all the same,
    /// so that the replay does not have to guess whether zero meant "one" or "unknown".
    int columns{1};

    /// The long side of one sheet, in millimetres — the height of the frame the plan is cut
    /// into. Zero when the run is not tiled, and that single fact is what the canvas reads
    /// to decide whether there is a grid to draw: a roll has no sheet, and the A0 model is
    /// never assembled side by side.
    ///
    /// Stored rather than looked up by the token, and it is what the readers take: the artboard
    /// grid and the PDF read the sheet from the generation and never from the model table. Now
    /// that no format carries a size the user can type, the two agree — the run keeps its own
    /// record of the sheet it was cut for all the same. The width is deliberately not stored
    /// beside it: documentWidthMm divided by the columns gives it back, and a second field is a
    /// second field for the replay to drop in silence.
    qreal frameHeightMm{0.0};

    /// The template of assembly marks the plan is drawn with, **by its file name** — the
    /// identity a template has: one file, one name, and nothing else identifies it (§12.2).
    ///
    /// The name, and not a copy of its settings. A template is a file the user edits, and a
    /// run that had copied its frame and its marks would go on drawing what the file said the
    /// day it was nested, while the editor shows something else — the canvas would then be
    /// lying about the very thing it is there to show. The name is resolved at every rebuild
    /// instead, so an edited template is the one on screen from the next rebuild on.
    ///
    /// Empty when there is nothing to draw, which is a state and not an error: a folder the
    /// user has emptied leaves every output without marks, and the canvas then draws the plan
    /// alone — as it did before there was a template at all.
    QString marksTemplateName{};

    /// The cover page this generation carries, **by its file name**, like the marks template
    /// and for the same reason: a template is a file the user edits, and a run that had copied
    /// it would go on drawing what the file said the day it was nested.
    ///
    /// **Empty means no cover page** — nothing reserved in the bin, nothing drawn on the
    /// canvas, no page added. One representation, tested at every step from the bin to the
    /// status bar, and the state the mode had before a cover page existed.
    QString coverPageTemplateName{};

    /// The value each axis stands at for this run, by axis name, as the measurement table
    /// labels it — `168`, `M`, whatever the user's own words are.
    ///
    /// **The page's own question, answered where it is known.** A cover page that states one
    /// size must state the size the RUN was asked for; the preview can only state the table's
    /// reference value, and a page announcing a size the paper does not carry is the defect
    /// §6 of the nomenclature spec was written about. Filled at nesting time, from the bases the
    /// walk actually applied — the same figures the reference layer's cartouche is resolved with —
    /// because that is the only moment where the run's size and the table that names it meet.
    ///
    /// On a range run it names only the axes that do NOT run: the graded one has no single value,
    /// and the page lists its sizes instead. Empty only when the run applied no axis at all (a
    /// single-size table), and in the preview, which is why the reader falls back to the table.
    QMap<QString, QString> pinnedAxes{};

    /// The member kept for each piece group: group id → piece id. **Only a cutting plan fills
    /// it**, and empty means the run cuts every piece the layout holds — which is what a pattern
    /// with no group asks for anyway.
    ///
    /// A group serves printing, where two pieces fill one shape on a sheet. On fabric a piece is
    /// cut once, so a cutting plan decomposes the group and keeps the member the user chose. The
    /// others are simply not collected, and the group's footprint is then the one piece's.
    QMap<quint32, quint32> groupMemberChoices{};

    /// @brief What this generation IS — see @ref OExportFormat.
    ///
    /// **One value, and not the pair of booleans it replaces.** `cuttingPlan` already carried a dozen
    /// unrelated decisions — the code said so itself — and `plotterPlan` was added beside it for the
    /// ones that were really about a mat. A third format would have made three flags, eight states for
    /// four formats, and every reader guessing which combinations mean anything.
    ///
    /// It says what the run IS, and the formats differ in more than a parameter: a cutting plan and a
    /// projector both lay pieces on fabric and follow the grain; a projector keeps the whole decor,
    /// where a cutting plan keeps none. Kept so the Refresh entry replays the same run.
    OExportFormat::Kind format{OExportFormat::Kind::Print};

    /// @brief « Sens unique » — aucune pièce n'est retournée d'un demi-tour.
    ///
    /// Le tissu a un sens : sur un velours, un carreau ou un imprimé, toutes les pièces doivent le
    /// prendre du même côté. L'option retire **un sens**, pas un axe — une pièce « Double » garde
    /// le droit-fil et le contre-fil, elle n'est simplement jamais posée à l'envers.
    bool oneWayLayout{false};

    /// @brief Whether this cutting plan was asked for on FOLDED fabric.
    ///
    /// Home sewing folds the cloth in two along the selvedge and traces the halves on the folded
    /// stack. Three things follow, and none of them is a parameter of the run: the nesting happens
    /// on **half the laize**, a piece cut on the fold is laid **folded and glued to the fold edge**,
    /// and a piece cut twice is laid **once** — the fold mirrors it.
    ///
    /// The laize the user entered is NOT this: she bought 140, the plan runs on 70, and the name
    /// says 140. Kept so that the Refresh entry replays the same plan.
    bool foldedFabric{false};


    /// @brief The cutting mat's usable size, in millimetres. **Zero on a run that has no mat.**
    ///
    /// What the user reads in her plotter's own software. These two numbers decide the whole split:
    /// a piece that exceeds one of them is cut at its multiples, counted from the piece's own corner.
    qreal matWidthMm{0.0};
    qreal matHeightMm{0.0};

    /// @brief The unit the mat is ENTERED and READ in — `cm` or `inch`, never the mm.
    ///
    /// Not a conversion: the mat stays stored in millimetres. It is what the user types and reads
    /// back, and a commercial mat's usable area is sold in inches.
    Unit matUnit{Unit::Cm};

    /// @brief Les pièces que l'utilisatrice a DÉCOCHÉES dans les groupes. Vide : aucune.
    ///
    /// Un plotter **décompose** les groupes — chaque membre est une pièce à couper — donc l'onglet
    /// Plotter n'offre pas de choisir un membre, mais de l'écarter. C'est une liste d'EXCLUSIONS, et
    /// surtout pas `groupMemberChoices` : celle-ci ne garderait qu'un membre par groupe, l'inverse
    /// exact de ce qu'un plotter fait. Le seul onglet qui remplit l'une ou l'autre est celui qui est
    /// courant, et les deux ne se croisent jamais.
    QSet<quint32> excludedPieces{};
};

/**
 * @brief Everything one run is asked for, in one piece.
 *
 * Grouped rather than passed as a row of arguments, and not only for the count: the list
 * holds three lengths (the width, the margin, the gap) and three bases, all of them
 * numbers of the same order, and a positional call would let any of them swap with its
 * neighbour without the compiler noticing. It is also exactly what the replay hands back.
 */
struct OExportRunRequest
{
    qreal documentWidthMm{0.0};

    /// @see OExportOutput::documentLiningWidthMm — la même longueur, de l'autre côté.
    qreal documentLiningWidthMm{0.0};

    /// @see OExportOutput::liningPlan — le même drapeau, de l'autre côté.
    bool liningPlan{false};

    Unit unit{Unit::Cm};
    QString rangeName{};
    bool singleSize{false};
    qreal baseA{0.0};
    qreal baseB{0.0};
    qreal baseC{0.0};
    qreal marginMm{5.0};
    qreal gapMm{5.0};
    QString printModelToken{};
    int columns{1};
    qreal frameHeightMm{0.0};
    QString marksTemplateName{};
    QString coverPageTemplateName{};
    QMap<QString, QString> pinnedAxes{};

    /// @see OExportOutput::groupMemberChoices — the same map, from the other side.
    QMap<quint32, quint32> groupMemberChoices{};

    /// @see OExportOutput::format — the same value, from the other side.
    OExportFormat::Kind format{OExportFormat::Kind::Print};

    /// @see OExportOutput::oneWayLayout — the same flag, from the other side.
    bool oneWayLayout{false};

    /// @see OExportOutput::foldedFabric — the same flag, from the other side.
    bool foldedFabric{false};


    /// @see OExportOutput::excludedPieces — the same set, from the other side.
    QSet<quint32> excludedPieces{};

    /// @see OExportOutput::matWidthMm — the same length, from the other side.
    qreal matWidthMm{0.0};

    /// @see OExportOutput::matHeightMm — the same length, from the other side.
    qreal matHeightMm{0.0};

    /// @see OExportOutput::matUnit — the same unit, from the other side.
    Unit matUnit{Unit::Cm};
};

/// @brief The request that reproduces @p output — what the Refresh entry hands back.
///
/// Written next to the data it reads, so that a field added to OExportOutput and forgotten
/// here is a field the replay silently drops: the two lists sit in the same file, and the
/// one that has to follow the other is right below it.
auto RequestOf(const OExportOutput &output) -> OExportRunRequest;

/**
 * @brief The generations of the session, owned by MainWindow.
 *
 * Session-scoped on purpose: see the file header. The store also carries the staleness
 * flag — whether the pattern has changed since the last successful generation — because
 * the two have the same lifetime and the same owner.
 */
class OExportOutputStore
{
public:
    /// @brief Drop every generation (pattern closed or opened).
    void Clear();

    auto IsEmpty() const -> bool;

    /**
     * @brief Take @p output, give it an id, and make it the active one.
     * @return the id it was given. Never 0 — 0 means "no generation".
     */
    auto Append(OExportOutput output) -> quint32;

    auto Outputs() const -> const QVector<OExportOutput> &;

    /// @brief The generation with this id, or nullptr. The pointer dies on the next Append/Clear.
    auto Find(quint32 id) const -> const OExportOutput *;

    auto ActiveId() const -> quint32;
    void SetActiveId(quint32 id);

    /// @brief Rename the generation @p id. The caller owns the contract — a trimmed,
    ///        non-empty name that no other listed generation carries; the dock's inline
    ///        rename and the entry dialog both enforce it before this is called.
    ///        No-op when the id is unknown.
    void SetName(quint32 id, const QString &name);

    /// @brief Write ONE placement of the generation @p id, and leave everything else alone.
    ///
    /// The plan's GEOMETRY is frozen once the nesting has run: this cannot reach `usedHeightPx`,
    /// so it cannot move a row, and it cannot change `binWidthPx`, so it cannot change what a
    /// sheet is. Manual correction moves pieces on the paper the generation was cut for; it never
    /// re-cuts it. That is held by this signature rather than by every caller remembering not to.
    ///
    /// **What a gesture does move is the page count.** Which sheets are printed follows the
    /// placements and not the grid (OExportSheets), so a sheet a hand has emptied is no longer
    /// written, and a piece put back on a dropped one makes its page real again. Reading the paper
    /// off the content rather than off the grid is what keeps the canvas and the PDF in step
    /// through it.
    ///
    /// Deliberately not a Replace(): that one copies the whole payload — every layer, every
    /// resolved piece — and a gesture touches none of it.
    ///
    /// No-op when the id is unknown or @p index is out of range: an undo command that
    /// outlived its generation — the pattern was closed — must be silent, not fatal.
    void SetPlacement(quint32 id, int index, const OExportPlacement &placement);

    /// @brief Replace the payload of @p id, keeping its identity and its name. This is the
    ///        Refresh entry: the result is new, the row is not — the user named it, and the
    ///        nesting knows nothing of that name. No-op when the id is unknown.
    void Replace(quint32 id, OExportOutput output);

    /// @brief Drop the generation @p id. When it was the active one, the active id falls
    ///        back to 0 — "nothing shown" — and the caller decides what to display next,
    ///        because it is the only one holding the visibility filter. No-op when the id
    ///        is unknown.
    void Remove(quint32 id);

    /// @brief Whether the pattern has changed since the last successful generation.
    auto IsStale() const -> bool;
    void SetStale(bool stale);

private:
    QVector<OExportOutput> m_outputs{};
    quint32 m_nextId{1}; ///< 0 is the "no generation" sentinel, so ids start at 1
    quint32 m_activeId{0};
    bool m_stale{false};
};

#endif // OEXPORTOUTPUT_H
