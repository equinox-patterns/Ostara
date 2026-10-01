/************************************************************************
 **
 **  @file   odialogspiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Dialog of the Spiral tool: the radius of the first quarter
 **          turn, the length wanted, the rotation, and the name of the
 **          point the spiral arrives at.
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

#ifndef ODIALOGSPIRAL_H
#define ODIALOGSPIRAL_H

#include "dialogtool.h"

class AngleFormulaWidget;
class FormulaInputWidget;
class QCheckBox;

namespace Ui
{
class ODialogSpiral;
}

class ODialogSpiral : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    ODialogSpiral(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~ODialogSpiral() override;

    auto GetStartPointId() const -> quint32;
    void SetStartPointId(quint32 id);

    auto GetFormulaRadius() const -> QString;
    void SetFormulaRadius(const QString &value);

    auto GetFormulaLength() const -> QString;
    void SetFormulaLength(const QString &value);

    auto GetFormulaAngle() const -> QString;
    void SetFormulaAngle(const QString &value);

    auto GetPenStyle() const -> QString;
    void SetPenStyle(const QString &value);

    auto GetColor() const -> QString;
    void SetColor(const QString &value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetAliasSuffix(const QString &alias);
    auto GetAliasSuffix() const -> QString;

    auto GetEndPointName() const -> QString;
    void SetEndPointName(const QString &name);

    /// Id of the point the spiral arrives at, NULL_ID while the tool does not exist yet. The curve does not own
    /// it: this is what a rename typed here has to reach.
    void SetEndPointId(quint32 id);

    auto GetEndShowLabel() const -> bool;
    void SetEndShowLabel(bool show);

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    auto UsesVisualization() const -> bool override;
    /**
     * @brief SaveData Put dialog data in local variables
     */
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    void ValidateAlias();
    void ValidateEndPointName();

private:
    Q_DISABLE_COPY_MOVE(ODialogSpiral) // NOLINT
    Ui::ODialogSpiral *ui;

    FormulaInputWidget *m_formulaRadius;
    FormulaInputWidget *m_formulaLength;
    AngleFormulaWidget *m_formulaAngle;

    QCheckBox *m_endShowLabelCheckbox{nullptr};
    bool m_showLabelEnd{false};

    bool m_flagAlias{true};
    bool m_flagEndPointName{true};

    QString m_originAliasSuffix{};
    QString m_originEndPointName{};
    quint32 m_endPointId{NULL_ID};

    void FinishCreating();

    /// Rename the arrival point when its name was edited here, the way the property browser does it.
    void ApplyEndPointName();
};

#endif // ODIALOGSPIRAL_H
