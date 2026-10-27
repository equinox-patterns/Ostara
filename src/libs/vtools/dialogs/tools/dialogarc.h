/************************************************************************
 **
 **  @file   dialogarc.h
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

#ifndef DIALOGARC_H
#define DIALOGARC_H

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
class DialogArc;
}

/**
 * @brief The DialogArc class dialog for ToolArc. Help create arc and edit option.
 */
class DialogArc final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogArc(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogArc() override;

    auto GetCenter() const -> quint32;
    void SetCenter(const quint32 &value);

    auto GetRadius() const -> QString;
    void SetRadius(const QString &value);

    auto GetF1() const -> QString;
    void SetF1(const QString &value);

    auto GetF2() const -> QString;
    void SetF2(const QString &value);

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
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private slots:
    void ValidateAlias();

private:
    Q_DISABLE_COPY_MOVE(DialogArc) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogArc *ui;

    FormulaInputWidget *m_formulaRadius;
    AngleFormulaWidget *m_formulaF1;
    AngleFormulaWidget *m_formulaF2;

    bool m_flagAlias{true};

    qreal m_angleF1{INT_MIN};
    qreal m_angleF2{INT_MIN};

    QString m_originAliasSuffix{};

    bool m_stageRadius{true};
    bool m_stageF1{false};

    bool m_firstRelease{false};

    QCheckBox *m_startShowLabelCheckbox{nullptr};
    QCheckBox *m_endShowLabelCheckbox{nullptr};
    bool m_showLabelStart{false};
    bool m_showLabelEnd{false};

    void FinishCreating();

    void InitIcons();

    void UpdatePointControls();
};

#endif // DIALOGARC_H
