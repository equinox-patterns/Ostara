/************************************************************************
 **
 **  @file   dialogundo.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   23 6, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 11/10/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef DIALOGUNDO_H
#define DIALOGUNDO_H

#include <QDialog>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

namespace Ui
{
class DialogUndo;
}

enum class UndoButton
{
    Undo,
    Fix,
    Cancel
};

class DialogUndo : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit DialogUndo(QWidget *parent = nullptr);
    auto Result() const -> UndoButton;

    /** @brief Say why the formula is refused, in place of the generic sentence.
     *
     *  A formula the parser cannot read needs no explanation — the user knows what a syntax error is. A rule
     *  met for the first time does: it is only obeyed if it says what to do. The sentence is shown where the
     *  generic one stood, and the two buttons underneath are unchanged. */
    void SetReason(const QString &reason);
    ~DialogUndo() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void Cancel();

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(DialogUndo) // NOLINT
    Ui::DialogUndo *ui;
    UndoButton result;
};

inline auto DialogUndo::Result() const -> UndoButton
{
    return result;
}

#endif // DIALOGUNDO_H
