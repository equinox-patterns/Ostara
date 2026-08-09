/************************************************************************
 **
 **  @file   TopBar.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/08/2026
 **
 **  @brief  Unified fixed top bar replacing individual QToolBars.
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
#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>

class QAction;
class QComboBox;
class QHBoxLayout;
class QLabel;
class QToolButton;

class TopBar : public QWidget
{
    Q_OBJECT

public:
    explicit TopBar(QWidget *parent = nullptr);

    void SetModeActions(QAction *draw, QAction *details, QAction *layout);
    void SetPatternPieceWidgets(QLabel *pieceLabel, QComboBox *combo, QAction *newPieceAction);
    void SetMeasurementWidgets(QAction *loadAction, QLabel *fileNameLabel,
                               QLabel *separator, QAction *tableAction);
    void SetZoomWidget(QWidget *zoomBlock);

    /// Reload themed SVG icons (called on theme change).
    void ReloadIcons();

    /// Remove all dimension drop-downs inserted in the center zone.
    void ClearDimensionWidgets();

    /// Insert a dimension combo / label pair before the table button.
    /// @param label  dimension name label (e.g. "A:")
    /// @param combo  dimension gradation combo
    void InsertDimensionWidgets(QLabel *label, QComboBox *combo);

    /// Show/hide the dimension separator before the first dim combo.
    void SetDimensionSeparatorVisible(bool visible);

    /// Show/hide the pattern piece section (label + combo + new piece button + separator).
    void SetPatternPieceVisible(bool visible);

private:
    QToolButton *MakeModeButton(QAction *action);
    void         UpdateModeButtonIcons();

    // --- Layout -----------------------------------------------------------
    QHBoxLayout *m_mainLayout   = nullptr;
    QHBoxLayout *m_leftLayout   = nullptr;
    QHBoxLayout *m_centerLayout = nullptr;
    QHBoxLayout *m_rightLayout  = nullptr;

    // --- Left buttons -----------------------------------------------------
    QToolButton *m_btnDraw    = nullptr;
    QToolButton *m_btnDetails = nullptr;
    QToolButton *m_btnLayout  = nullptr;
    QLabel *m_sepDrawDetails   = nullptr;
    QLabel *m_sepDetailsLayout = nullptr;

    // --- Center: pattern piece --------------------------------------------
    QComboBox   *m_comboDraws         = nullptr;
    QLabel      *m_pieceLabel         = nullptr;
    QToolButton *m_btnNewPiece        = nullptr;
    QLabel      *m_sepPatternMeasure  = nullptr; // line-vertical between new piece and ruler

    // --- Center: measurement ----------------------------------------------
    QLabel *m_measurementFileNameLabel = nullptr;
    QLabel *m_dimSeparator = nullptr; // line-vertical before first dim combo (multisize only)
};

#endif // TOPBAR_H
