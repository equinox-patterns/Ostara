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
#include "oexportfavoriteswidget.h"
#include "oexportgenerationlistwidget.h"
#include "oexportplotterwidget.h"
#include "oexportprintwidget.h"

#include "oexportbatchpreflightdialog.h"
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
/// The tab that holds the runs described but not fired. **C'est la page sur laquelle le dialogue
/// s'ouvre** : c'est là que se voit le travail déjà fait — les runs décrits, et les favoris gardés
/// — et une liste vide le dit elle-même, là où un formulaire d'impression vierge ne dit rien.
constexpr int kListTabIndex = 0;

/// The tab that cuts pieces out of fabric, one size at a time.
constexpr int kCuttingTabIndex = 1;

/// Where the Print tab sits.
constexpr int kPrintTabIndex = 2;

/// The tab that cuts a pattern on the mats of a domestic plotter.
constexpr int kPlotterTabIndex = 3;

// ── Les sous-onglets de la page « à générer » ────────────────────────────────────────────────
//
// La liste de travail, puis une liste de favoris par type. Les favoris sont triés par la recette
// elle-même (`cuttingPlan`, `plotterPlan`, ni l'un ni l'autre pour un print) : il n'y a pas de
// champ « type » à tenir en accord avec elle.
constexpr int kWorkingListTabIndex = 0;
constexpr int kCuttingFavoritesTabIndex = 1;
constexpr int kPrintFavoritesTabIndex = 2;
constexpr int kPlotterFavoritesTabIndex = 3;

/// Below this the dialog is cramped, whatever the tab bars need.
///
/// ⚠️ `setMinimumWidth()` is an **explicit** minimum: Qt then stops enforcing the layout's own
/// minimum (`QLayout::SetDefaultConstraint` yields to a widget that already has one), so whatever
/// the formula below forgets is not merely cramped — it is cut. Measured on a bench (French labels,
/// default style): the inner tab bar asks for 537 px, +kTabBarChrome = 577, so this constant was the
/// binding value and the dialog opened at 560 whatever the list page needed. Raised on
/// request (25-09) so the page that holds the runs — their rows and their bottom bar — has room to
/// spare, the vertical scrollbar included.
constexpr int kMinDialogWidth = 800;

