/************************************************************************
 **
 **  @file   dialoggroup.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   4 4, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#include "dialoggroup.h"

#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QScreen>

#include "../dialogtoolbox.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vwidgets/odecimalseparatorfilter.h"
#include "ui_dialoggroup.h"

//---------------------------------------------------------------------------------------------------------------------
DialogGroup::DialogGroup(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogGroup),
    group(),
    flagName(false)
{
    ui->setupUi(this);
    InitOkCancel(ui);

    // The layer's pen: the very values its objects will be offered as a default. A plain combo, not the
    // QtColorPicker + replacement pair the older dialogs use — the replaced picker is left unmanaged by
    // QLayout::replaceWidget and stays visible in a corner of the dialog.
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         LineStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                        ui->comboBoxPenStyle->palette().color(QPalette::Text)));
    // This one comes from the .ui, so the macOS popup fix has to be applied by hand — FillComboBoxTypeLine does
    // it for the pen style, MakeComboBox does it for combos built in code.
    ApplyComboBoxView(ui->comboBoxColor);
    InitColorCombo(ui->comboBoxColor);

    connect(ui->lineEditName, &QLineEdit::textChanged, this, &DialogGroup::NameChanged);

    // Tags are a comma-separated list ("avant, après"): its commas are separators, so the application-wide
    // decimal filter must leave them alone — otherwise a tag ending in a digit would swallow the comma.
    ODecimalSeparatorFilter::MarkCommaAsSeparator(ui->lineEditTags);
}

//---------------------------------------------------------------------------------------------------------------------
DialogGroup::~DialogGroup()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::PlaceDialog()
{
    // Centred, and on the frame rather than on the client size: `move()` places the frame, title bar
    // included, so centring the client area would leave the dialog half a title bar too low. Same idiom as
    // DialogNewPattern and OExportTemplateWindow, and called from showEvent — before the window is painted,
    // so it never appears anywhere else first.
    if (const QScreen *screen = this->screen())
    {
        const QRect available = screen->availableGeometry();
        QRect frame = frameGeometry();
        frame.moveCenter(available.center());

        // Larger than the screen, it cannot be centred on it: pinned to the top and the left, so what it
        // loses is the far edge rather than both.
        if (frame.height() > available.height())
        {
            frame.moveTop(available.top());
        }
        if (frame.width() > available.width())
        {
            frame.moveLeft(available.left());
        }

        move(frame.topLeft());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::SetName(const QString &name)
{
    ui->lineEditName->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGroup::GetName() const -> QString
{
    return ui->lineEditName->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::SetTags(const QStringList &tags)
{
    ui->lineEditTags->setText(tags.join(", "));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGroup::GetTags() const -> QStringList
{
    return ui->lineEditTags->text().split(",");
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::SetGroupCategories(const QStringList &categories)
{
    ui->lineEditTags->SetCompletion(categories);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::SetPenStyle(const QString &penStyle)
{
    ChangeCurrentData(ui->comboBoxPenStyle, penStyle);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGroup::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::SetColor(const QString &color)
{
    SetColorComboCurrentName(ui->comboBoxColor, color);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGroup::GetColor() const -> QString
{
    return GetColorComboCurrentName(ui->comboBoxColor);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::ShowDialog(bool click)
{
    if (not click)
    {
        if (group.isEmpty())
        {
            return;
        }

        SetName(tr("New layer"));

        setModal(true);
        emit ToolTip(QString());
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::CheckDependencyTreeComplete()
{
    // Does nothing
}

//---------------------------------------------------------------------------------------------------------------------
void DialogGroup::SelectedObject(bool selected, quint32 object, quint32 tool)
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
void DialogGroup::NameChanged()
{
    ui->lineEditName->text().isEmpty() ? flagName = false : flagName = true;
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogGroup::GetGroup() const -> QMap<quint32, quint32>
{
    return group;
}
