/************************************************************************
 **
 **  @file   dialogpointofcontact.h
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

#ifndef DIALOGPOINTOFCONTACT_H
#define DIALOGPOINTOFCONTACT_H

#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "dialogtool.h"

class FormulaInputWidget;

namespace Ui
{
class DialogPointOfContact;
}

/**
 * @brief The DialogPointOfContact class dialog for ToolPointOfContact. Help create point and edit option.
 */
class DialogPointOfContact final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    DialogPointOfContact(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogPointOfContact() override;

    auto GetPointName() const -> QString;
    void SetPointName(const QString &value);

    auto GetRadius() const -> QString;
    void SetRadius(const QString &value);

    auto GetCenter() const -> quint32;
    void SetCenter(quint32 value);

    auto GetFirstPoint() const -> quint32;
    void SetFirstPoint(quint32 value);

    auto GetSecondPoint() const -> quint32;
    void SetSecondPoint(quint32 value);

    void SetNotes(const QString &notes);
    auto GetNotes() const -> QString;

    void SetLineRoleName(const QString &suffix);
    auto GetLineRoleName() const -> QString;

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void ChosenObject(quint32 id, const SceneObject &type) override;
    void PointNameChanged() override;

private slots:
    void ValidateLineRoleName();

protected:
    void ShowVisualization() override;
    /**
     * @brief SaveData Put dialog data in local variables
     */
    void SaveData() override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    auto IsValid() const -> bool override;

private:
    Q_DISABLE_COPY_MOVE(DialogPointOfContact) // NOLINT

    /** @brief ui keeps information about user interface */
    Ui::DialogPointOfContact *ui;

    QString m_pointName{};

    FormulaInputWidget *m_formula;

    bool m_flagName{true};
    bool m_flagError{true};
    QString m_initialRoleName{}; // the name the line bears when the dialog opens: not a clash with itself
    bool flagRoleName{true};
    QLabel *m_labelRoleName{nullptr};

    /** @brief number number of handled objects */
    qint32 m_number{0};

    bool m_firstRelease{false};

    void FinishCreating();

    void InitIcons();
};

#endif // DIALOGPOINTOFCONTACT_H
