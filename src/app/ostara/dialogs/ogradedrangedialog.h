/************************************************************************
 **
 **  @file   ogradedrangedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Graded-mode entry dialog (2 steps): choose the graded axis (step 1)
 **          then build the size range (name, min/max, frozen values of the other
 **          axes) (step 2). Step 1 is skipped when the axis is already declared.
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

#ifndef OGRADEDRANGEDIALOG_H
#define OGRADEDRANGEDIALOG_H

#include <QDialog>
#include <QMap>
#include <QPair>
#include <QStringList>
#include <QVector>

#include "../vpatterndb/osizerange.h"

class QComboBox;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QWidget;

/**
 * @brief Creates one size range (definition only — the content is added by the
 *        graded canvas). Every combo is built with MakeComboBox().
 */
class OGradedRangeDialog : public QDialog
{
    Q_OBJECT

public:
    struct Config
    {
        QString canonicalPath;                 // measurement link (stored in the range)
        QStringList axisNames;                 // all table axes, in order
        QMap<QString, QStringList> axisValues; // axis -> values (CSV order)
        bool axisStepNeeded = false;           // step 1 (choose the size axis) shown
        QString declaredAxis;                  // axis to use when step 1 is skipped
        QStringList takenNames;                // range names already used in the pattern
    };

    explicit OGradedRangeDialog(const Config &config, QWidget *parent = nullptr);

    /// The graded axis chosen (from step 1, or the declared axis when skipped).
    auto sizeAxis() const -> QString;

    /// The size range definition built from the current page state.
    auto range() const -> OSizerange;

private:
    void BuildStep1Page();
    void BuildStep2Page();
    void GoToStep2();
    void UpdateButtons();
    void CheckState();

    Config m_config;
    QString m_sizeAxis;

    QStackedWidget *m_stack = nullptr;
    QWidget *m_step1 = nullptr;
    QWidget *m_step2 = nullptr;

    QComboBox *m_axisCombo = nullptr; // step 1: size axis
    QComboBox *m_minCombo = nullptr;  // step 2
    QComboBox *m_maxCombo = nullptr;  // step 2
    QLineEdit *m_nameEdit = nullptr;  // step 2

    QPushButton *m_nextButton = nullptr;
    QPushButton *m_okButton = nullptr;

    QVector<QPair<QString, QComboBox *>> m_otherCombos; // other axes rows of step 2
};

#endif // OGRADEDRANGEDIALOG_H
