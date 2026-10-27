/************************************************************************
 **
 **  @file   owaitdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Modal "please wait" dialog: the message, and — when asked — a bar and the seconds.
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

#ifndef OWAITDIALOG_H
#define OWAITDIALOG_H

#include <QDialog>
#include <QElapsedTimer>
#include <QTimer>

class QLabel;

/**
 * @brief Application-modal wait dialog: a message, and — on request — a progress bar and the
 *        seconds going by.
 *
 *        No buttons — the caller closes/destroys it once the blocking work is done, and the cross
 *        is the one way out: closing it is how a run is given up.
 *
 * ⚠️ **The seconds only move while an event loop runs.** The clock is a `QTimer` on this thread, so
 * the work it counts has to be somewhere else — a worker thread — with the waiting done in a nested
 * loop over here. A caller that blocks this thread gives a frozen "0.0 s", not a live one.
 */
class OWaitDialog : public QDialog
{
    Q_OBJECT

public:
    /// What the dialog shows besides the message.
    enum class Presentation
    {
        Message, ///< the message alone — a wait short enough that nothing has to move
        Timed,   ///< a bar and the seconds — what a run that takes minutes shows
    };

    explicit OWaitDialog(const QString &text, QWidget *parent = nullptr,
                         Presentation presentation = Presentation::Message);

private:
    QLabel *m_elapsed = nullptr;
    QElapsedTimer m_clock;
};

#endif // OWAITDIALOG_H
