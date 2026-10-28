/************************************************************************
 **
 **  @file   odialogdartcap.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  dialog of the cap of a dart
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

#ifndef ODIALOGDARTCAP_H
#define ODIALOGDARTCAP_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../../tools/drawTools/operation/opleatdisplay.h"
#include "dialogoperationtool.h"

#include "dialogtool.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QWidget;

namespace Ui
{
class ODialogDartCap;
}

/**
 * @brief The dialog of the cap of a dart: four clicks — the point of the dart, the two points of its
 * opening, then a point of the line of the pattern to extend — and one window with the cap's two tracés.
 *
 * The four points are shown nowhere: they are the ones clicked in the pattern, and the window only asks what
 * is left to decide — whether the cap is drawn, the names of its two tracés, and the pen they share.
 */
class ODialogDartCap final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogDartCap(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                            QWidget *parent = nullptr);
    ~ODialogDartCap() override;

    auto GetApexId() const -> quint32;
    void SetApexId(quint32 value);

    auto GetFirstPointId() const -> quint32;
    void SetFirstPointId(quint32 value);

    auto GetSecondPointId() const -> quint32;
    void SetSecondPointId(quint32 value);

    auto GetExtendPointId() const -> quint32;
    void SetExtendPointId(quint32 value);

    auto GetDrawCap() const -> bool;

    /** @brief The figure this dialog asks for, as the operation writes and draws it.
     *
     *  The two sides and the crank are the tool's to build — the ids are allocated where the objects are made — so
     *  what the dialog settles here is the look of the figure and whether it is drawn at all: the pen and the colour
     *  of the sides, and the flag. It is the same OPleatDisplay the pleat's dialog hands over, so the two
     *  operations are written and read the same way. */
    auto GetDisplay() const -> OPleatDisplay;

    /** @brief Load a figure into the dialog, so that an EDIT starts from what the tool holds.
     *
     *  The counterpart of GetDisplay: without it the dialog could only ever give a figure, never receive one, and
     *  editing a cap would reset its pen and its colour. The pleat's dialog has the same pair. */
    void SetDisplay(const OPleatDisplay &display);

    /** @brief The widgets an operation dialog shares with the tool it writes: the status line, the notes, and the
     *  layer group. DialogOperationTool reads them from here — and only from here. */
    auto Widgets() const -> OperationWidgets override;
    void SetDrawCap(bool value);

    //! Whether the cap follows the edge read at the second point of the opening: the sense of the cap. The two
    //! senses give two different caps on the same three points, so it travels in the file, like the pleat's.
    auto GetSecondSense() const -> bool;
    void SetSecondSense(bool value);

    //! Whether the crank is asked for: the cap point is where the fold line touches the edge, and the crank is
    //! what cuts the edge there, in two halves a piece can be built through.
    auto GetNotchDrawn() const -> bool;
    void SetNotchDrawn(bool value);

    auto GetTypeLine() const -> QString;
    void SetTypeLine(const QString &value);

    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &value);

    auto GetPointName() const -> QString;
    void SetPointName(const QString &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    //! Whether the name of the created point is shown on the canvas. The checkbox of the Point tab says it,
    //! and this is the only way to put it back when the tool is edited.
    void SetPointLabelVisible(bool visible);

    void CheckDependencyTreeComplete() override;

    void ShowDialog(bool click) override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void ShowVisualization() override;

    /** @brief SaveData Put dialog data in local variables */
    void SaveData() override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    //! Honour an incomplete pattern graph: nothing is edited while the fields have no points to work with.
    void UpdateCapControls();

private:
    Q_DISABLE_COPY_MOVE(ODialogDartCap) // NOLINT

    /** @brief The step of the creation: the four points of the dart cap, one after the other, and then the
     *  window. */
    enum class Step : qint8
    {
        Apex,
        FirstPoint,
        SecondPoint,
        ExtendPoint,
        Complete
    };

    Ui::ODialogDartCap *ui;

    /** @brief The four points, held in combos of their own and never shown: validating a picked object is the
     *  base class's job and it reads a combo. None of these names ever reaches the screen. */
    QComboBox *m_comboApex{nullptr};
    QComboBox *m_comboFirstPoint{nullptr};
    QComboBox *m_comboSecondPoint{nullptr};
    QComboBox *m_comboExtendPoint{nullptr};

    QCheckBox *m_showLabelCheckbox{nullptr};

    Step m_step{Step::Apex};

    /** @brief True while the release that has just picked a point still has to be swallowed: the same release
     *  ends the pick and would otherwise be read as the click that ends the next step. */
    bool m_swallowRelease{false};

    quint32 m_apexId{NULL_ID};
    quint32 m_firstPointId{NULL_ID};
    quint32 m_secondPointId{NULL_ID};
    quint32 m_extendPointId{NULL_ID};
    bool m_secondSense{false};
    bool m_notchDrawn{false};

    QString pointName;

    bool flagName{true};
    bool flagPoint{true};

    /** @brief The four points are known and tell each other apart — what a dart cap is built on. */
    auto PointsAreDistinct() const -> bool;

    /** @brief Whether the construction of the cap exists with these points: the rule, asked before the window
     *  opens, so that no cap is ever created from a construction that does not exist. The line is the edge of
     *  the piece, read at the end the sense designates, and the point the user may designate is the way out. */
    auto ConstructionExists() const -> bool;

    //! The direction of the edge of the piece at the end of the opening the sense designates, or nothing.
    auto EdgeDirection() const -> QPointF;

    //! Whether that edge is enough to build the cap: the rule's answer, asked on the read side alone.
    auto EdgeFound() const -> bool;

    void TakeApex(quint32 id);
    void TakeFirstPoint(quint32 id);
    void TakeSecondPoint(quint32 id);
    void TakeExtendPoint(quint32 id);

    /** @brief What the status line asks, at the step the dialog is on.
     *
     * The prompt of the step that BEGINS, and not the one of the object that has just been taken: a window that
     * pushes its message when the object lands leaves the line describing the click that is already done — at the
     * second point of the opening it still asked for the tip of the dart. One source for the four steps, so that no
     * call site can name a step that is not the one to answer, and a translated string that already exists. */
    auto StepPrompt() const -> QString;

    /** @brief Put the prompt of the step the dialog is on, and no other, on the status line. */
    void ShowStepPrompt();

    /** @brief Say why a click cannot answer the step, and leave the user on that same step.
     *
     * The status line alone is too discreet for a click that does nothing: the user does not read it and
     * believes the tool is stuck. One button, and the step is still the one to answer. */
    void Refuse(const QString &message);

    /** @brief Show the dart and the cap as far as they are known. */
    void RefreshPreview();
};

#endif // ODIALOGDARTCAP_H
