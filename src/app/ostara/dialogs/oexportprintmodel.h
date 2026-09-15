/************************************************************************
 **
 **  @file   oexportprintmodel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The print models of the Export mode — the sheet the nesting is
 **          laid out on. A table of data, and nothing else.
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

#ifndef OEXPORTPRINTMODEL_H
#define OEXPORTPRINTMODEL_H

#include <QString>
#include <QVector>

/// @brief The two shapes a model can take. What the widget shows follows from this,
///        and nothing else does.
enum class OExportPrintShape : qint8
{
    /// One continuous sheet: a fixed width, and a height the pattern decides. Nothing
    /// is assembled, so there is no sheet height to set and no column to multiply.
    Roll,

    /// Sheets of a fixed size, cut out of the plan one beside the other. The sheet's
    /// small side, times the column count, is the width the nesting works on.
    Tiled
};

/**
 * @brief One printable output: the sheet the nesting is laid out on.
 *
 * The six models differ by what they let the user set, and by nothing else — which is
 * why they are a table rather than six classes. What the table does NOT carry is a
 * behaviour: there is no pointer, no virtual, no callback. A model that needs its own
 * code gets its own shape class, and this file stays a list of values.
 *
 * The dimensions are in millimetres, always. They are the one fixed point of the unit
 * combo: millimetres go in, whatever unit the user reads comes out, and converting the
 * display never touches what is stored.
 */
struct OExportPrintModel
{
    const char *label{nullptr}; ///< what the combo shows — translatable
    const char *token{nullptr}; ///< what the generated name carries — never translated

    OExportPrintShape shape{OExportPrintShape::Tiled};

    qreal widthMm{0.0};  ///< the sheet's small side — the one the columns multiply
    qreal heightMm{0.0}; ///< the sheet's long side; irrelevant to a Roll

    /// Whether the two dimensions can be typed. True for the framed model alone: it is
    /// the one whose whole point is that its useful area is not a standard sheet, so the
    /// user sizes the frame. Everywhere else the dimensions ARE the standard, and typing
    /// them would only let the user build a sheet that does not exist.
    bool dimsEditable{false};

    /// Whether the model offers a column count. False for the A0 sheet, which is not
    /// assembled after printing: its sheets are whole pages stacked, never placed side
    /// by side, so a column count would have nothing to mean.
    bool hasColumns{true};

    /// Whether the combo lets the model be picked. False for the A0 sheet, whose nesting
    /// packs closed rectangles rather than a strip — engine work that is not done yet.
    /// The row stays visible: a model that silently vanished would read as a feature that
    /// was never planned.
    bool selectable{true};
};

/// @brief Every model, in the order the combo shows them — largest sheet first.
auto OExportPrintModels() -> const QVector<OExportPrintModel> &;

/// @brief The model carrying @p token, or nullptr. Used to replay a generation, which
///        remembers the token and nothing else about how it was printed.
auto FindPrintModel(const QString &token) -> const OExportPrintModel *;

#endif // OEXPORTPRINTMODEL_H
