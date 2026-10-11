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
class VCurveLength;
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

    /** @brief Name the object whose row carries the button that asked for it.
     *
     *  An object nobody named cannot be written in a formula, so the list that offers it offers the name as
     *  well — and this is where the name is given, without leaving the formula being written. A line and a curve
     *  are not named in the same place; the row says which one, and this reads it there. */
    void RenameObject();

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

    /** @brief True when the user cancelled the naming of the objects the formula uses.
     *
     *  Nothing was written — no name, no repair — so there is nothing to edit and the dialogue closes itself
     *  instead of showing a formula it would refuse. */
    bool m_nameRequestCancelled{false};

    /** @brief m_isInitialized true if window is initialized */
    bool m_isInitialized{false};

    void InitVariables();

    /** @brief Lay the table out for a list that offers to name its objects, or for one that does not.
     *
     *  The last column is taken away rather than hidden for the categories that have nothing to name: hidden,
     *  it keeps its header and its widths, so the table still carries a column with no reason to be there. */
    void SetTableColumns(bool nameable);

    /** @brief Show only the switches that mean something for the category on screen.
     *
     *  @a measurements for the one category whose values can be empty, @a unnamed for the ones whose objects can
     *  be without a name. A switch that does nothing is a question the user answers for nothing. */
    void SetCategoryCheckBoxes(bool measurements, bool unnamed);

    /** @brief Two columns of equal width, and the "Name it" one at the width its button needs. */
    void ApplyColumnWidths();

    /** @brief The width the "Name it" column needs for its header and its button, measured rather than guessed. */
    auto NameActionWidth() const -> int;

    /** @brief List @a vars, offering to name the ones nobody named when @a nameable.
     *
     *  @a unnamed says whether this list holds objects that can be without a name, and so whether the switch
     *  that hides them applies — a category of measurements has nothing to do with it. */
    template <class T> void ShowVariable(const QList<T> &vars, bool nameable = false, bool unnamed = false);

    /** @brief Put the "name it" button in @a row, for the object of @a reference.
     *
     *  @a reference is the pair of points for a line, the variable's own name for a curve. */
    void AddNameButton(int row, const QString &reference);

    /** @brief The row holding @a widget, or -1: what a cell widget cannot say about itself. */
    auto RowOfWidget(const QWidget *widget) const -> int;

    /** @brief The tool that draws the curve held under @a variableName, or NULL_ID. */
    auto CurveToolId(const QString &variableName) const -> quint32;

    /** @brief The alias the curve's own tool carries, empty while it has none. */
    auto CurveAlias(const QString &variableName) const -> QString;

    /** @brief Show again the category on screen, after a name has been given. */
    void ShowCurrentCategory();

    /** @brief Arc: the lengths and the radii of the pattern's arcs, in one list — see the definition. */
    void Arc();

    /** @brief Whether a curve length belongs to an arc — `Arc_…` or `ElArc_…` — and so to the "Arc" category. */
    static auto IsArcCurve(const QSharedPointer<VCurveLength> &var) -> bool;

    void ShowMeasurements(const QList<QSharedPointer<VMeasurement>> &vars);
    void ShowFunctions();
    void ShowConstants();
    void ShowIncrementsInPreviewCalculation(bool show);

    void SetMeasurementDescription(QTableWidgetItem *item, const QString &name);
    void SetPieceAreaDescription(QTableWidgetItem *item, const QString &name);

    void SetDescription(const QString &name, qreal value, bool specialUnits, const QString &description,
                        bool square = false);

    auto Eval(const FormulaData &formulaData, bool &flag) -> qreal;

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
