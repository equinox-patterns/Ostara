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

#include "ocustommarginrowwidget.h"
#include "odetailssavedmarginsdialog.h"

#include "../ifc/ifcdef.h"
#include "../vmisc/def.h"
#include "../vwidgets/osamarginpalette.h"
#include "../vmisc/FactoryWidgets.h"
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
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTimer>
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
    auto *combo = MakeComboBox(parent);
    combo->setIconSize(QSize(16, 16));
    for (const QColor &c : OSAMarginPalette::Colors())
    {
        QPixmap px(16, 16);
        px.fill(c);
        combo->addItem(QIcon(px), QString());
    }
    combo->setCurrentIndex(row % OSAMarginPalette::kColorCount);
    return combo;
}
} // namespace

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

    m_onDrawingCheck = new QCheckBox(tr("Export the sew line with the drawings (DXF-AAMA)"));
    m_onDrawingCheck->setToolTip(tr("Places the sew line on the Draw line (8) instead of the dedicated sew-line "
                                    "layer (14). DXF-AAMA export only."));
    mainLayout->addWidget(m_onDrawingCheck);

    auto *valueRow = new QHBoxLayout();
    m_valueEdit = new QLineEdit();
    m_valueEdit->setValidator(new QRegularExpressionValidator(kDecimalRegex, this));
    valueRow->addWidget(m_valueEdit);

    m_applyValueBtn = new QPushButton(tr("Apply"));
    valueRow->addWidget(m_applyValueBtn);
    mainLayout->addLayout(valueRow);

    m_addCustomBtn = new QPushButton(tr("+ Add custom seam allowance"));
    mainLayout->addWidget(m_addCustomBtn);

    m_listContainer = new QWidget();
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);
    mainLayout->addWidget(m_listContainer);

    m_manageSavedBtn = new QPushButton(tr("Manage saved custom seam allowances"));
    mainLayout->addWidget(m_manageSavedBtn);

    ConnectControls();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::RetranslateUI()
{
    m_activeCheck->setText(tr("Activate seam allowance"));
    m_hideMainPathCheck->setText(tr("Hide main path"));
    m_onDrawingCheck->setText(tr("Export the sew line with the drawings (DXF-AAMA)"));
    m_onDrawingCheck->setToolTip(tr("Places the sew line on the Draw line (8) instead of the dedicated sew-line "
                                    "layer (14). DXF-AAMA export only."));
    m_applyValueBtn->setText(tr("Apply"));
    m_addCustomBtn->setText(m_state == State::SelectingPoints ? tr("Cancel") : tr("+ Add custom seam allowance"));
    m_manageSavedBtn->setText(tr("Manage saved custom seam allowances"));

    // Re-translate the default names of custom rows — only those still carrying the
    // previous language's default; a user-renamed row is never overwritten.
    for (int i = 0; i < m_rows.size(); ++i)
    {
        auto *row = m_rows.at(i);
        const QString newDefault = tr("Custom seam allowance %1").arg(i + 1);
        if (row->nameEdit()->text() == row->property("defaultName").toString())
        {
            row->nameEdit()->setText(newDefault);
            row->setProperty("defaultName", newDefault);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::ConnectControls()
{
    // A value typed by the user (including an explicit 0) must never be mistaken for an unset field.
    connect(m_valueEdit, &QLineEdit::textEdited, this, [this]() { m_valueEdited = true; });

    // --- Active checkbox ---
    connect(m_activeCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        // Whether there is a seam allowance at all decides if the DXF-AAMA export option applies.
        UpdateOnDrawingEnabled();

        if (m_updating)
        {
            return;
        }

        if (checked)
        {
            bool okDefault = false;
            const double currentVal = ParseDecimal(m_valueEdit->text(), &okDefault);
            // Inject the configured default only into an untouched field (still showing 0). A 0 the user
            // typed themselves is preserved — activating seam allowances must not swallow an explicit 0.
            if (!okDefault || (qFuzzyIsNull(currentVal) && !m_valueEdited))
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
        // A hidden main path is exactly what the DXF-AAMA export option would move.
        UpdateOnDrawingEnabled();

        if (m_updating || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([checked](VPiece &det) { det.SetHideMainPath(checked); });
    });

    // --- Export the sew line with the drawings checkbox (DXF-AAMA) ---
    connect(m_onDrawingCheck, &QCheckBox::toggled, this, [this](bool checked)
    {
        if (m_updating || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([checked](VPiece &det) { det.SetSewLineOnDrawing(checked); });
    });

    UpdateOnDrawingEnabled();

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

    // --- Add custom seam allowance button ---
    connect(m_addCustomBtn, &QPushButton::clicked, this, [this]()
    {
        switch (m_state)
        {
            case State::Idle:
                m_state = State::SelectingPoints;
                m_selectedNodeIds.clear();
                m_addCustomBtn->setText(tr("Cancel"));
                emit ShowStatusTip(tr("Select the first and last point of the seam allowance segment"));
                break;
            case State::SelectingPoints:
                CancelSelection();
                break;
        }
    });

    // --- Manage saved custom seam allowances button ---
    connect(m_manageSavedBtn, &QPushButton::clicked, this, [this]()
    {
        auto *dialog = new ODetailsSavedMarginsDialog(this);
        dialog->exec();
        delete dialog;
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::UpdateOnDrawingEnabled()
{
    // The DXF-AAMA export option only relocates a sew line that is actually written: with no seam allowance, or
    // with the main path hidden, that export emits nothing it could move. The box is greyed rather than removed so
    // the setting stays readable, and the stored value is left untouched — it becomes live again as soon as the
    // option can have an effect.
    m_onDrawingCheck->setEnabled(m_activeCheck->isChecked() && not m_hideMainPathCheck->isChecked());
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::Show(const VPiece &det)
{
    m_path = det.GetPath();

    m_updating = true;
    m_valueEdited = false;

    m_activeCheck->setChecked(det.IsSeamAllowance());
    m_hideMainPathCheck->setChecked(det.IsHideMainPath());
    m_onDrawingCheck->setChecked(det.IsSewLineOnDrawing());
    m_valueEdit->setText(VAbstractApplication::VApp()->LocaleToString(det.GetSAWidth()));

    PopulateList(det);

    m_updating = false;
    UpdateOnDrawingEnabled();
    FlushDefaultPersist();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::Clear()
{
    m_path = VPiecePath{};
    m_state = State::Idle;
    m_selectedNodeIds.clear();
    m_addCustomBtn->setText(tr("+ Add custom seam allowance"));
    m_updating = false;
    m_valueEdited = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::RefreshList(const VPiece &detail)
{
    m_updating = true;
    PopulateList(detail);
    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::PopulateList(const VPiece &detail)
{
    for (auto *row : m_rows)
    {
        m_listLayout->removeWidget(row);
        delete row;
    }
    m_rows.clear();

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

        const int storedIdx = findStoredIdx(hashKey);
        if (storedIdx < 0)
        {
            m_pendingDefaultPersist = true;
        }
        const QString name = (storedIdx >= 0 && storedIdx < storedNames.size())
                                 ? storedNames.at(storedIdx)
                                 : tr("Custom seam allowance %1").arg(m_rows.size() + 1);
        const int storedColor = (storedIdx >= 0 && storedIdx < storedColors.size())
                                    ? storedColors.at(storedIdx)
                                    : static_cast<int>(m_rows.size() % OSAMarginPalette::kColorCount);
        const bool storedShow = (storedIdx >= 0 && storedIdx < storedShows.size()) ? storedShows.at(storedIdx) : false;

        auto *row = new OCustomMarginRowWidget(group, m_listContainer);
        row->nameEdit()->setText(name);
        row->setProperty("defaultName", name); // marker to detect user-renamed rows on retranslate

        bool valueOk = false;
        const qreal valuePx = groupValue.toDouble(&valueOk);
        row->valueEdit()->setText(valueOk ? QString::number(valuePx) : QStringLiteral("0"));
        row->valueEdit()->setValidator(new QRegularExpressionValidator(kDecimalRegex, this));
        // A stored index read from the XML is unbounded (unrestricted xs:int in the
        // schema); clamp it before setting the combo so a corrupt value cannot leave
        // it at -1 and then get persisted back on the next edit.
        const int safeColor = (storedColor >= 0 && storedColor < OSAMarginPalette::kColorCount) ? storedColor : 0;
        row->colorCombo()->setCurrentIndex(safeColor);
        row->showCheck()->setChecked(storedShow);

        // Name editing.
        connect(row->nameEdit(), &QLineEdit::editingFinished, this, [this, row, hashKey]()
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            const QString text = row->nameEdit()->text().trimmed();
            if (text.isEmpty())
            {
                return;
            }
            m_modifyPending([this, hashKey, text](VPiece &det)
            {
                CollectGroupsFromList(det);
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

        // Value editing — applies the new width to the group's per-node SA formulas.
        connect(row->valueEdit(), &QLineEdit::editingFinished, this, [this, row, group]()
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            bool ok = false;
            const double value = ParseDecimal(row->valueEdit()->text(), &ok);
            if (!ok || value < 0)
            {
                return;
            }
            const QString valueStr = QString::number(value);
            m_modifyPending([group, valueStr](VPiece &det)
            {
                auto nodes = det.GetPath().GetNodes();
                for (int j = 0; j < group.size(); ++j)
                {
                    const int idx = det.GetPath().indexOfNode(group.at(j));
                    if (idx < 0)
                    {
                        continue;
                    }
                    if (j == 0)
                    {
                        nodes[idx].SetFormulaSAAfter(valueStr);
                    }
                    else if (j == group.size() - 1)
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
            });
        });

        // Color.
        connect(row->colorCombo(), QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this, hashKey](int index)
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            m_modifyPending([this, hashKey, index](VPiece &det)
            {
                CollectGroupsFromList(det);
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

        // Show.
        connect(row->showCheck(), &QCheckBox::toggled, this, [this, hashKey](bool checked)
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            m_modifyPending([this, hashKey, checked](VPiece &det)
            {
                CollectGroupsFromList(det);
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

        // Delete.
        connect(row->deleteButton(), &QPushButton::clicked, this, [this, group, hashKey]()
        {
            if (m_updating || !m_modifyPending)
            {
                return;
            }
            m_modifyPending([group, hashKey](VPiece &det)
            {
                // Reset only the group's own SA formulas back to the default. The
                // boundary node's other side may belong to the neighbouring group, so
                // only the side facing this group is reset (first → after, last → before).
                auto nodes = det.GetPath().GetNodes();
                for (int j = 0; j < group.size(); ++j)
                {
                    const int idx = det.GetPath().indexOfNode(group.at(j));
                    if (idx < 0)
                    {
                        continue;
                    }
                    if (j == 0)
                    {
                        nodes[idx].SetFormulaSAAfter(currentSeamAllowance);
                    }
                    else if (j == group.size() - 1)
                    {
                        nodes[idx].SetFormulaSABefore(currentSeamAllowance);
                    }
                    else
                    {
                        nodes[idx].SetFormulaSABefore(currentSeamAllowance);
                        nodes[idx].SetFormulaSAAfter(currentSeamAllowance);
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

        m_listLayout->addWidget(row);
        m_rows.append(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::CollectGroupsFromList(VPiece &det) const
{
    QVector<QVector<quint32>> nodeIdGroups;
    QVector<QString> names;
    QVector<int> colors;
    QVector<bool> shows;

    for (const auto *row : m_rows)
    {
        auto nodeIds = row->nodeIds();
        if (nodeIds.isEmpty())
        {
            continue;
        }
        std::sort(nodeIds.begin(), nodeIds.end());
        nodeIdGroups.append(nodeIds);

        names.append(row->nameEdit()->text().trimmed());
        colors.append(row->colorCombo()->currentIndex());
        shows.append(row->showCheck()->isChecked());
    }

    det.SetSAMarginGroupNodeIds(nodeIdGroups);
    det.SetSAMarginGroupNames(names);
    det.SetSAMarginGroupColorIndices(colors);
    det.SetSAMarginGroupShowStates(shows);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::FlushDefaultPersist()
{
    // Persist the default name/color/show assigned to legacy groups (no stored
    // metadata) exactly once, so they stop shifting on later refreshes. Deferred via
    // singleShot: during Show() the piece widget's m_updating guard would silently
    // drop a synchronous persist.
    if (!m_pendingDefaultPersist || !m_modifyPending)
    {
        return;
    }
    m_pendingDefaultPersist = false;
    QTimer::singleShot(0, this, [this]()
    {
        if (!m_modifyPending)
        {
            return;
        }
        m_modifyPending([this](VPiece &det) { CollectGroupsFromList(det); });
    });
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
void ODetailsSeamAllowanceWidget::OnNodeHovered(quint32 nodeId)
{
    if (m_state != State::SelectingPoints)
    {
        return;
    }

    // Re-apply the persistent highlight for already-selected points — the point's
    // own repaint may reset the transient pen/brush applied by HighlightNode.
    for (quint32 id : m_selectedNodeIds)
    {
        emit HighlightNode(id, true);
    }

    // Clear the previous transient hover highlight (unless it is a selected point).
    if (m_hoveredNodeId != 0 && !m_selectedNodeIds.contains(m_hoveredNodeId))
    {
        emit HighlightNode(m_hoveredNodeId, false);
    }
    m_hoveredNodeId = 0;

    if (nodeId == 0 || m_selectedNodeIds.contains(nodeId))
    {
        return;
    }

    const int idx = m_path.indexOfNode(nodeId);
    if (idx == -1 || m_path.at(idx).IsExcluded())
    {
        return;
    }

    emit HighlightNode(nodeId, true);
    m_hoveredNodeId = nodeId;
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
    m_hoveredNodeId = 0;
    m_addCustomBtn->setText(tr("+ Add custom seam allowance"));
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
    dialog->setWindowTitle(tr("Apply custom seam allowance"));
    auto *dlgLayout = new QVBoxLayout(dialog);

    auto *savedLabel = new QLabel(tr("Select a saved custom seam allowance:"));
    dlgLayout->addWidget(savedLabel);

    auto *savedCombo = MakeComboBox();
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

    auto *createLabel = new QLabel(tr("Create a new custom seam allowance:"));
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

    auto *saveCheck = new QCheckBox(tr("Save to saved custom seam allowances"));
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
        if (s.colorIndex >= 0 && s.colorIndex < OSAMarginPalette::kColorCount)
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
        m_hoveredNodeId = 0;
        m_addCustomBtn->setText(tr("+ Add custom seam allowance"));
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

    if (dialog->exec() == QDialog::Rejected)
    {
        CancelSelection();
    }
    delete dialog;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSeamAllowanceWidget::SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify)
{
    m_modifyPending = std::move(modify);
}
