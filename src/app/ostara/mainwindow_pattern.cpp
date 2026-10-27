/************************************************************************
 **
 **  @file   mainwindow_pattern.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26/08/2026
 **
 **  @brief  Pattern parse/undo/state and status/message methods
 **          extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QtGlobal>
#include <memory>
#include <new>
#include "core/opiecebuilder.h"
#include "core/opiecedocument.h"
#include "core/opiecelibrary.h"
#include "core/opieceimport.h"
#include "core/opieceinsert.h"
#include "core/opiecemark.h"
#include "core/opiecename.h"
#include "../vtools/undocommands/vchangelinerolename.h"
#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "core/odetailspropertybrowser.h"
#include "dialogs/dialognewpattern.h"
#include "dialogs/oexportfiledialog.h"
#include "dialogs/opiecelibrarydialog.h"
#include "dialogs/opieceimportdialog.h"
#include "dialogs/osavepiecedialog.h"
#include "dialogs/owaitdialog.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetdependencies.h"
#include "dialogs/vwidgetgroups.h"
#include "vtheme.h"
#include "../ifc/ifcdef.h"
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
#include "../vtools/tools/vinteractivetool.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vmaingraphicsview.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief triggers the update of the visibility groups
 */
void MainWindow::UpdateVisibilityGroups()
{
    m_dockManager->groupsWidget()->UpdateGroups();
    UpdateActiveLayerIndicator();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateActiveLayerIndicator()
{
    if (m_activeLayerRule == nullptr)
    {
        return;
    }

    // Layers belong to the Draw mode — every other mode shows neither the rule nor the name — and the Layers
    // dock can switch the whole chrome off: it is a reading aid, and a user who wants the drawing alone takes it
    // away. Both conditions answer NULL_ID, and NULL_ID is what hides everything below.
    const bool drawing = VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation;
    const bool chromeWanted = VAbstractApplication::VApp()->Settings()->GetShowActiveLayerOnCanvas();
    const quint32 active = (drawing && chromeWanted) ? doc->GetActiveGroup() : NULL_ID;

    if (active == NULL_ID)
    {
        m_activeLayerRule->hide();
        ui->view->SetCornerLabel(QString());
        return;
    }

    // VTheme::ResetThemeSettings puts a fresh palette on every widget, so the colour is re-applied here rather
    // than set once — this function runs again after a theme change.
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;
    QPalette palette = m_activeLayerRule->palette();
    palette.setColor(QPalette::Window, ColorNameToQColorForTheme(doc->GetGroupColor(active), dark));
    m_activeLayerRule->setPalette(palette);
    m_activeLayerRule->show();

    ui->view->SetCornerLabel(doc->GetGroupName(active));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowActiveLayerOnCanvas(bool on)
{
    if (auto *settings = VAbstractApplication::VApp()->Settings(); settings != nullptr)
    {
        settings->SetShowActiveLayerOnCanvas(on);
    }

    // The rule and the name are this function's only readers: re-running it is what shows or hides them.
    UpdateActiveLayerIndicator();
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

    // Le fichier se nettoie ici, avant d'être lu : `Spl_` et `SplPath_` s'écrivent `Curve_` là où une formule cite
    // une courbe. Les trois têtes mènent au même objet, et le produit n'en garde qu'une — sans quoi chacun
    // corrige à la main des jetons que le patron sait réécrire.
    if (doc != nullptr)
    {
        if (const int written = OPieceMark::NormaliseCurveHeads(doc->documentElement()); written > 0)
        {
            qCDebug(vMainWindow, "Curve heads normalised: %d token(s).", written);
        }

        // L'alias est prioritaire : ce qui cite encore le nom composé d'un objet qui porte un alias est corrigé
        // ici, **avant** la lecture — les outils lisent donc les formules déjà justes, et la fenêtre des options
        // comme le navigateur de propriétés montrent l'alias du premier coup (demandé le 13-10-2026).
        if (const int named = OPieceMark::UseAliases(doc->documentElement()); named > 0)
        {
            qCDebug(vMainWindow, "Aliases used: %d token(s).", named);
        }
    }

    // A full reparse tears down all tools and the VContainer. A tool options dialog still open now references
    // freed data, and accepting it would crash, so it is rejected before the teardown — unless it drives a tool
    // this parse rebuilds and says so (see DialogTool::SurvivesFullParse): it is then kept, and linked again to the
    // tool the parser builds, at the end of this function.
    QVector<QPair<QPointer<DialogTool>, quint32>> keptDialogs;
    if (m_toolOptionsDialogVisible)
    {
        const QList<DialogTool *> openDialogs = findChildren<DialogTool *>();
        for (DialogTool *dialog : openDialogs)
        {
            if (dialog->SurvivesFullParse())
            {
                keptDialogs.append({QPointer<DialogTool>(dialog), dialog->GetToolId()});
            }
            else
            {
                dialog->reject();
            }
        }
    }

    // What the dock is showing, read while the tool it belongs to is still alive: a full parse destroys every
    // tool, and without this the panel would close under the user's hands — a change that only a full parse can
    // make, the objects an operation creates, would take the page away with it.
    const quint32 shownToolId = VAbstractTool::ObjectIdOfItem(m_dockManager->toolOptions()->currentItem());

    m_dockManager->toolOptions()->ClearPropertyBrowser();

    // The full reparse below clears and rebuilds the details scene (PrepareForParse →
    // sceneDetail->clear()), which destroys the group holders. Forget them before the teardown
    // so RefreshGroupHolders (fired by CheckLayout after the geometry is applied) never touches
    // a dangling holder.
    m_groupHolders.clear();

    if (not FullParsePattern())
    {
        // Nothing was rebuilt: the dialogs kept above have no tool left to drive, and they are closed, as every
        // other one was before the teardown.
        for (const auto &[dialog, toolId] : std::as_const(keptDialogs))
        {
            Q_UNUSED(toolId)
            if (not dialog.isNull())
            {
                dialog->reject();
            }
        }

        return;
    }

    // Une courbe entière — spline, chemin, arc, volant — n'écrit son nom nulle part : le patron le compose sur ses
    // points, et il n'existe qu'ici, une fois la lecture faite. La passe d'avant la lecture a déjà corrigé ce que
    // le fichier portait lui-même ; celle-ci finit le travail, et la relecture est ce qui met les formules
    // corrigées sous les yeux des outils (mêmes commandes que le nommage : `LiteParseTree`).
    if (OPieceMark::UseAliases(doc->documentElement(), pattern) > 0)
    {
        doc->LiteParseTree(Document::FullLiteParse);
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

    GlobalChangePP(patternPiece);

    SetEnableTool(m_comboBoxDraws->count() > 0);
    UpdatePieceSaveAction();
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
    // The miniatures may have to wait for a reference drawing: settle that before anything repaints.
    UpdateThumbnailReferenceFlag();
    doc->RefreshPieceGeometry();
    // A full parse can have brought in, or removed, pieces that show a miniature.
    ScheduleThumbnailSnapshots();

    VMainGraphicsView::NewSceneRect(m_sceneDraw, VAbstractValApplication::VApp()->getSceneView());
    VMainGraphicsView::NewSceneRect(m_sceneDetails, VAbstractValApplication::VApp()->getSceneView());

    // A full parse recreated the tools, and the dock was emptied above: put it back on the tool it was showing, if
    // that tool is still there. It is exactly what a click on the tool does, so the page comes back as the user
    // left it, and a tool a deletion took away leaves the dock empty, as before.
    if (shownToolId != NULL_ID)
    {
        if (auto *tool = dynamic_cast<QGraphicsItem *>(VAbstractPattern::FindTool(shownToolId)); tool != nullptr)
        {
            m_dockManager->toolOptions()->itemClicked(tool);
        }
    }

    // The options dialogs kept above drove the tools this parse has just rebuilt: each is linked again to the tool
    // the pattern keeps under the id it remembers — never built a second time — and reads its options again, so that
    // the figure it edits is the one the file now holds. A tool the parse did not bring back leaves a dialog nothing
    // to drive: that one is rejected here, as it would have been before the teardown.
    for (const auto &[dialog, toolId] : std::as_const(keptDialogs))
    {
        if (dialog.isNull())
        {
            continue;
        }

        auto *tool = qobject_cast<VInteractiveTool *>(VAbstractPattern::FindTool(toolId));
        if (tool == nullptr)
        {
            dialog->reject();
            continue;
        }

        tool->SetDialogLink(dialog);
        dialog->SetAssociatedTool(tool);
        // The parse took the dialog's preview with it: it is built again before the rebuilt tool pushes its options,
        // or the setters that follow would have no preview to talk to (see DialogTool::RebuildVisualization).
        dialog->RebuildVisualization();
        tool->SetDialog();
    }
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
        UpdateActiveLayerIndicator(); // each piece has its own active layer
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

        // The generations of the Export mode are frozen, so any modification may have
        // moved what one of them is made of. The signal is deliberately coarse — it
        // targets no row, and it relaunches nothing: it only asks the user's opinion
        // when she comes back to the mode.
        //
        // Saving is not modifying. This slot also receives QUndoStack::cleanChanged, and
        // counting a Cmd+S as a change would greet the user with the warning every time
        // she saves before entering Export, which is the natural gesture.
        if (not saved)
        {
            m_exportOutputs.SetStale(true);
        }
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

    // The same dialog as the first piece, minus the unit — the unit belongs to the pattern, not to the
    // piece — and with the layer the piece starts in. A piece name is unique among the pieces of its
    // pattern, and the list of taken names is what lets the dialog refuse a duplicate before accepting.
    //
    // It is asked again as long as the piece taken from the library cannot enter: such a refusal leaves the
    // pattern exactly as it was — no block, no increment, no name — so there is nothing to repair, and the reader
    // is given back the window where the answer is his: another piece, or one of his own.
    while (true)
    {
        DialogNewPattern dialog(pattern, patternPieceName, DialogNewPattern::Creation::Piece, this);
        QStringList taken;
        taken.reserve(m_comboBoxDraws->count());
        for (int i = 0; i < m_comboBoxDraws->count(); ++i)
        {
            taken.append(m_comboBoxDraws->itemText(i));
        }
        dialog.SetTakenNames(taken);

        // The library is given to the dialog, the way it is given to the piece library window: the caller is what
        // knows which folder it is talking about, and a dialog that reads the settings itself cannot be measured.
        // The unit too: a piece of another unit is not a choice, since the insertion would refuse it.
        dialog.SetLibraryDirectory(OPieceLibrary::DefaultDirectory());
        dialog.SetLibraryUnit(OPieceImport::HostUnit(doc));

        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        // Two answers behind one window: a piece named by hand, or a piece taken from the library. The second
        // brings its own name, its layers and its increments, so it goes in through the insertion and nothing else
        // of this dialog is read.
        if (const QString imported = dialog.ImportedPath(); not imported.isEmpty())
        {
            if (not InsertLibraryPiece(imported))
            {
                continue;
            }
            return;
        }

        qCDebug(vMainWindow, "PP name: %s", qUtf8Printable(dialog.name()));

        AddPP(dialog.name(), dialog.LayerName(), dialog.LayerColor(), dialog.LayerPenStyle());
        return;
    }
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionSavePiece_triggered()
{
    if (doc == nullptr || m_comboBoxDraws == nullptr)
    {
        return;
    }

    const QString blockName = doc->PatternBlockMapper()->GetActive();
    if (blockName.isEmpty())
    {
        return;
    }

    // Ce que la pièce emploie sans que personne l'ait nommé : une ligne connue par sa paire, une courbe connue
    // par la route qui la bâtit. La fenêtre les montre tous, le nom donné à chacun est écrit dans le patron — et
    // les formules qui les appelaient, chez les autres pièces comprises, sont réparées dans le même geste.
    const QVector<OPieceName::Blocker> blockers = OPieceName::Blockers(doc, pattern, blockName);
    // Une seule liste : deux appels rendraient deux temporaires différents, et les itérateurs de l'un ne vont pas
    // avec la fin de l'autre — la taille qui en sort est un hasard, et le `reserve` qui suit tombe en bad_alloc.
    const QStringList declared = OPieceMark::DeclaredNames(doc->documentElement());
    const QSet<QString> taken(declared.begin(), declared.end());
    bool named = false;

    while (true)
    {
        // The name and the folder are asked before anything is written — and asked again as long as the library
        // already holds one under that name: the question is the door, and the only way through it is to name the
        // piece something else, or to put it in another folder. A cancelled dialog leaves the library as it was.
        OSavePieceDialog dialog(blockName, OPieceLibrary::DefaultDirectory(),
                                named ? QVector<OPieceName::Blocker>() : blockers, taken, this);
        // Ce que le lecteur a déjà écrit sur la pièce : la fenêtre le montre, donc enregistrer de nouveau ne le
        // perd pas — et ce qui est montré est exactement ce qui sera écrit, dans le fichier comme sur le bloc.
        dialog.SetNotes(doc->BlockNotes(blockName));
        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        if (not named && not blockers.isEmpty())
        {
            const QStringList names = dialog.Names();
            QUndoStack *const undoStack = VAbstractApplication::VApp()->getUndoStack();

            // Une seule opération : nommer les objets d'une pièce est une intention, et un seul Ctrl+Z la reprend.
            undoStack->beginMacro(tr("name the objects of a piece"));
            QVector<QPair<QString, QString>> curves;
            for (int i = 0; i < blockers.size() && i < names.size(); ++i)
            {
                const OPieceName::Blocker &blocker = blockers.at(i);
                if (names.at(i).isEmpty())
                {
                    continue;
                }

                if (blocker.pairKey.isEmpty())
                {
                    // La courbe reçoit son alias par son outil — et ses citations par le nom composé, que
                    // l'outil ne connaît pas, sont réparées juste après, sur le document entier.
                    if (NameCurveObject(doc, pattern, blocker.curveId, names.at(i)))
                    {
                        curves.append({blocker.name, names.at(i)});
                    }
                }
                else
                {
                    auto *command = new VChangeLineRoleName(doc, blocker.pairKey,
                                                            pattern->LineRoleName(blocker.pairKey), names.at(i));
                    connect(command, &VChangeLineRoleName::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
                    undoStack->push(command);
                }
            }
            if (not curves.isEmpty())
            {
                undoStack->push(new OPieceName::NameCurves(doc, curves));
            }
            undoStack->endMacro();

            // Le nommage réécrit les formules **d'autres pièces** — `Manche` appelait `Courbe_R1_Eb+1` — mais les
            // outils gardent le texte qu'ils ont lu. Sans cette relecture, le navigateur de propriétés continuait
            // d'afficher l'ancien nom jusqu'à ce qu'on rouvre la formule (relevé le 13-10-2026 : « si je
            // clique sur assistant formule, ça se maj »). C'est la relecture que `VChangeLineRoleName` demande déjà
            // quand il change un rôle.
            doc->LiteParseTree(Document::FullLiteParse);
            named = true;
        }

        const QString name = dialog.name();
        const QString filePath = dialog.LibraryPath();

        if (OExportFileDialog::ConfirmOverwrite(this, filePath, tr("Save piece")))
        {
            SavePieceToLibrary(blockName, name, filePath, dialog.notes());
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SavePieceToLibrary(const QString &blockName, const QString &name, const QString &filePath,
                                   const QString &notes)
{
    OPieceDocument piece;
    QString error;

    // Ce qu'une pièce ne peut pas emporter, dit avant toute écriture : une formule qui cite un objet par un nom
    // que le patron compose. Seul un nom **donné** — un alias — porte une identité et suit son objet d'un fichier
    // à l'autre ; un nom composé, lui, ne dit rien de qui il désignait.
    if (not OPieceBuilder::CheckCitations(doc, blockName, error))
    {
        QMessageBox::warning(this, tr("Save piece"), error);
        return;
    }

    if (not OPieceBuilder::Build(doc, pattern, blockName, name, &piece, error))
    {
        QMessageBox::warning(this, tr("Save piece"), error);
        return;
    }

    // Ce que le lecteur a écrit dans la fenêtre, **après** la fabrication : le bloc a pu arriver avec sa propre
    // note, et ce que la fenêtre montre est ce qui doit partir.
    piece.SetNotes(notes);

    if (not OPieceLibrary::Save(&piece, filePath, &error))
    {
        QMessageBox::warning(this, tr("Save piece"), error);
        return;
    }

    // Et le patron garde la même note : une seule vérité, celle du bloc, dont le fichier vient d'être copié. Le
    // bloc est donc modifié par l'enregistrement — comme il l'est déjà quand la fenêtre nomme ses objets sans nom.
    doc->SetBlockNotes(blockName, notes);

    statusBar()->showMessage(tr("Piece saved: %1").arg(QFileInfo(filePath).fileName()), 5000);
    qCDebug(vMainWindow, "Piece %s saved.", qUtf8Printable(filePath));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionPieceLibrary()
{
    // The library is a window on a folder: it reads the pieces and says what each of them needs. It writes
    // nothing — the piece it hands back is inserted by the pattern, and that is a change of the pattern.
    OPieceLibraryDialog dialog(OPieceLibrary::DefaultDirectory(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    InsertLibraryPiece(dialog.SelectedPath());
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::InsertLibraryPiece(const QString &filePath) -> bool
{
    if (doc == nullptr || pattern == nullptr || filePath.isEmpty())
    {
        return false;
    }

    OPieceDocument piece;
    QString error;
    if (not OPieceLibrary::Load(filePath, &piece, &error))
    {
        QMessageBox::warning(this, tr("Piece library"), error);
        return false;
    }

    // What the piece may not do, said before anything is touched: a pattern that holds no measurement table to
    // read the piece against — or one that does not define what the piece reads —, a piece of another unit, and a
    // piece whose values live in a piece this pattern does not hold. All of them are refusals, never silent
    // entries — and the measurements are the only thing the user can repair on the spot, so the table is asked
    // for and the insertion carries on once it is loaded.
    if (not OPieceImport::CheckMeasurements(piece, doc, pattern, error))
    {
        QMessageBox ask(QMessageBox::Question, tr("Piece library"), error, QMessageBox::NoButton, this);
        QPushButton *const load = ask.addButton(tr("Load a measurement table"), QMessageBox::AcceptRole);
        ask.addButton(QMessageBox::Cancel);
        ask.exec();

        if (ask.clickedButton() == load)
        {
            OnLoadMeasurements();
        }

        // The table is only there once the chosen file has been read: a cancelled question, a cancelled file
        // dialog, or a legacy table that still has to be converted all leave the pattern without one — and the
        // piece has then nothing to be read with.
        if (not OPieceImport::CheckMeasurements(piece, doc, pattern, error))
        {
            // A table that **is** there and still does not define what the piece reads is another matter: it is
            // not a deliberate nothing, and the reader has to be told which measurement is missing rather than
            // watch the piece not enter.
            if (not doc->MPath().isEmpty())
            {
                QMessageBox::warning(this, tr("Piece library"), error);
            }
            return false;
        }
    }

    if (not OPieceImport::CheckUnit(piece, doc, error) || not OPieceImport::CheckDependencies(piece, doc, error))
    {
        QMessageBox::warning(this, tr("Piece library"), error);
        return false;
    }

    // The instance name is the piece's own, and a second one takes the next free name: the block name is what the
    // top bar shows, and two blocks cannot wear the same one.
    QString instanceName = piece.PieceName();
    const QStringList blockNames = doc->PatternBlockMapper()->GetBlockNames();
    for (int suffix = 2; blockNames.contains(instanceName) && suffix < 100; ++suffix)
    {
        instanceName = piece.PieceName() + QLatin1Char(' ') + QString::number(suffix);
    }

    // What the piece asks before it enters: the name it would wear, every alias it carries — cited by a formula or
    // not — and the mark the pattern proposes for them. Nothing is written yet, and nothing is decided here.
    OPieceInsert::Proposal proposal = OPieceInsert::Propose(piece, doc, pattern, instanceName);

    // Ce que la pièce lit ailleurs et que **plusieurs** blocs du patron peuvent lui offrir : deux instances de la
    // même pièce de bibliothèque portent la même identité et la même catégorie, et c'est au lecteur de dire sur
    // laquelle la pièce se base (`Buste` ou `Buste 2`). Un seul candidat ne se demande pas : c'est le lien
    // silencieux, et la formule marche telle quelle.
    QVector<OPieceImport::Reference> ambiguous;
    for (const OPieceImport::Reference &reference : OPieceImport::References(piece, doc))
    {
        if (reference.candidates.size() > 1)
        {
            ambiguous.append(reference);
        }
    }

    // The dialog is opened when there is something to decide, which is an alias to look at or a value read
    // elsewhere that several pieces can satisfy: a piece that asks nothing enters in one gesture, the gesture of
    // every other import.
    QHash<QString, QString> bound; // the piece's citations, rewritten to the instance chosen in the dialog
    if (not proposal.aliases.isEmpty() || not ambiguous.isEmpty())
    {
        OPieceImportDialog dialog(proposal, blockNames, OPieceInsert::Keys(pattern), ambiguous, this);
        if (dialog.exec() != QDialog::Accepted)
        {
            return false;
        }

        proposal.instanceName = dialog.InstanceName();
        proposal.table = dialog.Table();

        // Les citations de la pièce vers l'instance choisie : `Curve_dos` devient `Curve_dos__P1` quand c'est la
        // seconde instance qui est choisie, et **rien** quand c'est la première — ses noms sont ceux que la pièce
        // écrit déjà. Le renommage passe par la même porte que la marque, donc par les mêmes formes de jetons.
        bound = OPieceImport::ChosenNames(piece, doc, dialog.ChosenReferences());
    }

    // Une seconde instance réécrit tous les noms et tous les ids qu'elle porte avant d'entrer, et le parse qui
    // suit reconstruit tout le patron : ce travail bloque le fil de l'interface, et sans fenêtre pour le dire le
    // canvas passerait pour figé. La première instance, qui n'entre en collision avec rien, garde le chemin
    // silencieux — c'est l'affaire d'un souffle.
    std::unique_ptr<OWaitDialog> waitDialog;
    if (not proposal.table.isEmpty() || not bound.isEmpty())
    {
        waitDialog = std::unique_ptr<OWaitDialog>(new OWaitDialog(tr("Loading the piece. Please, wait."), this));
        waitDialog->show();
        // Le système de fenêtres a besoin de quelques tours d'événements pour mapper et peindre la fenêtre : un
        // seul pump peut laisser le dessin en attente, et le travail bloque le fil juste après — la fenêtre ne
        // serait alors jamais visible (même précaution que dans mainwindow_graded_build.cpp).
        for (int i = 0; i < 4; ++i)
        {
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }

    if (not bound.isEmpty())
    {
        QDomElement root = piece.documentElement();
        OPieceMark::Rename(root, bound);
    }

    // Where it takes place, decided before anything is written: a piece brings a whole construction with it, and
    // it starts beyond what the pattern already shows. The canvas thinks in pixels, the document in the pattern's
    // own unit, and this is the one place the two meet.
    const QPointF beyond = PositionBeyondDrawn();
    const QPointF anchorTarget(VAbstractValApplication::VApp()->fromPixel(beyond.x()),
                               VAbstractValApplication::VApp()->fromPixel(beyond.y()));

    const OPieceInsert::Prepared prepared = OPieceInsert::Prepare(piece, pattern, proposal, anchorTarget);
    if (not prepared.IsReady())
    {
        // The wait window is application-modal: let it go before the warning, or the two would stack.
        waitDialog.reset();
        QMessageBox::warning(this, tr("Piece library"), tr("This piece holds no block to insert."));
        return false;
    }

    // One gesture: the block, the increments it brings, the groups that hold them and the names of its lines all
    // go in through one command, so one Ctrl+Z takes the whole piece back out.
    VAbstractApplication::VApp()->getUndoStack()->push(
        new OPieceInsert::AddPieceFromLibrary(prepared, piece, doc, pattern));

    // And the new piece takes the canvas, the way a piece drawn by hand does. The selector is what remembers it:
    // the full parse that follows rebuilds the list and puts back the piece the selector names, so the piece has
    // to be named there **before** that parse — the same two lines AddPP writes for a piece born in the app.
    if (m_comboBoxDraws != nullptr)
    {
        const QSignalBlocker blocker(m_comboBoxDraws);
        if (m_comboBoxDraws->findText(prepared.instanceName) == -1)
        {
            m_comboBoxDraws->addItem(prepared.instanceName);
        }
        m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->findText(prepared.instanceName));
    }

    // And the view follows it, the way it follows a piece drawn by hand: the piece landed beyond everything, so
    // without this the gesture would show nothing at all.
    QApplication::postEvent(this, new FitBestCurrentEvent());

    statusBar()->showMessage(tr("Piece inserted: %1").arg(prepared.instanceName), 5000);
    qCDebug(vMainWindow, "Piece %s inserted as %s.", qUtf8Printable(filePath), qUtf8Printable(prepared.instanceName));

    return true;
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
