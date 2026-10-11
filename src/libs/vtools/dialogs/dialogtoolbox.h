/************************************************************************
 **
 **  @file   dialogtoolbox.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 1, 2019
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2019 Valentina project
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
#ifndef DIALOGTOOLBOX_H
#define DIALOGTOOLBOX_H

#include <chrono>
#include <QComboBox>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../ifc/ifcdef.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vmisc/typedef.h"
#include "../vpatterndb/variables/vinternalvariable.h"

class QPlainTextEdit;
class QPushButton;
class QDialog;
class QObject;
class QEvent;
class QLabel;
class QWidget;
class QColor;
class QLineEdit;
class VContainer;
class QListWidget;
class VPieceNode;
class QDialogButtonBox;
class VAbstractPattern;
namespace VPE
{
class QtColorPicker;
}

constexpr std::chrono::milliseconds formulaTimerTimeout = std::chrono::milliseconds(300);

extern const QColor errorColor;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Weffc++")

struct FormulaData
{
    QString formula;
    const QHash<QString, QSharedPointer<VInternalVariable>> *variables{nullptr};
    /** @brief The container of the pattern, where the tool that owns the field stands.
     *
     *  Not needed to compute a formula — @a variables is what the calculator reads — but needed to judge the
     *  names it uses: whether a line was given one is held by the container's role-name table, not by the
     *  variable. A caller that has the container passes it, and the field then refuses a name nobody gave; a
     *  caller that has only a table computes exactly as before. */
    const VContainer *container{nullptr};
    /** @brief The caption beside the formula field, when the caller has one to colour.
     *
     *  Optional, and null for a caller whose caption is a plain name: a name is not in error, so a colour
     *  there says nothing. The result label then carries the state on its own. */
    QLabel *labelEditFormula{nullptr};
    QLabel *labelResult{nullptr};
    QString postfix{};
    bool checkZero{false};
    bool checkLessThanZero{false};
};

QT_WARNING_POP

auto RowNode(QListWidget *listWidget, int i) -> VPieceNode;
auto NodeRowIndex(QListWidget *listWidget, quint32 id) -> int;
void MoveCursorToEnd(QPlainTextEdit *plainTextEdit);
void DeployFormula(QDialog *dialog, QPlainTextEdit *formula, QPushButton *buttonGrowLength, int formulaBaseHeight);

/**
 * @brief InitFormulaGrowButton keeps the icon of a formula expand/collapse button readable while the button
 * is hovered. A hovered button gets the accent background, which is dark in both themes (#8C4560 light /
 * #983E60 dark), where the icon of the active theme disappears — #2b2b2b on #8C4560 in light theme. The icon
 * is therefore repainted in Gray200 #e5e5e5, the colour of the Dark set, as long as the cursor stays on the
 * button, whatever icon the dialog set (Icons.md §2, "règle fond accent"). Safe to call more than once.
 */
void InitFormulaGrowButton(QPushButton *button);
auto FilterObject(QObject *object, QEvent *event) -> bool;
auto ToInternalNotation(const QString &formula) -> QString;
auto EvalToolFormula(QDialog *dialog, const FormulaData &data, bool &flag) -> qreal;

/** @brief The reason to show when a formula names an object the user never named.
 *
 *  It deliberately names no object: `Line_C7_C8` is a pair of points, and showing it back would hand the user
 *  the very habit the rule exists to break. Meant for the two places a formula is judged: the field itself,
 *  through EvalToolFormula, and the write, through VAbstractTool::CheckFormula. The token and the table stay
 *  in the signature so the message can gain an object name the day objects stopped being named after points. */
auto UnnamedObjectMessage(const QString &token, const VContainer *container,
                          const QHash<QString, QSharedPointer<VInternalVariable>> *variables) -> QString;
void ChangeColor(QWidget *widget, const QColor &color);
auto OkColor(const QWidget *widget) -> QColor;
auto GetColorDotPixmap(const QColor &color, int diameter = 10) -> QPixmap;
void CheckPointLabel(QDialog *dialog, QLineEdit *edit, QLabel *labelEditNamePoint, const QString &pointName,
                     const VContainer *data, bool &flag);
