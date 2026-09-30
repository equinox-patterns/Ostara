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
    /// Take away the sentence a refusal left in the status bar. A refusal arms nothing, so Escape has no picking to
    /// drop — but it does leave that sentence standing, and Escape is the gesture one reaches for to remove it.
    /// Only ever clears the dock's OWN sentence: a message another dock put in the same label is left alone.
    void ClearStatusTip();

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

    Ui::VWidgetGroups *ui;
    VAbstractPattern *m_doc;
    VContainer *m_pattern;
    QPushButton *m_addButton;
    QCheckBox *m_showActiveLayer{nullptr};

    ObjectSelection m_objectSelection{ObjectSelection::Idle};
    quint32 m_selectionLayerId{NULL_ID};
    QMap<quint32, quint32> m_selectedObjects; // objectId -> toolId, the shape the pattern expects
    /// What the layer already holds, marked magenta in both directions. Adding skips it — a layer holds an object
    /// once — and removing starts from it: what cannot leave is still part of the content.
    QMap<quint32, quint32> m_layerObjects;
    /// Of that content, the objects the current direction may act on. Adding has none — there the click itself
    /// decides, and everything the layer does not hold yet is fair game. A removal keeps what another layer holds
    /// too; a transfer keeps everything of the layer that can be drawn. Filled when the picking starts.
    QMap<quint32, quint32> m_candidateObjects;

    /// Set by a refusal, whose sentence is the only trace it leaves: nothing is armed, so nothing else would ever
    /// take that sentence away. Cleared as soon as the dock drops it, so Escape can never wipe a message another
    /// dock put in the same status label.
    bool m_tipToClear{false};

    // Button icon: the current-theme variant at rest, always the dark variant on hover
    // (the hovered button background is an accent, dark in both themes).
    QIcon m_addNormalIcon;
    QIcon m_addHoverIcon;

    void BuildAddButton();
    void UpdateAddButtonIcon();
    void BuildShowActiveLayerCheckBox();

    void FillTable(QMap<quint32, VGroupData> groups);
    void SetGroupVisibility(vidtype id, bool visible) const;
    void SetMultipleGroupsVisibility(const QVector<vidtype> &groups, bool visible) const;

    auto FilterGroups(const QMap<quint32, VGroupData> &groups) -> QMap<quint32, VGroupData>;

    auto GroupRow(vidtype id) const -> int;

    void ActionAddLayer();
    void ActionEditLayer(quint32 id);
    void ActionDeleteLayer(quint32 id);
    void ActionHideAll();
    void ActionShowAll();

    /// Arm the canvas picking for one layer, in one of the two directions. Cancels whatever was being picked.
    void StartObjectSelection(quint32 layerId, ObjectSelection mode);
    /// Refuse to arm a picking, saying why in the status bar. Nothing is armed afterwards: the sentence is all
    /// there is, and m_tipToClear is what remembers it is the dock's own.
    void RefuseSelection(const QString &reason);
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
    /// Put the highlight back on the active layer — the row actions must not move it.
    void SelectActiveRow();
};

#endif // VWIDGETGROUPS_H
