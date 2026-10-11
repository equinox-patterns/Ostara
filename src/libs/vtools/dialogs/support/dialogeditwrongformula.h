/************************************************************************
 **
 **  @file   dialogeditwrongformula.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   29 5, 2014
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

#ifndef DIALOGEDITWRONGFORMULA_H
#define DIALOGEDITWRONGFORMULA_H

#include <QDialog>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QTableWidgetItem>
#include <QtGlobal>

class VMeasurement;
struct FormulaData;
class VContainer;

namespace Ui
{
class DialogEditWrongFormula;
}

/**
 * @brief The DialogEditWrongFormula class dialog for editing wrong formula.
 *
 * When math parser find in formula error user can try fix issue. Dialog will show all variables that user can use in
 * this formula. Dialog check fixed variant of formula.
 *
 * Don't implemant button "Apply" for this dialog!!
 */
class DialogEditWrongFormula final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    DialogEditWrongFormula(const VContainer *data, quint32 toolId, QWidget *parent = nullptr);
    ~DialogEditWrongFormula() override;

    auto GetFormula() const -> QString;
    void SetFormula(const QString &value);
    void setCheckZero(bool value);
    void setCheckLessThanZero(bool value);
    void setPostfix(const QString &value);
    void SetMeasurementsMode();
    void SetIncrementsMode();
    void SetPreviewCalculationsMode();
    void ShowPieceArea(bool show) const;

public slots:
    void DialogAccepted();
    void DialogRejected();

    void EvalFormula();
    void ValChanged(int row);
    void PutHere();
    void PutVal(QTableWidgetItem *item);

    void Measurements();
    void LengthLines();
    void RadiusArcs();
    void AnglesCurves();
    void LengthCurves();
    void CurvesCLength();
    void AngleLines();
    void Increments();
    void PreviewCalculations();
    void PieceArea();
    void Functions();
    void Constants();

signals:
    /**
     * @brief DialogClosed signal dialog closed
     * @param result keep result
     */
    void DialogClosed(int result);
    /**
     * @brief DialogApplied emit signal dialog apply changes
     */
    void DialogApplied();

private slots:
    void FilterVariablesEdited(const QString &filter);

    /** @brief Name the line whose row carries the button that asked for it.
     *
     *  A line nobody named cannot be written in a formula, so the list that offers its pair offers the name as
     *  well — and this is where the name is given, without leaving the formula being written. */
    void RenameLine();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(DialogEditWrongFormula) // NOLINT
    Ui::DialogEditWrongFormula *ui;

    const VContainer *m_data;

    quint32 m_toolId;

    /** @brief formula string with formula */
    QString formula{};

    /** @brief formulaBaseHeight base height defined by dialogui */
    int formulaBaseHeight{0};

    bool checkZero{false};
    bool checkLessThanZero{false};
    QString postfix{};
    bool restoreCursor{false};

    QTimer *timerFormula;

    bool flagFormula{false};

    /** @brief The variable a refusal was about, as the list writes it, or empty when the last evaluation found
     *  nothing to refuse.
     *
     *  Kept so the list can point at the object instead of naming it: the refusal must not put a pair of points
     *  back in front of the user, and a highlighted row says which object without saying its name. Held as the
     *  displayed name, the one both the list and the formula field agree on. */
    QString m_unnamedObject{};

    /** @brief m_isInitialized true if window is initialized */
    bool m_isInitialized{false};

    void InitVariables();

    template <class T> void ShowVariable(const QList<T> &vars);

    /** @brief Put the "name it" button in @a row, for the line of @a pairKey. */
    void AddNameButton(int row, const QString &pairKey);
    void ShowMeasurements(const QList<QSharedPointer<VMeasurement>> &vars);
    void ShowFunctions();
    void ShowConstants();
    void ShowIncrementsInPreviewCalculation(bool show);

    void SetMeasurementDescription(QTableWidgetItem *item, const QString &name);
    void SetPieceAreaDescription(QTableWidgetItem *item, const QString &name);

    void SetDescription(const QString &name, qreal value, bool specialUnits, const QString &description,
                        bool square = false);

    auto Eval(const FormulaData &formulaData, bool &flag) -> qreal;

    void InitIcons();

    auto IsValid() const -> bool;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void CheckState();
};

//---------------------------------------------------------------------------------------------------------------------
inline auto DialogEditWrongFormula::IsValid() const -> bool
{
    return flagFormula;
}

#endif // DIALOGEDITWRONGFORMULA_H
