/************************************************************************
 **
 **  @file   opieceinfowidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   30/08/2026
 **
 **  @brief  Piece information fields for the Details dock General tab:
 **          number (with dynamic piece total), cut (fabric / interfacing /
 **          lining) and free text.
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

#ifndef OPIECEINFOWIDGET_H
#define OPIECEINFOWIDGET_H

#include <QWidget>

#include <functional>

#include "../vpatterndb/vpiece.h"

class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QSpinBox;
class QTabWidget;
class QTimer;
class VContainer;

/**
 * @brief Piece information fields for the Details dock General tab.
 *
 * Mutates VPiece through the same snapshot + undo pipeline as the rest of the
 * dock: the parent provides a ModifyPending callback via SetModifyPending().
 */
class OPieceInfoWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OPieceInfoWidget(QWidget *parent = nullptr);
    ~OPieceInfoWidget() override = default;

    /** Callback that applies a VPiece mutation and commits it to the undo stack. */
    void SetModifyPending(std::function<void(std::function<void(VPiece &)>)> callback);

    void Show(const VPiece &detail, const VContainer *data);
    void Clear();
    void RetranslateUI();

private:
    void InitCutTab(int index, const QString &title, QSpinBox **qtySpin, QCheckBox **onFoldCheck);

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OPieceInfoWidget) // NOLINT

    std::function<void(std::function<void(VPiece &)>)> m_modifyPending;

    QSpinBox *m_numberSpin{nullptr};
    QLabel *m_numberLabel{nullptr};
    QLabel *m_totalLabel{nullptr};

    QTabWidget *m_cutTabs{nullptr};
    QLabel *m_quantityLabels[3]{};
    QSpinBox *m_fabricQtySpin{nullptr};
    QSpinBox *m_interfacingQtySpin{nullptr};
    QSpinBox *m_liningQtySpin{nullptr};
    QCheckBox *m_fabricOnFoldCheck{nullptr};
    QCheckBox *m_interfacingOnFoldCheck{nullptr};
    QCheckBox *m_liningOnFoldCheck{nullptr};

    QLabel *m_freeTextLabel{nullptr};
    QPlainTextEdit *m_freeTextEdit{nullptr};
    QTimer *m_freeTextTimer{nullptr};
    bool m_loading{false};
};

#endif // OPIECEINFOWIDGET_H
