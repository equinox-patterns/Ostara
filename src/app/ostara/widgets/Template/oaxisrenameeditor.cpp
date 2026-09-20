/************************************************************************
 **
 **  @file   oaxisrenameeditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   19/09/2026
 **
 **  @brief  Which size axes an information shows, and what each is called.
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

#include "oaxisrenameeditor.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QLocale>
#include <QTabWidget>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OAxisRenameEditor::OAxisRenameEditor(QWidget *parent) : QWidget(parent)
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    m_root->setSpacing(4);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setWordWrap(true);
    m_root->addWidget(m_titleLabel);

    // The axes stand above the tabs, because they decide what the tabs hold: a control that
    // changed the columns from below would be answering a question after it was asked.
    auto *axesBox = new QWidget(this);
    m_axesLayout = new QVBoxLayout(axesBox);
    m_axesLayout->setContentsMargins(0, 0, 0, 0);
    m_axesLayout->setSpacing(2);
    m_root->addWidget(axesBox);

    m_tabs = new QTabWidget(this);
    m_root->addWidget(m_tabs);

    m_oneLineCheck = new QCheckBox(tr("Write each language on its own line"), this);
    connect(m_oneLineCheck, &QCheckBox::toggled, this, [this](bool on) { emit OneLineChanged(on); });
    m_root->addWidget(m_oneLineCheck);
}

//---------------------------------------------------------------------------------------------------------------------

void OAxisRenameEditor::SetTitle(const QString &title)
{
    m_title = title;
    m_titleLabel->setText(title);
}

//---------------------------------------------------------------------------------------------------------------------

void OAxisRenameEditor::SetReservedAxis(const QString &axisName)
{
    m_reservedAxis = axisName;
}

void OAxisRenameEditor::SetOneLineOffered(bool offered)
{
    m_oneLineOffered = offered;
    m_oneLineCheck->setVisible(offered);
}

void OAxisRenameEditor::SetData(const QList<OAxisChoice> &available, const QStringList &selected,
                                const QMap<QString, OAxisRenames> &renames, bool oneLine,
                                const QStringList &languages, bool selectable)
{
    m_available = available;
    m_selected = selected;
    m_renames = renames;
    m_oneLine = oneLine;
    m_languages = languages;
    m_selectable = selectable;
    RebuildAxes();
    RebuildTabs();
}

//---------------------------------------------------------------------------------------------------------------------

auto OAxisRenameEditor::TickedAxes() const -> QStringList
{
    // Read back in the order the boxes were laid out, which is the table's own order — so what is
    // stored is the order the table declares, whatever order the user ticked them in.
    QStringList axes;
    for (int i = 0; i < m_axesLayout->count(); ++i)
    {
        auto *box = qobject_cast<QCheckBox *>(m_axesLayout->itemAt(i)->widget());
        if (box != nullptr && box->isChecked())
        {
            axes.append(box->text());
        }
    }
    return axes;
}

//---------------------------------------------------------------------------------------------------------------------

void OAxisRenameEditor::StoreAlias(const QString &axis, const QString &locale, int valueIndex,
                                   const QString &text)
{
    // What the table itself calls it, so that typing that back means « no alias » rather than
    // storing the original as though the user had chosen it.
    QString original;
    for (const OAxisChoice &choice : m_available)
    {
        if (choice.name != axis)
        {
            continue;
        }
        if (valueIndex < 0)
        {
            original = choice.name;
        }
        else if (valueIndex < choice.values.size())
        {
            original = choice.values.at(valueIndex);
        }
        break;
    }

    OAxisRenames perLanguage = m_renames.value(axis);
    OAxisRename rename = perLanguage.value(locale);
    if (valueIndex < 0)
    {
        rename.name = (text == original) ? QString() : text;
    }
    else
    {
        while (rename.values.size() <= valueIndex)
        {
            rename.values.append(QString());
        }
        rename.values[valueIndex] = (text == original) ? QString() : text;
    }
    perLanguage.insert(locale, rename);
    m_renames.insert(axis, perLanguage);

    emit RenamesChanged(m_renames);
}

//---------------------------------------------------------------------------------------------------------------------

void OAxisRenameEditor::RebuildAxes()
{
    m_building = true;

    // Rebuilt when the table changed under this control — a new .omt declares other axes — and at
    // no other time. See the header: doing it from a box's own handler frees the box mid-click.
    while (QLayoutItem *item = m_axesLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }
    for (const OAxisChoice &axis : m_available)
    {
        if (!m_selectable)
        {
            break; // nothing to choose between: the axes are shown, they are not picked
        }
        auto *box = new QCheckBox(axis.name, m_axesLayout->parentWidget());
        box->setChecked(m_selected.contains(axis.name));
        if (!m_reservedAxis.isEmpty() && axis.name == m_reservedAxis)
        {
            // Unticked as well as greyed: the axis is stated by whatever speaks for it elsewhere,
            // and a page that also showed it as a value of its own would state it twice, two ways.
            box->setChecked(false);
            box->setDisabled(true);
            box->setToolTip(tr("This axis is the one the size range grades along."));
        }
        connect(box, &QCheckBox::toggled, this, [this](bool) {
            if (m_building)
            {
                return;
            }
            m_selected = TickedAxes();
            emit SelectionChanged(m_selected);
            // The columns are the ticked axes, so they follow — but the boxes themselves do not
            // move, and must not: this handler runs inside the click of the box that changed.
            RebuildTabs();
        });
        m_axesLayout->addWidget(box);
    }

    m_building = false;
}

//---------------------------------------------------------------------------------------------------------------------

void OAxisRenameEditor::RebuildTabs()
{
    // The tabs.
    while (m_tabs->count() > 0)
    {
        QWidget *page = m_tabs->widget(0);
        m_tabs->removeTab(0);
        delete page;
    }

    m_oneLineCheck->setChecked(m_oneLine);
    m_oneLineCheck->setEnabled(!m_languages.isEmpty());
    m_oneLineCheck->setVisible(m_oneLineOffered);
    m_tabs->setVisible(!m_languages.isEmpty());

    // The columns: the ticked axes, in the table's order rather than in the order they were ticked.
    QList<OAxisChoice> shown;
    for (const OAxisChoice &axis : m_available)
    {
        if (!m_selectable || m_selected.contains(axis.name))
        {
            shown.append(axis);
        }
    }

    for (const QString &locale : m_languages)
    {
        auto *page = new QWidget(m_tabs);
        auto *grid = new QGridLayout(page);
        grid->setContentsMargins(6, 6, 6, 6);
        grid->setHorizontalSpacing(6);
        grid->setVerticalSpacing(4);

        for (int column = 0; column < shown.size(); ++column)
        {
            const OAxisChoice &axis = shown.at(column);
            const OAxisRename rename = m_renames.value(axis.name).value(locale);

            // The column's own name heads it. Nothing labels it: the field already holds the name,
            // and a caption above it would be reading it aloud a second time. What it *offers*, when
            // the caller says the two differ, is the word rather than the key it is stored under.
            const QString offered = axis.displayName.isEmpty() ? axis.name : axis.displayName;
            auto *nameEdit = new QLineEdit(page);
            nameEdit->setText(rename.name.isEmpty() ? offered : rename.name);
            nameEdit->setPlaceholderText(offered);
            connect(nameEdit, &QLineEdit::textEdited, this,
                    [this, axisName = axis.name, locale](const QString &text) {
                        StoreAlias(axisName, locale, -1, text);
                    });
            grid->addWidget(nameEdit, 0, column);

            // Its values, one per row, in the table's order. An axis shorter than its neighbour
            // simply leaves the lower cells of its column empty: **a value belongs to its axis**,
            // and a grid that shifted the shorter columns up would make it belong to whichever
            // axis happened to be the longest.
            for (int i = 0; i < axis.values.size(); ++i)
            {
                const QString original = axis.values.at(i);
                const QString stored = (i < rename.values.size()) ? rename.values.at(i) : QString();
                auto *valueEdit = new QLineEdit(page);
                valueEdit->setText(stored.isEmpty() ? original : stored);
                valueEdit->setPlaceholderText(original);
                connect(valueEdit, &QLineEdit::textEdited, this,
                        [this, axisName = axis.name, locale, i](const QString &text) {
                            StoreAlias(axisName, locale, i, text);
                        });
                grid->addWidget(valueEdit, i + 1, column);
            }
        }

        m_tabs->addTab(page, QLocale(locale).nativeLanguageName());
    }
}
