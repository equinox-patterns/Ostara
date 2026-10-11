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
#include <QDomNamedNodeMap>
#include <utility>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "orewriteformulas.h"
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
    ReplaceTokensInFormulas(m_newSuffix, m_oldSuffix);
    emit NeedLiteParsing(Document::LiteParse);
}

//---------------------------------------------------------------------------------------------------------------------
void VChangeLineRoleName::redo()
{
    UpdateDom(m_oldSuffix, m_newSuffix);
    ReplaceTokensInFormulas(m_oldSuffix, m_newSuffix);
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

//---------------------------------------------------------------------------------------------------------------------
void VChangeLineRoleName::ReplaceTokensInFormulas(const QString &oldSuffix, const QString &newSuffix)
{
    // What a formula may be holding for this line, and what it has to hold afterwards. A line nobody named answers
    // to the pair of points it joins — `Line_C7_C8` — and that pair keeps resolving after the line is named, so a
    // formula written before the name existed is not broken: it is repaired, exactly like the one that carries the
    // name the line had until now. Both are the same line, and only the name may be written.
    QStringList fromHandles;
    if (not oldSuffix.isEmpty())
    {
        fromHandles << oldSuffix;
    }
    fromHandles << m_pairKey;
    fromHandles.removeDuplicates();

    // A name taken away leaves the pair, which is the only thing a formula can write for a line nobody named.
    const QString toHandle = newSuffix.isEmpty() ? m_pairKey : newSuffix;

    RewriteDocumentFormulas(Doc(),
                            [&fromHandles, &toHandle](const QString &token) -> QString
                            {
                                for (const QString &handle : std::as_const(fromHandles))
                                {
                                    if (token == line_ + handle)
                                    {
                                        return line_ + toHandle;
                                    }

                                    if (token == angleLine_ + handle)
                                    {
                                        return angleLine_ + toHandle;
                                    }
                                }

                                return token;
                            });
}
