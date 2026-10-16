/************************************************************************
 **
 **  @file   vcmdexport.h
 **  @author Alex Zaharov <alexzkhr@gmail.com>
 **  @date   25 8, 2015
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

#ifndef VCMDEXPORT_H
#define VCMDEXPORT_H

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QTextStream>
#include <QVector>
#include <memory>
#include <vector>

#include "dialogs/vabstractlayoutdialog.h"


class VCommandLine;
using VCommandLinePtr = std::shared_ptr<VCommandLine>;
enum class PageOrientation : bool;

//@brief: class used to install export command line options and parse their values
// QCommandLineParser* object must exists until this object alive
class VCommandLine
{
public:
    virtual ~VCommandLine() = default;

    //@brief creates object and applies export related options to parser

    //@brief tests if user enabled test mode from cmd, throws exception if not exactly 1 input VAL file supplied in
    // case test mode enabled
    auto IsTestModeEnabled() const -> bool;

    //@brief Make all parsing warnings into errors. Have effect only in console mode. Use to force Valentina to
    // immediately terminate if a pattern contains a parsing warning.
    auto IsPedantic() const -> bool;

    auto IsNoScalingEnabled() const -> bool;

    //@brief tests if user enabled export from cmd, throws exception if not exactly 1 input VAL file supplied in case
    // export enabled
    auto IsExportEnabled() const -> bool;

    //@brief tests if user enabled export final measurements from cmd, throws exception if not exactly 1 input VAL
    // file supplied in case export enabled
    auto IsExportFMEnabled() const -> bool;

    //@brief returns path to custom measure file or empty string
    auto OptMeasurePath() const -> QString;

    //@brief returns the base name of layout files or empty string if not set
    auto OptBaseName() const -> QString;

    //@brief returns the absolute path to output destination directory or path to application's current directory if
    // not set
    auto OptDestinationPath() const -> QString;

    //@brief returns export type set, defaults 0 - svg
    auto OptExportType() const -> int;

    auto DXFApparelCompatibilityType() const -> int;

    auto IsBinaryDXF() const -> bool;
    auto IsNoGrainline() const -> bool;
    auto IsTextAsPaths() const -> bool;
    auto IsExportOnlyDetails() const -> bool;
    auto IsCSVWithHeader() const -> bool;

    auto ExportXScale() const -> qreal;
    auto ExportYScale() const -> qreal;

    auto CurveApproximationScale() const -> qreal;

    //@brief returns the piece name regex or empty string if not set
    auto OptExportSuchDetails() const -> QString;

    //@brief returns user selected csv codec or empty string if not set
    auto OptCSVCodecName() const -> QString;

    //@brief returns user selected csv separator or empty string if not set
    auto OptCSVSeparator() const -> QChar;

    //@brief returns the destination path for export final measurements or empty string if not set
    auto OptExportFMTo() const -> QString;

    //@brief returns list of user defined materials
    auto OptUserMaterials() const -> QMap<int, QString>;

    // generator creation is moved here ... because most options are for it only, so no need to create extra getters...

    //@brief gets filenames which should be loaded
    auto OptInputFileNames() const -> QStringList;

    auto IsGuiEnabled() const -> bool;

    auto IsSetDimensionA() const -> bool;
    auto IsSetDimensionB() const -> bool;
    auto IsSetDimensionC() const -> bool;

    auto OptDimensionA() const -> int;
    auto OptDimensionB() const -> int;
    auto OptDimensionC() const -> int;

    auto TiledPageMargins() const -> QMarginsF;
    auto OptTiledPaperSize() const -> VAbstractLayoutDialog::PaperSizeTemplate;
    auto OptTiledPageOrientation() const -> PageOrientation;

    // ── Le métrage par lot ────────────────────────────────────────────────────────────────────
    // Voir Modifications/Ostara/ExportMode/MetrageCLI.md.
    //
    // ⚠️ Les longueurs sortent d'ici **en millimètres**, parce que c'est ce que la requête porte.
    // L'utilisatrice, elle, écrit des centimètres — c'est le widget de l'onglet qui parle cm, et
    // ces accesseurs font la même conversion, par la MÊME fonction. Un seul point de conversion.

    //@brief tests if the user asked for a métrage batch, and that exactly one pattern was given
    auto IsMetrageEnabled() const -> bool;

    //@brief the name of the model this run measures, written in the CSV
    auto OptMetrageModel() const -> QString;

    //@brief the "group=member" pairs, one per group
    auto OptMetrageGroups() const -> QStringList;

    //@brief every fabric width asked for, comma separated lists included, in millimetres
    auto OptMetrageFabricWidths() const -> QVector<qreal>;

    //@brief the lining width in millimetres, or 0 when it was not given
    auto OptMetrageLiningWidth() const -> qreal;

    auto IsMetrageLining() const -> bool;
    auto IsMetrageOneWay() const -> bool;
    auto IsMetrageFolded() const -> bool;

    //@brief the space between two pieces, in millimetres (the user writes centimetres)
    auto MetragePieceGap() const -> qreal;

    //@brief the space kept from the edges, in millimetres (the user writes centimetres)
    auto MetrageEdgeMargin() const -> qreal;

protected:
    VCommandLine();

    //@brief returns VAbstractLayoutDialog::PaperSizeTemplate


    //@brief: called in destructor of application, so instance destroyed and new maybe created (never happen scenario
    // though)
    static void Reset();

    //@brief called to create single object, by VApplication only
    static auto Get(const QCoreApplication &app) -> VCommandLinePtr;

private:
    Q_DISABLE_COPY_MOVE(VCommandLine) // NOLINT
    static VCommandLinePtr instance;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    QCommandLineParser parser{};
    bool isGuiEnabled{false};
    friend class VApplication;

    auto FormatSize(const QString &key) const -> VAbstractLayoutDialog::PaperSizeTemplate;

    void InitCommandLineOptions();
    auto IsOptionSet(const QString &option) const -> bool;
    auto OptionValue(const QString &option) const -> QString;
    auto OptionValues(const QString &option) const -> QStringList;



};

#endif // VCMDEXPORT_H
