/************************************************************************
 **
 **  @file   OPieceCartoucheTable.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/08/2026
 **
 **  @brief  Read-only facade over the pattern's loaded .omt size table.
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

#include "OPieceCartoucheTable.h"

#include "def.h"
#include "vabstractvalapplication.h"
#include "vexception.h"
#include "xml/oomtdocument.h"
#include "xml/vabstractpattern.h"

#include <QFileInfo>
#include <QStringList>

OPieceCartoucheTable::OPieceCartoucheTable() = default;

OPieceCartoucheTable::~OPieceCartoucheTable() = default;

void OPieceCartoucheTable::Clear()
{
    m_doc.reset();
    m_sourcePath.clear();
}

void OPieceCartoucheTable::Load()
{
    auto *app = VAbstractValApplication::VApp();
    VAbstractPattern *doc = app != nullptr ? app->getCurrentDocument() : nullptr;
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        Clear();
        return;
    }

    const QString absolute = AbsoluteMPath(app->GetPatternPath(), doc->MPath());
    const QFileInfo fi(absolute);
    if (fi.suffix().toLower() != QStringLiteral("omt") || !fi.exists())
    {
        // A single-size .otu table or a missing file is "no multi-size table".
        Clear();
        return;
    }

    if (absolute == m_sourcePath && m_doc != nullptr)
    {
        return;  // already parsed
    }

    try
    {
        auto table = std::make_unique<OOmtDocument>();
        table->setXMLContent(absolute);
        m_doc = std::move(table);
        m_sourcePath = absolute;
    }
    catch (const VException &)
    {
        Clear();
    }
}

bool OPieceCartoucheTable::HasMultiSizeTable() const
{
    return m_doc != nullptr;
}

QStringList OPieceCartoucheTable::AxisNames() const
{
    return m_doc != nullptr ? m_doc->GetSizeGroupNames() : QStringList();
}

QString OPieceCartoucheTable::AnchorValue(const QString &axisName) const
{
    return m_doc != nullptr ? m_doc->GetAnchorValue(axisName) : QString();
}

QStringList OPieceCartoucheTable::MeasurementNames() const
{
    return m_doc != nullptr ? m_doc->GetAllMNames() : QStringList();
}

QString OPieceCartoucheTable::MeasurementFullName(const QString &name) const
{
    return m_doc != nullptr ? m_doc->GetMFullName(name) : QString();
}

QString OPieceCartoucheTable::MeasurementNameFromFullName(const QString &fullName) const
{
    if (m_doc == nullptr || fullName.isEmpty())
    {
        return QString();
    }

    const QStringList names = m_doc->GetAllMNames();
    // Legacy fallback: an old .oct may store the @Name directly.
    if (names.contains(fullName))
    {
        return fullName;
    }

    for (const QString &name : names)
    {
        if (m_doc->GetMFullName(name) == fullName)
        {
            return name;
        }
    }
    return QString();
}

double OPieceCartoucheTable::MeasurementValue(const QString &name) const
{
    return m_doc != nullptr ? m_doc->GetMValue(name) : 0.0;
}

QString OPieceCartoucheTable::UnitsStr() const
{
    return m_doc != nullptr ? UnitsToStr(m_doc->GetUnits(), true) : QString();
}

QString OPieceCartoucheTable::FormatValue(double value)
{
    QString text = QString::number(value, 'f', 2);
    while (text.endsWith(QLatin1Char('0')))
    {
        text.chop(1);
    }
    if (text.endsWith(QLatin1Char('.')))
    {
        text.chop(1);
    }
    if (text.isEmpty() || text == QStringLiteral("-0"))
    {
        text = QStringLiteral("0");
    }
    return text;
}
