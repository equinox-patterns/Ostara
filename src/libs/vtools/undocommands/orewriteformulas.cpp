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
auto RewriteFormulas(QDomElement &root, const std::function<QString(const QString &)> &map) -> void
{
    if (root.isNull())
    {
        return;
    }

    // Check if all tags handled when we have new tool
    Q_STATIC_ASSERT(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 78);

    // **Tout** élément est visité, et **tout** attribut est lu : lequel porte une formule est l'affaire de l'outil,
    // pas de cette fonction, et une formule qui n'en est pas une revient inchangée du mapping.
    //
    // Une liste de balises laissait échapper les formules qui ne vivent pas sur l'élément de l'outil lui-même :
    // celles d'un `<pathPoint>` (`angle1`, `angle2`, `length1`, `length2`), d'un `<increment>` (`formula`), d'un
    // `<node>` de pièce (`seamAllowanceBefore`) ou d'un `figure`/`item` d'opération. C'est ainsi qu'une citation
    // `AngleLigne_M_M1` a **survécu** au rôle donné à sa ligne : la réécriture ne la voyait pas, la pièce ne
    // pouvait plus s'enregistrer, et le refus nommait un nom composé que plus personne ne lisait ailleurs
    // (mesuré le 13-10-2026 : `BustedeBase.ostara`, `<pathPoint angle2="AngleLine_M_M1">`).
    const std::function<void(QDomElement &)> rewrite = [&map, &rewrite](QDomElement &element) -> void
    {
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

            const QString rewritten = VFormula::MapTokens(formula, map);
            if (rewritten != formula)
            {
                element.setAttribute(attr.name(), rewritten);
            }
        }

        for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
        {
            rewrite(child);
        }
    };

    rewrite(root);
}

//---------------------------------------------------------------------------------------------------------------------
auto RewriteDocumentFormulas(VAbstractPattern *doc, const std::function<QString(const QString &)> &map) -> void
{
    if (doc == nullptr)
    {
        return;
    }

    QDomElement root = doc->documentElement();
    RewriteFormulas(root, map);
}
