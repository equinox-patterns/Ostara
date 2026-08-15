/************************************************************************
 **
 **  @file   otoolcurve.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/08/2026
 **
 **  @brief  Frontend collector for the unified "Curve" tool.
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

#ifndef OTOOLCURVE_H
#define OTOOLCURVE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../vgeometry/vsplinepath.h"
#include "dialogtool.h"

/**
 * @brief The OToolCurve class is a headless creation collector for the unified "Curve" tool.
 *
 * It accumulates the ids of the clicked points and, on Enter, lets the caller decide whether a
 * simple curve (`VSpline`, 2 points) or a curved path (`VSplinePath`, 3+ points) must be created.
 * The backend (`VToolSpline` / `VToolSplinePath`) and the editing dialogs (`DialogSpline` /
 * `DialogSplinePath`) stay untouched.
 */
class OToolCurve final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    OToolCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~OToolCurve() override;

    auto GetPointIds() const -> QVector<quint32>;
    auto GetPath() const -> VSplinePath;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void ShowDialog(bool click) override;

protected:
    void ShowVisualization() override;
    auto IsValid() const -> bool override;

private slots:
    void PathUpdated(const VSplinePath &path);

private:
    Q_DISABLE_COPY_MOVE(OToolCurve) // NOLINT

    QVector<quint32> m_pointIds{};
    VSplinePath m_path{};
};

#endif // OTOOLCURVE_H
