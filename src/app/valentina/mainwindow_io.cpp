/************************************************************************
 **
 **  @file   mainwindow_io.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  File I/O methods extracted from mainwindow.cpp
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

#include <QAction>
#include <QComboBox>
#include <QPointer>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QGlobalStatic>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QScopeGuard>
#include <QScrollBar>
#include <QSettings>
#include <QShowEvent>
#include <QStyleFactory>
#include <QTimer>
#include <QUndoStack>
#include <QUuid>
#include <QtConcurrent>
#include <QtDebug>
#include <QtGlobal>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptioninvalidhistory.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionundo.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../ifc/xml/utils.h"
#include "../ifc/xml/vbackgroundpatternimage.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vvitconverter.h"
#include "../ifc/xml/vvstconverter.h"
#include "../vformat/vmeasurements.h"
#include "../vformat/vpatternrecipe.h"
#include "../vganalytics/vganalytics.h"
#include "../vlayout/dialogs/dialoglayoutscale.h"
#include "../vlayout/dialogs/watermarkwindow.h"
#include "../vlayout/vlayoutexporter.h"
#include "../vmisc/customevents.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vmisc/def.h"
#include "../vmisc/dialogs/dialogaskcollectstatistic.h"
#include "../vmisc/dialogs/dialogselectlanguage.h"
#include "../vmisc/qxtcsvmodel.h"
#include "vtheme.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmodifierkey.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vfontinstaller.h"
#include "../vmisc/projectversion.h"
#include "../vmisc/dialogs/dialogselectmeasurementstype.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/variables/vmeasurement.h"
#include "../vtools/dialogs/support/dialogeditlabel.h"
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
#include "../vtools/dialogs/tools/dialogellipticalarcwithlength.h"
#include "../vtools/dialogs/tools/dialogellipticalarc.h"
#include "../vtools/dialogs/tools/dialogendline.h"
#include "../vtools/dialogs/tools/dialogflippingbyaxis.h"
#include "../vtools/dialogs/tools/dialogflippingbyline.h"
#include "../vtools/dialogs/tools/dialoggroup.h"
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
#include "../vtools/dialogs/tools/dialogtriangle.h"
#include "../vtools/dialogs/tools/dialogtruedarts.h"
#include "../vtools/dialogs/tools/dialoguniondetails.h"
#include "../vtools/dialogs/tools/piece/dialogduplicatedetail.h"
#include "../vtools/dialogs/tools/piece/dialoginsertnode.h"
#include "../vtools/dialogs/tools/piece/dialogpiecepath.h"
#include "../vtools/dialogs/tools/piece/dialogpin.h"
#include "../vtools/dialogs/tools/piece/dialogplacelabel.h"
#include "../vtools/dialogs/tools/piece/dialogseamallowance.h"
#include "../vtools/tools/backgroundimage/vbackgroundimagecontrols.h"
#include "../vtools/tools/backgroundimage/vbackgroundimageitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
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
#include "../vtools/tools/nodeDetails/vtoolpiecepath.h"
#include "../vtools/tools/nodeDetails/vtoolpin.h"
#include "../vtools/tools/nodeDetails/vtoolplacelabel.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"
#include "../vtools/undocommands/image/addbackgroundimage.h"
#include "../vtools/undocommands/image/deletebackgroundimage.h"
#include "../vtools/undocommands/renamepp.h"
#include "../vtools/undocommands/undogroup.h"
#include "../vtools/undocommands/label/movedoublelabel.h"
#include "../vtools/undocommands/label/movelabel.h"
#include "../vtools/undocommands/label/movesegmentlabel.h"
#include "../vtools/undocommands/label/operationmovelabel.h"
#include "../vwidgets/vabstractsimple.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/labelarrange/vdrawlabelcollector.h"
#include "../vwidgets/labelarrange/vlabelarrangeengine.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vscenepoint.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsimplepoint.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vwidgets/vtoolbuttonpopup.h"
#include "../vwidgets/vwidgetpopup.h"
#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/dialogaboutapp.h"
#include "dialogs/dialogaddbackgroundimage.h"
#include "dialogs/dialogfinalmeasurements.h"
#include "dialogs/dialogincrements.h"
#include "dialogs/dialognewpattern.h"
#include "dialogs/dialogpatternproperties.h"
#include "dialogs/dialogpreferences.h"
#include "../vtools/dialogs/tools/dialogparallelcurve.h"
#include "dialogs/vwidgetbackgroundimages.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetgroups.h"
#include "dialogs/vwidgetdependencies.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/dialogs/tools/dialoggraduatedcurve.h"
#include "vabstractapplication.h"
#include "vabstractshortcutmanager.h"
#include "vsinglelineoutlinechar.h"
#include "../ifc/xml/vpatternblockmapper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QSourceLocation>
#endif

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#if defined(Q_OS_MAC)
#include <QDrag>
#include <QMimeData>
#endif // defined(Q_OS_MAC)

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

#ifdef Q_OS_WIN
#ifdef DocumentProperties
#endif
#endif

#include "ui_mainwindow.h"

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

#include "mainwindow_helpers.h"



//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ReadSettings read setting for app.
 */
