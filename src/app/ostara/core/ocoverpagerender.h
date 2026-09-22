/************************************************************************
 **
 **  @file   ocoverpagerender.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  A cover page, resolved and painted — the ONE pipeline.
 **
 **          Lifted out of the template editor's preview the day the export
 **          canvas needed the same page. Two callers, one drawing: a page
 **          resolved twice would be two pages, and the screen is supposed
 **          to be evidence about the paper.
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

#ifndef OCOVERPAGERENDER_H
#define OCOVERPAGERENDER_H

#include "ocoverpageresolver.h"
#include "ocoverpagetemplate.h"

#include <QMap>
#include <QRectF>
#include <QVector>

#include <functional>

class OSizeTable;
class QPainter;

/// @brief What drawing a cover page takes, and nothing about who asked for it.
namespace OCoverPageRender
{
/// What one section reads.
///
/// **The caller's answer, given PER SECTION** — the axes an information shows live in its own
/// options, so a value built once for the page would be a value built for the wrong section. The
/// preview answers it from the pattern and the libraries; the export answers it from the RUN it is
/// drawing, which is the whole of the nomenclature spec's §6.
using DataForSection =
    std::function<OCoverPageResolver::PatternData(const OCoverPageTemplate::Section &)>;

/// @brief Every band of the page: each section resolved, folded to its band's own width, in the
///        order the template holds them.
///
/// **Folded before the bands are measured**, and not later: a band's height comes from the lines it
/// holds, so folding after the page was laid out would leave it cut for a text that no longer
/// exists — too short for what is drawn in it.
///
/// @param table            the measurement table, for the information that reads one. May be null;
///                         the resolver answers less rather than crashing.
/// @param substituteEmpty  whether an information that resolves to NOTHING stands in as its own
///                         name so the user can see it is there and waiting. **That is the
///                         preview's behaviour and the preview's alone** — the nomenclature spec's
///                         §6: the page itself draws nothing there, and a canvas showing what the
///                         paper will carry must pass false.
auto Bands(const OCoverPageTemplate &tpl, const DataForSection &data, const QFont &baseFont,
           const OSizeTable *table, bool substituteEmpty)
    -> QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>>;

/// @brief Paint a page, its bands already resolved, on @p page, at @p scale.
///
/// A scale of 1 IS the printed page: the bands are in print pixels, and @p page is the rectangle
/// they are measured against. Below 1 the whole of it is brought down — the fonts as much as the
/// geometry — so what is placed is what the export draws, at the size it will be printed.
///
/// **The page's white ground is not painted here.** What carries it — a pane, a sheet of paper, an
/// extra sheet above the plan — is the caller's, and a painter that filled white would be claiming
/// a paper it does not own.
void Paint(QPainter &painter, const QRectF &page,
           const QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> &bands,
           qreal scale);
} // namespace OCoverPageRender

#endif // OCOVERPAGERENDER_H
