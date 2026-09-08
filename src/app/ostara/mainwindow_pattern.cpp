/************************************************************************
 **
 **  @file   mainwindow_pattern.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26/08/2026
 **
 **  @brief  Pattern parse/undo/state and status/message methods
 **          extracted from mainwindow.cpp
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
#include "widgets/ODockManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QCursor>
#include <QDateTime>
#include <QFormLayout>
#include <QGuiApplication>
#include <QProgressBar>
#include <QSignalBlocker>
#include <QtGlobal>
#include <new>

#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "core/odetailspropertybrowser.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetdependencies.h"
#include "dialogs/vwidgetgroups.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionundo.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vexception/vexception.h"
#include "../vmisc/customevents.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vtools/dialogs/tools/dialogtool.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vmaingraphicsview.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief triggers the update of the visibility groups
 */
void MainWindow::UpdateVisibilityGroups()
{
    m_dockManager->groupsWidget()->UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief triggers the update of the details list
 */
void MainWindow::UpdateDetailsList()
{
    m_dockManager->detailsWidget()->UpdateList();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::FullParseFile()
{
    qCDebug(vMainWindow, "Full parsing file");

    if (m_toolOptionsDialogVisible)
    {
        // A full reparse tears down all tools and the VContainer. Any tool options dialog still open
        // now references freed data, and accepting it would crash. Reject it before teardown.
        const QList<DialogTool *> openDialogs = findChildren<DialogTool *>();
        for (DialogTool *dialog : openDialogs)
        {
            dialog->reject();
        }
    }

    m_dockManager->toolOptions()->ClearPropertyBrowser();

    // The full reparse below clears and rebuilds the details scene (PrepareForParse →
    // sceneDetail->clear()), which destroys the group holders. Forget them before the teardown
    // so RefreshGroupHolders (fired by CheckLayout after the geometry is applied) never touches
    // a dangling holder.
    m_groupHolders.clear();

    if (not FullParsePattern())
    {
        return;
    }

    QString patternPiece;
    if (m_comboBoxDraws->currentIndex() != -1)
    {
        patternPiece = m_comboBoxDraws->itemText(m_comboBoxDraws->currentIndex());
    }

    {
        const QSignalBlocker blocker(m_comboBoxDraws);
        m_comboBoxDraws->clear();

        QStringList patternPieceNames = doc->PatternBlockMapper()->GetBlockNames();
        patternPieceNames.sort();
        m_comboBoxDraws->addItems(patternPieceNames);

        if (not m_drawMode)
        {
            m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1);
        }
        else
        {
            const qint32 index = m_comboBoxDraws->findText(patternPiece);
            if (index != -1)
            {
                m_comboBoxDraws->setCurrentIndex(index);
            }
        }
    }
    ui->actionPattern_properties->setEnabled(true);

    GlobalChangePP(patternPiece);

    SetEnableTool(m_comboBoxDraws->count() > 0);
    m_dockManager->detailsWidget()->UpdateList();
    // A full parse recreated the tools: if the details-options dock still shows the piece page,
    // repopulate it with the recreated tool, so an undo/redo of a delete refreshes the internal
    // path list (otherwise the restored path stays off the list).
    m_dockManager->detailsOptions()->RefreshPieceAfterParse();
    // Flush the pieces' geometry synchronously instead of waiting for the 250 ms debounce timer:
    // otherwise the first paint after a full parse shows empty pieces (silhouette not yet applied)
    // with only the node points → a white "flash". RefreshPieceGeometry() stops the debounce timer
    // and applies the geometry now (Modeling mode). CheckLayout (emitted when the geometry is
    // applied) → RefreshGroupHolders rebuilds the group holders from the final geometry.
    doc->RefreshPieceGeometry();

    VMainGraphicsView::NewSceneRect(m_sceneDraw, VAbstractValApplication::VApp()->getSceneView());
    VMainGraphicsView::NewSceneRect(m_sceneDetails, VAbstractValApplication::VApp()->getSceneView());
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::GlobalChangePP(const QString &patternPiece)
{
    const qint32 index = m_comboBoxDraws->findText(patternPiece);
    if (index == m_comboBoxDraws->currentIndex() && doc->PatternBlockMapper()->GetActive() == patternPiece)
    {
        return;
    }

    try
    {
        if (index != -1)
        { // -1 for not found
            ChangePP(index, false);
            const QSignalBlocker blocker(m_comboBoxDraws);
            m_comboBoxDraws->setCurrentIndex(index);
        }
        else
        {
            ChangePP(0, false);
        }
    }
    catch (VExceptionBadId &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Bad id.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ChangePP(int index, bool zoomBestFit)
{
    if (index != -1)
    {
        doc->PatternBlockMapper()->SetActive(m_comboBoxDraws->itemText(index));
        doc->setCurrentData();
        if (m_drawMode && zoomBestFit)
        {
            ZoomFitBestCurrent();
        }
        m_dockManager->toolOptions()->itemClicked(nullptr); // hide options for tool in previous pattern piece
        m_dockManager->groupsWidget()->UpdateGroups();
        m_dockManager->dependenciesWidget()->UpdateDependencies();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief haveChange enable action save if we have unsaved change.
 */
void MainWindow::PatternChangesWereSaved(bool saved)
{
    if (m_guiEnabled)
    {
        const bool state = doc->IsModified() || !saved;
        setWindowModified(state);
        not m_patternReadOnly ? ui->actionSave->setEnabled(state) : ui->actionSave->setEnabled(false);
        m_layoutSettings->SetLayoutStale(true);
        isNeedAutosave = not saved;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowProgress()
{
    if (m_progressBar->isVisible() && m_progressBar->value() + 1 <= m_progressBar->maximum())
    {
        const int newValue = m_progressBar->value() + 1;
        m_progressBar->setValue(newValue);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_taskbarProgress->setValue(newValue);
#endif
        QCoreApplication::processEvents();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClearPatternMessages()
{
    ui->plainTextEditPatternMessages->clear();
    if (not m_unreadPatternMessage.isNull())
    {
        m_unreadPatternMessage->setText(QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PrintPatternMessage(QEvent *event)
{
    SCASSERT(event != nullptr)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    const auto *patternMessageEvent = static_cast<WarningMessageEvent *>(event);

    QString severity;

    switch (patternMessageEvent->Severity())
    {
        case QtDebugMsg:
            severity = tr("DEBUG");
            break;
        case QtWarningMsg:
            severity = tr("WARNING");
            break;
        case QtCriticalMsg:
            severity = tr("CRITICAL");
            break;
        case QtFatalMsg:
            severity = tr("FATAL");
            break;
        case QtInfoMsg:
            severity = tr("INFO");
            break;
        default:
            break;
    }

    const QString patternMessage = patternMessageEvent->Message()
            .remove(VAbstractValApplication::warningMessageSignature);
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    const auto message = QStringLiteral("%1: [%2] %3").arg(time, severity, patternMessage);
    ui->plainTextEditPatternMessages->appendFilteredText(message);
    if (not m_unreadPatternMessage.isNull())
    {
        m_unreadPatternMessage->setText(DialogWarningIcon(m_unreadPatternMessage) + tr("Pattern messages"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::FullParsePattern() -> bool
{
    auto HandleError = [this]()
    {
        SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_DATAERR);
        }
    };

    try
    {
        // QDomDocument is not thread-safe and Parse() rewrites the same document, so this must stay on this thread.
        if (VAbstractValApplication::VApp()->getOpeningPattern())
        {
            doc->TestUniqueId();
        }

        SetEnabledGUI(true);
        doc->Parse(Document::FullParse);
        ParseBackgroundImages();
    }
    catch (const VExceptionUndo &)
    {
        /* If user want undo last operation before undo we need finish broken redo operation. For those we post event
         * myself. Later in method customEvent call undo.*/
        QApplication::postEvent(this, new UndoEvent());
        return false;
    }
    catch (const VExceptionObjectError &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")), //-V807
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const VExceptionConversionError &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error can't convert value.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const VExceptionWrongId &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error wrong id.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (VException &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const std::bad_alloc &)
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Error parsing file (std::bad_alloc).")));
        HandleError();
        return false;
    }

    return true;
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshPieceGeometry()
{
  if(!doc->IsPieceGeometryDirty())
  {
    return;
  }

  doc->RefreshDirtyPieceGeometry(pattern->DataPieces()->keys());
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PreviousPatternPiece()
{
    int index = m_comboBoxDraws->currentIndex();

    if (index == -1 || m_comboBoxDraws->count() <= 1)
    {
        return;
    }

    if (index == 0)
    {
        index = m_comboBoxDraws->count() - 1;
    }
    else
    {
        --index;
    }

    m_comboBoxDraws->setCurrentIndex(index);
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::NextPatternPiece()
{
    int index = m_comboBoxDraws->currentIndex();

    if (index == -1 || m_comboBoxDraws->count() <= 1)
    {
        return;
    }

    if (index == m_comboBoxDraws->count() - 1)
    {
        index = 0;
    }
    else
    {
        ++index;
    }

    m_comboBoxDraws->setCurrentIndex(index);
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionNewDraw_triggered()
{
    qCDebug(vMainWindow, "New PP.");
    QString patternPieceName = tr("Pattern piece %1").arg(m_comboBoxDraws->count() + 1);
    qCDebug(vMainWindow, "Generated PP name: %s", qUtf8Printable(patternPieceName));

    qCDebug(vMainWindow, "PP count %d", m_comboBoxDraws->count());
    bool const ok = PatternPieceName(patternPieceName);
    qCDebug(vMainWindow, "PP name: %s", qUtf8Printable(patternPieceName));
    if (not ok)
    {
        return;
    }

    AddPP(patternPieceName);
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionShowMainPath_triggered(bool checked)
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetPieceShowMainPath(checked);
    const QList<quint32> ids = pattern->DataPieces()->keys();
    const bool updateChildren = false;
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    for (const auto &id : ids)
    {
        try
        {
            if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id)))
            {
                tool->RefreshGeometry(updateChildren);
            }
        }
        catch (VExceptionBadId &)
        {
        }
    }
    QGuiApplication::restoreOverrideCursor();
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionBoundaryTogetherWithNotches_triggered(bool checked)
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetBoundaryTogetherWithNotches(checked);
    const QList<quint32> ids = pattern->DataPieces()->keys();
    const bool updateChildren = false;
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    for (const auto &id : ids)
    {
        try
        {
            if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id)))
            {
                tool->RefreshGeometry(updateChildren);
            }
        }
        catch (VExceptionBadId &)
        {
        }
    }
    QGuiApplication::restoreOverrideCursor();
}
