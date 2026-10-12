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
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "core/vapplication.h"
#include "core/opiecedocument.h"
#include "core/opiecebuilder.h"
#include "core/opiecelibrary.h"
#include <QCoreApplication>
#include <QDir>
#include <QDomElement>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryFile>
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

void ProbeRemapIds(QDomElement &element, VContainer *pattern, QHash<QString, quint32> &idMap, int &remapped)
{
    // Liste blanche des attributs qui portent un id SANS le dire dans leur nom — source : le XSD.
    // Une règle fondée sur la VALEUR est fausse : les ids 1..N se confondent avec les énumérations
    // (sense, axisType, crossPoint, aScale…), et le fichier ne passe plus la validation du schéma.
    static const QStringList namedAttributes{
        QStringLiteral("object"),      QStringLiteral("tool"),        QStringLiteral("point"),
        QStringLiteral("side1"),       QStringLiteral("side2"),       QStringLiteral("curve1"),
        QStringLiteral("curve2"),      QStringLiteral("notch"),       QStringLiteral("firstPoint"),
        QStringLiteral("secondPoint"), QStringLiteral("thirdPoint"),  QStringLiteral("basePoint"),
        QStringLiteral("pShoulder"),   QStringLiteral("p1Line"),      QStringLiteral("p2Line"),
        QStringLiteral("p1Line1"),     QStringLiteral("p1Line2"),     QStringLiteral("p2Line1"),
        QStringLiteral("p2Line2"),     QStringLiteral("spline"),      QStringLiteral("splinePath"),
        QStringLiteral("apex"),        QStringLiteral("extendPoint"), QStringLiteral("notchCurve1"),
        QStringLiteral("notchCurve2"), QStringLiteral("notchPoint"),  QStringLiteral("center"),
        QStringLiteral("arc"),         QStringLiteral("elArc"),       QStringLiteral("curve"),
        QStringLiteral("firstArc"),    QStringLiteral("secondArc"),   QStringLiteral("c1Center"),
        QStringLiteral("c2Center"),    QStringLiteral("tangent"),     QStringLiteral("cCenter"),
        QStringLiteral("point1"),      QStringLiteral("point2"),      QStringLiteral("point3"),
        QStringLiteral("point4"),      QStringLiteral("dartP1"),      QStringLiteral("dartP2"),
        QStringLiteral("dartP3"),      QStringLiteral("baseLineP1"),  QStringLiteral("baseLineP2"),
        QStringLiteral("pSpline"),     QStringLiteral("duplicate"),   QStringLiteral("origin"),
        QStringLiteral("target"),      QStringLiteral("line"),        QStringLiteral("foldLine"),
        QStringLiteral("copy"),        QStringLiteral("departure"),   QStringLiteral("place"),
        QStringLiteral("end"),         QStringLiteral("pleat"),       QStringLiteral("start"),
        QStringLiteral("pivot"),       QStringLiteral("line1"),       QStringLiteral("line2"),
        QStringLiteral("inner"),       QStringLiteral("outer"),       QStringLiteral("cap1"),
        QStringLiteral("cap2"),        QStringLiteral("cap1Ref"),     QStringLiteral("cap2Ref"),
        QStringLiteral("p1"),          QStringLiteral("p2"),          QStringLiteral("path"),
        QStringLiteral("piece"),       QStringLiteral("gradedAlignPoint"),
        QStringLiteral("gradedAlignPointMember")};

    if (element.hasAttributes())
    {
        const QDomNamedNodeMap attrs = element.attributes();
        for (int i = 0; i < attrs.count(); ++i)
        {
            QDomAttr attr = attrs.item(i).toAttr();
            const QString name = attr.name();
            const QString value = attr.value();

            const bool byName = name == QStringLiteral("id")
                                || name.contains(QStringLiteral("id"), Qt::CaseInsensitive)
                                || namedAttributes.contains(name);
            bool isNumber = false;
            value.toUInt(&isNumber);
            if (not byName || not isNumber)
            {
                continue;
            }

            if (not idMap.contains(value))
            {
                idMap.insert(value, pattern->getNextId());
            }
            attr.setValue(QString::number(idMap.value(value)));
            ++remapped;
        }
    }

    for (QDomElement child = element.firstChildElement(); not child.isNull(); child = child.nextSiblingElement())
    {
        ProbeRemapIds(child, pattern, idMap, remapped);
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

// Seconde question, celle du produit : un bloc se relit-il comme une pièce, et le fichier obtenu passe-t-il
// son propre schéma ? Rien ici n'appartient au produit.
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

    const QDomElement source = draws.at(0).toElement();
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

    QHash<QString, quint32> idMap;
    int remapped = 0;
    ProbeRemapIds(clone, pattern, idMap, remapped);

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
            << "| declared ids =" << declared.size() << "| distinct ids remapped =" << idMap.size()
            << "| attributes rewritten =" << remapped << "| names marked =" << namesMarked;

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
