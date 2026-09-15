/************************************************************************
 **
 **  @file   oexporttiledformatwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The Tiled shape of the print tab — sheets of a fixed size, and the
 **          fields that go with them.
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

#ifndef OEXPORTTILEDFORMATWIDGET_H
#define OEXPORTTILEDFORMATWIDGET_H

#include <QWidget>

#include "../vmisc/def.h"

class QLabel;
class QLineEdit;
class QSpinBox;

/**
 * @brief The fields of a Tiled model: the sheet, and how many of them side by side.
 *
 * The tab shows the model's fields, never the nesting's. The nesting knows one width and
 * nothing of sheets — so what this widget collects is the SHEET (its two sides) and the
 * column count, and the width the nesting runs on is their product. Keeping the two apart
 * is what lets the same plan be cut into A4 or into Letter without nesting it twice.
 *
 * The dimensions are read-only except on the framed model, the only one that is not a
 * standard: everywhere else they ARE the standard, and letting them be typed would let the
 * user build a sheet that does not exist and that no printer has.
 *
 * All sides are portrait. The small side is the one the columns multiply, so the width the
 * nesting works on is the sum of the sheets placed side by side, never of their long sides.
 */
class OExportTiledFormatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportTiledFormatWidget(QWidget *parent = nullptr);

    /// @brief Adopt the sheet of a model, and reset the column count to its default.
    /// @param widthMm    the sheet's small side, in millimetres
    /// @param heightMm   the sheet's long side, in millimetres
    /// @param editable   whether the user may type the two sides — the framed model alone
    /// @param hasColumns whether a column count applies at all — false for the A0 sheet,
    ///                   which is never assembled and so is never placed side by side
    void SetSheet(qreal widthMm, qreal heightMm, bool editable, bool hasColumns);

    auto SheetWidthMm() const -> qreal;
    auto SheetHeightMm() const -> qreal;

    /// @brief How many sheets side by side the plan is laid out on. At least 1.
    auto Columns() const -> int;

    /// @brief Show every length in @p unit from now on.
    void SetUnit(const Unit &unit);

signals:
    /// Emitted when a field the user can act on changed — the column count, or an edited
    /// side. The tab recomputes the document width from it.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    void Refresh();
    void Retranslate();
    void CommitEditedSides();

    QLabel *m_widthLabel{nullptr};
    QLineEdit *m_widthEdit{nullptr};
    QLabel *m_heightLabel{nullptr};
    QLineEdit *m_heightEdit{nullptr};
    QLabel *m_columnsLabel{nullptr};
    QSpinBox *m_columns{nullptr};

    qreal m_widthMm{0.0};
    qreal m_heightMm{0.0};
    Unit m_unit{Unit::Cm};

    /// Whether the model offers a column count. Held as data, never read back off the
    /// spin box's visibility: a widget reports itself invisible until the whole window is
    /// shown, so a width computed during construction would silently come out as one
    /// column's worth — the sheet alone, and a plan too narrow to nest anything on.
    bool m_hasColumns{true};
};

#endif // OEXPORTTILEDFORMATWIDGET_H
