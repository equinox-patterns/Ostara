/************************************************************************
 **
 **  @file   odialogflouncespiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The dialog of the flounce spiral: two values, an angle, one
 **          named curve, one style and the three points of the figure.
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

#ifndef ODIALOGFLOUNCESPIRAL_H
#define ODIALOGFLOUNCESPIRAL_H

#include <QString>

#include "../../tools/drawTools/operation/oflouncespiraldisplay.h"
#include "dialogtool.h"

class AngleFormulaWidget;
class FormulaInputWidget;
class QCheckBox;
class QComboBox;

namespace Ui
{
class ODialogFlounceSpiral;
}

class ODialogFlounceSpiral : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    ODialogFlounceSpiral(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~ODialogFlounceSpiral() override;

    auto GetStartPointId() const -> quint32;
    void SetStartPointId(quint32 id);

    auto GetFormulaWidth() const -> QString;
    void SetFormulaWidth(const QString &value);
    auto GetFormulaLength() const -> QString;
    void SetFormulaLength(const QString &value);
    auto GetFormulaAngle() const -> QString;
    void SetFormulaAngle(const QString &value);

    /// The two values, the angle, the one name of the figure and its one style, as the tool reads them back.
    auto GetDisplay() const -> OFlounceSpiralDisplay;
    void SetDisplay(const OFlounceSpiralDisplay &display);

    /// The name of the ONE curve the figure offers: the outer edge, from A_2 to A_3.
    auto GetOuterAlias() const -> QString;
    void SetOuterAlias(const QString &alias);

    /// The pen and the colour of the whole figure — both edges and both joining segments.
    auto GetPenStyle() const -> QString;
    auto GetColor() const -> QString;

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    auto GetPointName1() const -> QString;
    void SetPointName1(const QString &name);
    auto GetPointName2() const -> QString;
    void SetPointName2(const QString &name);
    auto GetPointName3() const -> QString;
    void SetPointName3(const QString &name);

    auto GetPointShowLabel1() const -> bool;
    void SetPointShowLabel1(bool show);
    auto GetPointShowLabel2() const -> bool;
    void SetPointShowLabel2(bool show);
    auto GetPointShowLabel3() const -> bool;
    void SetPointShowLabel3(bool show);

    void ShowDialog(bool click) override;
    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;

protected:
    auto UsesVisualization() const -> bool override;
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    void ValidateAlias();
    void ValidatePointNames();

private:
    Q_DISABLE_COPY_MOVE(ODialogFlounceSpiral) // NOLINT

    Ui::ODialogFlounceSpiral *ui;

    FormulaInputWidget *m_formulaWidth;
    FormulaInputWidget *m_formulaLength;
    AngleFormulaWidget *m_formulaAngle;

    QComboBox *m_color{nullptr};

    QCheckBox *m_showLabel1{nullptr};
    QCheckBox *m_showLabel2{nullptr};
    QCheckBox *m_showLabel3{nullptr};

    bool m_flagAlias{true};
    bool m_flagPoint1{true};
    bool m_flagPoint2{true};
    bool m_flagPoint3{true};

    QString m_originAlias{};
    QString m_originPointName1{};
    QString m_originPointName2{};
    QString m_originPointName3{};

    void FinishCreating();
    void ApplyPointNames();
};

#endif // ODIALOGFLOUNCESPIRAL_H
