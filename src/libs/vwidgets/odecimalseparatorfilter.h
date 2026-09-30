/************************************************************************
 **
 **  @file   odecimalseparatorfilter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/09/2026
 **
 **  @brief  Global event filter: a typed decimal comma (',') becomes '.' in
 **          every QLineEdit / QTextEdit / QPlainTextEdit, so that "0,3" is
 **          accepted and stored as "0.3" in any numeric field of the app.
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

#ifndef ODECIMALSEPARATORFILTER_H
#define ODECIMALSEPARATORFILTER_H

#include <QObject>

class QWidget;

// Converts a typed decimal comma into a period at the key-press level, but only
// when the comma is in a numeric context (it directly follows a digit), i.e. in
// numeric/formula fields. A ',' typed in plain text (names, notes…) is preserved.
// The parser never sees a ',' so inputs like "0,3" are valid everywhere and are
// stored canonically as "0.3".
//
// Install once on the application:
//   ODecimalSeparatorFilter::InstallOnApplication();
class ODecimalSeparatorFilter : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit ODecimalSeparatorFilter(QObject *parent = nullptr);
    ~ODecimalSeparatorFilter() override = default;

    static void InstallOnApplication();

    /** @brief MarkCommaAsSeparator declare that @a widget holds a comma-separated list, not a number.
     *
     * Fields like a size axis' values ("152,158,164") or a group's tags ("avant, après") are lists: there a
     * comma is a separator and must survive as it was typed. Everywhere else the comma stays a decimal
     * separator and is rewritten to a period. Does nothing when @a widget is null. */
    static void MarkCommaAsSeparator(QWidget *widget);

protected:
    auto eventFilter(QObject *o, QEvent *e) -> bool override;

private:
    Q_DISABLE_COPY_MOVE(ODecimalSeparatorFilter) // NOLINT
};

#endif // ODECIMALSEPARATORFILTER_H
