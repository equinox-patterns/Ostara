/************************************************************************
 **
 **  @file   mainwindow_metrage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  Le métrage par lot depuis la ligne de commande : la composition du modèle, lus
 **          tailles complètes, et le lancement du lot. Voir
 **          Modifications/Ostara/ExportMode/MetrageCLI.md.
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

#include "core/oexportcuttingguard.h"
#include "core/oexportmetragebatch.h"
#include "core/ogradedsizeranges.h"
#include "core/vcmdexport.h"

#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <algorithm>
#include <functional>

namespace
{
/// Les deux fichiers du lot, côte à côte dans le dossier de destination.
const QString kMetrageCsvName = QStringLiteral("metrage.csv");
const QString kMetrageLogName = QStringLiteral("metrage.log");

/// Le même que les quatre autres fichiers qui associent un libellé à une base décimalaire
/// (oexportgeometry.cpp, oexportcuttingwidget.cpp, oexportplotterwidget.cpp, oexportprintwidget.cpp).
constexpr qreal kBaseTolerance = 1e-9;

/// @brief Le libellé du tableau pour @p base, ou le nombre quand le tableau n'en donne pas.
auto LabelOf(const MeasurementDimension_p &dimension, qreal base) -> QString
{
    const DimesionLabels labels = dimension->Labels();
    for (auto label = labels.constBegin(); label != labels.constEnd(); ++label)
    {
        if (qAbs(label.key() - base) < kBaseTolerance)
        {
            return label.value();
        }
    }
    return QString::number(base);
}

/**
 * @brief Le champ de la requête que porte un type d'axe.
 *
 * ⚠️ C'est le mapping du code lui-même : `VMeasurements` écrit les libellés de `X` dans `baseA`, de
 * `Y` dans `baseB`, de `W` dans `baseC` (vmeasurements.cpp:1904-1914). Une table `.omt` reçoit
 * exactement X, Y, W dans cet ordre (vmeasurements.cpp:1583-1585), donc une stature et une taille
 * tombent juste sur `baseA` et `baseB`.
 *
 * @return le slot 0, 1 ou 2 — ou -1 pour un axe que la requête ne sait pas porter.
 */
auto SlotOf(MeasurementDimension type) -> int
{
    switch (type)
    {
        case MeasurementDimension::X:
            return 0;
        case MeasurementDimension::Y:
            return 1;
        case MeasurementDimension::W:
            return 2;
        case MeasurementDimension::Z:
        default:
            // ⚠️ Z n'a pas de champ : les trois bases de la requête sont X, Y et W, et rien d'autre.
            // Une table qui déclarerait un axe de bassin ne peut pas être parcourue — le dire vaut
            // mieux que de l'ignorer en silence.
            return -1;
    }
}

