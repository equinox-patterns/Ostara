/************************************************************************
 **
 **  @file   osavepiecedialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The name a piece will wear in the library, asked before anything is written.
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
#ifndef OSAVEPIECEDIALOG_H
#define OSAVEPIECEDIALOG_H

#include <QDialog>
#include <QString>

namespace Ui {
class OSavePieceDialog;
}

class QEvent;

/**
 * @brief Asks for the name a piece takes in the library.
 *
 * The field comes prefilled with the piece's own name and stays editable: the name given here is the one
 * the library will call the piece, and the one its file will be named after. The dialogue answers and
 * writes nothing — the caller writes the file, once, after the last agreement — and a refusal costs
 * nothing, which is what lets the caller ask again with the name to change.
 */
class OSavePieceDialog : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit OSavePieceDialog(const QString &pieceName, QWidget *parent = nullptr);
    ~OSavePieceDialog() override;

    /// @brief The name given, meaningful once accepted.
    auto name() const -> QString;

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void CheckState();

private:
    Q_DISABLE_COPY_MOVE(OSavePieceDialog) // NOLINT

    Ui::OSavePieceDialog *ui;
};

#endif // OSAVEPIECEDIALOG_H
