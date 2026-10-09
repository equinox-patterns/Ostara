/************************************************************************
 **
 **  @file   dialoggraduatedcurve.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   4 12, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
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
#ifndef DIALOGGRADUATEDCURVE_H
#define DIALOGGRADUATEDCURVE_H

#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "dialogtool.h"

namespace Ui
{
class DialogGraduatedCurve;
}

struct VRawGraduatedCurveOffset;
class QCheckBox;

struct VGraduatedCurveOffsetFormula
{
    QString name{};
    VFormula formula{};
    QSharedPointer<VContainer> formulaData{};
    QString description{};
};

class DialogGraduatedCurve : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    explicit DialogGraduatedCurve(const VContainer *data,
                                  VAbstractPattern *doc,
                                  quint32 toolId,
                                  QWidget *parent = nullptr);
    ~DialogGraduatedCurve() override;

    auto GetOriginCurveId() const -> quint32;
    void SetOriginCurveId(const quint32 &value);

    auto GetOffsets() const -> QVector<VRawGraduatedCurveOffset>;
    void SetOffsets(const QVector<VRawGraduatedCurveOffset> &offsets);

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

    /**
     * @brief Shows one end of the curve as it is in the pattern: whether the point exists, what it is called, whether
     * its label is drawn. Called by the tool, both when the dialog opens on an existing curve and after the pattern
     * has been rebuilt under it.
     */
    void SetStartPoint(bool exists, const QString &name, bool showLabel);
    void SetEndPoint(bool exists, const QString &name, bool showLabel);

    /**
     * @brief The dialog drives a tool the pattern rebuilds when a point is added to the curve or taken away from it:
     * rather than being closed by that parse, it is linked again to the rebuilt curve, which reads its options anew
     * (see MainWindow::FullParseFile and DialogTool::RebuildVisualization).
     */
    auto SurvivesFullParse() const -> bool override { return true; }

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
    void ShowOffsetDetails();
    void AddOffset();
    void RemoveOffset();
    void MoveUp();
    void MoveDown();
    void DeployFormula();
    void Fx();
    void ValidateOffsetName();
    void SaveOffsetName();
    void SaveOffsetDescription();
    void SaveOffsetFormula();

private:
    Q_DISABLE_COPY_MOVE(DialogGraduatedCurve) // NOLINT
    Ui::DialogGraduatedCurve *ui;

    bool m_flagName{true};
    bool m_flagAlias{true};

    QString m_originName{};
    QString m_originAliasSuffix{};

    bool m_firstRelease{false};

    int m_formulaBaseHeight{0};

    QVector<VGraduatedCurveOffsetFormula> m_offsets{};

    /// Line whose name is being typed in the Name field, and what was typed for it. Kept here because the table can
    /// move on before the field is left: the rename has to land on the line the name was typed for, and a field the
    /// table has refilled since carries the name of another line — which is not a rename at all.
    int m_namedOffsetRow{-1};
    QString m_namedOffsetText{};

    QCheckBox *m_startShowLabelCheckbox{nullptr};
    QCheckBox *m_endShowLabelCheckbox{nullptr};
    bool m_showLabelStart{false};
    bool m_showLabelEnd{false};

    /// True when the point is already in the pattern. Its name is then the point's own business — every formula that
    /// uses it would have to follow a rename — so the tab shows it without offering to change it.
    bool m_startPointExists{false};
    bool m_endPointExists{false};

    void FinishCreating();

    void InitIcons();

    void FillOffsets();

    void Controls();
    void EnableDetails(bool enabled);

    auto GetOffsetName(bool translate) const -> QString;

    auto OffsetNameProblem(int row, const QString &name) const -> QString;

    auto VisualizationOffsets() const -> QVector<VRawGraduatedCurveOffset>;

    void ShowHeaderUnits(int column);

    void UpdatePointControls();

    /// Tells the preview which width is being edited — the rank of the table's line — or that none is.
    void SetActiveOffset(int row);
};

#endif // DIALOGGRADUATEDCURVE_H
