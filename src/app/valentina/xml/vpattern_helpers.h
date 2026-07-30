/************************************************************************
 **
 **  @file   vpattern_helpers.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Helper functions and structs extracted from vpattern.cpp
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **  Copyright (C) 2026 Sophie Denys
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

#ifndef VPATTERN_HELPERS_H
#define VPATTERN_HELPERS_H

#include <QDomElement>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "typedef.h"

struct VPieceFoldLineData
{
    quint32 p1{NULL_ID};
    quint32 p2{NULL_ID};
    bool mirrorLineVisible{true};
    QString heightFormula{};
    QString widthFormula{};
    QString centerFormula{};
    bool manualHeight{false};
    bool manualWidth{false};
    bool manualCenter{false};
    FoldLineType type{FoldLineType::TwoArrowsTextAbove};
    unsigned int fontSize{defFoldLineFontSize};
    bool italic{false};
    bool bold{false};
    QString label{};
    int alignment{Qt::AlignHCenter};
};

namespace VPatternHelpers
{

auto FileComment() -> QString;
void GatherCount(int &count, int nodes);
auto DefLabelLanguage() -> QString;
auto ParsePieceMirrorLine(const QDomElement &domElement) -> VPieceFoldLineData;
void CollectFromUnionSection(const QDomElement &parent, const QString &sectionTag, QVector<vidtype> &ids);
auto CollectUnionChildIds(const QDomElement &toolElement) -> QVector<vidtype>;

} // namespace VPatternHelpers

#endif // VPATTERN_HELPERS_H
