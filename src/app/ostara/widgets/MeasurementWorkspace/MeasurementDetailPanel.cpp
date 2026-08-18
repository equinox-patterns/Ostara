/************************************************************************
 **
 **  @file   MeasurementDetailPanel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Detail/options panel with range group support.
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

#include "MeasurementDetailPanel.h"
#include "MeasurementTablePanel.h"
#include "DialogRangeGroups.h"
#include "widgets/OSelectAllSpinBox.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFormLayout>
#include <QListView>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QVBoxLayout>

// ===== Constructor =====

MeasurementDetailPanel::MeasurementDetailPanel(QWidget *parent)
    : QGroupBox(tr("Measurement details"), parent)
{
    SetupUI();
}

void MeasurementDetailPanel::SetupUI()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // Dot-selector (.QWidget) styles only each widget itself, NOT children.
    // Transparent → QGroupBox background shows through in all themes.
    scrollArea->setStyleSheet(
        QStringLiteral(".QScrollArea { background: transparent; }"));
    scrollArea->viewport()->setStyleSheet(
        QStringLiteral(".QWidget { background: transparent; }"));
    outerLayout->addWidget(scrollArea);

    auto *scrollWidget = new QWidget();
    auto *mainLayout = new QVBoxLayout(scrollWidget);
    mainLayout->setSpacing(8);

    m_form = new QFormLayout();
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // Name
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(QStringLiteral("@..."));
    m_nameEdit->setToolTip(tr("Name used in formulas (@ auto)"));
    connect(m_nameEdit, &QLineEdit::textEdited, this, &MeasurementDetailPanel::OnFieldChanged);
    m_form->addRow(tr("Name:"), m_nameEdit);

    // Full name
    m_fullNameEdit = new QLineEdit();
    m_fullNameEdit->setPlaceholderText(tr("e.g., Chest circumference"));
    connect(m_fullNameEdit, &QLineEdit::textEdited, this, &MeasurementDetailPanel::OnFieldChanged);
    m_form->addRow(tr("Full name:"), m_fullNameEdit);

    // Description — default height ~60px. A grip below the field lets the
    // user resize it by dragging, even within the scroll area.
    m_descriptionEdit = new QPlainTextEdit();
    m_descriptionEdit->setPlaceholderText(tr("Measurement description"));
    m_descriptionEdit->setFixedHeight(60);
    connect(m_descriptionEdit, &QPlainTextEdit::textChanged, this, &MeasurementDetailPanel::OnFieldChanged);

    // Grip: thin bar the user can drag to resize the description
    m_descGrip = new QWidget();
    m_descGrip->setFixedHeight(5);
    m_descGrip->setCursor(Qt::SplitVCursor);
    m_descGrip->setObjectName(QStringLiteral("detailSeparator"));
    m_descGrip->installEventFilter(this);

    auto *descWidget = new QWidget();
    auto *descLayout = new QVBoxLayout(descWidget);
    descLayout->setContentsMargins(0, 0, 0, 0);
    descLayout->setSpacing(0);
    descLayout->addWidget(m_descriptionEdit);
    descLayout->addWidget(m_descGrip);

    m_form->addRow(tr("Description:"), descWidget);

    // Base value
    m_anchorValueSpin = new OSelectAllSpinBox();
    m_anchorValueSpin->setRange(-10000.0, 10000.0);
    m_anchorValueSpin->setDecimals(2);
    m_anchorValueSpin->setToolTip(tr("Value at reference size"));
    connect(m_anchorValueSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MeasurementDetailPanel::OnFieldChanged);
    m_form->addRow(tr("Base value:"), m_anchorValueSpin);

    mainLayout->addLayout(m_form);

    // Separator — 1px line (80% width)
    {
        auto *sepWidget = new QWidget();
        auto *sepLayout = new QHBoxLayout(sepWidget);
        sepLayout->setContentsMargins(0, 8, 0, 0);
        sepLayout->addStretch(1);
        auto *sep = new QWidget();
        sep->setFixedHeight(1);
        sep->setObjectName(QStringLiteral("detailSeparator"));
        sepLayout->addWidget(sep, 8);
        sepLayout->addStretch(1);
        mainLayout->addWidget(sepWidget);
    }

    // Increment sections area — compact, no stretch
    m_incrementArea = new QVBoxLayout();
    m_incrementArea->setSpacing(4);
    mainLayout->addLayout(m_incrementArea);

    // Placeholder — shown when nothing is selected
    m_placeholderLabel = new QLabel(tr("Select or create a measurement"));
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setWordWrap(true);
    m_placeholderLabel->setObjectName(QStringLiteral("fieldHint"));
    m_incrementArea->addWidget(m_placeholderLabel);

    // Push everything to the top
    mainLayout->addStretch(1);

    scrollArea->setWidget(scrollWidget);

    Clear();
}

void MeasurementDetailPanel::SetAxisGroups(const QStringList &groupNames)
{
    m_groupNames = groupNames;
    RebuildIncrementSections();

    // Solo mode: rename "Valeur de base" → "Valeur"
    if (m_anchorValueSpin)
    {
        QLabel *label = qobject_cast<QLabel *>(m_form->labelForField(m_anchorValueSpin));
        if (label)
        {
            label->setText(groupNames.isEmpty() ? tr("Value:") : tr("Base value:"));
            m_anchorValueSpin->setToolTip(groupNames.isEmpty()
                ? tr("Measurement value")
                : tr("Value at reference size"));
        }
    }
}

void MeasurementDetailPanel::SetGroupValues(const QMap<QString, QStringList> &groupValues)
{
    m_groupValues = groupValues;
}

void MeasurementDetailPanel::SetAllRecords(const QVector<MeasurementRecord> &records)
{
    m_allRecords = records;
}

void MeasurementDetailPanel::SetRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups)
{
    m_rangeGroups = rangeGroups;
}

auto MeasurementDetailPanel::GetRangeGroups() const -> QMap<QString, QVector<RangeGroup>>
{
    return m_rangeGroups;
}

// ===== Increment Sections =====

void MeasurementDetailPanel::RebuildIncrementSections()
{
    // Clear
    while (m_incrementArea->count() > 0)
    {
        QLayoutItem *item = m_incrementArea->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }
    m_groupWidgets.clear();

    for (int i = 0; i < m_groupNames.size(); ++i)
    {
        const QString &name = m_groupNames.at(i);
        GroupWidgets gw;

        // Separator between groups (except before first) — 1px line
        if (i > 0)
        {
            auto *sepWidget = new QWidget();
            auto *sepLayout = new QHBoxLayout(sepWidget);
            sepLayout->setContentsMargins(0, 4, 0, 4);
            sepLayout->addStretch(1);
            auto *sep = new QWidget();
            sep->setFixedHeight(1);
            sep->setObjectName(QStringLiteral("detailSeparator"));
            sepLayout->addWidget(sep, 8);
            sepLayout->addStretch(1);
            m_incrementArea->addWidget(sepWidget);
        }

        auto *container = new QWidget();
        auto *vl = new QVBoxLayout(container);
        vl->setContentsMargins(0, 2, 0, 2);
        vl->setSpacing(4);
        gw.container = container;

        // Spacing before title
        vl->addSpacing(4);

        // Title
        auto *titleLabel = new QLabel(tr("Interval <b>%1</b>").arg(name));
        gw.titleLabel = titleLabel;

        // Mode toggle + config button
        auto *modeRow = new QHBoxLayout();
        gw.linearRadio = new QRadioButton(tr("Linear"));
        modeRow->addWidget(gw.linearRadio);
        modeRow->addSpacing(24);
        gw.rangesRadio = new QRadioButton(tr("Non-linear"));
        gw.linearRadio->setChecked(true);
        gw.modeGroup = new QButtonGroup(this);
        gw.modeGroup->addButton(gw.linearRadio, 0);
        gw.modeGroup->addButton(gw.rangesRadio, 1);
        modeRow->addWidget(gw.rangesRadio);
        modeRow->addStretch();

        gw.configBtn = new QPushButton(tr("Configure ranges"));
        gw.configBtn->setVisible(false);
        int capturedIdx = i;
        connect(gw.configBtn, &QPushButton::clicked, this, [this, capturedIdx](){
            const QString &grpName = m_groupNames.at(capturedIdx);
            const QStringList &grpValues = m_groupValues.value(grpName);
            QVector<RangeGroup> &groups = m_rangeGroups[grpName];
            DialogRangeGroups dlg(grpName, grpValues, groups, this);
            if (dlg.exec() == QDialog::Accepted)
            {
                RefreshRangeGroupCombo(capturedIdx);
                OnFieldChanged();
            }
        });
        modeRow->addWidget(gw.configBtn);
        vl->addLayout(modeRow);

        connect(gw.modeGroup, QOverload<int>::of(&QButtonGroup::idClicked),
                this, [this, i](int){ OnModeToggled(i); });

        // Linear spin
        gw.linearSpin = new OSelectAllSpinBox();
        gw.linearSpin->setRange(0.0, 10000.0);
        gw.linearSpin->setDecimals(2);
        connect(gw.linearSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &MeasurementDetailPanel::OnFieldChanged);
        vl->addWidget(gw.linearSpin);

        // Range group dropdown (shown when Par plages)
        gw.rangeGroupCombo = new QComboBox();
        gw.rangeGroupCombo->setView(new QListView());
        gw.rangeGroupCombo->setVisible(false);
        connect(gw.rangeGroupCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, i](int){ OnRangeGroupChanged(i); });
        vl->addWidget(gw.rangeGroupCombo);

        // No ranges label
        gw.noRangesLabel = new QLabel(tr("No range groups defined. Click Configure ranges."));
        gw.noRangesLabel->setWordWrap(true);
        gw.noRangesLabel->setVisible(false);
        vl->addWidget(gw.noRangesLabel);

        // Range values area
        gw.rangeValuesLayout = new QVBoxLayout();
        gw.rangeValuesLayout->setSpacing(2);
        vl->addLayout(gw.rangeValuesLayout);

        m_incrementArea->addWidget(titleLabel);
        m_incrementArea->addWidget(container);
        // Hidden until a measurement is selected
        titleLabel->setVisible(false);
        container->setVisible(false);
        m_groupWidgets.append(gw);
    }

    // Re-create placeholder (it was deleted by the clear loop above)
    m_placeholderLabel = new QLabel(tr("Select or create a measurement"));
    m_placeholderLabel->setAlignment(Qt::AlignCenter);
    m_placeholderLabel->setWordWrap(true);
    m_placeholderLabel->setObjectName(QStringLiteral("fieldHint"));
    m_incrementArea->addWidget(m_placeholderLabel);
}

void MeasurementDetailPanel::OnModeToggled(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_groupWidgets.size()) return;

    auto &gw = m_groupWidgets[groupIndex];
    bool isLinear = gw.linearRadio->isChecked();

    gw.linearSpin->setVisible(isLinear);
    gw.configBtn->setVisible(!isLinear);
    gw.rangeGroupCombo->setVisible(!isLinear);

    if (!isLinear)
    {
        RefreshRangeGroupCombo(groupIndex);
    }
    else
    {
        gw.noRangesLabel->setVisible(false);
        gw.rangeGroupCombo->setVisible(false);
        ClearRangeValuesWidgets(groupIndex);
    }

    OnFieldChanged();
}

void MeasurementDetailPanel::RefreshRangeGroupCombo(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_groupWidgets.size()) return;

    auto &gw = m_groupWidgets[groupIndex];
    const QString &groupName = m_groupNames.at(groupIndex);

    QSignalBlocker blocker(gw.rangeGroupCombo);
    gw.rangeGroupCombo->clear();

    auto it = m_rangeGroups.find(groupName);
    if (it != m_rangeGroups.end() && !it->isEmpty())
    {
        for (const auto &rg : *it)
        {
            gw.rangeGroupCombo->addItem(rg.name);
        }
        gw.rangeGroupCombo->setVisible(true);
        gw.noRangesLabel->setVisible(false);
        OnRangeGroupChanged(groupIndex);
    }
    else
    {
        gw.rangeGroupCombo->setVisible(false);
        gw.noRangesLabel->setVisible(true);
        ClearRangeValuesWidgets(groupIndex);
    }
}

void MeasurementDetailPanel::OnRangeGroupChanged(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_groupWidgets.size()) return;

    auto &gw = m_groupWidgets[groupIndex];
    const QString &groupName = m_groupNames.at(groupIndex);

    // Clear previous range value widgets
    ClearRangeValuesWidgets(groupIndex);

    QString selectedName = gw.rangeGroupCombo->currentText();
    auto it = m_rangeGroups.find(groupName);
    if (it == m_rangeGroups.end())
    {
        return;
    }

    const RangeGroup *selectedRg = nullptr;
    for (const auto &rg : *it)
    {
        if (rg.name == selectedName)
        {
            selectedRg = &rg;
            break;
        }
    }
    if (!selectedRg)
    {
        return;
    }

    gw.currentRanges = selectedRg->ranges;
    gw.currentRangeValues.resize(gw.currentRanges.size());
    for (auto &v : gw.currentRangeValues) v = 0.0;

    // Create read-only labels and increment spin boxes
    for (int r = 0; r < selectedRg->ranges.size(); ++r)
    {
        const RangeDef &rd = selectedRg->ranges.at(r);

        auto *row = new QHBoxLayout();
        auto *label = new QLabel(QStringLiteral("%1 → %2").arg(rd.fromValue, rd.toValue));
        row->addWidget(label);

        auto *spin = new OSelectAllSpinBox();
        spin->setRange(0.0, 10000.0);
        spin->setDecimals(2);
        spin->setValue(0.0);
        int capturedRow = r;
        int capturedGroup = groupIndex;
        connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, [this, capturedGroup, capturedRow](double val){
            OnRangeValueChanged(capturedGroup, capturedRow, val);
        });
        row->addWidget(spin, 1);

        gw.rangeValuesLayout->addLayout(row);
    }

    OnFieldChanged();
}

void MeasurementDetailPanel::ClearRangeValuesWidgets(int groupIndex)
{
    if (groupIndex < 0 || groupIndex >= m_groupWidgets.size()) return;

    auto &gw = m_groupWidgets[groupIndex];
    while (gw.rangeValuesLayout->count() > 0)
    {
        QLayoutItem *item = gw.rangeValuesLayout->takeAt(0);
        if (item->layout())
        {
            QLayoutItem *child;
            while ((child = item->layout()->takeAt(0)) != nullptr)
            {
                if (child->widget()) delete child->widget();
                delete child;
            }
        }
        delete item;
    }
    gw.currentRanges.clear();
    gw.currentRangeValues.clear();
}

void MeasurementDetailPanel::OnRangeValueChanged(int groupIndex, int rangeIndex, double value)
{
    if (groupIndex < 0 || groupIndex >= m_groupWidgets.size()) return;

    auto &gw = m_groupWidgets[groupIndex];
    if (rangeIndex >= 0 && rangeIndex < gw.currentRangeValues.size())
    {
        gw.currentRangeValues[rangeIndex] = value;
    }
    OnFieldChanged();
}

// ===== Show / Clear / Build =====

void MeasurementDetailPanel::ShowMeasurement(const MeasurementRecord &rec)
{
    m_placeholderLabel->setVisible(false);

    // Show all form rows
    for (int i = 0; i < m_form->rowCount(); ++i)
    {
        auto *lbl = m_form->itemAt(i, QFormLayout::LabelRole);
        auto *fld = m_form->itemAt(i, QFormLayout::FieldRole);
        if (lbl && lbl->widget()) lbl->widget()->setVisible(true);
        if (fld && fld->widget()) fld->widget()->setVisible(true);
    }
    for (const auto &gw : m_groupWidgets)
    {
        gw.container->setVisible(true);
        gw.titleLabel->setVisible(true);
    }
    setTitle(tr("Measurement details"));
    m_isGroupMode = false;

    m_populating = true;
    setEnabled(true);

    QString displayName = rec.name;
    if (displayName.startsWith(QLatin1Char('@')))
    {
        displayName = displayName.mid(1);
    }
    m_nameEdit->setText(displayName);
    m_fullNameEdit->setText(rec.fullName);
    m_descriptionEdit->setPlainText(rec.description);
    m_anchorValueSpin->setValue(rec.anchorValue);

    for (int i = 0; i < m_groupNames.size() && i < m_groupWidgets.size(); ++i)
    {
        const GroupIncrement &gi = rec.increments.value(m_groupNames.at(i));
        auto &gw = m_groupWidgets[i];

        if (gi.isLinear)
        {
            gw.linearRadio->setChecked(true);
            gw.linearSpin->setValue(gi.linearValue);
            OnModeToggled(i);
        }
        else
        {
            gw.rangesRadio->setChecked(true);
            OnModeToggled(i);

            // Select the range group in the combo
            int idx = gw.rangeGroupCombo->findText(gi.rangeGroupName);
            if (idx >= 0)
            {
                gw.rangeGroupCombo->setCurrentIndex(idx);
            }

            // Set range values
            for (int r = 0; r < gi.rangeValues.size() && r < gw.currentRangeValues.size(); ++r)
            {
                gw.currentRangeValues[r] = gi.rangeValues.at(r);
            }

            // Update spin boxes in the UI
            for (int r = 0; r < gw.currentRangeValues.size(); ++r)
            {
                // Find the spin in the layout
                if (r < gw.rangeValuesLayout->count())
                {
                    auto *row = gw.rangeValuesLayout->itemAt(r)->layout();
                    if (row && row->count() > 1)
                    {
                        auto *spin = qobject_cast<QDoubleSpinBox *>(row->itemAt(1)->widget());
                        if (spin)
                        {
                            spin->setValue(gw.currentRangeValues.at(r));
                        }
                    }
                }
            }
        }
    }

    m_populating = false;
}

void MeasurementDetailPanel::ShowGroupHeader(const MeasurementRecord &rec)
{
    m_placeholderLabel->setVisible(false);
    m_populating = true;
    setEnabled(true);

    // Show only the first form row (Nom), hide the rest
    for (int i = 0; i < m_form->rowCount(); ++i)
    {
        auto *lbl = m_form->itemAt(i, QFormLayout::LabelRole);
        auto *fld = m_form->itemAt(i, QFormLayout::FieldRole);
        bool visible = (i == 0);
        if (lbl && lbl->widget()) lbl->widget()->setVisible(visible);
        if (fld && fld->widget()) fld->widget()->setVisible(visible);
    }
    m_nameEdit->setText(rec.name);

    for (const auto &gw : m_groupWidgets)
    {
        gw.container->setVisible(false);
        gw.titleLabel->setVisible(false);
    }

    setTitle(tr("Group properties"));
    m_isGroupMode = true;
    m_populating = false;
}

void MeasurementDetailPanel::Clear()
{
    m_populating = true;
    setEnabled(false);

    m_nameEdit->clear();
    m_fullNameEdit->clear();
    m_descriptionEdit->clear();
    m_anchorValueSpin->setValue(0.0);

    for (int i = 0; i < m_groupWidgets.size(); ++i)
    {
        auto &gw = m_groupWidgets[i];
        gw.linearRadio->setChecked(true);
        gw.linearSpin->setValue(0.0);
        OnModeToggled(i);
    }

    // Hide all form rows (widget + label)
    for (int i = 0; i < m_form->rowCount(); ++i)
    {
        auto *lbl = m_form->itemAt(i, QFormLayout::LabelRole);
        auto *fld = m_form->itemAt(i, QFormLayout::FieldRole);
        if (lbl && lbl->widget()) lbl->widget()->setVisible(false);
        if (fld && fld->widget()) fld->widget()->setVisible(false);
    }

    // Hide all increment sections
    for (const auto &gw : m_groupWidgets)
    {
        gw.container->setVisible(false);
        gw.titleLabel->setVisible(false);
    }
    m_placeholderLabel->setVisible(true);
    setTitle(tr("Measurement details"));
    m_isGroupMode = false;

    m_populating = false;
}

auto MeasurementDetailPanel::BuildRecord() const -> MeasurementRecord
{
    MeasurementRecord rec;

    if (m_isGroupMode)
    {
        rec.name = m_nameEdit->text().trimmed();
        rec.isSeparator = true;
        return rec;
    }

    QString name = m_nameEdit->text().trimmed();
    if (!name.isEmpty() && !name.startsWith(QLatin1Char('@')))
    {
        name.prepend(QLatin1Char('@'));
    }
    rec.name = name;
    rec.fullName = m_fullNameEdit->text().trimmed();
    rec.description = m_descriptionEdit->toPlainText().trimmed();
    rec.anchorValue = m_anchorValueSpin->value();

    for (int i = 0; i < m_groupNames.size() && i < m_groupWidgets.size(); ++i)
    {
        const auto &gw = m_groupWidgets[i];
        GroupIncrement gi;

        if (gw.linearRadio->isChecked())
        {
            gi.isLinear = true;
            gi.linearValue = gw.linearSpin->value();
        }
        else
        {
            gi.isLinear = false;
            gi.rangeGroupName = gw.rangeGroupCombo->currentText();
            gi.rangeValues = gw.currentRangeValues;
        }

        rec.increments[m_groupNames.at(i)] = gi;
    }

    return rec;
}

bool MeasurementDetailPanel::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_descGrip)
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton)
            {
                m_descResizing = true;
                m_descResizeStartY = static_cast<int>(me->globalPosition().y());
                m_descStartHeight = m_descriptionEdit->height();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove && m_descResizing)
        {
            auto *me = static_cast<QMouseEvent *>(event);
            int delta = static_cast<int>(me->globalPosition().y() - m_descResizeStartY);
            int newH = qMax(40, m_descStartHeight + delta);
            m_descriptionEdit->setFixedHeight(newH);
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease && m_descResizing)
        {
            m_descResizing = false;
            return true;
        }
    }
    return QGroupBox::eventFilter(obj, event);
}

void MeasurementDetailPanel::OnFieldChanged()
{
    if (m_populating) return;
    emit MeasurementChanged(BuildRecord());
}
