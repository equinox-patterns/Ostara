/************************************************************************
 **
 **  @file   odecimalseparatorfilter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/09/2026
 **
 **  @brief  Global event filter: a typed decimal comma (',') becomes '.' in
 **          every QLineEdit / QTextEdit / QPlainTextEdit.
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

#include "odecimalseparatorfilter.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QTextEdit>

namespace
{
// A ',' is only a decimal separator when it directly follows a digit (or an already inserted '.'),
// i.e. inside a number token. A ',' typed in plain text (names, notes…) is left untouched.
[[nodiscard]] auto IsDecimalCommaContext(const QString &text, int caret) -> bool
{
    if (caret <= 0 || caret > text.size())
    {
        return false;
    }
    const QChar left = text.at(caret - 1);
    return left.isDigit() || left == QLatin1Char('.');
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODecimalSeparatorFilter::ODecimalSeparatorFilter(QObject *parent)
  : QObject{parent}
{
}

//---------------------------------------------------------------------------------------------------------------------
void ODecimalSeparatorFilter::InstallOnApplication()
{
    qApp->installEventFilter(new ODecimalSeparatorFilter(qApp));
}

//---------------------------------------------------------------------------------------------------------------------
auto ODecimalSeparatorFilter::eventFilter(QObject *o, QEvent *e) -> bool
{
    if (e->type() == QEvent::KeyPress)
    {
        const auto *keyEvent = static_cast<const QKeyEvent *>(e);
        const bool plainComma = !keyEvent->text().isEmpty() && keyEvent->text().at(0) == QLatin1Char(',') &&
                                (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) ==
                                    0;
        if (plainComma)
        {
            if (auto *lineEdit = qobject_cast<QLineEdit *>(o);
                lineEdit != nullptr && lineEdit->isEnabled() && !lineEdit->isReadOnly() &&
                qobject_cast<QAbstractSpinBox *>(lineEdit->parentWidget()) == nullptr)
            {
                // Spin boxes handle the decimal separator themselves, according to the locale;
                // rewriting ',' there would fight their validator.
                const int caret = lineEdit->selectionStart() >= 0 ? lineEdit->selectionStart()
                                                                  : lineEdit->cursorPosition();
                if (IsDecimalCommaContext(lineEdit->text(), caret))
                {
                    lineEdit->insert(QLatin1String(".")); // replaces any selection, like a typed '.'
                    return true;                          // consume the comma
                }
            }
            if (auto *textEdit = qobject_cast<QTextEdit *>(o);
                textEdit != nullptr && textEdit->isEnabled() && !textEdit->isReadOnly())
            {
                const QTextCursor cursor = textEdit->textCursor();
                if (IsDecimalCommaContext(textEdit->toPlainText(), cursor.selectionStart()))
                {
                    QTextCursor editable = cursor;
                    editable.insertText(QLatin1String("."));
                    textEdit->setTextCursor(editable);
                    return true;
                }
            }
            if (auto *plainTextEdit = qobject_cast<QPlainTextEdit *>(o);
                plainTextEdit != nullptr && plainTextEdit->isEnabled() && !plainTextEdit->isReadOnly())
            {
                const QTextCursor cursor = plainTextEdit->textCursor();
                if (IsDecimalCommaContext(plainTextEdit->toPlainText(), cursor.selectionStart()))
                {
                    QTextCursor editable = cursor;
                    editable.insertText(QLatin1String("."));
                    plainTextEdit->setTextCursor(editable);
                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(o, e);
}
