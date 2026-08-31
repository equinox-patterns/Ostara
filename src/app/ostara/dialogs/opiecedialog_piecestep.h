/************************************************************************
 **
 **  @file   opiecedialog_piecestep.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Step 2 of the piece tool dialog: piece-level information
 **          (name, number, cut, free text, graded alignment).
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

#ifndef OPIECEDIALOG_PIECESTEP_H
#define OPIECEDIALOG_PIECESTEP_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;
class OGradedAlignWidget;

/**
 * @brief Piece-level fields of the piece tool dialog.
 *
 * The widget only collects values; the host dialog builds the VPiece from them.
 * Piece name is the only required field (auto-generated when left empty).
 */
class OPiecePieceStep final : public QWidget
{
    Q_OBJECT

public:
    explicit OPiecePieceStep(QWidget *parent = nullptr);

    auto GetPieceName() const -> QString;
    void SetPieceNameInvalid(bool invalid);
    auto GetPieceNumber() const -> uint;
    void SetPieceTotal(int count);
    auto GetCutFabricQty() const -> int;
    auto IsCutFabricOnFold() const -> bool;
    auto GetCutInterfacingQty() const -> int;
    auto IsCutInterfacingOnFold() const -> bool;
    auto GetCutLiningQty() const -> int;
    auto IsCutLiningOnFold() const -> bool;
    auto GetFreeText() const -> QString;
    auto GetGradedAlignH() const -> int;
    auto GetGradedAlignV() const -> int;

private:
    void InitCutTab(const QString &title, QSpinBox **qtySpin, QCheckBox **onFoldCheck);

    QLineEdit *m_nameEdit{nullptr};
    QSpinBox *m_numberSpin{nullptr};
    QLabel *m_totalLabel{nullptr};
    QTabWidget *m_cutTabs{nullptr};
    QSpinBox *m_fabricQtySpin{nullptr};
    QCheckBox *m_fabricOnFoldCheck{nullptr};
    QSpinBox *m_interfacingQtySpin{nullptr};
    QCheckBox *m_interfacingOnFoldCheck{nullptr};
    QSpinBox *m_liningQtySpin{nullptr};
    QCheckBox *m_liningOnFoldCheck{nullptr};
    QPlainTextEdit *m_freeTextEdit{nullptr};
    OGradedAlignWidget *m_alignWidget{nullptr};
};

#endif // OPIECEDIALOG_PIECESTEP_H
