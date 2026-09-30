/************************************************************************
 **
 **  @file   DialogSaveMeasurements.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/07/2026
 **
 **  @brief  Save dialog for measurement tables — implementation.
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

#include "DialogSaveMeasurements.h"
#include "vcommonsettings.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

DialogSaveMeasurements::DialogSaveMeasurements(const QString &extension,
                                               const QString &defaultName,
                                               const QString &currentPath,
                                               QWidget *parent)
    : QDialog(parent)
    , m_extension(extension)
{
    setWindowTitle(tr("Save table"));
    setMinimumWidth(480);
    resize(480, 260);

    SetupUI();

    // Pre-fill name from current path or table name
    if (!currentPath.isEmpty())
    {
        QFileInfo fi(currentPath);
        QString base = fi.completeBaseName();
        if (!base.isEmpty())
        {
            m_nameEdit->setText(base);
        }
    }
    else if (!defaultName.isEmpty())
    {
        m_nameEdit->setText(defaultName);
    }

    UpdateSaveButton();
}

void DialogSaveMeasurements::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // --- Explanatory text ---
    auto *explainLabel = new QLabel(
        tr("Save your table to the Ostara library to easily find it "
           "in the My Tables list."));
    explainLabel->setWordWrap(true);
    mainLayout->addWidget(explainLabel);

    // --- Name field ---
    auto *nameLayout = new QHBoxLayout();
    nameLayout->setSpacing(8);
    auto *nameLabel = new QLabel(tr("Name:"));
    nameLayout->addWidget(nameLabel);

    // Name + extension in a frame (visually one field)
    auto *nameFrame = new QFrame();
    nameFrame->setObjectName(QStringLiteral("saveNameFrame"));
    auto *nameFrameLayout = new QHBoxLayout(nameFrame);
    nameFrameLayout->setContentsMargins(0, 0, 0, 0);
    nameFrameLayout->setSpacing(0);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setObjectName(QStringLiteral("saveNameEdit"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, &DialogSaveMeasurements::OnNameChanged);
    nameFrameLayout->addWidget(m_nameEdit, 1);

    auto *extLabel = new QLabel(m_extension);
    extLabel->setObjectName(QStringLiteral("saveNameExt"));
    extLabel->setEnabled(false); // visually muted
    nameFrameLayout->addWidget(extLabel);

    nameLayout->addWidget(nameFrame, 1);
    mainLayout->addLayout(nameLayout);

    // --- Library hint ---
    // Kept as it was: the folder will be settable from Settings later, and this label already announces it.
    auto *libHint = new QLabel(
        tr("→ Find this table in the My Tables list, without searching your disk. "
           "Folder managed from Preferences."));
    libHint->setWordWrap(true);
    libHint->setObjectName(QStringLiteral("saveHint"));
    mainLayout->addWidget(libHint);

    mainLayout->addSpacing(8);

    // --- Buttons ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);
    m_saveBtn = buttonBox->button(QDialogButtonBox::Save);
    m_saveBtn->setText(tr("Save"));
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

// --- Accessors ---

auto DialogSaveMeasurements::fileName() const -> QString
{
    QString base = m_nameEdit->text().trimmed();
    if (base.isEmpty())
    {
        return {};
    }
    // Extension is already part of the name concept — append it
    return base + m_extension;
}

// --- Slots ---

void DialogSaveMeasurements::OnNameChanged()
{
    UpdateSaveButton();
}

void DialogSaveMeasurements::UpdateSaveButton()
{
    if (!m_saveBtn)
    {
        return;
    }

    // The name is the only thing left to decide: the destination is the library.
    m_saveBtn->setEnabled(!m_nameEdit->text().trimmed().isEmpty());
}
