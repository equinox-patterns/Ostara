/************************************************************************
 **
 **  @file   othumbnaildata.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Per-piece thumbnail state (see othumbnaildata.h).
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

#include "othumbnaildata.h"

#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"

//---------------------------------------------------------------------------------------------------------------------
OThumbnailData::OThumbnailData()
{
    // The units a miniature is born with are the preference's (see the header): the Thumbnail page of
    // the block editor decides them, and a miniature nobody has styled yet follows it. Saving the
    // piece freezes the two flags in the file, so a later change of preference cannot rewrite a
    // miniature that already carries its own answer.
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    m_showCentimeters = settings->IsThumbnailShowCentimeters();
    m_showInches = settings->IsThumbnailShowInches();

    // ⚠️ At least one unit is always on, whatever a settings file edited by hand may say: with
    // neither, the miniature prints no value at all (see OThumbnailTextBuilder::ValueText), and a
    // block holding a drawing captioned about nothing is worse than one holding a unit too many.
    if (!m_showCentimeters && !m_showInches)
    {
        m_showCentimeters = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::IsSet() const -> bool
{
    return m_set;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetSet(bool set)
{
    m_set = set;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::IsActive() const -> bool
{
    return m_active;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetActive(bool active)
{
    m_active = active;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::Geom() const -> OThumbnailGeom
{
    return m_geom;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::HasGeometry() const -> bool
{
    return m_hasGeom;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetGeom(const OThumbnailGeom &geom)
{
    m_geom = geom;
    m_hasGeom = true;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::Clear()
{
    m_set = false;
    m_active = false;
    m_geom = OThumbnailGeom();
    m_hasGeom = false;

    // A cleared miniature is a piece that never configured one: back to the defaults, not to a
    // half-state that would print nothing because no unit happens to be selected.
    m_showCentimeters = true;
    m_showInches = false;
    m_labels.clear();
    m_legendOffset = QPointF();
    m_hasLegendOffset = false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::ShowCentimeters() const -> bool
{
    return m_showCentimeters;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetShowCentimeters(bool show)
{
    m_showCentimeters = show;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::ShowInches() const -> bool
{
    return m_showInches;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetShowInches(bool show)
{
    m_showInches = show;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::Labels() const -> const QVector<OThumbnailLabel> &
{
    return m_labels;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetLabels(const QVector<OThumbnailLabel> &labels)
{
    m_labels = labels;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::LegendOffset() const -> QPointF
{
    return m_legendOffset;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::HasLegendOffset() const -> bool
{
    return m_hasLegendOffset;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetLegendOffset(const QPointF &offset)
{
    m_legendOffset = offset;
    m_hasLegendOffset = true;
}
