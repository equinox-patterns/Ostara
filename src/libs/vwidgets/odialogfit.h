/************************************************************************
 **
 **  @file   odialogfit.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  One rule for every dialog of the application: never taller than
 **          the screen, never stretched inside.
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

#ifndef ODIALOGFIT_H
#define ODIALOGFIT_H

#include <QMargins>
#include <QObject>
#include <QRect>

class QWidget;

/**
 * @brief The screen-fit policy of every dialog, installed once for the whole application.
 *
 * A dialog taller than the usable area does not merely look wrong: its buttons sit at the bottom
 * of the frame, and a frame that runs past the edge takes them with it, out of reach and with no
 * way to scroll there. Nothing in Qt prevents it — a window's opening size is clamped to two
 * thirds of the screen, but never below its layout's minimum, and a layout's minimum is the sum
 * of its rows. The same rule therefore has to hold for every dialog, and it is one rule, not one
 * hundred. That is what this class is: an application-wide event filter, so no dialog has to know
 * about it, and no dialog can be forgotten.
 *
 * <b>Two invariants.</b>
 *
 * 1. <b>Never taller than the usable area.</b> The frame is capped to the screen's
 *    availableGeometry(), and when the content does not fit under that cap, it is the *content*
 *    that scrolls — the bar of buttons stays in the frame, where it can still be clicked.
 *
 * 2. <b>No dead stretch.</b> A form row must not grow past what it asks for. Qt promotes a
 *    widget that owns a layout to "expanding" even when its own size policy says otherwise
 *    (QWidgetItem::expandingDirections), and a QFormLayout then hands the whole surplus to that
 *    row and drops its trailing stretch — which is why a row can swallow a hundred pixels of
 *    empty space while the rows below it are pushed off the bottom. Full-width rows that own a
 *    layout are capped here, so the surplus goes to the end of the form instead.
 *
 * <b>What it never does.</b> It does not resize a dialog that already fits, it does not touch
 * content that is meant to grow (tables, lists, text areas, canvases), and it does not wrap a
 * dialog twice. A dialog that still does not fit after this — a fixed-size widget larger than
 * the screen, say — is reported rather than broken.
 */
class ODialogFit final : public QObject
{
public:
    /**
     * @brief The verdict of applying the policy to one dialog.
     */
    enum class Verdict
    {
        Skipped,   // not a top-level dialog, or no room to measure against
        Fits,      // the frame fits the usable area
        Scrolls,   // the frame fits, and the content had to be given a scroll area to get there
        Overflows  // the frame cannot fit, whatever this class does — a defect, not a failure
    };

    static void Install();
    static auto Apply(QWidget *dialog, const QRect &available) -> Verdict;

    /// One line per dialog the policy has met, sorted, numbered like the audit: what it asked for, what
    /// the screen offered, and the verdict. This is the measured inventory — the log writes it when the
    /// application quits, and the release audit reads it there.
    static auto InventoryReport() -> QStringList;

    /// @brief Bring a dialog back inside the usable area, and no further.
    ///
    /// A dialog that moves itself after it has been shown is a dialog that cannot be moved: on X11 and
    /// on Wayland, a client that repositions itself while the user is dragging the window makes the
    /// window manager cancel the drag, and the frame snaps back under the cursor. The rule is therefore
    /// the same everywhere — the position the user chose is theirs, and it is only overruled when the
    /// frame is not wholly visible, then by the least move that brings it back.
    static void KeepOnScreen(QWidget *dialog);

    /// @brief Centre a dialog on the usable area, and keep it whole.
    ///
    /// Three rules, in this order: the frame never exceeds the area it must live in (it is shrunk, the
    /// content scrolling if it has to), it is centred, and a centred frame that would hang below the
    /// bottom is raised until it is whole — a window whose buttons are under the edge is not a window.
    static void CenterOnScreen(QWidget *dialog);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit ODialogFit(QObject *parent);

    static void ScheduleRefit(QWidget *dialog);
    static auto ContentToScroll(QWidget *dialog) -> QWidget *;
    static auto WrapInScrollArea(QWidget *content) -> bool;
    static auto WrapContent(QWidget *dialog) -> bool;
    static void CompactFormRows(QWidget *dialog);
    static auto KeepsItsGrowth(const QWidget *widget) -> bool;
    static auto FrameChrome(const QWidget *dialog) -> QMargins;
};

#endif // ODIALOGFIT_H
