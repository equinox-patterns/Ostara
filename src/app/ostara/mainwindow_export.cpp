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
#include "../vlayout/dialogs/dialoglayoutscale.h"
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
#include "../vtools/dialogs/dialogtoolbox.h" // TEMPORARY — sonde des objets à nommer, à retirer
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "core/vapplication.h"
#include "core/opiecedocument.h"
#include "core/opiecebuilder.h"
#include "core/opiecelibrary.h"
#include "core/opieceimport.h" // TEMPORARY — sonde de l'import, à retirer
#include "core/opieceinsert.h" // TEMPORARY — sonde de l'insertion, à retirer
#include "core/opiecemark.h"   // TEMPORARY — sonde du marquage, à retirer
#include "core/opiecename.h"    // TEMPORARY — sonde des objets à nommer, à retirer
#include "core/opieceremap.h" // TEMPORARY — la sonde d'insertion éprouve la règle du produit
#include "dialogs/dialogincrements.h" // TEMPORARY — sonde de la table des incréments, à retirer
#include "dialogs/dialognewpattern.h" // TEMPORARY — sonde de la dialog « nouvelle pièce », à retirer
#include "dialogs/opiecelibrarydialog.h" // TEMPORARY — sonde de la fenêtre de bibliothèque, à retirer
#include "dialogs/opieceimportdialog.h" // TEMPORARY — sonde de la dialog d'import, à retirer
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
void MainWindow::ExportLayoutAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionLayoutExportAs->setChecked(false); });

    if (m_layoutSettings->IsLayoutStale() && VPrintLayout::ContinueIfLayoutStale(this) == QMessageBox::No)
    {
        return;
    }

    try
    {
        m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(new DialogSaveLayout(
            static_cast<int>(m_layoutSettings->LayoutScenes().size()), Draw::Layout, FileName(), this));
        m_dialogSaveLayout->SetBoundaryTogetherWithNotches(m_layoutSettings->IsBoundaryTogetherWithNotches());

        if (m_dialogSaveLayout->exec() == QDialog::Rejected)
        {
            m_dialogSaveLayout.clear();
            return;
        }

        ExportData(QVector<VLayoutPiece>());
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
            QSharedPointer<DialogSaveLayout>(new DialogSaveLayout(1, Draw::Modeling, FileName(), this));

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
void MainWindow::on_actionOpenPuzzle_triggered()
{
    QStringList arguments;
    if (isNoScaling)
    {
        arguments.append(QStringLiteral("--") + LONG_OPTION_NO_HDPI_SCALING);
    }

    const QString path = VApplication::PuzzleFilePath();
    qDebug("Opening Puzzle: path = %s.", qUtf8Printable(path));
    VApplication::StartDetachedProcess(path, arguments);
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::on_actionCreateManualLayout_triggered()
{
    QTemporaryFile rldFile(QDir::tempPath() + "/puzzle.rld.XXXXXX"_L1);
    if (rldFile.open())
    {
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

        DialogLayoutScale layoutScale(false, this);
        layoutScale.SetXScale(1);
        layoutScale.SetYScale(1);

        if (int const res = layoutScale.exec(); res == QDialog::Rejected)
        {
            return;
        }

        VLayoutExporter exporter;
        exporter.SetFileName(rldFile.fileName());
        exporter.SetXScale(layoutScale.GetXScale());
        exporter.SetYScale(layoutScale.GetYScale());
        exporter.ExportToRLD(listDetails);

        QStringList arguments{"-r", rldFile.fileName()};
        if (isNoScaling)
        {
            arguments.append(QStringLiteral("--") + LONG_OPTION_NO_HDPI_SCALING);
        }

        rldFile.setAutoRemove(false);

        const QString path = VApplication::PuzzleFilePath();
        qDebug("Opening Puzzle: path = %s.", qUtf8Printable(path));
        VApplication::StartDetachedProcess(path, arguments);
    }
    else
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Unable to prepare raw layout data.")));
    }
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::on_actionUpdateManualLayout_triggered()
{
    const QString filter(tr("Manual layout files") + " (*.vlt)"_L1);

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

    // Use standard path to manual layouts
    const QString path = settings->GetPathManualLayouts();

    const QString filePath = QFileDialog::getOpenFileName(this, tr("Select manual layout"), path, filter, nullptr);

    if (filePath.isEmpty())
    {
        return;
    }

    settings->SetPathManualLayouts(QFileInfo(filePath).absolutePath());

    QTemporaryFile rldFile(QDir::tempPath() + "/puzzle.rld.XXXXXX"_L1);
    rldFile.setAutoRemove(false);
    if (rldFile.open())
    {
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

        DialogLayoutScale layoutScale(false, this);
        layoutScale.SetXScale(1);
        layoutScale.SetYScale(1);
        layoutScale.exec();

        VLayoutExporter exporter;
        exporter.SetFileName(rldFile.fileName());
        exporter.SetXScale(layoutScale.GetXScale());
        exporter.SetYScale(layoutScale.GetYScale());
        exporter.ExportToRLD(listDetails);

        QStringList arguments{filePath, "-r", rldFile.fileName()};
        if (isNoScaling)
        {
            arguments.append("--"_L1 + LONG_OPTION_NO_HDPI_SCALING);
        }

        rldFile.setAutoRemove(false);

        const QString puzzlePath = VApplication::PuzzleFilePath();
        qDebug("Opening Puzzle: path = %s.", qUtf8Printable(puzzlePath));
        VApplication::StartDetachedProcess(puzzlePath, arguments);
    }
    else
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Unable to prepare raw layout data.")));
    }
}
//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::DoExport(const VCommandLinePtr &expParams) -> bool
{
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
    listDetails = PrepareDetailsForLayout(details, doc);

    if (const bool exportOnlyDetails = expParams->IsExportOnlyDetails(); exportOnlyDetails)
    {
        try
        {
            m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(
                new DialogSaveLayout(1, Draw::Modeling, expParams->OptBaseName(), this));
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
    }
    else
    {
        auto settings = expParams->DefaultGenerator();
        settings->SetTextAsPaths(expParams->IsTextAsPaths());

        if (GenerateLayout(*settings.get()))
        {
            try
            {
                m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(
                    new DialogSaveLayout(static_cast<int>(m_layoutSettings->LayoutScenes().size()), Draw::Layout,
                                         expParams->OptBaseName(), this));
                m_dialogSaveLayout->SetDestinationPath(expParams->OptDestinationPath());
                m_dialogSaveLayout->SelectFormat(static_cast<LayoutExportFormats>(expParams->OptExportType()));
                m_dialogSaveLayout->SetBinaryDXFFormat(expParams->IsBinaryDXF());
                m_dialogSaveLayout->SetDxfCompatibility(
                    static_cast<DXFApparelCompatibility>(expParams->DXFApparelCompatibilityType()));
                m_dialogSaveLayout->SetShowGrainline(!expParams->IsNoGrainline());
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
                qCCritical(vMainWindow, "%s\n\n%s", qUtf8Printable(tr("Export error.")),
                           qUtf8Printable(e.ErrorMessage()));
                QCoreApplication::exit(V_EX_DATAERR);
                return false;
            }
        }
        else
        {
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }
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
// TEMPORARY — sonde de faisabilité pour la bibliothèque de pièces. À RETIRER.
// Répond à une seule question : un <draw> inséré à chaud dans un document vivant, avec des identifiants neufs,
// est-il relu correctement par un nouveau parse ? Rien ici n'appartient au produit.
namespace
{
const QString probeSuffix = QStringLiteral("__PROBE");

void ProbeCollectDeclaredIds(const QDomElement &element, QSet<QString> &declared)
{
    if (element.hasAttribute(QStringLiteral("id")))
    {
        declared.insert(element.attribute(QStringLiteral("id")));
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        ProbeCollectDeclaredIds(child, declared);
    }
}

// Marque chaque nom déclaré dans la copie — les points, lignes et courbes — du marqueur de pièce.
// C'est CE renommage qui doit faire disparaître la divergence entre le DOM et le conteneur.
void ProbeMarkNames(QDomElement &element, int &renamed)
{
    if (element.hasAttribute(AttrName) && element.tagName() != VAbstractPattern::TagDraw
        && element.tagName() != VAbstractPattern::TagDetail)
    {
        const QString name = element.attribute(AttrName);
        if (not name.isEmpty())
        {
            element.setAttribute(AttrName, name + QStringLiteral("__P1"));
            ++renamed;
        }
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        ProbeMarkNames(child, renamed);
    }
}

// Compare le bloc copié à son original, attribut par attribut, et dit ce qui a changé. Ce qui a changé DOIT
// être un emplacement d'identifiant de la règle ; tout le reste est une anomalie, et c'est là que se verrait un
// indice de pliage (departure, place, pleat, end) pris pour un id.
void ProbeCompareBlocks(const QDomElement &before,
                        const QDomElement &after,
                        QMap<QString, int> &perSlot,
                        QStringList &unexpected)
{
    QList<QPair<QDomElement, QDomElement>> pending;
    pending.append({before, after});

    while (not pending.isEmpty())
    {
        const QPair<QDomElement, QDomElement> pair = pending.takeLast();
        if (pair.first.tagName() != pair.second.tagName())
        {
            unexpected.append(QStringLiteral("structure : <%1> face à <%2>")
                                  .arg(pair.first.tagName(), pair.second.tagName()));
            continue;
        }

        const QDomNamedNodeMap attributes = pair.first.attributes();
        for (int i = 0; i < attributes.count(); ++i)
        {
            const QDomAttr attribute = attributes.item(i).toAttr();
            const QString name = attribute.name();
            const QString was = attribute.value();
            const QString now = pair.second.attribute(name);
            if (was == now || name == AttrName)
            {
                continue;
            }

            const QString slot = pair.first.tagName() + QStringLiteral("/@") + name;
            if (name == QStringLiteral("nodeIds"))
            {
                // Une seule valeur par id de la liste, comme la règle les compte.
                const QStringList ones = was.split(QLatin1Char(','));
                const QStringList others = now.split(QLatin1Char(','));
                for (int j = 0; j < qMin(ones.size(), others.size()); ++j)
                {
                    if (ones.at(j) != others.at(j))
                    {
                        ++perSlot[slot];
                    }
                }
                continue;
            }

            if (not OPieceRemap::IdSlots().contains(slot))
            {
                unexpected.append(QStringLiteral("%1 : %2 -> %3").arg(slot, was, now));
                continue;
            }
            ++perSlot[slot];
        }

        // Le texte ne se compare que sur les éléments de valeur : QDomElement::text() rend le texte de TOUS les
        // descendants, donc un conteneur « change » dès qu'un enfant change — ce n'est pas un emplacement.
        if (pair.first.firstChildElement().isNull() && pair.second.firstChildElement().isNull())
        {
            const QString textWas = pair.first.text().trimmed();
            const QString textNow = pair.second.text().trimmed();
            if (textWas != textNow)
            {
                const QString element = pair.first.tagName();
                if (OPieceRemap::IdElements().contains(element))
                {
                    ++perSlot[QStringLiteral("<") + element + QStringLiteral(">")];
                }
                else
                {
                    unexpected.append(QStringLiteral("<%1> : %2 -> %3").arg(element, textWas, textNow));
                }
            }
        }

        QDomElement one = pair.first.firstChildElement();
        QDomElement other = pair.second.firstChildElement();
        for (; not one.isNull() && not other.isNull(); one = one.nextSiblingElement(), other = other.nextSiblingElement())
        {
            pending.append({one, other});
        }
    }
}

// Seconde question, celle du produit : un bloc se relit-il comme une pièce, et le fichier obtenu passe-t-il
// son propre schéma ? Rien ici n'appartient au produit.
// TEMPORARY — sonde du marquage, à retirer à la fin du chantier.
// Aplatit un élément en une ligne par chemin : ses attributs triés, et son texte quand il n'a pas d'enfant.
// Deux arbres issus du même document donnent ainsi deux listes comparables ligne à ligne, dans l'ordre.
void ProbeFlatten(const QDomElement &element, const QString &path, QStringList &lines)
{
    QStringList attributes;
    const QDomNamedNodeMap attrs = element.attributes();
    for (int i = 0; i < attrs.count(); ++i)
    {
        const QDomAttr attribute = attrs.item(i).toAttr();
        attributes.append(attribute.name() + QStringLiteral("=") + attribute.value());
    }
    attributes.sort();
    lines.append(path + QStringLiteral("|") + attributes.join(QStringLiteral(" ")));

    if (element.firstChildElement().isNull())
    {
        lines.append(path + QStringLiteral("|text=") + element.text());
    }

    int index = 0;
    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        ProbeFlatten(child, path + QStringLiteral("/") + child.tagName() + QStringLiteral("[") + QString::number(index)
                                + QStringLiteral("]"),
                     lines);
        ++index;
    }
}

// TEMPORARY — les valeurs qui portent encore une marque après la découpe, et les clés de la table qu'elles
// contiennent : de quoi dire si la table ignorait le nom ou si la réécriture l'a manqué.
void ProbeResidues(const QDomElement &element, const QString &path, const QHash<QString, QString> &offered)
{
    const QDomNamedNodeMap attrs = element.attributes();
    for (int i = 0; i < attrs.count(); ++i)
    {
        const QDomAttr attribute = attrs.item(i).toAttr();
        if (not attribute.value().contains(QStringLiteral("__P")))
        {
            continue;
        }

        QStringList contained;
        for (auto it = offered.constBegin(); it != offered.constEnd(); ++it)
        {
            if (not attribute.value().contains(it.key()))
            {
                continue;
            }

            contained.append(it.key() + QStringLiteral(" -> ") + it.value());
        }

        qInfo() << "MARK:     RESIDU" << path + QStringLiteral("/@") + attribute.name() << "=" << attribute.value()
                << "| cles contenues =" << contained;
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        ProbeResidues(child, path + QStringLiteral("/") + child.tagName(), offered);
    }
}

// TEMPORARY — le va-et-vient du marquage : tous les noms de la pièce reçoivent `__P<n>`, puis le retirent, et le
// DOM doit revenir **exactement** à ce qu'il était — formules comprises, car c'est là que le marquage peut casser
// une pièce en silence. Rien ici n'appartient au produit : c'est la mesure du §4 de la spec.
void ProbeMarkRoundTrip(OPieceDocument &piece, VContainer *pattern)
{
    if (pattern == nullptr)
    {
        return;
    }

    QDomElement envelope = piece.documentElement();

    QStringList before;
    ProbeFlatten(envelope, envelope.tagName(), before);
    const auto marksBefore = static_cast<int>(piece.toString().count(QStringLiteral("__P")));

    QSet<QString> taken;
    const QHash<QString, QSharedPointer<VInternalVariable>> *variables = pattern->DataVariables();
    if (variables != nullptr)
    {
        for (auto it = variables->constBegin(); it != variables->constEnd(); ++it)
        {
            taken.insert(it.key());
        }
    }

    const int number = OPieceMark::HighestNumber(envelope) + 1;
    const QHash<QString, QString> table = OPieceMark::TableFor(envelope, taken, number);
    qInfo() << "MARK:" << piece.PieceName() << "| noms declares =" << OPieceMark::DeclaredNames(envelope).size()
            << "| noms du conteneur =" << taken.size() << "| numero tire du fichier =" << number
            << "| conflit =" << (table.isEmpty() ? QStringLiteral("non") : QStringLiteral("oui"))
            << "| table =" << table.size() << "| marques avant =" << marksBefore;

    if (table.isEmpty())
    {
        qInfo() << "MARK:   rien a marquer, la piece reste telle quelle";
        return;
    }

    const OPieceMark::Report marked = OPieceMark::Rename(envelope, table);
    const QStringList markedNames = OPieceMark::DeclaredNames(envelope);
    int carrying = 0;
    for (const QString &name : markedNames)
    {
        if (OPieceMark::Number(name) == number)
        {
            ++carrying;
        }
    }
    qInfo() << "MARK:   marque -> table rencontree =" << marked.names << "| emplacements =" << marked.written
            << "| noms apres =" << markedNames.size() << "| portant la marque =" << carrying
            << "| marques ecrites =" << piece.toString().count(QStringLiteral("__P"));

    // TEMPORARY — ce que la table de découpe offrira : les noms marques du DOM, tels qu'ils sont maintenant.
    QHash<QString, QString> offered;
    for (const QString &name : OPieceMark::DeclaredNames(envelope))
    {
        if (OPieceMark::Number(name) > 0)
        {
            offered.insert(name, OPieceMark::Unmarked(name));
        }
    }

    const OPieceMark::Report stripped = OPieceMark::Unmark(envelope);
    const auto marksAfter = static_cast<int>(piece.toString().count(QStringLiteral("__P")));
    qInfo() << "MARK:   demarque -> table rencontree =" << stripped.names << "| emplacements =" << stripped.written
            << "| marques restantes =" << marksAfter
            << "| plus grand numero residuel =" << OPieceMark::HighestNumber(envelope);

    QStringList after;
    ProbeFlatten(envelope, envelope.tagName(), after);

    int differences = 0;
    QStringList samples;
    for (int i = 0; i < qMax(before.size(), after.size()); ++i)
    {
        if (before.value(i) == after.value(i))
        {
            continue;
        }

        ++differences;
        if (samples.size() < 10)
        {
            samples.append(QStringLiteral("AVANT ") + before.value(i) + QStringLiteral("  /  APRES ")
                           + after.value(i));
        }
    }

    qInfo() << "MARK:   va-et-vient -> lignes =" << before.size() << "| ecarts =" << differences;
    for (const QString &sample : samples)
    {
        qInfo() << "MARK:     ECART" << sample;
    }

    ProbeResidues(envelope, envelope.tagName(), offered);
}

void ProbeBuildPiece(VPattern *doc, VContainer *pattern)
{
    const QDomElement root = doc->documentElement();

    QStringList blocks;
    for (QDomElement draw = root.firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        blocks.append(draw.attribute(VAbstractPattern::AttrName));
    }
    qInfo() << "PIECE-BUILD: blocs du patron =" << blocks;

    const auto countRows = [](const QDomElement &section) -> int
    {
        int rows = 0;
        for (QDomElement row = section.firstChildElement(); not row.isNull(); row = row.nextSiblingElement())
        {
            ++rows;
        }
        return rows;
    };

    for (const QString &blockName : blocks)
    {
        OPieceDocument piece;
        QString error;
        if (not OPieceBuilder::Build(doc, pattern, blockName, blockName, &piece, error))
        {
            qInfo() << "PIECE-BUILD:" << blockName << "-> ECHEC :" << error;
            continue;
        }

        const QDomElement draw = piece.DrawElement();
        const QList<OPieceDocument::Dependency> dependencies = piece.Dependencies();

        QStringList needed;
        for (const OPieceDocument::Dependency &dependency : dependencies)
        {
            needed.append(dependency.origin.isEmpty()
                              ? dependency.token
                              : dependency.token + QStringLiteral(" <- ") + dependency.origin);
        }

        qInfo() << "PIECE-BUILD:" << blockName << "| identifiant =" << piece.LibraryId()
                << "| unite =" << piece.Unit()
                << "| calques =" << draw.firstChildElement(QStringLiteral("groups")).childNodes().count()
                << "| groupes de calques ="
                << draw.firstChildElement(QStringLiteral("layerGroups")).childNodes().count()
                << "| increments =" << countRows(piece.Increments())
                << "| calculs preliminaires =" << countRows(piece.PreviewCalculations())
                << "| dependances =" << dependencies.size() << needed;

        const QString filePath = QStringLiteral("/tmp/probe_piece_%1.opiece")
                                     .arg(QString(blockName).replace(QChar(u' '), QChar(u'_')));
        QString saveError;
        if (not OPieceLibrary::Save(&piece, filePath, &saveError))
        {
            qInfo() << "PIECE-BUILD:   ECRITURE REFUSEE ->" << saveError;
            continue;
        }
        qInfo() << "PIECE-BUILD:   ecrit et valide ->" << filePath << QFileInfo(filePath).size() << "octets";

        OPieceDocument reloaded;
        QString loadError;
        if (OPieceLibrary::Load(filePath, &reloaded, &loadError))
        {
            qInfo() << "PIECE-BUILD:   relu -> nom =" << reloaded.PieceName()
                    << "| identifiant =" << reloaded.LibraryId()
                    << "| increments =" << countRows(reloaded.Increments())
                    << "| dependances =" << reloaded.Dependencies().size();
        }
        else
        {
            qInfo() << "PIECE-BUILD:   RELECTURE ECHOUEE ->" << loadError;
        }

        ProbeMarkRoundTrip(piece, pattern); // TEMPORARY — sonde du marquage
    }
}

// TEMPORARY — sonde des groupes d'incréments, à retirer à la fin du chantier.
// Exerce le modèle de bout en bout : création de la liste à sa place, rangement d'une ligne, pli, lecture, et
// surtout l'enregistrement — c'est le schéma qui dit si l'ordre du document tient.
void ProbeIncrementGroups(VPattern *doc, VContainer *pattern)
{
    if (pattern == nullptr || doc == nullptr)
    {
        qInfo() << "GROUPS: no document loaded";
        return;
    }

    const auto positionOf = [doc](const QString &tag) -> int {
        const QDomElement root = doc->documentElement();
        int index = 0;
        for (QDomNode node = root.firstChild(); not node.isNull(); node = node.nextSibling())
        {
            if (node.toElement().tagName() == tag)
            {
                return index;
            }
            ++index;
        }
        return -1;
    };

    // Deux vraies lignes de la table des incréments, prises dans le fichier.
    const QDomElement table = doc->documentElement().firstChildElement(VAbstractPattern::TagIncrements);
    QStringList rowNames;
    for (QDomNode node = table.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (const QDomElement row = node.toElement();
            not row.isNull() && row.tagName() == VAbstractPattern::TagIncrement)
        {
            rowNames << row.attribute(VAbstractPattern::AttrName);
        }
    }
    qInfo() << "GROUPS: lignes de la table =" << rowNames.size() << "| deux premieres =" << rowNames.value(0)
            << rowNames.value(1);

    // 1. Une lecture ne crée rien.
    const bool noListBefore = doc->documentElement().firstChildElement(VAbstractPattern::TagIncrementGroups).isNull();
    const qsizetype emptyRead = doc->GetIncrementGroups().size();
    const bool stillNoList =
        doc->documentElement().firstChildElement(VAbstractPattern::TagIncrementGroups).isNull();
    qInfo() << "GROUPS: 1. lecture seule -> avant =" << noListBefore << "| lue =" << emptyRead
            << "| toujours absente =" << stillNoList;

    // La même lecture sur l'autre table, et rien ne doit apparaître non plus.
    const qsizetype emptyReadPreview = doc->GetIncrementGroups(true).size();
    const bool noPreviewList =
        doc->documentElement().firstChildElement(VAbstractPattern::TagPreviewCalculationGroups).isNull();
    qInfo() << "GROUPS: 1b. lecture calculs preliminaires -> lue =" << emptyReadPreview
            << "| liste absente =" << noPreviewList;

    // 2. Création de la liste, à sa place.
    const quint32 groupId = pattern->getNextId();
    const quint32 inserted = doc->InsertIncrementGroup(groupId, QStringLiteral("Buste"));
    qInfo() << "GROUPS: 2. groupe cree -> id =" << groupId << "| rendu =" << inserted
            << "| increments a" << positionOf(VAbstractPattern::TagIncrements) << "| incrementGroups a"
            << positionOf(VAbstractPattern::TagIncrementGroups) << "| previewCalculations a"
            << positionOf(VAbstractPattern::TagPreviewCalculations);

    // 3. Ranger deux lignes, et lire ce rangement.
    if (rowNames.size() >= 2)
    {
        doc->SetIncrementGroup(rowNames.at(0), groupId);
        doc->SetIncrementGroup(rowNames.at(1), groupId);
    }
    const quint32 readBack = rowNames.isEmpty() ? NULL_ID : doc->GetIncrementGroup(rowNames.value(0));
    const quint32 loose = rowNames.size() < 3 ? NULL_ID : doc->GetIncrementGroup(rowNames.at(2));
    qInfo() << "GROUPS: 3. rangement -> relu =" << readBack << "| attendu =" << groupId
            << "| ligne non rangee =" << loose;

    // 4. La table compte : le meme nom lu dans l'autre table n'est pas cette ligne-la.
    const quint32 crossTable = rowNames.isEmpty() ? NULL_ID : doc->GetIncrementGroup(rowNames.value(0), true);
    qInfo() << "GROUPS: 4. meme nom dans l'autre table ->" << crossTable << "(NULL_ID attendu)";

    // 5. Le pli : ecrit quand il est plie, retire sinon.
    const QString groupsTag = VAbstractPattern::TagIncrementGroups;
    const auto collapsedAttr = [doc, &groupsTag, groupId]() -> QString {
        const QDomElement list = doc->documentElement().firstChildElement(groupsTag);
        for (QDomNode node = list.firstChild(); not node.isNull(); node = node.nextSibling())
        {
            if (const QDomElement group = node.toElement();
                not group.isNull() && group.tagName() == VAbstractPattern::TagIncrementGroup
                && group.attribute(VAbstractPattern::AttrId).toUInt() == groupId)
            {
                return group.attribute(VAbstractPattern::AttrCollapsed);
            }
        }
        return {};
    };
    doc->SetIncrementGroupCollapsed(groupId, true);
    const bool folded = doc->GetIncrementGroupCollapsed(groupId);
    const QString foldedAttr = collapsedAttr();
    doc->SetIncrementGroupCollapsed(groupId, false);
    const bool unfolded = doc->GetIncrementGroupCollapsed(groupId);
    qInfo() << "GROUPS: 5. pli -> plie =" << folded << "attribut ecrit =" << foldedAttr << "| deplie ="
            << unfolded << "attribut restant =" << collapsedAttr();

    // 6. Ce que la liste rend.
    const QMap<quint32, QString> names = doc->IncrementGroupNames();
    const QMap<quint32, VIncrementGroupData> groups = doc->GetIncrementGroups();
    qInfo() << "GROUPS: 6. liste -> noms =" << names << "| groupes =" << groups.size() << "| nom du groupe ="
            << doc->GetIncrementGroupName(groupId);

    // 7. L'enregistrement : c'est lui qui dit si l'ordre du document tient.
    if (QString error; doc->SaveDocument(QStringLiteral("/tmp/probe_groups.ostara"), error))
    {
        qInfo() << "GROUPS: 7. ENREGISTREMENT OK -> /tmp/probe_groups.ostara";
    }
    else
    {
        qInfo() << "GROUPS: 7. ENREGISTREMENT REFUSE ->" << error;
    }

    // 8. Une ligne qu'on delie perd son attribut.
    if (not rowNames.isEmpty())
    {
        doc->SetIncrementGroup(rowNames.value(0), NULL_ID);
        qInfo() << "GROUPS: 8. ligne deliee -> relu =" << doc->GetIncrementGroup(rowNames.value(0))
                << "(NULL_ID attendu) | l'autre ligne toujours rangee ="
                << (rowNames.size() >= 2 ? doc->GetIncrementGroup(rowNames.at(1)) : NULL_ID);
    }

    // 9. Le groupe part, et sa liste avec lui puisqu'il etait le dernier.
    doc->RemoveIncrementGroup(groupId);
    qInfo() << "GROUPS: 9. groupe retire -> groupes =" << doc->GetIncrementGroups().size()
            << "| liste encore la ="
            << (not doc->documentElement().firstChildElement(VAbstractPattern::TagIncrementGroups).isNull());

    // 10. Et l'enregistrement tient toujours sans la liste.
    if (QString error; doc->SaveDocument(QStringLiteral("/tmp/probe_groups_empty.ostara"), error))
    {
        qInfo() << "GROUPS: 10. ENREGISTREMENT OK -> /tmp/probe_groups_empty.ostara";
    }
    else
    {
        qInfo() << "GROUPS: 10. ENREGISTREMENT REFUSE ->" << error;
    }

    // 11. Une ligne seule dans un groupe : la retirer emporte le groupe.
    const quint32 soloId = pattern->getNextId();
    doc->InsertIncrementGroup(soloId, QStringLiteral("Groupe d'une ligne"));
    doc->SetIncrementGroup(QStringLiteral("#Epaule"), soloId);
    const qsizetype beforeSolo = doc->GetIncrementGroups().size();
    doc->RemoveIncrement(QStringLiteral("#Epaule"));
    qInfo() << "GROUPS: 11. une ligne seule -> avant =" << beforeSolo
            << "| apres =" << doc->GetIncrementGroups().size() << "| groupe encore la ="
            << (not doc->GetIncrementGroupName(soloId).isEmpty());

    // 12. Deux lignes dans un groupe : la premiere partie, le groupe reste et la survivante le garde.
    const quint32 pairId = pattern->getNextId();
    doc->InsertIncrementGroup(pairId, QStringLiteral("Groupe de deux lignes"));
    doc->SetIncrementGroup(QStringLiteral("#TailleSansPinces_2"), pairId);
    doc->SetIncrementGroup(QStringLiteral("#Poitrine"), pairId);
    const qsizetype beforePair = doc->GetIncrementGroups().size();
    doc->RemoveIncrement(QStringLiteral("#TailleSansPinces_2"));
    const qsizetype afterFirst = doc->GetIncrementGroups().size();
    const bool survivorKeeps = doc->GetIncrementGroup(QStringLiteral("#Poitrine")) == pairId;
    doc->RemoveIncrement(QStringLiteral("#Poitrine"));
    qInfo() << "GROUPS: 12. deux lignes -> avant =" << beforePair << "| apres la premiere =" << afterFirst
            << "(le groupe tient) | la survivante garde son groupe =" << survivorKeeps
            << "| apres la seconde, groupe encore la ="
            << (not doc->GetIncrementGroupName(pairId).isEmpty());

    // 13. Une ligne qui quitte son groupe sans etre supprimee emporte le groupe aussi.
    const quint32 leftId = pattern->getNextId();
    doc->InsertIncrementGroup(leftId, QStringLiteral("Groupe delaisse"));
    doc->SetIncrementGroup(QStringLiteral("#Bassin"), leftId);
    const qsizetype beforeLeft = doc->GetIncrementGroups().size();
    doc->SetIncrementGroup(QStringLiteral("#Bassin"), NULL_ID);
    qInfo() << "GROUPS: 13. ligne delaissee -> avant =" << beforeLeft
            << "| apres =" << doc->GetIncrementGroups().size() << "| groupe encore la ="
            << (not doc->GetIncrementGroupName(leftId).isEmpty());

    // 14. Et le dernier groupe parti emporte la liste : il ne reste alors que le groupe 821 et sa ligne.
    doc->SetIncrementGroup(QStringLiteral("#Cou"), NULL_ID);
    qInfo() << "GROUPS: 14. plus aucun groupe -> groupes =" << doc->GetIncrementGroups().size()
            << "| liste encore la ="
            << (not doc->documentElement().firstChildElement(VAbstractPattern::TagIncrementGroups).isNull());
}

// TEMPORARY — sonde de la table des incréments, à retirer à la fin du chantier.
// Construit la dialogue sur le document chargé et lit ce que la table montre vraiment : les lignes de groupe, le
// masquage des membres d'un groupe plié, le caret, et l'état écrit dans le document.
void ProbeIncrementGroupsUi(VPattern *doc, VContainer *pattern)
{
    if (pattern == nullptr || doc == nullptr)
    {
        qInfo() << "GROUPS-UI: no document loaded";
        return;
    }

    DialogIncrements dialog(pattern, doc);

    auto *increments = dialog.findChild<QTableWidget *>(QStringLiteral("tableWidgetIncrement"));
    auto *preview = dialog.findChild<QTableWidget *>(QStringLiteral("tableWidgetPC"));

    if (increments == nullptr || preview == nullptr)
    {
        qInfo() << "GROUPS-UI: tables not found";
        return;
    }

    const auto dumpTable = [](const QString &label, QTableWidget *table) {
        qInfo() << "GROUPS-UI:" << label << "-> lignes =" << table->rowCount();
        for (int row = 0; row < table->rowCount(); ++row)
        {
            const QTableWidgetItem *item = table->item(row, 0);
            if (item == nullptr)
            {
                qInfo() << "GROUPS-UI:   ligne" << row << "VIDE";
                continue;
            }

            const auto kind = item->data(Qt::UserRole + 1).toUInt();
            qInfo() << "GROUPS-UI:   ligne" << row << (kind == 1 ? "[GROUPE]" : "[incr. ]")
                    << "| texte =" << item->text() << "| id =" << item->data(Qt::UserRole).toUInt()
                    << "| masquee =" << table->isRowHidden(row)
                    << "| caret =" << (not item->icon().isNull()) << "| gras =" << item->font().bold();
        }
    };

    dumpTable(QStringLiteral("table des increments"), increments);
    dumpTable(QStringLiteral("table des calculs preliminaires"), preview);

    const auto groupById = [doc](quint32 id) -> QDomElement {
        for (const QString &tag : {VAbstractPattern::TagIncrementGroups, VAbstractPattern::TagPreviewCalculationGroups})
        {
            const QDomElement list = doc->documentElement().firstChildElement(tag);
            for (QDomNode node = list.firstChild(); not node.isNull(); node = node.nextSibling())
            {
                if (const QDomElement group = node.toElement();
                    not group.isNull() && group.tagName() == VAbstractPattern::TagIncrementGroup
                    && group.attribute(VAbstractPattern::AttrId).toUInt() == id)
                {
                    return group;
                }
            }
        }
        return {};
    };

    // La premiere ligne de groupe d'une table, pliee par le signal que la table emet vraiment.
    const auto clickGroupRow = [&groupById, &dumpTable](QTableWidget *table, const QString &label) {
        int groupRow = -1;
        for (int row = 0; row < table->rowCount(); ++row)
        {
            const QTableWidgetItem *item = table->item(row, 0);
            if (item != nullptr && item->data(Qt::UserRole + 1).toUInt() == 1)
            {
                groupRow = row;
                break;
            }
        }

        if (groupRow == -1)
        {
            qInfo() << "GROUPS-UI:" << label << "-> aucune ligne de groupe";
            return;
        }

        const quint32 groupId = table->item(groupRow, 0)->data(Qt::UserRole).toUInt();

        QMetaObject::invokeMethod(table, "cellClicked", Qt::DirectConnection, Q_ARG(int, groupRow), Q_ARG(int, 0));

        const QDomElement group = groupById(groupId);
        qInfo() << "GROUPS-UI:" << label << "-> clic sur la ligne" << groupRow << "| groupe" << groupId
                << "| nom =" << group.attribute(VAbstractPattern::AttrName)
                << "| collapse =" << group.attribute(VAbstractPattern::AttrCollapsed);

        dumpTable(label + QStringLiteral(" apres le clic"), table);
    };

    clickGroupRow(increments, QStringLiteral("table des increments"));
    clickGroupRow(preview, QStringLiteral("table des calculs preliminaires"));

    // Et l'enregistrement : deux groupes replies, un dans chaque liste, et c'est le schema qui tranche.
    if (QString error; doc->SaveDocument(QStringLiteral("/tmp/probe_ui_folded.ostara"), error))
    {
        qInfo() << "GROUPS-UI: ENREGISTREMENT OK -> /tmp/probe_ui_folded.ostara";
    }
    else
    {
        qInfo() << "GROUPS-UI: ENREGISTREMENT REFUSE ->" << error;
    }

    // Deplie a nouveau, pour laisser le document dans son etat de depart.
    clickGroupRow(increments, QStringLiteral("table des increments, second clic"));
    clickGroupRow(preview, QStringLiteral("table des calculs preliminaires, second clic"));
}

// TEMPORARY — sonde de la dialog « nouvelle pièce », à retirer à la fin du chantier. Elle construit la dialog et
// lit ce qu'elle montre : les deux groupes, ce que chaque ligne de la bibliothèque dit, ce qui ne peut pas être
// choisi, et ce que le bouton annonce. C'est ce qu'un coup d'œil ne prouve pas.
void ProbeNewPieceDialog(VPattern *doc, VContainer *pattern, const QString &libraryPath)
{
    const auto dump = [](const QString &label, DialogNewPattern &dialog)
    {
        auto *list = dialog.findChild<QListWidget *>(QStringLiteral("listPieces"));
        auto *hint = dialog.findChild<QLabel *>(QStringLiteral("labelImportHint"));
        auto *hidden = dialog.findChild<QLabel *>(QStringLiteral("labelImportHidden"));
        auto *groupImport = dialog.findChild<QGroupBox *>(QStringLiteral("groupImport"));
        auto *buttonBox = dialog.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));

        if (list == nullptr || hint == nullptr || hidden == nullptr || groupImport == nullptr || buttonBox == nullptr)
        {
            qInfo() << "NEWPP:" << label << "| les widgets de la dialog sont introuvables";
            return;
        }

        qInfo() << "NEWPP:" << label << "| groupe import =" << groupImport->title()
                << "| lignes =" << list->count() << "| bouton =" << buttonBox->button(QDialogButtonBox::Ok)->text()
                << "| actif =" << buttonBox->button(QDialogButtonBox::Ok)->isEnabled();
        qInfo() << "NEWPP:" << label << "| indication =" << hint->text();
        if (hidden->isVisible() || not hidden->text().isEmpty())
        {
            qInfo() << "NEWPP:" << label << "| pieces ecartees =" << hidden->text();
        }

        for (int row = 0; row < list->count(); ++row)
        {
            const QListWidgetItem *item = list->item(row);
            qInfo() << "NEWPP:" << label << "|   ligne" << row << "|" << item->text()
                    << "| choisissable =" << ((item->flags() & Qt::ItemIsEnabled) != 0)
                    << "| chemin =" << item->data(Qt::UserRole).toString()
                    << "| info =" << QString(item->toolTip()).replace(QLatin1Char('\n'), QStringLiteral(" / "));
        }
    };

    // 1. Une piece de plus dans un patron qui existe : l'unite vient du patron.
    {
        DialogNewPattern dialog(pattern, QStringLiteral("Piece 9"), DialogNewPattern::Creation::Piece);
        dialog.SetTakenNames({QStringLiteral("Piece 9")});
        dialog.SetLibraryDirectory(libraryPath);
        dialog.SetLibraryUnit(OPieceImport::HostUnit(doc));
        dump(QStringLiteral("piece, unite du patron"), dialog);
    }

    // 2. Le patron qui nait : l'unite est la question, et le tri la suit.
    {
        DialogNewPattern dialog(pattern, QStringLiteral("Piece 1"), DialogNewPattern::Creation::Pattern);
        dialog.SetLibraryDirectory(libraryPath);

        auto *units = dialog.findChild<QComboBox *>(QStringLiteral("comboBoxUnits"));
        if (units == nullptr)
        {
            qInfo() << "NEWPP: nouveau patron | la combo d'unites est introuvable";
            return;
        }

        for (const QString &wanted : {QStringLiteral("cm"), QStringLiteral("mm"), QStringLiteral("inch")})
        {
            const int index = units->findData(wanted);
            if (index != -1)
            {
                units->setCurrentIndex(index);
                dump(QStringLiteral("nouveau patron en ") + wanted, dialog);
            }
        }

        // Et le choix, puis le double-clic, sur la premiere piece offerte.
        auto *list = dialog.findChild<QListWidget *>(QStringLiteral("listPieces"));
        auto *buttonBox = dialog.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
        units->setCurrentIndex(units->findData(QStringLiteral("cm")));
        for (int row = 0; row < list->count(); ++row)
        {
            if ((list->item(row)->flags() & Qt::ItemIsEnabled) == 0)
            {
                continue;
            }

            list->itemDoubleClicked(list->item(row));
            qInfo() << "NEWPP: nouveau patron | double-clic sur la ligne" << row
                    << "| resultat =" << dialog.result()
                    << "(Accepted =" << static_cast<int>(QDialog::Accepted) << ") | chemin rendu ="
                    << dialog.ImportedPath() << "| bouton =" << buttonBox->button(QDialogButtonBox::Ok)->text();
            break;
        }
    }
}

// TEMPORARY — sonde de l'import des incréments, à retirer à la fin du chantier.
// Fait entrer dans le patron les incréments d'une pièce de la bibliothèque, et lit ce que le patron en a fait.
void ProbePieceImport(VPattern *doc, VContainer *pattern)
{
    if (pattern == nullptr || doc == nullptr)
    {
        qInfo() << "IMPORT: no document loaded";
        return;
    }

    OPieceDocument piece;
    QString loadError;
    const QString piecePath = qEnvironmentVariable("OSTARA_PIECE_IMPORT_FILE",
                                                   QStringLiteral("/tmp/probe_piece_Buste.opiece"));
    if (not OPieceLibrary::Load(piecePath, &piece, &loadError))
    {
        qInfo() << "IMPORT: piece illisible ->" << loadError;
        return;
    }

    // TEMPORARY — le garde-fou d'unité : la pièce telle quelle, puis la même en inch, en unité inconnue et sans
    // unité. Rien ici n'appartient au produit.
    {
        const auto Check = [doc](const OPieceDocument &candidate, const QString &label) -> void
        {
            QString refusal;
            const bool accepted = OPieceImport::CheckUnit(candidate, doc, refusal);
            qInfo() << "IMPORT: unite" << label << "| piece =" << candidate.Unit()
                    << "| patron =" << UnitsToStr(doc->Units(), true) << "| acceptee =" << accepted
                    << "| motif =" << refusal;
        };

        Check(piece, QStringLiteral("telle quelle"));

        OPieceDocument inches;
        inches.Create(QStringLiteral("inch"));
        Check(inches, QStringLiteral("en inch"));

        OPieceDocument unknown;
        unknown.Create(QStringLiteral("furlong"));
        Check(unknown, QStringLiteral("unite inconnue"));

        OPieceDocument silent;
        silent.Create(QString());
        Check(silent, QStringLiteral("sans unite"));
    }

    const auto Table = [doc](bool preview) -> QDomElement {
        return doc->documentElement().firstChildElement(preview ? VAbstractPattern::TagPreviewCalculations
                                                               : VAbstractPattern::TagIncrements);
    };

    const auto CountRows = [&Table](bool preview) {
        int count = 0;
        for (QDomElement row = Table(preview).firstChildElement(VAbstractPattern::TagIncrement); not row.isNull();
             row = row.nextSiblingElement(VAbstractPattern::TagIncrement))
        {
            ++count;
        }
        return count;
    };

    const auto CountInGroup = [&Table](bool preview, quint32 id) {
        int count = 0;
        for (QDomElement row = Table(preview).firstChildElement(VAbstractPattern::TagIncrement); not row.isNull();
             row = row.nextSiblingElement(VAbstractPattern::TagIncrement))
        {
            if (row.attribute(VAbstractPattern::AttrGroupId).toUInt() == id)
            {
                ++count;
            }
        }
        return count;
    };

    const auto DumpGroups = [doc, &CountInGroup]() {
        for (const bool preview : {false, true})
        {
            const QMap<quint32, VIncrementGroupData> groups = doc->GetIncrementGroups(preview);
            qInfo() << "IMPORT:" << (preview ? "calculs preliminaires" : "table des increments")
                    << "-> groupes =" << groups.size();
            for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
            {
                qInfo() << "IMPORT:   groupe" << it.key() << "| nom =" << it.value().name
                        << "| lignes rangees =" << CountInGroup(preview, it.key());
            }
        }
    };

    const auto Report = [](const QString &label, const OPieceImport::IncrementReport &r) {
        qInfo() << "IMPORT:" << label << "-> ajoutes =" << r.added << "| gardes =" << r.kept
                << "| groupes crees =" << r.groupsAdded << "| conflits =" << r.clashes.size();
        for (const OPieceImport::IncrementClash &clash : r.clashes)
        {
            qInfo() << "IMPORT:   conflit sur" << clash.name << "| calculs preliminaires =" << clash.previewCalculation
                    << "| definition differente =" << clash.differentDefinition;
        }
    };

    qInfo() << "IMPORT: piece -> nom =" << piece.PieceName() << "| lignes ="
            << piece.Increments().childNodes().count() + piece.PreviewCalculations().childNodes().count()
            << "| groupes que la piece porte ="
            << piece.IncrementGroups().childNodes().count()
                   + piece.PreviewCalculationGroups().childNodes().count();
    qInfo() << "IMPORT: patron avant -> increments =" << CountRows(false) << "| calculs =" << CountRows(true);
    DumpGroups();

    Report(QStringLiteral("1er import"),
           OPieceImport::ImportIncrements(piece, doc, pattern, QStringLiteral("Buste")));
    qInfo() << "IMPORT: patron apres 1 -> increments =" << CountRows(false) << "| calculs =" << CountRows(true);
    DumpGroups();

    // Le deuxieme import ne doit rien ajouter et ne creer aucun second groupe : le premier import gagne.
    Report(QStringLiteral("2e import"),
           OPieceImport::ImportIncrements(piece, doc, pattern, QStringLiteral("Buste")));
    qInfo() << "IMPORT: patron apres 2 -> increments =" << CountRows(false) << "| calculs =" << CountRows(true);
    DumpGroups();

    // Une definition changee cote patron : la piece doit le signaler sans rien arbitrer. On ne touche pas a une
    // ligne que la piece ne porte pas.
    const QString touched = QStringLiteral("#Cou");
    if (QDomElement row = doc->IncrementRow(touched, false);
        not row.isNull() && row.hasAttribute(VAbstractPattern::AttrFormula))
    {
        const QString before = row.attribute(VAbstractPattern::AttrFormula);
        row.setAttribute(VAbstractPattern::AttrFormula, QStringLiteral("42"));

        Report(QStringLiteral("3e import, une definition changee"),
               OPieceImport::ImportIncrements(piece, doc, pattern, QStringLiteral("Buste")));
        qInfo() << "IMPORT: la definition du patron est intacte ="
                << (doc->IncrementRow(touched, false).attribute(VAbstractPattern::AttrFormula) == QStringLiteral("42"))
                << "(formule ecrite = 42, formule de la piece =" << before << ")";
    }

    if (QString error; doc->SaveDocument(QStringLiteral("/tmp/probe_import.ostara"), error))
    {
        qInfo() << "IMPORT: ENREGISTREMENT OK -> /tmp/probe_import.ostara";
    }
    else
    {
        qInfo() << "IMPORT: ENREGISTREMENT REFUSE ->" << error;
    }

    // ── Les noms de role des lignes : ils vivent a la RACINE du patron, hors du bloc copie ───────────────────
    {
        const auto ReportRoles = [](const QString &label, const OPieceImport::LineNameReport &roles) {
            qInfo() << "IMPORT:" << label << "-> ajoutes =" << roles.added << "| gardes =" << roles.kept
                    << "| conflits =" << roles.clashes.size();
            for (const OPieceImport::LineNameClash &clash : roles.clashes)
            {
                qInfo() << "IMPORT:   conflit sur la paire" << clash.points << "| nom =" << clash.name
                        << "| nom deja pris =" << clash.nameTaken << "| le patron garde =" << clash.held;
            }
        };

        qInfo() << "IMPORT: la piece porte" << piece.LineNames().childNodes().count() << "nom(s) de role";

        const QHash<QString, QString> none;
        ReportRoles(QStringLiteral("roles tels quels"), OPieceImport::ImportLineNames(piece, pattern, none));

        // Le renommage, tel que le marquage le fera : la paire ET le nom du role.
        QHash<QString, QString> renamed;
        for (QDomElement row = piece.LineNames().firstChildElement(VDomDocument::TagLine); not row.isNull();
             row = row.nextSiblingElement(VDomDocument::TagLine))
        {
            const QStringList pair = row.attribute(AttrPoints).split(QLatin1Char(' '), Qt::SkipEmptyParts);
            for (const QString &name : pair)
            {
                renamed.insert(name, name + QStringLiteral("__P1"));
            }
            const QString role = row.attribute(AttrName);
            renamed.insert(role, role + QStringLiteral("__P1"));
        }
        ReportRoles(QStringLiteral("roles renommes"), OPieceImport::ImportLineNames(piece, pattern, renamed));
    }

    // ── L'identite : ce que le bloc emporte de la piece dont il vient ────────────────────────────────────────
    qInfo() << "IMPORT: piece -> nom (categorie d'origine) =" << piece.PieceName()
            << "| identifiant =" << piece.LibraryId();

    QDomElement block = piece.DrawElement().cloneNode(true).toElement();
    OPieceImport::SetOrigin(block, piece); // AVANT le renommage : c'est l'ordre qui fait la categorie
    block.setAttribute(AttrName, QStringLiteral("Dos"));

    qInfo() << "IMPORT: bloc prepare -> nom =" << block.attribute(AttrName)
            << "| identifiant =" << block.attribute(OPieceDocument::AttrLibraryId)
            << "| categorie =" << block.attribute(OPieceDocument::AttrCategory);

    const OPieceRemap::Report identities = OPieceRemap::RemapIds(block, pattern);
    doc->InsertRootElement(block);

    QString error;
    const QString written = QStringLiteral("/tmp/probe_import_identite.ostara");
    if (doc->SaveDocument(written, error))
    {
        QFile reread(written);
        QDomDocument again;
        if (reread.open(QIODevice::ReadOnly) && again.setContent(&reread))
        {
            const QDomNodeList draws = again.elementsByTagName(VAbstractPattern::TagDraw);
            for (int i = 0; i < draws.count(); ++i)
            {
                const QDomElement draw = draws.at(i).toElement();
                if (draw.attribute(AttrName) != QStringLiteral("Dos"))
                {
                    continue;
                }
                qInfo() << "IMPORT: relu du fichier -> nom =" << draw.attribute(AttrName)
                        << "| identifiant =" << draw.attribute(OPieceDocument::AttrLibraryId)
                        << "| categorie =" << draw.attribute(OPieceDocument::AttrCategory);
            }

            // La section est RECONSTRUITE depuis le conteneur a chaque enregistrement : ce qui suit est donc la
            // preuve que le role a bien ete depose dans le conteneur, et pas seulement ecrit dans le DOM.
            const QDomElement roles = again.elementsByTagName(VAbstractPattern::TagLineNames).at(0).toElement();
            for (QDomElement row = roles.firstChildElement(VDomDocument::TagLine); not row.isNull();
                 row = row.nextSiblingElement(VDomDocument::TagLine))
            {
                qInfo() << "IMPORT: <lineNames> relu -> points =" << row.attribute(AttrPoints)
                        << "| nom =" << row.attribute(AttrName);
            }
        }
        qInfo() << "IMPORT: ENREGISTREMENT OK (identite) ->" << written
                << "| ids remplaces sur le bloc =" << identities.ids << identities.written;
    }
    else
    {
        qInfo() << "IMPORT: ENREGISTREMENT REFUSE (identite) ->" << error;
    }

    // ── Une piece ne porte jamais de categorie : le schema de la piece la refuse ─────────────────────────────
    {
        OPieceDocument cut;
        cut.Create(piece.Unit());
        cut.SetDraw(block, QStringLiteral("Dos"), QUuid::createUuid().toString());

        QString cutError;
        const bool saved = OPieceLibrary::Save(&cut, QStringLiteral("/tmp/probe_piece_identite.opiece"), &cutError);
        qInfo() << "IMPORT: piece tiree d'un bloc qui portait une categorie -> categorie gardee ="
                << cut.DrawElement().hasAttribute(OPieceDocument::AttrCategory) << "| ecriture acceptee =" << saved
                << cutError;

        // Le correctif est-il necessaire ? On remet la categorie sur le <draw> de la piece et on reessaie : le
        // schema de la piece ne la connait pas, donc l'ecriture doit etre REFUSEE.
        cut.DrawElement().setAttribute(OPieceDocument::AttrCategory, QStringLiteral("Buste"));
        QString refusedError;
        const bool refused =
            OPieceLibrary::Save(&cut, QStringLiteral("/tmp/probe_piece_identite_refusee.opiece"), &refusedError);
        qInfo() << "IMPORT: le meme <draw> AVEC une categorie -> ecriture acceptee =" << refused << refusedError;
    }
}

void ProbeRuntimeBlockInsertion(VContainer *pattern, VPattern *doc)
{
    if (pattern == nullptr || doc == nullptr)
    {
        qInfo() << "PIECE-PROBE: no document loaded";
        return;
    }

    const QDomNodeList draws = doc->elementsByTagName(VAbstractPattern::TagDraw);
    qInfo() << "PIECE-PROBE: blocks in file =" << draws.size();

    // Combien d'objets du patron réclament DÉJÀ le même nom, sans aucune insertion ? Un nom « base_n » dont
    // « base » existe aussi est un doublon que le conteneur a numéroté lui-même : le fichier ne le contient pas.
    {
        const QHash<QString, QSharedPointer<VInternalVariable>> *vars = pattern->DataVariables();
        QSet<QString> all;
        for (auto it = vars->constBegin(); it != vars->constEnd(); ++it)
        {
            all.insert(it.key());
        }

        static const QRegularExpression dupTail(QStringLiteral("_(\\d+)$"));
        QStringList dupes;
        int dupeCount = 0;
        for (auto it = vars->constBegin(); it != vars->constEnd(); ++it)
        {
            const QRegularExpressionMatch match = dupTail.match(it.key());
            if (not match.hasMatch() || not all.contains(it.key().left(match.capturedStart())))
            {
                continue;
            }
            ++dupeCount;
            if (dupes.size() < 20)
            {
                dupes.append(it.key());
            }
        }
        qInfo() << "PIECE-PROBE: noms deja numerotes par le conteneur, sans import =" << dupeCount << dupes;

        // Les ids declares dans le fichier, et la table « id fabrique -> type d'operation qui le reclame ».
        // Croiser l'id de la courbe d'une valeur numerotee avec cette table designe l'outil responsable,
        // sans deduction.
        QSet<QString> declaredIds;
        ProbeCollectDeclaredIds(doc->documentElement(), declaredIds);

        QHash<QString, QString> operationOfDestination;
        const QDomNodeList operations = doc->elementsByTagName(QStringLiteral("operation"));
        for (int i = 0; i < operations.size(); ++i)
        {
            const QDomElement operation = operations.at(i).toElement();
            if (operation.isNull())
            {
                continue;
            }
            const QString type = operation.attribute(QStringLiteral("type"));
            for (QDomElement destination = operation.firstChildElement(QStringLiteral("destination"));
                 not destination.isNull();
                 destination = destination.nextSiblingElement(QStringLiteral("destination")))
            {
                for (QDomElement item = destination.firstChildElement(QStringLiteral("item")); not item.isNull();
                     item = item.nextSiblingElement(QStringLiteral("item")))
                {
                    operationOfDestination.insert(item.attribute(QStringLiteral("idObject")), type);
                }
            }
        }

        // Classement : un doublon est soit un OBJET de geometrie, soit une VALEUR derivee (longueur, angle,
        // aire). Pour une valeur derivee, l'id de sa courbe dit si la courbe est dans le fichier, et quelle
        // operation la reclame.
        for (auto it = vars->constBegin(); it != vars->constEnd(); ++it)
        {
            const QRegularExpressionMatch match = dupTail.match(it.key());
            if (not match.hasMatch() || not all.contains(it.key().left(match.capturedStart())))
            {
                continue;
            }

            const QSharedPointer<VInternalVariable> var = it.value();
            const int type = static_cast<int>(var->GetType());

            QString object = QStringLiteral("VALEUR DERIVEE");
            if (const QSharedPointer<VGObject> gobject = qSharedPointerDynamicCast<VGObject>(var);
                not gobject.isNull())
            {
                object = QStringLiteral("OBJET id=%1").arg(gobject->id());
                if (const QSharedPointer<VAbstractCurve> curve = qSharedPointerDynamicCast<VAbstractCurve>(var);
                    not curve.isNull())
                {
                    object += QStringLiteral(" duplicate=%1").arg(curve->GetDuplicate());
                }
            }
            else if (const QSharedPointer<VCurveVariable> curveVar = qSharedPointerDynamicCast<VCurveVariable>(var);
                     not curveVar.isNull())
            {
                const quint32 curveId = curveVar->GetId();
                const QString id = QString::number(curveId);
                QString detail = QStringLiteral("courbe ABSENTE du conteneur");
                try
                {
                    const QSharedPointer<VAbstractCurve> curve = pattern->GeometricObject<VAbstractCurve>(curveId);
                    detail = QStringLiteral("nom=%1 | duplicate=%2 | derivee=%3")
                                 .arg(curve->name())
                                 .arg(curve->GetDuplicate())
                                 .arg(curve->IsDerivative() ? QStringLiteral("oui") : QStringLiteral("non"));
                }
                catch (const VExceptionBadId &)
                {
                }
                object = QStringLiteral("courbe id=%1 | dans le fichier=%2 | operation=%3 | %4")
                             .arg(id, declaredIds.contains(id) ? QStringLiteral("oui") : QStringLiteral("NON"),
                                  operationOfDestination.value(id, QStringLiteral("-")), detail);
            }
            qInfo() << "PIECE-PROBE:   doublon" << it.key() << "| type=" << type << "|" << object;
        }
    }

    if (draws.isEmpty())
    {
        qInfo() << "PIECE-PROBE: no block to copy, nothing to test";
        return;
    }

    // Le bloc à copier : le premier, ou celui que OSTARA_PIECE_DRAW désigne — les pièces plissées ne sont pas
    // toujours la première du fichier, et ce sont elles qui éprouvent la règle des indices.
    const int drawIndex = qEnvironmentVariableIntValue("OSTARA_PIECE_DRAW");
    const QDomElement source = draws.at(qBound(0, drawIndex, static_cast<int>(draws.size()) - 1)).toElement();
    const QString sourceName = source.attribute(AttrName);
    const qsizetype variablesBefore = pattern->DataVariables()->size();

    QDomElement clone = source.cloneNode(true).toElement();
    const QString cloneName = sourceName + probeSuffix;
    clone.setAttribute(AttrName, cloneName);

    // Des noms de pièce distincts, sinon l'avertissement de doublon.
    const QDomNodeList details = clone.elementsByTagName(VAbstractPattern::TagDetail);
    for (int i = 0; i < details.count(); ++i)
    {
        QDomElement detail = details.at(i).toElement();
        detail.setAttribute(AttrName, detail.attribute(AttrName) + probeSuffix);
    }

    QSet<QString> declared;
    ProbeCollectDeclaredIds(clone, declared);

    // Sonde 2 : le marqueur de pièce sur chaque nom déclaré.
    int namesMarked = 0;
    ProbeMarkNames(clone, namesMarked);

    // La règle n'est plus écrite ici : elle est celle du produit, lue dans le schéma par OPieceRemap.
    QStringList rule = OPieceRemap::IdSlots().values();
    rule.sort();
    qInfo() << "PIECE-PROBE: regle |" << rule.size() << "slots |" << rule;
    qInfo() << "PIECE-PROBE: regle | elements de valeur =" << OPieceRemap::IdElements().values();

    const OPieceRemap::Report remap = OPieceRemap::RemapIds(clone, pattern);

    // Ce que la copie a réellement changé par rapport à son original, et ce qu'elle n'aurait pas dû toucher.
    {
        QMap<QString, int> perSlot;
        QStringList unexpected;
        ProbeCompareBlocks(source, clone, perSlot, unexpected);

        int total = 0;
        QStringList detail;
        for (auto it = perSlot.constBegin(); it != perSlot.constEnd(); ++it)
        {
            total += it.value();
            detail.append(it.key() + QLatin1Char('=') + QString::number(it.value()));
        }
        qInfo() << "PIECE-PROBE: comparaison | valeurs changees =" << total << "| remap.written =" << remap.written
                << "| anomalies (hors name) =" << unexpected.size() << unexpected.mid(0, 12);
        qInfo() << "PIECE-PROBE: comparaison |" << detail;
    }

    QSet<QString> namesBefore;
    {
        const QHash<QString, QSharedPointer<VInternalVariable>> *vars = pattern->DataVariables();
        for (auto it = vars->constBegin(); it != vars->constEnd(); ++it)
        {
            namesBefore.insert(it.key());
        }
    }

    doc->InsertRootElement(clone);

    bool inserted = false;
    const QDomNodeList afterInsert = doc->elementsByTagName(VAbstractPattern::TagDraw);
    for (int i = 0; i < afterInsert.count(); ++i)
    {
        if (afterInsert.at(i).toElement().attribute(AttrName) == cloneName)
        {
            inserted = true;
            break;
        }
    }

    qInfo() << "PIECE-PROBE: block" << sourceName << "->" << cloneName << "| inserted =" << inserted
            << "| declared ids =" << declared.size() << "| distinct ids remapped =" << remap.ids
            << "| slots rewritten =" << remap.written << "| names marked =" << namesMarked;

    try
    {
        doc->LiteParseTree(Document::FullLiteParse);

        int nan = 0;
        int total = 0;
        const QHash<QString, QSharedPointer<VInternalVariable>> *vars = pattern->DataVariables();
        for (auto it = vars->constBegin(); it != vars->constEnd(); ++it)
        {
            if (it.value().isNull() || it.value()->GetValue() == nullptr)
            {
                continue;
            }
            ++total;
            if (std::isnan(*it.value()->GetValue()))
            {
                ++nan;
            }
        }

        const QDomNodeList afterParse = doc->elementsByTagName(VAbstractPattern::TagDraw);
        qInfo() << "PIECE-PROBE: reparse OK | blocks" << draws.size() << "->" << afterParse.size()
                << "| variables" << variablesBefore << "->" << vars->size() << "| values" << total
                << "| NaN" << nan;

        // Un nom d'objet du bloc inséré n'est « suffixé d'office » que si, en lui retirant son _<n> final,
        // on retombe sur un nom qui existait AVANT l'insertion. C'est la détection exacte du dédoublonnage.
        static const QRegularExpression autoTail(QStringLiteral("_(\\d+)$"));
        QStringList autoSuffixed;
        QStringList markedNames;
        int markedCount = 0;
        for (auto it = vars->constBegin(); it != vars->constEnd(); ++it)
        {
            if (namesBefore.contains(it.key()))
            {
                continue;
            }

            const QRegularExpressionMatch match = autoTail.match(it.key());
            if (match.hasMatch() && namesBefore.contains(it.key().left(match.capturedStart())))
            {
                if (autoSuffixed.size() < 8)
                {
                    autoSuffixed.append(it.key());
                }
                continue;
            }

            if (it.key().contains(QStringLiteral("__P1")))
            {
                ++markedCount;
                if (markedNames.size() < 6)
                {
                    markedNames.append(it.key());
                }
            }
        }
        qInfo() << "PIECE-PROBE: new values carrying the piece marker =" << markedCount << markedNames;
        qInfo() << "PIECE-PROBE: new values the container had to suffix itself =" << autoSuffixed.size()
                << autoSuffixed;
    }
    catch (const VException &e)
    {
        qInfo() << "PIECE-PROBE: reparse FAILED |" << e.ErrorMessage();
    }

    const QString outFile = QDir::tempPath() + QStringLiteral("/ostara_piece_probe.ostara");
    QString error;
    const bool saved = doc->SaveDocument(outFile, error);
    qInfo() << "PIECE-PROBE: save =" << saved << "|" << error << "|" << outFile;
}

// TEMPORARY — sonde de la fenêtre de bibliothèque, à retirer à la fin du chantier. Elle ne montre rien : elle
// construit la fenêtre, puis lit ce qu'elle afficherait, ligne par ligne. C'est la seule façon de mesurer une
// fenêtre sans quelqu'un devant.
void ProbePieceLibrary(const QString &directory)
{
    OPieceLibraryDialog dialog(directory);

    auto *table = dialog.findChild<QTableWidget *>();
    auto *details = dialog.findChild<QLabel *>(QStringLiteral("descLabel"));
    if (table == nullptr || details == nullptr)
    {
        qInfo() << "LIB: la fenetre n'a ni table ni zone de details";
        return;
    }

    qInfo() << "LIB: dossier =" << directory << "| lignes =" << table->rowCount()
            << "| colonnes =" << table->columnCount();

    for (int row = 0; row < table->rowCount(); ++row)
    {
        QStringList cells;
        for (int column = 0; column < table->columnCount(); ++column)
        {
            const QTableWidgetItem *item = table->item(row, column);
            cells.append(item == nullptr ? QStringLiteral("<vide>") : item->text());
        }
        qInfo() << "LIB:  ligne" << row << "=" << cells;
    }

    for (int row = 0; row < table->rowCount(); ++row)
    {
        table->selectRow(row);
        qInfo() << "LIB: details de la ligne" << row << "=" << details->text().split(QLatin1Char('\n'));
    }

    // Et le cas d'une ligne filtrée : ce qui reste caché ne doit plus rien dire, et ce qui reste visible
    // doit être ce que la zone de détails raconte.
    auto *search = dialog.findChild<QLineEdit *>();
    if (search != nullptr)
    {
        auto report = [table, details, search](const QString &text)
        {
            search->setText(text);
            QStringList visible;
            for (int row = 0; row < table->rowCount(); ++row)
            {
                if (not table->isRowHidden(row))
                {
                    visible.append(table->item(row, 0)->text());
                }
            }
            qInfo() << "LIB: recherche" << text << "-> visibles =" << visible << "| details ="
                    << details->text().split(QLatin1Char('\n'));
        };

        report(QStringLiteral("zzz"));
        report(QStringLiteral("Manch"));
        report(QString());
    }
}
// TEMPORARY — fabrication du patron témoin du §11, à retirer avec le témoin qu'il produit. Ce que le chantier
// ne trouvait dans aucun patron réel : une formule qui cite un nom d'objet de **sa propre pièce** — les seules
// rencontres mesurées jusqu'ici citaient les objets d'une **autre** pièce, c'est-à-dire une dépendance. Le
// témoin est un `BustedeBase` auquel on ajoute un point dont la longueur est `Line_Em_Pro`, la ligne Em→Pro du
// Buste : au marquage, cette formule doit suivre ses points (`Line_Em__P1_Pro__P1`), et c'est exactement ce
// qu'aucune mesure ne pouvait montrer avant.
void ProbeWitnessFormulas(VPattern *doc, const QString &outPath)
{
    const QDomElement root = doc->documentElement();

    QDomElement calculation;
    for (QDomElement draw = root.firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        if (draw.attribute(VAbstractPattern::AttrName) == QStringLiteral("Buste"))
        {
            calculation = draw.firstChildElement(VAbstractPattern::TagCalculation);
        }
    }

    if (calculation.isNull())
    {
        qInfo() << "WITNESS: pas de bloc Buste";
        return;
    }

    // Le modèle vient du **même** bloc : ses références restent dans le bloc, donc la copie n'invente aucune
    // référence croisée — ce qu'une pièce ne peut pas porter.
    QDomElement model;
    for (QDomElement point = calculation.firstChildElement(VAbstractPattern::TagPoint); not point.isNull();
         point = point.nextSiblingElement(VAbstractPattern::TagPoint))
    {
        if (point.attribute(QStringLiteral("type")) == QStringLiteral("endLine"))
        {
            model = point;
            break;
        }
    }

    if (model.isNull())
    {
        qInfo() << "WITNESS: aucun point endLine dans le Buste, rien a fabriquer";
        return;
    }

    // Le plus grand id du document, par le même parcours que le reste des sondes : le joker `*` de
    // `elementsByTagName` ne rend rien ici.
    QSet<QString> declaredIds;
    ProbeCollectDeclaredIds(root, declaredIds);
    quint32 highest = 0;
    for (const QString &value : declaredIds)
    {
        bool ok = false;
        const quint32 id = value.toUInt(&ok);
        if (ok)
        {
            highest = qMax(highest, id);
        }
    }

    qInfo() << "WITNESS: ids declares =" << declaredIds.size() << "| plus grand id =" << highest;

    // Le nom du point témoin doit être **libre** : un doublon ferait deux objets sous une clé, et la mesure ne
    // dirait plus lequel des deux la formule suit. C'est le vocabulaire du marquage qui répond — celui du produit.
    const QString witnessName = QStringLiteral("ZZTemoin");
    if (OPieceMark::DeclaredNames(root).contains(witnessName))
    {
        qInfo() << "WITNESS:" << witnessName << "existe deja, rien a fabriquer";
        return;
    }

    QDomElement witness = model.cloneNode(true).toElement();
    witness.setAttribute(VDomDocument::AttrId, QString::number(highest + 1));
    witness.setAttribute(VAbstractPattern::AttrName, witnessName);
    // L'incrément est **cité** par la formule : c'est ce qui le fait voyager avec la pièce — le constructeur
    // n'emporte que ce que la pièce utilise, et un incrément que personne ne cite reste chez lui.
    witness.setAttribute(QStringLiteral("length"), QStringLiteral("Line_Em_Pro+#ZZTemoin"));
    witness.setAttribute(QStringLiteral("angle"), QStringLiteral("0"));
    calculation.appendChild(witness);

    qInfo() << "WITNESS: point" << witness.attribute(VAbstractPattern::AttrName)
            << "| id =" << witness.attribute(VDomDocument::AttrId)
            << "| basePoint =" << witness.attribute(QStringLiteral("basePoint"))
            << "| length =" << witness.attribute(QStringLiteral("length"));

    // Et un incrément que l'hôte ne porte pas : c'est lui qui fait **naître** une ligne et un groupe à
    // l'insertion, donc lui qui éprouve l'annulation de l'import. Sans lui, tout existe déjà dans l'hôte, rien
    // n'est ajouté, et « une seule passe d'undo » ne se mesurerait pas.
    QDomElement increments = root.firstChildElement(VAbstractPattern::TagIncrements);
    if (increments.isNull())
    {
        qInfo() << "WITNESS: pas de section increments, rien a fabriquer";
        return;
    }

    QDomElement row = doc->createElement(VAbstractPattern::TagIncrement);
    row.setAttribute(VAbstractPattern::AttrName, QStringLiteral("#ZZTemoin"));
    row.setAttribute(QStringLiteral("formula"), QStringLiteral("2"));
    row.setAttribute(QStringLiteral("description"), QStringLiteral("Increment temoin de l'insertion"));
    increments.appendChild(row);
    qInfo() << "WITNESS: increment" << row.attribute(VAbstractPattern::AttrName) << "ajoute";

    QString error;
    if (doc->SaveDocument(outPath, error))
    {
        qInfo() << "WITNESS: ecrit ->" << outPath << QFileInfo(outPath).size() << "octets";
    }
    else
    {
        qInfo() << "WITNESS: ECRITURE REFUSEE ->" << error;
    }
}

// TEMPORARY — sonde de l'insertion d'une pièce de bibliothèque, à retirer à la fin du chantier. Elle fait ce que
// le produit fera : lire la pièce, la refuser si elle n'a pas sa place, la préparer, puis **pousser la commande**
// — et elle mesure les deux choses qu'aucune relecture ne prouve : que la formule du témoin suit ses points, et
// qu'un seul Ctrl+Z remet le document exactement dans l'état d'avant.
namespace
{
/// Le contenu d'une section, **sérialisé** : `QDomElement::text()` ne rend rien quand tout est en attributs, et
/// c'est justement le cas des incréments — une comparaison sur `.text()` serait vraie sans rien comparer.
auto ProbeSectionText(const QDomElement &section) -> QString
{
    if (section.isNull())
    {
        return QStringLiteral("<absente>");
    }

    QDomDocument holder;
    holder.appendChild(section.cloneNode(true));
    return holder.toString(0);
}

auto ProbeSections(const QDomElement &root) -> QString
{
    QStringList parts;
    for (const QString &tag : {VAbstractPattern::TagIncrements, VAbstractPattern::TagIncrementGroups,
                               VAbstractPattern::TagPreviewCalculations,
                               VAbstractPattern::TagPreviewCalculationGroups, VAbstractPattern::TagLineNames})
    {
        const QDomElement section = root.firstChildElement(tag);
        const QString rows = section.isNull() ? QStringLiteral("-") : QString::number(section.childNodes().count());
        parts.append(tag + QLatin1Char('(') + rows + QLatin1String(")=") + ProbeSectionText(section));
    }
    return parts.join(QStringLiteral("\n    "));
}

auto ProbePointByName(const QDomElement &root, const QString &name) -> QDomElement
{
    const QDomNodeList points = root.elementsByTagName(VAbstractPattern::TagPoint);
    for (int i = 0; i < points.count(); ++i)
    {
        const QDomElement point = points.at(i).toElement();
        if (point.attribute(VAbstractPattern::AttrName) == name)
        {
            return point;
        }
    }
    return {};
}

auto ProbeUniqueInstance(VPattern *doc, const QString &wanted) -> QString
{
    const QStringList taken = doc->PatternBlockMapper()->GetBlockNames();
    if (not taken.contains(wanted))
    {
        return wanted;
    }

    for (int suffix = 2; suffix < 100; ++suffix)
    {
        const QString candidate = wanted + QLatin1Char(' ') + QString::number(suffix);
        if (not taken.contains(candidate))
        {
            return candidate;
        }
    }
    return wanted;
}
} // namespace

auto ProbePositionIn(const QDomElement &element) -> QPointF
{
    if (element.hasAttribute(AttrX) && element.hasAttribute(AttrY))
    {
        return {element.attribute(AttrX).toDouble(), element.attribute(AttrY).toDouble()};
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        const QPointF found = ProbePositionIn(child);
        if (not found.isNull())
        {
            return found;
        }
    }

    return {};
}

auto ProbeFirstPosition(const QDomElement &block) -> QPointF
{
    return ProbePositionIn(block);
}

void ProbePick(VPattern *doc, const QString &pointName)
{
    // Le point nomme, son outil, et **tout** ce que la scene trouve a sa position, dans l'ordre.
    for (QDomElement draw = doc->documentElement().firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
         draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
    {
        for (QDomElement point = draw.firstChildElement(QStringLiteral("calculation"))
                                      .firstChildElement(QStringLiteral("point"));
             not point.isNull(); point = point.nextSiblingElement(QStringLiteral("point")))
        {
            if (point.attribute(QStringLiteral("name")) != pointName)
            {
                continue;
            }

            const quint32 id = point.attribute(QStringLiteral("id")).toUInt();
            VDataTool *tool = VAbstractPattern::FindTool(id);
            qInfo() << "PICK:" << pointName << "| bloc =" << draw.attribute(VAbstractPattern::AttrName)
                    << "| id =" << id << "| type =" << point.attribute(QStringLiteral("type"))
                    << "| outil =" << (tool == nullptr ? QStringLiteral("aucun")
                                                       : QString::fromLatin1(tool->metaObject()->className()));

            auto *item = dynamic_cast<QGraphicsItem *>(tool);
            if (item != nullptr)
            {
                qInfo() << "PICK:   Z =" << item->zValue() << "| visible =" << item->isVisible()
                        << "| selectionnable =" << (item->flags() & QGraphicsItem::ItemIsSelectable)
                        << "| pos =" << item->pos() << "| scenePos =" << item->scenePos()
                        << "| boite =" << item->boundingRect();
            }

            if (auto *scene = VAbstractValApplication::VApp()->getCurrentScene(); scene != nullptr && item != nullptr)
            {
                const QList<QGraphicsItem *> found = scene->items(item->scenePos(), Qt::IntersectsItemShape,
                                                                  Qt::DescendingOrder);
                int rank = 0;
                for (QGraphicsItem *under : found)
                {
                    const auto *asTool = dynamic_cast<const QObject *>(dynamic_cast<const QGraphicsItem *>(under));
                    qInfo() << "PICK:   sous le curseur" << rank++ << "| Z =" << under->zValue()
                            << "| classe =" << (asTool == nullptr ? QStringLiteral("item")
                                                                 : QString::fromLatin1(asTool->metaObject()->className()))
                            << "| selectionnable =" << bool(under->flags() & QGraphicsItem::ItemIsSelectable);
                }
            }
            return;
        }
    }

    qInfo() << "PICK:" << pointName << "| introuvable";
}

void ProbeCitations(VPattern *doc, VContainer *pattern)
{
    const QStringList blocks = doc->PatternBlockMapper()->GetBlockNames();
    qInfo() << "CITATIONS: blocs =" << blocks;

    for (const QString &block : blocks)
    {
        const QDomElement element = doc->PatternBlockMapper()->GetElement(block);
        const QSet<QString> composed = OPieceMark::ComposedNames(element);
        qInfo() << "CITATIONS:" << block << "| noms composes =" << composed.size()
                << "| exemple =" << QStringList(composed.constBegin(), composed.constEnd()).mid(0, 6);

        QString error;
        const bool accepted = OPieceBuilder::CheckCitations(doc, block, error);
        qInfo() << "CITATIONS:" << block << "| enregistrable =" << accepted << "| refus =" << error;
    }

    // Ce que la fenetre d'enregistrement montrera, et par quel chemin chaque nom s'appliquera.
    for (const QString &block : blocks)
    {
        for (const OPieceName::Blocker &blocker : OPieceName::Blockers(doc, pattern, block))
        {
            const VDataTool *tool = VAbstractPattern::FindTool(blocker.curveId);
            qInfo() << "CITATIONS: a nommer | piece =" << block << "| nom =" << blocker.name
                    << "| nature =" << static_cast<int>(blocker.of) << "| slot =" << blocker.slot
                    << "| paire =" << blocker.pairKey << "| courbe =" << blocker.curveId
                    << "| outil =" << (tool == nullptr ? QStringLiteral("aucun")
                                                       : QString::fromLatin1(tool->metaObject()->className()))
                    << "| formule =" << blocker.formula.left(40);
        }
    }

    // Le detail : quelles formules du bloc citent un nom compose, et lesquelles viennent des autres blocs.
    for (const QString &block : blocks)
    {
        const QDomElement element = doc->PatternBlockMapper()->GetElement(block);
        const QSet<QString> everywhere = OPieceMark::ComposedNames(doc->documentElement());
        const QSet<QString> ofThisPiece = OPieceMark::ComposedNames(element);

        for (const VFormulaField &field : doc->ListExpressions())
        {
            bool inside = false;
            for (QDomNode parent = field.element; not parent.isNull(); parent = parent.parentNode())
            {
                if (parent == element)
                {
                    inside = true;
                    break;
                }
            }

            const QString cited = OPieceMark::ComposedCited(field.expression, inside ? everywhere : ofThisPiece);
            if (not cited.isEmpty())
            {
                qInfo() << "CITATIONS:" << block << (inside ? "| chez elle :" : "| chez une autre :") << cited
                        << "| champ =" << field.attribute << "| formule =" << field.expression.left(60);

                // Ce que la fenêtre montrera : les objets sans nom que cette formule cite, et le nom proposé.
                for (const DialogNameObject::Object &object : UnnamedObjects(pattern, field.expression))
                {
                    qInfo() << "CITATIONS:   a nommer | objet ="
                            << (object.pairKey.isEmpty() ? object.typeHead : QStringLiteral("Ligne"))
                            << "| nom propose =" << object.suffix << "| paire =" << object.pairKey
                            << "| courbe =" << object.curveId << "| montre =" << object.shown;
                }
            }
        }
    }
}

void ProbePieceInsert(VPattern *doc, VContainer *pattern, const QString &piecePath, const QPointF &anchorTarget)
{
    OPieceDocument piece;
    QString error;
    if (not OPieceLibrary::Load(piecePath, &piece, &error))
    {
        qInfo() << "INSERT: piece illisible ->" << error;
        return;
    }

    qInfo() << "INSERT: piece =" << piece.PieceName() << "| unite =" << piece.Unit()
            << "| dependances =" << piece.Dependencies().size();
    qInfo() << "INSERT: placement demande | cible =" << anchorTarget
            << "| point de base du morceau =" << ProbeFirstPosition(piece.DrawElement());

    if (not OPieceImport::CheckMeasurements(piece, doc, error))
    {
        qInfo() << "INSERT: aucune table de mesures ->" << error;
        return;
    }

    if (not OPieceImport::CheckUnit(piece, doc, error))
    {
        qInfo() << "INSERT: unite refusee ->" << error;
        return;
    }

    if (not OPieceImport::CheckDependencies(piece, doc, error))
    {
        qInfo() << "INSERT: dependance manquante ->" << error;
    }
    else
    {
        qInfo() << "INSERT: unite et dependances acceptees";
    }

    const QDomElement root = doc->documentElement();
    const QString sectionsBefore = ProbeSections(root);
    const int blocksBefore = static_cast<int>(doc->CountPatternBlockTags());
    const QDomElement mancheBefore = [doc]()
    {
        for (QDomElement draw = doc->documentElement().firstChildElement(VAbstractPattern::TagDraw); not draw.isNull();
             draw = draw.nextSiblingElement(VAbstractPattern::TagDraw))
        {
            if (draw.attribute(VAbstractPattern::AttrName) == QStringLiteral("Manche"))
            {
                return draw;
            }
        }
        return QDomElement();
    }();
    const QString mancheBeforeText = ProbeSectionText(mancheBefore);

    const QString instanceName = ProbeUniqueInstance(doc, piece.PieceName());

    // Ce que la pièce demande : **tous** ses alias, cités par une formule ou non, et la marque que le patron propose.
    const OPieceInsert::Proposal proposal = OPieceInsert::Propose(piece, doc, pattern, instanceName);
    qInfo() << "INSERT: proposition | alias =" << proposal.aliases.size() << "| marque =" << proposal.number
            << "| collision =" << proposal.collides << "| noms dans la table =" << proposal.table.size();
    for (const OPieceMark::Alias &alias : proposal.aliases)
    {
        qInfo() << "INSERT: alias |" << alias.name << "| slots =" << alias.writtenIn.join(QStringLiteral(" , "))
                << "| porteurs =" << alias.owners.join(QStringLiteral(" , "))
                << "| propose =" << proposal.table.value(alias.name, alias.name);
    }

    // La fenêtre, telle qu'elle s'ouvre — et la table qu'elle rend quand on n'y touche pas.
    {
        const QSet<QString> keys = OPieceInsert::Keys(pattern);
        qInfo() << "INSERT: cles du conteneur | taille =" << keys.size() << "| Em =" << keys.contains(QStringLiteral("Em"))
                << "| Line_Angle180 =" << keys.contains(QStringLiteral("Line_Angle180"));

        // La règle « le patron y répond » : les noms nus des points de l'hote, et les alias de la pièce.
        const QDomElement firstDraw = doc->documentElement().firstChildElement(VAbstractPattern::TagDraw);
        QStringList hosted;
        for (QDomElement point = firstDraw.firstChildElement(QStringLiteral("calculation"))
                                      .firstChildElement(QStringLiteral("point"));
             not point.isNull() && hosted.size() < 6; point = point.nextSiblingElement(QStringLiteral("point")))
        {
            const QString name = point.attribute(QStringLiteral("name"));
            if (not name.isEmpty())
            {
                hosted.append(name + (OPieceMark::AnswersTo(keys, name) ? QStringLiteral("=OUI") : QStringLiteral("=non")));
            }
        }
        QStringList answered;
        for (const OPieceMark::Alias &alias : proposal.aliases)
        {
            if (OPieceMark::AnswersTo(keys, alias.name))
            {
                answered.append(alias.name);
            }
        }
        qInfo() << "INSERT: noms de points de l'hote, le patron y repond =" << hosted;
        qInfo() << "INSERT: alias deja pris par le patron =" << answered << "(" << answered.size() << "sur"
                << proposal.aliases.size() << ")";

        OPieceImportDialog dialog(proposal, doc->PatternBlockMapper()->GetBlockNames(), OPieceInsert::Keys(pattern));
        auto *table = dialog.findChild<QTableWidget *>(QStringLiteral("tableAliases"));
        auto *nameEdit = dialog.findChild<QLineEdit *>(QStringLiteral("lineEditName"));
        auto *box = dialog.findChild<QDialogButtonBox *>(QStringLiteral("buttonBox"));
        QPushButton *ok = box == nullptr ? nullptr : box->button(QDialogButtonBox::Ok);
        auto *aliasProblem = dialog.findChild<QLabel *>(QStringLiteral("labelAliasProblem"));

        qInfo() << "INSERT: dialog | nom =" << (nameEdit == nullptr ? QString() : nameEdit->text())
                << "| lignes =" << (table == nullptr ? -1 : table->rowCount())
                << "| bouton =" << (ok == nullptr ? QString() : ok->text())
                << "| OK actif =" << (ok != nullptr && ok->isEnabled());

        if (table != nullptr)
        {
            for (int row = 0; row < table->rowCount(); ++row)
            {
                const auto *edit = qobject_cast<QLineEdit *>(table->cellWidget(row, 1));
                qInfo() << "INSERT: ligne" << row
                        << "| objet =" << (table->item(row, 0) == nullptr ? QString() : table->item(row, 0)->text())
                        << "| nom =" << (edit == nullptr ? QString() : edit->text());
            }

            const QHash<QString, QString> untouched = dialog.Table();
            bool same = untouched.size() == proposal.table.size();
            for (auto it = proposal.table.constBegin(); same && it != proposal.table.constEnd(); ++it)
            {
                same = untouched.value(it.key()) == it.value();
            }
            qInfo() << "INSERT: dialog | table rendue identique a la proposition =" << same
                    << "| noms =" << untouched.size();

            if (table->rowCount() > 0)
            {
                auto *first = qobject_cast<QLineEdit *>(table->cellWidget(0, 1));
                const auto state = [&](const QString &label)
                {
                    qInfo() << "INSERT: dialog" << label << "| OK actif =" << (ok != nullptr && ok->isEnabled())
                            << "| phrase =" << (aliasProblem == nullptr ? QString() : aliasProblem->text());
                };

                first->setText(QStringLiteral("Em"));
                state(QStringLiteral("alias deja pris (Em)"));
                first->setText(QStringLiteral("deux mots"));
                state(QStringLiteral("forme refusee"));
                first->setText(QStringLiteral("AliasdeSonde"));
                state(QStringLiteral("alias libre"));
                qInfo() << "INSERT: dialog | table apres une main |"
                        << dialog.Table().value(proposal.aliases.first().name) << "| noms =" << dialog.Table().size();
                first->setText(QString());
                state(QStringLiteral("alias vide"));
            }
        }
    }

    const OPieceInsert::Prepared prepared = OPieceInsert::Prepare(piece, pattern, proposal, anchorTarget);
    qInfo() << "INSERT: prepare | instance =" << prepared.instanceName << "| bloc pret =" << prepared.IsReady()
            << "| noms renommes =" << prepared.renamed.size() << "| marque posee ="
            << (prepared.renamed.isEmpty() ? QStringLiteral("non") : QStringLiteral("oui"))
            << "| position posee =" << ProbeFirstPosition(prepared.block) << "| ecart ="
            << (ProbeFirstPosition(prepared.block) - ProbeFirstPosition(piece.DrawElement()));

    VAbstractApplication::VApp()->getUndoStack()->push(
        new OPieceInsert::AddPieceFromLibrary(prepared, piece, doc, pattern));

    // Ce que la mesure doit montrer : la formule du témoin suit **ses** points.
    const QDomElement witness = ProbePointByName(doc->documentElement(), QStringLiteral("ZZTemoin__P1"));
    qInfo() << "INSERT: temoin | point ZZTemoin__P1 trouve =" << (not witness.isNull())
            << "| length =" << witness.attribute(QStringLiteral("length"))
            << "| angle =" << witness.attribute(QStringLiteral("angle"));

    const QDomElement hostEm = ProbePointByName(doc->documentElement(), QStringLiteral("Em"));
    const QDomElement hostPro = ProbePointByName(doc->documentElement(), QStringLiteral("Pro"));
    qInfo() << "INSERT: l'hote n'a pas bouge | Em nue =" << (not hostEm.isNull())
            << "| Pro nue =" << (not hostPro.isNull()) << "| blocs =" << blocksBefore << "->"
            << doc->CountPatternBlockTags();

    const auto nanCount = [pattern]()
    {
        int nan = 0;
        const QHash<QString, QSharedPointer<VInternalVariable>> *variables = pattern->DataVariables();
        for (auto it = variables->constBegin(); it != variables->constEnd(); ++it)
        {
            if (not it.value().isNull() && it.value()->GetValue() != nullptr && std::isnan(*it.value()->GetValue()))
            {
                ++nan;
            }
        }
        return nan;
    };
    qInfo() << "INSERT: apres insertion | variables =" << pattern->DataVariables()->size() << "| NaN =" << nanCount()
            << "| sections =" << ProbeSections(doc->documentElement());

    // Un seul Ctrl+Z : les quatre sections ET le bloc, ou rien.
    VAbstractApplication::VApp()->getUndoStack()->undo();
    const QString sectionsAfterUndo = ProbeSections(doc->documentElement());
    qInfo() << "INSERT: apres un Ctrl+Z | blocs =" << doc->CountPatternBlockTags() << "| sections identiques ="
            << (sectionsAfterUndo == sectionsBefore) << "| bloc temoin encore la ="
            << (not ProbePointByName(doc->documentElement(), QStringLiteral("ZZTemoin__P1")).isNull());
    if (sectionsAfterUndo != sectionsBefore)
    {
        qInfo() << "INSERT:   AVANT =" << sectionsBefore;
        qInfo() << "INSERT:   APRES =" << sectionsAfterUndo;
    }
    qInfo() << "INSERT: apres un Ctrl+Z | NaN =" << nanCount() << "| variables =" << pattern->DataVariables()->size();

    // Et un Ctrl+Y remet tout, sans rien laisser de plus.
    VAbstractApplication::VApp()->getUndoStack()->redo();
    const QDomElement witnessAgain = ProbePointByName(doc->documentElement(), QStringLiteral("ZZTemoin__P1"));
    qInfo() << "INSERT: apres un Ctrl+Y | temoin =" << (not witnessAgain.isNull())
            << "| length =" << witnessAgain.attribute(QStringLiteral("length")) << "| blocs ="
            << doc->CountPatternBlockTags() << "| sections =" << ProbeSections(doc->documentElement());

    const QString mancheAfterText = ProbeSectionText(mancheBefore);
    qInfo() << "INSERT: la Manche de l'hote, texte identique ="
            << (mancheBeforeText == QStringLiteral("<absente>")
                    ? QStringLiteral("(pas de Manche)")
                    : QString::number(mancheAfterText == mancheBeforeText));

    const QString outFile = QDir::tempPath() + QStringLiteral("/ostara_piece_insert.ostara");
    QString saveError;
    const bool saved = doc->SaveDocument(outFile, saveError);
    qInfo() << "INSERT: enregistrer =" << saved << "|" << saveError << "|" << outFile;
}
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
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_CITATIONS"))
        {
            ProbeCitations(doc, pattern); // TEMPORARY — sonde, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_PICK"))
        {
            ProbePick(doc, qEnvironmentVariable("OSTARA_PIECE_PICK")); // TEMPORARY — sonde, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_INSERT"))
        {
            // TEMPORARY — sonde, à retirer
            const QPointF beyond = PositionBeyondDrawn();
            ProbePieceInsert(doc, pattern, qEnvironmentVariable("OSTARA_PIECE_INSERT"),
                             QPointF(VAbstractValApplication::VApp()->fromPixel(beyond.x()),
                                     VAbstractValApplication::VApp()->fromPixel(beyond.y())));
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_WITNESS"))
        {
            ProbeWitnessFormulas(doc, qEnvironmentVariable("OSTARA_PIECE_WITNESS")); // TEMPORARY — sonde, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_NEWPP_DIALOG"))
        {
            // TEMPORARY — sonde, à retirer
            ProbeNewPieceDialog(doc, pattern, qEnvironmentVariable("OSTARA_NEWPP_DIALOG"));
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_LIBRARY"))
        {
            ProbePieceLibrary(qEnvironmentVariable("OSTARA_PIECE_LIBRARY")); // TEMPORARY — sonde, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_IMPORT"))
        {
            ProbePieceImport(doc, pattern); // TEMPORARY — sonde de faisabilité, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_INCREMENT_GROUPS_UI"))
        {
            ProbeIncrementGroupsUi(doc, pattern); // TEMPORARY — sonde de faisabilité, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_GROUPS"))
        {
            ProbeIncrementGroups(doc, pattern); // TEMPORARY — sonde de faisabilité, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_BUILD"))
        {
            ProbeBuildPiece(doc, pattern); // TEMPORARY — sonde de faisabilité, à retirer
        }
        else if (qEnvironmentVariableIsSet("OSTARA_PIECE_PROBE"))
        {
            ProbeRuntimeBlockInsertion(pattern, doc); // TEMPORARY — sonde de faisabilité, à retirer
        }

        QCoreApplication::exit(V_EX_OK); // close program after processing in console mode
    }
}
