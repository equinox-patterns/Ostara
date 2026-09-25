/************************************************************************
 **
 **  @file   ofontfaces.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  The faces a font family really has — implementation.
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

#include "ofontfaces.h"

#include <QComboBox>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QSignalBlocker>
#include <QStringList>

//---------------------------------------------------------------------------------------------------------------------
auto FontFaceFromWeight(const QString &family, int weight) -> QString
{
    if (family.isEmpty() || weight <= 0)
    {
        return QString();
    }
    // The probe is a font nobody draws with: it is asked what Qt answers for that weight, which is the
    // only authority on which face a number names.
    QFont probe(family);
    probe.setWeight(QFont::Weight(weight));
    return QFontInfo(probe).styleName();
}

//---------------------------------------------------------------------------------------------------------------------
void FillFontFaces(QComboBox *combo, const QString &family, const QString &stored)
{
    if (combo == nullptr)
    {
        return;
    }

    const QStringList all = QFontDatabase::styles(family);
    if (all.isEmpty())
    {
        return; // a family with no face to offer keeps what it had
    }

    const auto isItalic = [](const QString &face) {
        return face.contains(QStringLiteral("italic"), Qt::CaseInsensitive)
               || face.contains(QStringLiteral("oblique"), Qt::CaseInsensitive);
    };
    QStringList roman;
    for (const QString &face : all)
    {
        if (!isItalic(face))
        {
            roman.append(face);
        }
    }
    const QStringList faces = roman.isEmpty() ? all : roman;

    QSignalBlocker block(combo); // filling the list is not the user answering it
    const QString kept = combo->currentData().toString();
    combo->clear();
    for (const QString &face : faces)
    {
        combo->addItem(face, face);
    }

    const QString want = kept.isEmpty() ? stored : kept;
    int at = want.isEmpty() ? -1 : combo->findData(want);
    bool numbered = false;
    const int weight = want.toInt(&numbered);
    if (at < 0 && numbered && weight > 0)
    {
        // An old file, and the face its number really resolves to — the same answer the drawing
        // gets, so the list cannot show one face while the page is drawn in another.
        at = combo->findText(FontFaceFromWeight(family, weight));
    }
    if (at < 0)
    {
        at = combo->findText(QStringLiteral("Regular"));
    }
    combo->setCurrentIndex(at < 0 ? 0 : at);
}
