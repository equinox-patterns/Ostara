/************************************************************************
 **
 **  @file   ogradedalignwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Graded alignment selector (horizontal + vertical, 3 buttons each).
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

#include "ogradedalignwidget.h"

#include "../vtheme/vtheme.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSize>
#include <QVBoxLayout>

namespace
{

struct AlignItem
{
    int value;
    const char *text;
};

// Shared by the constructor — QT_TRANSLATE_NOOP lets lupdate extract them.
const AlignItem kAlignHItems[] = {
    {1, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Left")},
    {2, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Center")},
    {3, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Right")},
};

const AlignItem kAlignVItems[] = {
    {1, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Top")},
    {2, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Center")},
    {3, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Bottom")},
};

// Graded alignment icons: regular (outline) for the normal state, fill for the checked state.
// QCommonStyle renders a checked QPushButton icon in QIcon::On, unchecked in QIcon::Off.
auto MakeAlignIcon(const QString &baseName, bool dark) -> QIcon
{
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, baseName));                    // Off = regular
    icon.addFile(QStringLiteral(":/icons/%1/%2-fill_%1.svg").arg(theme, baseName), QSize(),
                 QIcon::Normal, QIcon::On);                                                       // On = fill
    return icon;
}

// alignValue 1/2/3 → Left/Center/Right horizontally, Top/Center/Bottom vertically.
auto AlignIconBaseName(bool vertical, int value) -> QString
{
    switch (value)
    {
        case 1:
            return vertical ? QStringLiteral("align-top-simple") : QStringLiteral("align-left-simple");
        case 3:
            return vertical ? QStringLiteral("align-bottom-simple") : QStringLiteral("align-right-simple");
        default:
            return vertical ? QStringLiteral("align-center-vertical-simple")
                            : QStringLiteral("align-center-horizontal-simple");
    }
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OGradedAlignWidget::OGradedAlignWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    auto *groupBox = new QGroupBox(tr("Graded alignment"));
    auto *alignLayout = new QVBoxLayout(groupBox);

    // Horizontal
    m_alignHLabel = new QLabel(tr("Horizontal"));
    auto *hLayout = new QHBoxLayout();
    m_alignHGroup = new QButtonGroup(this);
    m_alignHGroup->setExclusive(true);

    for (int i = 0; i < 3; ++i)
    {
        auto *btn = new QPushButton(tr(kAlignHItems[i].text));
        btn->setCheckable(true);
        btn->setAutoDefault(false); // never the dialog's default button
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setProperty("alignValue", kAlignHItems[i].value);
        m_alignHGroup->addButton(btn, kAlignHItems[i].value);
        hLayout->addWidget(btn);
        m_alignHBtns[i] = btn;
    }
    alignLayout->addWidget(m_alignHLabel);
    alignLayout->addLayout(hLayout);

    // Vertical
    m_alignVLabel = new QLabel(tr("Vertical"));
    auto *vLayout = new QHBoxLayout();
    m_alignVGroup = new QButtonGroup(this);
    m_alignVGroup->setExclusive(true);

    for (int i = 0; i < 3; ++i)
    {
        auto *btn = new QPushButton(tr(kAlignVItems[i].text));
        btn->setCheckable(true);
        btn->setAutoDefault(false); // never the dialog's default button
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setProperty("alignValue", kAlignVItems[i].value);
        m_alignVGroup->addButton(btn, kAlignVItems[i].value);
        vLayout->addWidget(btn);
        m_alignVBtns[i] = btn;
    }
    alignLayout->addWidget(m_alignVLabel);
    alignLayout->addLayout(vLayout);

    mainLayout->addWidget(groupBox);

    RefreshAlignIcons();
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &OGradedAlignWidget::RefreshAlignIcons);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedAlignWidget::RefreshAlignIcons()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QSize iconSize(16, 16);
    for (int i = 0; i < 3; ++i)
    {
        m_alignHBtns[i]->setIcon(MakeAlignIcon(AlignIconBaseName(false, i + 1), dark));
        m_alignHBtns[i]->setIconSize(iconSize);
        m_alignVBtns[i]->setIcon(MakeAlignIcon(AlignIconBaseName(true, i + 1), dark));
        m_alignVBtns[i]->setIconSize(iconSize);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedAlignWidget::SetAlignH(int value)
{
    if (QAbstractButton *btn = m_alignHGroup->button(value); btn != nullptr)
    {
        btn->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedAlignWidget::GetAlignH() const -> int
{
    const int id = m_alignHGroup->checkedId();
    return id >= 0 ? id : 2;
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedAlignWidget::SetAlignV(int value)
{
    if (QAbstractButton *btn = m_alignVGroup->button(value); btn != nullptr)
    {
        btn->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedAlignWidget::GetAlignV() const -> int
{
    const int id = m_alignVGroup->checkedId();
    return id >= 0 ? id : 2;
}
