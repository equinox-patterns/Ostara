/************************************************************************
 **
 **  @file   oclickselectallfilter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/08/2026
 **
 **  @brief  Global event filter: single left click selects all text in a
 **          QLineEdit / QTextEdit / QPlainTextEdit, and once everything is
 **          already selected a click restores the normal cursor placement.
 **          A mouse drag stays a normal drag: it selects the portion the user
 **          grabbed instead of being turned into a select all.
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
#ifndef OCLICKSELECTALLFILTER_H
#define OCLICKSELECTALLFILTER_H

#include <QObject>
#include <QPoint>
#include <QPointer>

class QMouseEvent;
class QWidget;

// Selects all text on a click in a QLineEdit / QTextEdit / QPlainTextEdit; when
// everything is already selected, a click restores the normal cursor placement.
// This is a state-based behaviour, not a timing one: the click is decided on
// release, so that a mouse drag keeps selecting the text the user grabbed.
//
// Install once on the application:
//   OClickSelectAllFilter::InstallOnApplication();
class OClickSelectAllFilter : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit OClickSelectAllFilter(QObject *parent = nullptr);
    ~OClickSelectAllFilter() override = default;

    static void InstallOnApplication();

protected:
    auto eventFilter(QObject *o, QEvent *e) -> bool override;

private:
    QPointer<QWidget> m_pendingWidget{}; // press whose drag selection is being built
    QPoint m_pressPos{};
    QPoint m_pressGlobalPos{};

    auto HandlePress(QObject *o, const QMouseEvent *e) -> bool;
    void HandleMove(const QMouseEvent *e);
    void HandleRelease(const QMouseEvent *e);
    void ForgetPending();

    static auto TargetWidget(QObject *o) -> QWidget *;
    static void ExtendSelection(QWidget *w, const QPoint &from, const QPoint &to);
    static void SelectAll(QWidget *w);
    static void PlaceCursor(QWidget *w, const QPoint &pos);
    static auto CursorPosition(QObject *o, QWidget *w, const QMouseEvent *e) -> QPoint;
    static auto HasFullSelection(const QWidget *w) -> bool;

    Q_DISABLE_COPY_MOVE(OClickSelectAllFilter) // NOLINT
};

#endif // OCLICKSELECTALLFILTER_H
