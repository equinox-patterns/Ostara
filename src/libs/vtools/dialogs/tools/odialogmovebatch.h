/************************************************************************
 **
 **  @file   odialogmovebatch.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The dialog of a move by batch: one row per cut line, and the three values each one applies.
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

#ifndef ODIALOGMOVEBATCH_H
#define ODIALOGMOVEBATCH_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../tools/drawTools/operation/omovebatchrule.h"
#include "dialogoperationtool.h"

namespace Ui
{
class DialogMoveBatch;
}

class AngleFormulaWidget;
class FormulaInputWidget;

class ODialogMoveBatch final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogMoveBatch(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                              QWidget *parent = nullptr);
    ~ODialogMoveBatch() override;

    /** @brief The moves, with the values the user last typed. */
    auto GetMoves() const -> QVector<OMoveBatchMove>;
    void SetMoves(const QVector<OMoveBatchMove> &moves);

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
    void ShowMoveDetails();
    void SaveMoveName(const QString &text);
    void LengthChanged(bool valid);
    void AngleChanged(bool valid);
    void RotationAngleChanged(bool valid);

private:
    Q_DISABLE_COPY_MOVE(ODialogMoveBatch) // NOLINT

    Ui::DialogMoveBatch *ui;

    FormulaInputWidget *m_formulaLength{nullptr};
    AngleFormulaWidget *m_formulaAngle{nullptr};
    AngleFormulaWidget *m_formulaRotationAngle{nullptr};

    /** @brief Where the gesture stands. Objects are picked first, then one pair (start, pivot) per move, then
     *  the dialog opens to edit the values. */
    enum class Step
    {
        Selecting, // objects are being picked; Enter ends the selection
        Start,     // the next point clicked is the start of the current move
        Pivot,     // the next point clicked is the pivot of the current move
        Editing    // the dialog is up
    };

    Step m_step{Step::Selecting};
    int m_currentMove{0};

    QVector<OMoveBatchMove> m_moves{};

    /** @brief The move the editor is currently showing, -1 when none. The formula fields evaluate on a timer, so
     *  an edit still pending when the selection moves must be written back to THIS row, not to the one that has
     *  just been selected. */
    int m_editorRow{-1};

    /** @brief The move whose details are shown in the editor, -1 when none. */
    auto CurrentRow() const -> int;

    /** @brief Rewrite one row of the table (name, length, angle, rotation angle) from the model. */
    void RefreshRow(int row);

    void FillMoves();
    void EnableDetails(bool enabled);

    /** @brief Write what the editor fields hold into one row of the model, and refresh that row. */
    void CommitEditorFor(int row);

    /** @brief Show a message in the status line of the single-page dialog. */
    void ShowError(const QString &message);
};

#endif // ODIALOGMOVEBATCH_H
