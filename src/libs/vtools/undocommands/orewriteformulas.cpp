/************************************************************************
 **
 **  @file   orewriteformulas.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/10/2026
 **
 **  @brief  Rewrite the formulas of a whole pattern, token by token.
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
#include "orewriteformulas.h"

#include <QDomAttr>
#include <QDomElement>
#include <QDomNamedNodeMap>
#include <QDomNodeList>

#include "../../ifc/xml/vabstractpattern.h"
#include "../../vpatterndb/vformula.h"
#include "../../vmisc/def.h"

//---------------------------------------------------------------------------------------------------------------------
auto RewriteDocumentFormulas(VAbstractPattern *doc, const std::function<QString(const QString &)> &map) -> void
{
    if (doc == nullptr)
    {
        return;
    }

    // Check if all tags handled when we have new tool
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 78);

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
        const QDomNodeList elements = doc->elementsByTagName(tagName);
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

                // Every attribute is read: which of them holds a formula is the tool's business, not this
                // function's, and a formula that is not one comes back unchanged from the mapping.
                const QString formula = attr.value();
                if (formula.isEmpty())
                {
                    continue;
                }

                const QString rewritten = VFormula::MapTokens(formula, map);
                if (rewritten != formula)
                {
                    element.setAttribute(attr.name(), rewritten);
                }
            }
        }
    }
}
