/************************************************************************
 **
 **  @file   odetailsseamallowancewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Seam allowance editor widget for the Details mode options dock
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

#include "odetailsseamallowancewidget.h"

#include "odetailssavedmarginsdialog.h"

#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace
{
[[nodiscard]] auto ParseDecimal(const QString &text, bool *ok) -> double
{
    QString normalized = text;
    normalized.replace(QLatin1Char(','), QLatin1Char('.'));
    return QLocale::c().toDouble(normalized, ok);
}

const QRegularExpression kDecimalRegex(QStringLiteral("[0-9]*[\\.,]?[0-9]*"));

auto MakeColorCombo(int row, QWidget *parent = nullptr) -> QComboBox *
{
    auto *combo = new QComboBox(parent);
    combo->setIconSize(QSize(16, 16));
    for (const QColor &c : kCustomMarginColors)
    {
        QPixmap px(16, 16);
        px.fill(c);
        combo->addItem(QIcon(px), QString());
    }
    combo->setCurrentIndex(row % kCustomMarginColors.size());
    return combo;
}
} // namespace

const QVector<QColor> kCustomMarginColors = {
    Qt::red,
    Qt::blue,
    Qt::green,
    QColor(255, 165, 0), // orange
    QColor(128, 0, 128)  // violet
};

//---------------------------------------------------------------------------------------------------------------------
auto FindCustomMarginGroups(const VPiece &detail) -> QVector<CustomMarginGroup>
{
    QVector<CustomMarginGroup> result;
    const VPiecePath &path = detail.GetPath();
    QVector<quint32> group;
    QString groupValue;

    auto closeGroup = [&]()
    {
        if (group.size() >= 2)
        {
            result.append({group, groupValue});
        }
        group.clear();
        groupValue.clear();
    };

    for (int i = 0; i < path.CountNodes(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.IsExcluded())
        {
            closeGroup();
            continue;
        }

        if (node.GetTypeTool() != Tool::NodePoint)
        {
            continue;
        }

        const QString saBefore = node.GetFormulaSABefore();
        const QString saAfter = node.GetFormulaSAAfter();

        if (group.isEmpty())
        {
            if (saAfter != currentSeamAllowance && saBefore != saAfter)
            {
                group.append(node.GetId());
                groupValue = saAfter;
            }
        }
        else
        {
            if (saBefore == groupValue && saAfter == groupValue)
            {
                group.append(node.GetId());
            }
            else if (saBefore == groupValue && saAfter == currentSeamAllowance)
            {
                group.append(node.GetId());
                closeGroup();
            }
            else if (saBefore == groupValue && saAfter != currentSeamAllowance)
            {
                group.append(node.GetId());
                closeGroup();
                group.append(node.GetId());
                groupValue = saAfter;
            }
            else if (saBefore == currentSeamAllowance && saAfter == currentSeamAllowance)
            {
                // Default node between start and end — skip.
            }
            else
            {
                closeGroup();
                if (saAfter != currentSeamAllowance && saBefore != saAfter)
                {
                    group.append(node.GetId());
                    groupValue = saAfter;
                }
            }
        }
    }

    // Handle wrap-around.
    if (!group.isEmpty())
    {
        for (int i = 0; i < path.CountNodes(); ++i)
        {
            const VPieceNode &node = path.at(i);
            if (node.IsExcluded() || node.GetTypeTool() != Tool::NodePoint)
            {
                continue;
            }

            const QString saBefore = node.GetFormulaSABefore();
            const QString saAfter = node.GetFormulaSAAfter();

            if (saBefore == groupValue && saAfter == currentSeamAllowance)
            {
                group.append(node.GetId());
                closeGroup();
                break;
            }
            if (saBefore == groupValue && saAfter == groupValue)
            {
                group.append(node.GetId());
                continue;
            }
            if (saBefore == groupValue && saAfter != currentSeamAllowance)
            {
                group.append(node.GetId());
                closeGroup();
                group.append(node.GetId());
                groupValue = saAfter;
                break;
            }

            closeGroup();
            break;
        }
    }

    return result;
}

//---------------------------------------------------------------------------------------------------------------------
ODetailsSeamAllowanceWidget::ODetailsSeamAllowanceWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_activeCheck = new QCheckBox(tr("Activate seam allowance"));
    mainLayout->addWidget(m_activeCheck);

    m_hideMainPathCheck = new QCheckBox(tr("Hide main path"));
    mainLayout->addWidget(m_hideMainPathCheck);

    auto *valueRow = new QHBoxLayout();
    m_valueEdit = new QLineEdit();
    m_valueEdit->setValidator(new QRegularExpressionValidator(kDecimalRegex, this));
    valueRow->addWidget(m_valueEdit);

    m_applyValueBtn = new QPushButton(tr("Apply"));
    valueRow->addWidget(m_applyValueBtn);
    mainLayout->addLayout(valueRow);

    m_addCustomBtn = new QPushButton(tr("+ Add custom margin"));
    mainLayout->addWidget(m_addCustomBtn);

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Color"), tr("Show"), QString()});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 65);
    m_table->setColumnWidth(2, 55);
    m_table->setColumnWidth(3, 45);
    m_table->setColumnWidth(4, 30);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table);

    m_manageSavedBtn = new QPushButton(tr("Manage saved custom margins"));
    mainLayout->addWidget(m_manageSavedBtn);

    ConnectControls();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::ConnectControls()
{
    // --- Active checkbox ---
    connect(m_activeCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating)
        {
            return;
        }

        if (checked)
        {
            bool okDefault = false;
            const double currentVal = ParseDecimal(m_valueEdit->text(), &okDefault);
            if (!okDefault || qFuzzyIsNull(currentVal))
            {
                const double defaultVal = UnitConvertor(VAbstractApplication::VApp()->Settings()->GetDefaultSeamAllowance(),
                                                        StrToUnits(VAbstractApplication::VApp()->Settings()->GetUnit()),
                                                        VAbstractValApplication::VApp()->patternUnits());
                m_valueEdit->setText(VAbstractApplication::VApp()->LocaleToString(defaultVal));
            }
        }

        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([checked](VPiece &det) { det.SetSeamAllowance(checked); });
    });

    // --- Hide main path checkbox ---
    connect(m_hideMainPathCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([checked](VPiece &det) { det.SetHideMainPath(checked); });
    });

    // --- SA width Apply button ---
    connect(m_applyValueBtn, &QPushButton::clicked, this, [this]()
    {
        if (m_updating || !m_modifyPending)
        {
            return;
        }
        bool ok = false;
        const double value = ParseDecimal(m_valueEdit->text(), &ok);
        if (!ok || value < 0)
        {
            return;
        }
        const QString formula = QString::number(value, 'f', 1);
        m_modifyPending([formula, value](VPiece &det) { det.SetFormulaSAWidth(formula, value); });
    });

    // --- Add custom margin button ---
    connect(m_addCustomBtn, &QPushButton::clicked, this, [this]()
    {
        switch (m_state)
        {
            case State::Idle:
                m_state = State::SelectingPoints;
                m_selectedNodeIds.clear();
                m_addCustomBtn->setText(tr("Cancel"));
                emit ShowStatusTip(tr("Select the first and last point of the margin segment"));
                break;
            case State::SelectingPoints:
                CancelSelection();
                break;
        }
    });

    // --- Manage saved custom margins button ---
    connect(m_manageSavedBtn, &QPushButton::clicked, this, [this]()
    {
        auto *dialog = new ODetailsSavedMarginsDialog(this);
        dialog->exec();
        delete dialog;
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::Show(const VPiece &det)
{
    m_path = det.GetPath();

    m_updating = true;

    m_activeCheck->setChecked(det.IsSeamAllowance());
    m_hideMainPathCheck->setChecked(det.IsHideMainPath());
    m_valueEdit->setText(VAbstractApplication::VApp()->LocaleToString(det.GetSAWidth()));

    PopulateTable(det);

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::Clear()
{
    m_path = VPiecePath{};
    m_state = State::Idle;
    m_selectedNodeIds.clear();
    m_addCustomBtn->setText(tr("+ Add custom margin"));
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::RefreshTable(const VPiece &detail)
{
    m_updating = true;
    PopulateTable(detail);
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::PopulateTable(const VPiece &detail)
{
    m_table->setRowCount(0);

    const QVector<CustomMarginGroup> groups = FindCustomMarginGroups(detail);

    const QVector<QVector<quint32>> storedGroups = detail.GetSAMarginGroupNodeIds();
    const QVector<QString> storedNames = detail.GetSAMarginGroupNames();
    const QVector<int> storedColors = detail.GetSAMarginGroupColorIndices();
    const QVector<bool> storedShows = detail.GetSAMarginGroupShowStates();

    // Pre-compute sorted keys for O(1) lookup.
    QVector<QVector<quint32>> storedKeys;
    storedKeys.reserve(storedGroups.size());
    for (const auto &sg : storedGroups)
    {
        auto key = sg;
        std::sort(key.begin(), key.end());
        storedKeys.append(key);
    }

    const auto findStoredIdx = [&](const QVector<quint32> &hashKey) -> int
    {
        for (int i = 0; i < storedKeys.size(); ++i)
        {
            if (storedKeys.at(i) == hashKey)
            {
                return i;
            }
        }
        return -1;
    };

    for (const auto &grp : groups)
    {
        const QVector<quint32> &group = grp.nodeIds;
        const QString &groupValue = grp.value;

        if (group.size() < 2)
        {
            continue;
        }

        QVector<quint32> hashKey = group;
        std::sort(hashKey.begin(), hashKey.end());

        const int row = m_table->rowCount();
        m_table->insertRow(row);

        const int storedIdx = findStoredIdx(hashKey);
        const QString name = (storedIdx >= 0 && storedIdx < storedNames.size())
                                 ? storedNames.at(storedIdx)
                                 : tr("Custom margin %1").arg(row + 1);
        const int storedColor = (storedIdx >= 0 && storedIdx < storedColors.size())
                                    ? storedColors.at(storedIdx)
                                    : row % kCustomMarginColors.size();
        const bool storedShow = (storedIdx >= 0 && storedIdx < storedShows.size()) ? storedShows.at(storedIdx) : false;

        // Column 0 — name (QLineEdit), node IDs stored in the item's UserRole.
        auto *nameWidget = new QLineEdit(name);
        m_table->setCellWidget(row, 0, nameWidget);
        auto *nodeItem = new QTableWidgetItem();
        nodeItem->setData(Qt::UserRole, QVariant::fromValue(group));
        m_table->setItem(row, 0, nodeItem);

        connect(nameWidget, &QLineEdit::editingFinished, this, [this, hashKey, nameWidget]()
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            const QString text = nameWidget->text().trimmed();
            if (text.isEmpty())
            {
                return;
            }
            m_modifyPending([this, hashKey, text](VPiece &det)
            {
                CollectGroupsFromTable(det);
                auto groups = det.GetSAMarginGroupNodeIds();
                auto names = det.GetSAMarginGroupNames();
                for (int i = 0; i < groups.size() && i < names.size(); ++i)
                {
                    auto g = groups.at(i);
                    std::sort(g.begin(), g.end());
                    if (g == hashKey)
                    {
                        names[i] = text;
                        break;
                    }
                }
                det.SetSAMarginGroupNames(names);
            });
        });

        // Column 1 — value (display only).
        bool ok = false;
        const qreal valuePx = groupValue.toDouble(&ok);
        const QString displayVal = ok ? QString::number(valuePx) : QStringLiteral("0");
        auto *valueWidget = new QLineEdit(displayVal);
        valueWidget->setAlignment(Qt::AlignCenter);
        valueWidget->setReadOnly(true);
        m_table->setCellWidget(row, 1, valueWidget);

        // Column 2 — color combo (wrapped in a container).
        auto *colorCombo = MakeColorCombo(storedColor);
        auto *colorContainer = new QWidget(m_table);
        auto *colorLayout = new QHBoxLayout(colorContainer);
        colorLayout->setContentsMargins(2, 0, 2, 0);
        colorLayout->addWidget(colorCombo);
        m_table->setCellWidget(row, 2, colorContainer);

        connect(colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this, hashKey](int index)
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            m_modifyPending([this, hashKey, index](VPiece &det)
            {
                CollectGroupsFromTable(det);
                auto groups = det.GetSAMarginGroupNodeIds();
                auto colors = det.GetSAMarginGroupColorIndices();
                for (int i = 0; i < groups.size() && i < colors.size(); ++i)
                {
                    auto g = groups.at(i);
                    std::sort(g.begin(), g.end());
                    if (g == hashKey)
                    {
                        colors[i] = index;
                        break;
                    }
                }
                det.SetSAMarginGroupColorIndices(colors);
            });
        });

        // Column 3 — show checkbox (wrapped in a container).
        auto *showCheck = new QCheckBox();
        showCheck->setChecked(storedShow);
        auto *showContainer = new QWidget(m_table);
        auto *showLayout = new QHBoxLayout(showContainer);
        showLayout->setContentsMargins(0, 0, 0, 0);
        showLayout->setAlignment(Qt::AlignCenter);
        showLayout->addWidget(showCheck);
        m_table->setCellWidget(row, 3, showContainer);

        connect(showCheck, &QCheckBox::toggled, this, [this, hashKey](bool checked)
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            m_modifyPending([this, hashKey, checked](VPiece &det)
            {
                CollectGroupsFromTable(det);
                auto groups = det.GetSAMarginGroupNodeIds();
                auto shows = det.GetSAMarginGroupShowStates();
                for (int i = 0; i < groups.size() && i < shows.size(); ++i)
                {
                    auto g = groups.at(i);
                    std::sort(g.begin(), g.end());
                    if (g == hashKey)
                    {
                        shows[i] = checked;
                        break;
                    }
                }
                det.SetSAMarginGroupShowStates(shows);
            });
        });

        // Column 4 — delete button.
        auto *deleteBtn = new QPushButton(QStringLiteral("×"));
        deleteBtn->setFixedWidth(30);
        m_table->setCellWidget(row, 4, deleteBtn);

        connect(deleteBtn, &QPushButton::clicked, this, [this, hashKey]()
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            m_modifyPending([hashKey](VPiece &det)
            {
                // Reset node SA formulas for this group back to the default.
                auto nodes = det.GetPath().GetNodes();
                for (int i = 0; i < nodes.size(); ++i)
                {
                    if (hashKey.contains(nodes.at(i).GetId()))
                    {
                        nodes[i].SetFormulaSABefore(currentSeamAllowance);
                        nodes[i].SetFormulaSAAfter(currentSeamAllowance);
                    }
                }
                det.GetPath().SetNodes(nodes);

                // Remove the group from the 4 parallel arrays.
                auto groups = det.GetSAMarginGroupNodeIds();
                auto names = det.GetSAMarginGroupNames();
                auto colors = det.GetSAMarginGroupColorIndices();
                auto shows = det.GetSAMarginGroupShowStates();
                for (qsizetype i = groups.size() - 1; i >= 0; --i)
                {
                    auto g = groups.at(i);
                    std::sort(g.begin(), g.end());
                    if (g == hashKey)
                    {
                        groups.removeAt(i);
                        if (i < names.size())
                        {
                            names.removeAt(i);
                        }
                        if (i < colors.size())
                        {
                            colors.removeAt(i);
                        }
                        if (i < shows.size())
                        {
                            shows.removeAt(i);
                        }
                        break;
                    }
                }
                det.SetSAMarginGroupNodeIds(groups);
                det.SetSAMarginGroupNames(names);
                det.SetSAMarginGroupColorIndices(colors);
                det.SetSAMarginGroupShowStates(shows);
            });
        });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::CollectGroupsFromTable(VPiece &det) const
{
    QVector<QVector<quint32>> nodeIdGroups;
    QVector<QString> names;
    QVector<int> colors;
    QVector<bool> shows;

    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        const auto *item = m_table->item(r, 0);
        if (!item)
        {
            continue;
        }
        auto nodeIds = item->data(Qt::UserRole).value<QVector<quint32>>();
        if (nodeIds.isEmpty())
        {
            continue;
        }
        std::sort(nodeIds.begin(), nodeIds.end());
        nodeIdGroups.append(nodeIds);

        auto *nameW = qobject_cast<QLineEdit *>(m_table->cellWidget(r, 0));
        auto *colorContainer = m_table->cellWidget(r, 2);
        auto *colorCombo = colorContainer ? colorContainer->findChild<QComboBox *>() : nullptr;
        auto *showContainer = m_table->cellWidget(r, 3);
        auto *showCheck = showContainer ? showContainer->findChild<QCheckBox *>() : nullptr;

        names.append(nameW ? nameW->text().trimmed() : QString());
        colors.append(colorCombo ? colorCombo->currentIndex()
                                 : static_cast<int>(r % kCustomMarginColors.size()));
        shows.append(showCheck ? showCheck->isChecked() : false);
    }

    det.SetSAMarginGroupNodeIds(nodeIdGroups);
    det.SetSAMarginGroupNames(names);
    det.SetSAMarginGroupColorIndices(colors);
    det.SetSAMarginGroupShowStates(shows);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsSeamAllowanceWidget::IsSelectingPoints() const -> bool
{
    return m_state == State::SelectingPoints;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::OnNodeSelected(quint32 nodeId)
{
    if (m_state != State::SelectingPoints)
    {
        return;
    }

    const int idx = m_path.indexOfNode(nodeId);
    if (idx == -1 || m_path.at(idx).IsExcluded())
    {
        return;
    }

    if (m_selectedNodeIds.contains(nodeId))
    {
        return;
    }

    m_selectedNodeIds.append(nodeId);
    emit HighlightNode(nodeId, true);

    if (m_selectedNodeIds.size() == 2)
    {
        FinishSelection();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::CancelSelection()
{
    if (m_state != State::SelectingPoints)
    {
        return;
    }
    m_state = State::Idle;
    m_selectedNodeIds.clear();
    m_addCustomBtn->setText(tr("+ Add custom margin"));
    emit ShowStatusTip(QString());
    emit ClearHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::FinishSelection()
{
    if (m_state != State::SelectingPoints || m_selectedNodeIds.size() < 2)
    {
        return;
    }

    // Walk the path clockwise from the first selected node to the last.
    QVector<quint32> sorted;
    const int idxStart = m_path.indexOfNode(m_selectedNodeIds.first());
    const int idxEnd = m_path.indexOfNode(m_selectedNodeIds.last());
    const auto totalNodes = m_path.CountNodes();

    if (idxStart < 0 || idxEnd < 0 || idxStart == idxEnd || totalNodes <= 0)
    {
        sorted = m_selectedNodeIds; // fallback to exact selection
    }
    else
    {
        sorted.append(m_selectedNodeIds.first());
        vsizetype cur = idxStart;
        while (cur != idxEnd)
        {
            cur = (cur + 1) % totalNodes;
            if (cur == idxStart)
            {
                break; // safety: full loop without finding end
            }
            const VPieceNode &node = m_path.at(cur);
            if (cur == idxEnd)
            {
                sorted.append(node.GetId());
                break;
            }
            if (!node.IsExcluded() && node.GetTypeTool() == Tool::NodePoint)
            {
                sorted.append(node.GetId());
            }
        }
    }

    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(tr("Apply custom margin"));
    auto *dlgLayout = new QVBoxLayout(dialog);

    auto *savedLabel = new QLabel(tr("Select a saved custom margin:"));
    dlgLayout->addWidget(savedLabel);

    auto *savedCombo = new QComboBox();
    savedCombo->addItem(QString()); // Empty = no selection
    const auto savedList = VAbstractValApplication::VApp()->ValentinaSettings()->GetSavedSeamAllowances();
    const Unit patternUnit = VAbstractValApplication::VApp()->patternUnits();
    for (const auto &s : savedList)
    {
        const qreal displayVal = UnitConvertor(s.value, Unit::Mm, patternUnit);
        savedCombo->addItem(QStringLiteral("%1 (%2 %3)")
                                .arg(s.name)
                                .arg(displayVal, 0, 'f', 1)
                                .arg(UnitsToStr(patternUnit, true)),
                            QVariant::fromValue(s));
    }
    dlgLayout->addWidget(savedCombo);

    auto *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    dlgLayout->addWidget(separator);

    auto *createLabel = new QLabel(tr("Create a new custom margin:"));
    dlgLayout->addWidget(createLabel);

    auto *form = new QFormLayout();
    auto *nameEdit = new QLineEdit();
    form->addRow(tr("Name"), nameEdit);
    auto *valueEdit = new QLineEdit();
    valueEdit->setValidator(new QRegularExpressionValidator(kDecimalRegex, dialog));
    form->addRow(tr("Value"), valueEdit);
    auto *colorCombo = MakeColorCombo(0);
    form->addRow(tr("Color"), colorCombo);
    dlgLayout->addLayout(form);

    auto *saveCheck = new QCheckBox(tr("Save to saved custom margins"));
    dlgLayout->addWidget(saveCheck);

    auto *applyBtn = new QPushButton(tr("Apply"));
    dlgLayout->addWidget(applyBtn);

    connect(savedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), dialog,
            [nameEdit, valueEdit, colorCombo, savedCombo](int index)
    {
        if (index <= 0)
        {
            return;
        }
        const auto s = savedCombo->itemData(index).value<SavedSeamAllowance>();
        nameEdit->setText(s.name);
        const qreal displayVal = UnitConvertor(s.value, Unit::Mm, VAbstractValApplication::VApp()->patternUnits());
        valueEdit->setText(VAbstractApplication::VApp()->LocaleToString(displayVal));
        if (s.colorIndex >= 0 && s.colorIndex < kCustomMarginColors.size())
        {
            colorCombo->setCurrentIndex(s.colorIndex);
        }
    });

    connect(applyBtn, &QPushButton::clicked, dialog,
            [this, dialog, nameEdit, valueEdit, saveCheck, colorCombo, sorted, patternUnit]()
    {
        const QString name = nameEdit->text().trimmed();
        if (name.isEmpty())
        {
            return;
        }
        bool ok = false;
        const double userValue = ParseDecimal(valueEdit->text(), &ok);
        if (!ok || userValue < 0)
        {
            return;
        }

        m_state = State::Idle;
        m_addCustomBtn->setText(tr("+ Add custom margin"));
        emit ShowStatusTip(QString());
        emit ClearHighlight();

        if (!m_modifyPending)
        {
            dialog->accept();
            return;
        }

        const QString valueStr = QString::number(userValue);
        const int colorIdx = colorCombo->currentIndex();

        m_modifyPending([sorted, valueStr, name, colorIdx](VPiece &det)
        {
            // Apply per-node SA formulas.
            auto nodes = det.GetPath().GetNodes();
            for (int j = 0; j < sorted.size(); ++j)
            {
                const int idx = det.GetPath().indexOfNode(sorted.at(j));
                if (idx < 0)
                {
                    continue;
                }
                if (j == 0)
                {
                    nodes[idx].SetFormulaSAAfter(valueStr);
                }
                else if (j == sorted.size() - 1)
                {
                    nodes[idx].SetFormulaSABefore(valueStr);
                }
                else
                {
                    nodes[idx].SetFormulaSABefore(valueStr);
                    nodes[idx].SetFormulaSAAfter(valueStr);
                }
            }
            det.GetPath().SetNodes(nodes);

            // Add/update the group metadata in the 4 parallel arrays.
            QVector<quint32> hashKey = sorted;
            std::sort(hashKey.begin(), hashKey.end());

            auto groups = det.GetSAMarginGroupNodeIds();
            auto groupNames = det.GetSAMarginGroupNames();
            auto groupColors = det.GetSAMarginGroupColorIndices();
            auto groupShows = det.GetSAMarginGroupShowStates();

            bool found = false;
            for (int i = 0; i < groups.size(); ++i)
            {
                auto g = groups.at(i);
                std::sort(g.begin(), g.end());
                if (g == hashKey)
                {
                    if (i < groupNames.size())
                    {
                        groupNames[i] = name;
                    }
                    if (i < groupColors.size())
                    {
                        groupColors[i] = colorIdx;
                    }
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                groups.append(hashKey);
                groupNames.append(name);
                groupColors.append(colorIdx);
                groupShows.append(false);
            }

            det.SetSAMarginGroupNodeIds(groups);
            det.SetSAMarginGroupNames(groupNames);
            det.SetSAMarginGroupColorIndices(groupColors);
            det.SetSAMarginGroupShowStates(groupShows);
        });

        if (saveCheck->isChecked())
        {
            auto savedList = VAbstractValApplication::VApp()->ValentinaSettings()->GetSavedSeamAllowances();
            const qreal valueMm = UnitConvertor(userValue, patternUnit, Unit::Mm);
            bool found = false;
            for (auto &s : savedList)
            {
                if (s.name == name)
                {
                    s.value = valueMm;
                    s.colorIndex = colorIdx;
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                savedList.append({name, valueMm, colorIdx});
            }
            VAbstractValApplication::VApp()->ValentinaSettings()->SetSavedSeamAllowances(savedList);
        }

        dialog->accept();
    });

    dialog->exec();
    delete dialog;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify)
{
    m_modifyPending = std::move(modify);
}
