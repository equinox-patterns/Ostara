/************************************************************************
 **
 **  @file   vcommonsettings.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 7, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
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

#include "vcommonsettings.h"
#include <QVariant>

#include <QApplication>
#include <QColor>
#include <QDate>
#include <QDir>
#include <QFont>
#include <QDebug>
#include <QGlobalStatic>
#include <QLocale>
#include <QMarginsF>
#include <QMessageLogger>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QVariant>
#include <QtDebug>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

#include "compatibility.h"
#include "def.h"
#include "defglobal.h"
#include "literals.h"

using namespace Qt::Literals::StringLiterals;

const int VCommonSettings::defaultScrollingDuration = 300;
const int VCommonSettings::scrollingDurationMin = 100;
const int VCommonSettings::scrollingDurationMax = 1000;

const int VCommonSettings::defaultScrollingUpdateInterval = 30;
const int VCommonSettings::scrollingUpdateIntervalMin = 10;
const int VCommonSettings::scrollingUpdateIntervalMax = 100;

const qreal VCommonSettings::defaultSensorMouseScale = 2.0;
const qreal VCommonSettings::sensorMouseScaleMin = 1.0;
const qreal VCommonSettings::sensorMouseScaleMax = 10.0;

const qreal VCommonSettings::defaultWheelMouseScale = 45.0;
const qreal VCommonSettings::wheelMouseScaleMin = 1.0;
const qreal VCommonSettings::wheelMouseScaleMax = 100.0;

const qreal VCommonSettings::defaultScrollingAcceleration = 1.3;
const qreal VCommonSettings::scrollingAccelerationMin = 1.0;
const qreal VCommonSettings::scrollingAccelerationMax = 10.0;

const QString VCommonSettings::defaultPieceLabelLocale = QStringLiteral("default");

#ifndef QPRINTENGINE_H
Q_DECLARE_METATYPE(QMarginsF) // NOLINT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-member-function")

namespace
{
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsIndividualMeasurements, ("paths/individual_measurements"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsMultisizeMeasurements, ("paths/standard_measurements"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsPattern, ("paths/pattern"_L1))                         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsManualLayouts, ("paths/manualLayouts"_L1))             // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsLayoutExport, ("paths/layoutExport"_L1))               // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsCustomImage, ("paths/customImage"_L1))                 // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsSVGFonts, ("paths/svgFonts"_L1))                       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsFontCorrections, ("paths/fontCorrections"_L1))         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsKnownMeasurements, ("paths/knownMeasurements"_L1))     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsMeasurementLibrary, ("paths/measurementLibrary"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsCartoucheTemplates, ("paths/cartoucheTemplates"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsPdfTemplates, ("paths/pdfTemplates"_L1))               // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsCoverPageTemplates, ("paths/coverPageTemplates"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsLogos, ("paths/logos"_L1))                              // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsSymbols, ("paths/symbols"_L1))                          // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsSeamAllowances, ("paths/seamAllowances"_L1))            // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsClients, ("paths/clients"_L1))                          // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsAnnotationStyles, ("paths/annotationstyles"_L1))        // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPathsCollections, ("paths/collections"_L1))                  // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingCollectionsSortAscending, ("infoBlock/collectionsSortAscending"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingMeasurementFavorites, ("paths/measurementFavorites"_L1))     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGradedSizeAxis, ("graded/sizeAxis"_L1))                       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGradedFavorites, ("graded/favorites"_L1))                     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingExportFavorites, ("export/favorites"_L1))                     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGradedSizeStyles, ("graded/sizeStyles"_L1))                   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGradedSizeStylesShowInCanvas,
                          ("graded/showSizeStylesInCanvas"_L1))                                                // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingShowActiveLayerOnCanvas,
                          ("draw/showActiveLayerOnCanvas"_L1))                                                 // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationOsSeparator, ("configuration/osSeparator"_L1)) // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationAutosaveState, ("configuration/autosave/state"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationAutosaveTime, ("configuration/autosave/time"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationLocale, ("configuration/locale"_L1))              // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationPieceLabelLocale, ("configuration/pieceLabelLocale"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingKnownMeasurementsId, ("configuration/knownMeasurementsId"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationUnit, ("configuration/unit"_L1)) // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationConfirmItemDeletion,
                          ("configuration/confirm_item_deletion"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationConfirmFormatRewriting,
                          ("configuration/confirm_format_rewriting"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationAskContinueIfLayoutStale,
                          ("configuration/askContinueIfLayoutStale"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationAskRegenerateOutputs,
                          ("configuration/askRegenerateOutputs"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationToolBarStyle, ("configuration/tool_bar_style"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationToolboxIconSizeSmall,
                          ("configuration/toolboxIconSizeSmall"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationFreeCurveMode, ("configuration/freeCurveMode"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationDoubleClickZoomFitBestCurrentPP,
                          ("configuration/doubleClickZoomFitBestCurrentPP"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationInteractiveTools, ("configuration/interactiveTools"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationDontUseNativeDialog,
                          ("configuration/dontUseNativeDialog"_L1))

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsConfigurationCrashEmail, ("configuration/crashEmail"_L1)) // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsConfigurationSendCrashReport, ("configuration/sendCrashReport"_L1))
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsConfigurationAskSendCrashReport,
                          ("configuration/askSendCrashReport"_L1))

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternUndo, ("pattern/undo"_L1))                         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternForceFlipping, ("pattern/forceFlipping"_L1))       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternSewLineOnDrawing, ("pattern/sewLineOnDrawing"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternHideMainPath, ("pattern/hideMainPath"_L1))         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingDoublePassmark, ("pattern/doublePassmark"_L1))            // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternDefaultSeamAllowance, ("pattern/defaultSeamAllowance"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternLabelFont, ("pattern/labelFont"_L1))       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternLabelSVGFont, ("pattern/labelSVGFont"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingCartoucheDefaultTemplate, ("cartouche/defaultTemplate"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPdfDefaultTemplate, ("pdfTemplate/defaultTemplate"_L1))     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingCoverPageDefaultTemplate, ("coverPage/defaultTemplate"_L1)) // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternSingleStrokeOutlineFont, ("pattern/singleStrokeOutlineFont"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternSingleLineFonts, ("pattern/singleLineFonts"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternLineWidth, ("pattern/lineWidth"_L1))             // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternCurveApproximationScale, ("pattern/curveApproximationScale"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternShowCurveDetails, ("pattern/showCurveDetails"_L1))     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternShowSegmentLabels, ("pattern/showSegmentLabels"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternPieceShowMainPath, ("pattern/pieceShowMainPath"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternLabelFontSize, ("pattern/labelFontSize"_L1))           // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternShowAllLabels, ("pattern/showAllLabels"_L1))                 // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternShowAccuracyRadius, ("pattern/showAccuracyRadius"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternUseOpenGLRender, ("pattern/useOpenGLRender"_L1))       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPatternGraphicalOutput, ("pattern/graphicalOutput"_L1))       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsPatternTranslateFormula, ("pattern/translateFormula"_L1))    // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralRecentFileList, ("recentFileList"_L1))                 // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralRecentMeasurementFileList, ("recentMeasurementFileList"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralRestoreFileList, ("restoreFileList"_L1))               // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralGeometry, ("geometry"_L1))                             // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralKMGeometry, ("kmGeometry"_L1))                         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralToolbarsState, ("toolbarsState"_L1))                   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingGeneralKMToolbarsState, ("kmToolbarsState"_L1))               // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingConfigurationThemeMode, ("configuration/themeMode"_L1))       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingPreferenceDialogSize, ("preferenceDialogSize"_L1))            // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingToolSeamAllowanceDialogSize, ("toolSeamAllowanceDialogSize"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingIncrementsDialogSize, ("toolIncrementsDialogSize"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingFormulaWizardDialogSize, ("formulaWizardDialogSize"_L1)) // NOLINT
// NOLINTNEXTLINE
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingFinalMeasurementsDialogSize, ("finalMeasurementsDialogSize"_L1))
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingLayoutSettingsDialogSize, ("layoutSettingsDialogSize"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingDialogSplinePathSize, ("splinePathDialogSize"_L1))           // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingAutomaticallyCheckUpdates, ("automaticallyCheckUpdates"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingLatestSkippedVersion, ("lastestSkippedVersion"_L1))          // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingDateOfLastRemind, ("dateOfLastRemind"_L1))                   // NOLINT

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingCSVWithHeader, ("csv/withHeader"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingCSVCodec, ("csv/withCodec"_L1))         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingCSVSeparator, ("csv/withSeparator"_L1)) // NOLINT

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingScrollingDuration, ("scrolling/duration"_L1))                 // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingScrollingUpdateInterval, ("scrolling/updateInterval"_L1))     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingScrollingSensorMouseScale, ("scrolling/sensorMouseScale"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingScrollingWheelMouseScale, ("scrolling/wheelMouseScale"_L1))   // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingScrollingAcceleration, ("scrolling/acceleration"_L1))         // NOLINT

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingTiledPDFMargins, ("tiledPDF/margins"_L1))         // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingTiledPDFOrientation, ("tiledPDF/orientation"_L1)) // NOLINT

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingWatermarkEditorSize, ("watermarkEditorSize"_L1))     // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingWatermarkCustomColors, ("watermarkCustomColors"_L1)) // NOLINT

Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsStatistictAskCollect, ("askCollect"_L1)) // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsStatistictCollect, ("collect"_L1))       // NOLINT
Q_GLOBAL_STATIC_WITH_ARGS(const QString, settingsStatistictClientID, ("clientID"_L1))     // NOLINT

// Reading settings file is very expensive, cache curve approximation to speed up getting value
qreal curveApproximationCached = -1;        // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
Q_GLOBAL_STATIC(QString, localeCached)      // NOLINT

//---------------------------------------------------------------------------------------------------------------------
// The app bundle's resolved language (used for LANG/QLocale) may differ from the
// user's system language: macOS resolves a bundled app without .lproj folders to
// its CFBundleDevelopmentRegion ("en") even on a French system. Read the system
// preference (AppleLanguages) so a fresh install starts in the user's language.
auto SystemPreferredLocale() -> QString
{
#ifdef Q_OS_MAC
    QProcess proc;
    proc.start(QStringLiteral("/usr/bin/defaults"),
               {QStringLiteral("read"), QStringLiteral("-g"), QStringLiteral("AppleLanguages")});
    if (proc.waitForFinished(1000) && proc.exitStatus() == QProcess::NormalExit)
    {
        const QString out = QString::fromUtf8(proc.readAllStandardOutput());
        // "( "fr-FR", "en" )" → the first quoted entry is the top preferred language.
        const QRegularExpression re(QStringLiteral("\"([^\"]+)\""));
        const QRegularExpressionMatch m = re.match(out);
        if (m.hasMatch())
        {
            QString locale = m.captured(1);
            locale.replace(QLatin1Char('-'), QLatin1Char('_'));  // "fr-FR" → "fr_FR" (qm names)
            return locale;
        }
    }
#endif
    return QLocale::system().name();
}
qreal lineWidthCached = 0;                  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
int labelFontSizeCached = 0;                // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
int pieceShowMainPath = -1;                 // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
int scrollingDurationCached = -1;           // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
int scrollingUpdateIntervalCached = -1;     // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
qreal scrollingSensorMouseScaleCached = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
qreal scrollingWheelMouseScaleCached = -1;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
qreal scrollingAccelerationCached = -1;     // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
int patternTranslateFormulaCached = -1;     // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
int themeModeCached = -1;                   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

} // namespace

Q_GLOBAL_STATIC_WITH_ARGS(const QString, commonIniFilename, ("common"_L1)) // NOLINT

#if !defined(Q_OS_WIN)
const QString VCommonSettings::unixStandardSharePath = QStringLiteral(PKGDATADIR); // NOLINT(cert-err58-cpp)
#endif

QT_WARNING_POP

//---------------------------------------------------------------------------------------------------------------------
VCommonSettings::VCommonSettings(Format format, Scope scope, const QString &organization, const QString &application,
                                 QObject *parent)
  : QSettings(format, scope, organization, application, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
VCommonSettings::VCommonSettings(const QString &fileName, QSettings::Format format, QObject *parent)
  : QSettings(fileName, format, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Upstream 81758f77d: a settings value can only be corrupt if it is enormous - drop it instead of letting Qt
// rewrite the whole ini on every change (which freezes the interface).
auto VCommonSettings::ValueSize(const QVariant &value) -> qsizetype
{
    if (value.userType() == QMetaType::QStringList)
    {
        qsizetype size = 0;
        for (const auto &item : value.toStringList())
        {
            size += item.size() + 2;
        }
        return size;
    }

    return value.toString().size();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::RemoveOversizedValues() -> QStringList
{
    QStringList oversized;
    for (const auto &key : allKeys())
    {
        if (ValueSize(value(key)) > oversizedValueThreshold)
        {
            oversized.append(key);
        }
    }

    for (const auto &key : oversized)
    {
        qInfo("Dropping corrupt settings value \"%s\" of %lld characters.", qUtf8Printable(key),
              static_cast<long long>(ValueSize(value(key))));
        remove(key);
    }

    return oversized;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::CastToLayoutExportFormat(qint8 f) -> qint8
{
    qint8 format = f;
    if (f == 15 || (f > 16 && f < 23))
    {
        format = 16;
    }
    else if (f == 24 || (f > 26 && f < 33))
    {
        format = 26;
    }

    return format;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathIndividualMeasurements() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsIndividualMeasurements, QDir::homePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathIndividualMeasurements(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsIndividualMeasurements, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathMultisizeMeasurements() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsMultisizeMeasurements, QDir::homePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathMultisizeMeasurements(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsMultisizeMeasurements, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathPattern() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsPattern, QDir::homePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathPattern(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsPattern, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathManualLayouts() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsManualLayouts, QDir::homePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathManualLayouts(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsManualLayouts, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathLayoutExport() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsLayoutExport, QDir::homePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathLayoutExport(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsLayoutExport, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathCustomImage() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsCustomImage, QDir::homePath()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathCustomImage(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsCustomImage, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefPathSVGFonts() -> QString
{
    auto const defPath = QStringList{QDir::homePath(), "Ostara"_L1, "Svg fonts"_L1}.join(QDir::separator());

#if defined(Q_OS_WIN)
    QString const docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docPath.isEmpty())
    {
        return defPath;
    }

    return QStringList{docPath, "Ostara"_L1, "Svg fonts"_L1}.join(QDir::separator());
#else
    return defPath;
#endif
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathSVGFonts() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsSVGFonts, GetDefPathSVGFonts()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathSVGFonts(const QString &value)
{
    const QString oldPath = GetPathSVGFonts();

    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsSVGFonts, value);
    settings.sync();

    if (oldPath != value)
    {
        emit SVGFontsPathChanged(oldPath, value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefPathFontCorrections() -> QString
{
    QString const defPath =
        QStringList{QDir::homePath(), "Ostara"_L1, "Font corrections"_L1}.join(QDir::separator());

#if defined(Q_OS_WIN)
    QString const docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docPath.isEmpty())
    {
        return defPath;
    }

    return QStringList{docPath, "Ostara"_L1, "Font corrections"_L1}.join(QDir::separator());
#else
    return defPath;
#endif
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathFontCorrections() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsFontCorrections, GetDefPathFontCorrections()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathFontCorrections(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsFontCorrections, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefPathKnownMeasurements() -> QString
{
    QString const defPath =
        QStringList{QDir::homePath(), "Ostara"_L1, "Known measurements"_L1}.join(QDir::separator());

#if defined(Q_OS_WIN)
    QString const docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docPath.isEmpty())
    {
        return defPath;
    }

    return QStringList{docPath, "Ostara"_L1, "Known measurements"_L1}.join(QDir::separator());
#else
    return defPath;
#endif
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathKnownMeasurements() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsKnownMeasurements, GetDefPathKnownMeasurements()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathKnownMeasurements(const QString &value)
{
    const QString oldPath = GetPathKnownMeasurements();

    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsKnownMeasurements, value);
    settings.sync();

    if (oldPath != value)
    {
        emit KnownMeasurementsPathChanged(oldPath, value);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathMeasurementLibrary() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsMeasurementLibrary,
                          QDir::homePath() + QStringLiteral("/Ostara/Measurements")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathMeasurementLibrary(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsMeasurementLibrary, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathCartoucheTemplates() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsCartoucheTemplates,
                          QDir::homePath() + QStringLiteral("/Ostara/CartoucheTemplates")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathCartoucheTemplates(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsCartoucheTemplates, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathPdfTemplates() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsPdfTemplates,
                          QDir::homePath() + QStringLiteral("/Ostara/PDFTemplate")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathPdfTemplates(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsPdfTemplates, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------

auto VCommonSettings::GetPathCoverPageTemplates() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsCoverPageTemplates,
                          QDir::homePath() + QStringLiteral("/Ostara/CoverPageTemplate")).toString();
}

//---------------------------------------------------------------------------------------------------------------------

void VCommonSettings::SetPathCoverPageTemplates(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsCoverPageTemplates, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathLogos() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsLogos, QDir::homePath() + QStringLiteral("/Ostara/Logos")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathLogos(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsLogos, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathSymbols() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsSymbols, QDir::homePath() + QStringLiteral("/Ostara/Symbols")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathSymbols(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsSymbols, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathSeamAllowances() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsSeamAllowances, QDir::homePath() + QStringLiteral("/Ostara/SeamAllowances"))
        .toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathSeamAllowances(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsSeamAllowances, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathClients() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsClients, QDir::homePath() + QStringLiteral("/Ostara/Clients")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathClients(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsClients, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathAnnotationStyles() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsAnnotationStyles, QDir::homePath() + QStringLiteral("/Ostara/AnnotationStyles"))
        .toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathAnnotationStyles(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsAnnotationStyles, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPathCollections() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPathsCollections, QDir::homePath() + QStringLiteral("/Ostara/Collections")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPathCollections(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPathsCollections, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetCollectionsSortAscending() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingCollectionsSortAscending, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCollectionsSortAscending(const bool &value)
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingCollectionsSortAscending, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetMeasurementFavorites() const -> QStringList
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingMeasurementFavorites).toStringList();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetMeasurementFavorites(const QStringList &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingMeasurementFavorites, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetSizeAxisForMeasurement(const QString &canonicalPath) const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    const QVariantMap axes = settings.value(*settingGradedSizeAxis).toMap();
    return axes.value(canonicalPath).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetSizeAxisForMeasurement(const QString &canonicalPath, const QString &axisName) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    QVariantMap axes = settings.value(*settingGradedSizeAxis).toMap();
    if (axisName.isEmpty())
    {
        axes.remove(canonicalPath); // undeclare the table
    }
    else
    {
        axes.insert(canonicalPath, axisName);
    }
    settings.setValue(*settingGradedSizeAxis, axes);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetGradedFavorites(const QString &canonicalPath, const QString &axisName) const -> QVariantMap
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingGradedFavorites).toMap()
        .value(canonicalPath).toMap()
        .value(axisName).toMap();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetGradedFavorite(const QString &canonicalPath, const QString &axisName, const QString &name,
                                        const QVariantMap &record) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);

    QVariantMap outer = settings.value(*settingGradedFavorites).toMap();
    QVariantMap perAxis = outer.value(canonicalPath).toMap();
    QVariantMap names = perAxis.value(axisName).toMap();
    names.insert(name, record);
    perAxis.insert(axisName, names);
    outer.insert(canonicalPath, perAxis);

    settings.setValue(*settingGradedFavorites, outer);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::RemoveGradedFavorite(const QString &canonicalPath, const QString &axisName,
                                           const QString &name) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);

    QVariantMap outer = settings.value(*settingGradedFavorites).toMap();
    QVariantMap perAxis = outer.value(canonicalPath).toMap();
    QVariantMap names = perAxis.value(axisName).toMap();
    names.remove(name);
    if (names.isEmpty())
    {
        perAxis.remove(axisName);
    }
    else
    {
        perAxis.insert(axisName, names);
    }
    if (perAxis.isEmpty())
    {
        outer.remove(canonicalPath);
    }
    else
    {
        outer.insert(canonicalPath, perAxis);
    }

    settings.setValue(*settingGradedFavorites, outer);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetExportFavorites(const QString &canonicalPath, const QString &axisName) const -> QVariantMap
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingExportFavorites).toMap()
        .value(canonicalPath).toMap()
        .value(axisName).toMap();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetExportFavorite(const QString &canonicalPath, const QString &axisName, const QString &name,
                                        const QVariantMap &record) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);

    QVariantMap outer = settings.value(*settingExportFavorites).toMap();
    QVariantMap perAxis = outer.value(canonicalPath).toMap();
    QVariantMap names = perAxis.value(axisName).toMap();
    names.insert(name, record);
    perAxis.insert(axisName, names);
    outer.insert(canonicalPath, perAxis);

    settings.setValue(*settingExportFavorites, outer);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::RemoveExportFavorite(const QString &canonicalPath, const QString &axisName,
                                           const QString &name) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);

    QVariantMap outer = settings.value(*settingExportFavorites).toMap();
    QVariantMap perAxis = outer.value(canonicalPath).toMap();
    QVariantMap names = perAxis.value(axisName).toMap();
    names.remove(name);
    if (names.isEmpty())
    {
        perAxis.remove(axisName);
    }
    else
    {
        perAxis.insert(axisName, names);
    }
    if (perAxis.isEmpty())
    {
        outer.remove(canonicalPath);
    }
    else
    {
        outer.insert(canonicalPath, perAxis);
    }

    // Et la clé elle-même quand plus rien ne pend dessous. Le store des favoris de gammes laisse
    // pour sa part une carte vide derrière lui (même code, autre clé) : une clé morte dans le
    // fichier de réglages n'est pas une faute, mais elle n'a rien à y faire non plus.
    if (outer.isEmpty())
    {
        settings.remove(*settingExportFavorites);
    }
    else
    {
        settings.setValue(*settingExportFavorites, outer);
    }
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetGradedSizeStyles(const QString &canonicalPath, const QString &axisName) const -> QVariantMap
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingGradedSizeStyles).toMap()
        .value(canonicalPath).toMap()
        .value(axisName).toMap();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetGradedSizeStyles(const QString &canonicalPath, const QString &axisName,
                                          const QVariantMap &styles) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);

    QVariantMap outer = settings.value(*settingGradedSizeStyles).toMap();
    QVariantMap perAxis = outer.value(canonicalPath).toMap();
    if (styles.isEmpty())
    {
        perAxis.remove(axisName); // no styles left for that axis → drop the level
    }
    else
    {
        perAxis.insert(axisName, styles);
    }
    if (perAxis.isEmpty())
    {
        outer.remove(canonicalPath); // no axes left for that table → drop the level
    }
    else
    {
        outer.insert(canonicalPath, perAxis);
    }

    settings.setValue(*settingGradedSizeStyles, outer);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetGradedSizeStylesShowInCanvas() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingGradedSizeStylesShowInCanvas, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetGradedSizeStylesShowInCanvas(const bool &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingGradedSizeStylesShowInCanvas, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetShowActiveLayerOnCanvas() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingShowActiveLayerOnCanvas, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetShowActiveLayerOnCanvas(const bool &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingShowActiveLayerOnCanvas, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetOsSeparator() const -> bool
{
    return value(*settingConfigurationOsSeparator, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetOsSeparator(const bool &value)
{
    setValue(*settingConfigurationOsSeparator, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetAutosaveState() const -> bool
{
    return value(*settingConfigurationAutosaveState, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAutosaveState(const bool &value)
{
    setValue(*settingConfigurationAutosaveState, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetAutosaveTime() const -> int
{
    bool ok = false;
    int val = value(*settingConfigurationAutosaveTime, 1).toInt(&ok);
    if (not ok)
    {
        qDebug() << "Could not convert value" << value(*settingConfigurationAutosaveTime, 1)
                 << "to int. Return default value for autosave time" << 1 << "minutes.";
        val = 1;
    }
    return val;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAutosaveTime(const int &value)
{
    setValue(*settingConfigurationAutosaveTime, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsLocaleSelected() const -> bool
{
    const auto fakeLocale = QStringLiteral("Fake");
    QString const locale = value(*settingConfigurationLocale, fakeLocale).toString();

    return locale != fakeLocale;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetLocale() const -> QString
{
    if (localeCached->isEmpty())
    {
        *localeCached = value(*settingConfigurationLocale, SystemPreferredLocale()).toString();
    }
    return *localeCached;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLocale(const QString &value)
{
    setValue(*settingConfigurationLocale, value);
    *localeCached = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPieceLabelLocale() const -> QString
{
    return value(*settingConfigurationPieceLabelLocale, VCommonSettings::defaultPieceLabelLocale).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPieceLabelLocale(const QString &value)
{
    setValue(*settingConfigurationPieceLabelLocale, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetKnownMeasurementsId() const -> QUuid
{
    return value(*settingKnownMeasurementsId, QString()).toUuid();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetKnownMeasurementsId(const QUuid &value)
{
    setValue(*settingKnownMeasurementsId, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetUnit() const -> QString
{
    return value(*settingConfigurationUnit, QLocale().measurementSystem() == QLocale::MetricSystem ? unitCM : unitINCH)
        .toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetUnit(const QString &value)
{
    setValue(*settingConfigurationUnit, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetConfirmItemDelete() const -> bool
{
    return value(*settingConfigurationConfirmItemDeletion, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetConfirmItemDelete(const bool &value)
{
    setValue(*settingConfigurationConfirmItemDeletion, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetConfirmFormatRewriting() const -> bool
{
    return value(*settingConfigurationConfirmFormatRewriting, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetConfirmFormatRewriting(const bool &value)
{
    setValue(*settingConfigurationConfirmFormatRewriting, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetAskContinueIfLayoutStale() const -> bool
{
    return value(*settingConfigurationAskContinueIfLayoutStale, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAskContinueIfLayoutStale(const bool &value)
{
    setValue(*settingConfigurationAskContinueIfLayoutStale, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetAskRegenerateOutputs() const -> bool
{
    return value(*settingConfigurationAskRegenerateOutputs, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAskRegenerateOutputs(const bool &value)
{
    setValue(*settingConfigurationAskRegenerateOutputs, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetToolBarStyle() const -> bool
{
    return value(*settingConfigurationToolBarStyle, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetToolBarStyle(const bool &value)
{
    setValue(*settingConfigurationToolBarStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetToolboxIconSizeSmall() const -> bool
{
    return value(*settingConfigurationToolboxIconSizeSmall, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetToolboxIconSizeSmall(bool value)
{
    setValue(*settingConfigurationToolboxIconSizeSmall, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetThemeMode() const -> VThemeMode
{
    if (themeModeCached < 0)
    {
        QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
        int val = settings.value(*settingConfigurationThemeMode, static_cast<int>(VThemeMode::Light)).toInt();
        themeModeCached = (val < 0 || val > 1) ? 0 : val;
    }

    return static_cast<VThemeMode>(themeModeCached);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetThemeMode(VThemeMode mode) const
{
    themeModeCached = static_cast<int>(mode);
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingConfigurationThemeMode, themeModeCached);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsFreeCurveMode() const -> bool
{
    return value(*settingConfigurationFreeCurveMode, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetFreeCurveMode(bool value)
{
    setValue(*settingConfigurationFreeCurveMode, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsDoubleClickZoomFitBestCurrentPP() const -> bool
{
    return value(*settingConfigurationDoubleClickZoomFitBestCurrentPP, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDoubleClickZoomFitBestCurrentPP(bool value)
{
    setValue(*settingConfigurationDoubleClickZoomFitBestCurrentPP, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsInteractiveTools() const -> bool
{
    return value(*settingConfigurationInteractiveTools, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetInteractiveTools(bool value)
{
    setValue(*settingConfigurationInteractiveTools, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetUndoCount() const -> int
{
    bool ok = false;
    int val = value(*settingPatternUndo, 0).toInt(&ok);
    if (not ok)
    {
        qDebug() << "Could not convert value" << value(*settingPatternUndo, 0)
                 << "to int. Return default value for undo counts 0 (no limit).";
        val = 0;
    }
    return val;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetUndoCount(const int &value)
{
    setValue(*settingPatternUndo, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetRecentFileList() const -> QStringList
{
    const QStringList files = value(*settingGeneralRecentFileList).toStringList();
    QStringList cleared;

    for (const auto &f : files)
    {
        if (QFileInfo::exists(f))
        {
            cleared.append(f);
        }
    }

    return cleared;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetRecentFileList(const QStringList &value)
{
    setValue(*settingGeneralRecentFileList, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetRecentMeasurementFileList() const -> QStringList
{
    const QStringList files = value(*settingGeneralRecentMeasurementFileList).toStringList();
    QStringList cleared;

    for (const auto &f : files)
    {
        if (QFileInfo::exists(f))
        {
            cleared.append(f);
        }
    }

    return cleared;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetRecentMeasurementFileList(const QStringList &value)
{
    setValue(*settingGeneralRecentMeasurementFileList, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetRestoreFileList() const -> QStringList
{
    return value(*settingGeneralRestoreFileList).toStringList();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetRestoreFileList(const QStringList &value)
{
    setValue(*settingGeneralRestoreFileList, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetGeometry() const -> QByteArray
{
    return value(*settingGeneralGeometry).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetGeometry(const QByteArray &value)
{
    setValue(*settingGeneralGeometry, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetKMGeometry() const -> QByteArray
{
    return value(*settingGeneralKMGeometry).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetKMGeometry(const QByteArray &value)
{
    setValue(*settingGeneralKMGeometry, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetToolbarsState() const -> QByteArray
{
    return value(*settingGeneralToolbarsState).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetToolbarsState(const QByteArray &value)
{
    setValue(*settingGeneralToolbarsState, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetKMToolbarsState() const -> QByteArray
{
    return value(*settingGeneralKMToolbarsState).toByteArray();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetKMToolbarsState(const QByteArray &value)
{
    setValue(*settingGeneralKMToolbarsState, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPreferenceDialogSize() const -> QSize
{
    return value(*settingPreferenceDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPreferenceDialogSize(const QSize &sz)
{
    setValue(*settingPreferenceDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetToolSeamAllowanceDialogSize() const -> QSize
{
    return value(*settingToolSeamAllowanceDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetToolSeamAllowanceDialogSize(const QSize &sz)
{
    setValue(*settingToolSeamAllowanceDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetFormulaWizardDialogSize() const -> QSize
{
    return value(*settingFormulaWizardDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetFormulaWizardDialogSize(const QSize &sz)
{
    setValue(*settingFormulaWizardDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetIncrementsDialogSize() const -> QSize
{
    return value(*settingIncrementsDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetIncrementsDialogSize(const QSize &sz)
{
    setValue(*settingIncrementsDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetFinalMeasurementsDialogSize() const -> QSize
{
    return value(*settingFinalMeasurementsDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetFinalMeasurementsDialogSize(const QSize &sz)
{
    setValue(*settingFinalMeasurementsDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetLayoutSettingsDialogSize() const -> QSize
{
    return value(*settingLayoutSettingsDialogSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLayoutSettingsDialogSize(const QSize &sz)
{
    setValue(*settingLayoutSettingsDialogSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDialogSplinePathSize() const -> QSize
{
    return value(*settingDialogSplinePathSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDialogSplinePathSize(const QSize &sz)
{
    setValue(*settingDialogSplinePathSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsAutomaticallyCheckUpdates() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingAutomaticallyCheckUpdates, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAutomaticallyCheckUpdates(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingAutomaticallyCheckUpdates, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetLatestSkippedVersion() const -> unsigned
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingLatestSkippedVersion, 0x0).toUInt();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLatestSkippedVersion(unsigned value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingLatestSkippedVersion, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDateOfLastRemind() const -> QDate
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingDateOfLastRemind, QDate(1900, 1, 1)).toDate();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDateOfLastRemind(const QDate &date) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingDateOfLastRemind, date);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetForceWorkpieceFlipping() const -> bool
{
    return value(*settingPatternForceFlipping, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetForceWorkpieceFlipping(bool value)
{
    setValue(*settingPatternForceFlipping, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetSewLineOnDrawing() const -> bool
{
    return value(*settingPatternSewLineOnDrawing, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetSewLineOnDrawing(bool value)
{
    setValue(*settingPatternSewLineOnDrawing, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsHideMainPath() const -> bool
{
    return value(*settingPatternHideMainPath, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetHideMainPath(bool value)
{
    setValue(*settingPatternHideMainPath, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsDoublePassmark() const -> bool
{
    // Defaults to true: this is the factory default of the "second passmark" funnel, and a stored
    // false would come from a legacy profile that never made the choice.
    return value(*settingDoublePassmark, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDoublePassmark(bool value)
{
    setValue(*settingDoublePassmark, value);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCSVWithHeader(bool withHeader) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingCSVWithHeader, withHeader);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetCSVWithHeader() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingCSVWithHeader, GetDefCSVWithHeader()).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefCSVWithHeader() -> bool
{
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCSVCodec(int mib) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingCSVCodec, mib);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetCSVCodec() const -> int
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingCSVCodec, GetDefCSVCodec()).toInt();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefCSVCodec() -> int
{
    return QTextCodec::codecForLocale()->mibEnum();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCSVSeparator(const QChar &separator) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    switch (separator.toLatin1())
    {
        case '\t':
            settings.setValue(*settingCSVSeparator, 0);
            break;
        case ';':
            settings.setValue(*settingCSVSeparator, 1);
            break;
        case ' ':
            settings.setValue(*settingCSVSeparator, 2);
            break;
        default:
            settings.setValue(*settingCSVSeparator, 3);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetCSVSeparator() const -> QChar
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    const auto separator = static_cast<quint8>(settings.value(*settingCSVSeparator, 3).toUInt());
    switch (separator)
    {
        case 0:
            return '\t'_L1;
        case 1:
            return ';'_L1;
        case 2:
            return ' '_L1;
        default:
            return ','_L1;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefCSVSeparator() -> QChar
{
    return ','_L1;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDefaultSeamAllowance(double value)
{
    setValue(*settingPatternDefaultSeamAllowance, UnitConvertor(value, StrToUnits(GetUnit()), Unit::Cm));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief returns the default seam allowance. The corresponding unit is the default unit.
 * @return the default seam allowance
 */
