/************************************************************************
 **
 **  @file   mainwindow_tools.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Tool slot methods extracted from mainwindow.cpp
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

// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "mainwindow.h"
#include "widgets/BreadcrumbBar.h"
#include "widgets/ODockManager.h"
#include "ui_mainwindow.h"
#include "mainwindow_helpers.h"
#include "vtheme.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmodifierkey.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vtools/dialogs/tools/piece/dialogplacelabel.h"
#include "../vtools/tools/nodeDetails/vtoolplacelabel.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vtoolbuttonpopup.h"
#include "../vwidgets/vwidgetpopup.h"
#include "core/vapplication.h"

// Dialog headers (one per tool)
#include "../vtools/dialogs/tools/dialogalongline.h"
#include "../vtools/dialogs/tools/dialogarc.h"
#include "../vtools/dialogs/tools/dialogarcwithlength.h"
#include "../vtools/dialogs/tools/dialogbisector.h"
#include "../vtools/dialogs/tools/dialogcubicbezier.h"
#include "../vtools/dialogs/tools/dialogcubicbezierpath.h"
#include "../vtools/dialogs/tools/dialogcurveintersectaxis.h"
#include "../vtools/dialogs/tools/dialogcutarc.h"
#include "../vtools/dialogs/tools/dialogcutspline.h"
#include "../vtools/dialogs/tools/dialogcutsplinepath.h"
#include "../vtools/dialogs/tools/dialogcutcurve.h"
#include "../vtools/dialogs/tools/dialogellipticalarc.h"
#include "../vtools/dialogs/tools/dialogellipticalarcwithlength.h"
#include "../vtools/dialogs/tools/dialogendline.h"
#include "../vtools/dialogs/tools/dialogflippingbyaxis.h"
#include "../vtools/dialogs/tools/dialogflippingbyline.h"
#include "../vtools/dialogs/tools/dialoggroup.h"
#include "../vtools/dialogs/tools/dialogattach.h"
#include "../vtools/dialogs/tools/dialogheight.h"
#include "../vtools/dialogs/tools/dialogline.h"
#include "../vtools/dialogs/tools/dialoglineintersect.h"
#include "../vtools/dialogs/tools/dialoglineintersectaxis.h"
#include "../vtools/dialogs/tools/dialogmove.h"
#include "../vtools/dialogs/tools/dialognormal.h"
#include "../vtools/dialogs/tools/dialogpointfromarcandtangent.h"
#include "../vtools/dialogs/tools/dialogpointfromcircleandtangent.h"
#include "../vtools/dialogs/tools/dialogpointofcontact.h"
#include "../vtools/dialogs/tools/dialogpointofintersection.h"
#include "../vtools/dialogs/tools/dialogpointofintersectionarcs.h"
#include "../vtools/dialogs/tools/dialogpointofintersectioncircles.h"
#include "../vtools/dialogs/tools/dialogpointofintersectioncurves.h"
#include "../vtools/dialogs/tools/dialogrotation.h"
#include "../vtools/dialogs/tools/dialogshoulderpoint.h"
#include "../vtools/dialogs/tools/dialogspline.h"
#include "../vtools/dialogs/tools/dialogsplinepath.h"
#include "../vtools/dialogs/tools/otoolcurve.h"
#include "../vtools/dialogs/tools/otoolarmholecurvecollector.h"
#include "../vtools/dialogs/tools/dialogtriangle.h"
#include "../vtools/dialogs/tools/dialogtruedarts.h"
#include "../vtools/dialogs/tools/dialoguniondetails.h"
#include "../vtools/dialogs/tools/dialoggraduatedcurve.h"
#include "../vtools/dialogs/tools/dialogparallelcurve.h"
#include "../vtools/dialogs/tools/piece/dialogduplicatedetail.h"
#include "../vtools/dialogs/tools/piece/dialoginsertnode.h"
#include "../vtools/dialogs/tools/piece/dialogpiecepath.h"
#include "../vtools/dialogs/tools/piece/dialogpin.h"
#include "../vtools/dialogs/tools/piece/dialogseamallowance.h"
#include "opiecedialog.h"

// VTool headers (one per tool)
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/otoolarmholecurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooltruedarts.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolalongline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolbisector.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolendline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolheight.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollineintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolnormal.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolshoulderpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoollineintersect.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromarcandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromcircleandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofcontact.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersection.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectionarcs.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncircles.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtooltriangle.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/nodeDetails/vtoolpiecepath.h"
#include "../vtools/tools/nodeDetails/vtoolpin.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"

// Docks used by tool callbacks
#include "dialogs/vwidgetgroups.h"
#include "dialogs/vwidgetdependencies.h"

// Undo commands used by tool callbacks
#include "../vtools/undocommands/renamepp.h"
#include "../vtools/undocommands/undogroup.h"
#include "../vtools/undocommands/undoattachment.h"

// Other needed headers
#include "../vmisc/def.h"
#include "../vpatterndb/variables/vmeasurement.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QtDebug>
#include <QIODevice>
#include <QMessageBox>
#include <QPointer>
#include <QProcess>

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedDialog handle close dialog
 * @param result result working dialog.
 */
