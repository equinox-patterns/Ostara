/************************************************************************
 **
 **  @file   ogratedsizestylesdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  "Edit size styles…" dialog: one row per value of the graded size
 **          axis. Each row edits the trace style of that size — optional layer
 **          name, hex color (+ live swatch) and a pt dash pattern (up to three
 **          "Dash / Space" pairs, fields empty = not set). Cancel writes
 **          nothing; OK returns the styles through styles().
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

#ifndef OGRADEDSIZESTYLESDIALOG_H
#define OGRADEDSIZESTYLESDIALOG_H

#include <QDialog>
#include <QHash>
#include <QStringList>
#include <QVector>

#include "../core/ogratedsizestyles.h"

class QLabel;
class QLineEdit;

/**
 * @brief The OGradedSizeStylesDialog class edits the trace styles of every value
 *        of the current size axis. Rows are built in code inside a QScrollArea
 *        (heterogeneous, wide rows — not a QTableWidget with setCellWidget).
 *        The dialog holds no pattern data: MainWindow feeds it the axis values
 *        and the stored styles, and collects the result through styles().
 */
class OGradedSizeStylesDialog final : public QDialog
{
    Q_OBJECT

public:
    OGradedSizeStylesDialog(const QStringList &sizeValues, const QHash<QString, OGradedSizeStyle> &styles,
                            QWidget *parent = nullptr);

    /** @brief The styles edited by the user, keyed by size-value label. A value
     *         whose row carries nothing set has no entry. */
    auto styles() const -> QHash<QString, OGradedSizeStyle>;

private:
    struct Row
    {
        QString value;
        QLineEdit *layerName{nullptr};
        QLineEdit *color{nullptr};
        QLabel *swatch{nullptr};
        QLineEdit *dash[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    };

    /** @brief Validate every row; on error sets @p message and returns false. */
    auto Validate(QString *message) const -> bool;

    QVector<Row> m_rows;
};

#endif // OGRADEDSIZESTYLESDIALOG_H
