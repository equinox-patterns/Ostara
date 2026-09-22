/************************************************************************
 **
 **  @file   oexportprintmodel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The table of print models, and the lookup the replay needs.
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

#include "oexportprintmodel.h"

namespace
{
// The labels are wrapped in QT_TRANSLATE_NOOP rather than tr(), and the context is
// spelled out at every call: lupdate reads the macro's arguments as written in the
// source, so a named constant would leave it with no context to file the string
// under, and a tr() here would not be extracted at all — the interface would come
// out half English in French. The translation itself happens where the label is
// shown, with QCoreApplication::translate("OExportPrintModel", …).
// The cover-seat column, read down the table: the block is an A4's own rectangle, so a format
// keeps it in a corner when it has room for a plan BESIDE it, and gives the page a sheet of its
// own when it has not. A3 (297 × 420) and Tabloid (279.4 × 431.8) have that room; A4 does not —
// the block would be the whole sheet — Letter is too short for 297, and A4/Letter framed is
// narrower than 210. A roll has no sheet height at all, so the block simply sits at its corner.
//
// ⚠️ A0 is the one seat nobody dictated: it is not selectable today, so no run can reach it.
// `InSheet` because an A0 sheet does admit the block; to revisit if the row is ever enabled.
// ⚠️ The columns are in DECLARATION order — a Seat sits between the two flags, because
// `hasFrame` and `coverSeat` are the format's two properties and belong together. Aggregate
// initialisation is positional: a column moved without the struct moved is a silent
// reinterpretation, and the enum class is what caught it.
const QVector<OExportPrintModel> kModels{
    //              label                    token              shape                     widthMm  heightMm  frame  coverSeat                          columns  selectable
    {QT_TRANSLATE_NOOP("OExportPrintModel", "A0 Roll"),           "A0Roll",          OExportPrintShape::Roll,   841.0,      0.0,  false, OExportCoverPage::Seat::InSheet,    false,   true},
    {QT_TRANSLATE_NOOP("OExportPrintModel", "A0"),                "A0",              OExportPrintShape::Tiled,  841.0,   1189.0,  false, OExportCoverPage::Seat::InSheet,    false,   false},
    {QT_TRANSLATE_NOOP("OExportPrintModel", "A3"),                "A3",              OExportPrintShape::Tiled,  297.0,    420.0,  false, OExportCoverPage::Seat::InSheet,    true,    true},
    {QT_TRANSLATE_NOOP("OExportPrintModel", "Tabloid"),           "Tabloid",         OExportPrintShape::Tiled,  279.4,    431.8,  false, OExportCoverPage::Seat::InSheet,    true,    true},
    {QT_TRANSLATE_NOOP("OExportPrintModel", "A4"),                "A4",              OExportPrintShape::Tiled,  210.0,    297.0,  false, OExportCoverPage::Seat::ExtraSheet, true,    true},
    {QT_TRANSLATE_NOOP("OExportPrintModel", "Letter"),            "Letter",          OExportPrintShape::Tiled,  215.9,    279.4,  false, OExportCoverPage::Seat::ExtraSheet, true,    true},
    // The two framed formats are the only ones whose dimensions are not a standard: the frame
    // is sized so the printable area stays inside the sheet's margins — ten millimetres a side
    // — which is exactly what lets one file come out on A4 OR on Letter, and the other on A3
    // OR on Tabloid. They are listed after the standards, together: they are variants of them.
    {QT_TRANSLATE_NOOP("OExportPrintModel", "A4/Letter framed"),  "A4LetterFramed",  OExportPrintShape::Tiled,  190.0,    259.0,   true,  OExportCoverPage::Seat::ExtraSheet, true,    true},
    {QT_TRANSLATE_NOOP("OExportPrintModel", "A3/Tabloid framed"), "A3TabloidFramed", OExportPrintShape::Tiled,  259.0,    400.0,   true,  OExportCoverPage::Seat::InSheet,    true,    true},
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintModels() -> const QVector<OExportPrintModel> &
{
    return kModels;
}

//---------------------------------------------------------------------------------------------------------------------
auto FindPrintModel(const QString &token) -> const OExportPrintModel *
{
    for (const OExportPrintModel &model : kModels)
    {
        if (token == QLatin1String(model.token))
        {
            return &model;
        }
    }
    return nullptr;
}
