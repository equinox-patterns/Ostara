/************************************************************************
 **
 **  @file   oexportpdf.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Export mode — write a generation as a PDF.
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

#ifndef OEXPORTPDF_H
#define OEXPORTPDF_H

#include <QString>

class VAbstractPattern;
class VContainer;
struct OExportOutput;

/**
 * @brief The PDF of a generation.
 *
 * What it writes is what the canvas shows: the same items, built by the same code, so the
 * paper cannot disagree with the screen. The layers come from that same walk — one per
 * size, plus the reference size's decor — and Trame turns them into optional content
 * groups, which is what lets a printer switch a size off in Acrobat.
 *
 * The page is the bin: the width the print model imposed, the height the nesting used.
 * Nothing is cropped and nothing is left blank, because it is the very rectangle the
 * canvas draws.
 *
 * A single-size run has nothing to switch between, so it goes out flat — no layers, one
 * stream. There is no layer panel to offer, and an entry with no choice is not one.
 */
namespace OExportPdf
{
/// @param output  the generation, as it was filed: its resolved layers and its placements.
/// @param pattern the pattern it was nested from — the decor is read from it live, exactly
///                as the canvas reads it, so what is exported is what is on screen now.
/// @param doc     the document, which the cartouche resolver reads the template from.
/// @return false when the run cannot be reproduced; the reason has been logged.
bool Write(const OExportOutput &output, const QString &pdfPath,
           const VContainer *pattern, VAbstractPattern *doc);
}  // namespace OExportPdf

#endif  // OEXPORTPDF_H
