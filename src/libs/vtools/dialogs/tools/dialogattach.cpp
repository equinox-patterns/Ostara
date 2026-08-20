/************************************************************************
 **
 **  @file   dialogattach.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/08/2026
 **
 **  @brief  Creation dialog for attachment groups (type Illustrator).
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

#include "dialogattach.h"

#include <QLineEdit>

#include "ui_dialogattach.h"

//---------------------------------------------------------------------------------------------------------------------
DialogAttach::DialogAttach(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogAttach),
    group()
{
    ui->setupUi(this);
    InitOkCancel(ui);
}

//---------------------------------------------------------------------------------------------------------------------
DialogAttach::~DialogAttach()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAttach::SetName(const QString &name)
{
    ui->lineEditName->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAttach::GetName() const -> QString
{
    return ui->lineEditName->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAttach::ShowDialog(bool click)
{
    if (not click)
    {
        if (group.isEmpty())
        {
            return;
        }

        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAttach::CheckDependencyTreeComplete()
{
    // Does nothing
}

//---------------------------------------------------------------------------------------------------------------------
void DialogAttach::SelectedObject(bool selected, quint32 object, quint32 tool)
{
    if (selected)
    {
        group.insert(object, tool);
    }
    else
    {
        group.remove(object);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogAttach::GetGroup() const -> QMap<quint32, quint32>
{
    return group;
}
