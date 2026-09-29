/************************************************************************
 **
 **  @file   mainwindow_selection.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Tool selection methods extracted from mainwindow.cpp
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
#include "mainwindow_helpers.h"
#include "vtheme.h"

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectPoint()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(true);
    emit EnableLineHover(false);
    emit EnableArcHover(false);
    emit EnableElArcHover(false);
    emit EnableSplineHover(false);
    emit EnableSplinePathHover(false);
    emit EnableImageBackgroundHover(false);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectPointByRelease()
{
    ToolSelectPoint();
    emit ItemsSelection(SelectionType::ByMouseRelease);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectPointByPress()
{
    ToolSelectPoint();
    emit ItemsSelection(SelectionType::ByMousePress);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectSpline()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(false);
    emit EnableLineHover(false);
    emit EnableArcHover(false);
    emit EnableElArcHover(false);
    emit EnableSplineHover(true);
    emit EnableSplinePathHover(false);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectSplinePath()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(false);
    emit EnableLineHover(false);
    emit EnableArcHover(false);
    emit EnableElArcHover(false);
    emit EnableSplineHover(false);
    emit EnableSplinePathHover(true);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectCutCurve()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(false);
    emit EnableLineHover(false);
    emit EnableArcHover(false);
    emit EnableElArcHover(false);
    emit EnableSplineHover(true);
    emit EnableSplinePathHover(true);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectArc()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(false);
    emit EnableLineHover(false);
    emit EnableArcHover(true);
    emit EnableElArcHover(false);
    emit EnableSplineHover(false);
    emit EnableSplinePathHover(false);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectArcElArc()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(false);
    emit EnableLineHover(false);
    emit EnableArcHover(true);
    emit EnableElArcHover(true);
    emit EnableSplineHover(false);
    emit EnableSplinePathHover(false);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectArcCurve()
{
    ToolSelectArcElArc();
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectPointArc()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(true);
    emit EnableLineHover(false);
    emit EnableArcHover(true);
    emit EnableElArcHover(false);
    emit EnableSplineHover(false);
    emit EnableSplinePathHover(false);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectCurve()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(false);
    emit EnableLineHover(false);
    emit EnableArcHover(true);
    emit EnableElArcHover(true);
    emit EnableSplineHover(true);
    emit EnableSplinePathHover(true);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectAllDrawObjects()
{
    // Only true for rubber band selection
    emit EnablePointSelection(false);
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    // Hovering
    emit EnablePointHover(true);
    emit EnableLineHover(false);
    emit EnableArcHover(true);
    emit EnableElArcHover(true);
    emit EnableSplineHover(true);
    emit EnableSplinePathHover(true);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectOperationObjects()
{
    emit EnablePointSelection(false);
    // Lines stay out of the shared selection for now: an operation tool only accepts them once its own
    // dialog routes SceneObject::Line to the line list (see MainWindow::ToolRotation).
    emit EnableLineSelection(false);
    emit EnableArcSelection(false);
    emit EnableElArcSelection(false);
    emit EnableSplineSelection(false);
    emit EnableSplinePathSelection(false);
    emit EnableBackgroundImageSelection(false);

    emit EnablePointHover(true);
    emit EnableLineHover(false);
    emit EnableArcHover(true);
    emit EnableElArcHover(true);
    emit EnableSplineHover(true);
    emit EnableSplinePathHover(true);
    emit EnableImageBackgroundHover(false);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowCustomRubberBand(true);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectOperationLines()
{
    // Lines are not geometric objects, but an operation can transform them: their two endpoints decide
    // whether the transformed line can exist (see OOperationLines). A line announces itself on click
    // (VToolLine::mouseReleaseEvent), so it is deliberately kept out of the Qt selection: that would draw
    // the native dotted frame instead of the preview highlight, which is what tells the user the line is
    // part of the operation.
    emit EnableLineHover(true);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolSelectDetail()
{
    // Only true for rubber band selection
    emit EnableNodePointSelection(false);
    emit EnableDetailSelection(true); // Disable when done visualization details

    // Hovering
    emit EnableNodePointHover(true);
    emit EnableDetailHover(true);

    emit ItemsSelection(SelectionType::ByMouseRelease);

    ui->view->AllowRubberBand(false);
}

