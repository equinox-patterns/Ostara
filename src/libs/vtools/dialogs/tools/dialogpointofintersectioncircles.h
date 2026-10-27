/************************************************************************
 **
 **  @file   dialogpointofintersectioncircles.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   29 5, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 31/07/2026.
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

#ifndef DIALOGPOINTOFINTERSECTIONCIRCLES_H
#define DIALOGPOINTOFINTERSECTIONCIRCLES_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;

namespace Ui
{
class DialogPointOfIntersectionCircles;
}

class DialogPointOfIntersectionCircles final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogPointOfIntersectionCircles(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                                     QWidget *parent = nullptr);
    ~DialogPointOfIntersectionCircles() override;

    auto GetPointName() const -> QString;
    void SetPointName(const QString &value);

    auto GetFirstCircleCenterId() const -> quint32;
    void SetFirstCircleCenterId(const quint32 &value);

    auto GetSecondCircleCenterId() const -> quint32;
    void SetSecondCircleCenterId(const quint32 &value);

    auto GetFirstCircleRadius() const -> QString;
    void SetFirstCircleRadius(const QString &value);

    auto GetSecondCircleRadius() const -> QString;
    void SetSecondCircleRadius(const QString &value);

    auto GetCrossCirclesPoint() const -> CrossCirclesPoint;
    void SetCrossCirclesPoint(const CrossCirclesPoint &p);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void PointChanged();

protected:
    void ShowVisualization() override;
    /**
     * @brief SaveData Put dialog data in local variables
     */
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(DialogPointOfIntersectionCircles) // NOLINT

    Ui::DialogPointOfIntersectionCircles *ui;

    QString m_pointName{};

    bool m_flagName{true};
    bool m_flagError{true};

    FormulaInputWidget *m_formulaCircle1;
    FormulaInputWidget *m_formulaCircle2;

    bool m_firstRelease{false};

    /** @brief number number of handled objects */
    qint32 m_stage{0};

    void FinishCreating();

    void InitIcons();
};

#endif // DIALOGPOINTOFINTERSECTIONCIRCLES_H
