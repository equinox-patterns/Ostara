/************************************************************************
 **
 **  @file   dialogspline.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#ifndef DIALOGSPLINE_H
#define DIALOGSPLINE_H

#include <QMetaObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../vgeometry/vspline.h"
#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;
class AngleFormulaWidget;

namespace Ui
{
class DialogSpline;
}

/**
 * @brief The DialogSpline class dialog for ToolSpline. Help create spline and edit option.
 */
class DialogSpline final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogSpline(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogSpline() override;

    auto GetSpline() const -> VSpline;
    void SetSpline(const VSpline &spline);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetDefPenStyle(const QString &value);
    void SetDefColor(const QString &value);

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void PointNameChanged() override;
    void ShowDialog(bool click) override;

protected:
    void ShowVisualization() override;
    /**
     * @brief SaveData Put dialog data in local variables
     */
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    void ValidateAlias();

private:
    Q_DISABLE_COPY_MOVE(DialogSpline) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogSpline *ui;

    /** @brief spl spline */
    VSpline spl{};

    bool flagError{false};
    bool flagAlias{true};

    FormulaInputWidget *m_formulaLength1;
    AngleFormulaWidget *m_formulaAngle1;
    FormulaInputWidget *m_formulaLength2;
    AngleFormulaWidget *m_formulaAngle2;

    QString originAliasSuffix{};

    /** @brief number number of handled objects */
    qint32 number{0};

    QString m_oldName{};

    auto GetP1() const -> QSharedPointer<VPointF>;
    auto GetP4() const -> QSharedPointer<VPointF>;

    void SaveSpline();

    void InitIcons();
};

#endif // DIALOGSPLINE_H
