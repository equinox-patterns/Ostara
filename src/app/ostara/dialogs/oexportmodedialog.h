/************************************************************************
 **
 **  @file   oexportmodedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode entry dialog — one tab per thing the mode can be
 **          asked for, and the list of runs it has been asked for.
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

#ifndef OEXPORTMODEDIALOG_H
#define OEXPORTMODEDIALOG_H

#include <QDialog>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

#include <functional>

#include "../core/oexportfavorites.h"
#include "../core/ogradedsizeranges.h"
#include "../core/oexportrunqueue.h"
#include "../vmisc/def.h"

class OExportProjectorWidget;

class OExportCuttingWidget;
class OExportFavoritesWidget;
class OExportGenerationListWidget;
class OExportPlotterWidget;
class OExportPrintWidget;
class QDialogButtonBox;
class QEvent;
class QLabel;
class QPushButton;
class QTabWidget;
class QVBoxLayout;
class VContainer;
class VMeasurements;

/**
 * @brief Entry dialog of the Export mode.
 *
 * The mode is asked for three different things — a cutting plan, a print, and a projection —
 * and they have almost nothing in common but the nesting underneath. Hence one tab each
 * rather than one form that grew a second meaning.
 *
 * <b>Le dialogue s'ouvre sur la liste</b> — la page 0, « List to generate » : celle qui montre ce
 * qui a déjà été décrit, les runs en attente et les favoris gardés, chacun dans sa sous-onglet. Une
 * liste vide le dit elle-même et renvoie vers l'onglet Print ; un formulaire vierge, lui, ne dirait
 * rien de ce qui existe déjà.
 *
 * Two ways out of it, and the difference matters. <b>Generate</b> nests what the Print tab
 * currently describes, once. <b>Add to list</b> nests nothing: it files the description in
 * the list to generate, to be fired later, on its own or with the rest of the list — which
 * is what lets a user describe six outputs and then leave the machine to them.
 *
 * <b>The button bar follows the tab.</b> The tabs have nothing to say to each other, so a
 * single bar under all of them would offer "Generate" and "Add to list" to a page where
 * there is nothing to describe, and would leave the page that holds the list with no way
 * out but a Cancel that cancels nothing. Each page therefore brings its own way out: the
 * Print tab its two verbs and a Cancel, the list its run-it-all and a Close, the pages that
 * are not built yet a Close alone.
 *
 * The dialog owns the list, and nothing else that a run needs: every value comes from the
 * Print tab, which is also the only side that can be read for a second copy of them. What
 * it does own is the naming — the axis labels a single size is named by are on the tab, and
 * the list a new name must not collide with is here.
 *
 * It cannot nest: it holds no document, no pattern and no scene. The nesting is handed in
 * as a RunGenerator instead, so the dialog asks and the window runs.
 */
class OExportModeDialog : public QDialog
{
    Q_OBJECT

public:
    /// @brief Nests one run and files the result under @p name.
    ///
    /// The name is settled here, when the line is added, and never re-derived: a batch
    /// hands each line the name the list is showing, which is the name the user agreed to.
    /// @return false when the run failed — the reason has already been shown.
    using RunGenerator = std::function<bool(const OExportRunRequest &request, const QString &name)>;

    /// @brief Drop the favorite named @p name from the software store.
    ///
    /// The same arrangement as the run generator, and for the same reason: the store is keyed by
    /// the (measurements table, axis) context, and the dialog holds no pattern to resolve one.
    /// The dialog asks, the window writes.
    using FavoriteRemover = std::function<void(const QString &name)>;

    /// @param context    the (measurements table, declared grading axis) the pattern is on.
    ///                   The dialog cannot resolve it: it holds no document. The range list
    ///                   is restricted to it, exactly as the Graded dock restricts its own.
    /// @param takenNames the names already listed in the dock, filtered exactly as the dock
    ///                   filters them. Naming happens here now, and a name is only unique
    ///                   against what the user can see.
    /// @param favorites  the favorites of that same context, already filtered the same way and
    ///                   with the recipes attached. The dialog shows them; it does not read the
    ///                   store, which is keyed by a context it cannot resolve.
    explicit OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                               const OGradedSizeRanges::Context &context, const QStringList &takenNames,
                               const QVector<OExportFavorite> &favorites, QWidget *parent = nullptr);

    /// @brief Everything the Print tab currently describes, in one piece.
    auto CurrentRequest() const -> OExportRunRequest;

    /// @brief The name the run currently described will be filed under.
    ///
    /// Derived, never typed. It is reserved against the generations the dock already lists
    /// and against the lines this dialog is holding: a line is named the moment it is
    /// added, so two identical lines are not born carrying the same name.
    auto CurrentName() const -> QString;

    /// @brief Le jeton qui dit ce que ce plan de coupe EST : la laize, et si le tissu est plié.
    ///
    /// ⚠️ **Une seule écriture du jeton, deux lecteurs** — la sortie décrite une par une
    /// (`CurrentName`) et les N sorties d'un lot (`DescribedOutputs`). Le second le composait pour
    /// son compte, et c'est exactement la faute qui a coûté la §16.3 : deux endroits qui répondent à
    /// la même question finissent par y répondre différemment.
    ///
    /// La laize y est parce que le même patron à 140 et à 150 sont deux plans différents ; « plié »
    /// pour la même raison, à 140 déplié et à 140 plié. Aucun des deux n'est ce sur quoi le nesting
    /// tourne — sur du tissu plié, il tourne sur la moitié.
    auto CuttingToken() const -> QString;

    /// @brief Le jeton de l'onglet Plotter, sur le même contrat que CuttingToken.
    auto PlotterToken() const -> QString;

    /// @brief Le jeton du nom d'une sortie projecteur — le mot seul, sans largeur.
    ///
    /// Un projecteur n'a pas de laize : sa largeur est cherchée par le run, et il n'y a donc rien
    /// à écrire dans le nom. La taille, elle, y est déjà — par ses axes, ou par le nom de la plage.
    auto ProjectorToken() const -> QString;

    /// @brief Hand in the one operation the dialog cannot do itself.
    void SetRunGenerator(RunGenerator generator);

    /// @brief Hand in the other one: dropping a favorite from the software store.
    void SetFavoriteRemover(FavoriteRemover remover);

    /// @brief Whether a batch has filed at least one generation.
    ///
    /// A dialog closed after a batch is refused, not accepted — and the caller must not
    /// read that refusal as a user giving up on the entry after the work was done.
    auto HasGenerated() const -> bool;

