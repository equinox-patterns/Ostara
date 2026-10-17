/************************************************************************
 **
 **  @file   oaxissession.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The gesture that gives a curve handle its angle from an axis: two points, then a direction.
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

#ifndef OCURVEAXISSESSION_H
#define OCURVEAXISSESSION_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVector>

#include "../vgeometry/oaxis.h"

class VDrawTool;

/**
 * @brief One run of the "angle by an axis" gesture: pick two points on the canvas, then choose a direction.
 *
 * The whole feature is a gesture, not a dialog: a button in the Options dock hands the session a curve and a
 * slot, the session collects two clicks, and only then opens the small window that names the four directions.
 * While that window is open the curve is shown as the chosen direction makes it, and nothing is written: the
 * file hears about the axis only when the user accepts, in a single undoable step.
 *
 * Kept apart from the dock because it owns a state of its own — which points are already picked, what the
 * curve looked like before — and from the window, which knows nothing of curves.
 */
class OAxisSession : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit OAxisSession(QObject *parent = nullptr);
    ~OAxisSession() override;

    /**
     * @brief Starts a session on one axis slot of a curve: the user then picks the two points of its axis.
     * @param tool the curve to change, and the one that will do the writing
     * @param slot which of its handles (see VDrawTool::AxisSlotCount)
     * @param slotName how the interface names that handle, for the sentence of the window
     *
     * Ignored while a session is already running, so a stray second click on the button cannot restart the
     * gesture under the user's hands.
     */
    void Start(VDrawTool *tool, int slot, const QString &slotName);

    auto IsRunning() const -> bool;

    /** @brief One point clicked on the canvas. The second one opens the window of directions. */
    void OnPointClicked(quint32 pointId);

    /** @brief Gives up, leaving the curve exactly as it was. */
    void Cancel();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightPoint(quint32 pointId, bool highlight);
    /** @brief The session is over, whichever way it ended: the dock shows the state the curve ended in. */
    void Finished();

private:
    Q_DISABLE_COPY_MOVE(OAxisSession) // NOLINT

    void OpenDirectionWindow();
    void Show(const OAxis &axis);
    void ReassertHighlights();
    auto AxisIntro(const QString &slotName) const -> QString;
    auto HasPoint(quint32 pointId) const -> bool;
    void ReleaseHighlights();
    void End();

    QPointer<VDrawTool> m_tool{};
    int m_slot{-1};
    QString m_slotName{};
    QVector<quint32> m_points{};
    AxisSense m_sense{AxisSense::Line};
};

#endif // OCURVEAXISSESSION_H
