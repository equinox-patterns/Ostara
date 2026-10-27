/************************************************************************
 **
 **  @file   vmodifierkey.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   14 2, 2019
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2019 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 17/08/2026.
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
#include "vmodifierkey.h"

#include <QKeyCombination>
#include <QKeySequence>
#include <QStringList>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto KeyToWord(int key) -> QString
{
    switch (key)
    {
        case int(Qt::ShiftModifier):
            return QStringLiteral("Shift");
        case int(Qt::ControlModifier):
#ifdef Q_OS_MAC
            return QStringLiteral("Cmd");
#else
            return QStringLiteral("Ctrl");
#endif
        case int(Qt::MetaModifier):
#ifdef Q_OS_MAC
            return QStringLiteral("Ctrl");
#else
            return QStringLiteral("Meta");
#endif
        case int(Qt::AltModifier):
#ifdef Q_OS_MAC
            return QStringLiteral("Option");
#else
            return QStringLiteral("Alt");
#endif
        case int(Qt::ControlModifier) | int(Qt::ShiftModifier):
#ifdef Q_OS_MAC
            return QStringLiteral("Cmd+Shift");
#else
            return QStringLiteral("Ctrl+Shift");
#endif
        case int(Qt::Key_Space):
            return QStringLiteral("Space");
        case int(Qt::Key_Return):
            return QStringLiteral("Return");
        case int(Qt::Key_Enter):
            return QStringLiteral("Enter");
        case int(Qt::Key_Backspace):
            return QStringLiteral("Backspace");
        case int(Qt::Key_Escape):
            return QStringLiteral("Esc");
        default:
            return QKeySequence(key).toString(QKeySequence::PortableText);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VModifierKey::VModifierKey(int key)
  : m_nativeText(KeyToWord(key))
{
}

//---------------------------------------------------------------------------------------------------------------------
VModifierKey::VModifierKey(Qt::KeyboardModifiers keys)
  : VModifierKey(static_cast<int>(keys))
{
    // nothing else
}

//---------------------------------------------------------------------------------------------------------------------
VModifierKey::VModifierKey(Qt::Key key)
  : VModifierKey(static_cast<int>(key))
{
    // nothing else
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Alt() -> const VModifierKey &
{
    static const VModifierKey key(Qt::AltModifier);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Control() -> const VModifierKey &
{
    static const VModifierKey key(Qt::ControlModifier);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::ControlShift() -> const VModifierKey &
{
    static const VModifierKey key(Qt::ControlModifier | Qt::ShiftModifier);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Meta() -> const VModifierKey &
{
    static const VModifierKey key(Qt::MetaModifier);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Shift() -> const VModifierKey &
{
    static const VModifierKey key(Qt::ShiftModifier);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Space() -> const VModifierKey &
{
    static const VModifierKey key(Qt::Key_Space);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::ReturnKey() -> const VModifierKey &
{
    static const VModifierKey key(Qt::Key_Return);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::EnterKey() -> const VModifierKey &
{
    static const VModifierKey key(Qt::Key_Enter);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Backspace() -> const VModifierKey &
{
    static const VModifierKey key(Qt::Key_Backspace);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Escape() -> const VModifierKey &
{
    static const VModifierKey key(Qt::Key_Escape);
    return key;
}

//---------------------------------------------------------------------------------------------------------------------
auto VModifierKey::Sequence(const QKeySequence &sequence) -> QString
{
    QStringList parts;
    parts.reserve(sequence.count());

    for (int i = 0; i < sequence.count(); ++i)
    {
        const QKeyCombination combination = sequence[static_cast<uint>(i)];
        QStringList tokens;

        const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
        if (modifiers & Qt::MetaModifier)
        {
            tokens.append(KeyToWord(int(Qt::MetaModifier)));
        }
        if (modifiers & Qt::ControlModifier)
        {
            tokens.append(KeyToWord(int(Qt::ControlModifier)));
        }
        if (modifiers & Qt::AltModifier)
        {
            tokens.append(KeyToWord(int(Qt::AltModifier)));
        }
        if (modifiers & Qt::ShiftModifier)
        {
            tokens.append(KeyToWord(int(Qt::ShiftModifier)));
        }

        tokens.append(QKeySequence(int(combination.key())).toString(QKeySequence::PortableText));
        parts.append(tokens.join("+"_L1));
    }

    return parts.join(", "_L1);
}
