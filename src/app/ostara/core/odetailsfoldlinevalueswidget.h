/************************************************************************
 **
 **  @file   odetailsfoldlinevalueswidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/09/2026
 **
 **  @brief  Width / Height / Center rows of the fold-line Property Browser pages
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

#ifndef ODETAILSFOLDLINEVALUESWIDGET_H
#define ODETAILSFOLDLINEVALUESWIDGET_H

#include "../vmisc/def.h"
#include "../vpatterndb/vpiece.h"

#include <QWidget>

#include <functional>

class QGridLayout;
class QLabel;
class QLineEdit;
class VContainer;
class VFoldLine;

// The three editable fold-line dimensions, one row each: a label, a field showing the value in
// effect, and a button opening the formula editor. Clearing the field, or typing 0, puts that
// dimension back on its automatic value.
//
// The fields are the frontend, and its naming is deliberately the other way round from the engine:
// a fold line almost always runs along a 90° line, so the size the user sees across the marker is
// the engine's fold height, and the length they see along the fold is the engine's fold width. The
// inversion is written down once, in the value table of the .cpp.
//
// Visibility is the caller's business: this widget shows values, it does not decide whether the
// fold line is defined or which type is selected.
class ODetailsFoldLineValuesWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsFoldLineValuesWidget(QWidget *parent = nullptr);
    ~ODetailsFoldLineValuesWidget() override = default;

    void Show(const VPiece &det, const VContainer *data, quint32 pieceId);
    void Clear();

    void SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify);

    void RetranslateUI();

private:
    static constexpr int kFieldCount = 3;

    // Field indexes, in presentation order. They index the value table of the .cpp.
    static constexpr int kWidthField = 0;
    static constexpr int kHeightField = 1;
    static constexpr int kCenterField = 2;

    void ConnectControls();
    void FillField(int field, const VPiece &det, const VFoldLine &fLine, const Unit &unit);
    void CommitFormula(int field);
    void OpenFormulaEditor(int field, QLineEdit *edit);
    void RestoreField(int field);

    QGridLayout *m_grid{nullptr};
    QLabel *m_labels[kFieldCount]{};
    QLineEdit *m_edits[kFieldCount]{};
    QLabel *m_hintLabel{nullptr};

    const VContainer *m_data{nullptr};
    quint32 m_pieceId{0};
    bool m_loading{false};

    std::function<void(std::function<void(VPiece &)>)> m_modifyPending;

    Q_DISABLE_COPY_MOVE(ODetailsFoldLineValuesWidget) // NOLINT
};

#endif // ODETAILSFOLDLINEVALUESWIDGET_H