auto VCommonSettings::GetDefaultSeamAllowance() -> double
{
    double defaultValue = 1;

    const Unit globalUnit = StrToUnits(GetUnit());

    switch (globalUnit)
    {
        case Unit::Mm:
            defaultValue = 10;
            break;
        case Unit::Inch:
            defaultValue = 0.25;
            break;
        case Unit::Cm:
        default:
            defaultValue = 1;
            break;
    }

    bool ok = false;
    double val = value(*settingPatternDefaultSeamAllowance, -1).toDouble(&ok);
    if (not ok)
    {
        qDebug() << "Could not convert value" << value(*settingPatternDefaultSeamAllowance, 0)
                 << "to real. Return default value for default seam allowance is " << defaultValue << '.'_L1;
        val = defaultValue;
    }

    if (val < 0)
    {
        val = defaultValue;
    }
    else
    {
        val = UnitConvertor(val, Unit::Cm, globalUnit);
    }

    return val;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetLabelFont() const -> QFont
{
    return FontFromString(value(*settingPatternLabelFont, QApplication::font().toString()).toString());
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLabelFont(const QFont &f)
{
    setValue(*settingPatternLabelFont, f.toString());
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetLabelSVGFont() const -> QString
{
    return value(*settingPatternLabelSVGFont, QString()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLabelSVGFont(const QString &family)
{
    setValue(*settingPatternLabelSVGFont, family);
}


//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefaultCartoucheTemplate() const -> QString
{
    return value(*settingCartoucheDefaultTemplate, QStringLiteral("Default")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDefaultCartoucheTemplate(const QString &name)
{
    setValue(*settingCartoucheDefaultTemplate, name);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefaultPdfTemplate() const -> QString
{
    return value(*settingPdfDefaultTemplate, QStringLiteral("Default")).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDefaultPdfTemplate(const QString &name)
{
    setValue(*settingPdfDefaultTemplate, name);
}

//---------------------------------------------------------------------------------------------------------------------

auto VCommonSettings::GetDefaultCoverPageTemplate() const -> QString
{
    // Empty by default, where the assembly marks fall back to « Default »: nothing seeds a cover
    // page, so the folder starts empty and there is no name for the favourite to point at until
    // the user stars one.
    return value(*settingCoverPageDefaultTemplate, QString()).toString();
}

//---------------------------------------------------------------------------------------------------------------------

void VCommonSettings::SetDefaultCoverPageTemplate(const QString &name)
{
    setValue(*settingCoverPageDefaultTemplate, name);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetSingleStrokeOutlineFont() const -> bool
{
    return value(*settingPatternSingleStrokeOutlineFont, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetSingleStrokeOutlineFont(bool value)
{
    setValue(*settingPatternSingleStrokeOutlineFont, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetSingleLineFonts() const -> bool
{
    return value(*settingPatternSingleLineFonts, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetSingleLineFonts(bool value)
{
    setValue(*settingPatternSingleLineFonts, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::MinPieceLabelFontPointSize() -> int
{
    return 5;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetPatternLabelFontSize() const -> int
{
    if (labelFontSizeCached <= 0)
    {
        bool ok = false;
        labelFontSizeCached = value(*settingPatternLabelFontSize, GetDefPatternLabelFontSize()).toInt(&ok);
        if (not ok)
        {
            labelFontSizeCached = GetDefPatternLabelFontSize();
        }
        labelFontSizeCached = qBound(minLabelFontSize, labelFontSizeCached, maxLabelFontSize);
    }
    return labelFontSizeCached;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPatternLabelFontSize(int size)
{
    size = qBound(minLabelFontSize, size, maxLabelFontSize);
    setValue(*settingPatternLabelFontSize, size);
    labelFontSizeCached = size;
    emit PatternLabelFontSizeChanged(size);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetDefPatternLabelFontSize() -> int
{
    return 32;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetShowAllLabels() const -> bool
{
    return value(*settingPatternShowAllLabels, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetShowAllLabels(bool value)
{
    setValue(*settingPatternShowAllLabels, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetShowAccuracyRadius() const -> bool
{
    return value(*settingPatternShowAccuracyRadius, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetShowAccuracyRadius(bool value)
{
    setValue(*settingPatternShowAccuracyRadius, value);
}


//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetCurveApproximationScale() const -> qreal
{
    if (curveApproximationCached < 0)
    {
        bool ok = false;
        const qreal scale = value(*settingPatternCurveApproximationScale, defCurveApproximationScale).toDouble(&ok);
        if (ok && scale >= minCurveApproximationScale && scale <= maxCurveApproximationScale)
        {
            curveApproximationCached = scale;
        }
        else
        {
            curveApproximationCached = defCurveApproximationScale;
        }
    }

    return curveApproximationCached;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCurveApproximationScale(qreal value)
{
    if (value >= minCurveApproximationScale && value <= maxCurveApproximationScale)
    {
        setValue(*settingPatternCurveApproximationScale, value);
        curveApproximationCached = value;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsShowCurveDetails() const -> bool
{
    return value(*settingPatternShowCurveDetails, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetShowCurveDetails(bool value)
{
    setValue(*settingPatternShowCurveDetails, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsShowSegmentLabels() const -> bool
{
    return value(*settingPatternShowSegmentLabels, true).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetShowSegmentLabels(bool value)
{
    setValue(*settingPatternShowSegmentLabels, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsPieceShowMainPath() const -> bool
{
    if (pieceShowMainPath < 0)
    {
        pieceShowMainPath = value(*settingPatternPieceShowMainPath, 0).toInt();
    }
    return pieceShowMainPath;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetPieceShowMainPath(bool value)
{
    pieceShowMainPath = value;
    setValue(*settingPatternPieceShowMainPath, pieceShowMainPath);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsDontUseNativeDialog() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingConfigurationDontUseNativeDialog, false).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetDontUseNativeDialog(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingConfigurationDontUseNativeDialog, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetLineWidth() const -> qreal
{
    if (lineWidthCached <= 0)
    {
        lineWidthCached = qBound(VCommonSettings::MinimalLineWidth(), value(*settingPatternLineWidth, 1.2).toDouble(),
                                 VCommonSettings::MaximalLineWidth());
    }

    return lineWidthCached;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetLineWidth(qreal width)
{
    lineWidthCached = qBound(VCommonSettings::MinimalLineWidth(), width, VCommonSettings::MaximalLineWidth());
    setValue(*settingPatternLineWidth, lineWidthCached);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::WidthMainLine() const -> qreal
{
    return GetLineWidth() / 25.4 * PrintDPI;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::WidthHairLine() const -> qreal
{
    return WidthMainLine() / 3.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::WidthBoldLine() const -> qreal
{
    // Three times the hairline, the weight the bold has always had. One and a half, then twice, were tried and both
    // are invisible on screen: the hairline is 1.5 px at 100%, and neither 2.3 px nor 3 px reads as a bolder stroke
    // (Sophie, 09-10-2026). What hid the points was never this width but the order the points are painted in.
    return WidthMainLine();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetScrollingDuration() const -> int
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return GetCachedValue(settings, scrollingDurationCached, *settingScrollingDuration, defaultScrollingDuration,
                          scrollingDurationMin, scrollingDurationMax);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetScrollingDuration(int duration) const
{
    scrollingDurationCached = qBound(scrollingDurationMin, duration, scrollingDurationMax);
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingScrollingDuration, scrollingDurationCached);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetScrollingUpdateInterval() const -> int
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return GetCachedValue(settings, scrollingUpdateIntervalCached, *settingScrollingUpdateInterval,
                          defaultScrollingUpdateInterval, scrollingUpdateIntervalMin, scrollingUpdateIntervalMax);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetScrollingUpdateInterval(int updateInterval) const
{
    scrollingUpdateIntervalCached = qBound(scrollingUpdateIntervalMin, updateInterval, scrollingUpdateIntervalMax);
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingScrollingUpdateInterval, scrollingUpdateIntervalCached);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetSensorMouseScale() const -> qreal
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return GetCachedValue(settings, scrollingSensorMouseScaleCached, *settingScrollingSensorMouseScale,
                          defaultSensorMouseScale, sensorMouseScaleMin, sensorMouseScaleMax);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetSensorMouseScale(qreal scale) const
{
    scrollingSensorMouseScaleCached = qBound(sensorMouseScaleMin, scale, sensorMouseScaleMax);
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingScrollingSensorMouseScale, scrollingSensorMouseScaleCached);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetWheelMouseScale() const -> qreal
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return GetCachedValue(settings, scrollingWheelMouseScaleCached, *settingScrollingWheelMouseScale,
                          defaultWheelMouseScale, wheelMouseScaleMin, wheelMouseScaleMax);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetWheelMouseScale(qreal scale) const
{
    scrollingWheelMouseScaleCached = qBound(wheelMouseScaleMin, scale, wheelMouseScaleMax);
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingScrollingWheelMouseScale, scrollingWheelMouseScaleCached);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetScrollingAcceleration() const -> qreal
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return GetCachedValue(settings, scrollingAccelerationCached, *settingScrollingAcceleration,
                          defaultScrollingAcceleration, scrollingAccelerationMin, scrollingAccelerationMax);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetScrollingAcceleration(qreal acceleration) const
{
    scrollingAccelerationCached = qBound(scrollingAccelerationMin, acceleration, scrollingAccelerationMax);
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingScrollingAcceleration, scrollingAccelerationCached);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsOpenGLRender() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPatternUseOpenGLRender, 0).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetOpenGLRender(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPatternUseOpenGLRender, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetGraphicalOutput() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingPatternGraphicalOutput, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetGraphicalOutput(const bool &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingPatternGraphicalOutput, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetTiledPDFMargins returns the tiled pdf margins in the given unit. When the setting is
 * called for the first time, the 4 default margins are 10mm.
 * @param unit the unit in which are the value. Necessary because we save the values
 * internaly as mm so there is conversion beeing made.
 * @return tiled pdf margins
 */
auto VCommonSettings::GetTiledPDFMargins(const Unit &unit) const -> QMarginsF
{
    // default value is 10mm. We save the margins in mm in the setting.
    return UnitConvertor(ValueOrDef<QMarginsF>(*this, *settingTiledPDFMargins, QMarginsF(10, 10, 10, 10)), Unit::Mm,
                         unit);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetTiledPDFMargins sets the setting tiled pdf margins to the given value.
 * @param value the margins to save
 * @param unit the unit in which are the value. Necessary because we save the values
 * internaly as mm so there is conversion beeing made.
 */
void VCommonSettings::SetTiledPDFMargins(const QMarginsF &value, const Unit &unit)
{
    setValue(*settingTiledPDFMargins, QVariant::fromValue(UnitConvertor(value, unit, Unit::Mm)));
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetTiledPDFOrientation() const -> PageOrientation
{
    auto const defaultValue = static_cast<bool>(PageOrientation::Portrait);
    bool const result = value(*settingTiledPDFOrientation, defaultValue).toBool();
    return static_cast<PageOrientation>(result);
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetTiledPDFOrientation(PageOrientation value)
{
    setValue(*settingTiledPDFOrientation, static_cast<bool>(value));
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetWatermarkEditorSize() const -> QSize
{
    return value(*settingWatermarkEditorSize, QSize(0, 0)).toSize();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetWatermarkEditorSize(const QSize &sz)
{
    setValue(*settingWatermarkEditorSize, sz);
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetWatermarkCustomColors() const -> QVector<QColor>
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    QStringList const colors = settings.value(*settingPatternGraphicalOutput, 1).toStringList();

    QVector<QColor> customColors;
    customColors.reserve(colors.size());

    for (const auto &color : colors)
    {
        QColor const c(color);
        if (c.isValid())
        {
            customColors.append(c);
        }
    }

    if (customColors.count() > 7)
    {
        customColors.remove(0, customColors.count() - 7);
    }

    return customColors;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetWatermarkCustomColors(QVector<QColor> colors) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);

    if (colors.count() > 7)
    {
        colors.remove(0, colors.count() - 7);
    }

    QStringList customColors;
    customColors.reserve(colors.size());

    for (const auto &color : colors)
    {
        customColors.append(color.name());
    }

    settings.setValue(*settingWatermarkCustomColors, customColors);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsAskCollectStatistic() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingsStatistictAskCollect, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAskCollectStatistic(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsStatistictAskCollect, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsCollectStatistic() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingsStatistictCollect, 0).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCollectStatistic(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsStatistictCollect, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetClientID() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingsStatistictClientID, QString()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetClientID(const QString &clientID) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsStatistictClientID, clientID);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsTranslateFormula() const -> bool
{
    if (patternTranslateFormulaCached < 0)
    {
        QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
        patternTranslateFormulaCached = settings.value(*settingsPatternTranslateFormula, 1).toBool();
    }

    return patternTranslateFormulaCached;
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetTranslateFormula(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsPatternTranslateFormula, value);
    settings.sync();
    patternTranslateFormulaCached = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetActionShortcuts(const QString &name, const QStringList &defaultShortcuts) -> QStringList
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.beginGroup("shortcuts"_L1);
    return settings.value(name, defaultShortcuts).toStringList();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetActionShortcuts(const QString &name, const QStringList &shortcuts) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.beginGroup("shortcuts"_L1);
    settings.setValue(name, shortcuts);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::RemoveActionShortcuts(const QString &name) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.beginGroup("shortcuts"_L1);
    settings.remove(name);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::GetCrashEmail() const -> QString
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingsConfigurationCrashEmail, QString()).toString();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetCrashEmail(const QString &value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsConfigurationCrashEmail, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsSendCrashReport() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingsConfigurationSendCrashReport, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SeSendCrashReport(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsConfigurationSendCrashReport, value);
    settings.sync();
}

//---------------------------------------------------------------------------------------------------------------------
auto VCommonSettings::IsAskSendCrashReport() const -> bool
{
    QSettings const settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    return settings.value(*settingsConfigurationAskSendCrashReport, 1).toBool();
}

//---------------------------------------------------------------------------------------------------------------------
void VCommonSettings::SetAskSendCrashReport(bool value) const
{
    QSettings settings(this->format(), this->scope(), this->organizationName(), *commonIniFilename);
    settings.setValue(*settingsConfigurationAskSendCrashReport, value);
    settings.sync();
}
