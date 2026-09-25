/************************************************************************
 **
 **  @file   owidgetoutputs.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Dock "Outputs" content (Export mode): a compact list of the
 **          generated outputs — one row each, an "Add an output" button and an
 **          "Export several outputs" button on top, and one icon action per row
 **          (favorite/refresh/export/delete). An "Edit" column was drawn and never
 **          wired; it was removed on 24-09-2026.
 **          The output shown in the canvas is marked by a filled check-circle.
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

#ifndef OWIDGETOUTPUTS_H
#define OWIDGETOUTPUTS_H

#include <QEvent>
#include <QIcon>
#include <QList>
#include <QString>
#include <QVector>
#include <QWidget>

class QPushButton;
class QTableWidget;

/**
 * @brief One row of the Outputs dock: what it takes to draw a generation.
 *
 * The dock never sees the geometry, and never sees the store either — it is handed the
 * rows that are to be listed, already filtered to the current (table, axis) context by
 * MainWindow, which is the only owner of that rule.
 */
struct OExportOutputRow
{
    quint32 id{0};   ///< stable identity — the row index is not one
    QString name{};  ///< what the user named it; unique among the rows of the same context
    bool favorite{false};    ///< a favorite of the current context already carries this name
    bool canFavorite{true};  ///< false when the pattern has no measurements table to key on

    /// Whether this generation was asked for as a cutting plan. A cutting plan has **no export**
    /// today — HPGL/PLT comes later — so its export cell is drawn disabled. The dock is told the
    /// fact and draws it; it is not the dock's to know what a format is (spec CuttingPlan §8).
    bool cuttingPlan{false};
};

/**
 * @brief The OWidgetOutputs class is the content of the Export-mode "Outputs"
 *        dock. It lists the generated outputs (active dot + name on the left,
 *        action icons on the right) grouped under the groups the user files
 *        them into. Like the Size Ranges dock it never touches the pattern: it
 *        will act through signals once the behaviour lands.
 *
 *        Favorite (24-09-2026) is wired: the star adds the generation's recipe to the
 *        software-level favorites, or removes it. The "Edit" column was removed the same
 *        day: it was drawn, never wired, and never would be.
 *        See spec Modifications/Ostara/ExportMode/DockOutputs.md.
 */
class OWidgetOutputs final : public QWidget
{
    Q_OBJECT

public:
    /// @brief Retraduit le panneau au changement de langue.
    ///
    /// Doc Qt, « Prepare for Dynamic Language Changes » : `LanguageChange` parcourt tout l'arbre des
    /// widgets, et **chaque widget doit surcharger `changeEvent`** — la classe de base ne retraduit
    /// rien. Sans cette surcharge, ce panneau garde la langue de sa construction.
    void changeEvent(QEvent *event) override;

    explicit OWidgetOutputs(QWidget *parent = nullptr);

    /**
     * @brief Show @p outputs — already filtered to the current (table, axis) context by
     *        MainWindow. Does not re-pop: the caller follows with UpdateList().
     */
    void SetOutputs(const QVector<OExportOutputRow> &outputs);

    /** @brief Re-pop the list, marking @p activeOutputId (0 = none) as the one shown. */
    void UpdateList(quint32 activeOutputId);

    /** @brief Re-label the chrome (LanguageChange); re-pops the list (icons too). */
    void RetranslateUI();

signals:
    /** @brief The "Add an output" button was clicked. MainWindow opens the dialog and runs. */
    void addRequested();

    /**
     * @brief The "Export several outputs" button was clicked: open the batch dialog.
     *
     * Carries nothing: a batch is not about a row. Which generations it offers, and by which
     * categories, is MainWindow's to resolve — it owns the store and the current context.
     */
    void exportSeveralRequested();

    /**
     * @brief The user renamed a row inline. @p id is the stable generation identity
     *        (a row index is not). The dock has already rejected empty/conflicting names.
     */
    void renameRequested(quint32 id, const QString &newName);

    /**
     * @brief The status icon of @p id was clicked: that generation is the one to show.
     *        Showing it recomputes nothing — the row already carries its result.
     */
    void activeRequested(quint32 id);

    /** @brief The Refresh icon of @p id was clicked: re-run its nesting. */
    void refreshRequested(quint32 id);

    /** @brief The Delete icon of @p id was clicked. The confirmation belongs to MainWindow:
     *         what deleting means is not a property of the button. */
    void deleteRequested(quint32 id);

    /** @brief The Export icon of @p id was clicked: write its plan as a PDF. */
    void exportRequested(quint32 id);

    /**
     * @brief The favorite star of @p id was clicked: add that generation's recipe to the
     *        favorites of the current (table, axis) context, or take it out when it is
     *        already there. Which of the two it is belongs to MainWindow — it owns the
     *        store, and the widget never sees it.
     */
    void favoriteRequested(quint32 id);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void OnAddClicked();
    void OnExportSeveralClicked();
    void OnNameEdited(QTableWidgetItem *item);
    void OnCellClicked(int row, int column);

private:
    void BuildUi();
    void Populate();
    void UpdateButtonIcons();
    QPushButton *MakeIconButton(const QString &objectName, const QString &iconBase);
    void FlashNameError(QTableWidgetItem *item);

    /// @brief Whether the batch button has anything to offer — and if not, why not, said on the
    ///        button's tooltip rather than left to be guessed.
    void UpdateExportSeveralState();

    QTableWidget *m_table{nullptr};
    QPushButton *m_addOutputButton{nullptr};
    QPushButton *m_exportSeveralButton{nullptr};

    // Icon buttons: base name + the two variants, re-read on every theme change.
    // A button created inline in a lambda would not be reachable from the slot,
    // hence the list (§16 of SubtiliteCode/03-polices-icones.md).
    struct IconButton
    {
        QPushButton *button{nullptr};
        QString baseName;
        QIcon normal; // current-theme variant
        QIcon hover;  // always the dark variant — the hovered background is an accent
    };
    QList<IconButton> m_iconButtons;

    QVector<OExportOutputRow> m_outputs{};
    quint32 m_activeOutputId{0};

    /// @brief Set during programmatic cell rewrites (Populate, name commit) so the
    ///        itemChanged handler skips the validation/rename chain and just accepts
    ///        what was just written.
    bool m_filling{false};
};

#endif // OWIDGETOUTPUTS_H
