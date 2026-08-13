/************************************************************************
 **
 **  @file   dialogcutcurve.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/08/2026
 **
 **  @brief  Thin collector for the unified "Segment a curve" tool.
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

#ifndef DIALOGCUTCURVE_H
#define DIALOGCUTCURVE_H

#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "dialogtool.h"

/**
 * @brief The DialogCutCurve class is a thin headless collector for the unified "Segment a curve" tool.
 *
 * On the first curve click it records the curve id and its type (Spline vs SplinePath) and concludes
 * immediately, so `MainWindow::ClosedCutCurveDialog` can route to the existing `ToolCutSpline` /
 * `ToolCutSplinePath` (whose backends stay untouched) and forward the click.
 */
class DialogCutCurve final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogCutCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogCutCurve() override;

    auto getCurveId() const -> quint32;
    auto IsSplinePath() const -> bool;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    auto IsValid() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(DialogCutCurve) // NOLINT

    quint32 m_curveId{NULL_ID};
    bool m_isSplinePath{false};
};

#endif // DIALOGCUTCURVE_H
