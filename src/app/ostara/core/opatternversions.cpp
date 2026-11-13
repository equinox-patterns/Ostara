/************************************************************************
 **
 **  @file   opatternversions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  The pattern's versions: what they are called, and who wears which.
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

#include "opatternversions.h"

#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vpatterndb/ipieceinfo.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QDomElement>
#include <QDomNodeList>

namespace
{
/// How several versions are shown on one line. A separator between words, not a saved value: nothing
/// in the file depends on it.
const QString kSeparator = QStringLiteral(" + ");
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersions::Name(const VAbstractPattern &doc, quint32 id) -> QString
{
    return doc.GetPatternVersion(id).name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersions::Ids(const VAbstractPattern &doc, const VContainer *data, const IPieceInfo &object) -> QList<quint32>
{
    const quint32 allVersions = doc.AllVersionsId();

    // A piece wears one version, and only one. 0 never reaches a piece — VPiece answers 1 for "not
    // told" — so this branch is the whole piece case.
    if (const quint32 own = object.GetPatternVersion(); own != 0)
    {
        return {own};
    }

    // A group wears none of its own: it shows the versions of its members.
    QList<quint32> ids;
    const auto *group = dynamic_cast<const OPieceGroup *>(&object);
    if (group == nullptr || data == nullptr)
    {
        return ids;
    }

    for (const quint32 pieceId : group->pieceIds)
    {
        try
        {
            const quint32 id = data->GetPiece(pieceId).GetPatternVersion();
            const quint32 effective = id == 0 ? allVersions : id;
            if (!ids.contains(effective))
            {
                ids.append(effective);
            }
        }
        catch (const VExceptionBadId &)
        {
            continue; // a member the scene no longer holds: nothing to say about it
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersions::Names(const VAbstractPattern &doc, const QList<quint32> &ids) -> QStringList
{
    QStringList names;
    for (const VPatternVersionData &version : doc.GetPatternVersions())
    {
        if (ids.contains(version.id))
        {
            names.append(version.name);
        }
    }

    // An id the list does not hold any more still names a version: say what the piece wears rather
    // than dropping it from the line.
    for (const quint32 id : ids)
    {
        const QString name = Name(doc, id);
        if (!name.isEmpty() && !names.contains(name))
        {
            names.append(name);
        }
    }
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersions::Join(const QStringList &names) -> QString
{
    return names.join(kSeparator);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternVersions::PieceIdsUsing(const VAbstractPattern &doc, quint32 versionId) -> QList<quint32>
{
    QList<quint32> ids;
    const quint32 allVersions = doc.AllVersionsId();

    const QDomNodeList details = doc.elementsByTagName(VAbstractPattern::TagDetail);
    for (int i = 0; i < details.size(); ++i)
    {
        const QDomElement detail = details.at(i).toElement();
        if (detail.isNull())
        {
            continue;
        }

        const quint32 pieceId = detail.attribute(VDomDocument::AttrId).toUInt();
        const quint32 id = detail.attribute(AttrPatternVersion).isEmpty()
                               ? allVersions
                               : detail.attribute(AttrPatternVersion).toUInt();
        if (pieceId != 0 && id == versionId)
        {
            ids.append(pieceId);
        }
    }
    return ids;
}
