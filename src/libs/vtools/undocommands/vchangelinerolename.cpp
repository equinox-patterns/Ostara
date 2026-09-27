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

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../qmuparser/qmutokenparser.h"
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
    if (oldSuffix.isEmpty() || newSuffix.isEmpty())
    {
        return;
    }

    const QString oldLineToken = line_ + oldSuffix;       // e.g. "Line_Test2"
    const QString newLineToken = line_ + newSuffix;       // e.g. "Line_MaLigne"
    const QString oldAngleToken = angleLine_ + oldSuffix; // e.g. "AngleLine_Test2"
    const QString newAngleToken = angleLine_ + newSuffix; // e.g. "AngleLine_MaLigne"

    // Check if all tags handled when we have new tool
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 72);

    const QStringList validTags = {VAbstractPattern::TagPoint,
                                   VAbstractPattern::TagOperation,
                                   VAbstractPattern::TagArc,
                                   VAbstractPattern::TagElArc,
                                   VAbstractPattern::TagSpline,
                                   VAbstractPattern::TagPath,
                                   VAbstractPattern::TagTools,
                                   VAbstractPattern::TagDetail};

    for (const QString &tagName : validTags)
    {
        const QDomNodeList elements = Doc()->elementsByTagName(tagName);
        for (int i = 0; i < elements.size(); ++i)
        {
            QDomElement element = elements.at(i).toElement();
            if (element.isNull())
            {
                continue;
            }

            const QDomNamedNodeMap attrs = element.attributes();
            for (int j = 0; j < attrs.size(); ++j)
            {
                const QDomAttr attr = attrs.item(j).toAttr();
                if (attr.isNull())
                {
                    continue;
                }

                const QString formula = attr.value();
                if (formula.isEmpty())
                {
                    continue;
                }

                // Tokenize and replace matching line tokens
                QString newFormula = formula;
                bool changed = false;

                try
                {
                    QScopedPointer<qmu::QmuTokenParser> const parser(
                        new qmu::QmuTokenParser(formula, false, false));
                    const QMap<vsizetype, QString> tokens = parser->GetTokens();

                    // Process tokens in reverse order to maintain correct positions during replacement
                    QList<vsizetype> positions = tokens.keys();
                    std::sort(positions.begin(), positions.end(), std::greater<>());

                    for (const vsizetype pos : positions)
                    {
                        const QString &token = tokens[pos];
                        if (token == oldLineToken)
                        {
                            newFormula.replace(pos, token.length(), newLineToken);
                            changed = true;
                        }
                        else if (token == oldAngleToken)
                        {
                            newFormula.replace(pos, token.length(), newAngleToken);
                            changed = true;
                        }
                    }
                }
                catch (qmu::QmuParserError &)
                {
                    // Skip formulas that can't be parsed
                    continue;
                }

                if (changed)
                {
                    element.setAttribute(attr.name(), newFormula);
                }
            }
        }
    }
}
