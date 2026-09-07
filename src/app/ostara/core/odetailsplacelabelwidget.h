/************************************************************************
 **
 **  @file   odetailsplacelabelwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Place label ("internal mark") property page of the Details dock
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

#ifndef ODETAILSPLACELABELWIDGET_H
#define ODETAILSPLACELABELWIDGET_H

#include <QWidget>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QToolButton;
class VPlaceLabelItem;
class VToolSeamAllowance;

/**
 * @brief The ODetailsPlaceLabelWidget class property page of the clicked place label
 * (an "internal mark" — "repère interne" in French) in the Details options dock.
 *
 * Styled after the pattern information page (OPatternInfoWidget): a QGroupBox titled with the
 * feature, then the fields, in order:
 *   1.  Width        — one line: label + real QLineEdit (raw formula) + fx button
 *   2.  Height       — idem
 *   3.  Angle        — idem
 *   4.  Type         — MakeComboBox (same content as the creation dialog)
 *   — separator —
 *   5.  Visible      — label + QLineEdit + fx, with the caption "1 = visible, 0 = invisible, …"
 *   6.  Not mirrored — checkbox (unchecked by default)
 *
 * Every field applies live (focus-out / combo change / checkbox toggle) through a mergeable
 * SavePlaceLabelOptions command, so the canvas updates in place and the change is one undo step.
 */
class ODetailsPlaceLabelWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsPlaceLabelWidget(QWidget *parent = nullptr);

    /** Sets the label context (owning piece + label id) and reloads the fields. */
    void Show(VToolSeamAllowance *piece, quint32 labelId);

    void RetranslateUI();

private:
    enum class Field
    {
        Width,
        Height,
        Angle,
        Visible
    };

    void BuildUi();
    void LoadFromLabel();
    auto FetchCurrentLabel(VPlaceLabelItem &out) const -> bool;
    void CommitFormula(Field field);
    void CommitType(int index);
    void CommitNotMirrored(bool checked);
    void OpenFormulaEditor(Field field, QLineEdit *edit);
    void PushLabelChange(const VPlaceLabelItem &current, const VPlaceLabelItem &updated);

    QGroupBox *m_groupBox{nullptr};
    QGridLayout *m_grid{nullptr};
    QLabel *m_widthLabel{nullptr};
    QLabel *m_heightLabel{nullptr};
    QLabel *m_angleLabel{nullptr};
    QLabel *m_typeLabel{nullptr};
    QLabel *m_visibleLabel{nullptr};
    QLabel *m_visibleHint{nullptr};
    QLineEdit *m_widthEdit{nullptr};
    QLineEdit *m_heightEdit{nullptr};
    QLineEdit *m_angleEdit{nullptr};
    QLineEdit *m_visibleEdit{nullptr};
    QComboBox *m_typeCombo{nullptr};
    QCheckBox *m_notMirroredCheck{nullptr};

    VToolSeamAllowance *m_piece{nullptr};
    quint32 m_labelId{0};
    bool m_loading{false};

    Q_DISABLE_COPY_MOVE(ODetailsPlaceLabelWidget) // NOLINT
};

#endif // ODETAILSPLACELABELWIDGET_H
