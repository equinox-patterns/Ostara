/************************************************************************
 **
 **  @file   odialogpleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The dialog of the pleats: the value and the boxes of each of them, and the figure.
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

#ifndef ODIALOGPLEAT_H
#define ODIALOGPLEAT_H

#include <QComboBox>
#include <QDialog>
#include <QVector>
#include <QtGlobal>

#include "../../tools/drawTools/operation/opleatdisplay.h"
#include "../../tools/drawTools/operation/opleatrule.h"
#include "dialogoperationtool.h"

class FormulaInputWidget;
class QPushButton;
class QTableWidget;
class VAbstractPattern;
class VContainer;

namespace Ui
{
class DialogPleat;
}

/**
 * @brief The dialog of the pleats: one line per departure line, and the editor of the line the user picked.
 *
 * The gesture picks the departure lines on the canvas, one after the other — as a slash-and-spread picks its cuts —
 * and the dialog opens once they are all there. It is then the table that says which pleat is being edited: the
 * value, the kind and the two caps below belong to the line the table has selected, and the pens are the ones of the
 * whole figure. A line the user adds or takes away changes the objects of the operation, and only a full parse can
 * follow that: it is asked for, in its own group of undo.
 */
class ODialogPleat final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogPleat(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~ODialogPleat() override;

    /** @brief The departure lines as the editor holds them, the line it is editing written back into its row. */
    auto GetDepartures() const -> QVector<OPleatDeparture>;

    auto IsFoldLineShown() const -> bool;
    auto GetDisplay() const -> OPleatDisplay;

    void SetPleat(const QVector<OPleatDeparture> &departures, bool foldLine);

    /** @brief Which of the two tools of the family this window is: the pleat taken on its first arm, or the one laid
     *  at the centre of a piece (see OPleatOrigin).
     *
     *  A pleat given by its middle is a **single** one — one line defines one middle — so the table of the lines and
     *  its two buttons have nothing to say and are not shown, and the gesture opens the window as soon as the line is
     *  laid instead of asking for the next one. Set once, when the window is built for the gesture; a window opened
     *  on an existing pleat is told by the tool itself (see OToolPleat::SetDialog). */
    void SetOrigin(OPleatOrigin origin);
    auto Origin() const -> OPleatOrigin { return m_origin; }
    void SetDisplay(const OPleatDisplay &display);

    /** @brief The objects of the figure another tool is built on (see OToolPleat::FigureObjectsInUse): the boxes that
     *  carry one of them are held, so that un-ticking them cannot take the object out of the pattern. */
    void SetFigureObjectsInUse(const QSet<quint32> &ids);

    void ShowDialog(bool click) override;
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void ShowVisualization() override;
    void SaveData() override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;
    void OnSourceObjectsSet() override;
    auto Widgets() const -> OperationWidgets override;

private slots:
    void ValueChanged(bool valid);
    void FigureOptionChanged();
    void DepartureSelected();
    void AddDeparture();
    void RemoveDeparture();

private:
    Q_DISABLE_COPY_MOVE(ODialogPleat) // NOLINT

    Ui::DialogPleat *ui;

    FormulaInputWidget *m_formulaValue{nullptr};
    QComboBox *m_lineType{nullptr};
    QComboBox *m_lineColor{nullptr};
    QComboBox *m_foldType{nullptr};
    QComboBox *m_foldColor{nullptr};

    /** @brief The kind of the pleat the editor is showing: one fold, or two side by side. Built in code, like every
     *  other combo of the house (see MakeComboBox). */
    QComboBox *m_kind{nullptr};

    /** @brief The departure lines, one row each, and the two buttons that add one or take one away. */
    QTableWidget *m_table{nullptr};
    QPushButton *m_add{nullptr};
    QPushButton *m_remove{nullptr};

    /** @brief Where the gesture stands: the objects that travel first, as a slash-and-spread asks for them, then the
     *  departure lines one after the other, then the edge of an end nothing could read, then the dialog. */
    enum class Step
    {
        Selecting, // the objects that travel are being picked; Enter ends the selection
        Points,    // the next two points clicked are a departure line: green first, red second
        Edge,      // the next point clicked carries the edge of one end of a departure line
        Editing    // the dialog is up
    };

    Step m_step{Step::Selecting};
    int m_pointRank{0};
    int m_edgeDeparture{0};
    int m_edgeRank{0};

    QVector<OPleatDeparture> m_departures{};

    /** Where the line the user draws sits among the places of its pleat: the tool's own nature (see SetOrigin). */
    OPleatOrigin m_origin{OPleatOrigin::FirstArm};

    /** The departure lines the canvas shows: the ones the tool holds, or, for a pleat not yet built, the ones the
     *  gesture has just laid. What the editor holds is NOT it — a value, a kind, a cap or a notch changed in the
     *  window waits for Apply, and the canvas must not move before it (see RefreshFigure). */
    QVector<OPleatDeparture> m_applied{};

    /** The first point of the departure line being picked, when there is one: the second click completes it. */
    OPleatEnd m_pending{};

    /** The row the editor is showing, -1 when there is none: what the value, the kind and the caps below belong to. */
    int m_editorRow{-1};

    /** Whether the dialog stepped aside so a line could be picked on the canvas: it comes back on the line just
     *  added, where the gesture that built the others left the user. */
    bool m_addingDeparture{false};

    OPleatDisplay m_display{};

    /** The figure objects with dependents, as the tool answers it when the dialog is linked: what no box may untick. */
    QSet<quint32> m_figureInUse{};

    /** How many pleats the travelling objects were last named for, and whether they were named at all: a pleat being
     *  edited has been named already, a pleat being built has not. */
    bool m_sourcesNamed{false};
    int m_namedPleats{0};

    /** @brief True when the line the user drew is the **middle** of its pleat (see SetOrigin). */
    auto Middle() const -> bool { return m_origin == OPleatOrigin::Middle; }

    /** @brief How many pleats the departure lines make: one per simple line, two per double one. */
    auto Pleats() const -> int;

    /** @brief The kind the combo of the editor is showing. */
    auto Kind() const -> OPleatKind;

    /** @brief The departure lines as the editor holds them: the rows, with the line being edited written back. */
    auto ShownDepartures() const -> QVector<OPleatDeparture>;

    /** @brief What the line of the departure being picked is missing, said by the colour it will take — point 1
     *  green, point 2 red — and, once a line exists, that Enter closes the series. */
    auto PointPrompt() const -> QString;

    /** @brief Say WHICH cap is which by its colour, never by a point name: Ostara is worked without the objects'
     *  names on screen, so the green dot and the red dot are what tell the two ends of a departure line apart. */
    void RefreshCapLabels();

    /** @brief The values of the departure lines in pixels. @a nominal stands in for the ones that hold nothing yet:
     *  the caps are judged while the gesture runs, and a cap is a direction, which no value can change. */
    auto Values(const QVector<OPleatDeparture> &departures, bool nominal) const -> QVector<qreal>;

    /** @brief The value of one formula in pixels, or 0 while it holds nothing that can be read. */
    static auto ValueInPixels(const QString &formula, const VContainer *data) -> qreal;

    auto MakePlan(bool nominal) const -> OPleatPlan;

    auto FailureText(const OPleatPlan &plan) const -> QString;

    /** @brief How many pleats the departure lines before @a departure make: the pleat index of the first fold of one
     *  line is the number of folds every line before it asked for. */
    auto PleatsBefore(int departure) const -> int;

    /** @brief True when one of the objects the box of @a end of pleat @a pleat carries is built on by another tool —
     *  the point, the two sides of a cap, the two halves of a cut edge. */
    auto BoxInUse(int pleat, int end, bool notch) const -> bool;

    /** @brief Ask for the edge of the first end that has a cap and none to follow.
     *
     *  @return true when every cap of every pleat has an edge to follow, false when the status bar is now asking for
     *  one: the caller decides what to do with an operation whose caps can all be built — the gesture goes on with
     *  another departure line, and Enter is what opens the dialog. */
    auto AskForEdge() -> bool;

    /** @brief The status bar's question while an edge is missing: which cap, said by its colour, and what to click. */
    auto EdgePrompt(int rank) const -> QString;

    /** @brief Name every object that travels after the copy it will become: <name>__m1 for one pleat, __m2 for two,
     *  and so on — the generation the manual construction gave them, so the two are told apart on the canvas.
     *
     *  A copy left without a name is called X, and two of them would share it: the name is what a modéliste reads on
     *  the pattern, and the pattern's own names must stay unique. The check is the container's, and the wrapper's own
     *  copies are left out of it — @a force is what the first naming does, and a new count of pleats what redoes it. */
    void NameSources(bool force);

    /** @brief Write the table and the editor from the model, and put the editor on one row. */
    void FillDepartures();

    void RefreshRow(int row);
    void ShowDepartureDetails(int row);
    void EnableDetails(bool enabled);

    auto CurrentRow() const -> int;

    void GoToEditing(int row);
    void RefreshFigure();
    void RefreshFoldLineState();

    /** @brief Grey the notch of an end whose cap is off: a notch marks the edge the cap follows, and an end that
     *  follows none has no edge to mark. The box keeps its state, so ticking the cap back brings the notch back. */
    void RefreshNotchState();

    void ShowError(const QString &message) const;
};

#endif // ODIALOGPLEAT_H
