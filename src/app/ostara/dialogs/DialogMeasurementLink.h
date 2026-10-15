/************************************************************************
 **
 **  @file   DialogMeasurementLink.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Dialog asking where to take the measurement table a pattern expects, when the table it
 **          points at cannot be used — the recorded file is gone, or none was ever recorded.
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

#ifndef DIALOGMEASUREMENTLINK_H
#define DIALOGMEASUREMENTLINK_H

#include <QDialog>
#include <QString>

/// The counterpart of the "legacy measurement file" offer, for the case where there is nothing to convert yet.
///
/// When a pattern is linked to a Tape table that still exists, the reader is offered the conversion of that file.
/// When the link cannot be used — the file was moved or deleted, or the pattern carries no link at all — that offer
/// has nothing to work with, and Valentina's own answer ("do you want to update the file location?" plus a raw file
/// picker) ignores the Ostara library, which is where the reader keeps her tables.
///
/// This dialog only asks, and opens nothing itself: the caller owns the gestures that follow — the file search and
/// `DialogOpenMeasurements` — so that the class stays a question and nothing more.
class DialogMeasurementLink : public QDialog
{
    Q_OBJECT

public:
    enum class Choice
    {
        SearchFile,  ///< Look for the table on the computer.
        LibraryTable ///< Take a table from the Ostara library (or browse from there).
    };

    /// @param recordedPath the table the pattern points at, absolute; empty when the pattern carries no link at all.
    explicit DialogMeasurementLink(const QString &recordedPath, QWidget *parent = nullptr);
    ~DialogMeasurementLink() override = default;

    /// Meaningful only when exec() returned QDialog::Accepted.
    auto SelectedChoice() const -> Choice;

private:
    Choice m_choice{Choice::SearchFile};
};

#endif // DIALOGMEASUREMENTLINK_H