/// What the tab widget's frame and margins add around the tab bar itself.
constexpr int kTabBarChrome = 40;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportModeDialog::OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                     const OGradedSizeRanges::Context &context, const QStringList &takenNames,
                                     const QVector<OExportFavorite> &favorites, QWidget *parent)
  : QDialog(parent),
    m_data(data),
    m_measurements(measurements),
    m_context(context),
    m_favorites(favorites),
    m_takenNames(takenNames)
{
    setWindowTitle(tr("Export"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    m_tabs = new QTabWidget(this);

    // ── La page 0 : la liste de travail, et les favoris ───────────────────
    //
    // La page est un CONTENEUR, et non plus la liste elle-même : quatre sous-onglets y vivent — la
    // liste de travail, et une liste de favoris par type de plan. Le dispatch des boutons ne bouge
    // pas pour autant : il lit toujours l'onglet EXTERNE (m_tabs), et la page 0 reste la page 0.
    m_listPage = new QWidget(m_tabs);
    auto *listPageLayout = new QVBoxLayout(m_listPage);
    listPageLayout->setContentsMargins(0, 0, 0, 0);

    m_listTabs = new QTabWidget(m_listPage);
    // La même règle que la barre extérieure : une barre à court de place raccourcit ses libellés ou
    // cache le surplus derrière des flèches, et ni l'un ni l'autre ne se lit.
    m_listTabs->setElideMode(Qt::ElideNone);
    m_listTabs->setUsesScrollButtons(false);

    m_listWidget = new OExportGenerationListWidget(m_listTabs);
    connect(m_listWidget, &OExportGenerationListWidget::RemoveRequested, this,
            [this](int index)
            {
                // The queue first, then the row: the row is a view of it, and a view that
                // ran ahead would be showing a line the batch could still walk into.
                m_queue.RemoveAt(index);
                m_listWidget->RemoveRow(index);
                RefreshGenerateButtons();
            });

    m_cuttingFavorites = new OExportFavoritesWidget(m_listTabs);
    m_printFavorites = new OExportFavoritesWidget(m_listTabs);
    m_plotterFavorites = new OExportFavoritesWidget(m_listTabs);

    m_listTabs->addTab(m_listWidget, QString());
    m_listTabs->addTab(m_cuttingFavorites, QString());
    m_listTabs->addTab(m_printFavorites, QString());
    m_listTabs->addTab(m_plotterFavorites, QString());
    m_listTabs->setTabToolTip(kWorkingListTabIndex,
                              tr("The runs described in the tabs beside, waiting to be generated.\n"
                                 "They are not kept: closing the pattern takes them with the session."));

    for (OExportFavoritesWidget *favoritesView : {m_cuttingFavorites, m_printFavorites, m_plotterFavorites})
    {
        // Ce qui est coché décide de ce que le lot lance : le bouton suit chaque case, sinon il
        // promettrait un lot vide ou refuserait un lot plein.
        connect(favoritesView, &OExportFavoritesWidget::CheckedChanged, this,
                &OExportModeDialog::RefreshGenerateButtons);
    }
    connect(m_cuttingFavorites, &OExportFavoritesWidget::RemoveRequested, this,
            [this](const QString &name) { OnFavoriteRemoveRequested(name, m_cuttingFavorites); });
    connect(m_printFavorites, &OExportFavoritesWidget::RemoveRequested, this,
            [this](const QString &name) { OnFavoriteRemoveRequested(name, m_printFavorites); });
    connect(m_plotterFavorites, &OExportFavoritesWidget::RemoveRequested, this,
            [this](const QString &name) { OnFavoriteRemoveRequested(name, m_plotterFavorites); });

    listPageLayout->addWidget(m_listTabs);

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

    m_plotterTab = new QWidget(m_tabs);
    m_plotterLayout = new QVBoxLayout(m_plotterTab);
    // The stretch keeps the form at the top of the page; the widget is inserted before it.
    m_plotterLayout->addStretch();

    m_plotterWidget = new OExportPlotterWidget(m_data, m_measurements, m_plotterTab);
    m_plotterLayout->insertWidget(0, m_plotterWidget);
    connect(m_plotterWidget, &OExportPlotterWidget::Changed, this, &OExportModeDialog::OnFormChanged);

    // A tab bar that runs out of room shortens its labels, or hides the surplus behind
    // scroll arrows. Neither is readable, so the dialog is widened to the labels instead
    // — see AdjustMinimumWidth, which Retranslate calls once the titles are known.
    m_tabs->setElideMode(Qt::ElideNone);
    m_tabs->setUsesScrollButtons(false);

    m_tabs->addTab(m_listPage, QString());
    m_tabs->addTab(m_cuttingTab, QString());
    m_tabs->addTab(m_printTab, QString());
    m_tabs->addTab(m_plotterTab, QString());
    m_tabs->setCurrentIndex(kListTabIndex);
    layout->addWidget(m_tabs);

    // Après les onglets et avant Retranslate, dont RebuildButtonBox demande l'état du bouton du lot
    // — lequel dépend de ce qui est coché ici.
    PopulateFavorites(m_favorites);

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

    // Le sous-onglet interne ne change pas de page : il change la PORTÉE du lot et le libellé du
    // bouton, tous deux lus au moment où la barre se construit. On la refait donc, par le même chemin
    // qu'un changement d'onglet externe — qui finit par RefreshGenerateButtons, donc par le même
    // critère que le clic.
    connect(m_listTabs, &QTabWidget::currentChanged, this, [this](int) { RebuildButtonBox(); });

    // The form of the Print tab, avant Retranslate : le dialogue s'ouvre sur la liste, dont les
    // boutons ne lisent pas ce formulaire, mais RefreshGenerateButtons lit sa validité — et un onglet
    // Print sans formulaire ferait un « Generate this output » mort.
    ResetPrintTab();

    Retranslate();
    RefreshGenerateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::ResetPlotterTab()
{
    delete m_plotterWidget;

    m_plotterWidget = new OExportPlotterWidget(m_data, m_measurements, m_plotterTab);
    m_plotterLayout->insertWidget(0, m_plotterWidget);

    connect(m_plotterWidget, &OExportPlotterWidget::Changed, this, &OExportModeDialog::OnFormChanged);

    RefreshGenerateButtons();
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

    if (m_tabs != nullptr && m_tabs->currentIndex() == kPlotterTabIndex)
    {
        if (m_plotterWidget == nullptr)
        {
            return request;
        }

        request.unit = m_plotterWidget->MatUnit();
        request.singleSize = true;
        request.baseA = m_plotterWidget->DimensionBase(0);
        request.baseB = m_plotterWidget->DimensionBase(1);
        request.baseC = m_plotterWidget->DimensionBase(2);

        // La destination. C'est elle qui fait découper la géométrie en morceaux — et c'est elle
        // aussi qui décidera du dialogue que le bouton d'export ouvrira.
        request.plotterPlan = true;
        request.matWidthMm = m_plotterWidget->MatWidthMm();
        request.matHeightMm = m_plotterWidget->MatHeightMm();
        request.matUnit = m_plotterWidget->MatUnit();

        // **Et les membres de groupe que l'utilisatrice a décochés.** Le plotter décompose les
        // groupes — chaque membre est une pièce à couper — donc ces cases ne choisissent pas un
        // membre, elles en écartent : c'est une liste d'EXCLUSIONS, et vide veut dire « toutes ».
        // Un `groupMemberChoices` ici serait le contresens exact : il ne garderait qu'un membre par
        // groupe, là où un plotter les coupe tous.
        request.excludedPieces = m_plotterWidget->ExcludedPieces();

        // Et rien d'autre : ni laize, ni pli, ni espacement, ni marge — un plotter coupe un patron,
        // une fois, sur un tapis. **Ni modèle de repères ni page de garde** : les laisser nommés
        // poserait des repères et un second document sur un fichier de découpe.
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
    //    la fin changeait. Trouvé.
    const bool cutting = m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex;
    const bool plotter = m_tabs != nullptr && m_tabs->currentIndex() == kPlotterTabIndex;

    QStringList bases;
    if (request.singleSize)
    {
        for (int slot = 0; slot < 3; ++slot)
        {
            if (cutting && m_cuttingWidget != nullptr)
            {
                bases.append(m_cuttingWidget->DimensionLabel(slot));
            }
            else if (plotter && m_plotterWidget != nullptr)
            {
                bases.append(m_plotterWidget->DimensionLabel(slot));
            }
            else
            {
                bases.append(m_printWidget->DimensionLabel(slot));
            }
        }
    }

    // Un plan de coupe n'a pas de modèle d'impression : il sort sur un rouleau de tissu, dont la
    // largeur EST la laize — et le jeton qui le dit est écrit une seule fois, pour ses deux lecteurs.
    // Un plan plotter non plus : sa largeur est celle de son tapis, et son jeton dit lequel.
    const QString token = cutting && m_cuttingWidget != nullptr ? CuttingToken()
                         : plotter && m_plotterWidget != nullptr ? PlotterToken()
                                                                 : request.printModelToken;

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
//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::PlotterToken() const -> QString
{
    if (m_plotterWidget == nullptr)
    {
        return {};
    }

    // Le tapis, comme la laize pour le plan de coupe : deux tapis différents donnent deux
    // découpages différents — un autre nombre de morceaux, à d'autres endroits — et le nom est le
    // seul endroit où la différence se voit. La zone utile est nommée en centimètres, l'unité dans
    // laquelle on la lit.
    return QStringLiteral("Plot-%1x%2")
        .arg(OExportUnitFormat::Format(m_plotterWidget->MatWidthMm(), Unit::Cm),
             OExportUnitFormat::Format(m_plotterWidget->MatHeightMm(), Unit::Cm));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetRunGenerator(RunGenerator generator)
{
    m_generator = std::move(generator);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetFavoriteRemover(FavoriteRemover remover)
{
    m_favoriteRemover = std::move(remover);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::KindOf(const OExportRunRequest &request) -> FavoriteKind
{
    if (request.cuttingPlan)
    {
        return FavoriteKind::Cutting;
    }
    if (request.plotterPlan)
    {
        return FavoriteKind::Plotter;
    }
    return FavoriteKind::Print;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::ViewFor(FavoriteKind kind) const -> OExportFavoritesWidget *
{
    switch (kind)
    {
        case FavoriteKind::Cutting:
            return m_cuttingFavorites;
        case FavoriteKind::Plotter:
            return m_plotterFavorites;
        case FavoriteKind::Print:
        default:
            return m_printFavorites;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::PopulateFavorites(const QVector<OExportFavorite> &favorites)
{
    QStringList cutting;
    QStringList print;
    QStringList plotter;

    for (const OExportFavorite &favorite : favorites)
    {
        switch (KindOf(favorite.request))
        {
            case FavoriteKind::Cutting:
                cutting.append(favorite.name);
                break;
            case FavoriteKind::Plotter:
                plotter.append(favorite.name);
                break;
            case FavoriteKind::Print:
            default:
                print.append(favorite.name);
                break;
        }
    }

    m_cuttingFavorites->SetFavorites(cutting);
    m_printFavorites->SetFavorites(print);
    m_plotterFavorites->SetFavorites(plotter);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::CheckedFavorites(FavoriteKind kind) const -> QVector<OExportFavorite>
{
    const OExportFavoritesWidget *view = ViewFor(kind);

    QVector<OExportFavorite> checked;
    for (const OExportFavorite &favorite : m_favorites)
    {
        if (KindOf(favorite.request) != kind)
        {
            continue;
        }
        // La case est la seule réponse à « ce favori part-il ? » — on la lit, on ne la recalcule pas.
        if (view != nullptr && view->IsChecked(favorite.name))
        {
            checked.append(favorite);
        }
    }
    return checked;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::BatchJobs() const -> QVector<BatchJob>
{
    QVector<BatchJob> jobs;

    const int innerTab = m_listTabs != nullptr ? m_listTabs->currentIndex() : kWorkingListTabIndex;

    // Le sous-onglet EN COURS, et lui seul (décidé le 25-09). Le lot tournait les quatre
    // à la fois — la liste de travail ET les trois jeux de favoris — donc des runs que l'utilisatrice
    // n'avait pas sous les yeux, et qu'elle oubliait qu'elle lançait.
    if (innerTab == kWorkingListTabIndex)
    {
        jobs.reserve(m_queue.Count());

        // Dans son ordre : c'est celui dans lequel elle les a décrits, et celui qu'elle voit.
        for (int index = 0; index < m_queue.Count(); ++index)
        {
            const OExportQueueItem &item = m_queue.At(index);
            jobs.append(BatchJob{BatchJob::Source::List, index, item.name, item.request});
        }

        return jobs;
    }

    // ⚠️ L'index de la file reste ABSOLU : `SetJobState` s'en sert pour la file ET pour la ligne,
    // donc renuméroter la sous-liste poserait l'état d'un run sur un autre.
    auto appendChecked = [this, &jobs](FavoriteKind kind)
    {
        for (const OExportFavorite &favorite : CheckedFavorites(kind))
        {
            jobs.append(BatchJob{BatchJob::Source::Favorite, 0, favorite.name, favorite.request});
        }
    };

    switch (innerTab)
    {
        case kCuttingFavoritesTabIndex:
            appendChecked(FavoriteKind::Cutting);
            break;

        case kPrintFavoritesTabIndex:
            appendChecked(FavoriteKind::Print);
            break;

        case kPlotterFavoritesTabIndex:
            appendChecked(FavoriteKind::Plotter);
            break;

        default:
            break;
    }

    return jobs;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::SetJobState(const BatchJob &job, OExportRunState state)
{
    if (job.source == BatchJob::Source::List)
    {
        m_queue.SetState(job.index, state);
        m_listWidget->SetState(job.index, state);
        return;
    }

    if (OExportFavoritesWidget *view = ViewFor(KindOf(job.request)); view != nullptr)
    {
        view->SetState(job.name, state);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::OnFavoriteRemoveRequested(const QString &name, OExportFavoritesWidget *from)
{
    if (m_favoriteRemover != nullptr)
    {
        m_favoriteRemover(name);
    }

    // La ligne part, les autres gardent leur case : reposer toute la liste décocherait ce que
    // l'utilisatrice venait de régler.
    if (from != nullptr)
    {
        from->RemoveRow(name);
    }

    // Et le favori sort de ce que le lot parcourt : une recette qui n'existe plus ne se lance pas.
    for (int index = static_cast<int>(m_favorites.size()) - 1; index >= 0; --index)
    {
        if (m_favorites.at(index).name == name)
        {
            m_favorites.removeAt(index);
        }
    }

    RefreshGenerateButtons();
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
    else if (m_tabs != nullptr && m_tabs->currentIndex() == kPlotterTabIndex)
    {
        ResetPlotterTab();
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
    //
    // ⚠️ Les pièces sans cut ne sont pas dans cette liste : elles ont déjà quitté le plan (voir
    // `PiecesToCut`), donc on ne leur reproche pas un droit-fil dont elles n'ont pas besoin.
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
    if (m_batchRunning || not m_generator)
    {
        return;
    }

    const QVector<BatchJob> jobs = BatchJobs();
    if (jobs.isEmpty())
    {
        // Le même critère que l'état du bouton : allumé pour un lot vide, ce clic serait un clic
        // qui ne fait rien.
        return;
    }

    // Le pré-vol. Les favoris ne portent PAS leurs choix de pièces — ils dépendent des pièces du
    // patron, et c'est ce qui les rend transposables (voir OExportFavorite) : on les demande donc
    // une fois pour le lot, et seulement pour les types qu'il contient réellement.
    //
    // ⚠️ Lu sur le LOT, jamais sur les vues : depuis que la portée est le sous-onglet en cours, un
    // lot peut n'être fait que de la liste de travail — qui n'a rien à demander, chaque run ayant
    // été décrit dans son propre onglet — ou d'un seul type de favoris.
    bool cuttingFavorites = false;
    bool plotterFavorites = false;
    for (const BatchJob &job : jobs)
    {
        if (job.source != BatchJob::Source::Favorite)
        {
            continue;
        }
        cuttingFavorites = cuttingFavorites || job.request.cuttingPlan;
        plotterFavorites = plotterFavorites || job.request.plotterPlan;
    }

    QMap<quint32, quint32> groupChoices;
    QSet<quint32> excludedPieces;
    if (cuttingFavorites || plotterFavorites)
    {
        OExportBatchPreflightDialog preflight(m_data, cuttingFavorites, plotterFavorites, this);
        if (preflight.HasQuestions() && preflight.exec() != QDialog::Accepted)
        {
            // Annuler le pré-vol annule le lot : rien ne part, pas même la liste de travail.
            return;
        }
        groupChoices = preflight.GroupChoices();
        excludedPieces = preflight.ExcludedPieces();
    }

    SetBatchRunning(true);

    for (const BatchJob &job : jobs)
    {
        SetJobState(job, OExportRunState::Running);
        // Ce que le lot écrit est ce que l'utilisatrice regarde pendant qu'il prend son temps : sans
        // repaint ici, rien ne se dessinerait avant la fin du lot entier.
        QCoreApplication::processEvents();

        OExportRunRequest request = job.request;
        if (job.source == BatchJob::Source::Favorite)
        {
            // Les réponses du pré-vol, écrites au dernier moment — le seul endroit qui sache qu'elles
            // valent pour ce run-ci. Un favori de la liste de travail n'est pas concerné : il a été
            // décrit dans son onglet, où ses choix ont déjà été faits.
            if (request.cuttingPlan)
            {
                request.groupMemberChoices = groupChoices;
            }
            else if (request.plotterPlan)
            {
                request.excludedPieces = excludedPieces;
            }
        }

        const bool done = m_generator(request, job.name);

        // A run the engine refused does not stop the walk: the others were asked for too, and the
        // one that failed says so on its own line.
        SetJobState(job, done ? OExportRunState::Done : OExportRunState::Failed);
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
auto OExportModeDialog::GenerateAllLabel() const -> QString
{
    const int innerTab = m_listTabs != nullptr ? m_listTabs->currentIndex() : kWorkingListTabIndex;

    // La barre est commune aux quatre sous-onglets, et le bouton ne tourne plus que celui qui est
    // affiché : « the whole list » serait donc faux sur un onglet de favoris — exactement le
    // malentendu que la portée rétablie supprime. Le libellé le dit, faute de quoi l'utilisatrice
    // devrait se souvenir de ce que le bouton fait au lieu de le lire.
    return innerTab == kWorkingListTabIndex ? tr("Generate the whole list")
                                            : tr("Generate the checked favorites");
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::RebuildButtonBox()
{
    for (QAbstractButton *button : m_buttonBox->buttons())
    {
        // removeButton hands the button back unparented: destroying it is ours to do, and not
        // doing it would stack one tab's buttons on the next one's.
        //
        // ⚠️ **Différé, jamais `delete` sec.** Cette méthode est atteinte depuis le `clicked()` de
        // l'un de ces boutons : « Generate the whole list » tourne un lot, le lot laisse passer la
        // boucle d'événements entre deux runs (`processEvents`, pour que les états se dessinent), et
        // un clic sur un onglet pendant ce passage arrive ici. Détruire le bouton dont le signal est
        // encore sur la pile est exactement ce que `deleteLater` existe pour éviter ; la ligne suivante
        // reconstruit la barre, donc le bouton condamné a déjà quitté la boîte et n'est plus lu.
        m_buttonBox->removeButton(button);
        button->deleteLater();
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

        case kPlotterTabIndex:
            // Le plotter décrit un run à la fois, comme le plan de coupe. Son bouton Generate
            // accepte le dialogue sans générer lui-même : c'est `OExportMode::Run` qui génère
            // ensuite, une fois et une seule, à partir de `CurrentRequest()`.
            m_addToListButton = m_buttonBox->addButton(tr("Add to list"), QDialogButtonBox::ActionRole);
            m_addToListButton->setAutoDefault(false);
            connect(m_addToListButton, &QPushButton::clicked, this, &OExportModeDialog::OnAddToList);

            m_generateButton = m_buttonBox->addButton(tr("Generate this output"), QDialogButtonBox::AcceptRole);

            m_buttonBox->addButton(QDialogButtonBox::Cancel);
            break;

        case kListTabIndex:
            m_generateAllButton = m_buttonBox->addButton(GenerateAllLabel(), QDialogButtonBox::ActionRole);
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
    for (OExportFavoritesWidget *favorites : {m_cuttingFavorites, m_printFavorites, m_plotterFavorites})
    {
        // Les listes de favoris sont verrouillées de la même façon, et pour la même raison : c'est
        // là que le lot écrit ses états, et c'est ce que l'utilisatrice regarde.
        if (favorites != nullptr)
        {
            favorites->SetBusy(running);
        }
    }
    m_printWidget->setEnabled(!running);
    if (m_cuttingWidget != nullptr)
    {
        m_cuttingWidget->setEnabled(!running);
    }
    if (m_plotterWidget != nullptr)
    {
        m_plotterWidget->setEnabled(!running);
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
    const bool plotterValid = m_plotterWidget != nullptr && m_plotterWidget->IsValid() && not m_batchRunning;
    const bool valid = m_tabs != nullptr && m_tabs->currentIndex() == kCuttingTabIndex ? cuttingValid
                     : m_tabs != nullptr && m_tabs->currentIndex() == kPlotterTabIndex ? plotterValid
                                                                                       : printValid;

    if (m_generateButton != nullptr)
    {
        m_generateButton->setEnabled(valid);
    }
    if (m_addToListButton != nullptr)
    {
        // ⚠️ Gouverné par l'onglet EN COURS, et non par la validité de Print. Le bouton
        // « Add to list » du plan de coupe était gouverné par le formulaire d'impression — un défaut
        // d'origine, que le troisième onglet aurait aggravé.
        m_addToListButton->setEnabled(valid);
    }
    if (m_generateAllButton != nullptr)
    {
        // Le bouton et le lot lisent la MÊME fonction : la portée ne peut donc pas diverger entre
        // ce qui s'allume et ce qui part. C'est la seule façon de tenir la promesse que ce commentaire
        // portait déjà — un bouton allumé pour un lot vide serait un bouton qui ment, et un bouton
        // éteint pour un lot plein un bouton qui bloque — maintenant que « le lot » dépend du
        // sous-onglet en cours.
        m_generateAllButton->setEnabled(not BatchJobs().isEmpty() && not m_batchRunning);
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
    m_tabs->setTabText(kPlotterTabIndex, tr("Plotter"));

    // Les sous-onglets de la page 0. « Working list » plutôt que « List to generate » : c'est la
    // liste qu'on remplit et qu'on ne garde pas, et le mot doit le dire — les trois autres
    // sous-onglets, eux, sont sauvegardés.
    m_listTabs->setTabText(kWorkingListTabIndex, tr("Working list"));
    m_listTabs->setTabText(kCuttingFavoritesTabIndex, tr("Cutting plan favorites"));
    m_listTabs->setTabText(kPrintFavoritesTabIndex, tr("Print favorites"));
    m_listTabs->setTabText(kPlotterFavoritesTabIndex, tr("Plotter favorites"));

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

    // Et la barre IMBRIQUÉE, qui vit dans la page 0 : la fenêtre doit être assez large pour elle
    // aussi, sinon c'est elle qui élide ses titres — et quatre sous-onglets illisibles sont quatre
    // sous-onglets qu'on ne peut pas choisir.
    const QTabBar *innerBar = m_listTabs != nullptr ? m_listTabs->tabBar() : nullptr;
    const int innerBarWidth = innerBar != nullptr ? innerBar->sizeHint().width() + kTabBarChrome : 0;

    setMinimumWidth(qMax(kMinDialogWidth, qMax(tabBarWidth + kTabBarChrome, innerBarWidth)));
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