public slots:
    /// @brief Close the dialog. Refused while a batch is running: returning from exec()
    ///        mid-walk would unwind the caller's loop from under the runs still to come.
    ///        Escape and the close box both land here.
    void reject() override;

protected:
    void changeEvent(QEvent *event) override;

private:
    /// @brief Give the Print tab a form that has never been touched.
    ///
    /// Rebuilt rather than reset field by field: the state a fresh tab is in is the state
    /// its constructor leaves it in, and a Reset() would be a second list of defaults to
    /// keep in step with the first. What it described is in the list by the time this runs.
    void ResetPrintTab();

    /// @brief Give the Cutting plan tab a form that has never been touched — same doctrine as
    ///        ResetPrintTab: rebuilt, never reset field by field.
    void ResetCuttingTab();

    /// @brief Give the Plotter tab a form that has never been touched — same doctrine as
    ///        ResetPrintTab: rebuilt, never reset field by field.
    void ResetPlotterTab();
    void ResetProjectorTab();

    /// @brief Tout ce que l'onglet en cours décrit, déjà nommé.
    ///
    /// Une entrée pour une taille unique — le cas de toujours sur une table sans axes — et **le
    /// produit cartésien des axes cochés** pour « plusieurs tailles ». Les noms sont réservés au fur
    /// et à mesure contre ceux déjà pris, y compris contre les entrées de ce même appel : sans ça,
    /// six sorties nées d'un coup porteraient six fois le même nom.
    auto DescribedOutputs() const -> QVector<OExportQueueItem>;

    /// @brief File what the Print tab describes in the list to generate. Nests nothing.
    void OnAddToList();

    /// @brief Run the Cutting plan tab — once the guard has let it through.
    ///
    /// The guard is the reason this is not a plain AcceptRole button: a cutting plan refuses to
    /// start on a piece whose direction is unknown, and it has to say so before anything runs.
    void OnGenerateCutting();

    /// @brief Walk the list, one run at a time, and let each line say how it went.
    void OnGenerateAll();

    /// @brief Say, left of the buttons, what was just filed. Cleared as soon as the tab is
    ///        left or a field is touched — it reports one add, not a state.
    void ShowAddedMessage(const QString &name);

    /// @brief Put the buttons the current tab asks for in the bar.
    void RebuildButtonBox();

    /// @brief The tab changed: its buttons come with it, and the last confirmation goes.
    void OnTabChanged();

    /// @brief The tab's form changed: what was added is no longer the last thing done, and the
    ///        buttons it feeds may no longer describe a run that can be asked for. One slot for
    ///        both forms — the Print tab and the Cutting plan tab ask the same question of it.
    void OnFormChanged();

    /// @brief Lock the dialog while the batch walks it.
    void SetBatchRunning(bool running);

    void RefreshGenerateButtons();
    void Retranslate();

    /// @brief Which of the three lists a favorite's recipe belongs to.
    ///
    /// Lu sur le FORMAT de la recette — et c'est la seule réponse à la question, puisque c'est
    /// aussi ce sur quoi le générateur bascule. Un type stocké à côté en serait une seconde.
    enum class FavoriteKind
    {
        Cutting,
        Print,
        Plotter,
        Projector
    };

    static auto KindOf(const OExportRunRequest &request) -> FavoriteKind;

    /// @brief Show @p favorites, each in the list its recipe belongs to, every box ticked.
    void PopulateFavorites(const QVector<OExportFavorite> &favorites);

    /// @brief The list that shows @p kind.
    auto ViewFor(FavoriteKind kind) const -> OExportFavoritesWidget *;

    /// @brief The ticked favorites of @p kind — what the batch will run of it.
    auto CheckedFavorites(FavoriteKind kind) const -> QVector<OExportFavorite>;

    /// @brief Drop the favorite @p name: the store through the remover, then the row, then what
    ///        the button bar allows.
    void OnFavoriteRemoveRequested(const QString &name, OExportFavoritesWidget *from);

    /// @brief One thing the batch will do, and where its line lives.
    ///
    /// Two sources, one walk. The runs of the Working list carry their own per-piece choices; the
    /// favorites do not (see OExportFavorite), so the pre-flight's answers are written into their
    /// request at the last moment. The generator is handed a request either way, so what differs
    /// is only which view shows the line's state.
    struct BatchJob
    {
        enum class Source
        {
            List,    ///< a line of the Working list, at @p index
            Favorite ///< a stored recipe, named @p name
        };

        Source source{Source::List};
        int index{0};
        QString name{};
        OExportRunRequest request{};
    };

    /// @brief Every run the batch would start, in the order it starts them — and **only for the
    ///        sub-tab in view**: the Working list when it is the one shown, otherwise the ticked
    ///        favorites of that one kind.
    ///
    /// The scope is read from `m_listTabs->currentIndex()` at call time, and it is the single source
    /// of both the batch and the button's enable state, so the two cannot drift apart.
    auto BatchJobs() const -> QVector<BatchJob>;

    /// @brief Write where @p job stands, in the view that shows it.
    void SetJobState(const BatchJob &job, OExportRunState state);

    /// @brief The label of the batch button, which is read from the INNER tab in view.
    ///
    /// The button bar is shared by the four sub-tabs of the list page, and the batch is now the one
    /// in view: on a favorites sub-tab, "the whole list" would name a scope the button no longer has.
    auto GenerateAllLabel() const -> QString;

    /// @brief Keep the dialog at least as wide as its tab bar.
    ///
    /// A QTabWidget whose tabs do not fit does not grow: it elides the labels, or grows
    /// scroll arrows beside them. Four tabs the user cannot read are four tabs she cannot
    /// choose between, so the width is imposed from the labels instead — and re-imposed
    /// whenever they change language.
    void AdjustMinimumWidth();

    // Kept to rebuild the Print tab with: the dialog is the only side that holds the three,
    // and a fresh form has to be built from the same material as the first one.
    const VContainer *m_data{nullptr};
    QSharedPointer<VMeasurements> m_measurements{};
    OGradedSizeRanges::Context m_context{};

    OExportPrintWidget *m_printWidget{nullptr};
    OExportCuttingWidget *m_cuttingWidget{nullptr};
    OExportPlotterWidget *m_plotterWidget{nullptr};
    OExportProjectorWidget *m_projectorWidget{nullptr};
    OExportGenerationListWidget *m_listWidget{nullptr};
    QTabWidget *m_tabs{nullptr};
    QVBoxLayout *m_printLayout{nullptr};
    QVBoxLayout *m_cuttingLayout{nullptr};
    QVBoxLayout *m_plotterLayout{nullptr};
    QVBoxLayout *m_projectorLayout{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    QLabel *m_addedLabel{nullptr};

    // The bar is rebuilt per tab, so these come and go. Every use is guarded: only the
    // Print tab's two and the list's own are ever non-null together.
    //
    // Close and Cancel are not kept: nothing sets them apart from the other buttons of the
    // bar — they are not enabled or disabled on their own, and reject() guards itself.
    QPushButton *m_generateButton{nullptr};
    QPushButton *m_addToListButton{nullptr};
    QPushButton *m_generateAllButton{nullptr};

    QWidget *m_cuttingTab{nullptr};
    QWidget *m_printTab{nullptr};
    QWidget *m_plotterTab{nullptr};
    QWidget *m_projectorTab{nullptr};

    /// La page 0 : un conteneur, et non plus la liste elle-même — quatre sous-onglets y vivent.
    /// Le dispatch des boutons ne bouge pas : il lit toujours l'onglet EXTERNE (m_tabs).
    QWidget *m_listPage{nullptr};
    QTabWidget *m_listTabs{nullptr};

    /// Les trois listes de favoris : la même classe, instanciée trois fois, chacune montrant les
    /// favoris d'un type. Le type n'est pas un réglage du widget — c'est la sous-onglet qui le dit.
    OExportFavoritesWidget *m_cuttingFavorites{nullptr};
    OExportFavoritesWidget *m_printFavorites{nullptr};
    OExportFavoritesWidget *m_plotterFavorites{nullptr};
    OExportFavoritesWidget *m_projectorFavorites{nullptr};

    /// Les favoris du contexte, tels que le MainWindow les a filtrés — RECETTES COMPRISES : le lot
    /// les rejoue, et le dialogue ne lit pas le store (il ne sait pas résoudre le contexte qui le
    /// indexe). Le retrait d'un favori passe par le remover, pas par une écriture d'ici.
    QVector<OExportFavorite> m_favorites{};
    FavoriteRemover m_favoriteRemover{};

    OExportRunQueue m_queue{};
    QStringList m_takenNames{};
    QString m_addedName{};
    RunGenerator m_generator{};
    bool m_batchRunning{false};
    bool m_generated{false};
};

#endif // OEXPORTMODEDIALOG_H
