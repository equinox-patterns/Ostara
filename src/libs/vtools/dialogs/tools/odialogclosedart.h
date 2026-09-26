/************************************************************************
 **
 **  @file   odialogclosedart.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The dialog of the tool that closes a dart, and the four steps of its creation.
 **
 **          Rotation asks for its objects, then for its pivot, and opens. This one asks for the objects, the
 **          pivot, the point that travels and the point it lands on — the three points of the dart, picked in
 **          the pattern, before the window opens. None of them is named anywhere: they are the ones the user
 **          clicked, and what the tool does with them is what matters.
 **
 **          A click that cannot answer the step is refused, and the step does not move: the status line says
 **          what to pick instead, the way Rotation already answers a refused pivot.
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

#ifndef ODIALOGCLOSEDART_H
#define ODIALOGCLOSEDART_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "dialogoperationtool.h"

class QComboBox;
class FormulaInputWidget;

namespace Ui
{
class DialogCloseDart;
}

class ODialogCloseDart final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit ODialogCloseDart(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                              QWidget *parent = nullptr);
    ~ODialogCloseDart() override;

    auto GetOrigPointId() const -> quint32;
    void SetOrigPointId(quint32 value);

    auto GetMovingPointId() const -> quint32;
    void SetMovingPointId(quint32 value);

    auto GetTargetPointId() const -> quint32;
    void SetTargetPointId(quint32 value);

    auto GetDivisor() const -> QString;
    void SetDivisor(const QString &value);

    void ShowDialog(bool click) override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void ShowVisualization() override;

    /** @brief SaveData Put dialog data in local variables */
    void SaveData() override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

    void OnSourceObjectsSet() override;

    auto Widgets() const -> OperationWidgets override;

private slots:
    void DivisorChanged();

private:
    Q_DISABLE_COPY_MOVE(ODialogCloseDart) // NOLINT

    /** @brief The step of the creation the dialog is waiting for. The objects come first, then the three
     *  points of the dart, and the window only opens once it is complete. */
    enum class Step : qint8
    {
        Objects,
        Pivot,
        Moving,
        Target,
        Complete
    };

    Ui::DialogCloseDart *ui;

    /** @brief m_formulaDivisor how much of the opening is closed — the only value of this tool, and the only
     *  thing the user types: the angle itself belongs to the dart. */
    FormulaInputWidget *m_formulaDivisor{nullptr};

    /** @brief The two points of the dart that are not the pivot, held in combos of their own and never shown.
     *  Validating a picked object is the base class's job and it reads a combo; the three points are the ones
     *  clicked in the pattern, so none of these names ever reaches the screen. */
    QComboBox *m_comboMovingPoint{nullptr};
    QComboBox *m_comboTargetPoint{nullptr};

    Step m_step{Step::Objects};

    /** @brief True while the release that has just picked a point still has to be swallowed: the same release
     *  ends the pick and would otherwise be read as the click that ends the next step. */
    bool m_swallowRelease{false};

    /** @brief The three points of the dart are known and tell each other apart — what a dart is made of. */
    auto PointsAreDistinct() const -> bool;

    void TakePivot(quint32 id);
    void TakeMovingPoint(quint32 id);
    void TakeTargetPoint(quint32 id);

    /** @brief Say why a click cannot answer the step, and leave the user on that same step.
     *
     * The status line alone is too discreet for a click that does nothing: the user does not read it and
     * believes the tool is stuck. One button, and the step is still the one to answer. */
    void Refuse(const QString &message);

    /** @brief Show the sources and the dart as far as they are known. */
    void RefreshPreview();
};

#endif // ODIALOGCLOSEDART_H
