/************************************************************************
 **
 **  @file   opieceinsert.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  A library piece entering the open pattern: made ready, and put in as one gesture.
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

#include "opieceinsert.h"

#include "opieceimport.h"
#include "opiecemark.h"
#include "opieceremap.h"

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vpatterndb/variables/vinternalvariable.h"
#include "../vpatterndb/vcontainer.h"
#include "../xml/vpattern.h"

#include <QCoreApplication>
#include <QPointF>
#include <QSet>
#include <QUndoCommand>

namespace
{
/**
 * @brief Every element below @a element, handed to @a visit one by one.
 *
 * QDomElement::elementsByTagName accepts no wildcard, so a whole subtree is walked by hand — which is also the
 * only way to visit the elements the schema does not make a list of.
 */
template <typename Visit> void ForEachElement(const QDomElement &element, Visit visit)
{
    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        visit(child);
        ForEachElement(child, visit);
    }
}

/**
 * @brief Move the construction so that it starts at @a target.
 *
 * A piece entering a pattern would otherwise land exactly where the block it was saved from still stands, and the
 * two would hide one behind the other. A block stores **exactly one kind of coordinate**: the position of its base
 * points — every other object, a point as much as a curve, reads its position from its parents when the pattern is
 * parsed. So a construction is moved by moving what it stores, and the whole piece follows on its own at the next
 * full parse. All the base points of the block take the same shift, which is what keeps the piece in one shape.
 *
 * Both coordinates are written the way the base point writes them (VToolBasePoint::SaveOptions), so what a move
 * through the canvas produces and what this produces are the same document.
 */
