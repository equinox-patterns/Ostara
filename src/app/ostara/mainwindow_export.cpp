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
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 17/10/2026.
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
void MainWindow::ExportDetailsAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionDetailExportAs->setChecked(false); });

    QVector<DetailForLayout> const detailsInLayout = SortDetailsForLayout(pattern->DataPieces(), pattern);

    if (detailsInLayout.isEmpty())
    {
        // "Layout mode" no longer exists: this is the Details export, and the title has to name it.
        QMessageBox::information(this, tr("Detail mode"),
                                 tr("There is no piece to export. Switch a piece on in the Details dock, then try "
                                    "again."),
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
        QMessageBox::warning(this, tr("Export details"),
                             tr("Ostara could not prepare the pieces for export:\n\n%1\n\nCheck that every piece "
                                "has a main path and a seam allowance, then try again.")
                                 .arg(e.ErrorMessage()),
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
        //
        // ⚠️ **Et la compatibilité DXF, pour la même raison** : le mode Clo3D/Style3D taille sans
        // valeurs de couture, et c'est la géométrie des pièces qui change — pas un calque. La
        // préparation est donc refaite quand il est choisi, même si le contour principal, lui, ne
        // l'est pas. Voir PrepareDetailsForLayout.
        const bool showMainPath = m_dialogSaveLayout->IsShowMainPath();
        const bool noSeamAllowance = m_dialogSaveLayout->DxfCompatibility() == DXFApparelCompatibility::CLO3D;
        if (showMainPath || noSeamAllowance)
        {
            listDetails = PrepareDetailsForLayout(detailsInLayout, doc, false, showMainPath, noSeamAllowance);
        }

        ExportData(listDetails);

        // ExportData() reports nothing about what it wrote: the exporter returns void, and a tiled PDF writes one
        // file per page. The claim is therefore checked on the folder — at least one non-empty file whose name
        // starts with the requested base name. It used to be enough that the folder existed, which announced
        // "exported" over a write that had failed (a read-only key, a protected share, a folder needing rights).
        // A dialog that says "exported" over a write that failed is worse than no dialog at all.
        const QString baseName = m_dialogSaveLayout->FileName();
        const QDir folder(m_dialogSaveLayout->Path());

        QStringList written;
        const QFileInfoList candidates = folder.entryInfoList(QStringList{baseName + '*'}, QDir::Files);
        for (const QFileInfo &candidate : candidates)
        {
            if (candidate.size() > 0)
            {
                written << candidate.fileName();
            }
        }

        if (written.isEmpty())
        {
            QMessageBox::warning(this, tr("Detail mode"),
                                 tr("Nothing was written to this folder:\n%1\n\nIt does not allow writing — a "
                                    "read-only key, a protected share, a folder that asks for rights. Choose "
                                    "another folder.")
                                     .arg(folder.absolutePath()),
                                 QMessageBox::Ok);
        }
        else if (written.size() == 1)
        {
            QMessageBox::information(this, tr("Detail mode"),
                                     tr("The file \"%1\" was successfully exported to:\n%2")
                                         .arg(written.constFirst(), folder.absolutePath()),
                                     QMessageBox::Ok);
        }
        else
        {
            QMessageBox::information(this, tr("Detail mode"),
                                     tr("The files were successfully exported to:\n%1").arg(folder.absolutePath()),
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
    QVector<VLayoutPiece> listDetails = PrepareDetailsForLayout(details, doc);

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

        // ⚠️ Même règle qu'en mode Details, et au même endroit du raisonnement : le mode Clo3D/Style3D
        // ne s'écrit pas, il se TAILLE. Les pièces sont donc relaid-out **sans leurs valeurs de
        // couture** dès que la compatibilité est choisie — la géométrie d'un `VLayoutPiece` ne se
        // reprend pas après coup — et le patron, lui, n'est pas touché.
        if (m_dialogSaveLayout->DxfCompatibility() == DXFApparelCompatibility::CLO3D)
        {
            listDetails = PrepareDetailsForLayout(details, doc, false, false, true);
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
            // ⚠️ C, pas B. La valeur de C était passée à SetDimensionB, et MainWindow::SetDimensionC
            // n'était appelé nulle part : --dimensionC n'avait jamais pu fonctionner. Sur une table à
            // trois axes, la valeur de C écrasait celle de B **en silence** dès qu'elle figurait aussi
            // dans la liste de B ; sinon elle était refusée, et le lot s'arrêtait.
            cSetted = SetDimensionC(cmd->OptDimensionC());
        }

        if (not(aSetted && bSetted && cSetted))
        {
            QCoreApplication::exit(V_EX_DATAERR);
            return;
        }

        // SetDimensionX() only starts the size-switch debounce: the measurements are recomputed and the pattern
        // re-parsed by GradationChanged(), one event-loop turn later. Nothing turns the loop before DoExport()
        // below, so without this flush the export would carry the geometry of the size stored in the file — the
        // flag would be accepted and change nothing.
        if (m_gradation->isActive())
        {
            GradationChanged();
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

            // Le métrage par lot : un plan de coupe sur chaque taille complète, pour chaque laize.
            // ⚠️ Aucun fichier d'export n'est écrit, et le patron n'est jamais modifié.
            if (cmd->IsMetrageEnabled() && not DoMetrageBatch(cmd))
            {
                QCoreApplication::exit(V_EX_DATAERR);
                return;
            }
        }

        QCoreApplication::exit(V_EX_OK); // close program after processing in console mode
    }
}
