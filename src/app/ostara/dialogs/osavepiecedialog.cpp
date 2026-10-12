/************************************************************************
 **
 **  @file   osavepiecedialog.cpp
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

#include "osavepiecedialog.h"

#include "ui_osavepiecedialog.h"

#include "def.h"

#include <QEvent>
#include <QLineEdit>
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
OSavePieceDialog::OSavePieceDialog(const QString &pieceName, QWidget *parent)
    : QDialog(parent), ui(new Ui::OSavePieceDialog)
{
    ui->setupUi(this);

    // The piece's own name, as a starting point and not as a decision: the library is shared, and two
    // patterns can hold a piece of the same name without it being the same piece.
    ui->lineEditName->setText(pieceName);

    CheckState();
    connect(ui->lineEditName, &QLineEdit::textChanged, this, &OSavePieceDialog::CheckState);
}

//---------------------------------------------------------------------------------------------------------------------
OSavePieceDialog::~OSavePieceDialog()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    QDialog::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavePieceDialog::name() const -> QString
{
    return ui->lineEditName->text();
}

//---------------------------------------------------------------------------------------------------------------------
void OSavePieceDialog::CheckState()
{
    QPushButton *bOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    SCASSERT(bOk != nullptr)

    // The name becomes a file name in the library, so it has to be able to be one: a name holding a path
    // separator would write the piece into a folder the library does not scan, where nobody would find it
    // again. An empty field needs no explanation; a separator does, so the hint says which rule is not met
    // instead of leaving a dead button.
    const QString name = ui->lineEditName->text();
    const bool isFileName = not name.contains(QLatin1Char('/')) && not name.contains(QLatin1Char('\\'));

    ui->labelHint->setVisible(not name.isEmpty() && not isFileName);
    bOk->setEnabled(not name.isEmpty() && isFileName);
}
