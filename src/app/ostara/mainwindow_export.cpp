/************************************************************************
 **
 **  @file   mainwindow_export.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Export methods extracted from mainwindow.cpp
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
#include "../vlayout/vlayoutexporter.h"
#include "../vformat/vpatternrecipe.h"
#include "../ifc/exception/vexceptioninvalidhistory.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vexception/vexception.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgobject.h"
#include "../vpatterndb/variables/vcurvevariable.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "core/vapplication.h"
#include "core/opiecedocument.h"
#include "core/opiecebuilder.h"
#include "core/opiecelibrary.h"
#include "core/opieceimport.h"
#include "core/opieceinsert.h"
#include "core/opiecemark.h"
#include "core/opiecename.h"
#include "core/opieceremap.h"
#include "dialogs/dialogincrements.h"
#include "dialogs/dialognewpattern.h"
#include "dialogs/opiecelibrarydialog.h"
#include "dialogs/opieceimportdialog.h"
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QGraphicsItem>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGroupBox>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryFile>
#include <QUuid>
#include <QtDebug>
#include <cmath>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportDraw(const QString &fileName)
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    VLayoutExporter exporter;

    QT_WARNING_POP

    exporter.SetFileName(fileName);

    int const verticalScrollBarValue = ui->view->verticalScrollBar()->value();
    int const horizontalScrollBarValue = ui->view->horizontalScrollBar()->value();

    QTransform const viewTransform = ui->view->transform();
    ui->view->ZoomFitBest(); // Resize all labels
    ui->view->repaint();
    ui->view->ZoomOriginal(); // Set to original scale

    // Enable all items on scene
    const QList<QGraphicsItem *> qItems = m_sceneDraw->items();
    for (auto *item : qItems)
    {
        item->setEnabled(true);
        if (item->type() == VGraphicsSimpleTextItem::Type)
        {
            auto *text = dynamic_cast<VGraphicsSimpleTextItem *>(item);
            text->RefreshColor(); // Regular update doesn't work on labels
        }
    }

    ui->view->repaint();

    const QRectF rect = m_sceneDraw->VisibleItemsBoundingRect();
    m_sceneDraw->update(rect);
    exporter.SetImageRect(rect);
    exporter.SetOffset(rect.topLeft()); // Correct positions to fit SVG view rect

    exporter.ExportToSVG(m_sceneDraw, QList<QGraphicsItem *>());

    // Restore scale, scrollbars and current active pattern piece
    ui->view->setTransform(viewTransform);
    VMainGraphicsView::NewSceneRect(ui->view->scene(), ui->view);
    ScaleChanged(ui->view->transform().m11());

    ui->view->verticalScrollBar()->setValue(verticalScrollBarValue);
    ui->view->horizontalScrollBar()->setValue(horizontalScrollBarValue);

    m_sceneDraw->EnableTools();
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportDrawAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionExportDraw->setChecked(false); });

    QString const filters(tr("Scalable Vector Graphics files") + QStringLiteral("(*.svg)"));
    QString const dir = QDir::homePath() + '/'_L1 + FileName() + QStringLiteral(".svg");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save draw"), dir, filters, nullptr,
                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (fileName.isEmpty())
    {
        return;
    }

    if (QFileInfo const f(fileName); f.suffix().isEmpty() || f.suffix() != "svg"_L1)
    {
        fileName += ".svg"_L1;
    }

    ExportDraw(fileName);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportDetailsAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionDetailExportAs->setChecked(false); });

    QVector<DetailForLayout> const detailsInLayout = SortDetailsForLayout(pattern->DataPieces(), pattern);

    if (detailsInLayout.isEmpty())
    {
        QMessageBox::information(this, tr("Layout mode"),
                                 tr("You don't have enough details to export. Please, "
                                    "include at least one detail in layout."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QVector<VLayoutPiece> listDetails;
    try
    {
        listDetails = PrepareDetailsForLayout(detailsInLayout, doc);
    }
    catch (VException &e)
    {
        QMessageBox::warning(this, tr("Export details"), tr("Can't export details.") + " \n"_L1 + e.ErrorMessage(),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    try
    {
        m_dialogSaveLayout =
            QSharedPointer<DialogSaveLayout>(new DialogSaveLayout(1, FileName(), this));

        VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
        m_dialogSaveLayout->SetBoundaryTogetherWithNotches(settings->IsBoundaryTogetherWithNotches());

        if (m_dialogSaveLayout->exec() == QDialog::Rejected)
        {
            m_dialogSaveLayout.clear();
            return;
        }

        // The miniatures are drawn at the REFERENCE size, and that drawing is captured on a timer:
        // an export fired in the 400 ms that follow an opening (or an undo) would write a document
        // without them, while the canvas is about to show them. Settle it now — the round trip is
        // what the timer would have done anyway, and it restores the modified flag.
        RefreshThumbnailSnapshots();

        // "Show the main path" is read here, once the dialog has been accepted: the pieces are laid out
        // again, this time as if their main path were visible. The pattern is not touched.
        if (m_dialogSaveLayout->IsShowMainPath())
        {
            listDetails = PrepareDetailsForLayout(detailsInLayout, doc, false, true);
        }

        ExportData(listDetails);

        // Say it worked, and where it landed — the same word Graded mode and Export mode already
        // give. The export path is not a single file for every format (a tiled PDF is a page per
        // file), so the claim is checked rather than asserted: the named file when it is there, the
        // folder it went to otherwise. A dialog that says "exported" over a write that failed would
        // be worse than no dialog at all.
        const QString suffix = VLayoutExporter::ExportFormatSuffix(m_dialogSaveLayout->Format());
        const QString fileName = m_dialogSaveLayout->FileName() + suffix;
        const QFileInfo exported(m_dialogSaveLayout->Path() + '/'_L1 + fileName);
        if (exported.exists())
        {
            QMessageBox::information(this, tr("Detail mode"),
                                     tr("The file \"%1\" was successfully exported to:\n%2")
                                         .arg(fileName, exported.absolutePath()),
                                     QMessageBox::Ok);
        }
        else if (exported.dir().exists())
        {
            QMessageBox::information(this, tr("Detail mode"),
                                     tr("The files were successfully exported to:\n%1")
                                         .arg(exported.absolutePath()),
                                     QMessageBox::Ok);
        }
        m_dialogSaveLayout.clear();
    }
    catch (const VException &e)
    {
        m_dialogSaveLayout.clear();
        qCritical("%s\n\n%s\n\n%s", qUtf8Printable(tr("Export error.")), qUtf8Printable(e.ErrorMessage()),
                  qUtf8Printable(e.DetailedInformation()));
        return;
    }
}



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionExportRecipe_triggered()
{
    QString const filters(tr("Recipe files") + QStringLiteral("(*.vpr)"));
    QString const fileName = QFileDialog::getSaveFileName(
        this, tr("Export recipe"), QDir::homePath() + '/' + tr("recipe") + QStringLiteral(".vpr"), filters, nullptr,
        VAbstractApplication::VApp()->NativeFileDialog());
    if (fileName.isEmpty())
    {
        return;
    }

    try
    {
        VPatternRecipe recipe(doc);
        QString error;
        if (not recipe.SaveDocument(fileName, error))
        {
            qCWarning(vMainWindow, "%s", qUtf8Printable(tr("Could not save recipe. %1").arg(error)));
        }
    }
    catch (const VExceptionInvalidHistory &e)
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Could not create recipe file. %1").arg(e.ErrorMessage())));
    }
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::DoExport(const VCommandLinePtr &expParams) -> bool
{
    if (not expParams->IsExportOnlyDetails())
    {
        // The console layout export used to place the pieces with the legacy nesting engine and write a
        // plan de coupe. That engine left with the Layout mode, and a cutting plan is nested and exported
        // by the Export mode now. Say so rather than exit without a word.
        qCCritical(vMainWindow, "%s",
                   qUtf8Printable(tr("The console layout export is gone with the Layout mode. Use "
                                     "--exportOnlyDetails, or the Export mode, to export a cutting plan.")));
        QCoreApplication::exit(V_EX_DATAERR);
        return false;
    }

    QVector<DetailForLayout> details;
    if (not VAbstractValApplication::VApp()->getOpeningPattern())
    {
        const QHash<quint32, VPiece> *allDetails = pattern->DataPieces();
        if (allDetails->isEmpty())
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("You can't export empty scene.")));
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }

        details = SortDetailsForLayout(allDetails, pattern, expParams->OptExportSuchDetails());

        if (details.isEmpty())
        {
            qCCritical(vMainWindow, "%s",
                       qUtf8Printable(tr("You can't export empty scene. Please, "
                                         "include at least one detail in layout.")));
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }
    }
    QVector<VLayoutPiece> const listDetails = PrepareDetailsForLayout(details, doc);

    try
    {
        m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(
            new DialogSaveLayout(1, expParams->OptBaseName(), this));
        m_dialogSaveLayout->SetDestinationPath(expParams->OptDestinationPath());
        m_dialogSaveLayout->SelectFormat(static_cast<LayoutExportFormats>(expParams->OptExportType()));
        m_dialogSaveLayout->SetBinaryDXFFormat(expParams->IsBinaryDXF());
        m_dialogSaveLayout->SetDxfCompatibility(
            static_cast<DXFApparelCompatibility>(expParams->DXFApparelCompatibilityType()));
        m_dialogSaveLayout->SetShowGrainline(!expParams->IsNoGrainline());
        m_dialogSaveLayout->SetTextAsPaths(expParams->IsTextAsPaths());
        m_dialogSaveLayout->SetXScale(expParams->ExportXScale());
        m_dialogSaveLayout->SetYScale(expParams->ExportYScale());

        if (static_cast<LayoutExportFormats>(expParams->OptExportType()) == LayoutExportFormats::PDFTiled)
        {
            m_dialogSaveLayout->SetTiledExportMode(true);
            m_dialogSaveLayout->SetTiledMargins(expParams->TiledPageMargins());
            m_dialogSaveLayout->SetTiledPageFormat(expParams->OptTiledPaperSize());
            m_dialogSaveLayout->SetTiledPageOrientation(expParams->OptTiledPageOrientation());
        }

        ExportData(listDetails);
        m_dialogSaveLayout.clear();
    }
    catch (const VException &e)
    {
        m_dialogSaveLayout.clear();
        qCCritical(vMainWindow, "%s\n\n%s", qUtf8Printable(tr("Export error.")), qUtf8Printable(e.ErrorMessage()));
        QCoreApplication::exit(V_EX_DATAERR);
        return false;
    }



    return true;
}
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DoFMExport process export final measurements
 * @param expParams command line options
 * @return true if succesfull
 */