void Place(QDomElement &block, const QPointF &target)
{
    QDomElement basePoint;
    ForEachElement(block,
                   [&basePoint](QDomElement &element) -> void
                   {
                       if (basePoint.isNull() && element.hasAttribute(AttrX) && element.hasAttribute(AttrY))
                       {
                           basePoint = element;
                       }
                   });

    if (basePoint.isNull())
    {
        return; // Nothing to hold on to: the block carries no position of its own.
    }

    const QPointF from(basePoint.attribute(AttrX).toDouble(), basePoint.attribute(AttrY).toDouble());
    const QPointF shift = target - from;
    if (shift.isNull())
    {
        return;
    }

    ForEachElement(block,
                   [&shift](QDomElement &element) -> void
                   {
                       if (not element.hasAttribute(AttrX))
                       {
                           return;
                       }

                       const QPointF position(element.attribute(AttrX).toDouble(),
                                              element.attribute(AttrY).toDouble());
                       element.setAttribute(AttrX, QString::number(position.x() + shift.x()));
                       if (element.hasAttribute(AttrY))
                       {
                           element.setAttribute(AttrY, QString::number(position.y() + shift.y()));
                       }
                   });
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceInsert::Keys(VContainer *data) -> QSet<QString>
{
    QSet<QString> taken;

    if (const QHash<QString, QSharedPointer<VInternalVariable>> *variables = data == nullptr ? nullptr : data->DataVariables();
        variables != nullptr)
    {
        for (auto it = variables->constBegin(); it != variables->constEnd(); ++it)
        {
            taken.insert(it.key());
        }
    }

    return taken;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceInsert::Propose(const OPieceDocument &piece, VPattern *doc, VContainer *data, const QString &instanceName)
    -> Proposal
{
    Proposal proposal;
    proposal.instanceName = instanceName;

    if (doc == nullptr || data == nullptr || piece.DrawElement().isNull())
    {
        return proposal;
    }

    // What the pattern **answers to**, not only the names it holds: the container keeps one key per object, and
    // that key is what an alias of the piece would overwrite in silence. Only the tail is compared, which is what
    // the heads the container builds on a name have in common.
    const QSet<QString> taken = Keys(data);

    // The piece's **envelope**, not only its block: the names of its line roles live at the root of a `.opiece`,
    // and a role the mark forgets is a copy that reads the original's lines. The aliases the dialog shows and the
    // table it proposes are therefore read from the same place, and cannot disagree.
    const QDomElement root = piece.documentElement();
    proposal.aliases = OPieceMark::Aliases(root);

    // The names of the piece's own line roles: they live at the root of its envelope, so the walk over the block
    // will not meet them — and yet a formula of the block cites them by `Line_<role>` and `AngleLine_<role>`.
    for (QDomElement row = piece.LineNames().firstChildElement(VDomDocument::TagLine); not row.isNull();
         row = row.nextSiblingElement(VDomDocument::TagLine))
    {
        const QString role = row.attribute(VAbstractPattern::AttrName);
        if (not role.isEmpty())
        {
            proposal.roles.append(role);
        }
    }

    // The number comes from the **pattern**, never from the piece: the marks will live in the pattern, and two
    // pieces entering one after the other have to wear P1 and then P2.
    proposal.number = OPieceMark::HighestNumber(doc->documentElement()) + 1;
    proposal.collides = OPieceMark::Collides(root, taken);
    proposal.table = OPieceMark::TableFor(root, taken, proposal.number);

    return proposal;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceInsert::Prepare(const OPieceDocument &piece, VContainer *data, const Proposal &proposal,
                           const QPointF &anchorTarget) -> Prepared
{
    Prepared prepared;

    if (data == nullptr || not proposal.IsReady())
    {
        return prepared;
    }

    // A copy: the library piece is read, never modified. What enters the pattern is this block, and it is the
    // only thing the marking, the renaming and the remapping touch.
    QDomElement block = piece.DrawElement().cloneNode(true).toElement();
    if (block.isNull())
    {
        return prepared;
    }

    // The origin first, while the block still wears the piece's name: `category` is what will remember where the
    // instance came from once it has a name of its own.
    OPieceImport::SetOrigin(block, piece);

    prepared.instanceName = proposal.instanceName;
    block.setAttribute(VAbstractPattern::AttrName, proposal.instanceName);

    // The place it takes, in the pattern's own unit: the piece keeps its shape and its distances, it simply does
    // not stand where the block it was saved from still stands.
    Place(block, anchorTarget);

    // The table the window decided — every name of the piece, or the marks it kept. Empty means the piece keeps
    // its names exactly as they are: what to mark is never decided here, since a dialog may have been asked.
    prepared.renamed = proposal.table;
    if (not prepared.renamed.isEmpty())
    {
        OPieceMark::Rename(block, prepared.renamed, proposal.roles);
    }

    // The ids last: they depend on no name, and the remapping leaves what the marking wrote alone.
    OPieceRemap::RemapIds(block, data);

    prepared.block = block;
    return prepared;
}

//---------------------------------------------------------------------------------------------------------------------
OPieceInsert::AddPieceFromLibrary::AddPieceFromLibrary(const Prepared &prepared, const OPieceDocument &piece,
                                                       VPattern *doc, VContainer *data, QUndoCommand *parent)
    : VUndoCommand(doc, parent), m_instanceName(prepared.instanceName), m_block(prepared.block),
      m_renamed(prepared.renamed), m_pattern(doc), m_data(data)
{
    setText(QCoreApplication::translate("OPieceInsert", "Insert the piece \"%1\"").arg(m_instanceName));

    // The command owns its own copy of the piece: a Ctrl+Z followed by a Ctrl+Y has to be able to write those
    // increments and those role names a second time, and the caller's document may be gone by then.
    const QDomElement source = piece.documentElement();
    if (not source.isNull())
    {
        QDomElement root = m_piece.createElement(source.tagName());
        for (QDomElement child = source.firstChildElement(); not child.isNull();
             child = child.nextSiblingElement())
        {
            root.appendChild(m_piece.importNode(child, true));
        }
        m_piece.appendChild(root);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceInsert::AddPieceFromLibrary::InstanceName() const -> QString
{
    return m_instanceName;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInsert::AddPieceFromLibrary::redo()
{
    if (m_pattern == nullptr || m_data == nullptr || m_block.isNull())
    {
        return;
    }

    // Taken on every redo, not once in the constructor: after an undo the sections are back to what they were, so
    // the state about to be changed is the same one, and saving it again is what keeps the two in step.
    SaveSections();

    m_pattern->InsertRootElement(m_block);

    VPatternBlockMapper *blocks = m_pattern->PatternBlockMapper();
    m_indexPatternBlock = blocks->GetId(m_instanceName);
    blocks->SetElementById(m_indexPatternBlock, m_block);
    blocks->SetActiveById(m_indexPatternBlock);

    OPieceImport::ImportIncrements(m_piece, m_pattern, m_data, m_instanceName);
    OPieceImport::ImportLineNames(m_piece, m_data, m_renamed);

    // And the document is told, in the same breath: the container alone would be a promise the next parse does
    // not keep — a parse reads the section, and a pattern whose lines are named only in memory is a pattern
    // whose formulas cite a name nobody defines.
    m_pattern->SyncLineNamesToDom();

    m_pattern->LiteParseTree(Document::FullLiteParse);
    m_pattern->NeedFullParsing();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInsert::AddPieceFromLibrary::undo()
{
    if (m_pattern == nullptr)
    {
        return;
    }

    // The root first: the increments, their groups, and nothing of what the piece brought is left behind. The
    // block itself goes after — a table naming rows that no longer exist would be read by the parse below.
    RestoreSections();

    const QDomElement block = m_pattern->PatternBlockMapper()->GetElementById(m_indexPatternBlock);
    if (not block.isNull())
    {
        m_pattern->documentElement().removeChild(block);
    }

    m_pattern->LiteParseTree(Document::FullLiteParse);
    m_pattern->NeedFullParsing();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInsert::AddPieceFromLibrary::SaveSections()
{
    const QDomElement root = m_pattern->documentElement();

    m_increments = root.firstChildElement(VAbstractPattern::TagIncrements).cloneNode(true).toElement();
    m_incrementGroups = root.firstChildElement(VAbstractPattern::TagIncrementGroups).cloneNode(true).toElement();
    m_previewCalculations =
        root.firstChildElement(VAbstractPattern::TagPreviewCalculations).cloneNode(true).toElement();
    m_previewCalculationGroups =
        root.firstChildElement(VAbstractPattern::TagPreviewCalculationGroups).cloneNode(true).toElement();
    m_lineNames = root.firstChildElement(VAbstractPattern::TagLineNames).cloneNode(true).toElement();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInsert::AddPieceFromLibrary::RestoreSections()
{
    const auto restore = [this](const QString &tag, const QDomElement &saved)
    {
        QDomElement root = m_pattern->documentElement();
        const QDomElement current = root.firstChildElement(tag);

        if (saved.isNull())
        {
            // The section did not exist before the import: the import is what made it, and it goes with it.
            if (not current.isNull())
            {
                root.removeChild(current);
            }
            return;
        }

        if (not current.isNull())
        {
            root.removeChild(current);
        }

        // Put back by the document's own authority on placement, never by an append: the order of the root is a
        // rule of the schema, and a section restored at the end would be a pattern the schema refuses.
        m_pattern->InsertRootElement(saved.cloneNode(true).toElement());
    };

    restore(VAbstractPattern::TagIncrements, m_increments);
    restore(VAbstractPattern::TagIncrementGroups, m_incrementGroups);
    restore(VAbstractPattern::TagPreviewCalculations, m_previewCalculations);
    restore(VAbstractPattern::TagPreviewCalculationGroups, m_previewCalculationGroups);
    restore(VAbstractPattern::TagLineNames, m_lineNames);
}
