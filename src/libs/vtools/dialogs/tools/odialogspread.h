/************************************************************************
 **
 **  @file   odialogspread.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The dialog of the slash-and-spread tool.
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

#ifndef ODIALOGSPREAD_H
#define ODIALOGSPREAD_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../tools/drawTools/operation/ospreaddisplay.h"
#include "../../tools/drawTools/operation/ospreadrule.h"
#include "dialogoperationtool.h"

namespace Ui
{
class DialogSpread;
}

class AngleFormulaWidget;
class FormulaInputWidget;
class QComboBox;
class OVisToolSpread;

class ODialogSpread final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogSpread(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~ODialogSpread() override;

    /** @brief The rotations, with the name and angle the user last typed. */
    auto GetRotations() const -> QVector<OSpreadRotation>;
    void SetRotations(const QVector<OSpreadRotation> &rotations);

    /** @brief The visual representation of the rotations, as the editor holds it: whether it is drawn, and the
     *  pen of its lines. The objects themselves belong to the operation, which keeps their ids: the editor has
     *  no business with them. */
    auto GetDisplay() const -> OSpreadDisplay;
    void SetDisplay(const OSpreadDisplay &display);

    /** @brief Whether the copies are smoothed at the pivots. Off, the operation makes the rigid image it has
     *  always made. */
    auto IsSmooth() const -> bool;
    void SetSmooth(bool smooth);

    /** @brief What the copies of curves, arcs and lines are named after: their source's name and this suffix,
     *  "_Rotation" by default. Points are not concerned — they keep the "__R" of the operation. */
    auto CopySuffixName() const -> QString;
    void SetCopySuffixName(const QString &suffix);

    void ShowDialog(bool click) override;
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void showEvent(QShowEvent *event) override;
    void ShowVisualization() override;
    void SaveData() override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;
    void OnSourceObjectsSet() override;
    auto Widgets() const -> OperationWidgets override;

private slots:
    void ShowRotationDetails();
    void UpdateSmoothingSummary();
    void KeepOnScreen();
    void SaveRotationName(const QString &text);
    void AngleChanged(bool valid);
    void LengthChanged(bool valid);

private:
    Q_DISABLE_COPY_MOVE(ODialogSpread) // NOLINT

    /** @brief True once the dialogue has been given its size: a resize by hand is then left alone. */
    bool m_sized{false};

    Ui::DialogSpread *ui;

    AngleFormulaWidget *m_formulaAngle{nullptr};
    FormulaInputWidget *m_formulaLength{nullptr};

    /** @brief The pen of the display's lines, built here rather than in the .ui: a pen and a colour combo are
     *  built by MakeComboBox, which is what gives them the application's own popup view. */
    QComboBox *m_displayTypeLine{nullptr};
    QComboBox *m_displayColor{nullptr};

    /** @brief Where the gesture stands. Objects are picked first, then one pair (start, pivot) per rotation,
     *  then the dialog opens to edit the angles. */
    enum class Step
    {
        Selecting, // objects are being picked; Enter ends the selection
        Start,     // the next point clicked is the start of the current rotation
        Pivot,     // the next point clicked is the pivot of the current rotation
        Editing    // the dialog is up
    };

    Step m_step{Step::Selecting};
    int m_currentRotation{0};

    QVector<OSpreadRotation> m_rotations{};

    /** @brief The rotation the editor is currently showing, -1 when none. The formula fields evaluate on a
     *  timer, so an edit still pending when the selection moves must be written back to THIS row, not to the
     *  one that has just been selected. */
    int m_editorRow{-1};

    bool m_derivedLengthSet{false};
    qreal m_derivedLength{0.};
    bool m_lengthIsPossible{true};

    /** @brief The rotation whose details are shown in the editor, -1 when none. */
    auto CurrentRow() const -> int;

    /** @brief The preview of this dialog, told whether the spread is already in the pattern — that is, whether the
     *  pattern draws the objects it spread and the preview keeps the images only (see OVisToolSpread::SetInPattern).
     *  Null when the dialog has no preview to configure. */
    auto ConfiguredVis() -> OVisToolSpread *;

    /** @brief Distance pivot -> start of one rotation, in pixels; 0 when the pair is incomplete. */
    auto RadiusFor(const OSpreadRotation &rotation) const -> qreal;

    /** @brief The length one rotation means, as its row shows it; an em dash when the pair gives no radius. */
    auto LengthTextFor(const OSpreadRotation &rotation) -> QString;

    /** @brief Rewrite one row of the table (name, angle, length) from the model. */
    void RefreshRow(int row);

    void FillRotations();
    void EnableDetails(bool enabled);

    void UpdateLengthFromAngle();
    void UpdateAngleFromLength();
    void CommitCurrentAngle();

    /** @brief Write what the two editor fields hold into one row of the model, and refresh that row. */
    void CommitEditorFor(int row);

    /** @brief Show a message in the status line of the single-page dialog. */
    void ShowError(const QString &message);
};

#endif // ODIALOGSPREAD_H
