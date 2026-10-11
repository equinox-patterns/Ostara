/************************************************************************
 **
 **  @file   dialognameobject.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/10/2026
 **
 **  @brief  Ask for a name for the objects of a formula that have none.
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
#include "dialognameobject.h"

#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

#include "../qmuparser/qmudef.h"
#include "../vpatterndb/vcontainer.h"

//---------------------------------------------------------------------------------------------------------------------
DialogNameObject::DialogNameObject(QList<Object> objects, const VContainer *data, QWidget *parent)
  : QDialog(parent),
    m_objects(std::move(objects)),
    m_names(QStringList(m_objects.size())),
    m_data(data)
{
    setWindowTitle(tr("Name the object"));

    auto *layout = new QVBoxLayout(this);

    auto *explanation
        = new QLabel(tr("This formula uses an object that has no name. Give it one to use it in a formula."), this);
    explanation->setWordWrap(true);
    layout->addWidget(explanation);

    // What the formula writes, so the user recognises the object, and then what to give it. The prefix is shown
    // and never typed: the field holds only the part of the name that belongs to the object.
    auto *objectRow = new QHBoxLayout();
    objectRow->addWidget(new QLabel(tr("Object:"), this));
    m_object = new QLabel(this);
    QFont bold = m_object->font();
    bold.setBold(true);
    m_object->setFont(bold);
    m_object->setTextInteractionFlags(Qt::TextSelectableByMouse);
    objectRow->addWidget(m_object, 1);
    layout->addLayout(objectRow);

    auto *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel(tr("Name:"), this));
    m_name = new QLineEdit(this);
    nameRow->addWidget(m_name, 1);
    layout->addLayout(nameRow);

    m_progress = new QLabel(this);
    layout->addWidget(m_progress);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &DialogNameObject::AcceptCurrent);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_name, &QLineEdit::returnPressed, this, &DialogNameObject::AcceptCurrent);

    ShowCurrent();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNameObject::Names() const -> QStringList
{
    return m_names;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNameObject::ShowCurrent()
{
    if (m_current < 0 || m_current >= m_objects.size())
    {
        return;
    }

    const Object &object = m_objects.at(m_current);
    m_object->setText(object.shown);
    m_name->setText(object.suffix);
    m_name->selectAll();
    m_name->setFocus();

    // Only worth saying when there is more than one: how many are left to name.
    m_progress->setText(m_objects.size() > 1 ? tr("Object %1 of %2").arg(m_current + 1).arg(m_objects.size())
                                             : QString());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogNameObject::AcceptCurrent()
{
    if (m_current < 0 || m_current >= m_objects.size())
    {
        return;
    }

    const Object &object = m_objects.at(m_current);
    const QString name = m_name->text().trimmed();

    // A name no formula could write would be worse than no name at all: the rule that says which names a formula
    // can carry is the pattern's own, asked here before anything is written.
    if (not QRegularExpression(NameRegExp()).match(name).hasMatch())
    {
        QMessageBox::warning(this, windowTitle(), tr("The name %1 cannot be used in a formula.").arg(name));
        return;
    }

    if (not IsNameFree(object, name))
    {
        QMessageBox::warning(this, windowTitle(), tr("The name %1 is already used. Choose another one.").arg(name));
        m_name->selectAll();
        return;
    }

    m_names[m_current] = name;
    ++m_current;

    if (m_current >= m_objects.size())
    {
        accept();
        return;
    }

    ShowCurrent();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNameObject::NameKey(const Object &object, const QString &name) -> QString
{
    // A line answers to the name alone: the length and the angle of one pair carry that one name, so two lines
    // never share it. A curve carries its prefix, so the same word names a spline and a spline path without ever
    // meeting — which is why they are compared as the token each of them would write.
    return object.pairKey.isEmpty() ? object.typeHead + name : name;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogNameObject::IsNameFree(const Object &object, const QString &name) const -> bool
{
    // What has been given in this very sequence is not written anywhere yet, so it is the dialogue that has to
    // remember it: two objects of one formula may not end up with one name.
    const QString key = NameKey(object, name);
    for (int i = 0; i < m_names.size(); ++i)
    {
        if (not m_names.at(i).isEmpty() && NameKey(m_objects.at(i), m_names.at(i)) == key)
        {
            return false;
        }
    }

    if (m_data == nullptr)
    {
        return true;
    }

    if (not object.pairKey.isEmpty())
    {
        const QHash<QString, QString> roles = m_data->LineRoleNames();
        for (auto i = roles.constBegin(); i != roles.constEnd(); ++i)
        {
            if (i.key() != object.pairKey && i.value() == name)
            {
                return false;
            }
        }

        return true;
    }

    // A curve is asked exactly as its own tool asks it: the name it will carry, against every name in the pattern.
    return object.typeHead.isEmpty() || m_data->IsUnique(object.typeHead + name);
}
