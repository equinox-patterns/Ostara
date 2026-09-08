/************************************************************************
 **
 **  @file   odetailsinternalpathwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   08/09/2026
 **
 **  @brief  Internal paths list widget for the Details mode options dock
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

#ifndef ODETAILSINTERNALPATHWIDGET_H
#define ODETAILSINTERNALPATHWIDGET_H

#include <QWidget>

#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"

class QTableWidget;
class VContainer;

/**
 * @brief The ODetailsInternalPathWidget class lists the internal paths of the
 *        current piece, one compact row each (active dot + name + edit/delete
 *        icons), in the same list style as the Graded "Size ranges" dock.
 *
 *        Activation is a single-active radio driven by the leading dot
 *        (circle / check-circle-fill): clicking the dot or the name activates a
 *        row and asks the canvas to highlight the whole path through the
 *        InternalPathActivated signal. It never touches the pattern itself —
 *        the parent ODetailsPieceWidget acts on its requests.
 */
class ODetailsInternalPathWidget final : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsInternalPathWidget(QWidget *parent = nullptr);
    ~ODetailsInternalPathWidget() override = default;

    /** @brief Show the internal paths of @p detail; resets the active row. */
    void Show(const VPiece &det, const VContainer *data);

    /** @brief Clear the list and the stored context (no piece selected). */
    void Clear();

    /**
     * @brief Refresh the list keeping the active row.
     * @return true when the active row no longer exists (the path was removed
     *         by an undo/redo) — the parent should clear the canvas highlight.
     */
    auto RefreshList(const VPiece &det, const VContainer *data) -> bool;

    /** @brief True when a row is currently active (dot checked, canvas highlight on). */
    auto HasActivePath() const -> bool;

    /** @brief Drop the active row (dot back to circle). Emits no signal. */
    void Deactivate();

    /** @brief Re-label the chrome and refresh icons (LanguageChange / theme). */
    void RetranslateUI();

signals:
    void ShowStatusTip(const QString &text);
    void EditRequested(quint32 pathId);
    void DeleteRequested(quint32 pathId);
    void InternalPathActivated(quint32 pathId);

private:
    void BuildUi();
    void ConnectControls();
    void Populate(const VPiece &det, const VContainer *data);
    void UpdateStatusColumn();
    void UpdateTableHeight();
    auto PathIdAtRow(int row) const -> quint32;

    // UI
    QTableWidget *m_table{nullptr};

    // State
    VPiece m_det{};
    const VContainer *m_data{nullptr};
    QVector<quint32> m_pathIds{}; // row → path id (ids not in the container are skipped)
    quint32 m_activePathId{0};    // runtime single-active selection, not persisted
    bool m_filling{false};

    Q_DISABLE_COPY_MOVE(ODetailsInternalPathWidget) // NOLINT
};

#endif // ODETAILSINTERNALPATHWIDGET_H
