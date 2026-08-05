/************************************************************************
 **
 **  @file   vlinecolorproperty.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   7 2, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "vlinecolorproperty.h"

#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

#include "../../ifc/ifcdef.h"
#include "../vproperty_p.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-member-function")

Q_GLOBAL_STATIC_WITH_ARGS(const QString, defaultColorName, ("default"_L1)) // NOLINT

// Return true if the application palette indicates dark mode (avoids cross-dylib VTheme call).
auto IsDarkMode() -> bool
{
    return qApp->palette().color(QPalette::Window).lightness() < 128;
}

QT_WARNING_POP
} // namespace

// Out-of-line destructor: keeps the scalar deleting destructor inside the DLL on MSVC,
// preventing heap corruption when deleting via a base-class pointer from a consuming module.
VPE::VLineColorProperty::~VLineColorProperty() = default;

VPE::VLineColorProperty::VLineColorProperty(const QString &name)
  : VProperty(name,
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
              QMetaType::QString)
#else
              QVariant::String)
#endif
{
}

auto VPE::VLineColorProperty::data(int column, int role) const -> QVariant
{
    if (column == DPC_Data && (Qt::DisplayRole == role || Qt::EditRole == role))
    {
        // Return the color constant name (e.g. "darkBlue") for both roles.
        // The combo handles the translated display name internally.
        return vproperty_d_ptr->VariantValue.toString();
    }

    return VProperty::data(column, role);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::createEditor(QWidget *parent, const QStyleOptionViewItem &options,
                                           const QAbstractItemDelegate *delegate) -> QWidget *
{
    Q_UNUSED(options)
    Q_UNUSED(delegate)

    auto *combo = new QComboBox(parent);
    const bool dark = IsDarkMode();
    const int iconSize = combo->style()->pixelMetric(QStyle::PM_SmallIconSize);

    QMap<QString, QString> colors = defaultColors;

    // "default" entry (for operations: no specific color)
    if (colors.contains(*defaultColorName))
    {
        combo->addItem(colors.value(*defaultColorName), *defaultColorName);
        colors.remove(*defaultColorName);
    }

    auto i = colors.constBegin();
    while (i != colors.constEnd())
    {
        const QColor swatch = ColorNameToQColorForTheme(i.key(), dark);
        QString displayName = i.value();
        if (dark && i.key() == ColorBlack)
        {
            displayName = QCoreApplication::translate("VLineColorProperty", "White");
        }

        QPixmap pix(iconSize, iconSize);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(swatch);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(1, 1, iconSize - 2, iconSize - 2, 3, 3);
        painter.end();

        combo->addItem(QIcon(pix), displayName);
        combo->setItemData(combo->count() - 1, i.key(), Qt::UserRole);
        ++i;
    }

    // Set current selection from the stored value.
    const QString storedName = vproperty_d_ptr->VariantValue.toString();
    int idx = -1;
    if (storedName == *defaultColorName || storedName.isEmpty())
    {
        idx = combo->findData(*defaultColorName, Qt::UserRole);
    }
    else
    {
        idx = combo->findData(storedName, Qt::UserRole);
        if (idx < 0)
        {
            // Try legacy name → hex → Ostara name.
            QColor c = ColorNameToQColor(storedName);
            if (!c.isValid())
            {
                c = QColor(storedName);
            }
            if (c.isValid())
            {
                const QString baseName = ColorNameFromQColor(c);
                idx = combo->findData(baseName, Qt::UserRole);
            }
        }
    }
    if (idx < 0)
    {
        idx = 0; // fallback: first item
    }
    combo->setCurrentIndex(idx);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &VLineColorProperty::currentColorChanged);

    VProperty::vproperty_d_ptr->editor = combo;
    return VProperty::vproperty_d_ptr->editor;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::setEditorData(QWidget *editor) -> bool
{
    if (auto *combo = qobject_cast<QComboBox *>(editor); combo != nullptr)
    {
        const QString value = vproperty_d_ptr->VariantValue.toString();

        // 1. Exact match on color constant name (e.g. "black", "darkBlue").
        int idx = combo->findData(value, Qt::UserRole);

        // 2. Translated display name → color constant (e.g. "bleu foncé" → "darkBlue").
        if (idx < 0)
        {
            auto di = defaultColors.constBegin();
            while (di != defaultColors.constEnd())
            {
                if (di.value() == value)
                {
                    idx = combo->findData(di.key(), Qt::UserRole);
                    break;
                }
                ++di;
            }
        }

        // 3. Legacy hex or SVG name → color constant (e.g. "#000000" → "black").
        if (idx < 0)
        {
            QColor c = ColorNameToQColor(value);
            if (!c.isValid())
            {
                c = QColor(value);
            }
            if (c.isValid())
            {
                const QString baseName = ColorNameFromQColor(c);
                if (!baseName.isEmpty())
                {
                    idx = combo->findData(baseName, Qt::UserRole);
                }
            }
        }
        if (idx >= 0 && idx != combo->currentIndex())
        {
            combo->blockSignals(true);
            combo->setCurrentIndex(idx);
            combo->blockSignals(false);
        }
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::getEditorData(const QWidget *editor) const -> QVariant
{
    if (const auto *combo = qobject_cast<const QComboBox *>(editor); combo != nullptr)
    {
        return combo->currentData(Qt::UserRole);
    }
    return *defaultColorName;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::getSetting(const QString &key) const -> QVariant
{
    Q_UNUSED(key)
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::getSettingKeys() const -> QStringList
{
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::type() const -> QString
{
    return QStringLiteral("lineColor");
}

//---------------------------------------------------------------------------------------------------------------------
auto VPE::VLineColorProperty::clone(bool include_children, VProperty *container) const -> VPE::VProperty *
{
    return VProperty::clone(include_children, container ? container : new VLineColorProperty(getName()));
}

//---------------------------------------------------------------------------------------------------------------------
void VPE::VLineColorProperty::currentColorChanged(int /*index*/)
{
    auto *event = new UserChangeEvent();
    QCoreApplication::postEvent(VProperty::vproperty_d_ptr->editor, event);
}

//---------------------------------------------------------------------------------------------------------------------
void VPE::VLineColorProperty::SetUseNativeDialog(bool newUseNativeDialog)
{
    useNativeDialog = newUseNativeDialog;
}

//---------------------------------------------------------------------------------------------------------------------
void VPE::VLineColorProperty::SetCustomColors(const QVector<QColor> &newCustomColors)
{
    customColors = newCustomColors;
}

//---------------------------------------------------------------------------------------------------------------------
void VPE::VLineColorProperty::SetDefaultColors(const QMap<QString, QString> &newDefaultColors)
{
    defaultColors = newDefaultColors;
}