auto MainWindow::DoFMExport(const VCommandLinePtr &expParams) -> bool
{
    QString filePath = expParams->OptExportFMTo();

    if (filePath.isEmpty())
    {
        qCCritical(vMainWindow, "%s\n\n%s", qUtf8Printable(tr("Export final measurements error.")),
                   qUtf8Printable(tr("Destination path is empty.")));
        QCoreApplication::exit(V_EX_DATAERR);
        return false;
    }

    if (QFileInfo const info(filePath); info.isRelative())
    {
        filePath = QDir::currentPath() + '/'_L1 + filePath;
    }

    const QString codecName = expParams->OptCSVCodecName();
    int mib = QTextCodec::codecForLocale()->mibEnum();
    if (not codecName.isEmpty())
    {
        if (QTextCodec  const*codec = QTextCodec::codecForName(codecName.toLatin1()))
        {
            mib = codec->mibEnum();
        }
    }

    QChar separator = expParams->OptCSVSeparator();
    if (separator.isNull())
    {
        separator = VCommonSettings::GetDefCSVSeparator();
    }

    return ExportFMeasurementsToCSVData(filePath, expParams->IsCSVWithHeader(), mib, separator);
}
//---------------------------------------------------------------------------------------------------------------------
// Répond à une seule question : un <draw> inséré à chaud dans un document vivant, avec des identifiants neufs,
// est-il relu correctement par un nouveau parse ? Rien ici n'appartient au produit.
namespace
{
const QString probeSuffix = QStringLiteral("__PROBE");

// le produit fera : lire la pièce, la refuser si elle n'a pas sa place, la préparer, puis **pousser la commande**
// — et elle mesure les deux choses qu'aucune relecture ne prouve : que la formule du témoin suit ses points, et
// qu'un seul Ctrl+Z remet le document exactement dans l'état d'avant.
namespace
{
} // namespace

} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ProcessCMD()
{
    const VCommandLinePtr cmd = VApplication::CommandLine();
    auto args = cmd->OptInputFileNames();

    isNoScaling = cmd->IsNoScalingEnabled();

    if (VApplication::IsGUIMode())
    {
        ReopenFilesAfterCrash(args);

        for (auto &arg : args)
        {
            LoadPattern(arg);
        }
    }
    else
    {
        if (args.size() != 1)
        {
            qCritical() << tr("Please, provide one input file.");
            QCoreApplication::exit(V_EX_NOINPUT);
            return;
        }

        VAbstractValApplication::VApp()->SetUserMaterials(cmd->OptUserMaterials());

        if (const bool loaded = LoadPattern(args.constFirst(), cmd->OptMeasurePath()); not loaded)
        {
            return; // process only one input file
        }

        bool aSetted = true;
        bool bSetted = true;
        bool cSetted = true;
        if (cmd->IsSetDimensionA())
        {
            aSetted = SetDimensionA(cmd->OptDimensionA());
        }

        if (cmd->IsSetDimensionB())
        {
            bSetted = SetDimensionB(cmd->OptDimensionB());
        }

        if (cmd->IsSetDimensionC())
        {
            cSetted = SetDimensionB(cmd->OptDimensionC());
        }

        if (not(aSetted && bSetted && cSetted))
        {
            QCoreApplication::exit(V_EX_DATAERR);
            return;
        }

        if (not cmd->IsTestModeEnabled())
        {
            if (cmd->IsExportEnabled() && not DoExport(cmd))
            {
                return;
            }

            if (cmd->IsExportFMEnabled() && not DoFMExport(cmd))
            {
                return;
            }
        }

        QCoreApplication::exit(V_EX_OK); // close program after processing in console mode
    }
}
