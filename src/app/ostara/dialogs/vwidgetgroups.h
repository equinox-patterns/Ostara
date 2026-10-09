/************************************************************************
 **
 **  @file   vwidgetgroups.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 4, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef VWIDGETGROUPS_H
#define VWIDGETGROUPS_H

#include "../vmisc/typedef.h"

#include <QIcon>
#include <QMap>
#include <QPair>
#include <QVector>
#include <QWidget>

class QPushButton;
class QCheckBox;
class QTableWidgetItem;
class VAbstractPattern;
class VContainer;
struct VGroupData;
struct VLayerGroupData;

namespace Ui
{
class VWidgetGroups;
}

class VWidgetGroups : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit VWidgetGroups(VContainer *pattern, VAbstractPattern *doc, QWidget *parent = nullptr);
    /// @brief Retraduit le panneau au changement de langue.
    ///
    /// Doc Qt, « Prepare for Dynamic Language Changes » : `LanguageChange` parcourt tout l'arbre des
    /// widgets, et **chaque widget doit surcharger `changeEvent`** — la classe de base ne retraduit
    /// rien. Sans cette surcharge, ce panneau garde la langue de sa construction.
    void changeEvent(QEvent *event) override;

    ~VWidgetGroups() override;

    void RetranslateUI();

public slots:
    void UpdateGroups();

    /// Tint the row of the active layer: the accent at low opacity, on the table's own stylesheet. A slot because
    /// the theme event reaches MainWindow, which re-applies it — the widget stylesheet is not rebuilt by itself.
    void UpdateActiveRowStyle();

    /// True while the canvas is picking objects for a layer — adding, removing or transferring them: every click
    /// toggles one, and Enter carries the direction out — for a transfer, by asking where the objects move to.
    auto IsSelectingObjects() const -> bool { return m_objectSelection != ObjectSelection::Idle; }
    /// Toggle a whole tool's objects (an (objectId, toolId) pair per object) in the current selection.
    void ToggleLayerObjects(const QVector<QPair<quint32, quint32>> &objects);
    /// Write the picked objects into the layer — or take them out of it — as one undoable step.
    void ValidateObjectSelection();
    void CancelObjectSelection();

signals:
    /// The canvas must start (or stop) routing its clicks to this dock.
    void ObjectSelectionStarted();
    void ObjectSelectionFinished();
    /// One object entered or left the layer's content: the canvas paints it magenta (or gives it back its own
    /// colour). The content is marked the same way in both directions — it is what the layer holds.
    void ObjectInLayerChanged(quint32 objectId, quint32 toolId, bool inLayer);
    /// One object entered or left the selection: the canvas paints it orange (or back to magenta). Orange is always
    /// what Enter is about to do — add the object, or take it out.
    void ObjectSelectedChanged(quint32 objectId, quint32 toolId, bool selected);
    void ShowStatusTip(const QString &text);
    /// The rule and the name over the canvas follow this switch. MainWindow persists it, because the
    /// setting is software-level and the chrome it drives is not this dock's.
    void ShowActiveLayerOnCanvasChanged(bool on);

private slots:
    void OnCellClicked(int row, int column);
    void RenameGroup(int row, int column);
    void CtxMenu(const QPoint &pos);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(VWidgetGroups) // NOLINT

    /// Picking the objects a layer should hold. The canvas does the clicking: MainWindow routes each click here,
    /// which is why this state lives with the dock and not in a dialog.
    enum class ObjectSelection
    {
        Idle,
        Adding,       ///< The layer takes objects it does not hold yet.
        Removing,     ///< The layer gives up objects another layer holds too.
        Transferring  ///< The layer gives up any of its own objects, straight into the layer chosen next.
    };

    /// What a row of the table stands for. A group row is the level above the layers: it wears the group's name,
    /// and the layers that follow it in the list are its content — everything down to the next group row.
    enum class RowKind : quint8
    {
        Layer,
        LayerGroup
    };

    Ui::VWidgetGroups *ui;
    VAbstractPattern *m_doc;
    VContainer *m_pattern;
    QPushButton *m_addButton;
    QPushButton *m_addGroupButton{nullptr};
    QCheckBox *m_showActiveLayer{nullptr};

    ObjectSelection m_objectSelection{ObjectSelection::Idle};
    quint32 m_selectionLayerId{NULL_ID};
    QMap<quint32, quint32> m_selectedObjects; // objectId -> toolId, the shape the pattern expects
    /// What the layer already holds. Adding skips it — a layer holds an object once — and a removal starts from
    /// it: what cannot leave is still part of the content.
    QMap<quint32, quint32> m_layerObjects;
    /// Of that content, the objects the current direction may act on. Adding has none — there the click itself
    /// decides, and everything the layer does not hold yet is fair game. A removal keeps what another layer holds
    /// too; a transfer keeps everything of the layer that can be drawn. Filled when the picking starts.
    QMap<quint32, quint32> m_candidateObjects;
    /// The objects wearing magenta on the canvas right now — the exact set this picking has to give back. For an
    /// addition it is the layer's content, for the two other directions the candidates, and nothing at all when
    /// the picking can lead nowhere. Remembered rather than recomputed: the colour must go back to exactly what
    /// it took, and a second computation is a second rule that can drift from the first.
    QMap<quint32, quint32> m_markedObjects;

    /// Where the list was scrolled to when the current row action began. A row action can rebuild the table — the
    /// layer dialog pushes its change through the document, and the rebuild scrolls the active layer (often the
    /// last of the list) into view. The position is therefore taken BEFORE the action and given back after it: a
    /// dialog opened from the list must not move the list (see SelectActiveRow).
    int m_scrollY{0};
    int m_scrollX{0};

    // Button icon: the current-theme variant at rest, always the dark variant on hover
    // (the hovered button background is an accent, dark in both themes).
    QIcon m_addNormalIcon;
    QIcon m_addHoverIcon;
    QIcon m_addGroupNormalIcon;
    QIcon m_addGroupHoverIcon;

    void BuildAddButtons();
    void UpdateAddButtonIcon();
    void UpdateAddGroupButtonIcon();
    void BuildShowActiveLayerCheckBox();

    void FillTable(QMap<quint32, VGroupData> groups);
    void FillLayerRow(int row, quint32 id, const VGroupData &data, bool dark);
    void FillLayerGroupRow(int row, quint32 id, const VLayerGroupData &group, const QVector<quint32> &layers,
                           bool anyVisible, bool dark);
    void SetGroupVisibility(vidtype id, bool visible) const;
    void SetMultipleGroupsVisibility(const QVector<vidtype> &groups, bool visible) const;

    /// The row kinds are what tell a group row from a layer row: a group row carries the group's id in the first
    /// column just like a layer row carries the layer's, and nothing else could tell them apart.
    auto KindOfRow(int row) const -> RowKind;
    /// The row of one layer, -1 when the layer is not listed — inside a folded group, or filtered out.
    auto GroupRow(vidtype id) const -> int;
    auto LayerGroupRow(vidtype groupId) const -> int;

    /// Every layer of the piece, id -> name, by name.
    auto LayerNames() const -> QMap<quint32, QString>;
    /// The layers a row of the group dialog may name: the ones no group holds, plus — when editing — the ones the
    /// group already holds, otherwise opening the dialog would empty it.
    auto CandidateLayers(quint32 groupId) const -> QMap<quint32, QString>;

    /// Turn the dialog's answer into a group that exists: the id the user picked, or a group created here when the
    /// answer was "a new one". The creation is pushed as its own command, so the caller opens a macro and one
    /// Ctrl+Z takes the group AND what joined it.
    auto MaterializeLayerGroup(quint32 choice, const QString &newName) -> quint32;

    void RefreshGroupEyes() const;
    /// Every layer of the group is shown. An empty group counts as shown: it has nothing to hide.
    auto LayerGroupFullyVisible(quint32 groupId) const -> bool;
    void SetLayerGroupVisibility(quint32 groupId, bool visible);

    void ActionAddLayer();
    void ActionEditLayer(quint32 id);
    void ActionDeleteLayer(quint32 id);
    void ActionAddLayerGroup();
    void ActionEditLayerGroup(quint32 id);
    void ActionDeleteLayerGroup(quint32 id);
    void ActionTransferLayerGroup(quint32 id);
    /// Fold or unfold one group: what the reader sees, written with the document so it is remembered.
    void ToggleLayerGroup(quint32 id);
    void ActionHideAll();
    void ActionShowAll();

    /// Arm the canvas picking for one layer, in one of the two directions. Cancels whatever was being picked.
    void StartObjectSelection(quint32 layerId, ObjectSelection mode);
    /// Why this picking can lead nowhere — the only layer of the piece, or a layer holding no object. Empty when
    /// the picking can be carried out. Such a picking is ARMED anyway (see StartObjectSelection): the sentence is
    /// said, and Enter says it again instead of writing anything.
    auto RefusalReason(quint32 layerId, ObjectSelection mode) -> QString;
    /// What one layer's entries really are: the tool still exists AND the object is one that tool produces.
    /// The two are not the same — re-editing an operation's options replaces its element, and an id that
    /// disappears leaves its `<item>` behind, naming an object nobody draws any more. A tool the model cannot
    /// enumerate (a pattern piece) keeps its entry, as the loading pass tolerates it. The ONE rule the dock
    /// reads a layer with: content, holders, and what `+`, `−` and `↗` may offer.
    auto RealItems(const VGroupData &layer) -> QVector<QPair<quint32, quint32>>;
    /// The objects a layer holds, in the (objectId, toolId) shape the pattern and the canvas both use.
    auto LayerItems(quint32 layerId) -> QVector<QPair<quint32, quint32>>;
    /// Of those, the ones another layer also holds. An object always belongs to at least one layer, so these are
    /// the only ones a removal may take out — the others would be left belonging to nothing.
    auto RemovableObjects(const QVector<QPair<quint32, quint32>> &items) -> QMap<quint32, quint32>;
    /// Everything of the layer that can be drawn, and so clicked. Wider than a removal on purpose: a transfer
    /// writes the object into another layer before taking it out of this one, so even an object this layer alone
    /// holds may move, and the invariant holds at every moment.
    auto TransferableObjects(const QVector<QPair<quint32, quint32>> &items) -> QMap<quint32, quint32>;
    /// Restate the workflow in the status bar: what is selected, what the colours mean, and what Enter will do.
    void UpdateSelectionTip();
    /// Say why a click did nothing: the object is not in the layer, or it is and nothing else holds it. A transfer
    /// only ever refuses the first — it moves an object without ever taking its last layer away.
    void ExplainRefusal(const QPair<quint32, quint32> &object);
    /// Say why an added click did nothing: the layer already holds it, so it is shown magenta and left alone.
    void ExplainAddRefusal();

    /// The layer the picked objects move to, asked in a dialog. NULL_ID when the user closed it — the selection
    /// is then left exactly as it was, so Enter can ask again.
    auto AskTargetLayer(quint32 sourceId) -> quint32;
    /// Move the picked objects out of one layer and into the chosen one. The target is written first, the source
    /// second, so no object is ever left belonging to nothing, and the whole move is one undoable step.
    void TransferSelectedObjects(quint32 sourceId);

    /// The layer the canvas draws into. A single click on the name is what sets it.
    void SetActiveLayer(quint32 id);
    /// Put the highlight back on the active layer — the row actions must not move it. @p keepScroll says the user is
    /// working in the list: the row comes back into the selection, not into view, so a long list stays where it is
    /// under their hands.
    void SelectActiveRow(bool keepScroll = false);
    void RememberScroll();
};

#endif // VWIDGETGROUPS_H
