/************************************************************************
 **
 **  @file   oannotationdata.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Per-piece annotation list (see oannotationdata.h).
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

#include "oannotationdata.h"

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationData::IsSet() const -> bool
{
    return m_set;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationData::SetSet(bool set)
{
    m_set = set;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationData::Annotations() const -> const QList<OAnnotation> &
{
    return m_annotations;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationData::SetAnnotations(const QList<OAnnotation> &annotations)
{
    m_annotations = annotations;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationData::Count() const -> int
{
    return static_cast<int>(m_annotations.size());
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationData::Clear()
{
    m_set = false;
    m_annotations.clear();
}
