/************************************************************************
 **
 **  @file   angleformulawidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/08/2026
 **
 **  @brief  Composite wrapping a FormulaInputWidget with a row of preset
 **          angle buttons (0/90/180/270°) placed below the formula field.
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
#include "angleformulawidget.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QVBoxLayout>

#include "formulainputwidget.h"
#include "themeDef.h"
#include "vmisc/literals.h"
#include "vtheme.h"

namespace
{
// Preset angles, ordered counter-clockwise (0° right, 90° up, 180° left, 270° down).
// Each maps to its arrow icon base name ("right"/"up"/"left"/"down").
struct AnglePreset
{
    int angle;
    const char *arrow;
};

const AnglePreset kPresets[] = {
    {0, "right"},
    {90, "up"},
    {180, "left"},
    {270, "down"},
};

// Rest icon: theme-appropriate (light theme -> light set, dark theme -> dark set).
auto restIconPath(const QString &arrow) -> QString
{
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const QString theme = isDark ? QStringLiteral("dark") : QStringLiteral("light");
    return QStringLiteral(":/icons/%1/arrow-%2_%1.svg").arg(theme, arrow);
}

// Hover icon: dark regular for both themes (the button background becomes accent).
auto hoverIconPath(const QString &arrow) -> QString
{
    return QStringLiteral(":/icons/dark/arrow-%1_dark.svg").arg(arrow);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
AngleFormulaWidget::AngleFormulaWidget(QWidget *parent)
  : QWidget(parent)
{
    buildUi();
    // Keep the composite's height fixed at its content (formula field + preset row),
    // so it never stretches and leaves the leftover space to the notes field.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::buildUi()
{
    m_formulaInput = new FormulaInputWidget(this);

    auto *presetLayout = new QHBoxLayout;
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetLayout->setSpacing(4);

    for (const AnglePreset &preset : kPresets)
    {
        const QString arrow = QString::fromLatin1(preset.arrow);

        auto *button = new QPushButton(QString::number(preset.angle) + degreeSymbol, this);
        button->setObjectName(QStringLiteral("anglePresetBtn") + QString::number(preset.angle));
        button->setIcon(QIcon(restIconPath(arrow)));
        // Icon to the right of the text ("0° →"): reverse the layout direction.
        button->setLayoutDirection(Qt::RightToLeft);
        button->setProperty("arrow", arrow);
        button->installEventFilter(this);
        connect(button, &QPushButton::clicked, this, [this, angle = preset.angle]() { setPresetAngle(angle); });
        presetLayout->addWidget(button);
    }

    m_presetRow = new QWidget(this);
    m_presetRow->setLayout(presetLayout);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);
    mainLayout->addWidget(m_formulaInput);
    mainLayout->addWidget(m_presetRow);
    setLayout(mainLayout);

    // Re-emit the inner valueChanged so dialogs connect to the composite exactly as before.
    connect(m_formulaInput, &FormulaInputWidget::valueChanged, this, &AngleFormulaWidget::valueChanged);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setPresetAngle(int angle)
{
    m_formulaInput->setFormula(QString::number(angle));
}

//---------------------------------------------------------------------------------------------------------------------
auto AngleFormulaWidget::eventFilter(QObject *object, QEvent *event) -> bool
{
    if (event->type() == QEvent::Enter || event->type() == QEvent::Leave)
    {
        if (auto *button = qobject_cast<QPushButton *>(object))
        {
            const QString arrow = button->property("arrow").toString();
            button->setIcon(QIcon(event->type() == QEvent::Enter ? hoverIconPath(arrow) : restIconPath(arrow)));
        }
    }
    return QWidget::eventFilter(object, event);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        m_presetRow->setVisible(isEnabled());
    }
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setData(const VContainer *data)
{
    m_formulaInput->setData(data);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setToolId(quint32 id)
{
    m_formulaInput->setToolId(id);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setPostfix(const QString &postfix)
{
    m_formulaInput->setPostfix(postfix);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setLabelText(const QString &text)
{
    m_formulaInput->setLabelText(text);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setDialogTitle(const QString &title)
{
    m_formulaInput->setDialogTitle(title);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setCheckZero(bool check)
{
    m_formulaInput->setCheckZero(check);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setCheckLessThanZero(bool check)
{
    m_formulaInput->setCheckLessThanZero(check);
}

//---------------------------------------------------------------------------------------------------------------------
void AngleFormulaWidget::setFormula(const QString &formula)
{
    m_formulaInput->setFormula(formula);
}

//---------------------------------------------------------------------------------------------------------------------
auto AngleFormulaWidget::formula() const -> QString
{
    return m_formulaInput->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto AngleFormulaWidget::value() const -> qreal
{
    return m_formulaInput->value();
}

//---------------------------------------------------------------------------------------------------------------------
auto AngleFormulaWidget::isValid() const -> bool
{
    return m_formulaInput->isValid();
}
