/************************************************************************
 **
 **  @file   mainwindow_icons.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  SetupDrawToolsIcons extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "vtheme.h"

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetupDrawToolsIcons()
{
    const auto resource = QStringLiteral("toolicon");

    // This check helps to find missed tools
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were handled.");

    ui->actionLineTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("line.png")));
    ui->actionEndLineTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("segment.png")));
    ui->actionAlongLineTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("along_line.png")));
    ui->actionMidpointTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("midpoint.png")));
    ui->actionNormalTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("normal.png")));
    ui->actionBisectorTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("bisector.png")));
    ui->actionHeightTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("height.png")));
    ui->actionPointOfIntersectionTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("point_of_intersection.png")));
    ui->actionLineIntersectTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("intersect.png")));
    ui->actionShoulderPointTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("shoulder.png")));
    ui->actionTriangleTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("triangle.png")));
    ui->actionLineIntersectAxisTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("line_intersect_axis.png")));
    ui->actionCurveIntersectAxisTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("curve_intersect_axis.png")));
    ui->actionArcIntersectAxisTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("arc_intersect_axis.png")));
    ui->actionSplineCutPointTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("spline_cut_point.png")));
    ui->actionSplinePathCutPointTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("splinePath_cut_point.png")));
    ui->actionArcCutPointTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("arc_cut.png")));
    ui->actionIntersectionCurvesTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("intersection_curves.png")));
    ui->actionPointOfIntersectionArcsTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("point_of_intersection_arcs.png")));
    ui->actionPointOfIntersectionCirclesTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("point_of_intersection_circles.png")));
    ui->actionPointFromArcAndTangentTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("point_from_arc_and_tangent.png")));
    ui->actionPointFromCircleAndTangentTool->setIcon(
        VTheme::GetIconResource(resource, QStringLiteral("point_from_circle_and_tangent.png")));
    ui->actionPointOfContactTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("point_of_contact.png")));
    ui->actionSplineTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("spline.png")));
    ui->actionCubicBezierTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("cubic_bezier.png")));
    ui->actionSplinePathTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("splinePath.png")));
    ui->actionCubicBezierPathTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("cubic_bezier_path.png")));
    ui->actionArcTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("arc.png")));
    ui->actionArcWithLengthTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("arc_with_length.png")));
    ui->actionEllipticalArcTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("el_arc.png")));
    ui->actionGroupTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("group_plus.png")));
    ui->actionFlippingByAxisTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("flipping_axis.png")));
    ui->actionFlippingByLineTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("flipping_line.png")));
    ui->actionRotationTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("rotation.png")));
    ui->actionMoveTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("move.png")));
    ui->actionTrueDartsTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("true_darts.png")));
    ui->actionNewDetailTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("new_detail.png")));
    ui->actionUnionDetailsTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("union.png")));
    ui->actionDuplicateDetailTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("duplicate_detail.png")));
    ui->actionInternalPathTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("path.png")));
    ui->actionPinTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("pin.png")));
    ui->actionInsertNodeTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("insert_node.png")));
    ui->actionPlaceLabelTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("place_label.png")));
    ui->actionArcStartPointTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("arc_start.png")));
    ui->actionArcEndPointTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("arc_end.png")));
    ui->actionEllipticalArcWithLengthTool->setIcon(
                VTheme::GetIconResource(resource, QStringLiteral("el_arc_with_length.png")));
    ui->actionParallelCurveTool->setIcon(VTheme::GetIconResource(resource, QStringLiteral("parallel_curve.png")));
    ui->actionGraduatedCurveTool->setIcon(
                VTheme::GetIconResource(resource, QStringLiteral("graduated_curve_offseting.png")));
}
