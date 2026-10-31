/************************************************************************
 **
 **  @file   DialogNewMeasurements.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Dialog for creating a new measurement table (Solo/Multi + unit + template).
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

#include "DialogNewMeasurements.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "vmisc/FactoryWidgets.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

DialogNewMeasurements::DialogNewMeasurements(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New measurement table"));
    setMinimumWidth(400);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // --- Table name ---
    auto *nameGroup = new QGroupBox(tr("Table name"));
    auto *nameLayout = new QVBoxLayout(nameGroup);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(tr("e.g., Standard measurements T38-60"));
    nameLayout->addWidget(m_nameEdit);

    mainLayout->addWidget(nameGroup);

    // --- Type selection ---
    auto *typeGroup = new QGroupBox(tr("Table type"));
    auto *typeLayout = new QVBoxLayout(typeGroup);

    m_typeGroup = new QButtonGroup(this);

    m_multiRadio = new QRadioButton(tr("Multi-size"));
    m_multiRadio->setChecked(true);
    m_typeGroup->addButton(m_multiRadio, 0);
    typeLayout->addWidget(m_multiRadio);

    auto *multiDesc = new QLabel(tr("Table with size groups (e.g., 32-58, 3 heights). "
                                     "Values are calculated automatically by increments."));
    multiDesc->setWordWrap(true);
    multiDesc->setObjectName(QStringLiteral("descLabel"));
    typeLayout->addWidget(multiDesc);

    m_soloRadio = new QRadioButton(tr("Single size"));
    m_typeGroup->addButton(m_soloRadio, 1);
    typeLayout->addWidget(m_soloRadio);

    auto *soloDesc = new QLabel(tr("Single size, no calculation. "
                                   "You enter each measurement value directly."));
    soloDesc->setWordWrap(true);
    soloDesc->setObjectName(QStringLiteral("descLabel"));
    typeLayout->addWidget(soloDesc);

    mainLayout->addWidget(typeGroup);

    // --- Unit selection ---
    auto *unitGroup = new QGroupBox(tr("Unit"));
    auto *unitLayout = new QVBoxLayout(unitGroup);

    m_unitGroup = new QButtonGroup(this);

    m_cmRadio = new QRadioButton(tr("Centimeters (cm)"));
    m_cmRadio->setChecked(true);
    m_unitGroup->addButton(m_cmRadio, 0);
    unitLayout->addWidget(m_cmRadio);

    m_mmRadio = new QRadioButton(tr("Millimeters (mm)"));
    m_unitGroup->addButton(m_mmRadio, 1);
    unitLayout->addWidget(m_mmRadio);

    m_inchRadio = new QRadioButton(tr("Inches (inch)"));
    m_unitGroup->addButton(m_inchRadio, 2);
    unitLayout->addWidget(m_inchRadio);

    mainLayout->addWidget(unitGroup);

    // --- Template selection ---
    auto *templateGroup = new QGroupBox(tr("Template (optional)"));
    auto *templateLayout = new QVBoxLayout(templateGroup);

    auto *templateDesc = new QLabel(tr("Start from an existing table. "
                                        "Only measurement names will be reused, without values."));
    templateDesc->setWordWrap(true);
    templateDesc->setObjectName(QStringLiteral("descLabel"));
    templateLayout->addWidget(templateDesc);

    m_templateCombo = MakeComboBox();
    m_templateCombo->addItem(tr("No template"));

    const QString libPath = VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary();
    const QStringList favs = VAbstractApplication::VApp()->Settings()->GetMeasurementFavorites();
    const QList<LibraryFileEntry> entries = MeasurementLibraryScanner::ScanLibrary(libPath, favs);
    for (const auto &e : entries)
    {
        m_templateCombo->addItem(e.displayName);
        m_templatePaths.append(e.filePath);
    }

    templateLayout->addWidget(m_templateCombo);

    mainLayout->addWidget(templateGroup);

    // --- Buttons ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty())
        {
            QMessageBox::warning(this, tr("Name required"),
                                 tr("Please name your table."));
            m_nameEdit->setFocus();
            return;
        }
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    mainLayout->addStretch();
}

auto DialogNewMeasurements::MeasurementsType() const -> ::MeasurementsType
{
    if (m_multiRadio->isChecked())
    {
        return MeasurementsType::Multisize;
    }
    return MeasurementsType::Individual;
}

auto DialogNewMeasurements::Unit() const -> ::Unit
{
    int id = m_unitGroup->checkedId();
    switch (id)
    {
        case 0: return Unit::Cm;
        case 1: return Unit::Mm;
        case 2: return Unit::Inch;
    }
    return Unit::Cm;
}

auto DialogNewMeasurements::TemplatePath() const -> QString
{
    int idx = m_templateCombo->currentIndex();
    if (idx <= 0 || idx > m_templatePaths.size())
    {
        return {};
    }
    return m_templatePaths.at(idx - 1);
}

auto DialogNewMeasurements::UseTemplate() const -> bool
{
    return m_templateCombo->currentIndex() > 0;
}

auto DialogNewMeasurements::TableName() const -> QString
{
    return m_nameEdit->text().trimmed();
}
