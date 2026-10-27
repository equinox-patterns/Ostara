/************************************************************************
 **
 **  @file   vpattern_helpers.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Helper functions extracted from vpattern.cpp
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **  Copyright (C) 2026 Sophie Denys
 **  Modified for Ostara (fork of Valentina) on 27/10/2026.
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

#include "vpattern_helpers.h"

#include <QDomElement>

#include "../core/vapplication.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/def.h"
#include "../vmisc/projectversion.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"

namespace VPatternHelpers
{

//---------------------------------------------------------------------------------------------------------------------
auto FileComment() -> QString
{
    return QStringLiteral("Pattern created with Ostara v%1.").arg(AppVersionStr());
}

//---------------------------------------------------------------------------------------------------------------------
void GatherCount(int &count, const int nodes)
{
    count += nodes;
}

//---------------------------------------------------------------------------------------------------------------------
auto DefLabelLanguage() -> QString
{
    // A, B, C, always: the alphabet was a preference, and a pattern that wants another one already carries its
    // own labelPrefix attribute, which this default never overrides.
    return QStringLiteral("en");
}

//---------------------------------------------------------------------------------------------------------------------
auto ParsePieceMirrorLine(const QDomElement &domElement) -> VPieceFoldLineData
{
    return {
        .p1 = VDomDocument::GetParametrUInt(domElement, VAbstractPattern::AttrMirrorLineP1, NULL_ID_STR),
        .p2 = VDomDocument::GetParametrUInt(domElement, VAbstractPattern::AttrMirrorLineP2, NULL_ID_STR),
        .mirrorLineVisible = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrMirrorLineVisible, trueStr),
        .heightFormula = VDomDocument::GetParametrEmptyString(domElement, VAbstractPattern::AttrFoldLineHeightFormula),
        .widthFormula = VDomDocument::GetParametrEmptyString(domElement, VAbstractPattern::AttrFoldLineWidthFormula),
        .centerFormula = VDomDocument::GetParametrEmptyString(domElement, VAbstractPattern::AttrFoldLineCenterFormula),
        .manualHeight = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrFoldLineManualHeight, falseStr),
        .manualWidth = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrFoldLineManualWidth, falseStr),
        .manualCenter = VDomDocument::GetParametrBool(domElement, VAbstractPattern::AttrFoldLineManualCenter, falseStr),
        .type = StringToFoldLineType(
            VDomDocument::GetParametrString(domElement,
                                            VAbstractPattern::AttrFoldLineType,
                                            FoldLineTypeToString(FoldLineType::TwoArrowsTextAbove))),
        .label = VDomDocument::GetParametrEmptyString(domElement, VAbstractPattern::AttrFoldLineLabel)};
}

//---------------------------------------------------------------------------------------------------------------------
void CollectFromUnionSection(const QDomElement &parent, const QString &sectionTag, QVector<vidtype> &ids)
{
    const QDomElement section = parent.firstChildElement(sectionTag);
    if (section.isNull())
    {
        return;
    }
    const QDomNodeList childList = section.elementsByTagName(VToolUnionDetails::TagChild);
    for (int i = 0; i < childList.size(); ++i)
    {
        if (const QDomElement child = childList.at(i).toElement(); !child.isNull())
        {
            ids.append(static_cast<vidtype>(child.text().toUInt()));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto CollectUnionChildIds(const QDomElement &toolElement) -> QVector<vidtype>
{
    const QDomElement tagChildren = toolElement.firstChildElement(VToolUnionDetails::TagChildren);
    if (tagChildren.isNull())
    {
        return {};
    }

    QVector<vidtype> ids;
    CollectFromUnionSection(tagChildren, VAbstractPattern::TagNodes, ids);
    CollectFromUnionSection(tagChildren, VToolSeamAllowance::TagCSA, ids);
    CollectFromUnionSection(tagChildren, VToolSeamAllowance::TagIPaths, ids);
    CollectFromUnionSection(tagChildren, VToolSeamAllowance::TagPlaceLabels, ids);
    return ids;
}

} // namespace VPatternHelpers
