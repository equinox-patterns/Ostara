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

#include <QString>
#include <QVector>

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
    qreal marginMm{5.0}; ///< the document margin the run was asked for, in millimetres
    qreal gapMm{5.0};    ///< the space between two pieces, in millimetres

    /// The unit the run was entered in. It changes nothing about the result — every length
    /// is stored in millimetres — but it is what the figures are written in, here and in
    /// the status bar: without it the same run would read "84,1 cm" in the dialog and
    /// "841 mm" once generated.
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
    /// Stored rather than looked up by the token, because the framed model is the one whose
    /// sides the user may TYPE — FindPrintModel would hand back the table's 259 mm whatever
    /// she entered. The width is deliberately not stored beside it: documentWidthMm divided
    /// by the columns gives it back, and a second field is a second field for the replay to
    /// drop in silence.
    qreal frameHeightMm{0.0};
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
