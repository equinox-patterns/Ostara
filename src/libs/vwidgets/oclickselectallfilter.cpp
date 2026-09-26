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
            HandleMove(static_cast<const QMouseEvent *>(e));
            break;

        case QEvent::MouseButtonRelease:
            HandleRelease(static_cast<const QMouseEvent *>(e));
            break;

        default:
            break;
    }

    return QObject::eventFilter(o, e);
}

//---------------------------------------------------------------------------------------------------------------------
// The select all happens here, on the press, and the press is consumed: once it is done, nothing left in the
// chain can undo it. That is what makes the behaviour reliable — deciding it later, on the release, made it
// depend on the pointer and on which widget the release reached, and it could silently never happen.
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

    ForgetPending();
    m_pendingWidget = widget;
    m_pressPos = e->position().toPoint();
    m_pressGlobalPos = e->globalPosition().toPoint();

    widget->setFocus();
    SelectAll(widget);

    // The widget never sees this press: it cannot collapse the selection on its own, and a drag is built by
    // HandleMove instead, the way the widget would have built it.
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
// A press on a field that was not fully selected has just selected everything. The moment the pointer drags
// instead of clicking, the selection goes back to the dragged portion, and follows the pointer to the release.
void OClickSelectAllFilter::HandleMove(const QMouseEvent *e)
{
    if (m_pendingWidget.isNull() || (e->buttons() & Qt::LeftButton) == 0)
    {
        return; // no press to extend, or a plain hover move
    }

    const QPoint delta = e->globalPosition().toPoint() - m_pressGlobalPos;
    if (delta.manhattanLength() < QApplication::startDragDistance())
    {
        return; // still a click: the select all stands
    }

    ExtendSelection(m_pendingWidget, m_pressPos, e->position().toPoint());
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::HandleRelease(const QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        ForgetPending(); // the click is over, whatever it became
    }
}

//---------------------------------------------------------------------------------------------------------------------
// The same drag selection for both text edit classes, which share the QTextCursor API.
template <typename T> static void SelectTextRange(T *edit, const QPoint &from, const QPoint &to)
{
    const int anchor = edit->cursorForPosition(from).position();
    const int cursor = edit->cursorForPosition(to).position();

    QTextCursor selection = edit->textCursor();
    selection.setPosition(anchor);
    selection.setPosition(cursor, QTextCursor::KeepAnchor);
    edit->setTextCursor(selection);
}

//---------------------------------------------------------------------------------------------------------------------
// The whole point of installing this filter on the application: build, from the press point to the current
// point, the selection the widget itself would have built had it received the press.
void OClickSelectAllFilter::ExtendSelection(QWidget *w, const QPoint &from, const QPoint &to)
{
    if (auto *lineEdit = qobject_cast<QLineEdit *>(w))
    {
        const int anchor = lineEdit->cursorPositionAt(from);
        const int cursor = lineEdit->cursorPositionAt(to);
        lineEdit->setSelection(qMin(anchor, cursor), qAbs(cursor - anchor));
        return;
    }
    if (auto *textEdit = qobject_cast<QTextEdit *>(w))
    {
        SelectTextRange(textEdit, from, to);
        return;
    }
    if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(w))
    {
        SelectTextRange(plainTextEdit, from, to);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OClickSelectAllFilter::ForgetPending()
{
    m_pendingWidget = nullptr;
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
