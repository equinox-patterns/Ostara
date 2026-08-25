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
#include "xml/ootudocument.h"
#include "xml/vabstractpattern.h"

#include <QFileInfo>
#include <QStringList>

namespace
{
/// Resolve a stored full name (or a legacy @Name) back to the @Name key.
/// Works on both OOmtDocument (.omt) and OOtuDocument (.otu): they share the
/// same measurement API without a common base class.
template <typename DocT> QString ResolveMNameFromFullName(const DocT *doc, const QString &fullName)
{
    if (doc == nullptr)
    {
        return QString();
    }

    const QStringList names = doc->GetAllMNames();
    // Legacy fallback: an old .oct may store the @Name directly.
    if (names.contains(fullName))
    {
        return fullName;
    }

    for (const QString &name : names)
    {
        if (doc->GetMFullName(name) == fullName)
        {
            return name;
        }
    }
    return QString();
}
}  // namespace

OPieceCartoucheTable::OPieceCartoucheTable() = default;

OPieceCartoucheTable::~OPieceCartoucheTable() = default;

void OPieceCartoucheTable::Clear()
{
    m_omtDoc.reset();
    m_otuDoc.reset();
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
    if (!fi.exists())
    {
        Clear();  // a missing table is "no table"
        return;
    }

    if (absolute == m_sourcePath && (m_omtDoc != nullptr || m_otuDoc != nullptr))
    {
        return;  // already parsed
    }

    Clear();  // new path (or a previously failed parse) — re-parse from scratch

    const QString suffix = fi.suffix().toLower();
    try
    {
        if (suffix == QStringLiteral("omt"))
        {
            auto table = std::make_unique<OOmtDocument>();
            table->setXMLContent(absolute);
            m_omtDoc = std::move(table);
            m_sourcePath = absolute;
        }
        else if (suffix == QStringLiteral("otu"))
        {
            auto table = std::make_unique<OOtuDocument>();
            table->setXMLContent(absolute);
            m_otuDoc = std::move(table);
            m_sourcePath = absolute;
        }
        // else: a legacy .vst/.vit or unknown table is "no table" — nothing cached.
    }
    catch (const VException &)
    {
        Clear();
    }
}

bool OPieceCartoucheTable::HasMultiSizeTable() const
{
    return m_omtDoc != nullptr;
}

bool OPieceCartoucheTable::HasTable() const
{
    return m_omtDoc != nullptr || m_otuDoc != nullptr;
}

QStringList OPieceCartoucheTable::AxisNames() const
{
    return m_omtDoc != nullptr ? m_omtDoc->GetSizeGroupNames() : QStringList();
}

QString OPieceCartoucheTable::AnchorValue(const QString &axisName) const
{
    return m_omtDoc != nullptr ? m_omtDoc->GetAnchorValue(axisName) : QString();
}

QStringList OPieceCartoucheTable::MeasurementNames() const
{
    if (m_omtDoc != nullptr)
    {
        return m_omtDoc->GetAllMNames();
    }
    return m_otuDoc != nullptr ? m_otuDoc->GetAllMNames() : QStringList();
}

QString OPieceCartoucheTable::MeasurementFullName(const QString &name) const
{
    if (m_omtDoc != nullptr)
    {
        return m_omtDoc->GetMFullName(name);
    }
    return m_otuDoc != nullptr ? m_otuDoc->GetMFullName(name) : QString();
}

QString OPieceCartoucheTable::MeasurementNameFromFullName(const QString &fullName) const
{
    if (fullName.isEmpty())
    {
        return QString();
    }

    if (m_omtDoc != nullptr)
    {
        return ResolveMNameFromFullName(m_omtDoc.get(), fullName);
    }
    return ResolveMNameFromFullName(m_otuDoc.get(), fullName);
}

double OPieceCartoucheTable::MeasurementValue(const QString &name) const
{
    if (m_omtDoc != nullptr)
    {
        return m_omtDoc->GetMValue(name);
    }
    return m_otuDoc != nullptr ? m_otuDoc->GetMValue(name) : 0.0;
}

QString OPieceCartoucheTable::UnitsStr() const
{
    if (m_omtDoc != nullptr)
    {
        return UnitsToStr(m_omtDoc->GetUnits(), true);
    }
    return m_otuDoc != nullptr ? UnitsToStr(m_otuDoc->GetUnits(), true) : QString();
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
