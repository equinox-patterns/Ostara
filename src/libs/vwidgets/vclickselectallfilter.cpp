/************************************************************************
 **
 **  @file   vclickselectallfilter.cpp
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
#include "vclickselectallfilter.h"

#include <QApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextEdit>

#include "vlineedit.h"

//---------------------------------------------------------------------------------------------------------------------
VClickSelectAllFilter::VClickSelectAllFilter(QObject *parent)
  : QObject{parent}
{
}

//---------------------------------------------------------------------------------------------------------------------
void VClickSelectAllFilter::InstallOnApplication()
{
    qApp->installEventFilter(new VClickSelectAllFilter(qApp));
}

//---------------------------------------------------------------------------------------------------------------------
auto VClickSelectAllFilter::eventFilter(QObject *o, QEvent *e) -> bool
{
    if (e->type() != QEvent::MouseButtonPress && e->type() != QEvent::MouseButtonDblClick)
    {
        return QObject::eventFilter(o, e);
    }

    const auto *mouseEvent = static_cast<const QMouseEvent *>(e);
    if (mouseEvent->button() != Qt::LeftButton)
    {
        return QObject::eventFilter(o, e);
    }

    QWidget *widget = TargetWidget(o);
    if (widget == nullptr)
    {
        return QObject::eventFilter(o, e);
    }

    if (HasFullSelection(widget))
    {
        // Everything is already selected: this click places the cursor.
        if (e->type() == QEvent::MouseButtonDblClick)
        {
            PlaceCursor(widget, mouseEvent->position().toPoint());
            return true; // consume: avoid the default word selection
        }
        // Single click: let the default behaviour place the cursor.
        return QObject::eventFilter(o, e);
    }

    // Not fully selected: select all right away (no timer, no latency).
    widget->setFocus();
    SelectAll(widget);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto VClickSelectAllFilter::TargetWidget(QObject *o) -> QWidget *
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
void VClickSelectAllFilter::SelectAll(QWidget *w)
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
void VClickSelectAllFilter::PlaceCursor(QWidget *w, const QPoint &pos)
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
auto VClickSelectAllFilter::HasFullSelection(const QWidget *w) -> bool
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
