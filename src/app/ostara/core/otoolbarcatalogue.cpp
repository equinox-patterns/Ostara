/************************************************************************
 **
 **  @file   otoolbarcatalogue.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "otoolbarcatalogue.h"

#include "vtheme.h"

#include <QCoreApplication>
#include <QHash>

#include <algorithm>

namespace
{
// Every id the palette knows, with the label the palette writes in its tooltip. The strings live
// in the "DrawToolBar" context because that is where the translations already are.
auto ToolNames() -> const QHash<QString, const char *> &
{
    // clang-format off
    static const QHash<QString, const char *> names{
        // Selection
        {QStringLiteral("arrow_cursor"),                  QT_TRANSLATE_NOOP("DrawToolBar", "Select")},
        // Points
        {QStringLiteral("segment"),                       QT_TRANSLATE_NOOP("DrawToolBar", "Point at distance and angle")},
        {QStringLiteral("along_line"),                    QT_TRANSLATE_NOOP("DrawToolBar", "Point at distance along line")},
        {QStringLiteral("midpoint"),                      QT_TRANSLATE_NOOP("DrawToolBar", "Midpoint between two points")},
        {QStringLiteral("normal"),                        QT_TRANSLATE_NOOP("DrawToolBar", "Point along perpendicular")},
        {QStringLiteral("bisector"),                      QT_TRANSLATE_NOOP("DrawToolBar", "Point along bisector")},
        {QStringLiteral("height"),                        QT_TRANSLATE_NOOP("DrawToolBar", "Perpendicular point along line")},
        {QStringLiteral("point_of_intersection"),         QT_TRANSLATE_NOOP("DrawToolBar", "Point from X and Y of two other points")},
        {QStringLiteral("intersect"),                     QT_TRANSLATE_NOOP("DrawToolBar", "Point at line intersection")},
        {QStringLiteral("shoulder"),                      QT_TRANSLATE_NOOP("DrawToolBar", "Special point on shoulder")},
        {QStringLiteral("triangle"),                      QT_TRANSLATE_NOOP("DrawToolBar", "Triangle tool")},
        {QStringLiteral("line_intersect_axis"),           QT_TRANSLATE_NOOP("DrawToolBar", "Point intersect line and axis")},
        // Line
        {QStringLiteral("line"),                          QT_TRANSLATE_NOOP("DrawToolBar", "Line between points")},
        // Curves
        {QStringLiteral("cubic_bezier"),                  QT_TRANSLATE_NOOP("DrawToolBar", "Curve tool which uses point as control handle")},
        {QStringLiteral("cubic_bezier_path"),             QT_TRANSLATE_NOOP("DrawToolBar", "Curved path tool which uses point as control handle")},
        {QStringLiteral("curve_intersect_axis"),          QT_TRANSLATE_NOOP("DrawToolBar", "Point intersect curve and axis")},
        {QStringLiteral("intersection_curves"),           QT_TRANSLATE_NOOP("DrawToolBar", "Point intersection curves")},
        {QStringLiteral("parallel_curve"),                QT_TRANSLATE_NOOP("DrawToolBar", "Parallel curve tool")},
        {QStringLiteral("graduated_curve_offseting"),     QT_TRANSLATE_NOOP("DrawToolBar", "Graduated curve tool")},
        // Arcs
        {QStringLiteral("arc"),                           QT_TRANSLATE_NOOP("DrawToolBar", "Arc")},
        {QStringLiteral("arc_with_length"),               QT_TRANSLATE_NOOP("DrawToolBar", "Arc with given length")},
        {QStringLiteral("el_arc"),                        QT_TRANSLATE_NOOP("DrawToolBar", "Elliptical Arc")},
        {QStringLiteral("el_arc_with_length"),            QT_TRANSLATE_NOOP("DrawToolBar", "Elliptical arc with given length")},
        {QStringLiteral("arc_cut"),                       QT_TRANSLATE_NOOP("DrawToolBar", "Segment an arc")},
        {QStringLiteral("arc_start"),                     QT_TRANSLATE_NOOP("DrawToolBar", "Arc start point")},
        {QStringLiteral("arc_end"),                       QT_TRANSLATE_NOOP("DrawToolBar", "Arc end point")},
        {QStringLiteral("point_of_intersection_arcs"),    QT_TRANSLATE_NOOP("DrawToolBar", "Point of intersection arcs")},
        {QStringLiteral("arc_intersect_axis"),            QT_TRANSLATE_NOOP("DrawToolBar", "Point of intersection arc and axis")},
        {QStringLiteral("point_of_intersection_circles"), QT_TRANSLATE_NOOP("DrawToolBar", "Point of intersection circles")},
        {QStringLiteral("point_from_arc_and_tangent"),    QT_TRANSLATE_NOOP("DrawToolBar", "Point from arc and tangent")},
        {QStringLiteral("point_from_circle_and_tangent"), QT_TRANSLATE_NOOP("DrawToolBar", "Point from circle and tangent")},
        {QStringLiteral("point_of_contact"),              QT_TRANSLATE_NOOP("DrawToolBar", "Point of intersection circle and segment")},
        // Operations
        {QStringLiteral("flipping_axis"),                 QT_TRANSLATE_NOOP("DrawToolBar", "Flipping objects by axis")},
        {QStringLiteral("flipping_line"),                 QT_TRANSLATE_NOOP("DrawToolBar", "Flipping objects by line")},
        {QStringLiteral("rotation"),                      QT_TRANSLATE_NOOP("DrawToolBar", "Rotate objects")},
        {QStringLiteral("move"),                          QT_TRANSLATE_NOOP("DrawToolBar", "Move objects")},
        {QStringLiteral("true_darts"),                    QT_TRANSLATE_NOOP("DrawToolBar", "True darts")},
        // Pieces (Draw)
        {QStringLiteral("new_detail"),                    QT_TRANSLATE_NOOP("DrawToolBar", "Workpiece tool")},
        {QStringLiteral("path"),                          QT_TRANSLATE_NOOP("DrawToolBar", "Internal path tool")},
        {QStringLiteral("insert_node"),                   QT_TRANSLATE_NOOP("DrawToolBar", "Insert node tool")},
        {QStringLiteral("place_label"),                   QT_TRANSLATE_NOOP("DrawToolBar", "Internal mark")},
        // New
        {QStringLiteral("curve"),                         QT_TRANSLATE_NOOP("DrawToolBar", "Curve")},
        {QStringLiteral("curve_cut_point"),               QT_TRANSLATE_NOOP("DrawToolBar", "Segment a curve")},
        {QStringLiteral("armhole_curve"),                 QT_TRANSLATE_NOOP("DrawToolBar", "Armhole curve passing through 4 points")},
        {QStringLiteral("close_dart"),                    QT_TRANSLATE_NOOP("DrawToolBar", "Close dart tool")},
        {QStringLiteral("dart_cap"),                      QT_TRANSLATE_NOOP("DrawToolBar", "Dart cap tool")},
        {QStringLiteral("rectangle"),                     QT_TRANSLATE_NOOP("DrawToolBar", "Rectangle tool")},
        {QStringLiteral("transfert"),                     QT_TRANSLATE_NOOP("DrawToolBar", "Transfer tool")},
        {QStringLiteral("spiral"),                        QT_TRANSLATE_NOOP("DrawToolBar", "Spiral tool")},
        {QStringLiteral("spread"),                        QT_TRANSLATE_NOOP("DrawToolBar", "Slash and spread tool")},
        {QStringLiteral("move_batch"),                    QT_TRANSLATE_NOOP("DrawToolBar", "Move by batch tool")},
        {QStringLiteral("pleat"),                         QT_TRANSLATE_NOOP("DrawToolBar", "Pleat tool")},
        {QStringLiteral("pleat_mid"),                     QT_TRANSLATE_NOOP("DrawToolBar", "Middle pleat tool")},
        {QStringLiteral("flounce_spiral"),                QT_TRANSLATE_NOOP("DrawToolBar", "Flounce spiral tool")},
        // The other modes
        {QStringLiteral("export_templates"),              QT_TRANSLATE_NOOP("DrawToolBar", "Export templates")},
        {QStringLiteral("export_image"),                  QT_TRANSLATE_NOOP("DrawToolBar", "Export as image...")},
        {QStringLiteral("union"),                         QT_TRANSLATE_NOOP("DrawToolBar", "Union tool")},
        {QStringLiteral("duplicate_detail"),              QT_TRANSLATE_NOOP("DrawToolBar", "Duplicate detail tool")},
        {QStringLiteral("detail_export"),                 QT_TRANSLATE_NOOP("DrawToolBar", "Export")},
    };
    // clang-format on
    return names;
}

/// The buttons that open a window instead of arming a tool.
auto IsWindowButton(const QString &id) -> bool
{
    return id == QStringLiteral("detail_export") || id == QStringLiteral("export_templates")
           || id == QStringLiteral("export_image");
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::ToolName(const QString &id) -> QString
{
    const auto &names = ToolNames();
    const auto it = names.constFind(id);
    if (it == names.cend())
    {
        return {};
    }
    return QCoreApplication::translate("DrawToolBar", it.value());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::IsCheckable(const QString &id) -> bool
{
    return not IsWindowButton(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::IconFor(const QString &id) -> QIcon
{
    const QString theme = VTheme::ColorSheme() == VColorSheme::Dark ? QStringLiteral("dark")
                                                                    : QStringLiteral("light");

    // Three tools carry an icon our library sets, because there is no <id>.svg for them under
    // :/tools/. They take the theme variant, so they follow Light and Dark like the rest.
    if (id == QStringLiteral("detail_export"))
    {
        return QIcon(QStringLiteral(":/icons/%1/file-image_%1.svg").arg(theme));
    }
    if (id == QStringLiteral("export_templates"))
    {
        return QIcon(QStringLiteral(":/icons/%1/grid-four-fill_%1.svg").arg(theme));
    }
    if (id == QStringLiteral("export_image"))
    {
        return QIcon(QStringLiteral(":/icons/%1/file-image-fill_%1.svg").arg(theme));
    }

    // Every tool has its own artwork now, in both themes: `<id>.svg` under :/tools/.
    return QIcon(QStringLiteral(":/tools/%1/%2.svg").arg(theme, id));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::DrawGroups() -> QVector<Group>
{
    // clang-format off
    return {
        {QStringLiteral("selection"),  GroupName(QStringLiteral("selection")),
         {QStringLiteral("arrow_cursor")}, ModeDraw},
        {QStringLiteral("points"),     GroupName(QStringLiteral("points")),
         {QStringLiteral("segment"), QStringLiteral("along_line"), QStringLiteral("midpoint"),
          QStringLiteral("normal"), QStringLiteral("bisector"), QStringLiteral("height"),
          QStringLiteral("point_of_intersection"), QStringLiteral("intersect"),
          QStringLiteral("shoulder"), QStringLiteral("triangle"),
          QStringLiteral("line_intersect_axis"), QStringLiteral("rectangle")}, ModeDraw},
        {QStringLiteral("line"),       GroupName(QStringLiteral("line")),
         {QStringLiteral("line")}, ModeDraw},
        {QStringLiteral("curves"),     GroupName(QStringLiteral("curves")),
         {QStringLiteral("cubic_bezier"), QStringLiteral("cubic_bezier_path"),
          QStringLiteral("curve_intersect_axis"), QStringLiteral("intersection_curves"),
          QStringLiteral("parallel_curve"), QStringLiteral("graduated_curve_offseting"),
          QStringLiteral("curve"), QStringLiteral("curve_cut_point"), QStringLiteral("armhole_curve"),
          QStringLiteral("spiral"), QStringLiteral("flounce_spiral")}, ModeDraw},
        {QStringLiteral("arcs"),       GroupName(QStringLiteral("arcs")),
         {QStringLiteral("arc"), QStringLiteral("arc_with_length"), QStringLiteral("el_arc"),
          QStringLiteral("el_arc_with_length"), QStringLiteral("arc_cut"), QStringLiteral("arc_start"),
          QStringLiteral("arc_end"), QStringLiteral("point_of_intersection_arcs"),
          QStringLiteral("arc_intersect_axis"), QStringLiteral("point_of_intersection_circles"),
          QStringLiteral("point_from_arc_and_tangent"), QStringLiteral("point_from_circle_and_tangent"),
          QStringLiteral("point_of_contact")}, ModeDraw},
        {QStringLiteral("operations"), GroupName(QStringLiteral("operations")),
         {QStringLiteral("flipping_axis"), QStringLiteral("flipping_line"), QStringLiteral("rotation"),
          QStringLiteral("move"), QStringLiteral("true_darts"), QStringLiteral("close_dart"),
          QStringLiteral("dart_cap"), QStringLiteral("transfert"), QStringLiteral("spread"),
          QStringLiteral("move_batch"), QStringLiteral("pleat"), QStringLiteral("pleat_mid")}, ModeDraw},
        {QStringLiteral("pieces"),     GroupName(QStringLiteral("pieces")),
         {QStringLiteral("new_detail"), QStringLiteral("path"), QStringLiteral("insert_node"),
          QStringLiteral("place_label")}, ModeDraw},
    };
    // clang-format on
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::HiddenByDefault() -> QStringList
{
    // Arc start and Arc end are redundant with the "create start/end point" boxes of Arc and Arc
    // with given length; True darts is superseded by Close dart. Hidden, not removed: the three
    // stay in their group's table above, which is what keeps them present in the dispatch list.
    return {QStringLiteral("arc_start"), QStringLiteral("arc_end"), QStringLiteral("true_darts")};
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::OtherGroups() -> QVector<Group>
{
    // A group of one in these modes is structural, not tidy: outside Draw::Calculation there is no
    // caret, so a second tool of a group that the row budget cannot fit is hidden with nowhere to
    // find it. A group of one cannot overflow.
    return {
        {QStringLiteral("selectionDetail"), GroupName(QStringLiteral("selection")),
         {QStringLiteral("arrow_cursor")}, ModeDetail | ModeExport},
        {QStringLiteral("exportTemplates"), GroupName(QStringLiteral("exportTemplates")),
         {QStringLiteral("export_templates")}, ModeExport},
        {QStringLiteral("exportImage"), GroupName(QStringLiteral("exportImage")),
         {QStringLiteral("export_image")}, ModeExport},
        {QStringLiteral("piecesDetail"), GroupName(QStringLiteral("pieces")),
         {QStringLiteral("union"), QStringLiteral("duplicate_detail")}, ModeDetail},
        {QStringLiteral("piecesDetailExport"), GroupName(QStringLiteral("pieces")),
         {QStringLiteral("detail_export")}, ModeDetail},
    };
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::GroupName(const QString &key) -> QString
{
    if (key == QStringLiteral("selection"))
    {
        return QCoreApplication::translate("DrawToolBar", "Selection");
    }
    if (key == QStringLiteral("points"))
    {
        return QCoreApplication::translate("DrawToolBar", "Points");
    }
    if (key == QStringLiteral("line"))
    {
        return QCoreApplication::translate("DrawToolBar", "Line");
    }
    if (key == QStringLiteral("curves"))
    {
        return QCoreApplication::translate("DrawToolBar", "Curves");
    }
    if (key == QStringLiteral("arcs"))
    {
        return QCoreApplication::translate("DrawToolBar", "Arcs");
    }
    if (key == QStringLiteral("operations"))
    {
        return QCoreApplication::translate("DrawToolBar", "Operations");
    }
    if (key == QStringLiteral("pieces"))
    {
        return QCoreApplication::translate("DrawToolBar", "Pieces");
    }
    if (key == QStringLiteral("exportTemplates"))
    {
        return QCoreApplication::translate("DrawToolBar", "Export templates");
    }
    if (key == QStringLiteral("exportImage"))
    {
        return QCoreApplication::translate("DrawToolBar", "Export image");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::Tools() -> QVector<Tool>
{
    QVector<Tool> tools;

    const auto Append = [&tools](const QStringList &ids) {
        for (const QString &id : ids)
        {
            const auto known = std::find_if(tools.cbegin(), tools.cend(),
                                            [&id](const Tool &tool) { return tool.id == id; });
            if (known == tools.cend())
            {
                tools.append(Tool{id, ToolName(id)});
            }
        }
    };

    for (const Group &group : DrawGroups())
    {
        Append(group.tools);
    }
    for (const Group &group : OtherGroups())
    {
        Append(group.tools);
    }

    return tools;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarCatalogue::DrawTools() -> QVector<Tool>
{
    QVector<Tool> tools;
    for (const Group &group : DrawGroups())
    {
        for (const QString &id : group.tools)
        {
            const auto known = std::find_if(tools.cbegin(), tools.cend(),
                                            [&id](const Tool &tool) { return tool.id == id; });
            if (known == tools.cend())
            {
                tools.append(Tool{id, ToolName(id)});
            }
        }
    }
    return tools;
}
