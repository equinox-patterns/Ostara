/************************************************************************
 **
 **  @file   DialogMeasurementLink.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Dialog asking where to take the measurement table a pattern expects, when the table it
 **          points at cannot be used — implementation.
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

#include "DialogMeasurementLink.h"

#include <QDialogButtonBox>
#include <QFileInfo>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DialogMeasurementLink::DialogMeasurementLink(const QString &recordedPath, QWidget *parent)
    : QDialog(parent)
{
    // The linked file that is gone keeps the wording of the conversion offer it replaces; a pattern that never
    // carried a link has no legacy file to name, and saying "legacy" there would describe nothing.
    setWindowTitle(recordedPath.isEmpty() ? tr("Measurements table") : tr("Legacy measurement file"));

    auto *mainLayout = new QVBoxLayout(this);

    auto *text = new QLabel(this);
    text->setWordWrap(true);
    if (recordedPath.isEmpty())
    {
        text->setText(tr("This pattern reads measurements and no table is linked to it.\n\n"
                         "Ostara uses the .omt/.otu format. Choose how to proceed:"));
    }
    else
    {
        text->setText(tr("The pattern is linked to a legacy Tape file:\n\n%1\n\n"
                         "but it could not be found. Ostara uses the .omt/.otu format. Choose how to proceed:")
                          .arg(QFileInfo(recordedPath).fileName()));
    }
    mainLayout->addWidget(text);

    auto *buttonBox = new QDialogButtonBox(this);

    // "Convert this file" is deliberately not offered: there is nothing to convert until a file is found, and
    // searching for it is the gesture that replaces it. The library stays one click away, exactly as it is when
    // the linked file still exists.
    auto *searchBtn = buttonBox->addButton(tr("Search for the file"), QDialogButtonBox::AcceptRole);
    auto *libraryBtn = buttonBox->addButton(tr("Replace with an existing .omt/.otu"), QDialogButtonBox::ActionRole);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    connect(searchBtn, &QPushButton::clicked, this,
            [this]()
            {
                m_choice = Choice::SearchFile;
                accept();
            });
    connect(libraryBtn, &QPushButton::clicked, this,
            [this]()
            {
                m_choice = Choice::LibraryTable;
                accept();
            });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

auto DialogMeasurementLink::SelectedChoice() const -> Choice
{
    return m_choice;
}
