/************************************************************************
 **
 **  @file   DialogOpenMeasurements.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/07/2026
 **
 **  @brief  Dialog to open a measurement table — implementation.
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

#include "DialogOpenMeasurements.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

DialogOpenMeasurements::DialogOpenMeasurements(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Open a table"));
    setMinimumSize(550, 500);
    SetupUI();
    PopulateLists();
}

void DialogOpenMeasurements::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // --- Section 1: Multi-tailles from library ---
    auto *omtGroup = new QGroupBox(tr("Multi-size - Library (.omt)"));
    auto *omtLayout = new QVBoxLayout(omtGroup);

    m_omtList = new QListWidget();
    m_omtList->setAlternatingRowColors(true);
    m_omtList->setMinimumHeight(120);
    connect(m_omtList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_selectedPath = item->data(Qt::UserRole).toString();
        accept();
    });
    omtLayout->addWidget(m_omtList);

    auto *omtHint = new QLabel(tr("Double-click a table to open it."));
    omtHint->setObjectName(QStringLiteral("descLabel"));
    omtLayout->addWidget(omtHint);

    mainLayout->addWidget(omtGroup);

    // --- Section 2: Taille unique from library ---
    auto *otuGroup = new QGroupBox(tr("Single size - Library (.otu)"));
    auto *otuLayout = new QVBoxLayout(otuGroup);

    m_otuList = new QListWidget();
    m_otuList->setAlternatingRowColors(true);
    m_otuList->setMinimumHeight(120);
    connect(m_otuList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        m_selectedPath = item->data(Qt::UserRole).toString();
        accept();
    });
    otuLayout->addWidget(m_otuList);

    auto *otuHint = new QLabel(tr("Double-click a table to open it."));
    otuHint->setObjectName(QStringLiteral("descLabel"));
    otuLayout->addWidget(otuHint);

    mainLayout->addWidget(otuGroup);

    // --- Section 3: Browse ---
    auto *browseGroup = new QGroupBox(tr("Other location"));
    auto *browseLayout = new QVBoxLayout(browseGroup);

    auto *browseBtn = new QPushButton(tr("Browse folders…"));
    browseBtn->setMinimumHeight(40);
    connect(browseBtn, &QPushButton::clicked, this, &DialogOpenMeasurements::OnBrowse);
    browseLayout->addWidget(browseBtn);

    auto *browseHint = new QLabel(
        tr("Open a .omt, .otu, .vst, or .vit file from any folder."));
    browseHint->setWordWrap(true);
    browseHint->setObjectName(QStringLiteral("descLabel"));
    browseLayout->addWidget(browseHint);

    mainLayout->addWidget(browseGroup);

    // --- Cancel button ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void DialogOpenMeasurements::PopulateLists()
{
    const QString libPath = VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary();
    const QStringList favs = VAbstractApplication::VApp()->Settings()->GetMeasurementFavorites();
    const QList<LibraryFileEntry> entries = MeasurementLibraryScanner::ScanLibrary(libPath, favs);

    for (const auto &e : entries)
    {
        const bool isOmt = e.filePath.endsWith(QStringLiteral(".omt"), Qt::CaseInsensitive);
        const bool isOtu = e.filePath.endsWith(QStringLiteral(".otu"), Qt::CaseInsensitive);

        QListWidget *list = isOmt ? m_omtList : (isOtu ? m_otuList : nullptr);
        if (!list)
        {
            continue;
        }

        auto *item = new QListWidgetItem(e.displayName);
        item->setData(Qt::UserRole, e.filePath);
        item->setToolTip(e.filePath);
        list->addItem(item);
    }
}

void DialogOpenMeasurements::OnBrowse()
{
    QString filter = tr("Measurement tables (*.omt *.otu *.vst *.vit);;"
                        "All files (*)");
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open measurement table"),
        VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary(),
        filter);

    if (!path.isEmpty())
    {
        m_selectedPath = path;
        accept();
    }
}

auto DialogOpenMeasurements::selectedPath() const -> QString
{
    return m_selectedPath;
}
