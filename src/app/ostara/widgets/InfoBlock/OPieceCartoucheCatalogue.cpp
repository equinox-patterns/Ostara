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
}  // namespace

auto OPieceCartoucheCatalogue::Build() -> QList<OPieceCartoucheSection>
{
    const OPieceCartoucheOptionField kFontFamily{QStringLiteral("fontFamily"),
                                                 OPieceCartoucheOptionKind::FontFamily, T("Font family")};
    const OPieceCartoucheOptionField kFontWeight{QStringLiteral("fontWeight"),
                                                 OPieceCartoucheOptionKind::FontWeight, T("Font weight")};
    const OPieceCartoucheOptionField kFontSize{QStringLiteral("fontSize"),
                                               OPieceCartoucheOptionKind::FontSize, T("Font size (px)")};
    const OPieceCartoucheOptionField kAxis{QStringLiteral("axis"), OPieceCartoucheOptionKind::Axis, T("Axis")};
    const OPieceCartoucheOptionField kMeasureName{QStringLiteral("measureName"),
                                                  OPieceCartoucheOptionKind::MeasureName, T("Measurement")};
    const OPieceCartoucheOptionField kSeparator{QStringLiteral("separator"),
                                                OPieceCartoucheOptionKind::Separator, T("Separator")};
    const OPieceCartoucheOptionField kLogo{QStringLiteral("logo"), OPieceCartoucheOptionKind::Logo, T("Logo")};
    const OPieceCartoucheOptionField kLogoWidth{QStringLiteral("logoWidth"),
                                                OPieceCartoucheOptionKind::LogoWidth, T("Logo width (cm)")};
    const OPieceCartoucheOptionField kLogoHeight{QStringLiteral("logoHeight"),
                                                 OPieceCartoucheOptionKind::LogoHeight, T("Logo height (cm)")};

    const QList<OPieceCartoucheOptionField> textFields = {kFontFamily, kFontWeight, kFontSize};

    return {
        OPieceCartoucheSection(QStringLiteral("patternName"), T("Pattern name"),
                               T("The name of the pattern (set in the pattern toolbar)."), textFields),
        OPieceCartoucheSection(QStringLiteral("company"), T("Brand / Company"),
                               T("The brand or company name stored in the document."), textFields),
        OPieceCartoucheSection(QStringLiteral("client"), T("Client"),
                               T("The client's name, birth date and email from the document."), textFields),
        OPieceCartoucheSection(QStringLiteral("pieceName"), T("Piece name"),
                               T("The name of the piece (from the piece options)."), textFields),
        OPieceCartoucheSection(QStringLiteral("pieceNumber"), T("Piece number"),
                               T("The piece number, entered in the information block."), textFields),
        OPieceCartoucheSection(QStringLiteral("cut"), T("Cut (Fabric / Interfacing / Lining)"),
                               T("Cut quantity per fabric — only filled tabs are shown."), textFields),
        OPieceCartoucheSection(QStringLiteral("freeText"), T("Free text"),
                               T("Free text entered per piece in the information block."), textFields),
        OPieceCartoucheSection(QStringLiteral("axis"), T("Sizes (Height, Commercial size, Group...)"),
                               T("Requires a multi-size table (.omt). Shows a size-axis value under its user-given "
                                 "name. With a single-size table, this information is simply ignored."),
                               {kAxis, kSeparator, kFontFamily, kFontWeight, kFontSize}),
        OPieceCartoucheSection(QStringLiteral("measure"), T("Measurement"),
                               T("Read from the pattern's size table. Shows the measurement's full name, its "
                                 "calculated value at the reference size and its unit."),
                               {kMeasureName, kSeparator, kFontFamily, kFontWeight, kFontSize}),
        OPieceCartoucheSection(QStringLiteral("logo"), T("Logo"),
                               T("The reference logo (configured in the settings)."),
                               {kLogo, kLogoWidth, kLogoHeight, kFontFamily, kFontWeight, kFontSize}),
        OPieceCartoucheSection(QStringLiteral("collection"), T("Collection"),
                               T("The collection name."), textFields),
        OPieceCartoucheSection(QStringLiteral("fabricInfo"), T("Fabric info"),
                               T("Fabric information (nature, material, weight…)."), textFields),
    };
}

auto OPieceCartoucheCatalogue::Sections() -> const QList<OPieceCartoucheSection> &
{
    static const QList<OPieceCartoucheSection> sections = Build();
    return sections;
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
