/************************************************************************
 **
 **  @file   odialogpleat.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The dialog of a pleat: the departure line, the objects that travel, the value, and what is drawn.
 **
 **          The objects that travel are picked first, as a slash-and-spread asks for them, then Enter, then the two
 **          points of the departure line — the first green, the second red — and the dialog opens. Everything the
 **          pleat needs is derived from those three things, so the dialog only asks for what the rule cannot decide:
 **          the value, whether each end is capped and which way, whether it is a double pleat, and the pens.
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
class VAbstractPattern;
class VContainer;

namespace Ui
{
class DialogPleat;
}

class ODialogPleat final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogPleat(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~ODialogPleat() override;

    auto GetFirst() const -> OPleatEnd;
    auto GetSecond() const -> OPleatEnd;
    auto GetValue() const -> QString;
    auto GetKind() const -> OPleatKind;
    auto IsFoldLineShown() const -> bool;
    auto GetDisplay() const -> OPleatDisplay;

    void SetPleat(const OPleatEnd &first, const OPleatEnd &second, const QString &value, OPleatKind kind,
                  bool foldLine);
    void SetDisplay(const OPleatDisplay &display);

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

private:
    Q_DISABLE_COPY_MOVE(ODialogPleat) // NOLINT

    Ui::DialogPleat *ui;

    FormulaInputWidget *m_formulaValue{nullptr};
    QComboBox *m_lineType{nullptr};
    QComboBox *m_lineColor{nullptr};
    QComboBox *m_foldType{nullptr};
    QComboBox *m_foldColor{nullptr};

    /** @brief The kind of the pleat the user picked: one fold, or two side by side. Built in code, like every other
     *  combo of the house (see MakeComboBox). */
    QComboBox *m_kind{nullptr};

    /** @brief Where the gesture stands: the objects that travel first, as a slash-and-spread asks for them, then the
     *  two points of the pleat's departure line, then the edge of an end nothing could read, then the dialog. */
    enum class Step
    {
        Selecting, // the objects that travel are being picked; Enter ends the selection
        Points,    // the next two points clicked are the departure line: green first, red second
        Edge,      // the next point clicked carries the edge of one end of the departure line
        Editing    // the dialog is up
    };

    Step m_step{Step::Selecting};
    int m_pointRank{0};
    int m_edgeRank{0};

    OPleatEnd m_first{};
    OPleatEnd m_second{};

    OPleatDisplay m_display{};

    /** The kind the travelling objects were last named for, and whether they were named at all: a pleat being
     *  edited has been named already, a pleat being built has not. */
    bool m_sourcesNamed{false};
    OPleatKind m_namedKind{OPleatKind::Simple};

    /** @brief The two ends as the widgets describe them: the geometry comes from the departure line, the options from
     *  the boxes, so there is one place a value is read from. */
    auto ShownEnd(int rank) const -> OPleatEnd;

    /** @brief What the status bar asks for while the departure line is being picked: the point that is missing, said
     *  by the colour it will take — point 1 green, point 2 red. */
    auto PointPrompt() const -> QString;

    /** @brief Say WHICH cap is which by its colour, never by a point name: Ostara is worked without the objects' names
     *  on screen, so the green dot and the red dot are what tell the two ends of the departure line apart. */
    void RefreshCapLabels();

    /** @brief The value of the pleat in pixels, or 0 while the field holds nothing that can be read. */
    auto PleatValue() const -> qreal;

    auto MakePlan() const -> OPleatPlan;

    auto FailureText(const OPleatPlan &plan) const -> QString;

    /** @brief Ask for the edge of the first end that has a cap and none to follow, or open the dialog when every
     *  cap has one. */
    void AskForEdge();

    /** @brief Name every object that travels after the copy it will become: <name>__m1 for a simple pleat, __m2 for
     *  a double one — the generation the manual construction gave them, so the two are told apart on the canvas.
     *
     *  A copy left without a name is called X, and two of them would share it: the name is what a modéliste reads on
     *  the pattern, and the pattern's own names must stay unique. The check is the container's, and the wrapper's own
     *  copies are left out of it — @a force is what the first naming does, and a change of kind what redoes it. */
    void NameSources(bool force);

    void GoToEditing();
    void RefreshFigure();
    void RefreshFoldLineState();
    void ShowError(const QString &message) const;
};

#endif // ODIALOGPLEAT_H
