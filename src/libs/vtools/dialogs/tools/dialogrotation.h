/************************************************************************
 **
 **  @file   dialogrotation.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   10 4, 2016
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

#ifndef DIALOGROTATION_H
#define DIALOGROTATION_H

#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "dialogoperationtool.h"

namespace Ui
{
class DialogRotation;
}

class AngleFormulaWidget;
class FormulaInputWidget;
class QRadioButton;

class DialogRotation final : public DialogOperationTool
{
    Q_OBJECT // NOLINT

public:
    explicit DialogRotation(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogRotation() override;

    auto GetOrigPointId() const -> quint32;
    void SetOrigPointId(quint32 value);

    auto GetAngle() const -> QString;
    void SetAngle(const QString &value);

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
    void PointChanged();

private:
    Q_DISABLE_COPY_MOVE(DialogRotation) // NOLINT
    Ui::DialogRotation *ui;

    /** @brief m_formulaAngle angle field — auto-growing height, value display and 0/90/180/270° presets */
    AngleFormulaWidget *m_formulaAngle{nullptr};

    /** @brief m_formulaLength length field — the chord of the reference point, another way to say the angle */
    FormulaInputWidget *m_formulaLength{nullptr};

    /** @brief the two ways to set a rotation: only the checked field is editable, the other one is derived */
    QRadioButton *m_radioAngle{nullptr};
    QRadioButton *m_radioLength{nullptr};

    /** @brief m_referencePointId the source point the length refers to, NULL_ID when the sources hold none */
    quint32 m_referencePointId{NULL_ID};

    /** @brief m_lengthIsPossible false when the typed length passes the circle diameter */
    bool m_lengthIsPossible{true};

    bool m_firstRelease{false};

    /** @brief UserAngle the angle in the user's own notation, which is what the visualization expects:
     *  VisToolRotation::SetAngle evaluates a user formula. */
    auto UserAngle() const -> QString;

    /** @brief ReferenceRadius distance from the pivot to the reference point, in scene units; 0 when the
     *  two are not both known. */
    auto ReferenceRadius() const -> qreal;

    /** @brief Derive the length from the angle, and the angle from the length: the two say the same rotation. */
    void UpdateLengthFromAngle();
    void UpdateAngleFromLength();

    /** @brief Find the reference point again — the length holds for that one point only. */
    void UpdateReferencePoint();

    /** @brief Show a message in the status line, which the single-page dialog hides when all is well. */
    void ShowError(const QString &message);

    void AngleChanged(bool valid);
    void LengthChanged(bool valid);
};

#endif // DIALOGROTATION_H