/// @brief Normalise un nom pour la comparaison : espaces autour et casse, RIEN d'autre.
///
/// ⚠️ Ce n'est pas de l'approximation, c'est ce qui rend le mécanisme possible : `Lien ` porte une
/// espace finale dans le patron, et une espace finale ne survit pas à une ligne de commande. Le
/// préfixe, la tolérance aux fautes, l'à-peu-près : **c'est le skill**, qui rend ici un nom que ce
/// code reconnaît. Ce qui ne correspond pas exactement après normalisation est refusé.
auto Normalize(const QString &name) -> QString
{
    return name.trimmed().toCaseFolded();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::MetrageCombinations(QString &error) -> QVector<OExportMetrageCombination>
{
    QVector<OExportMetrageCombination> combinations;

    if (m_m.isNull())
    {
        error = tr("No measurements table is loaded. The metrage mode needs one with size axes.");
        return combinations;
    }

    // ── Les axes, dans l'ordre du tableau ────────────────────────────────────────────────────
    //
    // L'ordre est celui de `MeasurementDimension` (X, Y, W), qui est celui de la table : c'est aussi
    // l'ordre dans lequel `DimensionRestrictedValues` lit ses restrictions (index 0, puis 1, puis 2).
    QVector<MeasurementDimension_p> axes;
    // ⚠️ Pas `slots` : c'est une macro de Qt, la déclaration se viderait de son nom.
    QVector<int> axisSlots;
    const VDimensions dimensions = m_m->Dimensions();
    for (auto it = dimensions.constBegin(); it != dimensions.constEnd(); ++it)
    {
        const int slot = SlotOf(it.key());
        if (slot < 0)
        {
            error = tr("The measurements table declares an axis of hip (%1) that a cutting plan cannot "
                       "address. The three axes a run carries are height, size and waist.")
                        .arg(static_cast<int>(it.key()));
            return {};
        }
        axes.append(it.value());
        axisSlots.append(slot);
    }

    if (axes.isEmpty())
    {
        error = tr("The measurements table declares no size axis: there is no complete size to walk.");
        return {};
    }

    // ── La marche imbriquée ──────────────────────────────────────────────────────────────────
    //
    // ⚠️ Chaque axe n'offre que ce que SA restriction permet **pour les valeurs déjà retenues en
    // amont** — jamais le produit cartésien de listes calculées indépendamment. Sur une table qui ne
    // déclare aucune restriction les deux donnent le même ensemble ; sur une qui en déclare, seul
    // celui-ci est juste.
    std::function<void(int, OExportMetrageCombination)> walk = [&](int axis, OExportMetrageCombination combination)
    {
        if (axis >= axes.size())
        {
            combinations.append(combination);
            return;
        }

        const QVector<qreal> bases =
            DimensionRestrictedValues(axis, axes.at(axis), combination.baseA, combination.baseB);

        for (const qreal base : bases)
        {
            OExportMetrageCombination next = combination;
            const QString label = LabelOf(axes.at(axis), base);

            switch (axisSlots.at(axis))
            {
                case 0:
                    next.baseA = base;
                    next.stature = label;
                    break;
                case 1:
                    next.baseB = base;
                    next.size = label;
                    break;
                default:
                    next.baseC = base;
                    break;
            }

            walk(axis + 1, next);
        }
    };

    walk(0, OExportMetrageCombination{});

    if (combinations.isEmpty())
    {
        error = tr("The measurements table offers no combination: every axis came back empty.");
    }

    return combinations;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::MetrageFiguresOf(const OExportRunRequest &request, QString &error) -> std::optional<OExportMetrageFigures>
{
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);

    std::optional<OExportOutput> output = RunExportNesting(request, context, &error);
    if (not output.has_value())
    {
        return std::nullopt;
    }

    // ⚠️ La même expression que la barre de statut (oexportoutputs.cpp:306) : `drawnRect` vaut le bac
    // sur un run non tuilé — le cas du plan de coupe — et le plan papier sur un run tuilé. Simplifier
    // en `usedHeightPx` donnerait, sur un tuilé, un chiffre qui n'est pas celui de l'écran.
    const OExportDrawing drawing = ComputeExportDrawing(*output);

    OExportMetrageFigures figures;
    figures.fabricLengthMm = FromPixel(drawing.drawnRect.height(), Unit::Mm);

    if (request.liningPlan)
    {
        // La doublure a son propre plan et donc son propre métrage — lu dans SON résultat.
        figures.hasLining = true;
        figures.liningLengthMm = FromPixel(output->liningResult.usedHeightPx, Unit::Mm);
    }

    return figures;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::DoMetrageBatch(const VCommandLinePtr &expParams) -> bool
{
    // ══ 1. Ce qui est demandé ═════════════════════════════════════════════════════════════════
    const QVector<qreal> fabricWidths = expParams->OptMetrageFabricWidths();
    if (fabricWidths.isEmpty())
    {
        qCritical() << tr("The metrage mode needs at least one --fabricWidth, in centimetres.");
        return false;
    }

    const bool liningPlan = expParams->IsMetrageLining();

    OExportRunRequest request;
    request.cuttingPlan = true;
    request.oneWayLayout = expParams->IsMetrageOneWay();
    request.foldedFabric = expParams->IsMetrageFolded();
    request.gapMm = expParams->MetragePieceGap();
    request.marginMm = expParams->MetrageEdgeMargin();
    request.singleSize = true; // ⚠️ un run par taille complète : c'est ce que veut « plusieurs tailles »
    request.liningPlan = liningPlan;
    request.unit = Unit::Cm;

    // ══ 2. La composition — le membre que chaque groupe contribue ═══════════════════════════
    //
    // ⚠️ **Il n'y a PAS de liste de pièces ici, et c'est délibéré.** Pour un plan de coupe,
    // l'ensemble des pièces vient des cases *in layout* **DU FICHIER** :
    // `OExportCuttingGuard::PiecesToCut` les lit, et `excludedPieces` n'est consulté que pour un plan
    // de plotter (oexportoutputs.cpp:688). Une liste de pièces reçue en ligne de commande ne serait
    // donc **jamais lue** — c'est vérifié à l'exécution, et c'est pourquoi l'option n'existe plus.
    //
    // Un modèle s'exprime par **une copie du patron** portant ses propres cases *in layout* : c'est la
    // règle du skill, avec sa ligne rouge — on ne touche jamais au fichier d'origine.
    //
    // Ce qui reste réglable ici, et qui l'est vraiment, c'est **le membre que chaque groupe
    // contribue** : il voyage dans la requête (`groupMemberChoices`), et `PiecesToCut` le lit.
    //
    // ⚠️ `pattern` EST le conteneur : VAbstractPattern dérive de VContainer, et c'est ainsi que le
    // garde-fou du plan de coupe le reçoit lui aussi (oexportoutputs.cpp:710).
    const VContainer *data = pattern;
    if (data == nullptr || data->DataPieces() == nullptr)
    {
        qCritical() << tr("The pattern carries no piece.");
        return false;
    }

    const QHash<quint32, VPiece> *pieces = data->DataPieces();
    const QHash<quint32, OPieceGroup> *groups = data->DataPieceGroups();

    QStringList compositionNote;

    // ── Le membre que chaque groupe contribue ───────────────────────────────────────────────
    const QStringList groupPairs = expParams->OptMetrageGroups();

    if (not groupPairs.isEmpty() && (groups == nullptr || groups->isEmpty()))
    {
        qCritical() << tr("This pattern declares no piece group, so --group names nothing.");
        return false;
    }

    for (const QString &pair : groupPairs)
    {
        const qsizetype cut = pair.indexOf(QLatin1Char('='));
        if (cut <= 0 || cut == pair.size() - 1)
        {
            qCritical() << tr("A --group must read \"Group name=Member name\"; this one reads:")
                        << pair;
            return false;
        }

        const QString wantedGroup = pair.left(cut).trimmed();
        const QString wantedMember = pair.mid(cut + 1).trimmed();

        quint32 groupId = NULL_ID;
        for (auto it = groups->constBegin(); it != groups->constEnd(); ++it)
        {
            if (Normalize(it.value().name) == Normalize(wantedGroup))
            {
                groupId = it.key();
                break;
            }
        }

        if (groupId == NULL_ID)
        {
            QStringList available;
            for (auto it = groups->constBegin(); it != groups->constEnd(); ++it)
            {
                available.append(it.value().name);
            }
            std::sort(available.begin(), available.end());

            qCritical() << tr("No group of this pattern is called \"%1\". The groups are: %2")
                               .arg(wantedGroup, available.join(QStringLiteral(", ")));
            return false;
        }

        const OPieceGroup group = groups->value(groupId);
        if (not group.IsInLayout())
        {
            qCritical() << tr("The group \"%1\" is out of the layout in this pattern: switch it on, or "
                              "leave it out of the model.")
                               .arg(group.name);
            return false;
        }

        quint32 memberId = NULL_ID;
        QStringList members;
        for (const quint32 member : group.pieceIds)
        {
            if (not pieces->contains(member))
            {
                continue; // a stale member id (hand-edited file) — not offered, not chosen
            }
            const QString name = pieces->value(member).GetName();
            members.append(name);
            if (Normalize(name) == Normalize(wantedMember))
            {
                memberId = member;
            }
        }

        if (memberId == NULL_ID)
        {
            std::sort(members.begin(), members.end());
            qCritical() << tr("\"%1\" is not a member of the group \"%2\". Its members are: %3")
                               .arg(wantedMember, group.name, members.join(QStringLiteral(", ")));
            return false;
        }

        request.groupMemberChoices.insert(groupId, memberId);

        compositionNote.append(group.name + QStringLiteral(" → ") + pieces->value(memberId).GetName());
    }

    // ══ 3. La viabilité, AVANT de lancer ════════════════════════════════════════════════════
    //
    // ⚠️ Le garde-fou du run refuse un plan dont une pièce n'a pas de direction. Le laisser le dire
    // 42 fois serait une façon de perdre la journée : on le demande une fois, ici, et on le dit avec
    // les noms.
    const QVector<OExportCuttingGuard::Material> materials =
        liningPlan ? QVector<OExportCuttingGuard::Material>{OExportCuttingGuard::Material::Fabric,
                                                           OExportCuttingGuard::Material::Lining}
                   : QVector<OExportCuttingGuard::Material>{OExportCuttingGuard::Material::Fabric};

    const QVector<OExportCuttingGuard::FaultyPiece> faulty =
        OExportCuttingGuard::FaultyPieces(request.groupMemberChoices, data, materials);

    if (not faulty.isEmpty())
    {
        QStringList lines;
        for (const OExportCuttingGuard::FaultyPiece &piece : faulty)
        {
            lines.append(QStringLiteral("%1. %2").arg(piece.number).arg(piece.name));
        }

        qCritical() << tr("A cutting plan needs a direction — an enabled grainline, or a fold line — for "
                          "every piece it cuts. These have none: %1")
                           .arg(lines.join(QStringLiteral(" · ")));
        return false;
    }

    // ⚠️ Une pièce à cut vide n'est pas coupée du tout — le garde-fou la lit comme « pas là », pas
    // comme fautive. Le dire, parce que le métrage sortira plus petit sans que rien ne le signale.
    for (const OExportCuttingGuard::Material material : materials)
    {
        // Le membre que le modèle a RETENU, groupe par groupe — `values()` le donne directement.
        for (auto chosen = request.groupMemberChoices.constBegin();
             chosen != request.groupMemberChoices.constEnd(); ++chosen)
        {
            const quint32 member = chosen.value();
            if (OExportCuttingGuard::CutQuantity(member, data, material) <= 0)
            {
                // ⚠️ `CutQuantity` retombe sur la quantité du **GROUPE** quand la pièce n'a pas la
                // sienne (oexportcuttingguard.cpp:80-95). Si ce message sort, c'est donc que ni l'une
                // ni l'autre n'est renseignée — et là le membre n'est pas coupé du tout.
                qWarning() << tr("Careful: \"%1\" has no %2 cut — neither its own nor its group's — so it "
                                 "is not cut at all: the run will measure the plan WITHOUT it.")
                                  .arg(pieces->value(member).GetName(),
                                       material == OExportCuttingGuard::Material::Lining ? tr("lining")
                                                                                        : tr("fabric"));
            }
        }
    }

    // ══ 4. Les tailles complètes ═════════════════════════════════════════════════════════════
    QString error;
    const QVector<OExportMetrageCombination> combinations = MetrageCombinations(error);
    if (combinations.isEmpty())
    {
        qCritical() << tr("No size to cut:") << error;
        return false;
    }

    // ══ 5. Les runs, et le tableau ═══════════════════════════════════════════════════════════
    QString destination = expParams->OptDestinationPath();
    if (destination.isEmpty())
    {
        // ⚠️ Sans destination, le lot écrirait dans le dossier courant sans le dire. On le dit —
        // refuser coûterait un aller-retour à Sophie, et le journal nomme le CSV de toute façon.
        destination = QDir::currentPath();
        qWarning() << tr("No --destination was given: the table goes to %1").arg(destination);
    }
    QDir().mkpath(destination);

    const qreal liningWidth = expParams->OptMetrageLiningWidth();

    QVector<OExportMetrageJob> jobs;
    for (const qreal width : fabricWidths)
    {
        OExportMetrageJob job;
        job.model = expParams->OptMetrageModel();
        job.fabricWidthMm = width;
        job.liningPlan = liningPlan;
        // La laize de doublure suit celle du tissu tant qu'elle n'est pas dite — c'est ce que fait
        // l'onglet du mode Export.
        job.liningWidthMm = liningPlan ? (liningWidth > 0.0 ? liningWidth : width) : 0.0;
        jobs.append(job);
    }

    QChar separator = expParams->OptCSVSeparator();
    if (separator.isNull())
    {
        separator = VCommonSettings::GetDefCSVSeparator();
    }

    OExportMetrageBatch batch(
        QDir(destination).filePath(kMetrageCsvName), QDir(destination).filePath(kMetrageLogName), separator,
        [this](const OExportRunRequest &one, QString &reason) { return MetrageFiguresOf(one, reason); });

    batch.SetRequestTemplate(request);
    batch.SetCombinations(combinations);
    batch.SetJobs(jobs);
    batch.SetCompositionNote(compositionNote.join(QStringLiteral(" · ")));

    if (not batch.Run())
    {
        qCritical() << tr("The metrage batch could not start: %1")
                           .arg(QDir(destination).filePath(kMetrageCsvName));
        return false;
    }

    qInfo() << tr("Metrage: %1 run(s) done, %2 failed, %3 already there — %4 s. Table: %5")
                   .arg(batch.Succeeded())
                   .arg(batch.Failed())
                   .arg(batch.Resumed())
                   .arg(static_cast<double>(batch.DurationMs()) / 1000.0, 0, 'f', 1)
                   .arg(QDir(destination).filePath(kMetrageCsvName));

    // ⚠️ Un lot qui n'a RIEN produit — ni ligne nouvelle, ni ligne reprise — sort en erreur. Un lot
    // dont quelques runs ont échoué, non : c'est un tableau avec des trous, pas un échec.
    return batch.Succeeded() + batch.Resumed() > 0;
}
