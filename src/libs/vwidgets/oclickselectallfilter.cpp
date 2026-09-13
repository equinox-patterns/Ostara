/************************************************************************
 **
 **  @file   oclickselectallfilter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/08/2026
 **
 **  @brief  Global event filter: single left click selects all text in a
 **          QLineEdit / QTextEdit / QPlainTextEdit, and once everything is
 **          already selected a click restores the normal cursor placement.
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
#include "oclickselectallfilter.h"

#include <QAbstractScrollArea>
#include <QApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextEdit>

#include "vlineedit.h"

//---------------------------------------------------------------------------------------------------------------------
OClickSelectAllFilter::OClickSelectAllFilter(QObject *parent)
  : QObject{parent}
{
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::InstallOnApplication()
{
    qApp->installEventFilter(new OClickSelectAllFilter(qApp));
}

//---------------------------------------------------------------------------------------------------------------------
auto OClickSelectAllFilter::eventFilter(QObject *o, QEvent *e) -> bool
{
    switch (e->type())
    {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            if (HandlePress(o, static_cast<const QMouseEvent *>(e)))
            {
                return true;
            }
            break;

        case QEvent::MouseMove:
            TrackDrag(static_cast<const QMouseEvent *>(e));
            break;

        case QEvent::MouseButtonRelease:
            if (HandleRelease(o, static_cast<const QMouseEvent *>(e)))
            {
                return true;
            }
            break;

        default:
            break;
    }

    return QObject::eventFilter(o, e);
}

//---------------------------------------------------------------------------------------------------------------------
// The press is never consumed: the widget must receive it, otherwise it cannot
// start a mouse drag selection. The select all is decided on release instead.
auto OClickSelectAllFilter::HandlePress(QObject *o, const QMouseEvent *e) -> bool
{
    if (e->button() != Qt::LeftButton)
    {
        return false;
    }

    QWidget *widget = TargetWidget(o);
    if (widget == nullptr)
    {
        return false;
    }

    if (HasFullSelection(widget))
    {
        // Everything is already selected: this click places the cursor. Collapse the
        // selection here rather than letting the widget do it, so that a drag started
        // on this click extends a new selection instead of moving the selected text.
        ForgetPending();
        PlaceCursor(widget, CursorPosition(o, widget, e));
        return e->type() == QEvent::MouseButtonDblClick; // consume: avoid the word selection
    }

    // Not fully selected: remember the press and let the widget place the cursor
    // and build the selection. A single click becomes a select all on release.
    m_pendingWidget = widget;
    m_pressGlobalPos = e->globalPosition().toPoint();
    m_dragged = false;
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OClickSelectAllFilter::HandleRelease(QObject *o, const QMouseEvent *e) -> bool
{
    if (m_pendingWidget.isNull() || e->button() != Qt::LeftButton)
    {
        return false;
    }

    QWidget *const widget = m_pendingWidget;
    const bool wasDragged = m_dragged;
    ForgetPending();

    // The user dragged: what the widget selected is what was asked for.
    if (wasDragged || TargetWidget(o) != widget)
    {
        return false;
    }

    SelectAll(widget);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::TrackDrag(const QMouseEvent *e)
{
    if (m_pendingWidget.isNull() || m_dragged)
    {
        return;
    }

    const QPoint delta = e->globalPosition().toPoint() - m_pressGlobalPos;
    if (delta.manhattanLength() >= QApplication::startDragDistance())
    {
        m_dragged = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::ForgetPending()
{
    m_pendingWidget = nullptr;
    m_pressGlobalPos = QPoint();
    m_dragged = false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OClickSelectAllFilter::TargetWidget(QObject *o) -> QWidget *
{
    if (auto *lineEdit = qobject_cast<QLineEdit *>(o))
    {
        // The completion popup fights with the select-all behaviour: skip it.
        return qobject_cast<VCompleterLineEdit *>(o) != nullptr ? nullptr : lineEdit;
    }
    if (auto *textEdit = qobject_cast<QTextEdit *>(o))
    {
        return textEdit;
    }
    if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(o))
    {
        return plainTextEdit;
    }

    // QTextEdit / QPlainTextEdit deliver mouse events to their viewport child.
    if (auto *viewport = qobject_cast<QWidget *>(o))
    {
        if (auto *textEdit = qobject_cast<QTextEdit *>(viewport->parent()))
        {
            return textEdit;
        }
        if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(viewport->parent()))
        {
            return plainTextEdit;
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
// QTextEdit / QPlainTextEdit want viewport coordinates. The event reaches the
// scroll area itself (a click on its frame) only when it misses the viewport.
auto OClickSelectAllFilter::CursorPosition(QObject *o, QWidget *w, const QMouseEvent *e) -> QPoint
{
    const QPoint pos = e->position().toPoint();

    if (o == w)
    {
        if (auto *scrollArea = qobject_cast<QAbstractScrollArea *>(w))
        {
            return scrollArea->viewport()->mapFrom(scrollArea, pos);
        }
    }

    return pos;
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::SelectAll(QWidget *w)
{
    if (auto *lineEdit = qobject_cast<QLineEdit *>(w))
    {
        lineEdit->selectAll();
    }
    else if (auto *textEdit = qobject_cast<QTextEdit *>(w))
    {
        textEdit->selectAll();
    }
    else if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(w))
    {
        plainTextEdit->selectAll();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::PlaceCursor(QWidget *w, const QPoint &pos)
{
    if (auto *lineEdit = qobject_cast<QLineEdit *>(w))
    {
        lineEdit->setCursorPosition(lineEdit->cursorPositionAt(pos));
    }
    else if (auto *textEdit = qobject_cast<QTextEdit *>(w))
    {
        textEdit->setTextCursor(textEdit->cursorForPosition(pos));
    }
    else if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(w))
    {
        plainTextEdit->setTextCursor(plainTextEdit->cursorForPosition(pos));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OClickSelectAllFilter::HasFullSelection(const QWidget *w) -> bool
{
    if (const auto *lineEdit = qobject_cast<const QLineEdit *>(w))
    {
        return lineEdit->hasSelectedText() && lineEdit->selectionStart() == 0 &&
               lineEdit->cursorPosition() == lineEdit->text().length();
    }
    if (const auto *textEdit = qobject_cast<const QTextEdit *>(w))
    {
        const QTextCursor cursor = textEdit->textCursor();
        return cursor.hasSelection() && cursor.selectionStart() == 0 &&
               cursor.selectionEnd() == textEdit->toPlainText().length();
    }
    if (const auto *plainTextEdit = qobject_cast<const QPlainTextEdit *>(w))
    {
        const QTextCursor cursor = plainTextEdit->textCursor();
        return cursor.hasSelection() && cursor.selectionStart() == 0 &&
               cursor.selectionEnd() == plainTextEdit->toPlainText().length();
    }
    return false;
}
