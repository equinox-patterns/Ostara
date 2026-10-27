/************************************************************************
 **
 **  @file   dialogsplinepath.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 09/10/2026.
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

#ifndef DIALOGSPLINEPATH_H
#define DIALOGSPLINEPATH_H

#include <QMetaObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../vgeometry/vsplinepath.h"
#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;
class AngleFormulaWidget;

namespace Ui
{
class DialogSplinePath;
}

/**
 * @brief The DialogSplinePath class dialog for ToolSplinePath. Help create spline path and edit option.
 */
class DialogSplinePath final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogSplinePath(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogSplinePath() override;

    auto GetPath() const -> VSplinePath;
    void SetPath(const VSplinePath &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetDefPenStyle(const QString &value);
    void SetDefColor(const QString &value);

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void ShowDialog(bool click) override;
    void PathUpdated(const VSplinePath &path);

protected:
    void ShowVisualization() override;
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void PointChanged(int row);
    void currentPointChanged(int index);
    void NewPointChanged();

    void AddPoint();
    void RemovePoint();

    void MoveTop();
    void MoveUp();
    void MoveDown();
    void MoveBottom();

    void ValidateAlias();

private:
    Q_DISABLE_COPY_MOVE(DialogSplinePath) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogSplinePath *ui;

    /** @brief path spline path */
    VSplinePath path{};

    bool flagError{false};
    bool flagAlias{true};

    QString originAliasSuffix{};

    bool m_dependencyReady{true};

    QString m_oldName{};

    FormulaInputWidget *m_formulaLength1{nullptr};
    AngleFormulaWidget *m_formulaAngle1{nullptr};
    FormulaInputWidget *m_formulaLength2{nullptr};
    AngleFormulaWidget *m_formulaAngle2{nullptr};

    bool m_visConnected{false};

    void NewItem(const VSplinePoint &point);
    void DataPoint(const VSplinePoint &p);
    void SavePath();
    void SaveCurrentPoint();
    void Angle1Changed();
    void Angle2Changed();
    void Length1Changed();
    void Length2Changed();
    auto AllIds() const -> QSet<quint32>;
    auto IsPathValid() const -> bool;
    auto ExtractPath() const -> VSplinePath;

    // A row of the point table carries the point and the validity of its four handle fields in its first cell, so
    // that a row can be read, rebuilt or moved on its own — the rows are the model, the widgets only show it.
    void CreateRow(int row);
    auto RowPoint(int row) const -> VSplinePoint;
    void SetRowPoint(int row, const VSplinePoint &p);
    auto RowValidity(int row) const -> int;
    void SetRowValidity(int row, int validity);
    void UpdateRowValidity(int row, int flag, bool valid);
    void RefreshRow(int row);
    void RefreshRows();
    auto PointLabel(int row) const -> QString;
    void MoveRow(int from, int to);
    auto CurrentRow() const -> int;

    /** @brief Shows the row being edited as the active point of the path in the canvas. */
    void SetActivePoint(int row);

    void MoveControls();
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetPath return spline path
 * @return path
 */
inline auto DialogSplinePath::GetPath() const -> VSplinePath
{
    return path;
}

#endif // DIALOGSPLINEPATH_H
