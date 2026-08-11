/************************************************************************
 **
 **  @file   vchangelinerolename.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   11 8, 2026
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2026 Valentina project
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
#include "vchangelinerolename.h"

#include <QDomElement>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../tools/vabstracttool.h"

//---------------------------------------------------------------------------------------------------------------------
VChangeLineRoleName::VChangeLineRoleName(
    VAbstractPattern *doc, QString pairKey, QString oldSuffix, QString newSuffix, QUndoCommand *parent)
  : VUndoCommand(doc, parent),
    m_pairKey(std::move(pairKey)),
    m_oldSuffix(std::move(oldSuffix)),
    m_newSuffix(std::move(newSuffix))
{
}

//---------------------------------------------------------------------------------------------------------------------
void VChangeLineRoleName::undo()
{
    UpdateDom(m_newSuffix, m_oldSuffix);
    emit NeedLiteParsing(Document::LiteParse);
}

//---------------------------------------------------------------------------------------------------------------------
void VChangeLineRoleName::redo()
{
    UpdateDom(m_oldSuffix, m_newSuffix);
    emit NeedLiteParsing(Document::LiteParse);
}

//---------------------------------------------------------------------------------------------------------------------
void VChangeLineRoleName::UpdateDom(const QString & /*oldSuffix*/, const QString &newSuffix)
{
    QDomElement root = Doc()->documentElement();
    if (root.isNull())
    {
        return;
    }

    // Find or create the <lineNames> element
    QDomElement lineNamesElement = root.firstChildElement(VAbstractPattern::TagLineNames);
    if (lineNamesElement.isNull())
    {
        if (newSuffix.isEmpty())
        {
            return; // Nothing to remove
        }
        lineNamesElement = Doc()->createElement(VAbstractPattern::TagLineNames);
        const QDomElement drawElement = root.firstChildElement(VAbstractPattern::TagDraw);
        if (!drawElement.isNull())
        {
            root.insertBefore(lineNamesElement, drawElement);
        }
        else
        {
            root.appendChild(lineNamesElement);
        }
    }

    // Convert pairKey "A_B" to points "A B" for the DOM attribute
    const QStringList pointNames = m_pairKey.split('_', Qt::SkipEmptyParts);
    if (pointNames.size() != 2)
    {
        return;
    }
    const QString points = pointNames.at(0) + ' ' + pointNames.at(1);

    // Find existing <line> child with matching points
    QDomElement existingLine = lineNamesElement.firstChildElement(VDomDocument::TagLine);
    while (!existingLine.isNull())
    {
        if (existingLine.attribute(AttrPoints) == points)
        {
            break;
        }
        existingLine = existingLine.nextSiblingElement(VDomDocument::TagLine);
    }

    if (newSuffix.isEmpty())
    {
        // Remove the entry
        if (!existingLine.isNull())
        {
            lineNamesElement.removeChild(existingLine);
        }
        // If <lineNames> is now empty, remove it too
        if (lineNamesElement.firstChildElement(VDomDocument::TagLine).isNull())
        {
            root.removeChild(lineNamesElement);
        }
    }
    else
    {
        if (!existingLine.isNull())
        {
            existingLine.setAttribute(AttrName, newSuffix);
        }
        else
        {
            QDomElement lineEl = Doc()->createElement(VDomDocument::TagLine);
            lineEl.setAttribute(AttrPoints, points);
            lineEl.setAttribute(AttrName, newSuffix);
            lineNamesElement.appendChild(lineEl);
        }
    }
}
