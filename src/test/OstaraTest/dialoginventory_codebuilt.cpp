/************************************************************************
 **
 **  @file   dialoginventory_codebuilt.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  Les dialogues que rien ne peut mesurer depuis un `.ui` : celles qui construisent leur
 **          interface en code. Elles sont bâties ici par leur propre constructeur, dans une
 **          application de test, et mesurées comme les autres.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
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

#include "dialoginventory_cases.h"

#include "dialogs/oaboutdialog.h"
#include "core/odetailssavedmarginsdialog.h"
#include "dialogs/../core/odetailssavedmarginsdialog.h"
#include "dialogs/oexportcuttingfiledialog.h"
#include "vmisc/../vtools/dialogs/support/dialognameobject.h"
#include "dialogs/olayergroupdialog.h"
#include "dialogs/olicencedialog.h"
#include "dialogs/olibraryfolderdialog.h"
#include "dialogs/ochooseaxisdirectiondialog.h"
#include "dialogs/ochooselayerdialog.h"
#include "dialogs/ocoversizerangedialog.h"
#include "dialogs/ogradedrangedialog.h"
#include "dialogs/DialogMeasurementLink.h"
#include "dialogs/oexportbatchdialog.h"
#include "dialogs/oexportbatchpage.h"
#include "dialogs/oannotationdialog.h"
#include "dialogs/ogratedsizestylesdialog.h"
#include "oexportplanimage.h"
#include "oexportplanimagewriter.h"
#include "widgets/MeasurementWorkspace/DialogRangeGroups.h"
#include "widgets/MeasurementWorkspace/dialogues/DialogImportStructure.h"
#include "dialogs/oexportbatchpreflightdialog.h"
#include "dialogs/oexportfiledialog.h"
#include "dialogs/oexportimagedialog.h"
#include "dialogs/oexportimagelistdialog.h"
#include "dialogs/oexportpdfdialog.h"
#include "dialogs/oexportmodedialog.h"
#include "core/ogradedsizeranges.h"
#include "dialogs/opatternfolderdialog.h"
#include "dialogs/opatternlibrarydialog.h"
#include "dialogs/opatternopendialog.h"
#include "dialogs/opiecelibrarydialog.h"
#include "dialogs/opiecegroupdialog.h"
#include "dialogs/osavepatterndialog.h"
#include "dialogs/otransferlayersdialog.h"
#include "dialogs/owaitdialog.h"
#include "dialogs/DialogManageMeasurements.h"
#include "dialogs/DialogNewMeasurements.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "widgets/MeasurementWorkspace/dialogues/DialogSaveMeasurements.h"

#include "vcontainer.h"
#include "vdimensions.h"
#include "vmeasurements.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// A pattern container with nothing in it: the dialogs measured here build their rows from their own
/// parameters, not from a pattern, so an empty one is the honest minimum.
auto EmptyContainer() -> VContainer *
{
    static const Unit patternUnit = Unit::Cm;
    static VContainer container(nullptr, &patternUnit, VContainer::UniqueNamespace());
    return &container;
}

//---------------------------------------------------------------------------------------------------------------------
/// A table with two axes and a range of sizes — ce qu'une patronnière a sous la main quand elle ouvre
/// le mode Export. Sans elle, les pages mesurées sont vides et la mesure ne dit rien du vrai cas.
auto SampleMeasurements() -> QSharedPointer<VMeasurements>
{
    static QSharedPointer<VMeasurements> measurements;
    if (measurements.isNull())
    {
        const Unit unit = Unit::Cm;
        const QSharedPointer<VXMeasurementDimension> x = QSharedPointer<VXMeasurementDimension>::create(unit, 50, 200, 6);
        x->SetBaseValue(176);
        const QSharedPointer<VYMeasurementDimension> y = QSharedPointer<VYMeasurementDimension>::create(unit, 22, 72, 2);
        y->SetBaseValue(50);
        const QVector<MeasurementDimension_p> dimensions{x, y};
        measurements = QSharedPointer<VMeasurements>::create(unit, dimensions, EmptyContainer());
    }
    return measurements;
}

//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
auto EmptyMeasurements() -> QSharedPointer<VMeasurements>
{
    return SampleMeasurements();
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto AllCodeBuiltCases() -> QVector<DialogCase>
{
    return {
        {"OAboutDialog", [](QDialog &dialog) { new OAboutDialog(&dialog); }},
        {"OLicenceDialog", [](QDialog &dialog) { new OLicenceDialog(&dialog); }},
        {"OWaitDialog",
         [](QDialog &dialog) { new OWaitDialog(QStringLiteral("Preparing the export…"), &dialog); }},
        {"OTransferLayersDialog", [](QDialog &dialog) { new OTransferLayersDialog(&dialog); }},
        {"OLayerGroupDialog", [](QDialog &dialog) { new OLayerGroupDialog(&dialog); }},
        {"OLibraryFolderDialog",
         [](QDialog &dialog) { new OLibraryFolderDialog(QStringLiteral("/tmp"), &dialog); }},
        {"OPatternFolderDialog",
         [](QDialog &dialog) { new OPatternFolderDialog(QStringLiteral("/tmp"), &dialog); }},
        {"OPatternOpenDialog",
         [](QDialog &dialog) { new OPatternOpenDialog(QStringLiteral("/tmp"), &dialog); }},
        {"OSavePatternDialog",
         [](QDialog &dialog)
         { new OSavePatternDialog(QStringLiteral("/tmp"), QStringLiteral("pattern.ostara"), QString(), &dialog); }},
        {"OPieceLibraryDialog",
         [](QDialog &dialog) { new OPieceLibraryDialog(QStringLiteral("/tmp"), &dialog); }},
        {"OPatternLibraryDialog",
         [](QDialog &dialog) { new OPatternLibraryDialog(QStringLiteral("/tmp"), &dialog); }},
        {"OPieceGroupDialog",
         [](QDialog &dialog) { new OPieceGroupDialog(EmptyContainer(), 3, &dialog); }},
        {"DialogManageMeasurements",
         [](QDialog &dialog) { new DialogManageMeasurements(&dialog); }},
        {"DialogNewMeasurements", [](QDialog &dialog) { new DialogNewMeasurements(&dialog); }},
        {"DialogOpenMeasurements", [](QDialog &dialog) { new DialogOpenMeasurements(&dialog); }},
        {"DialogSaveMeasurements",
         [](QDialog &dialog)
         {
             new DialogSaveMeasurements(QStringLiteral("omt"), QStringLiteral("measurements.omt"), QString(), &dialog);
         }},
        {"OExportModeDialog",
         [](QDialog &dialog)
         {
             new OExportModeDialog(EmptyContainer(), EmptyMeasurements(), OGradedSizeRanges::Context{}, QStringList{},
                                   QVector<OExportFavorite>{}, &dialog);
         }},
        {"OExportFileDialog",
         [](QDialog &dialog)
         {
             new OExportFileDialog(QStringLiteral("plan"), QStringLiteral("/tmp"), QStringLiteral("pdf"), &dialog);
         }},
        {"OExportBatchDialog",
         [](QDialog &dialog)
         {
             new OExportBatchDialog(QVector<OExportBatchPage::Row>{}, QVector<OExportBatchPage::Row>{},
                                    QVector<OExportBatchPage::Row>{},
                                    QStringLiteral("/tmp/ostara-inventory-nowhere"), &dialog);
         }},
        {"OChooseAxisDirectionDialog",
         [](QDialog &dialog) { new OChooseAxisDirectionDialog(OChooseAxisDirectionDialog::Config{}, &dialog); }},
        {"OChooseLayerDialog", [](QDialog &dialog) { new OChooseLayerDialog(OChooseLayerDialog::Config{}, &dialog); }},
        {"DialogMeasurementLink", [](QDialog &dialog) { new DialogMeasurementLink(QString(), &dialog); }},
        {"DialogImportStructure", [](QDialog &dialog) { new DialogImportStructure({}, {}, &dialog); }},
        {"OGradedRangeDialog",
         [](QDialog &dialog)
         {
             OGradedRangeDialog::Config config;
             config.canonicalPath = QStringLiteral("/tmp/measurements.omt");
             config.axisNames = {QStringLiteral("Height"), QStringLiteral("Size")};
             config.axisValues = {{QStringLiteral("Height"), {QStringLiteral("172"), QStringLiteral("176")}},
                                  {QStringLiteral("Size"), {QStringLiteral("50"), QStringLiteral("52")}}};
             new OGradedRangeDialog(config, &dialog);
         }},
        {"OCoverSizeRangeDialog",
         [](QDialog &dialog)
         {
             OCoverSizeRangeDialog::Config config;
             config.canonicalPath = QStringLiteral("/tmp/measurements.omt");
             config.axisNames = {QStringLiteral("Height"), QStringLiteral("Size")};
             config.axisValues = {{QStringLiteral("Height"), {QStringLiteral("172"), QStringLiteral("176")}},
                                  {QStringLiteral("Size"), {QStringLiteral("50"), QStringLiteral("52")}}};
             new OCoverSizeRangeDialog(config, &dialog);
         }},
        {"OAnnotationDialog", [](QDialog &dialog) { new OAnnotationDialog(EmptyContainer(), &dialog); }},
        {"OGradedSizeStylesDialog",
         [](QDialog &dialog) { new OGradedSizeStylesDialog({QStringLiteral("50"), QStringLiteral("52")}, {}, &dialog); }},
        {"OExportPdfDialog",
         [](QDialog &dialog) { new OExportPdfDialog(QStringLiteral("plan"), QStringLiteral("/tmp"), &dialog); }},
        {"OExportImageDialog",
         [](QDialog &dialog) { new OExportImageDialog(QStringLiteral("/tmp"), OExportPlanImage::Plan{}, true, &dialog); }},
        {"OExportImageListDialog",
         [](QDialog &dialog)
         { new OExportImageListDialog({}, OExportPlanImageWriter::Format::Png, &dialog); }},
        {"ODetailsSavedMarginsDialog", [](QDialog &dialog) { new ODetailsSavedMarginsDialog(&dialog); }},
        {"DialogNameObject", [](QDialog &dialog) { new DialogNameObject({}, EmptyContainer(), &dialog); }},
        {"DialogRangeGroups",
         [](QDialog &dialog)
         {
             QVector<RangeGroup> groups;
             new DialogRangeGroups(QStringLiteral("Height"), {QStringLiteral("172"), QStringLiteral("176")}, groups,
                                   &dialog);
         }},
        {"OExportCuttingFileDialog",
         [](QDialog &dialog) { new OExportCuttingFileDialog(OExportOutput{}, QStringLiteral("/tmp"), &dialog); }},
        {"OExportBatchPreflightDialog",
         [](QDialog &dialog) { new OExportBatchPreflightDialog(EmptyContainer(), true, false, &dialog); }},
    };
}
