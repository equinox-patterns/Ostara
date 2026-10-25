/************************************************************************
 **
 **  @file   oexportmetragebatch.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The métrage batch: the walk, the CSV, and the journal.
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

#include "oexportmetragebatch.h"

#include "olengthunits.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QStringConverter>
#include <QStringList>

#include <utility>

namespace
{
/// @brief A CSV field, quoted only when it has to be.
auto Field(const QString &value, QChar separator) -> QString
{
    if (not value.contains(separator) && not value.contains(QLatin1Char('"')) && not value.contains(QLatin1Char('\n')))
    {
        return value;
    }

    QString quoted = value;
    quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + quoted + QLatin1Char('"');
}

/// @brief A length in millimetres, written exact. The arrondi is a column of its own, never the value.
auto Mm(qreal value) -> QString
{
    return QString::number(value, 'f', 3);
}

/**
 * @brief Read one CSV line back, quotes included.
 *
 * The alternative — forbidding the separator in a model name — would push a writing constraint
 * onto the user to spare twenty lines here. The writer and the reader agree instead, and a model
 * may be called what it is called.
 */
auto ParseCsvLine(const QString &line, QChar separator) -> QStringList
{
    QStringList fields;
    QString current;
    bool quoted = false;

    for (qsizetype i = 0; i < line.size(); ++i)
    {
        const QChar c = line.at(i);

        if (quoted)
        {
            if (c == QLatin1Char('"'))
            {
                if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"'))
                {
                    current.append(QLatin1Char('"'));
                    ++i;
                }
                else
                {
                    quoted = false;
                }
            }
            else
            {
                current.append(c);
            }
        }
        else if (c == QLatin1Char('"'))
        {
            quoted = true;
        }
        else if (c == separator)
        {
            fields.append(current);
            current.clear();
        }
        else
        {
            current.append(c);
        }
    }

    fields.append(current);
    return fields;
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportMetrageBatch::OExportMetrageBatch(QString csvPath, QString logPath, QChar separator, Runner runner)
  : m_csvPath(std::move(csvPath)),
    m_logPath(std::move(logPath)),
    m_separator(separator),
    m_runner(std::move(runner))
{
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::SetRequestTemplate(const OExportRunRequest &request)
{
    m_template = request;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::SetCombinations(QVector<OExportMetrageCombination> combinations)
{
    m_combinations = std::move(combinations);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::SetJobs(QVector<OExportMetrageJob> jobs)
{
    m_jobs = std::move(jobs);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::SetCompositionNote(const QString &note)
{
    m_compositionNote = note;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Planned() const -> int
{
    return static_cast<int>(m_jobs.size() * m_combinations.size());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Succeeded() const -> int
{
    return m_succeeded;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Failed() const -> int
{
    return m_failed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Resumed() const -> int
{
    return m_resumed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::DurationMs() const -> qint64
{
    return m_durationMs;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::RowKey(const QString &model, qreal fabricWidthMm, const QString &stature,
                                 const QString &size, bool folded, bool oneWay, qreal gapMm, qreal marginMm,
                                 bool lining) -> QString
{
    const auto flag = [](bool value) { return value ? QStringLiteral("1") : QStringLiteral("0"); };

    // A unit separator, so that "A" + "1" cannot collide with "A1" + "". Nothing user-typed
    // contains it. ⚠️ Les options en font partie : deux lots réglés différemment ne doivent jamais
    // se prendre l'un pour l'autre (voir le commentaire de la déclaration).
    return model + QLatin1Char('\x1f') + Mm(fabricWidthMm) + QLatin1Char('\x1f') + stature + QLatin1Char('\x1f')
           + size + QLatin1Char('\x1f') + flag(folded) + QLatin1Char('\x1f') + flag(oneWay) + QLatin1Char('\x1f')
           + Mm(gapMm) + QLatin1Char('\x1f') + Mm(marginMm) + QLatin1Char('\x1f') + flag(lining);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Header(QChar separator) -> QString
{
    const QStringList fields{QStringLiteral("modele"),             QStringLiteral("laize_mm"),
                             QStringLiteral("laize_cm"),           QStringLiteral("stature"),
                             QStringLiteral("taille"),             QStringLiteral("plie"),
                             QStringLiteral("meme_sens"),          QStringLiteral("ecart_mm"),
                             QStringLiteral("bord_mm"),            QStringLiteral("doublure"),
                             QStringLiteral("metrage_tissu_mm"),   QStringLiteral("metrage_tissu_cm"),
                             QStringLiteral("metrage_tissu_yd"),   QStringLiteral("metrage_doublure_mm"),
                             QStringLiteral("metrage_doublure_cm"), QStringLiteral("metrage_doublure_yd"),
                             QStringLiteral("statut")};

    return fields.join(separator);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Row(const OExportMetrageJob &job, const OExportMetrageCombination &combination,
                              const std::optional<OExportMetrageFigures> &figures, const QString &error) const
    -> QString
{
    // Les mêmes booléens que RowKey(), écrits de la même façon : la clé se relit depuis le fichier.
    const auto flag = [](bool value) { return value ? QStringLiteral("1") : QStringLiteral("0"); };

    // ⚠️ Les millimètres d'abord, parce qu'un arrondi ne doit jamais être la donnée ; les centimètres
    // et les yards ensuite, **par les fonctions de l'application**, pour que le CSV dise exactement
    // ce que la barre de statut dit.
    //
    // ⚠️ Et **les options du run** entre les deux : sans elles, une ligne reprise ne saurait pas dans
    // quelles conditions elle a été mesurée, et deux lots réglés différemment se mélangeraient.
    QStringList fields{Field(job.model, m_separator),
                       Field(Mm(job.fabricWidthMm), m_separator),
                       Field(OLengthUnits::Format(job.fabricWidthMm, Unit::Cm), m_separator),
                       Field(combination.stature, m_separator),
                       Field(combination.size, m_separator),
                       Field(flag(m_template.foldedFabric), m_separator),
                       Field(flag(m_template.oneWayLayout), m_separator),
                       Field(Mm(m_template.gapMm), m_separator),
                       Field(Mm(m_template.marginMm), m_separator),
                       Field(flag(job.liningPlan), m_separator)};

    if (figures.has_value())
    {
        fields << Field(Mm(figures->fabricLengthMm), m_separator)
               << Field(OLengthUnits::Format(figures->fabricLengthMm, Unit::Cm), m_separator)
               << Field(OLengthUnits::Yards(figures->fabricLengthMm), m_separator);

        if (figures->hasLining)
        {
            fields << Field(Mm(figures->liningLengthMm), m_separator)
                   << Field(OLengthUnits::Format(figures->liningLengthMm, Unit::Cm), m_separator)
                   << Field(OLengthUnits::Yards(figures->liningLengthMm), m_separator);
        }
        else
        {
            // A model without a lining leaves the three columns empty rather than repeating the
            // fabric in them: an empty cell means "there is none", a filled one would mean "here
            // it is".
            fields << QString() << QString() << QString();
        }

        fields << Field(QStringLiteral("ok"), m_separator);
    }
    else
    {
        // Six empty measure fields, then why. The laize is written even here — it belongs to the
        // job, not to the run.
        //
        // ⚠️ Un motif d'échec peut être multiligne — le diagnostic du moteur l'est. Les retours à la
        // ligne deviennent un séparateur, pour qu'une ligne du CSV reste UNE ligne : un champ cité
        // contenant un saut de ligne se relirait de travers.
        QString reason = error.isEmpty() ? QStringLiteral("echec") : error;
        reason.replace(QLatin1Char('\n'), QStringLiteral(" · "));
        reason.replace(QLatin1Char('\r'), QString());
        reason.replace(QLatin1Char('\t'), QLatin1Char(' '));

        fields << QString() << QString() << QString() << QString() << QString() << QString()
               << Field(reason, m_separator);
    }

    return fields.join(m_separator);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::LoadDone()
{
    QFile csv(m_csvPath);
    if (not csv.exists() || not csv.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QTextStream in(&csv);
    bool columns = true;
    while (not in.atEnd())
    {
        const QString line = in.readLine();
        if (columns)
        {
            columns = false; // the first line is the column names, whoever wrote them
            continue;
        }

        if (line.trimmed().isEmpty())
        {
            continue;
        }

        const QStringList fields = ParseCsvLine(line, m_separator);
        if (fields.size() < kColumnCount)
        {
            continue; // not one of ours: never read a foreign line as a finished combination
        }

        if (fields.at(kStatusColumn) == QStringLiteral("ok"))
        {
            // Les neuf valeurs de la clé, relues dans le même ordre que Row() les écrit.
            m_done.insert(RowKey(fields.at(0), fields.at(1).toDouble(), fields.at(3), fields.at(4),
                                 fields.at(5) == QStringLiteral("1"), fields.at(6) == QStringLiteral("1"),
                                 fields.at(7).toDouble(), fields.at(8).toDouble(),
                                 fields.at(9) == QStringLiteral("1")));
        }
    }

    csv.close();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::OpenOutputs() -> bool
{
    const QFileInfo csvInfo(m_csvPath);
    QDir().mkpath(csvInfo.absolutePath());

    const bool fresh = not csvInfo.exists() || csvInfo.size() == 0;

    m_csvFile.setFileName(m_csvPath);
    if (not m_csvFile.open(QIODevice::Append | QIODevice::Text))
    {
        return false;
    }
    m_csvStream.setDevice(&m_csvFile);
    m_csvStream.setEncoding(QStringConverter::Utf8);

    if (fresh)
    {
        m_csvStream << Header(m_separator) << '\n';
        m_csvStream.flush();
    }

    const QFileInfo logInfo(m_logPath);
    QDir().mkpath(logInfo.absolutePath());

    m_logFile.setFileName(m_logPath);
    if (not m_logFile.open(QIODevice::Append | QIODevice::Text))
    {
        return false;
    }
    m_logStream.setDevice(&m_logFile);
    m_logStream.setEncoding(QStringConverter::Utf8);

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::LogLine(const QString &text)
{
    m_logStream << QDateTime::currentDateTime().toString(Qt::ISODate) << QLatin1Char(' ') << text << '\n';
    // ⚠️ Vidé à chaque ligne : le journal est le seul moyen de savoir où en est un batch qu'on ne
    // regarde pas tourner.
    m_logStream.flush();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::LogPlan()
{
    // ⚠️ Le plan résolu, écrit dès la première seconde : une faute de frappe dans un prompt de cinq
    // minutes se voit ici, pas trois heures plus tard.
    m_logStream << '\n';
    LogLine(QStringLiteral("=== Plan ==="));
    LogLine(QStringLiteral("combinaisons de tailles : ") + QString::number(m_combinations.size()));
    LogLine(QStringLiteral("runs prévus             : ") + QString::number(Planned()));

    if (not m_compositionNote.isEmpty())
    {
        LogLine(QStringLiteral("composition             : ") + m_compositionNote);
    }

    LogLine(QStringLiteral("plan de coupe           : ")
            + (OExportFormat::IsCutting(m_template.format) ? QStringLiteral("oui") : QStringLiteral("non")));
    LogLine(QStringLiteral("tissu plié              : ") + (m_template.foldedFabric ? QStringLiteral("oui") : QStringLiteral("non")));
    LogLine(QStringLiteral("pièces dans le même sens: ") + (m_template.oneWayLayout ? QStringLiteral("oui") : QStringLiteral("non")));
    LogLine(QStringLiteral("écart entre les pièces  : ") + QString::number(m_template.gapMm, 'f', 3) + QStringLiteral(" mm"));
    LogLine(QStringLiteral("écart au bord           : ") + QString::number(m_template.marginMm, 'f', 3) + QStringLiteral(" mm"));
    LogLine(QStringLiteral("pièces exclues          : ") + QString::number(m_template.excludedPieces.size()));
    LogLine(QStringLiteral("membres de groupe fixés : ") + QString::number(m_template.groupMemberChoices.size()));

    const QStringList laizes = [this]() -> QStringList
    {
        QStringList list;
        for (const OExportMetrageJob &job : std::as_const(m_jobs))
        {
            QString entry = OLengthUnits::Format(job.fabricWidthMm, Unit::Cm);
            if (job.liningPlan)
            {
                entry += QStringLiteral(" + doublure ")
                         + OLengthUnits::Format(job.liningWidthMm, Unit::Cm);
            }
            if (not list.contains(entry))
            {
                list.append(entry);
            }
        }
        return list;
    }();

    LogLine(QStringLiteral("laizes                  : ") + laizes.join(QStringLiteral(", ")));
    LogLine(QStringLiteral("modèles                 : ") + QString::number(m_jobs.size()));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportMetrageBatch::LogSummary()
{
    m_logStream << '\n';
    LogLine(QStringLiteral("=== Résumé ==="));
    LogLine(QStringLiteral("réussis : ") + QString::number(m_succeeded));
    LogLine(QStringLiteral("échoués : ") + QString::number(m_failed));
    LogLine(QStringLiteral("repris  : ") + QString::number(m_resumed));
    LogLine(QStringLiteral("durée   : ") + QString::number(static_cast<double>(m_durationMs) / 1000.0, 'f', 1)
            + QStringLiteral(" s"));
    LogLine(QStringLiteral("CSV     : ") + m_csvPath);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMetrageBatch::Run() -> bool
{
    if (not m_runner || m_jobs.isEmpty() || m_combinations.isEmpty())
    {
        return false;
    }

    if (not OpenOutputs())
    {
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    // ⚠️ **Le CSV est l'état.** Ce qui est déjà `ok` est sauté : un batch interrompu à la 137ᵉ ligne
    // sur 252 ne reperd rien, et un second modèle s'ajoute au même tableau.
    LoadDone();
    LogPlan();

    for (const OExportMetrageJob &job : std::as_const(m_jobs))
    {
        for (const OExportMetrageCombination &combination : std::as_const(m_combinations))
        {
            if (m_done.contains(RowKey(job.model, job.fabricWidthMm, combination.stature, combination.size,
                                       m_template.foldedFabric, m_template.oneWayLayout, m_template.gapMm,
                                       m_template.marginMm, job.liningPlan)))
            {
                ++m_resumed;
                continue;
            }

            OExportRunRequest request = m_template;
            request.baseA = combination.baseA;
            request.baseB = combination.baseB;
            request.baseC = combination.baseC;
            request.documentWidthMm = job.fabricWidthMm;
            request.liningPlan = job.liningPlan;
            request.documentLiningWidthMm = job.liningPlan ? job.liningWidthMm : 0.0;

            QString error;
            const std::optional<OExportMetrageFigures> figures = m_runner(request, error);

            if (figures.has_value())
            {
                ++m_succeeded;
            }
            else
            {
                ++m_failed;
            }

            m_csvStream << Row(job, combination, figures, error) << '\n';
            m_csvStream.flush();

            // ⚠️ `OLengthUnits::Format` écrit **déjà** son unité : ajouter « cm » ici donnait
            // « 140 cm cm ».
            QString line = job.model + QStringLiteral(" · ") + OLengthUnits::Format(job.fabricWidthMm, Unit::Cm)
                           + QStringLiteral(" · ") + combination.stature + QStringLiteral(" · ")
                           + combination.size;
            if (figures.has_value())
            {
                line += QStringLiteral(" → ") + OLengthUnits::Format(figures->fabricLengthMm, Unit::Cm)
                        + QStringLiteral(" (") + OLengthUnits::Yards(figures->fabricLengthMm) + QLatin1Char(')');
                if (figures->hasLining)
                {
                    line += QStringLiteral(" + doublure ")
                            + OLengthUnits::Format(figures->liningLengthMm, Unit::Cm);
                }
            }
            else
            {
                // ⚠️ Un échec n'arrête pas le batch : il s'écrit, et la marche continue. Trois heures
                // de calcul ne doivent pas mourir sur une combinaison.
                line += QStringLiteral(" → ÉCHEC : ") + (error.isEmpty() ? QStringLiteral("sans motif") : error);
            }
            LogLine(line);
        }
    }

    m_durationMs = timer.elapsed();
    LogSummary();

    m_csvStream.flush();
    m_logStream.flush();
    m_csvFile.close();
    m_logFile.close();

    return true;
}
