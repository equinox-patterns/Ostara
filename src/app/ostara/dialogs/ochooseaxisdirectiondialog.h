/************************************************************************
 **
 **  @file   ochooseaxisdirectiondialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The four directions a curve handle can take from its axis, as a choice.
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

#ifndef OCHOOSEAXISDIRECTIONDIALOG_H
#define OCHOOSEAXISDIRECTIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QVector>

#include "../vgeometry/ocurveaxis.h"

class QRadioButton;

/**
 * @brief The direction a handle takes from its axis, chosen among four.
 *
 * No .ui file: it is a sentence and four lines of text, and the layout is the whole of it — the model is
 * OChooseLayerDialog. The window previews as it goes: every change of direction is announced, so the caller
 * can show the curve it produces before anything is written.
 */
class OChooseAxisDirectionDialog final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    struct Config
    {
        QString intro{};      // the sentence above the choices, translated by the caller; may be empty
        QString acceptText{}; // the action's own word — "OK" when the caller has none
        CurveAxisSense selected{CurveAxisSense::Line};
    };

    explicit OChooseAxisDirectionDialog(const Config &config, QWidget *parent = nullptr);
    ~OChooseAxisDirectionDialog() override = default;

    /** @brief The direction the user settled on. */
    auto ChosenSense() const -> CurveAxisSense;

signals:
    /** @brief The direction the user is looking at, before any validation. Not emitted when the click only
     *  re-checks the direction already in force: nothing changed, so there is nothing to preview. */
    void SenseChanged(CurveAxisSense sense);

private:
    Q_DISABLE_COPY_MOVE(OChooseAxisDirectionDialog) // NOLINT

    QVector<QRadioButton *> m_buttons{};
    CurveAxisSense m_selected{CurveAxisSense::Line};
};

/** @brief The name of a direction, as the radio buttons say it and as the Options dock repeats it. */
auto CurveAxisSenseText(CurveAxisSense sense) -> QString;

#endif // OCHOOSEAXISDIRECTIONDIALOG_H
