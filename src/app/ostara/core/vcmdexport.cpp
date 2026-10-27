/************************************************************************
 **
 **  @file   vcmdexport.cpp
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

#include "vcmdexport.h"
#include "oexportunitformat.h"
#include "../dialogs/dialogsavelayout.h"
#include "../ifc/xml/vdomdocument.h"
#include "../vmisc/commandoptions.h"
#include "../vmisc/dialogs/dialogexporttocsv.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

VCommandLinePtr VCommandLine::instance = nullptr; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

#define translate(context, source)                                                                                     \
    QCoreApplication::translate((context), (source)) // NOLINT(cppcoreguidelines-macro-usage)

namespace
{
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
} // anonymous namespace

//---------------------------------------------------------------------------------------------------------------------
VCommandLine::VCommandLine()
{
    parser.setApplicationDescription(translate("VCommandLine", "Pattern making program."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("filename"), translate("VCommandLine", "Pattern file."));

    InitCommandLineOptions();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::FormatSize(const QString &key) const -> VAbstractLayoutDialog::PaperSizeTemplate
{
    int ppsize = 0;
    if (IsOptionSet(key))
    {
        ppsize = OptionValue(key).toInt();
    }
    return static_cast<VAbstractLayoutDialog::PaperSizeTemplate>(ppsize);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::Get(const QCoreApplication &app) -> VCommandLinePtr
{
    if (instance == nullptr)
    {
        instance.reset(new VCommandLine());
    }
    instance->parser.process(app);

    // fixme: in case of additional options/modes which will need to disable GUI - add it here too
    instance->isGuiEnabled =
        not(instance->IsExportEnabled() || instance->IsTestModeEnabled() || instance->IsExportFMEnabled()
            || instance->IsMetrageEnabled());

    return instance;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommandLine::Reset()
{
    instance.reset();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsTestModeEnabled() const -> bool
{
    const bool r = IsOptionSet(LONG_OPTION_TEST);
    if (r && parser.positionalArguments().size() != 1)
    {
        qCritical() << translate("VCommandLine", "Test option can be used with single input file only.") << "/n";
        const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
    }
    return r;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsPedantic() const -> bool
{
    // Pedantic doesn't work in GUI mode
    return IsGuiEnabled() ? false : IsOptionSet(LONG_OPTION_PENDANTIC);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsNoScalingEnabled() const -> bool
{
    return IsOptionSet(LONG_OPTION_NO_HDPI_SCALING);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsExportEnabled() const -> bool
{
    const bool r = IsOptionSet(LONG_OPTION_BASENAME);
    if (r && parser.positionalArguments().size() != 1)
    {
        qCritical() << translate("VCommandLine", "Export options can be used with single input file only.") << "/n";
        const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
    }
    return r;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsExportFMEnabled() const -> bool
{
    const bool r = IsOptionSet(LONG_OPTION_CSVEXPORTFM);
    if (r && parser.positionalArguments().size() != 1)
    {
        qCritical() << translate("VCommandLine", "Export options can be used with single input file only.") << "/n";
        const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
    }
    return r;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptMeasurePath() const -> QString
{
    QString measure;
    // ⚠️ Le mode métrage doit être dans cette liste : `--mfile` est ignoré partout ailleurs, et un
    // lot qui chargerait le tableau du patron au lieu de celui demandé mesurerait autre chose que ce
    // qu'on lui a dit.
    if (IsOptionSet(LONG_OPTION_MEASUREFILE)
        && (IsExportEnabled() || IsTestModeEnabled() || IsMetrageEnabled()))
    // todo: don't want yet to allow user set measure file for general loading,
    // because need to fix multiply opened windows as well
    {
        measure = OptionValue(LONG_OPTION_MEASUREFILE);
    }

    return measure;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptBaseName() const -> QString
{
    QString path;
    if (IsExportEnabled())
    {
        path = OptionValue(LONG_OPTION_BASENAME);
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptDestinationPath() const -> QString
{
    QString path;
    // ⚠️ `--destination` n'était lu que par le mode export ; un lot de métrage s'écrivait alors
    // dans le vide, et QFile("") refusait d'ouvrir le moindre CSV.
    if (IsExportEnabled() || IsMetrageEnabled())
    {
        path = OptionValue(LONG_OPTION_DESTINATION);
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptExportType() const -> int
{
    int r = 0;
    if (IsOptionSet(LONG_OPTION_EXP2FORMAT))
    {
        r = OptionValue(LONG_OPTION_EXP2FORMAT).toInt();
    }
    return r;
}

//---------------------------------------------------------------------------------------------------------------------
int VCommandLine::DXFApparelCompatibilityType() const
{
    int r = 0;
    if (IsOptionSet(LONG_OPTION_DXF_APPAREL_COMP))
    {
        r = OptionValue(LONG_OPTION_DXF_APPAREL_COMP).toInt();
    }
    return r;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsBinaryDXF() const -> bool
{
    return IsOptionSet(LONG_OPTION_BINARYDXF);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsNoGrainline() const -> bool
{
    return IsOptionSet(LONG_OPTION_NOGRAINLINE);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsTextAsPaths() const -> bool
{
    return IsOptionSet(LONG_OPTION_TEXT2PATHS);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsExportOnlyDetails() const -> bool
{
    return IsOptionSet(LONG_OPTION_EXPORTONLYDETAILS);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsCSVWithHeader() const -> bool
{
    return IsOptionSet(LONG_OPTION_CSVWITHHEADER);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::ExportXScale() const -> qreal
{
    qreal xs = 1;
    if (IsOptionSet(LONG_OPTION_EXPXSCALE))
    {
        xs = qBound(0.01, OptionValue(LONG_OPTION_EXPXSCALE).toDouble(), 3.0);
    }
    return xs;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::ExportYScale() const -> qreal
{
    qreal ys = 1;
    if (IsOptionSet(LONG_OPTION_EXPYSCALE))
    {
        ys = qBound(0.01, OptionValue(LONG_OPTION_EXPYSCALE).toDouble(), 3.0);
    }
    return ys;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::CurveApproximationScale() const -> qreal
{
    qreal scale = 0;
    if (IsOptionSet(LONG_OPTION_CURVE_APPROXIMATION_SCALE))
    {
        scale = qBound(minCurveApproximationScale,
                       OptionValue(LONG_OPTION_CURVE_APPROXIMATION_SCALE).toDouble(),
                       maxCurveApproximationScale);
    }
    return scale;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptExportSuchDetails() const -> QString
{
    QString path;
    if (IsExportEnabled())
    {
        path = OptionValue(LONG_OPTION_EXPORTSUCHDETAILS);
    }

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptCSVCodecName() const -> QString
{
    return OptionValue(LONG_OPTION_CSVCODEC);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptCSVSeparator() const -> QChar
{
    const QString value = OptionValue(LONG_OPTION_CSVSEPARATOR);
    return not value.isEmpty() ? value.at(0) : QChar();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptExportFMTo() const -> QString
{
    return OptionValue(LONG_OPTION_CSVEXPORTFM);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptUserMaterials() const -> QMap<int, QString>
{
    QMap<int, QString> userMaterials;
    const QStringList values = OptionValues(LONG_OPTION_USER_MATERIAL);
    for (const auto &value : values)
    {
        const QStringList parts = value.split('@');
        if (parts.size() != 2)
        {
            qCritical() << translate("VCommandLine", "Invalid user material '%1'. Separator is missing.").arg(value)
                        << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }

        bool ok = false;
        const int number = parts.constFirst().toInt(&ok);

        if (not ok or number < 1 or number > userMaterialPlaceholdersQuantity)
        {
            qCritical() << translate("VCommandLine", "Invalid user material '%1'. Wrong material number.").arg(value)
                        << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }

        userMaterials.insert(number, parts.constLast());
    }

    return userMaterials;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptInputFileNames() const -> QStringList
{
    return parser.positionalArguments();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsGuiEnabled() const -> bool
{
    return isGuiEnabled;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsSetDimensionA() const -> bool
{
    return IsOptionSet(LONG_OPTION_DIMENSION_A);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsSetDimensionB() const -> bool
{
    return IsOptionSet(LONG_OPTION_DIMENSION_B);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsSetDimensionC() const -> bool
{
    return IsOptionSet(LONG_OPTION_DIMENSION_C);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptDimensionA() const -> int
{
    const QString value = OptionValue(LONG_OPTION_DIMENSION_A);

    bool ok = false;
    if (int const dimensionAValue = value.toInt(&ok); ok && dimensionAValue > 0)
    {
        return dimensionAValue;
    }

    qCritical() << translate("VCommandLine", "Invalid dimension A value.") << "\n";
    const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptDimensionB() const -> int
{
    const QString value = OptionValue(LONG_OPTION_DIMENSION_B);

    bool ok = false;
    if (int const dimensionBValue = value.toInt(&ok); ok && dimensionBValue > 0)
    {
        return dimensionBValue;
    }

    qCritical() << translate("VCommandLine", "Invalid dimension B value.") << "\n";
    const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptDimensionC() const -> int
{
    const QString value = OptionValue(LONG_OPTION_DIMENSION_C);

    bool ok = false;
    if (int const dimensionCValue = value.toInt(&ok); ok && dimensionCValue > 0)
    {
        return dimensionCValue;
    }

    qCritical() << translate("VCommandLine", "Invalid dimension C value.") << "\n";
    const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::TiledPageMargins() const -> QMarginsF
{
    QMarginsF margins(10, 10, 10, 10); // mm

    Unit unit = Unit::Cm;

    if (IsOptionSet(LONG_OPTION_SHIFTUNITS))
    {
        const QString value = OptionValue(LONG_OPTION_SHIFTUNITS);
        if (const auto supportedUnits = QStringList() << unitMM << unitCM << unitINCH;
            not supportedUnits.contains(value))
        {
            qCritical() << translate("VCommandLine", "Unsupported paper units.") << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }
        unit = StrToUnits(value);
    }

    if (IsOptionSet(LONG_OPTION_LEFT_MARGIN))
    {
        bool ok = false;
        qreal const margin = UnitConvertor(OptionValue(LONG_OPTION_LEFT_MARGIN).toDouble(&ok), unit, Unit::Mm);
        if (not ok)
        {
            qCritical() << translate("VCommandLine", "Invalid tiled page left margin.") << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }
        margins.setLeft(margin);
    }

    if (IsOptionSet(LONG_OPTION_RIGHT_MARGIN))
    {
        bool ok = false;
        qreal const margin = UnitConvertor(OptionValue(LONG_OPTION_RIGHT_MARGIN).toDouble(&ok), unit, Unit::Mm);
        if (not ok)
        {
            qCritical() << translate("VCommandLine", "Invalid tiled page right margin.") << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }
        margins.setLeft(margin);
    }

    if (IsOptionSet(LONG_OPTION_TOP_MARGIN))
    {
        bool ok = false;
        qreal const margin = UnitConvertor(OptionValue(LONG_OPTION_TOP_MARGIN).toDouble(&ok), unit, Unit::Mm);
        if (not ok)
        {
            qCritical() << translate("VCommandLine", "Invalid tiled page top margin.") << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }
        margins.setLeft(margin);
    }

    if (IsOptionSet(LONG_OPTION_BOTTOM_MARGIN))
    {
        bool ok = false;
        qreal const margin = UnitConvertor(OptionValue(LONG_OPTION_BOTTOM_MARGIN).toDouble(&ok), unit, Unit::Mm);
        if (not ok)
        {
            qCritical() << translate("VCommandLine", "Invalid tiled page bottom margin.") << "\n";
            const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
        }
        margins.setLeft(margin);
    }

    return margins;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptTiledPaperSize() const -> VAbstractLayoutDialog::PaperSizeTemplate
{
    return FormatSize(LONG_OPTION_TILED_PDF_PAGE_TEMPLATE);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptTiledPageOrientation() const -> PageOrientation
{
    return static_cast<PageOrientation>(not IsOptionSet(LONG_OPTION_TILED_PDF_LANDSCAPE));
}

//----------------------------------------------------------------------------------------------------------------------
void VCommandLine::InitCommandLineOptions()
{
    // keep in mind order here - that is how user will see it, so group-up for usability
    //==================================================================================================================
    parser.addOptions({
        // First, because it is not about what the run does but about how much it says. The debug lines always reach
        // the log file; without this key the console keeps to warnings and criticals.
        {LONG_OPTION_VERBOSE,
         translate("VCommandLine",
                   "Print the debug messages to the console too. They always go to the log file; this key only "
                   "repeats them on the console.")},
        {{SINGLE_OPTION_BASENAME, LONG_OPTION_BASENAME},
         translate("VCommandLine", "The base filename of exported layout files. Use it to enable console export mode."),
         translate("VCommandLine", "The base filename of layout files")},
        {{SINGLE_OPTION_DESTINATION, LONG_OPTION_DESTINATION},
         translate("VCommandLine",
                   "The path to output destination folder. By default the directory at which the "
                   "application was started."),
         translate("VCommandLine", "The destination folder")},
        {{SINGLE_OPTION_MEASUREFILE, LONG_OPTION_MEASUREFILE},
         translate("VCommandLine", "Path to custom measure file (export mode)."),
         translate("VCommandLine", "The measure file")},
        {{SINGLE_OPTION_EXP2FORMAT, LONG_OPTION_EXP2FORMAT},
         translate("VCommandLine", "Number corresponding to output format (default = 0, export mode):")
             + DialogSaveLayout::MakeHelpFormatList(),
         translate("VCommandLine", "Format number"),
         QChar('0')},
        {LONG_OPTION_BINARYDXF, translate("VCommandLine", "Export dxf in binary form.")},
        {LONG_OPTION_DXF_APPAREL_COMP,
         translate("VCommandLine",
                   "Number corresponding to compatibility mode for DXF AAMA/ASTM format (default = 0, export mode):")
             + DialogSaveLayout::MakeHelpDxfApparelCompatibilityList(),
         translate("VCommandLine", "Mode number"),
         QChar('0')},
        {LONG_OPTION_NOGRAINLINE, translate("VCommandLine", "Show/hide grainline when export layout.")},
        {LONG_OPTION_TEXT2PATHS, translate("VCommandLine", "Export text as paths.")},
        {LONG_OPTION_EXPORTONLYDETAILS,
         translate("VCommandLine",
                   "Export only details. Export details as they positioned in the details mode. Any "
                   "layout related options will be ignored.")},
        {LONG_OPTION_EXPORTSUCHDETAILS,
         translate("VCommandLine", "Export only details that match a piece name regex."),
         translate("VCommandLine", "The name regex")},

        {LONG_OPTION_DIMENSION_A,
         translate("VCommandLine", "Set base for dimension A in the multisize measurements units (export mode)."),
         translate("VCommandLine", "The dimension A base")},

        {LONG_OPTION_DIMENSION_B,
         translate("VCommandLine", "Set base for dimension B in the multisize measurements units (export mode)."),
         translate("VCommandLine", "The dimension B base")},

        {LONG_OPTION_DIMENSION_C,
         translate("VCommandLine", "Set base for dimension C in the multisize measurements units (export mode)."),
         translate("VCommandLine", "The dimension C base")},

        {LONG_OPTION_USER_MATERIAL,
         translate("VCommandLine",
                   "Use this option to override user material defined in pattern. The value must be in "
                   "form <number>@<user matrial name>. The number should be in range from 1 to %1. For "
                   "example, "
                   "1@Fabric2. The "
                   "key can be used multiple times. Has no effect in GUI mode.")
             .arg(userMaterialPlaceholdersQuantity),
         translate("VCommandLine", "User material")},
        {LONG_OPTION_CURVE_APPROXIMATION_SCALE,
         translate("VCommandLine",
                   "Override the global curve approximation scale setting from the application preferences. "
                   "The value must be between %1 and %2.")
             .arg(minCurveApproximationScale)
             .arg(maxCurveApproximationScale),
         translate("VCommandLine", "Curve approximation")},
        {{SINGLE_OPTION_LEFT_MARGIN, LONG_OPTION_LEFT_MARGIN},
         translate("VCommandLine",
                   "Page left margin in current units like 3.0 (export mode). If not set will be used "
                   "value from default printer. Or 0 if none printers was found."),
         translate("VCommandLine", "The left margin")},
        {{SINGLE_OPTION_RIGHT_MARGIN, LONG_OPTION_RIGHT_MARGIN},
         translate("VCommandLine",
                   "Page right margin in current units like 3.0 (export mode). If not set will be used "
                   "value from default printer. Or 0 if none printers was found."),
         translate("VCommandLine", "The right margin")},
        {{SINGLE_OPTION_TOP_MARGIN, LONG_OPTION_TOP_MARGIN},
         translate("VCommandLine",
                   "Page top margin in current units like 3.0 (export mode). If not set will be used "
                   "value from default printer. Or 0 if none printers was found."),
         translate("VCommandLine", "The top margin")},
        {{SINGLE_OPTION_BOTTOM_MARGIN, LONG_OPTION_BOTTOM_MARGIN},
         translate("VCommandLine",
                   "Page bottom margin in current units like 3.0 (export mode). If not set will be "
                   "used value from default printer. Or 0 if none printers was found."),
         translate("VCommandLine", "The bottom margin")},
        {LONG_OPTION_EXPXSCALE,
         translate("VCommandLine", "Set horizontal scale factor from 0.01 to 3.0 (default = 1.0, export mode)."),
         translate("VCommandLine", "Horizontal scale")},
        {LONG_OPTION_EXPYSCALE,
         translate("VCommandLine", "Set vertical scale factor from 0.01 to 3.0 (default = 1.0, export mode)."),
         translate("VCommandLine", "Vertical scale")},
        {{SINGLE_OPTION_SHIFTUNITS, LONG_OPTION_SHIFTUNITS},
         translate("VCommandLine", "Layout units (as paper's one except px, export mode). Default units cm."),
         translate("VCommandLine", "The unit")},
        {{SINGLE_OPTION_TEST, LONG_OPTION_TEST},
         translate("VCommandLine",
                   "Run the program in a test mode. The program in this mode loads a single pattern "
                   "file and silently quit without showing the main window. The key have priority "
                   "before key '%1'.")
             .arg(LONG_OPTION_BASENAME)},
        {LONG_OPTION_PENDANTIC,
         translate("VCommandLine",
                   "Make all parsing warnings into errors. Have effect only in console mode. Use to "
                   "force Ostara to immediately terminate if a pattern contains a parsing warning.")},
        {LONG_OPTION_NO_HDPI_SCALING,
         translate("VCommandLine",
                   "Disable high dpi scaling. Call this option if has problem with scaling (by default "
                   "scaling enabled). Alternatively you can use the %1 environment variable.")
             .arg(QStringLiteral("QT_AUTO_SCREEN_SCALE_FACTOR=0"))},
        //==============================================================================================================
        {LONG_OPTION_CSVWITHHEADER, translate("VCommandLine", "Export to csv with header. By default disabled.")},
        {LONG_OPTION_CSVCODEC,
         translate("VCommandLine",
                   "Specify codec that will be used to save data. List of supported codecs provided by "
                   "Qt. Default value depend from system. On Windows, the codec will be based on a "
                   "system locale. On Unix "
                   "systems, the codec will might fall back to using the iconv library if no builtin "
                   "codec for the locale "
                   "can be "
                   "found. Valid values for this installation:")
             + DialogExportToCSV::MakeHelpCodecsList(),
         translate("VCommandLine", "Codec name"),
         QString(QTextCodec::codecForLocale()->name())},
        {LONG_OPTION_CSVSEPARATOR,
         translate("VCommandLine", "Specify csv separator character. Default value is '%1'. Valid characters:")
                 .arg(VCommonSettings::GetDefCSVSeparator())
             + DialogExportToCSV::MakeHelpSeparatorList(),
         translate("VCommandLine", "Separator character"),
         QString(VCommonSettings::GetDefCSVSeparator())},
        {LONG_OPTION_CSVEXPORTFM,
         translate("VCommandLine",
                   "Calling this command enable exporting final measurements. Specify path to csv file "
                   "with final measurements. The path must contain path to directory and name of file. "
                   "It can be "
                   "absolute or "
                   "relatetive. In case of relative path will be used current working directory to calc "
                   "a destination "
                   "path."),
         translate("VCommandLine", "Path to csv file")},
        //==============================================================================================================
        {LONG_OPTION_TILED_PDF_PAGE_TEMPLATE,
         translate("VCommandLine",
                   "Number corresponding to tiled pdf page template (default = 0, export mode with "
                   "tiled pdf format):"),
         translate("VCommandLine", "Template number"),
         QChar('0')},
        {LONG_OPTION_TILED_PDF_LANDSCAPE,
         translate("VCommandLine",
                   "Set tiled page orienatation to landscape (export mode). Default value if not set "
                   "portrait.")},
        //==============================================================================================================
        // Le métrage par lot — voir Modifications/Ostara/ExportMode/MetrageCLI.md
        {LONG_OPTION_METRAGE,
         translate("VCommandLine",
                   "Compute the fabric length of a cutting plan for every complete size of the "
                   "measurements table, for each fabric width of --fabricWidth, and write the table "
                   "as a CSV in --destination. Nothing is ever written to the pattern.")},
        {LONG_OPTION_METRAGE_MODEL,
         translate("VCommandLine", "Name of the model this run measures. Written in the CSV."),
         translate("VCommandLine", "Name")},
        {LONG_OPTION_METRAGE_GROUP,
         translate("VCommandLine",
                   "The member a piece group contributes, as \"Group name=Member name\". Repeat once "
                   "per group: a cutting plan takes exactly one member per group."),
         translate("VCommandLine", "Group=member")},
        {LONG_OPTION_METRAGE_FABRIC_WIDTH,
         translate("VCommandLine",
                   "Fabric width in centimetres. Comma separated to ask for several in one run."),
         translate("VCommandLine", "Width in cm")},
        {LONG_OPTION_METRAGE_LINING_WIDTH,
         translate("VCommandLine", "Lining width in centimetres. Defaults to the fabric width."),
         translate("VCommandLine", "Width in cm")},
        {LONG_OPTION_METRAGE_LINING,
         translate("VCommandLine",
                   "Ask for the lining plan too: one run, two passes, two lengths in the table.")},
        {LONG_OPTION_METRAGE_ONE_WAY,
         translate("VCommandLine", "Lay every piece the same way round.")},
        {LONG_OPTION_METRAGE_FOLDED,
         translate("VCommandLine", "The plan is laid on folded fabric.")},
        {LONG_OPTION_METRAGE_PIECE_GAP,
         translate("VCommandLine", "Space between two pieces, in centimetres (default 0)."),
         translate("VCommandLine", "Space in cm"),
         QStringLiteral("0")},
        {LONG_OPTION_METRAGE_EDGE_MARGIN,
         translate("VCommandLine", "Space kept from the edges, in centimetres (default 0)."),
         translate("VCommandLine", "Space in cm"),
         QStringLiteral("0")},
    });
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsOptionSet(const QString &option) const -> bool
{
    return parser.isSet(option);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptionValue(const QString &option) const -> QString
{
    return parser.value(option);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptionValues(const QString &option) const -> QStringList
{
    return parser.values(option);
}

//---------------------------------------------------------------------------------------------------------------------
// Le métrage par lot — voir Modifications/Ostara/ExportMode/MetrageCLI.md
//
// ⚠️ Ces accesseurs sont le SEUL endroit où les centimètres de l'utilisatrice deviennent les
// millimètres de la requête. Ils passent par OExportUnitFormat::Parse, la même fonction que
// l'onglet Plan de coupe : un chiffre tapé ici et le même chiffre tapé là-bas donnent le même mm.
//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsMetrageEnabled() const -> bool
{
    const bool r = IsOptionSet(LONG_OPTION_METRAGE);
    if (r && parser.positionalArguments().size() != 1)
    {
        qCritical() << translate("VCommandLine", "The metrage options can be used with single input file only.")
                    << "/n";
        const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
    }
    return r;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptMetrageModel() const -> QString
{
    return IsOptionSet(LONG_OPTION_METRAGE_MODEL) ? OptionValue(LONG_OPTION_METRAGE_MODEL).trimmed() : QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptMetrageGroups() const -> QStringList
{
    QStringList groups;
    for (const QString &raw : OptionValues(LONG_OPTION_METRAGE_GROUP))
    {
        const QString pair = raw.trimmed();
        if (not pair.isEmpty())
        {
            groups.append(pair);
        }
    }
    return groups;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptMetrageFabricWidths() const -> QVector<qreal>
{
    QVector<qreal> widths;
    for (const QString &raw : OptionValues(LONG_OPTION_METRAGE_FABRIC_WIDTH))
    {
        for (const QString &part : raw.split(QLatin1Char(',')))
        {
            const QString text = part.trimmed();
            if (text.isEmpty())
            {
                continue;
            }

            bool ok = false;
            const qreal mm = OExportUnitFormat::Parse(text, Unit::Cm, &ok);
            if (ok && mm > 0.0)
            {
                widths.append(mm);
            }
            else
            {
                qCritical() << translate("VCommandLine", "Not a fabric width:") << text << "/n";
                const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
            }
        }
    }
    return widths;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::OptMetrageLiningWidth() const -> qreal
{
    if (not IsOptionSet(LONG_OPTION_METRAGE_LINING_WIDTH))
    {
        return 0.0;
    }

    bool ok = false;
    const qreal mm = OExportUnitFormat::Parse(OptionValue(LONG_OPTION_METRAGE_LINING_WIDTH).trimmed(), Unit::Cm, &ok);
    if (not ok || mm <= 0.0)
    {
        qCritical() << translate("VCommandLine", "Not a lining width.") << "/n";
        const_cast<VCommandLine *>(this)->parser.showHelp(V_EX_USAGE);
    }
    return mm;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsMetrageLining() const -> bool
{
    return IsOptionSet(LONG_OPTION_METRAGE_LINING);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsMetrageOneWay() const -> bool
{
    return IsOptionSet(LONG_OPTION_METRAGE_ONE_WAY);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::IsMetrageFolded() const -> bool
{
    return IsOptionSet(LONG_OPTION_METRAGE_FOLDED);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::MetragePieceGap() const -> qreal
{
    return OExportUnitFormat::Parse(OptionValue(LONG_OPTION_METRAGE_PIECE_GAP), Unit::Cm);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommandLine::MetrageEdgeMargin() const -> qreal
{
    return OExportUnitFormat::Parse(OptionValue(LONG_OPTION_METRAGE_EDGE_MARGIN), Unit::Cm);
}

#undef translate