void MainWindow::ReadSettings()
{
    qCDebug(vMainWindow, "Reading settings.");
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    if (settings->status() == QSettings::NoError)
    {
        restoreGeometry(settings->GetGeometry());
        restoreState(settings->GetToolbarsState(), kToolbarStateVersion);

        m_groupsActive = settings->IsDockWidgetGroupsActive();
        m_toolOptionsActive = settings->IsDockWidgetToolOptionsActive();
        m_patternMessagesActive = settings->IsDockWidgetPatternMessagesActive();
        m_backgroundImagesActive = settings->IsDockWidgetBackgroundImagesActive();
        m_dependenciesActive = settings->IsDockWidgetDependenciesActive();

        ui->dockWidgetGroups->setVisible(m_groupsActive);
        ui->dockWidgetToolOptions->setVisible(m_toolOptionsActive);
        ui->dockWidgetMessages->setVisible(m_patternMessagesActive);
        ui->dockWidgetBackgroundImages->setVisible(m_backgroundImagesActive);
        ui->dockWidgetDependencies->setVisible(m_dependenciesActive);

        // Scene antialiasing
        ui->view->SetAntialiasing(settings->GetGraphicalOutput());

        // Stack limit
        VAbstractApplication::VApp()->getUndoStack()->setUndoLimit(settings->GetUndoCount());

        // Text under tool buton icon
        ToolBarStyles();

        ToolboxIconSize();

        QFont f = ui->plainTextEditPatternMessages->font();
        f.setPointSize(qMax(settings->GetPatternMessageFontSize(f.pointSize()), 1));
        ui->plainTextEditPatternMessages->setFont(f);

        if (VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager())
        {
            manager->UpdateShortcuts();
        }
    }
    else
    {
        qWarning() << tr("Cannot read settings from a malformed .INI file.");
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief WriteSettings save setting for app.
 */
void MainWindow::WriteSettings()
{
    ActionDraw(true);

    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetGeometry(saveGeometry());
    settings->SetToolbarsState(saveState(kToolbarStateVersion));

    settings->SetDockWidgetGroupsActive(ui->dockWidgetGroups->isVisible());
    settings->SetDockWidgetToolOptionsActive(ui->dockWidgetToolOptions->isVisible());
    settings->SetDockWidgetPatternMessagesActive(ui->dockWidgetMessages->isVisible());
    settings->SetDockWidgetBackgroundImagesActive(actionDockWidgetBackgroundImages->isChecked());
    settings->SetDockWidgetDependenciesActive(ui->dockWidgetDependencies->isVisible());

    settings->sync();
    if (settings->status() == QSettings::AccessError)
    {
        qWarning() << tr("Cannot save settings. Access denied.");
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SavePattern save pattern file.
 * @param fileName pattern file name.
 * @return true if all is good.
 */
auto MainWindow::SavePattern(const QString &fileName, QString &error) -> bool
{
    qCDebug(vMainWindow, "Saving pattern file %s.", qUtf8Printable(fileName));
    QFileInfo const tempInfo(fileName);

    const QString mPath = AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
    if (not mPath.isEmpty() && VAbstractValApplication::VApp()->GetPatternPath() != fileName)
    {
        doc->SetMPath(RelativeMPath(fileName, mPath));
    }

    const bool result = doc->SaveDocument(fileName, error);
    if (result)
    {
        if (tempInfo.suffix() != "autosave"_L1)
        {
            setCurrentFile(fileName);
            statusBar()->showMessage(tr("File saved"), 5000);
            qCDebug(vMainWindow, "File %s saved.", qUtf8Printable(fileName));
            PatternChangesWereSaved(result);
        }
    }
    else
    {
        doc->SetMPath(mPath);
        emit doc->UpdatePatternLabel();
        qCDebug(vMainWindow, "Could not save file %s. %s.", qUtf8Printable(fileName), qUtf8Printable(error));
    }
    return result;
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AutoSavePattern start safe saving.
 */
void MainWindow::AutoSavePattern()
{
    if (VApplication::IsGUIMode() && not VAbstractValApplication::VApp()->GetPatternPath().isEmpty() &&
        isWindowModified() && isNeedAutosave)
    {
        qCDebug(vMainWindow, "Autosaving pattern.");
        QString error;
        if (SavePattern(VAbstractValApplication::VApp()->GetPatternPath() + AutosavePrefix(), error))
        {
            isNeedAutosave = false;
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setCurrentFile the function is called to reset the state of a few variables when a file
 * is loaded or saved, or when the user starts editing a new file (in which case fileName is empty).
 * @param fileName file name.
 */
void MainWindow::setCurrentFile(const QString &fileName)
{
    qCDebug(vMainWindow, "Set current name to \"%s\"", qUtf8Printable(fileName));
    VAbstractValApplication::VApp()->SetPatternPath(fileName);
    doc->SetPatternWasChanged(true);
    emit doc->UpdatePatternLabel();
    VAbstractApplication::VApp()->getUndoStack()->setClean();

    if (not VAbstractValApplication::VApp()->GetPatternPath().isEmpty() && VApplication::IsGUIMode())
    {
        qCDebug(vMainWindow, "Updating recent file list.");
        VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
        QStringList files = settings->GetRecentFileList();
        files.removeAll(fileName);
        files.prepend(fileName);
        while (files.size() > MaxRecentFiles)
        {
            files.removeLast();
        }

        settings->SetRecentFileList(files);
        UpdateRecentFileActions();

        qCDebug(vMainWindow, "Updating restore file list.");
        QStringList restoreFiles = settings->GetRestoreFileList();
        restoreFiles.removeAll(fileName);
        restoreFiles.prepend(fileName);
        settings->SetRestoreFileList(restoreFiles);
    }

    UpdateWindowTitle();
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::FileClosedCorrect()
{
    WriteSettings();

    // File was closed correct.
    QStringList restoreFiles = VAbstractValApplication::VApp()->ValentinaSettings()->GetRestoreFileList();
    restoreFiles.removeAll(VAbstractValApplication::VApp()->GetPatternPath());
    VAbstractValApplication::VApp()->ValentinaSettings()->SetRestoreFileList(restoreFiles);

    // Remove autosave file
    if (QFile autofile(VAbstractValApplication::VApp()->GetPatternPath() + AutosavePrefix()); autofile.exists())
    {
        autofile.remove();
    }
    qCDebug(vMainWindow, "File %s closed correct.", qUtf8Printable(VAbstractValApplication::VApp()->GetPatternPath()));
}

