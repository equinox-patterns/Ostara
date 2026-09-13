/************************************************************************
 **
 **  @file   OPieceCartoucheCatalogue.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Registry of the cartouche section types (the catalogue, spec §3).
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

#include "OPieceCartoucheCatalogue.h"

#include <QCoreApplication>

namespace
{
// Translation helper — strings live in the section-type context. Field labels
// are built lazily inside Build() (not at static-init time) so they pick up a
// loaded translator at first use.
auto T(const char *source) -> QString
{
    return QCoreApplication::translate("OPieceCartoucheSection", source);
}

// Build a catalogue entry from its source (English) strings — the display name
// and description are translated for the current UI language; the source name
// stays available for rendering the block in another display language.
auto MakeSection(const char *id, const char *sourceName, const char *descriptionSource,
                 const QList<OPieceCartoucheOptionField> &fields) -> OPieceCartoucheSection
{
    return OPieceCartoucheSection(QString::fromUtf8(id), QString::fromUtf8(sourceName), T(sourceName),
                                  T(descriptionSource), fields);
}
}  // namespace

auto OPieceCartoucheCatalogue::Build() -> QList<OPieceCartoucheSection>
{
    const OPieceCartoucheOptionField kFontFamily{QStringLiteral("fontFamily"),
                                                 OPieceCartoucheOptionKind::FontFamily, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Font family"))};
    const OPieceCartoucheOptionField kFontWeight{QStringLiteral("fontWeight"),
                                                 OPieceCartoucheOptionKind::FontWeight, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Font weight"))};
    const OPieceCartoucheOptionField kFontSize{QStringLiteral("fontSize"),
                                               OPieceCartoucheOptionKind::FontSize, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Font size (px)"))};
    const OPieceCartoucheOptionField kLineSpacing{QStringLiteral("lineSpacing"),
                                                  OPieceCartoucheOptionKind::LineSpacing,
                                                  T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Line spacing (px)"))};
    const OPieceCartoucheOptionField kAxis{QStringLiteral("axis"), OPieceCartoucheOptionKind::Axis, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Axis"))};
    const OPieceCartoucheOptionField kMeasureName{QStringLiteral("measureName"),
                                                  OPieceCartoucheOptionKind::MeasureName, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Measurement"))};
    const OPieceCartoucheOptionField kSeparator{QStringLiteral("separator"),
                                                OPieceCartoucheOptionKind::Separator, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Separator"))};
    // Axis/measure separator without "-": "-" is reserved for joining the block
    // languages on a single line ("Stature : 168 - Height : 168").
    const OPieceCartoucheOptionField kSeparatorNoDash{QStringLiteral("separator"),
                                                      OPieceCartoucheOptionKind::Separator,
                                                      T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Separator")),
                                                      {QString(), QStringLiteral(":"), QStringLiteral("/"), QStringLiteral("·")},
                                                      QString()};
    // Piece number: "1 of 10", "1 - 10" or "1 / 10" — no "no separator" choice.
    const OPieceCartoucheOptionField kPieceSeparator{
        QStringLiteral("separator"), OPieceCartoucheOptionKind::Separator, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Separator")),
        {QT_TRANSLATE_NOOP("OInfoBlockPreview", "of"), QStringLiteral("-"), QStringLiteral("/")},
        QT_TRANSLATE_NOOP("OInfoBlockPreview", "of")};
    // Piece number style: a single digit as is ("1 of 10") or padded ("01 of 10").
    const OPieceCartoucheOptionField kPieceNumberStyle{
        OPieceCartoucheNumber::kStyle, OPieceCartoucheOptionKind::NumberStyle,
        T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Number format")),
        {OPieceCartoucheNumber::kPlain, OPieceCartoucheNumber::kPadded}, OPieceCartoucheNumber::kPlain};
    const OPieceCartoucheOptionField kLogo{QStringLiteral("logo"), OPieceCartoucheOptionKind::Logo, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Logo"))};
    const OPieceCartoucheOptionField kLogoSize{QStringLiteral("logoSize"),
                                               OPieceCartoucheOptionKind::LogoSize, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Logo size (cm)"))};
    const OPieceCartoucheOptionField kCut{QStringLiteral("cut"), OPieceCartoucheOptionKind::Cut, T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Cut"))};

    const QList<OPieceCartoucheOptionField> textFields = {kFontFamily, kFontWeight, kFontSize, kLineSpacing};
    // Axis rename: per-language display aliases of the axis and its values
    // (the title shown above the per-language tabs). Reused by the Measurement
    // section with a per-language name only (no values).
    const OPieceCartoucheOptionField kAxisRename{QStringLiteral("axisRenames"),
                                                 OPieceCartoucheOptionKind::AxisRename,
                                                 T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Rename the axis and its values"))};
    const OPieceCartoucheOptionField kMeasureRename{QStringLiteral("axisRenames"),
                                                    OPieceCartoucheOptionKind::AxisRename,
                                                    T(QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Rename the measurement"))};

    return {
        MakeSection("patternName",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Pattern name"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "The name of the pattern (set in the pattern toolbar)."), textFields),
        MakeSection("client",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Client"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "The client's name, birth date and email from the document."), textFields),
        MakeSection("pieceName",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Piece name"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "The name of the piece (from the piece options)."), textFields),
        MakeSection("pieceNumber",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Piece number"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "The piece number and its total, e.g. 1 of 10. The total shown here is an "
                                      "example — it is computed automatically from the number of pieces in the layout."),
                    {kPieceNumberStyle, kPieceSeparator, kFontFamily, kFontWeight, kFontSize, kLineSpacing}),
        MakeSection("cut",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Cut (Fabric / Interfacing / Lining)"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "One section covers all three fabrics — no need to add Cut several times. "
                                      "The values here are only an example so you can see how the section "
                                      "renders: in the pieces, the cut quantities are entered per piece and are "
                                      "empty by default."),
                    {kCut, kFontFamily, kFontWeight, kFontSize, kLineSpacing}),
        MakeSection("freeText",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Free text"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Free text entered per piece in the information block. It can span "
                                      "several lines."),
                    textFields),
        MakeSection("axis",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Sizes (Height, Commercial size, Group...)"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Requires a multi-size table (.omt). Shows a size-axis value under its user-given "
                                      "name. With a single-size table, this information is simply ignored."),
                    {kAxis, kSeparatorNoDash, kAxisRename, kFontFamily, kFontWeight, kFontSize, kLineSpacing}),
        MakeSection("measure",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Measurement"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Read from the pattern's size table. Shows the measurement's full name, its "
                                      "calculated value at the reference size and its unit."),
                    {kMeasureName, kSeparator, kMeasureRename, kFontFamily, kFontWeight, kFontSize, kLineSpacing}),
        MakeSection("logo",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Logo"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "A logo from your library (add your SVG logos in the Logo page)."),
                    {kLogo, kLogoSize, kLineSpacing}),
        MakeSection("collection",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Collection"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "The collection name."), textFields),
        MakeSection("fabricInfo",
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Fabric info"),
                    QT_TRANSLATE_NOOP("OPieceCartoucheSection", "Fabric information (nature, material, weight…)."), textFields),
    };
}

namespace
{
bool g_catalogueBuilt = false;
}

auto OPieceCartoucheCatalogue::Sections() -> const QList<OPieceCartoucheSection> &
{
    static QList<OPieceCartoucheSection> sections;
    if (!g_catalogueBuilt)
    {
        sections = Build();
        g_catalogueBuilt = true;
    }
    return sections;
}

void OPieceCartoucheCatalogue::InvalidateCache()
{
    g_catalogueBuilt = false;
}

auto OPieceCartoucheCatalogue::Section(const QString &id) -> const OPieceCartoucheSection &
{
    for (const OPieceCartoucheSection &section : Sections())
    {
        if (section.Id() == id)
        {
            return section;
        }
    }
    static const OPieceCartoucheSection kEmpty;
    return kEmpty;
}
