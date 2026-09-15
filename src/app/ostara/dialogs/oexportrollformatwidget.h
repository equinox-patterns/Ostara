/************************************************************************
 **
 **  @file   oexportrollformatwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The Roll shape of the print tab — one continuous sheet, and the
 **          fields that go with it.
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

#ifndef OEXPORTROLLFORMATWIDGET_H
#define OEXPORTROLLFORMATWIDGET_H

#include <QWidget>

#include "../vmisc/def.h"

class QLabel;
class QLineEdit;

/**
 * @brief The fields of a Roll model: a width, and nothing else.
 *
 * A roll is one continuous sheet: its width is fixed by the standard, and its height is
 * whatever the pattern needs — the nesting produces it, so there is nothing to set and
 * nothing to show. That absence is the whole shape: the two fields a tiled model needs,
 * a height and a column count, would have nothing to mean here.
 *
 * The width is shown rather than left out, read-only: a user who cannot see the width the
 * nesting will run on has no way to tell a plan that came out short from one that was
 * asked for short.
 */
class OExportRollFormatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OExportRollFormatWidget(QWidget *parent = nullptr);

    /// @brief Adopt the width @p widthMm the model imposes, in millimetres.
    void SetWidthMm(qreal widthMm);

    /// @brief The width the nesting runs on, in millimetres.
    auto WidthMm() const -> qreal;

    /// @brief Show the length in @p unit from now on.
    void SetUnit(const Unit &unit);

protected:
    void changeEvent(QEvent *event) override;

private:
    void Refresh();
    void Retranslate();

    QLabel *m_widthLabel{nullptr};
    QLineEdit *m_widthEdit{nullptr};

    qreal m_widthMm{0.0};
    Unit m_unit{Unit::Cm};
};

#endif // OEXPORTROLLFORMATWIDGET_H
