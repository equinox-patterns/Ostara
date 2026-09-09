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

#include <QWidget>

#include <functional>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class VPieceNode;
class VToolSeamAllowance;

/**
 * @brief Contextual property browser of a clicked piece contour (main path) node.
 *
 * Two sections, mirroring the layout of the place-label page
 * (ODetailsPlaceLabelWidget):
 *  - "Passmark" — line type, angle type, "show second passmark on seam line",
 *    manual length (reveals a Length row), visibility formula;
 *  - "Seam allowance angle" — the seam allowance angle (squaring) at the point.
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
        PassmarkLength,
        Visibility
    };

    void PopulateCombos();
    void SetManualLengthRowVisible(bool visible);
    auto LoadFromNode() -> bool;
    auto FetchCurrentNode(VPieceNode &out) const -> bool;
    /** @brief Current automatic notch length (pattern units) as it would render for this node. */
    auto AutoLengthInPatternUnit(const VPieceNode &node) const -> qreal;
    void CommitPassmarkType(int index);
    void CommitPassmarkAngle(int index);
    void CommitShowSecondPassmark(bool checked);
    void CommitNotMirrored(bool checked);
    void CommitManualPassmarkLength(bool checked);
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
    QCheckBox *m_manualLengthCheck{nullptr};
    QLabel *m_lengthLabel{nullptr};
    QLineEdit *m_lengthEdit{nullptr};
    QLabel *m_visibleLabel{nullptr};
    QLineEdit *m_visibleEdit{nullptr};
    QLabel *m_visibleHint{nullptr};

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
