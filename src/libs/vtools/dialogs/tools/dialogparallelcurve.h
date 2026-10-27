/************************************************************************
 **
 **  @file   dialogparallelcurve.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   28 11, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 20/08/2026.
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
#ifndef DIALOGPARALLELCURVE_H
#define DIALOGPARALLELCURVE_H

#include "dialogtool.h"

class FormulaInputWidget;
class QCheckBox;

namespace Ui
{
class DialogParallelCurve;
}

class DialogParallelCurve : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogParallelCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogParallelCurve() override;

    auto GetOriginCurveId() const -> quint32;
    void SetOriginCurveId(const quint32 &value);

    auto GetFormulaWidth() const -> QString;
    void SetFormulaWidth(const QString &value);

    auto GetPenStyle() const -> QString;
    void SetPenStyle(const QString &value);

    auto GetColor() const -> QString;
    void SetColor(const QString &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetName(const QString &name);
    auto GetName() const -> QString;

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
    void ValidateName();
    void ValidateAlias();

private:
    Q_DISABLE_COPY_MOVE(DialogParallelCurve) // NOLINT
    Ui::DialogParallelCurve *ui;

    FormulaInputWidget *m_formula;

    bool m_flagName{true};
    bool m_flagAlias{true};

    QString m_originName{};
    QString m_originAliasSuffix{};

    bool m_firstRelease{false};

    QCheckBox *m_startShowLabelCheckbox{nullptr};
    QCheckBox *m_endShowLabelCheckbox{nullptr};
    bool m_showLabelStart{false};
    bool m_showLabelEnd{false};

    void FinishCreating();

    void InitIcons();

    void UpdatePointControls();
};

#endif // DIALOGPARALLELCURVE_H
