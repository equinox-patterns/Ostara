/************************************************************************
 **
 **  @file   odetailsnodewidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/09/2026
 **
 **  @brief  Property browser page for a piece contour (main path) node in the
 **          Details dock — passmark and seam allowance angle options of the point.
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

#ifndef ODETAILSNODEWIDGET_H
#define ODETAILSNODEWIDGET_H

#include <QVector>
#include <QWidget>

#include <functional>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QToolButton;
class VPieceNode;
class VPassmark;
class VToolSeamAllowance;

/**
 * @brief Contextual property browser of a clicked piece contour (main path) node.
 *
 * Two sections, mirroring the layout of the place-label page
 * (ODetailsPlaceLabelWidget):
 *  - "Passmark" — line type, angle rule, "show second passmark on seam line", and the three notch
 *    dimensions (Length, Width, Angle) as editable formulas, then the visibility formula;
 *  - "Seam allowance angle" — the seam allowance angle (squaring) at the point.
 *
 * Each of the three dimension rows shows the value the engine would draw when its field is left
 * empty — emptying a field therefore restores the automatic value, with nothing to untick first.
 *
 * Every change is pushed to the undo stack through SavePieceOptions (whole piece
 * snapshot) so the piece geometry refreshes live, exactly like the other
 * per-node edits of the Details dock.
 */
class ODetailsNodeWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsNodeWidget(QWidget *parent = nullptr);

    /** @brief Sets the node context (owning piece + node id) and reloads the fields.
     *         Returns false if the node is not a contour (main path) node of the piece. */
    auto Show(VToolSeamAllowance *piece, quint32 nodeId) -> bool;

    void RetranslateUI();

private:
    enum class FormulaField
    {
        Length,
        Width,
        Angle,
        Visibility
    };

    void PopulateCombos();
    auto LoadFromNode() -> bool;
    auto FetchCurrentNode(VPieceNode &out) const -> bool;
    /** @brief The engine's own description of this node's notch, built the way the canvas builds it. */
    auto FetchPassmark(VPassmark &out) const -> bool;
    auto EditFor(FormulaField field) const -> QLineEdit *;
    /** @brief Re-reads the page from the model after a commit — nothing else refreshes this page. */
    void ReloadAfterCommit();
    /** @brief Re-reads the fx icons for the current theme; the dock survives a light↔dark toggle. */
    void RefreshFxIcons();
    /** @brief The value the engine would draw for one dimension, or 0 when it has none. */
    auto AutomaticValue(FormulaField field, const VPassmark &passmark, bool hasPassmark) const -> qreal;
    /** @brief Shows the manual formula, or the engine's automatic value when there is none. */
    void FillValueField(FormulaField field, const QString &formula, qreal automatic, bool isLength, bool force);
    /** @brief Greys what has no object: everything but the type on "None", the width on "One line",
     *         the angle elsewhere than on the "Straightforward" rule. */
    void UpdatePassmarkEnabled(const VPieceNode &node);
    /** @brief Puts the field back to what the piece currently holds, after a refused entry. */
    void RestoreField(FormulaField field);
    /** @brief The formula the model holds for one dimension, empty when it carries none of its own. */
    static auto StoredFormula(const VPieceNode &node, FormulaField field) -> QString;
    /** @brief Writes a dimension's formula and its manual flag — the flag follows the formula. */
    static void SetOverride(VPieceNode &node, FormulaField field, const QString &formula);
    void CommitPassmarkType(int index);
    void CommitPassmarkAngle(int index);
    void CommitShowSecondPassmark(bool checked);
    void CommitNotMirrored(bool checked);
    void CommitFormula(FormulaField field);
    void CommitSeamAllowanceAngle(int index);
    void OpenFormulaEditor(FormulaField field, QLineEdit *edit);
    void PushNodeChange(const std::function<void(VPieceNode &node)> &mutate);

    QGroupBox *m_passmarkGroup{nullptr};
    QGroupBox *m_seamGroup{nullptr};

    QLabel *m_typeLabel{nullptr};
    QLabel *m_angleLabel{nullptr};
    QCheckBox *m_secondPassmarkCheck{nullptr};
    QCheckBox *m_notMirroredCheck{nullptr};

    QLabel *m_lengthLabel{nullptr};
    QLineEdit *m_lengthEdit{nullptr};
    QLabel *m_widthLabel{nullptr};
    QLineEdit *m_widthEdit{nullptr};
    QLabel *m_valueAngleLabel{nullptr};
    QLineEdit *m_valueAngleEdit{nullptr};
    QLabel *m_valueHint{nullptr};

    QLabel *m_visibleLabel{nullptr};
    QLineEdit *m_visibleEdit{nullptr};
    QLabel *m_visibleHint{nullptr};

    QVector<QToolButton *> m_fxButtons; // Theme-dependent icons, re-read by RefreshFxIcons().

    QLabel *m_seamTypeLabel{nullptr};

    QComboBox *m_typeCombo{nullptr};        // PassmarkLineType (+ "None")
    QComboBox *m_angleCombo{nullptr};       // PassmarkAngleType
    QComboBox *m_seamTypeCombo{nullptr};    // PieceNodeAngle (seam allowance squaring)

    VToolSeamAllowance *m_piece{nullptr};
    quint32 m_nodeId{0};
    bool m_loading{false};

    Q_DISABLE_COPY_MOVE(ODetailsNodeWidget) // NOLINT
};

#endif // ODETAILSNODEWIDGET_H
