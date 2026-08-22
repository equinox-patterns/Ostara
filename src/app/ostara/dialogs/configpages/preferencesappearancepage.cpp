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
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vvalentinasettings.h"

#include <QComboBox>
#include <QFontComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
struct FoldLineWeight
{
    QFont::Weight weight;
    const char *label;
};
constexpr FoldLineWeight kFoldLineWeights[] = {
    {QFont::Thin,       "Thin"},
    {QFont::ExtraLight, "Extra Light"},
    {QFont::Light,      "Light"},
    {QFont::Normal,     "Normal"},
    {QFont::Medium,     "Medium"},
    {QFont::DemiBold,   "Demi Bold"},
    {QFont::Bold,       "Bold"},
    {QFont::ExtraBold,  "Extra Bold"},
    {QFont::Black,      "Black"},
};
} // namespace

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

    InitFoldLineText();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesAppearancePage::InitFoldLineText()
{
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    m_fontFamily->setCurrentFont(QFont(settings->GetFoldLineLabelFontFamily()));

    m_weightCombo->clear();
    for (const auto &w : kFoldLineWeights)
    {
        m_weightCombo->addItem(tr(w.label), static_cast<int>(w.weight));
    }
    const int currentWeight = static_cast<int>(settings->GetFoldLineLabelFontWeight());
    const qint32 idx = m_weightCombo->findData(currentWeight);
    m_weightCombo->setCurrentIndex(idx != -1 ? idx : m_weightCombo->findData(static_cast<int>(QFont::Normal)));

    m_sizeSpin->setValue(settings->GetFoldLineLabelFontPixelSize());
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesAppearancePage::Apply() -> QStringList
{
    QStringList options;
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetFoldLineLabelFontFamily(m_fontFamily->currentFont().family());
    settings->SetFoldLineLabelFontWeight(static_cast<QFont::Weight>(m_weightCombo->currentData().toInt()));
    settings->SetFoldLineLabelFontPixelSize(m_sizeSpin->value());
    return options;
}
