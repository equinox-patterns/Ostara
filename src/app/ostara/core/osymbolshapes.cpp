/************************************************************************
 **
 **  @file   osymbolshapes.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The marks Ostara knows how to draw, and the words they are
 **          shown under — one list, read by both.
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

#include "osymbolshapes.h"

#include <QCoreApplication>

namespace OSymbolShapes
{
//---------------------------------------------------------------------------------------------------------------------
// The two lines have no table of their own — there is one shape per family — but their names are
// looked up in the context that already carries them, like a notch's is in the dock's. Borrowing it
// costs nothing and buys the words that exist: a translator has already written "Droit-fil" for
// "Grainline", and inventing a second context would only ask for them again.
namespace
{
constexpr const char *kGrainlineContext = "DialogSeamAllowance";
constexpr const char *kGrainlineName = QT_TRANSLATE_NOOP("DialogSeamAllowance", "Grainline");
constexpr const char *kFoldLineName = QT_TRANSLATE_NOOP("DialogSeamAllowance", "Fold line");

constexpr const char *kPassmarkContext = "ODetailsNodeWidget";
constexpr const char *kPlaceLabelContext = "ODetailsPlaceLabelWidget";
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto Passmarks() -> const QVector<PassmarkEntry> &
{
    // A "Check mark" is one type in the enumeration and two shapes on the canvas, told apart by the
    // opening it is drawn with — hence two entries for it here, as the dock has always had.
    static const QVector<PassmarkEntry> table = {
        {PassmarkLineType::OneLine, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "One line"), false},
        {PassmarkLineType::TwoLines, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Two lines"), false},
        {PassmarkLineType::ThreeLines, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Three lines"), false},
        {PassmarkLineType::TMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "T mark"), false},
        {PassmarkLineType::ExternalVMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "External V mark"), false},
        {PassmarkLineType::InternalVMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Internal V mark"), false},
        {PassmarkLineType::UMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "U mark"), false},
        {PassmarkLineType::BoxMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Box mark"), false},
        {PassmarkLineType::CheckMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Check mark"), false},
        {PassmarkLineType::CheckMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Check mark clockwise"), true},
    };
    return table;
}

//---------------------------------------------------------------------------------------------------------------------
auto PlaceLabels() -> const QVector<PlaceLabelEntry> &
{
    static const QVector<PlaceLabelEntry> table = {
        {PlaceLabelType::Cross, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Cross")},
        {PlaceLabelType::Segment, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Segment")},
        {PlaceLabelType::Rectangle, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Rectangle")},
        {PlaceLabelType::Tshaped, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "T-shaped")},
        {PlaceLabelType::Doubletree, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Doubletree")},
        {PlaceLabelType::Corner, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Corner")},
        {PlaceLabelType::Triangle, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Triangle")},
        {PlaceLabelType::Hshaped, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "H-shaped")},
        {PlaceLabelType::Button, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Button")},
        {PlaceLabelType::Circle, QT_TRANSLATE_NOOP("ODetailsPlaceLabelWidget", "Circle")},
    };
    return table;
}

//---------------------------------------------------------------------------------------------------------------------
auto All() -> QVector<Shape>
{
    QVector<Shape> shapes;

    // The two lines first, then the notches, then the marks placed inside a piece: the order the
    // families are read in, from the one every pattern carries to the ones only some do.
    shapes.append({Family::Grainline, 0, false});
    shapes.append({Family::FoldLine, 0, false});

    for (const PassmarkEntry &entry : Passmarks())
    {
        shapes.append({Family::Passmark, static_cast<int>(entry.type), entry.clockwise});
    }

    for (const PlaceLabelEntry &entry : PlaceLabels())
    {
        shapes.append({Family::PlaceLabel, static_cast<int>(entry.type), false});
    }

    return shapes;
}

//---------------------------------------------------------------------------------------------------------------------
auto Name(const Shape &shape) -> QString
{
    switch (shape.family)
    {
        case Family::Grainline:
            return QCoreApplication::translate(kGrainlineContext, kGrainlineName);
        case Family::FoldLine:
            return QCoreApplication::translate(kGrainlineContext, kFoldLineName);
        case Family::Passmark:
            for (const PassmarkEntry &entry : Passmarks())
            {
                if (static_cast<int>(entry.type) == shape.type && entry.clockwise == shape.clockwise)
                {
                    return QCoreApplication::translate(kPassmarkContext, entry.name);
                }
            }
            break;
        case Family::PlaceLabel:
            for (const PlaceLabelEntry &entry : PlaceLabels())
            {
                if (static_cast<int>(entry.type) == shape.type)
                {
                    return QCoreApplication::translate(kPlaceLabelContext, entry.name);
                }
            }
            break;
    }

    // A shape that names no mark has no name to give. Nothing calls this with one — a shape is built
    // from these tables or read back through FromKey — and answering nothing is better than naming
    // a mark that is not there.
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto Key(const Shape &shape) -> QString
{
    switch (shape.family)
    {
        case Family::Grainline:
            return QStringLiteral("grainline");
        case Family::FoldLine:
            return QStringLiteral("foldline");
        case Family::Passmark:
            return QStringLiteral("passmark:%1:%2").arg(shape.type).arg(shape.clockwise ? 1 : 0);
        case Family::PlaceLabel:
            return QStringLiteral("placelabel:%1").arg(shape.type);
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto FromKey(const QString &key) -> std::optional<Shape>
{
    // **Built by asking every shape for its token, rather than by taking the token apart.** What a
    // key is written as then exists in one place only, and the two can never drift: a shape added to
    // a table above is readable from an index without anything here being touched.
    for (const Shape &shape : All())
    {
        if (Key(shape) == key)
        {
            return shape;
        }
    }

    return std::nullopt;
}
} // namespace OSymbolShapes
