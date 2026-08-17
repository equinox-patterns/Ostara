/************************************************************************
 **
 **  @file   dialogendline.h
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

#ifndef DIALOGENDLINE_H
#define DIALOGENDLINE_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;
class AngleFormulaWidget;

namespace Ui
{
class DialogEndLine;
}

/**
 * @brief The DialogEndLine class dialog for ToolEndLine. Help create point and edit option.
 */
class DialogEndLine final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogEndLine(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogEndLine() override;

    auto GetPointName() const -> QString;
    void SetPointName(const QString &value);

    auto GetTypeLine() const -> QString;
    void SetTypeLine(const QString &value);

    auto GetFormula() const -> QString;
    void SetFormula(const QString &value);

    auto GetAngle() const -> QString;
    void SetAngle(const QString &value);

    auto GetBasePointId() const -> quint32;
    void SetBasePointId(const quint32 &value);

    auto GetLineColor() const -> QString;
    void SetLineColor(const QString &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetLineRoleName(const QString &suffix);
    auto GetLineRoleName() const -> QString;

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    void ShowVisualization() override;
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    void ValidateLineRoleName();

private:
    Q_DISABLE_COPY_MOVE(DialogEndLine) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogEndLine *ui;

    FormulaInputWidget *m_formulaLength;
    AngleFormulaWidget *m_formulaAngle;

    QString pointName;

    bool m_firstRelease;

    quint32 m_basePointId{NULL_ID};

    bool flagName;

    bool flagRoleName{true};
    QLabel *m_labelRoleName{nullptr};

    void FinishCreating();

    void InitIcons();
};

#endif // DIALOGENDLINE_H
