/************************************************************************
 **
 **  @file   dialogellipticalarc.h
 **  @author Valentina Zhuravska <zhuravska19(at)gmail.com>
 **  @date   15 9, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 19/08/2026.
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

#ifndef DIALOGELLIPTICALARC_H
#define DIALOGELLIPTICALARC_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;
class AngleFormulaWidget;
class QCheckBox;

namespace Ui
{
class DialogEllipticalArc;
}

class DialogEllipticalArc final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogEllipticalArc(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogEllipticalArc() override;

    auto GetCenter() const -> quint32;
    void SetCenter(quint32 value);

    auto GetRadius1() const -> QString;
    void SetRadius1(const QString &value);

    auto GetRadius2() const -> QString;
    void SetRadius2(const QString &value);

    auto GetF1() const -> QString;
    void SetF1(const QString &value);

    auto GetF2() const -> QString;
    void SetF2(const QString &value);

    auto GetRotationAngle() const -> QString;
    void SetRotationAngle(const QString &value);

    auto GetPenStyle() const -> QString;
    void SetPenStyle(const QString &value);

    auto GetColor() const -> QString;
    void SetColor(const QString &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetAliasSuffix(const QString &alias);
    auto GetAliasSuffix() const -> QString;

    auto IsCreateStartPoint() const -> bool;
    auto IsCreateEndPoint() const -> bool;
    auto GetStartPointName() const -> QString;
    auto GetEndPointName() const -> QString;
    auto GetStartShowLabel() const -> bool;
    auto GetEndShowLabel() const -> bool;

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

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
    Q_DISABLE_COPY_MOVE(DialogEllipticalArc) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogEllipticalArc *ui;

    FormulaInputWidget *m_formulaRadius1;
    FormulaInputWidget *m_formulaRadius2;
    AngleFormulaWidget *m_formulaF1;
    AngleFormulaWidget *m_formulaF2;
    AngleFormulaWidget *m_formulaRotationAngle;

    bool m_flagAlias{true};

    qreal m_angleF1{INT_MIN};
    qreal m_angleF2{INT_MIN};
    qreal m_angleRotation{INT_MIN};

    QString m_originAliasSuffix{};

    bool m_firstRelease{false};

    int m_stage{0};

    QCheckBox *m_startShowLabelCheckbox{nullptr};
    QCheckBox *m_endShowLabelCheckbox{nullptr};
    bool m_showLabelStart{false};
    bool m_showLabelEnd{false};

    void FinishCreating();

    void InitIcons();

    void UpdatePointControls();
};

#endif // DIALOGELLIPTICALARC_H
