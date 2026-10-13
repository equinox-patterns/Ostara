/************************************************************************
 **
 **  @file   opiecename.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/10/2026
 **
 **  @brief  Nommer les objets sans nom qu'une pièce emploie, avant de l'enregistrer.
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
#include "opiecename.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../xml/vpattern.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "undocommands/orewriteformulas.h"
#include "undocommands/renametoken.h"

#include <QDomElement>
#include <QSet>

namespace
{
/** @brief True when @p node sits inside @p block. */
auto IsInside(const QDomNode &node, const QDomElement &block) -> bool
{
    for (QDomNode parent = node; not parent.isNull(); parent = parent.parentNode())
    {
        if (parent == block)
        {
            return true;
        }
    }
    return false;
}

/** @brief Le bloc où vit @p node — vide pour ce qui n'appartient à aucun bloc. */
auto BlockOf(const QDomNode &node) -> QString
{
    for (QDomNode parent = node; not parent.isNull(); parent = parent.parentNode())
    {
        const QDomElement element = parent.toElement();
        if (not element.isNull() && element.tagName() == VAbstractPattern::TagDraw)
        {
            return element.attribute(VAbstractPattern::AttrName);
        }
    }

    return {};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceName::Blockers(VPattern *doc, VContainer *data, const QString &blockName) -> QVector<Blocker>
{
    QVector<Blocker> blockers;
    if (doc == nullptr || data == nullptr)
    {
        return blockers;
    }

    const QDomElement block = doc->PatternBlockMapper()->GetElement(blockName);
    if (block.isNull())
    {
        return blockers;
    }

    // Ce que la pièce emploie : chez elle, n'importe quel nom composé du document — un objet qu'elle lit ailleurs
    // est aussi fragile qu'un des siens — et chez les autres, les noms composés **de cette pièce**, et d'elle
    // seule : c'est ce que la pièce ne peut pas emporter, et ce qu'une autre pièce casserait en la renommant.
    const QSet<QString> everywhere = OPieceMark::ComposedNames(doc->documentElement());
    const QSet<QString> ofThisPiece = OPieceMark::ComposedNames(block);
    const QHash<QString, OPieceMark::Carrier> carriers = OPieceMark::Carriers(block);

    QSet<QString> seen;
    for (const VFormulaField &field : doc->ListExpressions())
    {
        const bool inside = IsInside(field.element, block);

        // Une formule qui n'appartient à aucun bloc — une ligne d'incrément du patron, un calcul d'aperçu — n'est
        // à personne en propre : rien ne dit qu'elle vient de cette pièce, et la nommer toucherait une autre.
        if (not inside && BlockOf(field.element).isEmpty())
        {
            continue;
        }

        const QSet<QString> &composed = inside ? everywhere : ofThisPiece;

        // Les objets sans nom de **cette** formule, par la même lecture que la dialog des formules : la première
        // formule qui appelle un objet est celle que la fenêtre montre, et un objet déjà vu n'est pas redemandé.
        for (const DialogNameObject::Object &object : UnnamedObjects(data, field.expression))
        {
            const QString name = object.suffix;
            if (name.isEmpty() || not composed.contains(name) || seen.contains(name))
            {
                continue;
            }
            seen.insert(name);

            const OPieceMark::Carrier carrier = carriers.value(name);

            Blocker blocker;
            blocker.name = name;
            blocker.slot = carrier.slot;
            blocker.carrier = carrier.element;
            // Une paire de points n'est pas un élément du fichier : c'est une ligne, et le conteneur la
            // connaît par la paire qu'elle joint tant que personne ne l'a nommée.
            blocker.of = object.pairKey.isEmpty() ? OPieceMark::ObjectOf(carrier.slot) : OPieceMark::AliasOf::Line;
            blocker.pairKey = object.pairKey;
            blocker.curveId = object.curveId;
            blocker.formula = field.expression;
            blockers.append(blocker);
        }
    }

    return blockers;
}

//---------------------------------------------------------------------------------------------------------------------
OPieceName::NameCurves::NameCurves(VPattern *doc, const QVector<QPair<QString, QString>> &renamed,
                                   QUndoCommand *parent)
    : VUndoCommand(doc, parent), m_renamed(renamed)
{
    setText(QObject::tr("name the curves of a piece"));
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceName::NameCurves::redo()
{
    // Ce que les formules disaient, gardé une fois : l'undo les remet exactement, sans recalculer quoi que ce soit.
    if (not m_saved)
    {
        for (const VFormulaField &field : Doc()->ListExpressions())
        {
            m_fields.append(Field{field.element, field.attribute, field.element.attribute(field.attribute)});
        }
        m_saved = true;
    }

    RewriteDocumentFormulas(Doc(),
                            [this](const QString &token) -> QString
                            {
                                for (const QPair<QString, QString> &pair : m_renamed)
                                {
                                    // Les têtes d'une courbe — `Spl_`, `SplPath_`, `Curve_`, `Handle1Curve_`… —
                                    // sont celles que la marque connaît déjà : une seule règle, un seul endroit.
                                    const QString mapped = ReplaceTokenAlias(token, CurveAliasType::All, pair.first,
                                                                            pair.second);
                                    if (mapped != token)
                                    {
                                        return mapped;
                                    }
                                }
                                return token;
                            });

    emit NeedLiteParsing(Document::LiteParse);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceName::NameCurves::undo()
{
    for (Field &field : m_fields)
    {
        field.element.setAttribute(field.attribute, field.text);
    }

    emit NeedLiteParsing(Document::LiteParse);
}
