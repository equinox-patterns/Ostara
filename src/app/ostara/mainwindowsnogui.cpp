/************************************************************************
 **
 **  @file   mainwindowsnogui.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 5, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "mainwindowsnogui.h"
#include "../ifc/xml/vvitconverter.h"
#include "../ifc/xml/vvstconverter.h"
#include "../vdxf/libdxfrw/drw_base.h"
#include "../vformat/vmeasurements.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutexporter.h"
#include "../vlayout/vlayoutgenerator.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/dialogs/dialogexporttocsv.h"
#include "../vmisc/qxtcsvmodel.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/othumbnaildata.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "core/vapplication.h"
#include "dialogs/dialoglayoutprogress.h"
#include "dialogs/dialoglayoutsettings.h"
#include "dialogs/dialogsavelayout.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"
#include "widgets/InfoBlock/oinfoblockexport.h"
#include <QAction>
#include <QSet>

#include <memory>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QMessageBox>
#include <QPageSize>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinterInfo>
#include <QProcess>
#include <QProgressDialog>
#include <QToolButton>
#include <QtConcurrent>
#include <QtMath>
#include <QtSvg>
#include <functional>

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wmissing-prototypes")
QT_WARNING_DISABLE_INTEL(1418)

Q_LOGGING_CATEGORY(vMainNoGUIWindow, "v.mainnoguiwindow") // NOLINT

QT_WARNING_POP

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto CreateLayoutPath(const QString &path) -> bool
{
    bool usedNotExistedDir = true;
    QDir dir(path);
    dir.setPath(path);
    if (not dir.exists(path))
    {
        usedNotExistedDir = dir.mkpath(QChar('.'));
    }
    return usedNotExistedDir;
}

//---------------------------------------------------------------------------------------------------------------------
void RemoveLayoutPath(const QString &path, bool usedNotExistedDir)
{
    if (usedNotExistedDir)
    {
        QDir const dir(path);
        dir.rmpath(QChar('.'));
    }
}

//---------------------------------------------------------------------------------------------------------------------
#if !defined(V_NO_ASSERT)
Q_DECL_UNUSED void InsertGlobalContours(const QList<QGraphicsScene *> &scenes, const QList<QGraphicsItem *> &gcontours);
void InsertGlobalContours(const QList<QGraphicsScene *> &scenes, const QList<QGraphicsItem *> &gcontours)
{
    if (scenes.size() != gcontours.size())
    {
        return;
    }

    for (int i = 0; i < scenes.size(); ++i)
    {
        scenes.at(i)->addItem(gcontours.at(i));
    }
}
#endif
} // namespace

//---------------------------------------------------------------------------------------------------------------------
MainWindowsNoGUI::MainWindowsNoGUI(QWidget *parent)
  : VAbstractMainWindow(parent),
    m_watcher(new QFileSystemWatcher(this)),
    pattern(new VContainer(VAbstractApplication::VApp()->TrVars(),
                           VAbstractValApplication::VApp()->patternUnitsP(),
                           valentinaNamespace))
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    ,
    m_taskbarButton(new QWinTaskbarButton(this))
#endif
{
    InitTempLayoutScene();

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarButton->setWindow(this->windowHandle());
    m_taskbarProgress = m_taskbarButton->progress();
    m_taskbarProgress->setMinimum(0);
#endif

    m_layoutSettings->SetParentWidget(this);
}

//---------------------------------------------------------------------------------------------------------------------
MainWindowsNoGUI::~MainWindowsNoGUI()
{
    delete m_unreadPatternMessage.data();
    delete m_mouseCoordinate.data();
    delete tempSceneLayout;
    delete pattern;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ToolLayoutSettings(bool checked)
{
    auto *tButton = qobject_cast<QAction *>(this->sender());
    SCASSERT(tButton != nullptr)

    if (checked)
    {
        VLayoutGenerator lGenerator;

        DialogLayoutSettings layout(&lGenerator, this);
        if (layout.exec() == QDialog::Rejected)
        {
            tButton->setChecked(false);
            return;
        }
        m_layoutSettings->SetLayoutPrinterName(layout.SelectedPrinter());
        GenerateLayout(lGenerator);
        tButton->setChecked(false);
    }
    else
    {
        tButton->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::GenerateLayout(VLayoutGenerator &lGenerator) -> bool
{
    lGenerator.SetDetails(listDetails);

    QElapsedTimer timer;
    timer.start();

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTimer *progressTimer = nullptr;
#endif

    QSharedPointer<DialogLayoutProgress> progress;
    if (VApplication::IsGUIMode())
    {
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_taskbarProgress->setVisible(true);
        m_taskbarProgress->setValue(0);
        m_taskbarProgress->setMaximum(lGenerator.GetNestingTime() * 60);
        progressTimer = new QTimer(this);
        connect(progressTimer, &QTimer::timeout, this,
                [this, timer]() { m_taskbarProgress->setValue(static_cast<int>(timer.elapsed() / 1000)); });
        progressTimer->start(1000);
#endif

        progress = QSharedPointer<DialogLayoutProgress>(
            new DialogLayoutProgress(timer, lGenerator.GetNestingTimeMSecs(), this));

        connect(progress.data(), &DialogLayoutProgress::Abort, &lGenerator, &VLayoutGenerator::Abort);
        connect(progress.data(), &DialogLayoutProgress::Timeout, &lGenerator, &VLayoutGenerator::Timeout);

        progress->Start();
    }
    else
    {
        // Because the progress bar dialog will not terminate nesting we must create separate timer for this
        auto *progressTimer = new QTimer(this);
        connect(progressTimer, &QTimer::timeout, this,
                [timer, &lGenerator, progressTimer]()
                {
                    const auto timeout = static_cast<int>(lGenerator.GetNestingTimeMSecs() - timer.elapsed());

                    if (timeout <= 1000)
                    {
                        lGenerator.Timeout();
                        progressTimer->stop();
                        progressTimer->deleteLater();
                    }
                });
        progressTimer->start(1s);
    }

    LayoutErrors nestingState = LayoutErrors::NoError;

    auto IsTimeout = [&progress, &lGenerator, timer, &nestingState]()
    {
        if (timer.hasExpired(lGenerator.GetNestingTimeMSecs()))
        {
            if (nestingState != LayoutErrors::EmptyPaperError)
            {
                nestingState = LayoutErrors::Timeout;
            }

            if (VApplication::IsGUIMode())
            {
                progress->Finished();
            }
            return true;
        }
        return false;
    };

    bool rotationUsed = false;
    int rotatate = 1;
    lGenerator.SetShift(-1); // Trigger first shift calulation
    lGenerator.SetRotate(false);
    vsizetype papersCount = INT_MAX;
    qreal efficiency = 0;
    bool hasResult = false;

    QCoreApplication::processEvents();

#ifdef LAYOUT_DEBUG
    const QString path = QDir::homePath() + QStringLiteral("/LayoutDebug");
    QDir debugDir(path);
    debugDir.removeRecursively();
    debugDir.mkpath(path);
#endif

    forever
    {
        if (IsTimeout())
        {
            break;
        }

        {
            QEventLoop wait;
            QFutureWatcher<void> fw;
            connect(qApp, &QCoreApplication::aboutToQuit, &fw,
                    [&fw]()
                    {
                        fw.cancel();
                        fw.waitForFinished();
                    });
            QObject::connect(&fw, &QFutureWatcher<void>::finished, &wait, &QEventLoop::quit);
            fw.setFuture(
                QtConcurrent::run([&lGenerator, timer, nestingState]()
                                  { lGenerator.Generate(timer, lGenerator.GetNestingTimeMSecs(), nestingState); }));
            wait.exec();
        }

        if (IsTimeout())
        {
            break;
        }

        nestingState = lGenerator.State();

        switch (nestingState)
        {
            case LayoutErrors::NoError:
                if (lGenerator.PapersCount() <= papersCount)
                {
                    const qreal layoutEfficiency = lGenerator.LayoutEfficiency();
                    if (efficiency < layoutEfficiency || lGenerator.PapersCount() < papersCount)
                    {
                        efficiency = layoutEfficiency;
                        if (VApplication::IsGUIMode())
                        {
                            progress->Efficiency(efficiency);
                        }

                        CleanLayout();
                        m_layoutSettings->SetLayoutPapers(lGenerator.GetPapersItems());      // Blank sheets
                        m_layoutSettings->SetLayoutDetails(lGenerator.GetAllDetailsItems()); // All details items
                        detailsOnLayout = lGenerator.GetAllDetails();                        // All details items
                        m_layoutSettings->SetLayoutShadows(CreateShadows(m_layoutSettings->LayoutPapers()));
                        m_layoutSettings->SetLayoutPortrait(lGenerator.IsPortrait());
                        m_layoutSettings->SetLayoutScenes(CreateScenes(m_layoutSettings->LayoutPapers(),
                                                                       m_layoutSettings->LayoutShadows(),
                                                                       m_layoutSettings->LayoutDetails()));
#if !defined(V_NO_ASSERT)
                        // Uncomment to debug, shows global contour
//                        gcontours = lGenerator.GetGlobalContours(); // uncomment for debugging
//                        InsertGlobalContours(scenes, gcontours); // uncomment for debugging
#endif
                        if (VApplication::IsGUIMode())
                        {
                            PrepareSceneList(PreviewQuatilty::Fast);
                        }
                        m_layoutSettings->SetIgnorePrinterMargins(not lGenerator.IsUsePrinterFields());
                        m_layoutSettings->SetLayoutMargins(lGenerator.GetPrinterFields());
                        m_layoutSettings->SetLayoutPaperSize(
                            QSizeF(lGenerator.GetPaperWidth(), lGenerator.GetPaperHeight()));
                        m_layoutSettings->SetAutoCropLength(lGenerator.GetAutoCropLength());
                        m_layoutSettings->SetAutoCropWidth(lGenerator.GetAutoCropWidth());
                        m_layoutSettings->SetUnitePages(lGenerator.IsUnitePages());
                        m_layoutSettings->SetLayoutStale(false);
                        m_layoutSettings->SetBoundaryTogetherWithNotches(lGenerator.IsBoundaryTogetherWithNotches());
                        papersCount = lGenerator.PapersCount();
                        hasResult = true;
                        qDebug() << "Layout efficiency: " << efficiency;
                    }
                    else
                    {
                        if (lGenerator.IsRotationNeeded())
                        {
                            lGenerator.SetRotate(true);
                            lGenerator.SetRotationNumber(++rotatate);
                            rotationUsed = true;
                        }
                    }
                }
                else
                {
                    if (lGenerator.IsRotationNeeded())
                    {
                        lGenerator.SetRotate(true);
                        lGenerator.SetRotationNumber(++rotatate);
                        rotationUsed = true;
                    }
                }
                lGenerator.SetShift(lGenerator.GetShift() / 2.0);
                break;
            case LayoutErrors::EmptyPaperError:
                if (lGenerator.IsRotationNeeded())
                {
                    if (not rotationUsed)
                    {
                        lGenerator.SetRotate(true);
                        lGenerator.SetRotationNumber(++rotatate);
                        rotationUsed = true;
                    }
                    else
                    {
                        lGenerator.SetShift(lGenerator.GetShift() / 2.0);
                        rotationUsed = false;
                    }
                }
                else
                {
                    lGenerator.SetShift(lGenerator.GetShift() / 2.0);
                }
                break;
            case LayoutErrors::Timeout:
            case LayoutErrors::PrepareLayoutError:
            case LayoutErrors::ProcessStoped:
            case LayoutErrors::TerminatedByException:
            default:
                break;
        }

        if (nestingState == LayoutErrors::PrepareLayoutError || nestingState == LayoutErrors::ProcessStoped ||
            nestingState == LayoutErrors::TerminatedByException)
        {
            break;
        }

        if (nestingState == LayoutErrors::NoError && not qFuzzyIsNull(lGenerator.GetEfficiencyCoefficient()) &&
            efficiency >= lGenerator.GetEfficiencyCoefficient() &&
            (not lGenerator.IsPreferOneSheetSolution() || lGenerator.PapersCount() == 1))
        {
            break;
        }

        if (IsTimeout())
        {
            break;
        }
    }

    if (VApplication::IsGUIMode())
    {
        PrepareSceneList(PreviewQuatilty::Slow);
        progress->Finished();
    }

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if (VApplication::IsGUIMode())
    {
        progressTimer->stop();
        m_taskbarProgress->setVisible(false);
    }
#endif

    if (VApplication::IsGUIMode())
    {
        QApplication::alert(this);
    }

    if (hasResult && nestingState != LayoutErrors::ProcessStoped)
    {
        return true;
    }

    ShowLayoutError(nestingState);
    if (not VApplication::IsGUIMode())
    {
        QCoreApplication::exit(V_EX_DATAERR);
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ShowLayoutError(const LayoutErrors &state)
{
    switch (state)
    {
        case LayoutErrors::NoError:
            return;
        case LayoutErrors::PrepareLayoutError:
            qCritical() << tr("Couldn't prepare data for creation layout");
            break;
        case LayoutErrors::EmptyPaperError:
            qCritical() << tr("One or more pattern pieces are bigger than the paper format you selected. Please, "
                              "select a bigger paper format.");
            break;
        case LayoutErrors::Timeout:
            qCritical() << tr("Timeout.");
            break;
        case LayoutErrors::TerminatedByException:
            qCritical() << tr("Process has been stoped because of exception.");
            break;
        case LayoutErrors::ProcessStoped:
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportFMeasurementsToCSV()
{
    QString const fileName = CSVFilePath();

    if (fileName.isEmpty())
    {
        return;
    }

    DialogExportToCSV dialog(this);
    dialog.SetWithHeader(VAbstractApplication::VApp()->Settings()->GetCSVWithHeader());
    dialog.SetSelectedMib(VAbstractApplication::VApp()->Settings()->GetCSVCodec());
    dialog.SetSeparator(VAbstractApplication::VApp()->Settings()->GetCSVSeparator());

    if (dialog.exec() == QDialog::Accepted)
    {
        ExportFMeasurementsToCSVData(fileName, dialog.IsWithHeader(), dialog.GetSelectedMib(), dialog.GetSeparator());

        VAbstractApplication::VApp()->Settings()->SetCSVSeparator(dialog.GetSeparator());
        VAbstractApplication::VApp()->Settings()->SetCSVCodec(dialog.GetSelectedMib());
        VAbstractApplication::VApp()->Settings()->SetCSVWithHeader(dialog.IsWithHeader());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportData(const QVector<VLayoutPiece> &listDetails)
{
    if (const LayoutExportFormats format = m_dialogSaveLayout->Format();
        format == LayoutExportFormats::DXF_AAMA || format == LayoutExportFormats::DXF_ASTM ||
        format == LayoutExportFormats::RLD || format == LayoutExportFormats::HPGL ||
        format == LayoutExportFormats::HPGL2 || format == LayoutExportFormats::HPGL_PLT ||
        format == LayoutExportFormats::HPGL2_PLT)
    {
        if (m_dialogSaveLayout->Mode() == Draw::Layout)
        {
            for (int i = 0; i < detailsOnLayout.size(); ++i)
            {
                const QString index = detailsOnLayout.size() > 1 ? QString::number(i + 1) : QString();
                const QString name = m_dialogSaveLayout->Path() + '/'_L1 + m_dialogSaveLayout->FileName() + index
                                     + VLayoutExporter::ExportFormatSuffix(m_dialogSaveLayout->Format());

                auto *paper = qgraphicsitem_cast<QGraphicsRectItem *>(m_layoutSettings->LayoutPapers().at(i));
                SCASSERT(paper != nullptr)

                ExportApparelLayout(detailsOnLayout.at(i), name, paper->rect().size().toSize());
            }
        }
        else
        {
            ExportDetailsAsApparelLayout(listDetails);
        }
    }
    else
    {
        if (m_dialogSaveLayout->Mode() == Draw::Layout)
        {
            ExportFlatLayout(m_layoutSettings->LayoutScenes(), m_layoutSettings->LayoutPapers(),
                             m_layoutSettings->LayoutShadows(), m_layoutSettings->LayoutDetails(),
                             m_layoutSettings->IgnorePrinterMargins(), m_layoutSettings->LayoutMargins());
        }
        else
        {
            ExportDetailsAsFlatLayout(listDetails);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportFlatLayout(const QList<QGraphicsScene *> &scenes, const QList<QGraphicsItem *> &papers,
                                        const QList<QGraphicsItem *> &shadows,
                                        const QList<QList<QGraphicsItem *>> &details, bool ignorePrinterFields,
                                        const QMarginsF &margins)
{
    const QString path = m_dialogSaveLayout->Path();
    bool const usedNotExistedDir = CreateLayoutPath(path);
    if (not usedNotExistedDir)
    {
        qCritical() << tr("Can't create a path");
        return;
    }

    VAbstractValApplication::VApp()->ValentinaSettings()->SetPathLayoutExport(path);

    if (const LayoutExportFormats format = m_dialogSaveLayout->Format();
        format == LayoutExportFormats::PDFTiled && m_dialogSaveLayout->Mode() == Draw::Layout)
    {
        const QString name = path + '/'_L1 + m_dialogSaveLayout->FileName();
        PdfTiledFile(name);
    }
    else
    {
        ExportScene(scenes, papers, shadows, details, ignorePrinterFields, margins);
    }

    RemoveLayoutPath(path, usedNotExistedDir);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportDetailsAsFlatLayout(const QVector<VLayoutPiece> &listDetails)
{
    if (listDetails.isEmpty())
    {
        return;
    }

    QScopedPointer<QGraphicsScene> const scene(new QGraphicsScene());

    QList<QGraphicsItem *> list;
    list.reserve(listDetails.count());

    const bool svgFormat = (m_dialogSaveLayout->Format() == LayoutExportFormats::SVG);

    for (auto piece : listDetails)
    {
        QGraphicsItem *item = piece.GetItem(m_dialogSaveLayout->IsTextAsPaths(),
                                            m_dialogSaveLayout->IsBoundaryTogetherWithNotches(), false);
        qreal diff = 0;
        if (piece.IsForceFlipping())
        {
            const qreal x = item->boundingRect().x();
            piece.Mirror();
            item = piece.GetItem(m_dialogSaveLayout->IsTextAsPaths(),
                                 m_dialogSaveLayout->IsBoundaryTogetherWithNotches(), false);
            diff = item->boundingRect().x() - x;
        }

        if (piece.IsForceFlipping())
        {
            item->setPos(piece.GetMx() - diff, piece.GetMy());
        }
        else
        {
            item->setPos(piece.GetMx(), piece.GetMy());
        }

        // On the canvas a grouped piece masks its own grainline (the group holder shows the
        // group's) and masks its own fold line unless this member is the one that carries the
        // group's fold line. Mirror that in the SVG export by hiding those children.
        if (svgFormat)
        {
            const quint32 gid = pattern->GetPieceGroupId(piece.GetId());
            if (gid != NULL_ID)
            {
                const OPieceGroup &group = pattern->DataPieceGroups()->value(gid);
                const bool foldCarrier = (group.m_foldLineMemberId == piece.GetId());
                const QList<QGraphicsItem *> children = item->childItems();
                for (QGraphicsItem *child : children)
                {
                    if (child->type() == VGraphicsFillItem::Type)
                    {
                        child->hide();
                    }
                    else if (!foldCarrier && (child->type() == VGraphicsFoldLineItem::Type
                                              || child->type() == QGraphicsSimpleTextItem::Type))
                    {
                        child->hide();
                    }
                }
            }
        }

        list.append(item);
    }

    // --- Groups (SVG only) ------------------------------------------------
    // On the canvas a grouped piece masks its own information blocks and the
    // group (OPieceGroupHolder) shows its own blocks. Add a geometry-less
    // placeholder per group (it carries the group's grainline and, at export,
    // the group's blocks) so the SVG writer emits them at the right origin,
    // like the holder (origin = union of the member boxes).
    QList<int> groupPlaceholderIdx;  // aligned: index of the placeholder in `list`
    QList<int> groupFirstMemberIdx;  // index in `list` of the group's first member
    QList<quint32> groupIds;
    QHash<int, QVector<int>> groupMembers;  // placeholder index -> member piece indices
    if (svgFormat)
    {
        const QHash<quint32, OPieceGroup> *const groups = pattern->DataPieceGroups();
        if (groups != nullptr && !groups->isEmpty())
        {
            for (auto it = groups->cbegin(); it != groups->cend(); ++it)
            {
                const OPieceGroup &group = it.value();
                int firstMember = -1;
                int present = 0;
                QVector<int> gMembers;
                for (int gi = 0; gi < listDetails.size(); ++gi)
                {
                    if (group.pieceIds.contains(listDetails.at(gi).GetId()))
                    {
                        ++present;
                        gMembers.append(gi);
                        if (firstMember < 0)
                        {
                            firstMember = gi;
                        }
                    }
                }
                if (present == 0 || firstMember < 0)
                {
                    continue;
                }
                QRectF box;
                bool have = false;
                for (int gi = 0; gi < listDetails.size(); ++gi)  // members only live among the source pieces
                {
                    if (group.pieceIds.contains(listDetails.at(gi).GetId()))
                    {
                        const QRectF r = QRectF(list.at(gi)->pos() + list.at(gi)->boundingRect().topLeft(),
                                                list.at(gi)->boundingRect().size());
                        box = have ? box.united(r) : r;
                        have = true;
                    }
                }
                auto *holder = new QGraphicsPathItem();
                holder->setPos(box.topLeft());

                // Group grainline: reproduce OPieceGroupHolderItem::RefreshGrainline semantics,
                // but as a plain path child of the placeholder so the SVG writer serializes it
                // (it only knows path items). Shape/path built exactly like a piece grainline.
                const VGrainlineData &gl = group.m_glGrainline;
                if (gl.IsEnabled() && gl.IsVisible())
                {
                    QPointF glPos = gl.GetPos();  // stored positions are relative to the group origin
                    if (qFuzzyIsNull(glPos.x()) && qFuzzyIsNull(glPos.y()))
                    {
                        QGraphicsItem *const firstMemberItem = list.at(firstMember);
                        glPos = firstMemberItem->pos() + firstMemberItem->boundingRect().center()
                                - box.topLeft();
                    }

                    qreal rotationDeg = 0.0;
                    const QString rotationFormula = gl.GetRotation();
                    if (!rotationFormula.isEmpty())
                    {
                        bool ok = false;
                        const qreal value = rotationFormula.toDouble(&ok);
                        rotationDeg = ok ? value : 0.0;
                    }
                    const qreal lengthCm = gl.GetLength().isEmpty() ? 10.0 : gl.GetLength().toDouble();
                    const qreal lengthPx = ToPixel(lengthCm, *pattern->GetPatternUnit());

                    const VPieceGrainline grainline(glPos, lengthPx, qDegreesToRadians(rotationDeg),
                                                    gl.GetArrowType());
                    const GrainlineShape shape = grainline.Shape();
                    if (!shape.isEmpty())
                    {
                        auto *grainlineItem = new VGraphicsFillItem(VLayoutPiece::GrainlinePath(shape), holder);
                        grainlineItem->SetCustomPen(true);
                        QPen pen = grainlineItem->pen();
                        pen.setWidthF(VAbstractApplication::VApp()->Settings()->WidthHairLine());
                        pen.setCapStyle(Qt::RoundCap);
                        pen.setJoinStyle(Qt::RoundJoin);
                        grainlineItem->setPen(pen);
                    }
                }

                const int gIdx = static_cast<int>(list.size());
                groupPlaceholderIdx.append(gIdx);
                groupFirstMemberIdx.append(firstMember);
                groupIds.append(it.key());
                groupMembers.insert(gIdx, gMembers);
                list.append(holder);
            }
        }
    }

    for (auto *item : list)
    {
        scene->addItem(item);
    }

    QList<QGraphicsItem *> papers; // Blank sheets
    QRect rect = scene->itemsBoundingRect().toRect();

    const int mx = rect.x();
    const int my = rect.y();

    QTransform matrix;
    matrix = matrix.translate(-mx, -my);

    for (auto *item : list)
    {
        item->setTransform(matrix);
    }

    rect = scene->itemsBoundingRect().toRect();

    if (svgFormat)
    {
        const QString path = m_dialogSaveLayout->Path();
        bool const notExistedDir = CreateLayoutPath(path);
        if (not notExistedDir)
        {
            qCritical() << tr("Can't create a path");
            return;
        }
        VAbstractValApplication::VApp()->ValentinaSettings()->SetPathLayoutExport(path);

        Q_RELAXED_CONSTEXPR qreal marginPx = ToPixel(1, Unit::Cm);
        QMarginsF const svgMargins(marginPx, marginPx, marginPx, marginPx);

        auto blocksFromState = [](const OInfoBlockData &info, const OInfoBlockEffectiveState &state,
                                  const QPointF &centerLocal, bool flipped) -> QVector<OInfoBlockSvg::Block>
        {
            QVector<OInfoBlockSvg::Block> out;
            const QList<int> blockIndexes = state.blockContent.keys();
            for (int blockIndex : blockIndexes)
            {
                const QVector<OCartoucheRenderLine> lines = state.blockContent.value(blockIndex).toVector();
                if (lines.isEmpty())
                {
                    continue;
                }
                const OInfoBlockGeom geom = info.BlockGeom(blockIndex);
                const qreal fontScale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
                const QSizeF natural = OCartouchePainter::NaturalSize(lines);
                const QSizeF size(natural.width() * fontScale, natural.height() * fontScale);
                QPointF topLeft;
                qreal rotation = 0.0;
                if (info.HasStoredGeometry(blockIndex))
                {
                    topLeft = geom.pos;
                    rotation = geom.rotation;
                }
                else
                {
                    topLeft = centerLocal
                              + QPointF(0.0, (blockIndex - 1) * (size.height() + 12.0));
                }
                if (flipped)
                {
                    topLeft.setX(-topLeft.x());
                    rotation = -rotation;
                }
                OInfoBlockSvg::Block block;
                block.lines = lines;
                block.fontScale = fontScale;
                block.rect = QRectF(topLeft, size);
                block.rotation = rotation;
                out.append(block);
            }
            return out;
        };

        auto evalAnnotationRotation = [&](const QString &formula) -> qreal
        {
            // Mirrors VToolSeamAllowance::EvaluateAnnotationRotation: a rotation
            // formula is evaluated against the pattern variables; NaN/Inf → 0°.
            if (formula.isEmpty())
            {
                return 0.0;
            }
            Calculator cal;
            const qreal value = cal.EvalFormula(pattern->DataVariables(), formula);
            return (qIsNaN(value) || qIsInf(value)) ? 0.0 : value;
        };

        auto evalGroupAnnotationRotation = [](const QString &formula) -> qreal
        {
            // Mirrors OPieceGroupHolderItem::EvaluateRotation: a group rotation
            // is a plain angle literal ("90"), not a variable formula.
            if (formula.isEmpty())
            {
                return 0.0;
            }
            bool ok = false;
            const qreal value = formula.toDouble(&ok);
            return ok ? value : 0.0;
        };

        // Build the per-annotation SVG records exactly like the canvas item:
        // the box is at least the natural content size × fontScale and the text
        // style is the resolver-resolved render style.
        auto annotationsFromState = [](const OAnnotationData &annData, const QPointF &centerLocal,
                                       bool flipped,
                                       const std::function<qreal(const QString &)> &evalRotation)
            -> QVector<OInfoBlockSvg::Annotation>
        {
            QVector<OInfoBlockSvg::Annotation> out;
            const QList<OAnnotation> annotations = annData.Annotations();
            out.reserve(annotations.size());
            for (const OAnnotation &annotation : annotations)
            {
                if (annotation.text.isEmpty())
                {
                    continue;
                }
                const OAnnotationStyleResolver &resolver = VToolSeamAllowance::AnnotationStyleResolver();
                const OAnnotationRenderStyle style =
                    resolver ? resolver(annotation.templateName) : OAnnotationRenderStyle();

                // Natural content size at fontScale 1.0 (same as OAnnotationItem::SetContent).
                QFont font;
                if (!style.fontFamily.isEmpty())
                {
                    font.setFamily(style.fontFamily);
                }
                if (!style.fontFace.isEmpty())
                {
                    font.setStyleName(style.fontFace);
                }
                if (style.fontSize > 0)
                {
                    font.setPixelSize(style.fontSize);
                }
                const QFontMetricsF fm(font);
                const QRectF br = fm.boundingRect(QRectF(0.0, 0.0, 0.0, 0.0), Qt::AlignLeft,
                                                  annotation.text);

                // Box geometry (OAnnotationItem::SetAnnotationGeom semantics).
                const qreal fontScale = annotation.geom.fontScale > 0.0 ? annotation.geom.fontScale
                                                                        : 1.0;
                const qreal contentW = br.width() * fontScale;
                const qreal contentH = br.height() * fontScale;
                qreal w = annotation.geom.boxWidth;
                qreal h = annotation.geom.boxHeight;
                w = qFuzzyIsNull(w) ? contentW : qMax(w, contentW);
                h = qFuzzyIsNull(h) ? contentH : qMax(h, contentH);

                QPointF topLeft;
                if (qFuzzyIsNull(annotation.geom.pos.x()) && qFuzzyIsNull(annotation.geom.pos.y()))
                {
                    topLeft = centerLocal;  // never moved: auto-place, centered
                }
                else
                {
                    topLeft = annotation.geom.pos;
                }
                qreal rotation = evalRotation(annotation.rotationFormula);
                if (flipped)
                {
                    topLeft.setX(-topLeft.x());
                    rotation = -rotation;
                }

                OInfoBlockSvg::Annotation ann;
                ann.text = annotation.text;
                ann.fontFamily = style.fontFamily;
                ann.fontFace = style.fontFace;
                ann.fontSizePx = style.fontSize;
                ann.textColor = style.textColor.isEmpty() ? QStringLiteral("#000000")
                                                          : style.textColor;
                ann.backgroundColor = style.backgroundColor;
                ann.fontScale = fontScale;
                ann.rotation = rotation;
                ann.rect = QRectF(topLeft, QSizeF(w, h));
                out.append(ann);
            }
            return out;
        };

        // The miniatures the canvas shows, rebuilt from the very sources it reads. One per piece,
        // and one per member for a group — a member's own stays masked while it is linked, exactly
        // as its blocks and annotations do.
        //
        // Nothing here recomputes where a word goes, nor what the drawing is: the record builder is
        // the graded sheet's own as well, so the two documents and both canvases read ONE
        // computation and cannot drift apart.
        auto appendThumbnail = [&](QVector<OInfoBlockSvg::Thumbnail> &out, quint32 pieceId,
                                   const VPiece &piece, const OThumbnailData &data,
                                   const QRectF &hostBounds)
        {
            OInfoBlockSvg::Thumbnail record = OInfoBlockSvg::BuildThumbnail(
                pieceId, piece, data, data.Geom(), hostBounds, data.HasGeometry());
            if (!record.content.isEmpty())
            {
                // The tool is gone, or the reference drawing was never captured: the canvas draws
                // no miniature either, and a block with nothing in it is not a block to write.
                out.append(record);
            }
        };

        // The box a piece's own miniature is measured against when nobody ever sized it: the contour
        // as it is exported, which is the rectangle the canvas measures too.
        auto pieceHostBounds = [](QGraphicsItem *item) -> QRectF
        {
            if (auto *pathItem = qgraphicsitem_cast<QGraphicsPathItem *>(item); pathItem != nullptr)
            {
                return pathItem->path().controlPointRect();
            }
            return item != nullptr ? item->boundingRect() : QRectF();
        };

        QList<QVector<OInfoBlockSvg::Block>> blocksByPiece;
        QList<QVector<OInfoBlockSvg::Annotation>> annotationsByPiece;
        QList<QVector<OInfoBlockSvg::Thumbnail>> thumbnailsByPiece;
        QStringList pieceNames;
        blocksByPiece.reserve(list.size());
        annotationsByPiece.reserve(list.size());
        thumbnailsByPiece.reserve(list.size());
        pieceNames.reserve(list.size());

        for (int i = 0; i < list.size(); ++i)
        {
            QVector<OInfoBlockSvg::Block> pieceBlocks;
            QVector<OInfoBlockSvg::Annotation> pieceAnnotations;
            QVector<OInfoBlockSvg::Thumbnail> pieceThumbnails;
            QString baseName;
            if (i < listDetails.size())
            {
                const VLayoutPiece layout = listDetails.at(i);
                const VPiece source = pattern->DataPieces()->value(layout.GetId());
                baseName = source.GetName();
                // A grouped piece masks its own blocks/annotations on the canvas:
                // emit none, the group placeholder below carries them.
                const bool member = pattern->GetPieceGroupId(layout.GetId()) != NULL_ID;
                if (!member)
                {
                    const OInfoBlockEffectiveState state =
                        OInfoBlockResolver::ResolveEffectiveState(source, doc, pattern);
                    const QPointF centerLocal = list.at(i)->boundingRect().center();
                    pieceBlocks = blocksFromState(source.GetInfoBlockData(), state, centerLocal,
                                                  layout.IsForceFlipping());
                    pieceAnnotations = annotationsFromState(source.GetAnnotationData(), centerLocal,
                                                            layout.IsForceFlipping(),
                                                            evalAnnotationRotation);
                    if (source.GetThumbnailData().IsActive())
                    {
                        appendThumbnail(pieceThumbnails, layout.GetId(), source,
                                        source.GetThumbnailData(), pieceHostBounds(list.at(i)));
                    }
                }
            }
            else
            {
                const int gpos = static_cast<int>(groupPlaceholderIdx.indexOf(i));
                const quint32 gid = groupIds.at(gpos);
                const OPieceGroup &group = pattern->DataPieceGroups()->value(gid);
                baseName = group.name;
                const OInfoBlockEffectiveState state =
                    OInfoBlockResolver::ResolveEffectiveState(group, doc, pattern);
                const QPointF origin = list.at(i)->mapToScene(QPointF(0.0, 0.0));
                const QPointF firstCenter =
                    list.at(groupFirstMemberIdx.at(gpos))->sceneBoundingRect().center();
                pieceBlocks = blocksFromState(group.GetInfoBlockData(), state, firstCenter - origin,
                                              false);
                pieceAnnotations = annotationsFromState(group.GetAnnotationData(),
                                                        firstCenter - origin, false,
                                                        evalGroupAnnotationRotation);
                // The group's miniatures, in the group's own frame: the canvas holder measures each
                // member's box the same way, which is why the two agree on where a member's
                // miniature sits without either of them storing a second copy of it.
                for (int mi : groupMembers.value(i))
                {
                    if (mi < 0 || mi >= listDetails.size() || list.at(mi) == nullptr)
                    {
                        continue;
                    }
                    const quint32 memberId = listDetails.at(mi).GetId();
                    if (const OThumbnailData memberThumb = group.MemberThumbnail(memberId);
                        memberThumb.IsActive())
                    {
                        const QRectF memberBox =
                            list.at(mi)->sceneBoundingRect().translated(-origin.x(), -origin.y());
                        appendThumbnail(pieceThumbnails, memberId,
                                        pattern->DataPieces()->value(memberId), memberThumb,
                                        memberBox);
                    }
                }
            }
            pieceNames.append(baseName.isEmpty() ? QStringLiteral("piece_%1").arg(i + 1) : baseName);
            blocksByPiece.append(pieceBlocks);
            annotationsByPiece.append(pieceAnnotations);
            thumbnailsByPiece.append(pieceThumbnails);
        }

        const QString name = path + '/'_L1 + m_dialogSaveLayout->FileName()
                             + VLayoutExporter::ExportFormatSuffix(m_dialogSaveLayout->Format());
        OInfoBlockSvg::WriteSvg(name, list, blocksByPiece, annotationsByPiece, thumbnailsByPiece,
                                pieceNames, rect, svgMargins, m_dialogSaveLayout->GetXScale(),
                                m_dialogSaveLayout->GetYScale(), tr("Pattern"),
                                doc->GetDescription().toHtmlEscaped(),
                                m_dialogSaveLayout->IsTextAsPaths(),
                                m_dialogSaveLayout->IsShowGrainline(), groupMembers);

        RemoveLayoutPath(path, notExistedDir);
        return;
    }

    auto *paper = new QGraphicsRectItem(rect);
    paper->setPen(QPen(Qt::black, 1));
    paper->setBrush(QBrush(Qt::white));
    papers.append(paper);

    QList<QList<QGraphicsItem *>> details; // All details
    details.append(list);

    QList<QGraphicsItem *> const shadows = CreateShadows(papers);
    QList<QGraphicsScene *> const scenes = CreateScenes(papers, shadows, details);

    const bool ignorePrinterFields = false;
    Q_RELAXED_CONSTEXPR qreal margin = ToPixel(1, Unit::Cm);
    ExportFlatLayout(scenes, papers, shadows, details, ignorePrinterFields, QMarginsF(margin, margin, margin, margin));

    qDeleteAll(scenes); // Scene will clear all other items
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportApparelLayout(const QVector<VLayoutPiece> &details, const QString &name,
                                           const QSize &size) const
{
    const QString path = m_dialogSaveLayout->Path();
    bool const usedNotExistedDir = CreateLayoutPath(path);
    if (not usedNotExistedDir)
    {
        qCritical() << tr("Can't create a path");
        return;
    }

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetPathLayoutExport(path);
    const LayoutExportFormats format = m_dialogSaveLayout->Format();

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    VLayoutExporter exporter;

    QT_WARNING_POP

    exporter.SetFileName(name);
    exporter.SetImageRect(QRectF(0, 0, size.width(), size.height()));
    exporter.SetXScale(m_dialogSaveLayout->GetXScale());
    exporter.SetYScale(m_dialogSaveLayout->GetYScale());
    exporter.SetBinaryDxfFormat(m_dialogSaveLayout->IsBinaryDXFFormat());
    exporter.SetShowGrainline(m_dialogSaveLayout->IsShowGrainline());
    exporter.SetBoundaryTogetherWithNotches(m_dialogSaveLayout->IsBoundaryTogetherWithNotches());

    switch (format)
    {
        case LayoutExportFormats::DXF_ASTM:
            exporter.SetDxfVersion(DRW::AC1009);
            exporter.SetDxfApparelCompatibility(m_dialogSaveLayout->DxfCompatibility());
            exporter.ExportToASTMDXF(details);
            break;
        case LayoutExportFormats::DXF_AAMA:
            exporter.SetDxfVersion(DRW::AC1009);
            exporter.SetDxfApparelCompatibility(m_dialogSaveLayout->DxfCompatibility());
            exporter.ExportToAAMADXF(details);
            break;
        case LayoutExportFormats::RLD:
            exporter.ExportToRLD(details);
            break;
        case LayoutExportFormats::HPGL:
        case LayoutExportFormats::HPGL_PLT:
            exporter.SetSingleLineFont(settings->GetSingleLineFonts());
            exporter.SetSingleStrokeOutlineFont(settings->GetSingleStrokeOutlineFont());
            exporter.SetPenWidth(qCeil(settings->WidthHairLine()));
            exporter.ExportToHPGL(details);
            break;
        case LayoutExportFormats::HPGL2:
        case LayoutExportFormats::HPGL2_PLT:
            exporter.SetSingleLineFont(settings->GetSingleLineFonts());
            exporter.SetSingleStrokeOutlineFont(settings->GetSingleStrokeOutlineFont());
            exporter.SetPenWidth(qCeil(settings->WidthHairLine()));
            exporter.ExportToHPGL2(details);
            break;
        default:
            qDebug() << "Can't recognize file type." << Q_FUNC_INFO;
            break;
    }

    RemoveLayoutPath(path, usedNotExistedDir);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportDetailsAsApparelLayout(QVector<VLayoutPiece> listDetails)
{
    if (listDetails.isEmpty())
    {
        return;
    }

    QScopedPointer<QGraphicsScene> const scene(new QGraphicsScene());

    QList<QGraphicsItem *> list;
    list.reserve(listDetails.count());
    for (int i = 0; i < listDetails.count(); ++i)
    {
        VLayoutPiece piece = listDetails.at(i);
        QGraphicsItem *item = piece.GetItem(m_dialogSaveLayout->IsTextAsPaths(),
                                            m_dialogSaveLayout->IsBoundaryTogetherWithNotches(), false);
        qreal diff = 0;
        if (piece.IsForceFlipping())
        {
            const qreal x = item->boundingRect().x();
            piece.Mirror();
            delete item;
            item = piece.GetItem(m_dialogSaveLayout->IsTextAsPaths(),
                                 m_dialogSaveLayout->IsBoundaryTogetherWithNotches(), false);
            diff = item->boundingRect().x() - x;
        }

        QTransform moveMatrix = piece.GetMatrix();
        if (piece.IsForceFlipping())
        {
            item->setPos(piece.GetMx() - diff, piece.GetMy());
            moveMatrix = moveMatrix.translate(-piece.GetMx() + diff, piece.GetMy());
        }
        else
        {
            item->setPos(piece.GetMx(), piece.GetMy());
            moveMatrix = moveMatrix.translate(piece.GetMx(), piece.GetMy());
        }
        listDetails[i].SetMatrix(moveMatrix);
        list.append(item);
    }

    for (auto *item : list)
    {
        scene->addItem(item);
    }

    QRect rect = scene->itemsBoundingRect().toRect();

    const int mx = rect.x();
    const int my = rect.y();

    QTransform matrix;
    matrix = matrix.translate(-mx, -my);

    for (auto *item : list)
    {
        item->setTransform(matrix);
    }

    rect = scene->itemsBoundingRect().toRect();

    for (int i = 0; i < listDetails.count(); ++i)
    {
        QTransform moveMatrix = listDetails.at(i).GetMatrix();
        if (listDetails.at(i).IsForceFlipping())
        {
            moveMatrix = moveMatrix.translate(mx, -my);
        }
        else
        {
            moveMatrix = moveMatrix.translate(-mx, -my);
        }
        listDetails[i].SetMatrix(moveMatrix);
    }

    const QString name = m_dialogSaveLayout->Path() + '/' + m_dialogSaveLayout->FileName()
                         + VLayoutExporter::ExportFormatSuffix(m_dialogSaveLayout->Format());

    ExportApparelLayout(listDetails, name, rect.size());
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::PrintPreviewOrigin()
{
    m_layoutSettings->SetFileName(FileName());
    m_layoutSettings->PrintPreviewOrigin();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::PrintPreviewTiled()
{
    m_layoutSettings->SetFileName(FileName());

    if (not m_dialogSaveLayout.isNull())
    {
        m_layoutSettings->SetTiledMargins(m_dialogSaveLayout->GetTiledMargins());
        m_layoutSettings->SetTiledPDFOrientation(m_dialogSaveLayout->GetTiledPageOrientation());

        VAbstractLayoutDialog::PaperSizeTemplate const tiledFormat = m_dialogSaveLayout->GetTiledPageFormat();
        m_layoutSettings->SetTiledPDFPaperSize(VAbstractLayoutDialog::GetTemplateSize(tiledFormat, Unit::Mm));
    }
    else
    {
        VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
        m_layoutSettings->SetTiledMargins(settings->GetTiledPDFMargins(Unit::Mm));
        m_layoutSettings->SetTiledPDFOrientation(settings->GetTiledPDFOrientation());
        m_layoutSettings->SetTiledPDFPaperSize(
            QSizeF(settings->GetTiledPDFPaperWidth(Unit::Mm), settings->GetTiledPDFPaperHeight(Unit::Mm)));
    }

    m_layoutSettings->SetWatermarkPath(
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->GetWatermarkPath()));
    m_layoutSettings->PrintPreviewTiled();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::PrintOrigin()
{
    m_layoutSettings->SetFileName(FileName());
    m_layoutSettings->PrintOrigin();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::PrintTiled()
{
    m_layoutSettings->SetFileName(FileName());

    if (not m_dialogSaveLayout.isNull())
    {
        m_layoutSettings->SetTiledMargins(m_dialogSaveLayout->GetTiledMargins());
        m_layoutSettings->SetTiledPDFOrientation(m_dialogSaveLayout->GetTiledPageOrientation());

        VAbstractLayoutDialog::PaperSizeTemplate const tiledFormat = m_dialogSaveLayout->GetTiledPageFormat();
        m_layoutSettings->SetTiledPDFPaperSize(VAbstractLayoutDialog::GetTemplateSize(tiledFormat, Unit::Mm));
    }
    else
    {
        VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
        m_layoutSettings->SetTiledMargins(settings->GetTiledPDFMargins(Unit::Mm));
        m_layoutSettings->SetTiledPDFOrientation(settings->GetTiledPDFOrientation());
        m_layoutSettings->SetTiledPDFPaperSize(
            QSizeF(settings->GetTiledPDFPaperWidth(Unit::Mm), settings->GetTiledPDFPaperHeight(Unit::Mm)));
    }

    m_layoutSettings->SetWatermarkPath(
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->GetWatermarkPath()));
    m_layoutSettings->PrintTiled();
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::PrepareDetailsForLayout(const QVector<DetailForLayout> &details,
                                               bool showProgress) -> QVector<VLayoutPiece>
{
    if (details.isEmpty())
    {
        return {};
    }

    // Prepared SERIALLY on the calling (GUI) thread. The previous QtConcurrent::mapped +
    // modal QProgressDialog::exec() version was a correctness hazard: VLayoutPiece::Create
    // re-submits inner QtConcurrent tasks to the same global pool and blocks on them (deadlock
    // risk), the workers read the shared VContainer while the modal nested event loop could
    // re-enter the doc, and Create touches VApp/QSettings from pool threads. Serial execution
    // removes all of those; the outer callers already parallelize at a higher level if needed.
    QVector<VLayoutPiece> layoutDetails;
    layoutDetails.reserve(details.size());

    std::unique_ptr<QProgressDialog> progress;
    if (showProgress && VApplication::IsGUIMode())
    {
        progress = std::make_unique<QProgressDialog>(
            QCoreApplication::translate("MainWindowsNoGUI", "Preparing details for layout"), QString(), 0,
            static_cast<int>(details.size()));
        progress->setWindowModality(Qt::ApplicationModal);
        progress->setWindowTitle(QCoreApplication::translate("MainWindowsNoGUI", "Preparing details for layout"));
        progress->show();
    }

    int done = 0;
    for (const DetailForLayout &data : details)
    {
        auto *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::getTool(data.id));
        SCASSERT(tool != nullptr)
        layoutDetails.append(VLayoutPiece::Create(data.piece, data.id, tool->getData())); // may throw → caller catches
        if (progress != nullptr)
        {
            progress->setValue(++done);
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents); // keep the dialog painted
        }
    }

    if (progress != nullptr)
    {
        progress->close();
    }

    return layoutDetails;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::InitTempLayoutScene()
{
    tempSceneLayout = new VMainGraphicsScene();
    tempSceneLayout->setBackgroundBrush(QBrush(QColor(Qt::gray), Qt::SolidPattern));
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::RecentFileList() const -> QStringList
{
    return VAbstractValApplication::VApp()->ValentinaSettings()->GetRecentFileList();
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::ScenePreview(int i, QSize iconSize, PreviewQuatilty quality) const -> QIcon
{
    QImage image;

    if (auto *paper = qgraphicsitem_cast<QGraphicsRectItem *>(m_layoutSettings->LayoutPapers().at(i)); paper)
    {
        if (quality == PreviewQuatilty::Fast)
        {
            image = QImage(iconSize, QImage::Format_RGB32);
            image.fill(Qt::white);
        }
        else
        {
            const QRectF r = paper->rect();
            // Create the image with the exact size of the shrunk scene
            image =
                QImage(QSize(static_cast<qint32>(r.width()), static_cast<qint32>(r.height())), QImage::Format_RGB32);

            if (not image.isNull())
            {
                image.fill(Qt::white);
                QPainter painter(&image);
                painter.setFont(QFont(QStringLiteral("Arial"), 8, QFont::Normal));
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QPen(Qt::black, VAbstractApplication::VApp()->Settings()->WidthMainLine(), Qt::SolidLine,
                                    Qt::RoundCap, Qt::RoundJoin));
                painter.setBrush(QBrush(Qt::NoBrush));
                m_layoutSettings->LayoutScenes().at(i)->render(&painter, r, r, Qt::IgnoreAspectRatio);
                painter.end();
            }
            else
            {
                qDebug() << "Cannot create image. Size " << r.size() << "too big";

                image = QImage(iconSize, QImage::Format_RGB32);
                image.fill(Qt::white);
            }
        }
    }
    else
    {
        image = QImage(iconSize, QImage::Format_RGB32);
        image.fill(Qt::white);
    }
    return {QBitmap::fromImage(image)};
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::CreateShadows(const QList<QGraphicsItem *> &papers) -> QList<QGraphicsItem *>
{
    QList<QGraphicsItem *> shadows;
    shadows.reserve(papers.size());

    for (auto *paper : papers)
    {
        qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (auto *item = qgraphicsitem_cast<QGraphicsRectItem *>(paper))
        {
            item->rect().getCoords(&x1, &y1, &x2, &y2);
            auto *shadowPaper = new QGraphicsRectItem(QRectF(x1 + 4, y1 + 4, x2 + 4, y2 + 4));
            shadowPaper->setBrush(QBrush(Qt::black));
            shadows.append(shadowPaper);
        }
        else
        {
            shadows.append(nullptr);
        }
    }

    return shadows;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::CreateScenes(const QList<QGraphicsItem *> &papers, const QList<QGraphicsItem *> &shadows,
                                    const QList<QList<QGraphicsItem *>> &details) -> QList<QGraphicsScene *>
{
    QList<QGraphicsScene *> scenes;
    scenes.reserve(papers.size());
    for (int i = 0; i < papers.size(); ++i)
    {
        auto *scene = new VMainGraphicsScene();
        scene->SetNonInteractive(true);
        scene->setBackgroundBrush(QBrush(QColor(Qt::gray), Qt::SolidPattern));
        scene->addItem(shadows.at(i));
        scene->addItem(papers.at(i));

        const QList<QGraphicsItem *> &paperDetails = details.at(i);
        for (const auto &detail : paperDetails)
        {
            scene->addItem(detail);
        }

        scenes.append(scene);
    }

    return scenes;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::PdfTiledFile(const QString &name)
{
    if (not m_dialogSaveLayout.isNull())
    {
        m_layoutSettings->SetTiledMargins(m_dialogSaveLayout->GetTiledMargins());
        m_layoutSettings->SetTiledPDFOrientation(m_dialogSaveLayout->GetTiledPageOrientation());

        VAbstractLayoutDialog::PaperSizeTemplate const tiledFormat = m_dialogSaveLayout->GetTiledPageFormat();
        m_layoutSettings->SetTiledPDFPaperSize(VAbstractLayoutDialog::GetTemplateSize(tiledFormat, Unit::Mm));

        m_layoutSettings->SetXScale(m_dialogSaveLayout->GetXScale());
        m_layoutSettings->SetYScale(m_dialogSaveLayout->GetYScale());
    }
    else
    {
        VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
        m_layoutSettings->SetTiledMargins(QMarginsF(settings->GetTiledPDFMargins(Unit::Mm)));
        m_layoutSettings->SetTiledPDFOrientation(settings->GetTiledPDFOrientation());
        m_layoutSettings->SetTiledPDFPaperSize(
            QSizeF(settings->GetTiledPDFPaperWidth(Unit::Mm), settings->GetTiledPDFPaperHeight(Unit::Mm)));
        m_layoutSettings->SetXScale(1);
        m_layoutSettings->SetYScale(1);
    }

    m_layoutSettings->SetWatermarkPath(
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->GetWatermarkPath()));
    m_layoutSettings->PdfTiledFile(name);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::ExportScene(const QList<QGraphicsScene *> &scenes, const QList<QGraphicsItem *> &papers,
                                   const QList<QGraphicsItem *> &shadows, const QList<QList<QGraphicsItem *>> &details,
                                   bool ignorePrinterFields, const QMarginsF &margins) const
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    VLayoutExporter exporter;

    QT_WARNING_POP

    exporter.SetMargins(margins);
    exporter.SetXScale(m_dialogSaveLayout->GetXScale());
    exporter.SetYScale(m_dialogSaveLayout->GetYScale());
    exporter.SetTitle(tr("Pattern"));
    exporter.SetDescription(doc->GetDescription().toHtmlEscaped());
    exporter.SetIgnorePrinterMargins(ignorePrinterFields);
    exporter.SetBinaryDxfFormat(m_dialogSaveLayout->IsBinaryDXFFormat());
    exporter.SetShowGrainline(m_dialogSaveLayout->IsShowGrainline());
    exporter.SetBoundaryTogetherWithNotches(m_dialogSaveLayout->IsBoundaryTogetherWithNotches());

    for (int i = 0; i < scenes.size(); ++i)
    {
        auto *paper = qgraphicsitem_cast<QGraphicsRectItem *>(papers.at(i));
        if (paper != nullptr)
        {
            const QString index = scenes.size() > 1 ? QString::number(i + 1) : QString();
            const QString name = m_dialogSaveLayout->Path() + '/' + m_dialogSaveLayout->FileName() + index
                                 + VLayoutExporter::ExportFormatSuffix(m_dialogSaveLayout->Format());
            auto *brush = new QBrush();
            brush->setColor(QColor(Qt::white));
            QGraphicsScene *scene = scenes.at(i);
            scene->setBackgroundBrush(*brush);
            shadows[i]->setVisible(false);
            const qreal thinPen = 0.1;
            paper->setPen(QPen(QBrush(Qt::white, Qt::NoBrush), thinPen, Qt::NoPen));

            exporter.SetFileName(name);
            exporter.SetImageRect(paper->rect());

            QPen const defaultPen(Qt::black, VAbstractApplication::VApp()->Settings()->WidthHairLine(), Qt::SolidLine,
                                  Qt::RoundCap, Qt::RoundJoin);

            switch (m_dialogSaveLayout->Format())
            {
                case LayoutExportFormats::SVG:
                    paper->setVisible(false);
                    exporter.SetPen(defaultPen);
                    exporter.ExportToSVG(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::PDF:
                    exporter.SetPen(defaultPen);
                    exporter.ExportToPDF(scene, details.at(i));
                    break;
                case LayoutExportFormats::PNG:
                    exporter.SetPen(defaultPen);
                    exporter.ExportToPNG(scene, details.at(i));
                    break;
                case LayoutExportFormats::OBJ:
                    paper->setVisible(false);
                    exporter.ExportToOBJ(scene);
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::PS:
                    exporter.SetPen(defaultPen);
                    exporter.ExportToPS(scene, details.at(i));
                    break;
                case LayoutExportFormats::EPS:
                    exporter.SetPen(defaultPen);
                    exporter.ExportToEPS(scene, details.at(i));
                    break;
                case LayoutExportFormats::DXF_AC1006_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1006);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1009_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1009);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1012_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1012);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1014_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1014);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1015_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1015);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1018_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1018);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1021_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1021);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1024_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1024);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::DXF_AC1027_Flat:
                    paper->setVisible(false);
                    exporter.SetDxfVersion(DRW::AC1027);
                    exporter.ExportToFlatDXF(scene, details.at(i));
                    paper->setVisible(true);
                    break;
                case LayoutExportFormats::TIF:
                    exporter.SetPen(defaultPen);
                    exporter.ExportToTIF(scene, details.at(i));
                    break;
                default:
                    qDebug() << "Can't recognize file type." << Q_FUNC_INFO;
                    break;
            }
            paper->setPen(QPen(Qt::black, 1));
            brush->setColor(QColor(Qt::gray));
            brush->setStyle(Qt::SolidPattern);
            scenes[i]->setBackgroundBrush(*brush);
            shadows[i]->setVisible(true);
            delete brush;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::FileName() const -> QString
{
    QString fileName;
    VAbstractValApplication::VApp()->GetPatternPath().isEmpty()
        ? fileName = tr("unnamed")
        : fileName = VAbstractValApplication::VApp()->GetPatternPath();
    return QFileInfo(fileName).baseName();
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::ExportFMeasurementsToCSVData(const QString &fileName, bool withHeader, int mib,
                                                    const QChar &separator) const -> bool
{
    QxtCsvModel csv;

    csv.insertSingleColumn(0);
    csv.insertSingleColumn(1);
    csv.insertSingleColumn(2);

    if (withHeader)
    {
        csv.setHeaderText(0, tr("Name"));
        csv.setHeaderText(1, tr("Value"));
        csv.setHeaderText(2, tr("Description"));
    }

    const QVector<VFinalMeasurement> measurements = doc->GetFinalMeasurements();
    VContainer completeData = doc->GetCompleteData();
    completeData.FillPiecesAreas(VAbstractValApplication::VApp()->patternUnits());

    for (int i = 0; i < measurements.size(); ++i)
    {
        const VFinalMeasurement &m = measurements.at(i);

        csv.insertSingleRow(i);
        csv.setText(i, 0, m.name); // name

        if (not m.formula.isEmpty())
        {
            try
            {
                QScopedPointer<Calculator> const cal(new Calculator());
                const qreal result = cal->EvalFormula(completeData.DataVariables(), m.formula);

                csv.setText(i, 1, VAbstractApplication::VApp()->LocaleToString(result)); // value

                if (qIsInf(result) || qIsNaN(result))
                {
                    qCritical(
                        "%s\n\n%s", qUtf8Printable(tr("Export final measurements error.")),
                        qUtf8Printable(
                            tr("Value in line %1 is infinite or NaN. Please, check your calculations.").arg(i + 1)));
                    if (not VApplication::IsGUIMode())
                    {
                        QCoreApplication::exit(V_EX_DATAERR);
                    }
                    return false;
                }
            }
            catch (qmu::QmuParserError &e)
            {
                qCritical("%s\n\n%s", qUtf8Printable(tr("Export final measurements error.")),
                          qUtf8Printable(tr("Parser error at line %1: %2.").arg(i + 1).arg(e.GetMsg())));
                if (not VApplication::IsGUIMode())
                {
                    QCoreApplication::exit(V_EX_DATAERR);
                }
                return false;
            }
        }

        csv.setText(i, 2, m.description); // description
    }

    QString error;
    const bool success = csv.toCSV(fileName, error, withHeader, separator, QTextCodec::codecForMib(mib));

    if (not success)
    {
        qCritical("%s\n\n%s", qUtf8Printable(tr("Export final measurements error.")),
                  qUtf8Printable(tr("File error %1.").arg(error)));
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_CANTCREAT);
        }
    }

    return success;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::OpenMeasurementFile(const QString &patternPath, QString &path) -> QSharedPointer<VMeasurements>
{
    if (path.isEmpty())
    {
        return {};
    }

    const QStringList patternMeasurements = doc->ListMeasurements();
    QString currentPath = path;
    while (true)
    {
        QSharedPointer<VMeasurements> m = LoadMeasurements(patternPath, currentPath, path);
        if (!m || m->Type() == MeasurementsType::Unknown)
        {
            return {};
        }

        const QSet<QString> missingMeasurements = ConvertToSet<QString>(patternMeasurements)
                                                      .subtract(ConvertToSet<QString>(m->ListAll()));
        if (missingMeasurements.isEmpty())
        {
            path = currentPath;
            return m;
        }

        if (!HandleMissingMeasurements(m, missingMeasurements, currentPath))
        {
            return {};
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::LoadMeasurements(const QString &patternPath, const QString &currentPath, const QString &oldPath)
    -> QSharedPointer<VMeasurements>
{
    try
    {
        auto m = QSharedPointer<VMeasurements>(new VMeasurements(pattern));
        m->setXMLContent(currentPath);

        if (m->Type() == MeasurementsType::Unknown)
        {
            throw VException(QCoreApplication::translate("MainWindowsNoGUI", "Measurement file has unknown format."));
        }

        if (currentPath != oldPath)
        {
            VAbstractValApplication::VApp()->SetMeasurementsType(m->Type());
            if (not doc->MPath().isEmpty())
            {
                m_watcher->removePath(AbsoluteMPath(patternPath, doc->MPath()));
            }
            doc->SetMPath(RelativeMPath(patternPath, currentPath));
            m_watcher->addPath(currentPath);
        }

        const QString formatVersion = ApplyConverter(m, currentPath);
        CollectStatistics(m->Type(), formatVersion);

        return m;
    }
    catch (const VException &e)
    {
        qCCritical(vMainNoGUIWindow,
                   "%s\n\n%s\n\n%s",
                   qUtf8Printable(QCoreApplication::translate("MainWindowsNoGUI", "File error.")),
                   qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));

        if (!VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }

        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::ApplyConverter(const QSharedPointer<VMeasurements> &m, const QString &path) const -> QString
{
    // .omt/.otu files are already in the target Ostara format — no conversion needed.
    // Only legacy .vst/.vit files require format upgrade through the converter.
    const QDomElement root = m->documentElement();
    const QString rootTag = root.tagName();
    if (rootTag == VMeasurements::TagOMT || rootTag == VMeasurements::TagOTU)
    {
        // Return the version from the root element attribute (.omt/.otu format)
        if (root.hasAttribute(QStringLiteral("version")))
        {
            return root.attribute(QStringLiteral("version"));
        }
        return QStringLiteral("0.1.0");
    }

    std::unique_ptr<VAbstractMConverter> converter;
    if (m->Type() == MeasurementsType::Multisize)
    {
        converter = std::make_unique<VVSTConverter>(path);
    }
    else
    {
        converter = std::make_unique<VVITConverter>(path);
    }

    m->setXMLContent(converter->Convert());
    return converter->GetFormatVersionStr();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindowsNoGUI::CollectStatistics(MeasurementsType measurementsType, const QString &formatVersion) const
{
    // VGAnalyticsLib removed — no tracking
    Q_UNUSED(measurementsType)
    Q_UNUSED(formatVersion)
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::HandleMissingMeasurements(const QSharedPointer<VMeasurements> &m,
                                                 const QSet<QString> &missing,
                                                 QString &currentPath) -> bool
{
    const QStringList missingList = ConvertToList(missing);

    if (!VApplication::IsGUIMode())
    {
        qCCritical(vMainNoGUIWindow,
                   "%s\n\n%s\n\n%s",
                   qUtf8Printable(QCoreApplication::translate("MainWindowsNoGUI", "File error.")),
                   qUtf8Printable(tr("Measurement file doesn't include all required measurements.")),
                   qUtf8Printable(tr("Please, additionally provide: %1").arg(missingList.join(", "_L1))));
        return false;
    }

    if (const QString message
        = tr("Measurement file doesn't include all required measurements.\n\nMissing: %1\n\nWould you like "
             "to select another file?")
              .arg(missingList.join(", "));
        QMessageBox::question(this, tr("Missing Measurements"), message) != QMessageBox::Yes)
    {
        m->clear();
        return false;
    }

    return SelectNewFile(m, currentPath);
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindowsNoGUI::SelectNewFile(const QSharedPointer<VMeasurements> &m, QString &currentPath) -> bool
{
    QString filter;
    if (m->Type() == MeasurementsType::Multisize)
    {
        filter = tr("Multisize measurements") + " (*.omt);;"_L1 + tr("Individual measurements") + " (*.otu)"_L1;
    }
    else
    {
        filter = tr("Individual measurements") + " (*.otu);;"_L1 + tr("Multisize measurements") + " (*.omt)"_L1;
    }

    const QString dirPath = QFileInfo(currentPath).absolutePath();
    QFileDialog dialog(this, tr("Select Measurement File"), dirPath, filter);
    dialog.selectFile(QFileInfo(currentPath).fileName());
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setOption(QFileDialog::DontUseNativeDialog,
                     VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());

    if (dialog.exec() == QDialog::Accepted)
    {
        currentPath = dialog.selectedFiles().value(0);

        if (const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
            m->Type() == MeasurementsType::Multisize)
        {
            settings->SetPathMultisizeMeasurements(currentPath);
        }
        else
        {
            settings->SetPathIndividualMeasurements(currentPath);
        }
    }
    else
    {
        m->clear();
        return false;
    }

    return true;
}
