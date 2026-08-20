/************************************************************************
 **
 **  @file   dialogattach.h
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

#ifndef DIALOGATTACH_H
#define DIALOGATTACH_H

#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "dialogtool.h"

namespace Ui
{
class DialogAttach;
}

class DialogAttach final : public DialogTool
{
    Q_OBJECT // NOLINT

public:
    explicit DialogAttach(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent = nullptr);
    ~DialogAttach() override;

    void SetName(const QString &name);
    auto GetName() const -> QString;

    auto GetGroup() const -> QMap<quint32, quint32>;

    void ShowDialog(bool click) override;

    void CheckDependencyTreeComplete() override;

public slots:
    void SelectedObject(bool selected, quint32 object, quint32 tool) override;

protected:
    auto IsValid() const -> bool override;
    auto UsesVisualization() const -> bool override { return false; }

private:
    Q_DISABLE_COPY_MOVE(DialogAttach) // NOLINT
    Ui::DialogAttach *ui;
    QMap<quint32, quint32> group;
};

//---------------------------------------------------------------------------------------------------------------------
inline auto DialogAttach::IsValid() const -> bool
{
    // The attachment name is optional, so the dialog is always valid.
    return true;
}

#endif // DIALOGATTACH_H
