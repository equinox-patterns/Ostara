/************************************************************************
 **
 **  @file   olayergroupdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Name a group of layers and settle the layers it holds — the
 **          dialog behind "Add a group" and behind the group row's gear.
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

#include "olayergroupdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>

#include "oexportrowiconbutton.h"

#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/typedef.h"

namespace
{
// Enough of the list to see three rows and reach the fourth: past that the dialog would run off the screen for a
// group a reader cannot take in at a glance anyway.
constexpr int kRowsViewHeight = 132;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OLayerGroupDialog::OLayerGroupDialog(QWidget *parent)
  : QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Group of layers"));
    setMinimumWidth(460);

    auto *layout = new QVBoxLayout(this);

    auto *nameRow = new QGridLayout();
    nameRow->addWidget(new QLabel(tr("Group name:"), this), 0, 0);

    m_name = new QLineEdit(this);
    m_name->setPlaceholderText(tr("Choose group name"));
    nameRow->addWidget(m_name, 0, 1);
    layout->addLayout(nameRow);

    // Only shown when there is something to say: a second group cannot wear a name a first one already wears.
    m_problem = new QLabel(this);
    m_problem->setWordWrap(true);
    m_problem->setVisible(false);
    layout->addWidget(m_problem);

    layout->addWidget(new QLabel(tr("Layers:"), this));

    // The rows scroll: a group is not limited to what fits on the screen, and the two buttons have to stay
    // reachable whatever the count.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setMinimumHeight(kRowsViewHeight);
    auto *rowsHost = new QWidget(scroll);
    m_rowsLayout = new QVBoxLayout(rowsHost);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(6);
    m_rowsLayout->addStretch(1);
    scroll->setWidget(rowsHost);
    layout->addWidget(scroll, 1);

    m_addLayer = new QPushButton(tr("Add another layer"), this);
    m_addLayer->setAutoDefault(false);
    layout->addWidget(m_addLayer);

    auto *buttonBox = new QDialogButtonBox(this);
    m_acceptButton = buttonBox->addButton(QDialogButtonBox::Ok);
    m_acceptButton->setAutoDefault(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once the whole box is in: a box lays its buttons out as they arrive and can hand the default
    // to the one added after it.
    m_acceptButton->setDefault(true);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    connect(m_name, &QLineEdit::textChanged, this, [this](const QString &) { RefreshState(); });

    // One row at a time, and only layers that are still free: the list shrinks as the group is filled, so a
    // reader never has to wonder which click was the one that counted.
    connect(m_addLayer, &QPushButton::clicked, this,
            [this]()
            {
                for (auto it = m_available.constBegin(); it != m_available.constEnd(); ++it)
                {
                    if (not ChosenLayers().contains(it.key()))
                    {
                        AddRow(it.key());
                        RefreshRows();
                        return;
                    }
                }
            });

    RefreshRows();
    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::SetLayers(const QMap<quint32, QString> &available, const QVector<quint32> &selected)
{
    // `available` is the caller's answer to "which layers may a row name": the free ones, plus, when editing, the
    // ones the group already holds — a list built from the free layers alone would drop its whole content.
    m_available = available;

    for (const Row &row : m_rows)
    {
        if (row.host != nullptr)
        {
            row.host->deleteLater();
        }
    }
    m_rows.clear();
    m_rowLayers.clear();

    for (const quint32 layer : selected)
    {
        AddRow(layer);
    }

    RefreshRows();
    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::SetTakenNames(const QStringList &names)
{
    m_takenNames = names;
    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::SetGroupName(const QString &name)
{
    m_name->setText(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupDialog::GroupName() const -> QString
{
    return m_name->text().simplified();
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupDialog::Layers() const -> QVector<quint32>
{
    return ChosenLayers();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::AddRow(quint32 layerId)
{
    Row row;
    row.host = new QWidget(m_rowsLayout->parentWidget());
    row.combo = MakeComboBox(row.host);
    row.trash = new OExportRowIconButton(QStringLiteral("trash"), row.host);
    row.trash->setToolTip(tr("Remove the layer from the group"));
    row.trash->setAutoDefault(false);

    if (const auto it = m_available.constFind(layerId); it != m_available.constEnd())
    {
        row.combo->addItem(it.value(), it.key());
    }

    // The row is pointed at by its combo, never by its position: a removal shifts every index after it, and a
    // lambda holding a number would then act on the wrong layer.
    QComboBox *combo = row.combo;
    connect(combo, &QComboBox::currentIndexChanged, this, [this, combo](int) { OnLayerChanged(combo); });
    connect(row.trash, &QPushButton::clicked, this, [this, combo]() { RemoveRow(RowIndex(combo)); });

    auto *hostLayout = new QGridLayout(row.host);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->addWidget(row.combo, 0, 0);
    hostLayout->addWidget(row.trash, 0, 1);

    m_rows.append(row);
    m_rowLayers.append(layerId);
    // Before the stretch, so the rows stay at the top of the scrolling area.
    m_rowsLayout->insertWidget(static_cast<int>(m_rows.size()) - 1, row.host);
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::OnLayerChanged(QComboBox *combo)
{
    const int index = RowIndex(combo);
    if (index < 0 or index >= m_rowLayers.size())
    {
        return;
    }

    const quint32 picked = combo->currentIndex() < 0 ? NULL_ID : combo->currentData().toUInt();
    const quint32 previous = m_rowLayers.at(index);

    // The row that was showing the layer just picked takes the one this row was showing: two rows never name the
    // same layer — the group holds it once — and the gesture is an exchange rather than a layer quietly leaving
    // the group. The layer the picked entry names is not in this list at all, so there is nothing else to swap.
    for (int i = 0; i < m_rowLayers.size(); ++i)
    {
        if (i != index && m_rowLayers.at(i) == picked)
        {
            m_rowLayers[i] = previous;
            break;
        }
    }

    m_rowLayers[index] = picked;

    RefreshRows();
    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::RemoveRow(int index)
{
    if (index < 0 or index >= m_rows.size())
    {
        return;
    }

    if (const Row row = m_rows.takeAt(index); row.host != nullptr)
    {
        row.host->deleteLater();
    }

    if (index < m_rowLayers.size())
    {
        m_rowLayers.removeAt(index);
    }

    RefreshRows();
    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupDialog::RowIndex(const QComboBox *combo) const -> int
{
    for (int i = 0; i < m_rows.size(); ++i)
    {
        if (m_rows.at(i).combo == combo)
        {
            return i;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupDialog::ChosenLayers() const -> QVector<quint32>
{
    QVector<quint32> layers;
    layers.reserve(m_rowLayers.size());

    for (const quint32 id : m_rowLayers)
    {
        if (id != NULL_ID && not layers.contains(id))
        {
            layers.append(id);
        }
    }

    return layers;
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::RefreshRows()
{
    const QVector<quint32> chosen = ChosenLayers();

    // By name, like the dock: a list read twice has to be the same list.
    QVector<QPair<QString, quint32>> sorted;
    sorted.reserve(m_available.size());
    for (auto it = m_available.constBegin(); it != m_available.constEnd(); ++it)
    {
        sorted.append({it.value(), it.key()});
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, quint32> &left, const QPair<QString, quint32> &right)
              {
                  const int order = QString::compare(left.first, right.first, Qt::CaseInsensitive);
                  return order != 0 ? order < 0 : left.second < right.second;
              });

    for (int i = 0; i < m_rows.size(); ++i)
    {
        Row &row = m_rows[i];
        const quint32 current = m_rowLayers.at(i);

        const QSignalBlocker blocker(row.combo);
        row.combo->clear();

        for (const auto &[name, id] : sorted)
        {
            // A layer another row already names is NOT listed at all: shown but disabled it still filled the list
            // with entries that answer nothing, and the list is what the reader goes through to find a layer. What
            // this row holds stays, of course — it is what the row displays.
            if (id != current && chosen.contains(id))
            {
                continue;
            }

            row.combo->addItem(name, id);
        }

        int index = row.combo->findData(current);
        if (index < 0)
        {
            // The layer left the candidates (it joined another group elsewhere): the row falls back on the first
            // entry it is offered, and remembers it, so the list and what the dialog answers never drift apart.
            index = row.combo->count() > 0 ? 0 : -1;
            m_rowLayers[i] = index < 0 ? NULL_ID : row.combo->itemData(0).toUInt();
        }

        row.combo->setCurrentIndex(index);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupDialog::RefreshState()
{
    const QString name = GroupName();

    bool duplicated = false;
    for (const QString &taken : m_takenNames)
    {
        if (QString::compare(taken, name, Qt::CaseInsensitive) == 0)
        {
            duplicated = true;
            break;
        }
    }

    m_problem->setText(duplicated ? tr("Another group already wears this name.") : QString());
    m_problem->setVisible(duplicated);

    if (m_acceptButton != nullptr)
    {
        m_acceptButton->setEnabled(not name.isEmpty() && not duplicated);
    }

    // A row can only be filled while a layer is still free.
    if (m_addLayer != nullptr)
    {
        const QVector<quint32> chosen = ChosenLayers();
        m_addLayer->setEnabled(chosen.size() < m_available.size());
    }
}
