/************************************************************************
 **
 **  @file   preferencesappearancepage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22 August, 2026
 **
 **  @brief  Label appearance preferences page (Foldline text)
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

#include "preferencesappearancepage.h"

#include "../../core/vapplication.h"
#include "../../core/ofontfaces.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vvalentinasettings.h"

#include <QComboBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
PreferencesAppearancePage::PreferencesAppearancePage(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);

    auto *group = new QGroupBox(tr("Foldline text"));
    auto *grid = new QGridLayout(group);

    m_fontFamily = new QFontComboBox();
    m_weightCombo = MakeComboBox();
    m_sizeSpin = new QSpinBox();
    m_sizeSpin->setRange(5, 200);
    m_sizeSpin->setSuffix(tr(" px"));

    // Labels on the first line, inputs below (3 columns).
    grid->addWidget(new QLabel(tr("Family")), 0, 0);
    grid->addWidget(new QLabel(tr("Weight")), 0, 1);
    grid->addWidget(new QLabel(tr("Size")), 0, 2);
    grid->addWidget(m_fontFamily, 1, 0);
    grid->addWidget(m_weightCombo, 1, 1);
    grid->addWidget(m_sizeSpin, 1, 2);

    mainLayout->addWidget(group);
    mainLayout->addStretch();

    // **The faces belong to the family, so they change with it** — a list still offering the faces of
    // the family chosen before it would name none of the ones the new family really has.
    connect(m_fontFamily, &QFontComboBox::currentFontChanged, this, [this](const QFont &f) {
        FillFontFaces(m_weightCombo, f.family(), QString());
    });

    InitFoldLineText();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesAppearancePage::InitFoldLineText()
{
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    const QString family = settings->GetFoldLineLabelFontFamily();
    m_fontFamily->setCurrentFont(QFont(family));

    // **Only the faces the family really has, italics excluded** — see @ref FillFontFaces. The stored
    // answer is a face NAME; a settings file written before it was one holds a number, and FillFontFaces
    // resolves that number to the face it really draws — the same answer the drawing gets, so the list
    // cannot show one face while the label is drawn in another.
    const QString face = settings->GetFoldLineLabelFontFace();
    FillFontFaces(m_weightCombo, family,
                  face.isEmpty() ? QString::number(static_cast<int>(settings->GetFoldLineLabelFontWeight()))
                                 : face);

    m_sizeSpin->setValue(settings->GetFoldLineLabelFontPixelSize());
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesAppearancePage::Apply() -> QStringList
{
    QStringList options;
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetFoldLineLabelFontFamily(m_fontFamily->currentFont().family());
    settings->SetFoldLineLabelFontFace(m_weightCombo->currentData().toString());
    settings->SetFoldLineLabelFontPixelSize(m_sizeSpin->value());
    return options;
}
