/************************************************************************
 **
 **  @file   oexportmetragebatch.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The métrage batch: one run per (model, size, stature, fabric width), written as a
 **          table. It enumerates nothing and nests nothing — it is handed the combinations and a
 **          runner, and its whole job is to walk them, to write the CSV as it goes, and to be
 **          resumable by reading what it already wrote.
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

#ifndef OEXPORTMETRAGEBATCH_H
#define OEXPORTMETRAGEBATCH_H

#include <QChar>
#include <QFile>
#include <QSet>
#include <QString>
#include <QTextStream>
#include <QVector>

#include <functional>
#include <optional>

#include "../vmisc/def.h"
#include "oexportoutput.h"

/**
 * @brief One complete size: the values the run is asked for, and the labels the user reads.
 *
 * A combination carries all three bases because a request carries all three, even though a table
 * may declare one axis or two. What the batch knows is a value and a label per axis, and nothing
 * about tables.
 */
struct OExportMetrageCombination
{
    QString stature{}; ///< axis A, as the table labels it ("160")
    QString size{};    ///< axis B, as the table labels it ("32")
    qreal baseA{0.0};
    qreal baseB{0.0};
    qreal baseC{0.0};
};

/**
 * @brief One run configuration: a model, and the fabric it is cut from.
 *
 * ⚠️ The model is a NAME, not a composition. The composition travelled in the request template
 * (which pieces, which group member), because that is what the user described. What varies here is
 * only what multiplies the runs.
 */
struct OExportMetrageJob
{
    QString model{};
    qreal fabricWidthMm{0.0};
    qreal liningWidthMm{0.0};
    bool liningPlan{false};
};

/**
 * @brief What one run measured — in MILLIMETRES, exactly as the run produced it.
 *
 * ⚠️ Millimetres are the source of truth. The centimetres and the yards of the CSV are rendered
 * from these, by the application's own formatters, so that the table says exactly what the status
 * bar says. An arrondi must never be the value.
 */
struct OExportMetrageFigures
{
    qreal fabricLengthMm{0.0};

    bool hasLining{false};
    qreal liningLengthMm{0.0};
};

/**
 * @brief The métrage batch — the loop and the table, and nothing else.
 *
 * Three decisions are worth reading before the code:
 *
 * 1. **The runner is injected** (dependency inversion). The batch does not know what nests, nor
 *    that a window exists: it is handed something that turns a request into figures. That is what
 *    makes it usable from the command line, and what keeps the nesting out of this file.
 *
 * 2. **The CSV is the state.** Nothing is appended to a JSON that has to stay valid, and there is
 *    no second file to keep in step: a row already written with `ok` is a combination already
 *    done, so re-running resumes instead of restarting. A job interrupted at row 137 of 252 has
 *    lost nothing.
 *
 * 3. **A failure never stops the batch.** It is written in the `statut` column and in the journal,
 *    and the walk carries on — a batch of three hours must not die on one bad combination.
 */
class OExportMetrageBatch
{
public:
    /// @brief Turns a request into figures, or explains in @p error why it could not.
    using Runner = std::function<std::optional<OExportMetrageFigures>(const OExportRunRequest &request, QString &error)>;

    OExportMetrageBatch(QString csvPath, QString logPath, QChar separator, Runner runner);

    /// @brief The common part of every run: a cutting plan, its options, and the composition.
    void SetRequestTemplate(const OExportRunRequest &request);

    /// @brief The complete sizes — the product of the axes, already enumerated by the caller.
    void SetCombinations(QVector<OExportMetrageCombination> combinations);

    /// @brief The runs to make: each model, each fabric width.
    void SetJobs(QVector<OExportMetrageJob> jobs);

    /// @brief How the composition was understood, in the user's own words — for the journal.
    ///
    /// ⚠️ It is a NOTE, not data: the batch writes it into the plan so the user can check, in one
    /// glance and before paying three hours, that "Parementure devant" resolved to the piece she
    /// meant. The names are resolved by the caller, which is the only one that knows them.
    void SetCompositionNote(const QString &note);

    /**
     * @brief Walk every (job, combination) not already `ok` in the CSV, and write as it goes.
     *
     * @return false only when the walk could not START — an unwritable CSV, no job, no combination.
     *         A run that failed is **not** a failure of the batch: it is a row, and the walk goes
     *         on. The caller reads Succeeded()/Failed()/Resumed() to decide what to exit with.
     */
    auto Run() -> bool;

    auto Planned() const -> int;
    auto Succeeded() const -> int;
    auto Failed() const -> int;
    auto Resumed() const -> int;
    auto DurationMs() const -> qint64;

    /// @brief The identity of a row: what makes a combination "already done".
    ///
    /// ⚠️ **The run's options are part of it**, and they have to be: two batches that differ by
    /// "folded", by a gap, or by the lining would produce different lengths for the same
    /// (model, width, stature, size) — skipping one as the other's would mix two configurations into
    /// one table, silently. That is why the options are written as columns too: the key can only be
    /// rebuilt from the file if the file carries what the key is made of.
    static auto RowKey(const QString &model, qreal fabricWidthMm, const QString &stature, const QString &size,
                       bool folded, bool oneWay, qreal gapMm, qreal marginMm, bool lining) -> QString;

    /// @brief The column names, in the order Row() writes them.
    static auto Header(QChar separator) -> QString;

    /// @brief The column the status is written in, read back from, and how many there are.
    ///
    /// ⚠️ Les options du run sont des COLONNES, pas seulement une clé : le CSV est l'état du lot, et
    /// un état qui ne dit pas dans quelles conditions une ligne a été mesurée ne se reprend pas.
    /// Ordonc : modele, laize_mm, laize_cm, stature, taille, plie, meme_sens, ecart_mm, bord_mm,
    /// doublure, les six longueurs, statut.
    static constexpr int kStatusColumn = 16;
    static constexpr int kColumnCount = 17;

private:
    /// @brief One CSV line — the identity, **the run's options**, the six lengths, and the status.
    auto Row(const OExportMetrageJob &job, const OExportMetrageCombination &combination,
             const std::optional<OExportMetrageFigures> &figures, const QString &error) const -> QString;

    auto LoadDone() -> void;

    /// @brief Open the CSV (append) and the journal (append); write the columns if the CSV is new.
    auto OpenOutputs() -> bool;

    auto LogLine(const QString &text) -> void;
    auto LogPlan() -> void;
    auto LogSummary() -> void;

    const QString m_csvPath;
    const QString m_logPath;
    const QChar m_separator;

    /// The runner, injected. Never null — the constructor refuses otherwise.
    const Runner m_runner;

    QFile m_csvFile{};
    QTextStream m_csvStream{};
    QFile m_logFile{};
    QTextStream m_logStream{};

    OExportRunRequest m_template{};
    QString m_compositionNote{};
    QVector<OExportMetrageCombination> m_combinations{};
    QVector<OExportMetrageJob> m_jobs{};

    /// Rows already `ok` in the CSV, by RowKey(). The resume, and nothing else.
    QSet<QString> m_done{};

    /// Everything the summary needs. Counted as the walk goes.
    int m_succeeded{0};
    int m_failed{0};
    int m_resumed{0};
    qint64 m_durationMs{0};
};

#endif // OEXPORTMETRAGEBATCH_H
