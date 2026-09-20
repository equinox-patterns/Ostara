/************************************************************************
 **
 **  @file   ocoversizerangedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  Choosing the size range a cover page's preview lists.
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

#ifndef OCOVERSIZERANGEDIALOG_H
#define OCOVERSIZERANGEDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QStringList>

#include "core/ocovergradedaxis.h"

class QComboBox;
class QLabel;
class QPushButton;

/**
 * @brief Asks which sizes a cover page should list while it is being designed.
 *
 * **A range, and only a range.** The Graded mode's own dialog also asks for a name, a value for
 * every axis it does not grade, a folder and a scale — everything an output needs. None of that
 * exists here: this range is drawn on a preview and nothing else, so the whole question is *which
 * sizes*, and the answer is the axis that runs and the two ends of it.
 *
 * **It does not read the saved ranges.** One belongs to the Graded mode, was made to be printed,
 * and carries answers — a value for every axis it does not grade, an export folder — that a preview
 * has no use for. Offering it here would be offering to answer questions this dialog is not asking;
 * the sizes a page is designed against are chosen in ten seconds, and choosing them here keeps the
 * two ideas apart.
 */
class OCoverSizeRangeDialog : public QDialog
{
    Q_OBJECT

public:
    struct Config
    {
        QString canonicalPath;                  ///< the loaded table, so the range knows its own.
        QStringList axisNames;                  ///< every axis of the table, in its own order.
        QMap<QString, QStringList> axisValues;  ///< axis -> values, in the table's order.
        QString declaredAxis;                   ///< the table's declared grading axis, if it has one.
        OCoverGradedAxis current;               ///< what the information already holds.
    };

    explicit OCoverSizeRangeDialog(const Config &config, QWidget *parent = nullptr);

    /// The range the user settled on. Meaningful once the dialog has been accepted.
    Q_REQUIRED_RESULT auto Chosen() const -> OCoverGradedAxis;

private:
    void BuildUi();
    void ApplyAxis(const QString &axisName);
    void UpdateState();
    auto CurrentAxis() const -> QString;

    Config m_config;
    OCoverGradedAxis m_chosen;
    /// The axis whose values the two counters hold — a change of axis is what resets them.
    QString m_lastAxis;

    QComboBox *m_axisCombo{nullptr};
    QComboBox *m_minCombo{nullptr};
    QComboBox *m_maxCombo{nullptr};
    QPushButton *m_okButton{nullptr};
};

#endif // OCOVERSIZERANGEDIALOG_H