template <typename DrawTool> void MainWindow::ClosedDialog(int result)
{
    SCASSERT(not m_dialogTool.isNull())
    if (result == QDialog::Accepted)
    {
        auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
        SCASSERT(scene != nullptr)

        auto *tool = dynamic_cast<QGraphicsItem *>(DrawTool::Create(m_dialogTool, scene, doc, pattern));
        // Do not check for nullptr! See issue #719.
        emit ui->view->itemClicked(tool);
    }
    ArrowTool(true);
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedDialogWithApply handle close dialog that has apply button
 * @param result result working dialog.
 */
template <typename DrawTool> void MainWindow::ClosedDialogWithApply(int result, VMainGraphicsScene *scene)
{
    SCASSERT(not m_dialogTool.isNull())
    if (result == QDialog::Accepted)
    {
        ApplyDialog<DrawTool>(scene);
    }
    // If before Cancel was used Apply we have an item
    auto *vtool = qobject_cast<DrawTool *>(m_dialogTool->GetAssociatedTool()); // Don't check for nullptr here
    if (m_dialogTool->GetAssociatedTool() != nullptr)
    {
        SCASSERT(vtool != nullptr)
        vtool->DialogLinkDestroy();
        connect(vtool, &DrawTool::ToolTip, this, &MainWindow::ShowToolTip);
    }
    ArrowTool(true);
    ui->view->itemClicked(vtool); // Don't check for nullptr here
    if (vtool)
    {
        vtool->setFocus();
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolEndLine handler tool endLine.
 * @param checked true - button checked.
 */
void MainWindow::ToolEndLine(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogEndLine>(checked, Tool::EndLine, QStringLiteral("segment_cursor.png"),
                                          tr("Select point"), &MainWindow::ClosedDrawDialogWithApply<VToolEndLine>,
                                          &MainWindow::ApplyDrawDialog<VToolEndLine>);
    LogPatternToolUsed(checked, QStringLiteral("Segment tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolLine handler tool line.
 * @param checked true - button checked.
 */
void MainWindow::ToolLine(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogLine>(checked, Tool::Line, QStringLiteral("line_cursor.png"), tr("Select first point"),
                                       &MainWindow::ClosedDrawDialogWithApply<VToolLine>,
                                       &MainWindow::ApplyDrawDialog<VToolLine>);
    LogPatternToolUsed(checked, QStringLiteral("Line tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolAlongLine handler tool alongLine.
 * @param checked true - button checked.
 */
void MainWindow::ToolAlongLine(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogAlongLine>(checked, Tool::AlongLine, QStringLiteral("along_line_cursor.png"),
                                            tr("Select point"), &MainWindow::ClosedDrawDialogWithApply<VToolAlongLine>,
                                            &MainWindow::ApplyDrawDialog<VToolAlongLine>);
    LogPatternToolUsed(checked, QStringLiteral("Along line tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolMidpoint(bool checked)
{
    ToolSelectPointByRelease();
    // Reuse DialogAlongLine and VToolAlongLine but with different cursor
    SetToolButtonWithApply<DialogAlongLine>(checked, Tool::Midpoint, QStringLiteral("midpoint_cursor.png"),
                                            tr("Select point"), &MainWindow::ClosedDrawDialogWithApply<VToolAlongLine>,
                                            &MainWindow::ApplyDrawDialog<VToolAlongLine>);
    LogPatternToolUsed(checked, QStringLiteral("Midpoint tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolShoulderPoint handler tool shoulderPoint.
 * @param checked true - button checked.
 */
void MainWindow::ToolShoulderPoint(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogShoulderPoint>(
        checked, Tool::ShoulderPoint, QStringLiteral("shoulder_cursor.png"), tr("Select point"),
        &MainWindow::ClosedDrawDialogWithApply<VToolShoulderPoint>, &MainWindow::ApplyDrawDialog<VToolShoulderPoint>);
    LogPatternToolUsed(checked, QStringLiteral("Shoulder point tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolNormal handler tool normal.
 * @param checked true - button checked.
 */
void MainWindow::ToolNormal(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogNormal>(
        checked, Tool::Normal, QStringLiteral("normal_cursor.png"), tr("Select first point of line"),
        &MainWindow::ClosedDrawDialogWithApply<VToolNormal>, &MainWindow::ApplyDrawDialog<VToolNormal>);
    LogPatternToolUsed(checked, QStringLiteral("Normal tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolBisector handler tool bisector.
 * @param checked true - button checked.
 */
void MainWindow::ToolBisector(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogBisector>(
        checked, Tool::Bisector, QStringLiteral("bisector_cursor.png"), tr("Select first point of angle"),
        &MainWindow::ClosedDrawDialogWithApply<VToolBisector>, &MainWindow::ApplyDrawDialog<VToolBisector>);
    LogPatternToolUsed(checked, QStringLiteral("Bisector tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolLineIntersect handler tool lineIntersect.
 * @param checked true - button checked.
 */
void MainWindow::ToolLineIntersect(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogLineIntersect>(
        checked, Tool::LineIntersect, QStringLiteral("intersect_cursor.png"), tr("Select first point of first line"),
        &MainWindow::ClosedDrawDialogWithApply<VToolLineIntersect>, &MainWindow::ApplyDrawDialog<VToolLineIntersect>);
    LogPatternToolUsed(checked, QStringLiteral("Line intersect tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolSpline handler tool spline.
 * @param checked true - button checked.
 */
void MainWindow::ToolSpline(bool checked)
{
    ToolSelectPointByPress();
    SetToolButtonWithApply<DialogSpline>(
        checked, Tool::Spline, QStringLiteral("spline_cursor.png"), tr("Select first point curve"),
        &MainWindow::ClosedDrawDialogWithApply<VToolSpline>, &MainWindow::ApplyDrawDialog<VToolSpline>);
    LogPatternToolUsed(checked, QStringLiteral("Spline tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolCubicBezier(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogCubicBezier>(
        checked, Tool::CubicBezier, QStringLiteral("cubic_bezier_cursor.png"), tr("Select first curve point"),
        &MainWindow::ClosedDrawDialogWithApply<VToolCubicBezier>, &MainWindow::ApplyDrawDialog<VToolCubicBezier>);
    LogPatternToolUsed(checked, QStringLiteral("Cubic bezier tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolCutSpline handler tool CutSpline.
 * @param checked true - button is checked
 */
void MainWindow::ToolCutSpline(bool checked)
{
    ToolSelectSpline();
    SetToolButtonWithApply<DialogCutSpline>(
        checked, Tool::CutSpline, QStringLiteral("spline_cut_point_cursor.png"), tr("Select simple curve"),
        &MainWindow::ClosedDrawDialogWithApply<VToolCutSpline>, &MainWindow::ApplyDrawDialog<VToolCutSpline>);
    LogPatternToolUsed(checked, QStringLiteral("Cut spline tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolArc handler tool arc.
 * @param checked true - button checked.
 */
void MainWindow::ToolArc(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogArc>(
        checked, Tool::Arc, QStringLiteral("arc_cursor.png"), tr("Select point of center of arc"),
        &MainWindow::ClosedDrawDialogWithApply<VToolArc>, &MainWindow::ApplyDrawDialog<VToolArc>);
    LogPatternToolUsed(checked, QStringLiteral("Arc tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolEllipticalArc handler tool arc.
 * @param checked true - button checked.
 */
void MainWindow::ToolEllipticalArc(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogEllipticalArc>(checked, Tool::EllipticalArc, QStringLiteral("el_arc_cursor.png"),
                                                tr("Select point of center of elliptical arc"),
                                                &MainWindow::ClosedDrawDialogWithApply<VToolEllipticalArc>,
                                                &MainWindow::ApplyDrawDialog<VToolEllipticalArc>);
    LogPatternToolUsed(checked, QStringLiteral("Elliptical arc tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolSplinePath handler tool splinePath.
 * @param checked true - button checked.
 */
void MainWindow::ToolSplinePath(bool checked)
{
    ToolSelectPointByPress();
    SetToolButtonWithApply<DialogSplinePath>(
        checked, Tool::SplinePath, QStringLiteral("splinePath_cursor.png"), tr("Select point of curve path"),
        &MainWindow::ClosedDrawDialogWithApply<VToolSplinePath>, &MainWindow::ApplyDrawDialog<VToolSplinePath>);
    LogPatternToolUsed(checked, QStringLiteral("Spline path tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolCurve(bool checked)
{
    ToolSelectPointByPress();
    SetToolButton<OToolCurve>(checked, Tool::Curve, QStringLiteral("spline_cursor.png"),
                              tr("Select the first point of the curve"), &MainWindow::ClosedCurveDialog);
    LogPatternToolUsed(checked, QStringLiteral("Curve tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolCubicBezierPath(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogCubicBezierPath>(
        checked, Tool::CubicBezierPath, QStringLiteral("cubic_bezier_path_cursor.png"),
        tr("Select point of cubic bezier path"), &MainWindow::ClosedDrawDialogWithApply<VToolCubicBezierPath>,
        &MainWindow::ApplyDrawDialog<VToolCubicBezierPath>);
    LogPatternToolUsed(checked, QStringLiteral("Cubic bezier path tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolCutSplinePath handler tool CutSplinePath.
 * @param checked true - button is checked
 */
void MainWindow::ToolCutSplinePath(bool checked)
{
    ToolSelectSplinePath();
    SetToolButtonWithApply<DialogCutSplinePath>(
        checked, Tool::CutSplinePath, QStringLiteral("splinePath_cut_point_cursor.png"), tr("Select curve path"),
        &MainWindow::ClosedDrawDialogWithApply<VToolCutSplinePath>, &MainWindow::ApplyDrawDialog<VToolCutSplinePath>);
    LogPatternToolUsed(checked, QStringLiteral("Cut spline path tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolCutCurve handler tool CutCurve (unified: Spline + SplinePath).
 * @param checked true - button is checked
 */
void MainWindow::ToolCutCurve(bool checked)
{
    ToolSelectCutCurve();
    SetToolButton<DialogCutCurve>(checked, Tool::CutCurve, QStringLiteral("spline_cut_point_cursor.png"),
                                  tr("Select a curve"), &MainWindow::ClosedCutCurveDialog);
    LogPatternToolUsed(checked, QStringLiteral("Cut curve tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolPointOfContact handler tool pointOfContact.
 * @param checked true - button checked.
 */
void MainWindow::ToolPointOfContact(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogPointOfContact>(
        checked, Tool::PointOfContact, QStringLiteral("point_of_contact_cursor.png"), tr("Select first point of line"),
        &MainWindow::ClosedDrawDialogWithApply<VToolPointOfContact>, &MainWindow::ApplyDrawDialog<VToolPointOfContact>);
    LogPatternToolUsed(checked, QStringLiteral("Point of contact tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolDetail handler tool detail.
 * @param checked true - button checked.
 */
void MainWindow::ToolDetail(bool checked)
{
    ToolSelectAllDrawObjects();
    SetToolButton<OPieceDialog>(checked, Tool::Piece, QStringLiteral("new_detail_cursor.png"),
                                tr("Select main path objects clockwise."), &MainWindow::ClosedPieceDialog);
    LogPatternToolUsed(checked, QStringLiteral("Piece tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPiecePath(bool checked)
{
    ToolSelectAllDrawObjects();
    SetToolButton<DialogPiecePath>(
        checked, Tool::PiecePath, QStringLiteral("path_cursor.png"),
        tr("Select the path objects in order - the direction is free, no clockwise rule. A full curve is reversed "
           "automatically; hold <b>%1</b> to reverse a curve, <b>%2</b> to finish.")
            .arg(VModifierKey::Shift(), VModifierKey::EnterKey()),
        &MainWindow::ClosedDialogPiecePath);
    LogPatternToolUsed(checked, QStringLiteral("Piece path tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPin(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButton<DialogPin>(checked, Tool::Pin, QStringLiteral("pin_cursor.png"), tr("Select pin point"),
                             &MainWindow::ClosedDialogPin);
    LogPatternToolUsed(checked, QStringLiteral("Pin tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPlaceLabel(bool checked)
{
    ToolSelectPointByRelease();
    const QString tooltip = tr("Select all the points of the internal marks to place, <b>%1</b> - confirm selection. "
                               "Every mark will be attached to the piece you choose in the dialog, so select only "
                               "points lying on that piece.")
                                .arg(VModifierKey::EnterKey());
    SetToolButton<DialogPlaceLabel>(checked, Tool::PlaceLabel, QStringLiteral("place_label_cursor.png"), tooltip,
                                    &MainWindow::ClosedDialogPlaceLabel);
    LogPatternToolUsed(checked, QStringLiteral("Place label tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolHeight handler tool height.
 * @param checked true - button checked.
 */
void MainWindow::ToolHeight(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogHeight>(checked, Tool::Height, QStringLiteral("height_cursor.png"),
                                         tr("Select base point"), &MainWindow::ClosedDrawDialogWithApply<VToolHeight>,
                                         &MainWindow::ApplyDrawDialog<VToolHeight>);
    LogPatternToolUsed(checked, QStringLiteral("Height tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolTriangle handler tool triangle.
 * @param checked true - button checked.
 */
void MainWindow::ToolTriangle(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogTriangle>(
        checked, Tool::Triangle, QStringLiteral("triangle_cursor.png"), tr("Select first point of axis"),
        &MainWindow::ClosedDrawDialogWithApply<VToolTriangle>, &MainWindow::ApplyDrawDialog<VToolTriangle>);
    LogPatternToolUsed(checked, QStringLiteral("Triangle tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolPointOfIntersection handler tool pointOfIntersection.
 * @param checked true - button checked.
 */
void MainWindow::ToolPointOfIntersection(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogPointOfIntersection>(
        checked, Tool::PointOfIntersection, QStringLiteral("point_of_intersection_cursor.png"),
        tr("Select point for X value (vertical)"), &MainWindow::ClosedDrawDialogWithApply<VToolPointOfIntersection>,
        &MainWindow::ApplyDrawDialog<VToolPointOfIntersection>);
    LogPatternToolUsed(checked, QStringLiteral("Point of intersection tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolUnionDetails handler tool unionDetails.
 * @param checked true - button checked.
 */
void MainWindow::ToolUnionDetails(bool checked)
{
    ToolSelectDetail();
    SetToolButton<DialogUnionDetails>(checked, Tool::UnionDetails, QStringLiteral("union_cursor.png"),
                                      tr("Select detail"), &MainWindow::ClosedDialogUnionDetails);
    LogPatternToolUsed(checked, QStringLiteral("Union details tool"));
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedDialogUnionDetails actions after closing DialogUnionDetails.
 * @param result result of dialog working.
 */
void MainWindow::ClosedDialogUnionDetails(int result)
{
    ClosedDialog<VToolUnionDetails>(result); // Avoid error: Template function as signal or slot
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolDuplicateDetail(bool checked)
{
    ToolSelectDetail();
    SetToolButton<DialogDuplicateDetail>(checked, Tool::DuplicateDetail, QStringLiteral("duplicate_detail_cursor.png"),
                                         tr("Select detail"), &MainWindow::ClosedDialogDuplicateDetail);
    LogPatternToolUsed(checked, QStringLiteral("Duplicate detail tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogDuplicateDetail(int result)
{
    SCASSERT(not m_dialogTool.isNull())
    if (result == QDialog::Accepted)
    {
        auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
        SCASSERT(scene != nullptr)

        auto *tool = dynamic_cast<QGraphicsItem *>(VToolSeamAllowance::Duplicate(m_dialogTool, scene, doc));
        // Do not check for nullptr! See issue #719.
        emit ui->view->itemClicked(tool);
    }
    ArrowTool(true);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolGroup(bool checked)
{
    ToolSelectGroupObjects();
    const QString tooltip = tr("Select one or more objects, hold <b>%1</b> - for multiple selection, "
                               "<b>%2</b> - finish creation")
                                .arg(VModifierKey::Control(), VModifierKey::EnterKey());
    SetToolButton<DialogGroup>(checked, Tool::Group, QStringLiteral("group_plus_cursor.png"), tooltip,
                               &MainWindow::ClosedDialogGroup);
    LogPatternToolUsed(checked, QStringLiteral("Group tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolAttach(bool checked)
{
    ToolSelectGroupObjects();
    const QString tooltip = tr("Select one or more objects, hold <b>%1</b> - for multiple selection, "
                               "<b>%2</b> - finish creation")
                                .arg(VModifierKey::Control(), VModifierKey::EnterKey());
    SetToolButton<DialogAttach>(checked, Tool::Attach, QStringLiteral("group_plus_cursor.png"), tooltip,
                                &MainWindow::ClosedDialogAttach);
    LogPatternToolUsed(checked, QStringLiteral("Attach tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolRotation(bool checked)
{
    ToolSelectOperationObjects();
    const QString tooltip = tr("Select one or more objects, <b>%1</b> - confirm selection")
                                .arg(VModifierKey::EnterKey());
    SetToolButtonWithApply<DialogRotation>(checked, Tool::Rotation, QStringLiteral("rotation_cursor.png"), tooltip,
                                           &MainWindow::ClosedDrawDialogWithApply<VToolRotation>,
                                           &MainWindow::ApplyDrawDialog<VToolRotation>);
    LogPatternToolUsed(checked, QStringLiteral("Rotation tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolFlippingByLine(bool checked)
{
    ToolSelectOperationObjects();
    const QString tooltip = tr("Select one or more objects, <b>%1</b> - confirm selection")
                                .arg(VModifierKey::EnterKey());
    SetToolButtonWithApply<DialogFlippingByLine>(
        checked, Tool::FlippingByLine, QStringLiteral("flipping_line_cursor.png"), tooltip,
        &MainWindow::ClosedDrawDialogWithApply<VToolFlippingByLine>, &MainWindow::ApplyDrawDialog<VToolFlippingByLine>);
    LogPatternToolUsed(checked, QStringLiteral("Flipping by line tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolFlippingByAxis(bool checked)
{
    ToolSelectOperationObjects();
    const QString tooltip = tr("Select one or more objects, <b>%1</b> - confirm selection")
                                .arg(VModifierKey::EnterKey());
    SetToolButtonWithApply<DialogFlippingByAxis>(
        checked, Tool::FlippingByAxis, QStringLiteral("flipping_axis_cursor.png"), tooltip,
        &MainWindow::ClosedDrawDialogWithApply<VToolFlippingByAxis>, &MainWindow::ApplyDrawDialog<VToolFlippingByAxis>);
    LogPatternToolUsed(checked, QStringLiteral("Flipping by axis tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolMove(bool checked)
{
    ToolSelectOperationObjects();
    const QString tooltip = tr("Select one or more objects, <b>%1</b> - confirm selection")
                                .arg(VModifierKey::EnterKey());
    SetToolButtonWithApply<DialogMove>(checked, Tool::Move, QStringLiteral("move_cursor.png"), tooltip,
                                       &MainWindow::ClosedDrawDialogWithApply<VToolMove>,
                                       &MainWindow::ApplyDrawDialog<VToolMove>);
    LogPatternToolUsed(checked, QStringLiteral("Move tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogGroup(int result)
{
    SCASSERT(m_dialogTool != nullptr)
    if (result == QDialog::Accepted)
    {
        const QPointer<DialogGroup> dialog = qobject_cast<DialogGroup *>(m_dialogTool);
        SCASSERT(not dialog.isNull())

        if (const QDomElement group =
                doc->CreateGroup(pattern->getNextId(), dialog->GetName(), dialog->GetTags(), dialog->GetGroup());
                !group.isNull())
        {
            auto *addGroup = new AddGroup(group, doc);
            connect(addGroup, &AddGroup::UpdateGroups, m_dockManager->groupsWidget(), &VWidgetGroups::UpdateGroups);
            VAbstractApplication::VApp()->getUndoStack()->push(addGroup);
        }
    }
    ArrowTool(true);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogAttach(int result)
{
    SCASSERT(m_dialogTool != nullptr)
    if (result == QDialog::Accepted)
    {
        const QPointer<DialogAttach> dialog = qobject_cast<DialogAttach *>(m_dialogTool);
        SCASSERT(not dialog.isNull())

        if (const QDomElement attachment =
                doc->CreateAttachment(pattern->getNextId(), dialog->GetName(), dialog->GetGroup());
                !attachment.isNull())
        {
            auto *addAttachment = new AddAttachment(attachment, doc);
            VAbstractApplication::VApp()->getUndoStack()->push(addAttachment);
        }
    }
    ArrowTool(true);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogPiecePath(int result)
{
    SCASSERT(m_dialogTool != nullptr);
    if (result == QDialog::Accepted)
    {
        VToolPiecePath::Create(m_dialogTool, m_sceneDetails, doc, pattern);
    }
    ArrowTool(true);
    doc->LiteParseTree(Document::LiteParse);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogPlaceLabel(int result)
{
    SCASSERT(m_dialogTool != nullptr);
    if (result == QDialog::Accepted)
    {
        VToolPlaceLabel::CreateBatch(m_dialogTool, doc, pattern);
    }
    ArrowTool(true);
    doc->LiteParseTree(Document::LiteParse);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogPin(int result)
{
    SCASSERT(m_dialogTool != nullptr);
    if (result == QDialog::Accepted)
    {
        VToolPin::Create(m_dialogTool, doc, pattern);
    }
    ArrowTool(true);
    doc->LiteParseTree(Document::LiteParse);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClosedDialogInsertNode(int result)
{
    SCASSERT(m_dialogTool != nullptr);
    if (result == QDialog::Accepted)
    {
        const QPointer<DialogInsertNode> dTool = qobject_cast<DialogInsertNode *>(m_dialogTool);
        SCASSERT(not dTool.isNull())
        const bool placed = VToolSeamAllowance::InsertNodes(dTool->GetNodes(), dTool->GetPieceId(), m_sceneDetails,
                                                            pattern, doc);
        if (not placed)
        {
            QMessageBox::information(this, tr("Insert node"),
                                     tr("None of the selected nodes could be placed in the piece."));
        }
    }
    ArrowTool(true);
    doc->LiteParseTree(Document::LiteParse);
}



//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolCutArc handler tool cutArc.
 * @param checked true - button checked.
 */
void MainWindow::ToolCutArc(bool checked)
{
    ToolSelectArcCurve();
    SetToolButtonWithApply<DialogCutArc>(checked, Tool::CutArc, QStringLiteral("arc_cut_cursor.png"), tr("Select arc"),
                                         &MainWindow::ClosedDrawDialogWithApply<VToolCutArc>,
                                         &MainWindow::ApplyDrawDialog<VToolCutArc>);
    LogPatternToolUsed(checked, QStringLiteral("Cut arc tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolLineIntersectAxis(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogLineIntersectAxis>(
        checked, Tool::LineIntersectAxis, QStringLiteral("line_intersect_axis_cursor.png"),
        tr("Select first point of line"), &MainWindow::ClosedDrawDialogWithApply<VToolLineIntersectAxis>,
        &MainWindow::ApplyDrawDialog<VToolLineIntersectAxis>);
    LogPatternToolUsed(checked, QStringLiteral("Line intersect axis tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolCurveIntersectAxis(bool checked)
{
    ToolSelectAllDrawObjects();
    SetToolButtonWithApply<DialogCurveIntersectAxis>(
        checked, Tool::CurveIntersectAxis, QStringLiteral("curve_intersect_axis_cursor.png"), tr("Select curve"),
        &MainWindow::ClosedDrawDialogWithApply<VToolCurveIntersectAxis>,
        &MainWindow::ApplyDrawDialog<VToolCurveIntersectAxis>);
    LogPatternToolUsed(checked, QStringLiteral("Curve intersect axis tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolArcIntersectAxis(bool checked)
{
    ToolSelectAllDrawObjects();
    // Reuse ToolCurveIntersectAxis but with different cursor and tool tip
    SetToolButtonWithApply<DialogCurveIntersectAxis>(checked, Tool::ArcIntersectAxis,
                                                     QStringLiteral("arc_intersect_axis_cursor.png"), tr("Select arc"),
                                                     &MainWindow::ClosedDrawDialogWithApply<VToolCurveIntersectAxis>,
                                                     &MainWindow::ApplyDrawDialog<VToolCurveIntersectAxis>);
    LogPatternToolUsed(checked, QStringLiteral("Arc intersect axis tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPointOfIntersectionArcs(bool checked)
{
    ToolSelectArc();
    SetToolButtonWithApply<DialogPointOfIntersectionArcs>(
        checked, Tool::PointOfIntersectionArcs, QStringLiteral("point_of_intersection_arcs_cursor.png"),
        tr("Select first an arc"), &MainWindow::ClosedDrawDialogWithApply<VToolPointOfIntersectionArcs>,
        &MainWindow::ApplyDrawDialog<VToolPointOfIntersectionArcs>);
    LogPatternToolUsed(checked, QStringLiteral("Point of intersection arcs tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPointOfIntersectionCircles(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogPointOfIntersectionCircles>(
        checked, Tool::PointOfIntersectionCircles, QStringLiteral("point_of_intersection_circles_cursor.png"),
        tr("Select first circle center"), &MainWindow::ClosedDrawDialogWithApply<VToolPointOfIntersectionCircles>,
        &MainWindow::ApplyDrawDialog<VToolPointOfIntersectionCircles>);
    LogPatternToolUsed(checked, QStringLiteral("Point of intersection circles tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPointOfIntersectionCurves(bool checked)
{
    ToolSelectCurve();
    SetToolButtonWithApply<DialogPointOfIntersectionCurves>(
        checked, Tool::PointOfIntersectionCurves, QStringLiteral("intersection_curves_cursor.png"),
        tr("Select first curve"), &MainWindow::ClosedDrawDialogWithApply<VToolPointOfIntersectionCurves>,
        &MainWindow::ApplyDrawDialog<VToolPointOfIntersectionCurves>);
    LogPatternToolUsed(checked, QStringLiteral("Point of intersection curves tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPointFromCircleAndTangent(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogPointFromCircleAndTangent>(
        checked, Tool::PointFromCircleAndTangent, QStringLiteral("point_from_circle_and_tangent_cursor.png"),
        tr("Select point on tangent"), &MainWindow::ClosedDrawDialogWithApply<VToolPointFromCircleAndTangent>,
        &MainWindow::ApplyDrawDialog<VToolPointFromCircleAndTangent>);
    LogPatternToolUsed(checked, QStringLiteral("Point from circle and tangent tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolPointFromArcAndTangent(bool checked)
{
    ToolSelectPointArc();
    SetToolButtonWithApply<DialogPointFromArcAndTangent>(
        checked, Tool::PointFromArcAndTangent, QStringLiteral("point_from_arc_and_tangent_cursor.png"),
        tr("Select point on tangent"), &MainWindow::ClosedDrawDialogWithApply<VToolPointFromArcAndTangent>,
        &MainWindow::ApplyDrawDialog<VToolPointFromArcAndTangent>);
    LogPatternToolUsed(checked, QStringLiteral("Point from arc and tangent tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolArcWithLength(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogArcWithLength>(
        checked, Tool::ArcWithLength, QStringLiteral("arc_with_length_cursor.png"),
        tr("Select point of the center of the arc"), &MainWindow::ClosedDrawDialogWithApply<VToolArcWithLength>,
        &MainWindow::ApplyDrawDialog<VToolArcWithLength>);
    LogPatternToolUsed(checked, QStringLiteral("Arc with length tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolTrueDarts(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogTrueDarts>(
        checked, Tool::TrueDarts, QStringLiteral("true_darts_cursor.png"), tr("Select the first base line point"),
        &MainWindow::ClosedDrawDialogWithApply<VToolTrueDarts>, &MainWindow::ApplyDrawDialog<VToolTrueDarts>);
    LogPatternToolUsed(checked, QStringLiteral("True darts tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolInsertNode(bool checked)
{
    ToolSelectOperationObjects();
    const QString tooltip = tr("Select one or more objects, <b>%1</b> - confirm selection")
                                .arg(VModifierKey::EnterKey());
    SetToolButton<DialogInsertNode>(checked, Tool::InsertNode, QStringLiteral("insert_node_cursor.png"), tooltip,
                                    &MainWindow::ClosedDialogInsertNode);
    LogPatternToolUsed(checked, QStringLiteral("Insert node tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolArcStart(bool checked)
{
    ToolSelectArcElArc();
    // Reuse DialogCutArc and VToolCutArc but with different cursor
    SetToolButtonWithApply<DialogCutArc>(checked, Tool::ArcStart, QStringLiteral("arc_start_cursor.png"),
                                         tr("Select arc"), &MainWindow::ClosedDrawDialogWithApply<VToolCutArc>,
                                         &MainWindow::ApplyDrawDialog<VToolCutArc>);
    LogPatternToolUsed(checked, QStringLiteral("Arc start tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolArcEnd(bool checked)
{
    ToolSelectArcElArc();
    // Reuse DialogCutArc and VToolCutArc but with different cursor
    SetToolButtonWithApply<DialogCutArc>(checked, Tool::ArcEnd, QStringLiteral("arc_end_cursor.png"), tr("Select arc"),
                                         &MainWindow::ClosedDrawDialogWithApply<VToolCutArc>,
                                         &MainWindow::ApplyDrawDialog<VToolCutArc>);
    LogPatternToolUsed(checked, QStringLiteral("Arc end tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolEllipticalArcWithLength(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButtonWithApply<DialogEllipticalArcWithLength>(
        checked, Tool::EllipticalArcWithLength, QStringLiteral("el_arc_with_length_cursor.png"),
        tr("Select point of the center of the elliptical arc"),
                &MainWindow::ClosedDrawDialogWithApply<VToolEllipticalArcWithLength>,
        &MainWindow::ApplyDrawDialog<VToolEllipticalArcWithLength>);
    LogPatternToolUsed(checked, QStringLiteral("Elliptical arc with length tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolParallelCurve(bool checked)
{
    ToolSelectCurve();
    SetToolButtonWithApply<DialogParallelCurve>(
        checked, Tool::ParallelCurve, QStringLiteral("parallel_curve_cursor.png"),
        tr("Select curve"), &MainWindow::ClosedDrawDialogWithApply<VToolParallelCurve>,
        &MainWindow::ApplyDrawDialog<VToolParallelCurve>);
    LogPatternToolUsed(checked, QStringLiteral("Parallel curve tool"));
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolGraduatedCurve(bool checked)
{
    ToolSelectCurve();
    SetToolButtonWithApply<DialogGraduatedCurve>(
        checked, Tool::GraduatedCurve, QStringLiteral("graduated_curve_offseting_cursor.png"),
        tr("Select curve"), &MainWindow::ClosedDrawDialogWithApply<VToolGraduatedCurve>,
        &MainWindow::ApplyDrawDialog<VToolGraduatedCurve>);
    LogPatternToolUsed(checked, QStringLiteral("Graduated curve tool"));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolArmholeCurve(bool checked)
{
    ToolSelectPointByRelease();
    SetToolButton<OToolArmholeCurveCollector>(
        checked, Tool::ArmholeCurve, QStringLiteral("cubic_bezier_cursor.png"),
        tr("Select the armhole point. Optional: create a perpendicular point beforehand to end the curve "
           "perpendicular to the shoulder line."),
        &MainWindow::ClosedArmholeCurveDialog);
    LogPatternToolUsed(checked, QStringLiteral("Armhole curve tool"));
}


template <typename Dialog, typename Func>
void MainWindow::SetToolButton(bool checked, Tool t, const QString &cursor, const QString &toolTip,
                               Func closeDialogSlot)
{
    if (!checked)
    {
        if (auto *tButton = qobject_cast<QAction *>(this->sender()))
        {
            tButton->setChecked(true);
        }
        return;
    }

    CancelTool();
    doc->ClearIsolation();
    if (m_breadcrumb != nullptr)
    {
        m_breadcrumb->hide();
    }
    emit EnableItemMove(false);
    m_currentTool = m_lastUsedTool = t;

    if (const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
        settings->GetPointerMode() == VToolPointerMode::ToolIcon)
    {
        const auto resource = QStringLiteral("toolcursor");
        QPixmap const pixmap(ResolveToolCursorResource(resource, cursor));
        QCursor const cur(pixmap, 2, 2);
        ui->view->viewport()->setCursor(cur);
        ui->view->setCurrentCursorShape(); // Hack to fix problem with a cursor
    }

    m_statusLabel->setText(toolTip);
    ui->view->setShowToolOptions(false);
    auto *dialogTool = new Dialog(pattern, doc, 0, this);

    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 70, "Check if need to extend.");

    switch (t)
    {
        case Tool::ArcStart:
        case Tool::ArcEnd:
        case Tool::Midpoint:
            dialogTool->Build(t);
            break;
        case Tool::PiecePath:
        case Tool::Pin:
        case Tool::InsertNode:
        case Tool::PlaceLabel:
            dialogTool->SetPiecesList(doc->GetActivePPPieces());
            break;
        case Tool::Rotation:
        case Tool::Move:
        case Tool::FlippingByAxis:
        case Tool::FlippingByLine:
        case Tool::Group:
            dialogTool->SetGroupCategories(doc->GetGroupCategories());
            break;
        case Tool::Piece:
            dialogTool->SetPatternDoc(doc);
            break;
        default:
            break;
    }

    VValentinaSettings  const*settings = VApplication::VApp()->ValentinaSettings();

    if constexpr (std::is_same_v<Dialog,DialogEndLine> || std::is_same_v<Dialog,DialogAlongLine> ||
                  std::is_same_v<Dialog,DialogBisector> || std::is_same_v<Dialog,DialogCurveIntersectAxis> ||
                  std::is_same_v<Dialog,DialogHeight> || std::is_same_v<Dialog,DialogLine> ||
                  std::is_same_v<Dialog,DialogLineIntersectAxis> || std::is_same_v<Dialog,DialogNormal> ||
                  std::is_same_v<Dialog,DialogShoulderPoint>)
    {
        dialogTool->SetTypeLine(settings->GetGlobalPenStyle());
        dialogTool->SetLineColor(settings->GetGlobalToolColor());
    }

    if constexpr (std::is_same_v<Dialog,DialogArc> || std::is_same_v<Dialog,DialogArcWithLength> ||
                  std::is_same_v<Dialog,DialogEllipticalArc> ||
                  std::is_same_v<Dialog,DialogEllipticalArcWithLength> ||
                  std::is_same_v<Dialog,DialogParallelCurve> || std::is_same_v<Dialog,DialogGraduatedCurve>)
    {
        dialogTool->SetPenStyle(settings->GetGlobalPenStyle());
        dialogTool->SetColor(settings->GetGlobalToolColor());
    }

    if constexpr (std::is_same_v<Dialog,OToolArmholeCurveCollector> || std::is_same_v<Dialog,DialogCubicBezier> || std::is_same_v<Dialog,DialogCubicBezierPath> ||
                  std::is_same_v<Dialog,DialogSpline> || std::is_same_v<Dialog,DialogSplinePath>)
    {
        dialogTool->SetDefPenStyle(settings->GetGlobalPenStyle());
        dialogTool->SetDefColor(settings->GetGlobalToolColor());
    }

    auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
    SCASSERT(scene != nullptr)

    m_dialogTool = dialogTool;

    // A full reparse (e.g. deleting a tool from the dependencies dock) would clear the scene and destroy this
    // dialog's visualization. Block tool deletion until the creation is finished.
    m_dockManager->dependenciesWidget()->SetToolCreationActive(true);

    // A full reparse while this creation dialog is open destroys its visualization (vis becomes null).
    // Drop the scene event in that state — ChosenObject overrides dereference vis and would crash.
    connect(scene, &VMainGraphicsScene::ChoosedObject, dialogTool,
            [dialogTool](quint32 id, SceneObject type)
            {
                if (dialogTool->VisualizationBroken())
                {
                    return;
                }
                dialogTool->ChosenObject(id, type);
            });
    if (t == Tool::Group || t == Tool::Attach)
    {
        connect(scene, &VMainGraphicsScene::SelectedObject, m_dialogTool.data(), &DialogTool::SelectedObject);
    }
    connect(m_dialogTool.data(), &DialogTool::DialogClosed, this, closeDialogSlot);
    connect(m_dialogTool.data(), &DialogTool::ToolTip, this, &MainWindow::ShowToolTip);
    connect(m_dialogTool.data(), &DialogTool::destroyed, this, [this] () -> void { ShowToolTip(QString()); });
    emit ui->view->itemClicked(nullptr);
}





//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetToolButtonWithApply set tool and show dialog.
 * @param checked true if tool button checked.
 * @param t tool type.
 * @param cursor path tool cursor icon.
 * @param toolTip first tooltipe.
 * @param closeDialogSlot function to handle close of dialog.
 * @param applyDialogSlot function to handle apply in dialog.
 */
template <typename Dialog, typename Func, typename Func2>
void MainWindow::SetToolButtonWithApply(bool checked, Tool t, const QString &cursor, const QString &toolTip,
                                        Func closeDialogSlot, Func2 applyDialogSlot)
{
    if (checked)
    {
        SetToolButton<Dialog>(checked, t, cursor, toolTip, closeDialogSlot);

        connect(m_dialogTool.data(), &DialogTool::DialogApplied, this, applyDialogSlot);
    }
    else
    {
        if (auto *tButton = qobject_cast<QAction *>(this->sender()))
        {
            tButton->setChecked(true);
        }
    }
}



//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ApplyDialog handle apply in dialog
 */
template <typename DrawTool> void MainWindow::ApplyDialog(VMainGraphicsScene *scene)
{
    SCASSERT(not m_dialogTool.isNull())

    // Only create tool if not already created with apply
    if (m_dialogTool->GetAssociatedTool() == nullptr)
    {
        SCASSERT(scene != nullptr)

        m_dialogTool->SetAssociatedTool(DrawTool::Create(m_dialogTool, scene, doc, pattern));
    }
    else
    { // Or update associated tool with data
        auto *vtool = qobject_cast<DrawTool *>(m_dialogTool->GetAssociatedTool());
        SCASSERT(vtool != nullptr)
        vtool->FullUpdateFromGuiApply();
    }
}


//---------------------------------------------------------------------------------------------------------------------
template <typename DrawTool> void MainWindow::ClosedDrawDialogWithApply(int result)
{
    ClosedDialogWithApply<DrawTool>(result, m_sceneDraw);
}


//---------------------------------------------------------------------------------------------------------------------
template <typename DrawTool> void MainWindow::ApplyDrawDialog()
{
    ApplyDialog<DrawTool>(m_sceneDraw);
}


//---------------------------------------------------------------------------------------------------------------------
template <typename DrawTool> void MainWindow::ClosedDetailsDialogWithApply(int result)
{
    ClosedDialogWithApply<DrawTool>(result, m_sceneDetails);
}


//---------------------------------------------------------------------------------------------------------------------
template <typename DrawTool> void MainWindow::ApplyDetailsDialog()
{
    ApplyDialog<DrawTool>(m_sceneDetails);
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedCutCurveDialog routes the unified "Segment a curve" click to the existing
 *        ToolCutSpline / ToolCutSplinePath and forwards the click.
 * @param result result working dialog.
 */
void MainWindow::ClosedCutCurveDialog(int result)
{
    SCASSERT(not m_dialogTool.isNull())
    if (result == QDialog::Accepted)
    {
        auto *curveDialog = qobject_cast<DialogCutCurve *>(m_dialogTool.data());
        SCASSERT(curveDialog != nullptr)

        const quint32 curveId = curveDialog->getCurveId();
        const bool isPath = curveDialog->IsSplinePath();

        CancelTool();

        if (isPath)
        {
            ToolCutSplinePath(true);
        }
        else
        {
            ToolCutSpline(true);
        }

        m_dialogTool->ChosenObject(curveId, isPath ? SceneObject::SplinePath : SceneObject::Spline);
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedCurveDialog routes the unified "Curve" creation to a Spline (2 points) or a SplinePath (3+ points).
 * @param result result working dialog.
 */
void MainWindow::ClosedCurveDialog(int result)
{
    SCASSERT(not m_dialogTool.isNull())
    QGraphicsItem *tool = nullptr;
    if (result == QDialog::Accepted)
    {
        auto *curveDialog = qobject_cast<OToolCurve *>(m_dialogTool.data());
        SCASSERT(curveDialog != nullptr)

        const QVector<quint32> pointIds = curveDialog->GetPointIds();
        auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
        SCASSERT(scene != nullptr)

        if (pointIds.size() == 2)
        {
            VToolSplineInitData initData;
            initData.scene = scene;
            initData.doc = doc;
            initData.data = pattern;
            initData.parse = Document::FullParse;
            initData.typeCreation = Source::FromGui;
            initData.point1 = pointIds.at(0);
            initData.point4 = pointIds.at(1);

            const auto path = curveDialog->GetPath();
            auto *spl = new VSpline(path.GetSpline(1));

            tool = VToolSpline::Create(initData, spl);
        }
        else
        {
            VToolSplinePathInitData initData;
            initData.scene = scene;
            initData.doc = doc;
            initData.data = pattern;
            initData.parse = Document::FullParse;
            initData.typeCreation = Source::FromGui;
            initData.points = pointIds;

            auto *path = new VSplinePath(curveDialog->GetPath());

            tool = VToolSplinePath::Create(initData, path);
        }
    }
    ArrowTool(true);
    if (tool != nullptr)
    {
        emit ui->view->itemClicked(tool);
        tool->setFocus();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedArmholeCurveDialog creates the armhole curve collected by OToolArmholeCurveCollector.
 * @param result result working dialog.
 */
void MainWindow::ClosedArmholeCurveDialog(int result)
{
    SCASSERT(not m_dialogTool.isNull())
    QGraphicsItem *tool = nullptr;
    if (result == QDialog::Accepted)
    {
        auto *collector = qobject_cast<OToolArmholeCurveCollector *>(m_dialogTool.data());
        SCASSERT(collector != nullptr)

        auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
        SCASSERT(scene != nullptr)

        OToolArmholeCurveInitData initData;
        initData.scene = scene;
        initData.doc = doc;
        initData.data = pattern;
        initData.parse = Document::FullParse;
        initData.typeCreation = Source::FromGui;
        initData.path = new VCubicBezierPath(collector->GetPath());

        tool = OToolArmholeCurve::Create(initData);
    }
    ArrowTool(true);
    if (tool != nullptr)
    {
        emit ui->view->itemClicked(tool);
        tool->setFocus();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ClosedPieceDialog closes the two-step piece creation dialog and creates the piece.
 * @param result dialog result.
 */
void MainWindow::ClosedPieceDialog(int result)
{
    SCASSERT(not m_dialogTool.isNull())
    if (result == QDialog::Accepted)
    {
        // Pieces are final details: create them in the Details scene, like the legacy
        // ClosedDetailsDialogWithApply did.
        ApplyPieceDialog(m_sceneDetails);
    }
    // If before Cancel was used Apply we have an item.
    auto *vtool = qobject_cast<VToolSeamAllowance *>(m_dialogTool->GetAssociatedTool()); // Don't check for nullptr here
    if (m_dialogTool->GetAssociatedTool() != nullptr)
    {
        SCASSERT(vtool != nullptr)
        vtool->DialogLinkDestroy();
        connect(vtool, &VToolSeamAllowance::ToolTip, this, &MainWindow::ShowToolTip);
    }
    ArrowTool(true);
    ui->view->itemClicked(vtool); // Don't check for nullptr here
    if (vtool)
    {
        vtool->setFocus();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ApplyPieceDialog creates the piece from the two-step dialog (OPieceDialog is
 * an app class, so VToolSeamAllowance::Create(dialog,…) cannot see it — the app
 * builds VToolSeamAllowanceInitData itself and calls Create(initData)).
 * @param scene draw scene.
 */
void MainWindow::ApplyPieceDialog(VMainGraphicsScene *scene)
{
    SCASSERT(not m_dialogTool.isNull())

    if (m_dialogTool->GetAssociatedTool() == nullptr)
    {
        SCASSERT(scene != nullptr)
        auto *pieceDialog = qobject_cast<OPieceDialog *>(m_dialogTool.data());
        SCASSERT(pieceDialog != nullptr)

        VToolSeamAllowanceInitData initData;
        initData.detail = pieceDialog->GetPiece();
        initData.width = initData.detail.GetFormulaSAWidth();
        initData.scene = scene;
        initData.doc = doc;
        initData.data = pattern;
        initData.parse = Document::FullParse;
        initData.typeCreation = Source::FromGui;

        initData.detail.GetPath().SetNodes(VAbstractTool::PrepareNodes(initData.detail.GetPath(), scene, doc, pattern));

        VToolSeamAllowance *piece = VToolSeamAllowance::Create(initData);
        if (piece != nullptr)
        {
            m_dialogTool->SetAssociatedTool(piece);
            piece->RefreshGeometry(true); // Refresh internal paths
        }
    }
    else
    {
        auto *vtool = qobject_cast<VToolSeamAllowance *>(m_dialogTool->GetAssociatedTool());
        SCASSERT(vtool != nullptr)
        vtool->FullUpdateFromGuiApply();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CancelTool cancel tool.
 */
void MainWindow::CancelTool()
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 70, "Not all tools were handled.");

    qCDebug(vMainWindow, "Canceling tool.");
    if (not m_dialogTool.isNull())
    {
        m_dialogTool->hide();
        m_dialogTool->deleteLater();
    }
    m_dockManager->dependenciesWidget()->SetToolCreationActive(false); // Creation finished/cancelled: allow deletion again.
    qCDebug(vMainWindow, "Dialog closed.");

    currentScene->setFocus(Qt::OtherFocusReason);
    currentScene->clearSelection();
    emit ui->view->itemClicked(nullptr); // Hide visualization to avoid a crash

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (m_currentTool)
    {
        case Tool::Arrow:
            ui->actionToolSelect->setChecked(false);
            m_statusLabel->setText(QString());
            m_comboBoxDraws->setEnabled(false);

            // Crash: using CRTL+Z while using line tool.
            // related bug report:
            // https://bitbucket.org/dismine/valentina/issues/454/crash-using-crtl-z-while-using-line-tool
            undoAction->setEnabled(false);
            redoAction->setEnabled(false);
            VAbstractTool::m_suppressContextMenu = true;
            return;
        case Tool::BasePoint:
        case Tool::SinglePoint:
        case Tool::DoublePoint:
        case Tool::LinePoint:
        case Tool::AbstractSpline:
        case Tool::Cut:
        case Tool::ArcPoint:
        case Tool::CurvePoint:
        case Tool::LAST_ONE_DO_NOT_USE:
        case Tool::NodePoint:
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
        case Tool::BackgroundImage:
        case Tool::BackgroundImageControls:
        case Tool::BackgroundPixmapImage:
        case Tool::BackgroundSVGImage:
            Q_UNREACHABLE(); //-V501
            // Nothing to do here because we can't create this tool from main window.
            break;
        case Tool::EndLine:
            ui->actionEndLineTool->setChecked(false);
            break;
        case Tool::Line:
            ui->actionLineTool->setChecked(false);
            break;
        case Tool::AlongLine:
            ui->actionAlongLineTool->setChecked(false);
            break;
        case Tool::Midpoint:
            ui->actionMidpointTool->setChecked(false);
            break;
        case Tool::ShoulderPoint:
            ui->actionShoulderPointTool->setChecked(false);
            break;
        case Tool::Normal:
            ui->actionNormalTool->setChecked(false);
            break;
        case Tool::Bisector:
            ui->actionBisectorTool->setChecked(false);
            break;
        case Tool::LineIntersect:
            ui->actionLineIntersectTool->setChecked(false);
            break;
        case Tool::Curve:
            ui->actionCurveTool->setChecked(false);
            break;
        case Tool::Spline:
            ui->actionSplineTool->setChecked(false);
            break;
        case Tool::CubicBezier:
            ui->actionCubicBezierTool->setChecked(false);
            break;
        case Tool::ArmholeCurve:
            ui->actionArmholeCurveTool->setChecked(false);
            break;
        case Tool::Arc:
            ui->actionArcTool->setChecked(false);
            break;
        case Tool::ArcWithLength:
            ui->actionArcWithLengthTool->setChecked(false);
            break;
        case Tool::SplinePath:
            ui->actionSplinePathTool->setChecked(false);
            break;
        case Tool::CubicBezierPath:
            ui->actionCubicBezierPathTool->setChecked(false);
            break;
        case Tool::PointOfContact:
            ui->actionPointOfContactTool->setChecked(false);
            break;
        case Tool::Piece:
            ui->actionNewDetailTool->setChecked(false);
            break;
        case Tool::PiecePath:
            ui->actionInternalPathTool->setChecked(false);
            break;
        case Tool::Height:
            ui->actionHeightTool->setChecked(false);
            break;
        case Tool::Triangle:
            ui->actionTriangleTool->setChecked(false);
            break;
        case Tool::PointOfIntersection:
            ui->actionPointOfIntersectionTool->setChecked(false);
            break;
        case Tool::CutCurve:
            ui->actionCutCurveTool->setChecked(false);
            break;
        case Tool::CutSpline:
            ui->actionSplineCutPointTool->setChecked(false);
            break;
        case Tool::CutSplinePath:
            ui->actionSplinePathCutPointTool->setChecked(false);
            break;
        case Tool::UnionDetails:
            ui->actionUnionDetailsTool->setChecked(false);
            break;
        case Tool::DuplicateDetail:
            ui->actionDuplicateDetailTool->setChecked(false);
            break;
        case Tool::CutArc:
            ui->actionArcCutPointTool->setChecked(false);
            break;
        case Tool::LineIntersectAxis:
            ui->actionLineIntersectAxisTool->setChecked(false);
            break;
        case Tool::CurveIntersectAxis:
            ui->actionCurveIntersectAxisTool->setChecked(false);
            break;
        case Tool::ArcIntersectAxis:
            ui->actionArcIntersectAxisTool->setChecked(false);
            break;
        case Tool::PointOfIntersectionArcs:
            ui->actionPointOfIntersectionArcsTool->setChecked(false);
            break;
        case Tool::PointOfIntersectionCircles:
            ui->actionPointOfIntersectionCirclesTool->setChecked(false);
            break;
        case Tool::PointOfIntersectionCurves:
            ui->actionIntersectionCurvesTool->setChecked(false);
            break;
        case Tool::PointFromCircleAndTangent:
            ui->actionPointFromCircleAndTangentTool->setChecked(false);
            break;
        case Tool::PointFromArcAndTangent:
            ui->actionPointFromArcAndTangentTool->setChecked(false);
            break;
        case Tool::TrueDarts:
            ui->actionTrueDartsTool->setChecked(false);
            break;
        case Tool::Group:
            ui->actionGroupTool->setChecked(false);
            break;
        case Tool::Attach:
            ui->actionAttachTool->setChecked(false);
            break;
        case Tool::Rotation:
            ui->actionRotationTool->setChecked(false);
            break;
        case Tool::FlippingByLine:
            ui->actionFlippingByLineTool->setChecked(false);
            break;
        case Tool::FlippingByAxis:
            ui->actionFlippingByAxisTool->setChecked(false);
            break;
        case Tool::Move:
            ui->actionMoveTool->setChecked(false);
            break;
        case Tool::EllipticalArc:
            ui->actionEllipticalArcTool->setChecked(false);
            break;
        case Tool::Pin:
            ui->actionPinTool->setChecked(false);
            break;
        case Tool::InsertNode:
            ui->actionInsertNodeTool->setChecked(false);
            break;
        case Tool::PlaceLabel:
            ui->actionPlaceLabelTool->setChecked(false);
            break;
        case Tool::ArcStart:
            ui->actionArcStartPointTool->setChecked(false);
            break;
        case Tool::ArcEnd:
            ui->actionArcEndPointTool->setChecked(false);
            break;
        case Tool::EllipticalArcWithLength:
            ui->actionEllipticalArcWithLengthTool->setChecked(false);
            break;
        case Tool::ParallelCurve:
            ui->actionParallelCurveTool->setChecked(false);
            break;
        case Tool::GraduatedCurve:
            ui->actionGraduatedCurveTool->setChecked(false);
            break;
    }

    QT_WARNING_POP

    // Crash: using CRTL+Z while using line tool.
    // related bug report:
    // https://bitbucket.org/dismine/valentina/issues/454/crash-using-crtl-z-while-using-line-tool
    undoAction->setEnabled(not m_toolOptionsDialogVisible &&
                           VAbstractApplication::VApp()->getUndoStack()->canUndo());
    redoAction->setEnabled(not m_toolOptionsDialogVisible &&
                           VAbstractApplication::VApp()->getUndoStack()->canRedo());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArrowTool enable arrow tool.
 */
void MainWindow::ArrowTool(bool checked)
{
    if (checked && m_currentTool != Tool::Arrow)
    {
        qCDebug(vMainWindow, "Arrow tool.");
        CancelTool();
        ui->actionToolSelect->setChecked(true);
        m_comboBoxDraws->setEnabled(true);
        m_currentTool = Tool::Arrow;
        emit EnableItemMove(true);
        emit ItemsSelection(SelectionType::ByMouseRelease);
        VAbstractTool::m_suppressContextMenu = false;

        // Only true for rubber band selection
        emit EnablePointSelection(false);
        emit EnableLineSelection(false);
        emit EnableArcSelection(false);
        emit EnableElArcSelection(false);
        emit EnableSplineSelection(false);
        emit EnableSplinePathSelection(false);
        emit EnableNodePointSelection(true);
        emit EnableDetailSelection(true); // Disable when done visualization details
        emit EnableBackgroundImageSelection(true);

        // Hovering
        emit EnablePointHover(true);
        emit EnableLineHover(true);
        emit EnableArcHover(true);
        emit EnableElArcHover(true);
        emit EnableSplineHover(true);
        emit EnableSplinePathHover(true);
        emit EnableNodePointHover(true);
        emit EnableDetailHover(true);
        emit EnableImageBackgroundHover(true);

        ui->view->AllowRubberBand(true);
        ui->view->viewport()->unsetCursor();
        ui->view->viewport()->setCursor(QCursor(Qt::ArrowCursor));
        ui->view->setCurrentCursorShape(); // Hack to fix problem with a cursor
        m_statusLabel->setText(QString());
        ui->view->setShowToolOptions(true);
        qCDebug(vMainWindow, "Enabled arrow tool.");
    }
    else
    {
        ui->actionToolSelect->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::LastUsedTool()
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 70, "Not all tools were handled.");

    if (m_currentTool == m_lastUsedTool)
    {
        return;
    }

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (m_lastUsedTool)
    {
        case Tool::Arrow:
            ui->actionToolSelect->setChecked(true);
            ArrowTool(true);
            break;
        case Tool::BasePoint:
        case Tool::SinglePoint:
        case Tool::DoublePoint:
        case Tool::LinePoint:
        case Tool::AbstractSpline:
        case Tool::Cut:
        case Tool::ArcPoint:
        case Tool::CurvePoint:
        case Tool::LAST_ONE_DO_NOT_USE:
        case Tool::NodePoint:
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
        case Tool::BackgroundImage:
        case Tool::BackgroundImageControls:
        case Tool::BackgroundPixmapImage:
        case Tool::BackgroundSVGImage:
            Q_UNREACHABLE(); //-V501
            // Nothing to do here because we can't create this tool from main window.
            break;
        case Tool::EndLine:
            ui->actionEndLineTool->setChecked(true);
            ToolEndLine(true);
            break;
        case Tool::Line:
            ui->actionLineTool->setChecked(true);
            ToolLine(true);
            break;
        case Tool::AlongLine:
            ui->actionAlongLineTool->setChecked(true);
            ToolAlongLine(true);
            break;
        case Tool::Midpoint:
            ui->actionMidpointTool->setChecked(true);
            ToolMidpoint(true);
            break;
        case Tool::ShoulderPoint:
            ui->actionShoulderPointTool->setChecked(true);
            ToolShoulderPoint(true);
            break;
        case Tool::Normal:
            ui->actionNormalTool->setChecked(true);
            ToolNormal(true);
            break;
        case Tool::Bisector:
            ui->actionBisectorTool->setChecked(true);
            ToolBisector(true);
            break;
        case Tool::LineIntersect:
            ui->actionLineIntersectTool->setChecked(true);
            ToolLineIntersect(true);
            break;
        case Tool::Curve:
            ui->actionCurveTool->setChecked(true);
            ToolCurve(true);
            break;
        case Tool::Spline:
            ui->actionSplineTool->setChecked(true);
            ToolSpline(true);
            break;
        case Tool::CubicBezier:
            ui->actionCubicBezierTool->setChecked(true);
            ToolCubicBezier(true);
            break;
        case Tool::ArmholeCurve:
            ui->actionArmholeCurveTool->setChecked(true);
            ToolArmholeCurve(true);
            break;
        case Tool::Arc:
            ui->actionArcTool->setChecked(true);
            ToolArc(true);
            break;
        case Tool::SplinePath:
            ui->actionSplinePathTool->setChecked(true);
            ToolSplinePath(true);
            break;
        case Tool::CubicBezierPath:
            ui->actionCubicBezierPathTool->setChecked(true);
            ToolCubicBezierPath(true);
            break;
        case Tool::PointOfContact:
            ui->actionPointOfContactTool->setChecked(true);
            ToolPointOfContact(true);
            break;
        case Tool::Piece:
            ui->actionNewDetailTool->setChecked(true);
            ToolDetail(true);
            break;
        case Tool::PiecePath:
            ui->actionInternalPathTool->setChecked(true);
            ToolPiecePath(true);
            break;
        case Tool::Height:
            ui->actionHeightTool->setChecked(true);
            ToolHeight(true);
            break;
        case Tool::Triangle:
            ui->actionTriangleTool->setChecked(true);
            ToolTriangle(true);
            break;
        case Tool::PointOfIntersection:
            ui->actionPointOfIntersectionTool->setChecked(true);
            ToolPointOfIntersection(true);
            break;
        case Tool::PointOfIntersectionArcs:
            ui->actionPointOfIntersectionArcsTool->setChecked(true);
            ToolPointOfIntersectionArcs(true);
            break;
        case Tool::CutCurve:
            ui->actionCutCurveTool->setChecked(true);
            ToolCutCurve(true);
            break;
        case Tool::CutSpline:
            ui->actionSplineCutPointTool->setChecked(true);
            ToolCutSpline(true);
            break;
        case Tool::CutSplinePath:
            ui->actionSplinePathCutPointTool->setChecked(true);
            ToolCutSplinePath(true);
            break;
        case Tool::UnionDetails:
            ui->actionUnionDetailsTool->setChecked(true);
            ToolUnionDetails(true);
            break;
        case Tool::DuplicateDetail:
            ui->actionDuplicateDetailTool->setChecked(true);
            ToolDuplicateDetail(true);
            break;
        case Tool::CutArc:
            ui->actionArcCutPointTool->setChecked(true);
            ToolCutArc(true);
            break;
        case Tool::LineIntersectAxis:
            ui->actionLineIntersectAxisTool->setChecked(true);
            ToolLineIntersectAxis(true);
            break;
        case Tool::CurveIntersectAxis:
            ui->actionCurveIntersectAxisTool->setChecked(true);
            ToolCurveIntersectAxis(true);
            break;
        case Tool::ArcIntersectAxis:
            ui->actionArcIntersectAxisTool->setChecked(true);
            ToolArcIntersectAxis(true);
            break;
        case Tool::PointOfIntersectionCircles:
            ui->actionPointOfIntersectionCirclesTool->setChecked(true);
            ToolPointOfIntersectionCircles(true);
            break;
        case Tool::PointOfIntersectionCurves:
            ui->actionIntersectionCurvesTool->setChecked(true);
            ToolPointOfIntersectionCurves(true);
            break;
        case Tool::PointFromCircleAndTangent:
            ui->actionPointFromCircleAndTangentTool->setChecked(true);
            ToolPointFromCircleAndTangent(true);
            break;
        case Tool::PointFromArcAndTangent:
            ui->actionPointFromArcAndTangentTool->setChecked(true);
            ToolPointFromArcAndTangent(true);
            break;
        case Tool::ArcWithLength:
            ui->actionArcWithLengthTool->setChecked(true);
            ToolArcWithLength(true);
            break;
        case Tool::TrueDarts:
            ui->actionTrueDartsTool->setChecked(true);
            ToolTrueDarts(true);
            break;
        case Tool::Group:
            ui->actionGroupTool->setChecked(true);
            ToolGroup(true);
            break;
        case Tool::Attach:
            ui->actionAttachTool->setChecked(true);
            ToolAttach(true);
            break;
        case Tool::Rotation:
            ui->actionRotationTool->setChecked(true);
            ToolRotation(true);
            break;
        case Tool::FlippingByLine:
            ui->actionFlippingByLineTool->setChecked(true);
            ToolFlippingByLine(true);
            break;
        case Tool::FlippingByAxis:
            ui->actionFlippingByAxisTool->setChecked(true);
            ToolFlippingByAxis(true);
            break;
        case Tool::Move:
            ui->actionMoveTool->setChecked(true);
            ToolMove(true);
            break;
        case Tool::EllipticalArc:
            ui->actionEllipticalArcTool->setChecked(true);
            ToolEllipticalArc(true);
            break;
        case Tool::Pin:
            ui->actionPinTool->setChecked(true);
            ToolPin(true);
            break;
        case Tool::InsertNode:
            ui->actionInsertNodeTool->setChecked(true);
            ToolInsertNode(true);
            break;
        case Tool::PlaceLabel:
            ui->actionPlaceLabelTool->setChecked(true);
            ToolPlaceLabel(true);
            break;
        case Tool::ArcStart:
            ui->actionArcStartPointTool->setChecked(true);
            ToolArcStart(true);
            break;
        case Tool::ArcEnd:
            ui->actionArcEndPointTool->setChecked(true);
            ToolArcEnd(true);
            break;
        case Tool::EllipticalArcWithLength:
            ui->actionEllipticalArcWithLengthTool->setChecked(true);
            ToolEllipticalArcWithLength(true);
            break;
        case Tool::ParallelCurve:
            ui->actionParallelCurveTool->setChecked(true);
            ToolParallelCurve(true);
            break;
        case Tool::GraduatedCurve:
            ui->actionGraduatedCurveTool->setChecked(true);
            ToolGraduatedCurve(true);
            break;
    }

    QT_WARNING_POP
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ShowTool  highlight tool.Tip show tools tooltip.
 * @param toolTip tooltip text.
 */
void MainWindow::ShowToolTip(const QString &toolTip)
{
    m_statusLabel->setText(toolTip);
}
//---------------------------------------------------------------------------------------------------------------------
#if defined(Q_OS_MAC)
void MainWindow::OpenAt(QAction *where)
{
    const QString path = VAbstractValApplication::VApp()->GetPatternPath().left(
                             VAbstractValApplication::VApp()->GetPatternPath().indexOf(where->text())) +
                         where->text();
    if (path == VAbstractValApplication::VApp()->GetPatternPath())
    {
        return;
    }
    QProcess process;
    process.start(QStringLiteral("/usr/bin/open"), QStringList() << path, QIODevice::ReadOnly);
    process.waitForFinished();
}
#endif // defined(Q_OS_MAC)
