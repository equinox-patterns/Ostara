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
    quint32 id{0}; ///< stable identity — a row index is not one
    QString name{}; ///< "Unnamed": no Name field exists yet

    QString measurementCanonical{}; ///< absolute path of the table it was generated under
    QString sizeAxis{};             ///< declared grading axis at that moment; empty lifts the filter

    OSizerange range{};   ///< copy of the range, as it was when the run started
    bool hasRange{false}; ///< false for a single-size run
    QString rangeName{};  ///< the range name — the label, and the key the size styles resolve by

    qreal documentWidth{0.0};            ///< the width the user asked for, in pattern units
    QVector<OExportSizeLayer> layers{};  ///< the pattern resolved for every size
    OExportNestingResult result{};       ///< the placements, and the bin they were placed in
};

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
