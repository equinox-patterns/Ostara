/************************************************************************
 **
 **  @file   dialognormal.h
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

#ifndef DIALOGNORMAL_H
#define DIALOGNORMAL_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;

namespace Ui
{
class DialogNormal;
}

/**
 * @brief The DialogNormal class dialog for ToolNormal. Help create point and edit option.
 */
class DialogNormal final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogNormal(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogNormal() override;

    auto GetPointName() const -> QString;
    void SetPointName(const QString &value);

    auto GetTypeLine() const -> QString;
    void SetTypeLine(const QString &value);

    auto GetFormula() const -> QString;
    void SetFormula(const QString &value);

    auto GetAngle() const -> qreal;
    void SetAngle(qreal value);

    auto GetFirstPointId() const -> quint32;
    void SetFirstPointId(quint32 value);

    auto GetSecondPointId() const -> quint32;
    void SetSecondPointId(quint32 value);

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
    Q_DISABLE_COPY_MOVE(DialogNormal) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogNormal *ui;

    FormulaInputWidget *m_formula;

    /** @brief angle aditional angle of normal */
    qreal m_angle{0};

    QString m_pointName{};

    quint32 m_firstPointId{NULL_ID};
    quint32 m_secondPointId{NULL_ID};

    bool m_flagName{true};
    bool m_flagAngle{true};

    QString m_initialRoleName{}; // the name the line bears when the dialog opens: not a clash with itself
    bool flagRoleName{true};
    QLabel *m_labelRoleName{nullptr};

    bool m_firstRelease{false};

    /** @brief number number of handled objects */
    qint32 m_number{0};

    void FinishCreating();

    void InitIcons();
};

#endif // DIALOGNORMAL_H