auto AddShowPointNameCheckbox(QFormLayout *layout) -> QCheckBox *;
auto FindNotExcludedNeighborNodeDown(QListWidget *listWidget, int candidate) -> int;
auto FindNotExcludedNeighborNodeUp(QListWidget *listWidget, int candidate) -> int;
auto FindNotExcludedNodeDown(QListWidget *listWidget, int candidate) -> int;
auto FindNotExcludedNodeUp(QListWidget *listWidget, int candidate) -> int;
auto FindNotExcludedPointDown(QListWidget *listWidget, int start) -> int;
auto FindNotExcludedCurveDown(QListWidget *listWidget, int start) -> int;
auto FirstPointEqualLast(QListWidget *listWidget, const VContainer *data, QString &error) -> bool;
auto DoublePoints(QListWidget *listWidget, const VContainer *data, QString &error) -> bool;
auto DoubleCurves(QListWidget *listWidget, const VContainer *data, QString &error) -> bool;
auto EachPointLabelIsUnique(QListWidget *listWidget) -> bool;
auto InvalidSegment(QListWidget *listWidget, const VContainer *data, QString &error) -> bool;
auto DialogWarningIcon(const QWidget *w) -> QString;
auto NodeFont(QFont font, bool nodeExcluded = false) -> QFont;
void CurrentCurveLength(vidtype curveId, VContainer *data);
void SetTabStopDistance(QPlainTextEdit *edit, int tabWidthChar = 4);
auto SegmentAliases(GOType curveType, const QString &alias1, const QString &alias2) -> QPair<QString, QString>;
auto SegmentNames(GOType curveType, const QString &name1, const QString &name2) -> QPair<QString, QString>;
auto GetNodeName(const VContainer *data, const VPieceNode &node, bool showPassmarkDetails = false) -> QString;
auto MirrorLinePointsNeighbors(QListWidget *listWidget, quint32 startPoint, quint32 endPoint) -> bool;
void InitColorPicker(VPE::QtColorPicker *button);
void InitOperationColorPicker(VPE::QtColorPicker *button);

// QComboBox-based color selection (fixed 10-color Ostara palette).
void InitColorCombo(QComboBox *combo);
void SetColorComboCurrentName(QComboBox *combo, const QString &colorName);
auto GetColorComboCurrentName(const QComboBox *combo) -> QString;
auto ReplaceColorPickerWithCombo(QWidget *parent, VPE::QtColorPicker *oldPicker) -> QComboBox *;
void FillComboBoxTypeLine(QComboBox *box, const QMap<QString, QIcon> &stylesPics, const QString &def = TypeLineLine);
void ChangeCurrentData(QComboBox *box, const QVariant &value);
auto GetComboBoxCurrentData(const QComboBox *box, const QString &def) -> QString;
auto GenerateUniqueCurveName(const VContainer *data,
                             const QString &typeHead,
                             const QString &base,
                             const QString &pointName = QString()) -> QString;
auto GenerateDefSubCurveName(const VContainer *data,
                             quint32 curveId,
                             const QString &derBase,
                             const QString &base,
                             const QString &pointName = QString()) -> QString;
auto GenerateDefOffsetCurveName(const VContainer *data, quint32 curveId, const QString &derBase, const QString &base)
    -> QString;
void MakeFilterableComboBox(QComboBox *combo, Qt::CaseSensitivity cs = Qt::CaseInsensitive);

//---------------------------------------------------------------------------------------------------------------------
template<typename T>
inline void FillList(QComboBox *box, const QMap<QString, T> &list)
{
    SCASSERT(box != nullptr)
    box->clear();
    QMapIterator iter(list);
    while (iter.hasNext())
    {
        iter.next();
        box->addItem(iter.key(), QVariant::fromValue(iter.value()));
    }
}

#endif // DIALOGTOOLBOX_H
