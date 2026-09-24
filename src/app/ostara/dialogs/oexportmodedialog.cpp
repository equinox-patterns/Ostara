/************************************************************************
 **
 **  @file   oexportmodedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Implementation of the Export mode entry dialog.
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

#include "oexportmodedialog.h"

#include "oexportcuttingwidget.h"
#include "oexportgenerationlistwidget.h"
#include "oexportprintwidget.h"

#include "../core/oexportcuttingguard.h"
#include "../core/oexportnamebuilder.h"
#include "../core/oexportunitformat.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>
#include <QVBoxLayout>

#include <utility>

namespace
{
/// The tab that holds the runs described but not fired.
constexpr int kListTabIndex = 0;

/// The tab that cuts pieces out of fabric, one size at a time.
constexpr int kCuttingTabIndex = 1;

/// Where the tab that can be acted on sits. The dialog opens on it rather than on the
/// first tab: two of the four have nothing to offer yet, and landing on one of those
/// would show an empty page every time the mode is entered.
constexpr int kPrintTabIndex = 2;

/// The tab that throws the plan on a wall.
constexpr int kProjectorTabIndex = 3;

/// Below this the Print tab's form is cramped, whatever the tab bar needs.
constexpr int kMinDialogWidth = 560;

/// What the tab widget's frame and margins add around the tab bar itself.
constexpr int kTabBarChrome = 40;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportModeDialog::OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                     const OGradedSizeRanges::Context &context, const QStringList &takenNames,
                                     QWidget *parent)
  : QDialog(parent),
    m_data(data),
    m_measurements(measurements),
    m_context(context),
    m_takenNames(takenNames)
{
    setWindowTitle(tr("Export"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);

    m_listWidget = new OExportGenerationListWidget(m_tabs);
    connect(m_listWidget, &OExportGenerationListWidget::RemoveRequested, this,
            [this](int index)
            {
                // The queue first, then the row: the row is a view of it, and a view that
                // ran ahead would be showing a line the batch could still walk into.
                m_queue.RemoveAt(index);
                m_listWidget->RemoveRow(index);
                RefreshGenerateButtons();
            });

    m_cuttingTab = new QWidget(m_tabs);
    m_cuttingLayout = new QVBoxLayout(m_cuttingTab);
    // The stretch keeps the form at the top of the page; the widget is inserted before it.
    m_cuttingLayout->addStretch();

    m_cuttingWidget = new OExportCuttingWidget(m_data, m_measurements, m_cuttingTab);
    m_cuttingLayout->insertWidget(0, m_cuttingWidget);
    connect(m_cuttingWidget, &OExportCuttingWidget::Changed, this, &OExportModeDialog::OnFormChanged);

    m_printTab = new QWidget(m_tabs);
    m_printLayout = new QVBoxLayout(m_printTab);
    // The stretch is what keeps the form at the top of the page; ResetPrintTab inserts the
    // form at index 0, before it.
    m_printLayout->addStretch();

    m_projectorTab = MakeComingSoonTab(m_tabs);

    // A tab bar that runs out of room shortens its labels, or hides the surplus behind
    // scroll arrows. Neither is readable, so the dialog is widened to the labels instead
    // — see AdjustMinimumWidth, which Retranslate calls once the titles are known.
    m_tabs->setElideMode(Qt::ElideNone);
    m_tabs->setUsesScrollButtons(false);

    m_tabs->addTab(m_listWidget, QString());
    m_tabs->addTab(m_cuttingTab, QString());
    m_tabs->addTab(m_printTab, QString());
    m_tabs->addTab(m_projectorTab, QString());
    m_tabs->setCurrentIndex(kPrintTabIndex);
    layout->addWidget(m_tabs);

    // The bar sits on the right with the last thing said on its left: what was added is
    // reported where the user is already looking when she clicks, not in a status line
    // somewhere else in the window.
    auto *barRow = new QHBoxLayout();
    m_addedLabel = new QLabel(this);
    m_addedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_addedLabel->setObjectName(QStringLiteral("exportAddedLabel"));
    barRow->addWidget(m_addedLabel, 1);

    m_buttonBox = new QDialogButtonBox(this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    barRow->addWidget(m_buttonBox, 0);

    layout->addLayout(barRow);

    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int) { OnTabChanged(); });

    // After the page it goes on, and before Retranslate, which builds the bar from the tab
    // in force — the Print tab, whose buttons read the form this creates.
    ResetPrintTab();

    Retranslate();
    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::MakeComingSoonTab(QWidget *parent) -> QWidget *
{
    auto *page = new QWidget(parent);
    auto *layout = new QVBoxLayout(page);

    auto *label = new QLabel(page);
    label->setAlignment(Qt::AlignCenter);
    label->setObjectName(QStringLiteral("comingSoonLabel"));
    label->setText(OExportModeDialog::tr("Coming soon"));
    layout->addWidget(label);

    return page;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::ResetPrintTab()
{
    delete m_printWidget;

    m_printWidget = new OExportPrintWidget(m_data, m_measurements, m_context, m_printTab);
    m_printLayout->insertWidget(0, m_printWidget);

    connect(m_printWidget, &OExportPrintWidget::Changed, this, &OExportModeDialog::OnFormChanged);

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::DescribedOutputs() const -> QVector<OExportQueueItem>
{
    QVector<OExportQueueItem> outputs;

    const bool cutting = m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex;
    if (not cutting || m_cuttingWidget == nullptr || m_cuttingWidget->IsSingleSize())
    {
        OExportQueueItem one;
        one.name = CurrentName();
        one.request = CurrentRequest();
        outputs.append(one);
        return outputs;
    }

    // Le produit cartésien, axe par axe — « tailles 32, 34, 36 × statures 160, 168 » donne six
    // sorties. On garde le slot de chaque axe, parce que c'est lui qui nomme X, Y, W dans la requête.
    QVector<int> axisSlots;
    QVector<QVector<qreal>> values;
    QVector<QStringList> names;
    for (int slot = 0; slot < 3; ++slot)
    {
        const QVector<qreal> bases = m_cuttingWidget->CheckedBases(slot);
        if (not bases.isEmpty())
        {
            axisSlots.append(slot);
            values.append(bases);
            names.append(m_cuttingWidget->CheckedLabels(slot));
        }
    }

    if (values.isEmpty())
    {
        return outputs;
    }

    const QString token = CuttingToken();

    // Les noms se réservent au fur et à mesure, **contre les entrées de ce même appel aussi** :
    // six sorties nommées d'un seul tenant porteraient sinon six fois le même nom.
    QStringList taken = m_takenNames;
    taken += m_queue.Names();

    QVector<int> cursor(values.size(), 0);
    for (;;)
    {
        OExportRunRequest request = CurrentRequest();
        QStringList sizeLabels;
        for (int axis = 0; axis < static_cast<int>(values.size()); ++axis)
        {
            const qreal base = values.at(axis).at(cursor.at(axis));
            switch (axisSlots.at(axis))
            {
                case 0: request.baseA = base; break;
                case 1: request.baseB = base; break;
                default: request.baseC = base; break;
            }
            sizeLabels.append(names.at(axis).at(cursor.at(axis)));
        }

        OExportQueueItem item;
        item.request = request;
        item.name = OExportNameBuilder::Unique(OExportNameBuilder::Base(QString(), sizeLabels, token), taken);
        taken.append(item.name);
        outputs.append(item);

        // Avance d'un cran dans le produit, du dernier axe vers le premier.
        int axis = static_cast<int>(cursor.size()) - 1;
        for (; axis >= 0; --axis)
        {
            if (++cursor[axis] < values.at(axis).size())
            {
                break;
            }
            cursor[axis] = 0;
        }
        if (axis < 0)
        {
            break; // le tour complet est fait
        }
    }

    return outputs;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::ResetCuttingTab()
{
    delete m_cuttingWidget;

    m_cuttingWidget = new OExportCuttingWidget(m_data, m_measurements, m_cuttingTab);
    m_cuttingLayout->insertWidget(0, m_cuttingWidget);

    connect(m_cuttingWidget, &OExportCuttingWidget::Changed, this, &OExportModeDialog::OnFormChanged);

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::CurrentRequest() const -> OExportRunRequest
{
    OExportRunRequest request;

    // Two tabs describe a run, and they describe it their own way: the Print tab says which sheet
    // it prints on, the Cutting plan tab how wide the fabric is — **and both now say the two spaces
    // that hold the pieces off each other and off the fabric's edges.** Nothing is left at the
    // request's default on this path.
    if (m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex)
    {
        if (m_cuttingWidget == nullptr)
        {
            return request;
        }

        request.documentWidthMm = m_cuttingWidget->FabricWidthMm();

        // L'unité du run est celle de la LAIZE : le monde dans lequel l'utilisatrice achète son
        // tissu, et celui dans lequel elle vient de taper son chiffre. Elle ne pèse sur aucun
        // résultat — la laize est stockée en millimètres juste au-dessus, et le droit-fil gelé se
        // mesure dans l'unité du PATRON, jamais dans celle-ci (voir RunExportNesting).
        request.unit = m_cuttingWidget->DisplayUnit();
        request.singleSize = true;
        request.gapMm = m_cuttingWidget->GapMm();

        // **L'espace depuis les bords, et c'est ICI qu'il devient la marge du run.** Le champ de
        // l'onglet porte le nom du tissu, la requête porte le nom du modèle — `marginMm` — et les
        // deux sont la même grandeur : ce qui reste libre depuis le bord du bac. **Un seul nombre,
        // deux lectures** : un bac déplié le met sur ses quatre côtés, un bac plié le répartit par
        // rectangle, en exemptant le bord de chaque pli (`SeatMargins`, dans oexportnesting.cpp).
        //
        // ⚠️ Et il n'y a donc plus rien à forcer ici : une marge de `m` sur le côté du pli écartait
        // la pièce au pli du pli, et la pièce dépliée sortait trop étroite de `2 m` — c'était la
        // raison du `if (foldedFabric) marginMm = 0.0` d'hier. C'est le nesting qui porte
        // désormais cette règle, par rectangle, parce que lui seul sait de quel côté est le pli.
        request.marginMm = m_cuttingWidget->EdgeMarginMm();
        request.baseA = m_cuttingWidget->DimensionBase(0);
        request.baseB = m_cuttingWidget->DimensionBase(1);
        request.baseC = m_cuttingWidget->DimensionBase(2);
        request.groupMemberChoices = m_cuttingWidget->MemberChoices();
        request.cuttingPlan = true;
        request.oneWayLayout = m_cuttingWidget->IsOneWayLayout();
        request.foldedFabric = m_cuttingWidget->IsFoldedFabric();

        return request;
    }

    request.documentWidthMm = m_printWidget->DocumentWidthMm();
    request.unit = m_printWidget->DisplayUnit();
    request.rangeName = m_printWidget->SizeRangeName();
    request.singleSize = m_printWidget->IsSingleSize();
    request.baseA = m_printWidget->DimensionBase(0);
    request.baseB = m_printWidget->DimensionBase(1);
    request.baseC = m_printWidget->DimensionBase(2);
    request.marginMm = m_printWidget->MarginMm();
    request.gapMm = m_printWidget->GapMm();
    request.printModelToken = m_printWidget->PrintModelToken();
    request.columns = m_printWidget->Columns();
    request.frameHeightMm = m_printWidget->FrameHeightMm();
    request.marksTemplateName = m_printWidget->MarksTemplateName();
    request.coverPageTemplateName = m_printWidget->CoverPageTemplateName();
    return request;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::CurrentName() const -> QString
{
    const OExportRunRequest request = CurrentRequest();

    // A single size is named by the axes it sits on, read as the user reads them — the
    // label the combo shows ("160", "32"), never the base underneath. A range is named by
    // the range. An axis the table does not carry contributes nothing and is skipped.
    // ⚠️ **Quel onglet décrit le run** — et il commande les DEUX morceaux du nom. Les axes se
    //    lisent sur l'onglet en cours : lire ceux de l'onglet Impression pendant que
    //    l'utilisatrice remplit celui du plan de coupe nommait la ligne d'après des combos que
    //    PERSONNE n'a touchés — ils restent sur leur première entrée, donc sur les plus petites
    //    valeurs du patron. La laize, elle, était déjà lue au bon endroit, d'où un nom dont seule
    //    la fin changeait. Trouvé par Sophie.
    const bool cutting = m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex;

    QStringList bases;
    if (request.singleSize)
    {
        for (int slot = 0; slot < 3; ++slot)
        {
            bases.append(cutting && m_cuttingWidget != nullptr ? m_cuttingWidget->DimensionLabel(slot)
                                                               : m_printWidget->DimensionLabel(slot));
        }
    }

    // Un plan de coupe n'a pas de modèle d'impression : il sort sur un rouleau de tissu, dont la
    // largeur EST la laize — et le jeton qui le dit est écrit une seule fois, pour ses deux lecteurs.
    const QString token = cutting && m_cuttingWidget != nullptr ? CuttingToken() : request.printModelToken;

    const QString base = OExportNameBuilder::Base(request.singleSize ? QString() : request.rangeName,
                                                  bases, token);

    // The lines of the list count as taken just as much as the generations the dock lists:
    // a line is named when it is added, so two identical runs added one after the other
    // would otherwise be born carrying the same name.
    QStringList taken = m_takenNames;
    taken += m_queue.Names();

    return OExportNameBuilder::Unique(base, taken);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::CuttingToken() const -> QString
{
    if (m_cuttingWidget == nullptr)
    {
        return {};
    }

    QString token = QStringLiteral("Cut-%1").arg(
        OExportUnitFormat::Format(m_cuttingWidget->FabricWidthMm(), Unit::Cm));

    // « Plié » est un jeton de plus, pour la même raison que la laize : le même patron à 140 déplié
    // et à 140 plié sont deux plans différents, et le nom est le seul endroit où la différence se
    // voit. ⚠️ La laize, elle, reste celle du tissu acheté : le nesting tourne sur la moitié, mais
    // personne n'achète 70.
    if (m_cuttingWidget->IsFoldedFabric())
    {
        token += QStringLiteral("-Folded");
    }

    return token;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetRunGenerator(RunGenerator generator)
{
    m_generator = std::move(generator);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::HasGenerated() const -> bool
{
    return m_generated;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnAddToList()
{
    // The button exists on two tabs now, and each has its own idea of a run that can be filed.
    const bool cutting = m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex;
    const bool valid = cutting ? m_cuttingWidget != nullptr && m_cuttingWidget->IsValid()
                               : m_printWidget != nullptr && m_printWidget->IsValid();

    if (m_batchRunning || not valid)
    {
        return;
    }

    // Named before it is filed, and filed under that name: the list shows it, and what the
    // list shows is what the run will be called. Une description en décrit parfois plusieurs —
    // « plusieurs tailles » en décrit une par combinaison d'axes.
    const QVector<OExportQueueItem> outputs = DescribedOutputs();
    const QString name = outputs.isEmpty() ? QString() : outputs.constFirst().name;

    for (const OExportQueueItem &item : outputs)
    {
        m_queue.Append(item);
        m_listWidget->AppendRow(item.name);
    }
    RefreshGenerateButtons();

    // The form is spent: what it described is in the list now, and the next run starts
    // where this one started. Reset before the message, never after — rebuilding the form
    // raises Changed, and Changed is what clears the message.
    if (cutting)
    {
        ResetCuttingTab();
    }
    else
    {
        ResetPrintTab();
    }
    ShowAddedMessage(name);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnGenerateCutting()
{
    if (m_batchRunning || m_cuttingWidget == nullptr)
    {
        return;
    }

    // A cutting plan lays pieces on fabric, and fabric has a grain. The nesting cannot invent a
    // direction: a plan launched on a piece that has none comes out looking right and being wrong,
    // which is the one failure this mode cannot afford. So it is refused, and the pieces to fix
    // are named — the number she gave them, because a piece name is not unique.
    const QVector<OExportCuttingGuard::FaultyPiece> faulty =
        OExportCuttingGuard::FaultyPieces(m_cuttingWidget->MemberChoices(), m_data);

    if (not faulty.isEmpty())
    {
        QStringList pieces;
        pieces.reserve(faulty.size());
        for (const OExportCuttingGuard::FaultyPiece &piece : faulty)
        {
            pieces.append(QStringLiteral("%1. %2").arg(piece.number).arg(piece.name));
        }

        QMessageBox::warning(
            this, tr("Cutting plan"),
            tr("The nesting cannot invent a direction, and these pieces have none:\n\n%1\n\n"
               "Give each one a grainline or a fold line in the Details mode, then ask for the "
               "plan again.")
                .arg(pieces.join(QLatin1Char('\n'))));
        return;
    }

    // **Tout ce que le formulaire décrit, toujours — même quand cela ne vaut qu'une sortie.**
    //
    // « Générer cette sortie » veut dire *tout de suite* par opposition à *plus tard*, pas *une seule*
    // par opposition à *toutes* : on enchaîne donc ici la sélection entière, comme la liste le fait,
    // et la fenêtre n'a pas à connaître ce découpage.
    //
    // ⚠️ **Et surtout pas le raccourci d'avant** — « une seule sortie décrite : la fenêtre s'en charge
    // après la fermeture ». Sous « Several sizes », cocher UNE seule taille décrit bien une sortie
    // unique, et le raccourci laissait alors la fenêtre générer `CurrentRequest()` : sur cet onglet
    // c'est une requête de TAILLE UNIQUE, lue dans les combos mono-taille restés à leur premier index.
    // La taille cochée était donc silencieusement remplacée par la plus petite de chaque axe.
    // « Une seule chose décrite » et « ce que la fenêtre décrit » ne sont pas la même chose, et c'est
    // ce raccourci qui les confondait.
    const QVector<OExportQueueItem> outputs = DescribedOutputs();

    SetBatchRunning(true);
    for (const OExportQueueItem &item : outputs)
    {
        QCoreApplication::processEvents();
        // Le compte rendu de chaque run est porté par le générateur lui-même : ce que ce dialogue doit
        // retenir, c'est qu'il a FAIT son lot, pas ce que chaque run est devenu.
        m_generator(item.request, item.name);
    }
    SetBatchRunning(false);

    // ⚠️ **« A fait son lot », et pas « a réussi ».**
    //
    // C'est ce que la fenêtre doit savoir pour ne pas générer une seconde fois (`oexportmode.cpp:290`) :
    // si le lot échouait ENTIER, le drapeau resterait faux et elle retenterait `CurrentRequest()` — une
    // requête de taille unique lue dans les combos mono-taille, donc la sortie fantôme que ce dialogue
    // vient précisément de cesser de produire.
    //
    // Le dialogue a décrit, exécuté et refermé : son lot est fait, réussisse-t-il ou non. La branche
    // d'annulation, elle, garde l'autre sens — « a réussi » — et c'est pourquoi ce drapeau est posé ici
    // en dur plutôt que par le `done` de chaque run.
    m_generated = true;

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnGenerateAll()
{
    if (m_batchRunning || not m_generator || m_queue.IsEmpty())
    {
        return;
    }

    SetBatchRunning(true);

    for (int index = 0; index < m_queue.Count(); ++index)
    {
        // Read out before the call, never held across it: the line belongs to the queue,
        // and the generator below runs an event loop of its own while the engine works.
        const OExportQueueItem item = m_queue.At(index);

        m_queue.SetState(index, OExportRunState::Running);
        m_listWidget->SetState(index, OExportRunState::Running);
        // The list is what the user watches while this takes its time. Without a repaint
        // here nothing of the batch would be drawn until the whole of it was over.
        QCoreApplication::processEvents();

        const bool done = m_generator(item.request, item.name);

        // A line the engine refused does not stop the walk: the other runs were asked for
        // too, and the one that failed says so on its own line.
        m_queue.SetState(index, done ? OExportRunState::Done : OExportRunState::Failed);
        m_listWidget->SetState(index, done ? OExportRunState::Done : OExportRunState::Failed);
        m_generated = m_generated || done;
    }

    SetBatchRunning(false);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::ShowAddedMessage(const QString &name)
{
    m_addedName = name;
    m_addedLabel->setText(tr("%1 was added to the list.").arg(m_addedName));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnFormChanged()
{
    // The message reports one add. The first field touched after it makes it stale — the
    // user is describing something else now.
    m_addedName.clear();
    m_addedLabel->clear();

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnTabChanged()
{
    // The confirmation belongs to the page it was made on.
    m_addedName.clear();
    m_addedLabel->clear();

    RebuildButtonBox();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::RebuildButtonBox()
{
    for (QAbstractButton *button : m_buttonBox->buttons())
    {
        // removeButton hands the button back unparented: deleting it is ours to do, and not
        // doing it would stack one tab's buttons on the next one's.
        m_buttonBox->removeButton(button);
        delete button;
    }

    m_generateButton = nullptr;
    m_addToListButton = nullptr;
    m_generateAllButton = nullptr;

    switch (m_tabs->currentIndex())
    {
        case kPrintTabIndex:
            m_addToListButton = m_buttonBox->addButton(tr("Add to list"), QDialogButtonBox::ActionRole);
            m_addToListButton->setAutoDefault(false);
            connect(m_addToListButton, &QPushButton::clicked, this, &OExportModeDialog::OnAddToList);

            // « this output » : le bouton dit exactement ce qu'il fait, et le distingue de la
            // génération de toute une liste, qui est l'autre bouton de l'autre onglet.
            m_generateButton = m_buttonBox->addButton(tr("Generate this output"), QDialogButtonBox::AcceptRole);

            m_buttonBox->addButton(QDialogButtonBox::Cancel);
            break;

        case kCuttingTabIndex:
            // Same two ways out as the Print tab: file the description and describe the next one,
            // or run this one now. The difference is *when*, not *what*.
            m_addToListButton = m_buttonBox->addButton(tr("Add to list"), QDialogButtonBox::ActionRole);
            m_addToListButton->setAutoDefault(false);
            connect(m_addToListButton, &QPushButton::clicked, this, &OExportModeDialog::OnAddToList);

            // An action button rather than an accept one, unlike the Print tab: the plan has to be
            // checked before it may run, and an AcceptRole button accepts whether or not anyone
            // looked. Return still reaches it — it is the default button of the bar.
            m_generateButton = m_buttonBox->addButton(tr("Generate this output"), QDialogButtonBox::ActionRole);
            m_generateButton->setAutoDefault(false);
            connect(m_generateButton, &QPushButton::clicked, this, &OExportModeDialog::OnGenerateCutting);

            m_buttonBox->addButton(QDialogButtonBox::Cancel);
            break;

        case kListTabIndex:
            m_generateAllButton = m_buttonBox->addButton(tr("Generate the whole list"),
                                                         QDialogButtonBox::ActionRole);
            m_generateAllButton->setAutoDefault(false);
            connect(m_generateAllButton, &QPushButton::clicked, this, &OExportModeDialog::OnGenerateAll);

            m_buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);
            break;

        default:
            // Nothing on these pages can be acted on: the way out is the only honest thing
            // to offer.
            m_buttonBox->addButton(tr("Close"), QDialogButtonBox::RejectRole);
            break;
    }

    // Asserted last, once every button is in: the box lays its buttons out as they arrive
    // and can hand the default to the one added after Generate — Return would then close
    // the dialog instead of nesting what the tab describes.
    if (m_generateButton != nullptr)
    {
        m_generateButton->setDefault(true);
    }

    m_buttonBox->setEnabled(!m_batchRunning);
    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetBatchRunning(bool running)
{
    m_batchRunning = running;

    // The list is locked but kept legible, not greyed: it is what the batch writes to, and
    // its states are what the user is watching. Everything else can go grey — while a run
    // is in the engine, nothing on the Print tab can be acted on.
    m_listWidget->SetBusy(running);
    m_printWidget->setEnabled(!running);
    if (m_cuttingWidget != nullptr)
    {
        m_cuttingWidget->setEnabled(!running);
    }
    // The bar as well: mid-batch there is nothing to confirm, nothing to add and nowhere to
    // go — and reject() refuses the close anyway, so an enabled button would be a button
    // that lies.
    m_buttonBox->setEnabled(!running);

    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::RefreshGenerateButtons()
{
    // Each is only alive on its own tab: the bar carries one tab's buttons at a time. Generate is
    // shared by two tabs, and each has its own idea of a run that can be asked for — a sheet for
    // the Print tab, a laize and one piece per group for the Cutting plan.
    const bool printValid = m_printWidget != nullptr && m_printWidget->IsValid() && not m_batchRunning;
    const bool cuttingValid = m_cuttingWidget != nullptr && m_cuttingWidget->IsValid() && not m_batchRunning;
    const bool valid = m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex ? cuttingValid : printValid;

    if (m_generateButton != nullptr)
    {
        m_generateButton->setEnabled(valid);
    }
    if (m_addToListButton != nullptr)
    {
        m_addToListButton->setEnabled(printValid);
    }
    if (m_generateAllButton != nullptr)
    {
        m_generateAllButton->setEnabled(not m_queue.IsEmpty() && not m_batchRunning);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::reject()
{
    if (m_batchRunning)
    {
        return;
    }

    QDialog::reject();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::Retranslate()
{
    setWindowTitle(tr("Export"));

    m_tabs->setTabText(kListTabIndex, tr("List to generate"));
    m_tabs->setTabText(kCuttingTabIndex, tr("Cutting plan"));
    m_tabs->setTabText(kPrintTabIndex, tr("Print"));
    m_tabs->setTabText(kProjectorTabIndex, tr("Projector"));

    // Read back off the page rather than kept beside it: a second copy of the label would be one
    // more thing to hold together, and the label is right there.
    // tr() rather than QObject::tr(): the placeholder belongs to this dialog, and a string filed
    // under QObject lands in a context every widget in the application shares.
    if (auto *label = m_projectorTab->findChild<QLabel *>(QStringLiteral("comingSoonLabel")))
    {
        label->setText(tr("Coming soon"));
    }

    m_addedLabel->setText(m_addedName.isEmpty() ? QString() : tr("%1 was added to the list.").arg(m_addedName));

    // The bar is rebuilt rather than rewritten: its buttons belong to the tab in force, and
    // the three that exist at any one time are not the same three in every language.
    RebuildButtonBox();

    // Last, because it reads the titles written just above — and because the tab bar only
    // knows how wide it wants to be once it carries them.
    AdjustMinimumWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::AdjustMinimumWidth()
{
    const QTabBar *bar = m_tabs->tabBar();
    const int tabBarWidth = bar != nullptr ? bar->sizeHint().width() : 0;

    setMinimumWidth(qMax(kMinDialogWidth, tabBarWidth + kTabBarChrome));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
