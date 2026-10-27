/************************************************************************
 **
 **  @file   vcommonsettings.h
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

#ifndef VCOMMONSETTINGS_H
#define VCOMMONSETTINGS_H

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QSettings>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

#include "../vlayout/vlayoutdef.h"
#include "../vmisc/def.h"
#include "quuid.h"

class QMarginsF;

enum class VThemeMode
{
    Light = 0,
    Dark = 1
};

class VCommonSettings : public QSettings
{
    Q_OBJECT // NOLINT

public:
    VCommonSettings(Format format, Scope scope, const QString &organization, const QString &application = QString(),
                    QObject *parent = nullptr);
    VCommonSettings(const QString &fileName, Format format, QObject *parent = nullptr);
    ~VCommonSettings() override = default;

    static auto CastToLayoutExportFormat(qint8 f) -> qint8;

    /**
     * @brief oversizedValueThreshold the size past which a settings value can only be corrupt.
     *
     * The largest value written here in normal use is a saveState() blob, a couple of kilobytes. Nothing legitimate
     * comes close to this.
     */
    static constexpr qsizetype oversizedValueThreshold = 64 * 1024;

    static auto ValueSize(const QVariant &value) -> qsizetype;
    auto RemoveOversizedValues() -> QStringList;

    auto GetPathIndividualMeasurements() const -> QString;
    void SetPathIndividualMeasurements(const QString &value) const;

    auto GetPathMultisizeMeasurements() const -> QString;
    void SetPathMultisizeMeasurements(const QString &value) const;

    auto GetPathPattern() const -> QString;
    void SetPathPattern(const QString &value) const;

    auto GetPathLayoutExport() const -> QString;
    void SetPathLayoutExport(const QString &value) const;


    auto GetPathCustomImage() const -> QString;
    void SetPathCustomImage(const QString &value) const;


    static auto GetDefPathFontCorrections() -> QString;
    auto GetPathFontCorrections() const -> QString;
    void SetPathFontCorrections(const QString &value) const;

    static auto GetDefPathKnownMeasurements() -> QString;
    auto GetPathKnownMeasurements() const -> QString;
    void SetPathKnownMeasurements(const QString &value);

    auto GetPathMeasurementLibrary() const -> QString;
    void SetPathMeasurementLibrary(const QString &value) const;

    auto GetPathCartoucheTemplates() const -> QString;
    void SetPathCartoucheTemplates(const QString &value) const;

    auto GetPathPieces() const -> QString;
    void SetPathPieces(const QString &value) const;

    auto GetPathPatterns() const -> QString;
    void SetPathPatterns(const QString &value) const;

    auto GetPathPdfTemplates() const -> QString;
    void SetPathPdfTemplates(const QString &value) const;

    auto GetPathCoverPageTemplates() const -> QString;
    void SetPathCoverPageTemplates(const QString &value) const;

    auto GetPathLogos() const -> QString;
    void SetPathLogos(const QString &value) const;

    auto GetPathSymbols() const -> QString;
    void SetPathSymbols(const QString &value) const;

    auto GetPathSeamAllowances() const -> QString;
    void SetPathSeamAllowances(const QString &value) const;

    auto GetPathClients() const -> QString;
    void SetPathClients(const QString &value) const;

    auto GetPathAnnotationStyles() const -> QString;
    void SetPathAnnotationStyles(const QString &value) const;

    auto GetPathCollections() const -> QString;
    void SetPathCollections(const QString &value) const;

    auto GetCollectionsSortAscending() const -> bool;
    void SetCollectionsSortAscending(const bool &value);

    auto GetMeasurementFavorites() const -> QStringList;
    void SetMeasurementFavorites(const QStringList &value) const;

    /// Size axis (the graded axis) declared for a measurements table, stored per
    /// canonical path (software-level store, never in the .omt / .ostara).
    auto GetSizeAxisForMeasurement(const QString &canonicalPath) const -> QString;
    void SetSizeAxisForMeasurement(const QString &canonicalPath, const QString &axisName) const;

    /// Graded size-range favorites, stored per canonical path + size axis in the
    /// software store (never in the .omt / .ostara). Returns name -> record, where
    /// a record is a QVariantMap with "min", "max" and "axisValues" (other axes).
    auto GetGradedFavorites(const QString &canonicalPath, const QString &axisName) const -> QVariantMap;
    void SetGradedFavorite(const QString &canonicalPath, const QString &axisName, const QString &name,
                           const QVariantMap &record) const;
    void RemoveGradedFavorite(const QString &canonicalPath, const QString &axisName, const QString &name) const;

    /// Export-mode favorites, stored per canonical path + size axis in the software store
    /// (never in the .omt / .ostara) — the same (table, axis) key as the graded favorites,
    /// so that every pattern loading the same table offers the same ones. Returns
    /// name -> record, where a record holds the recipe of one run, field by field.
    auto GetExportFavorites(const QString &canonicalPath, const QString &axisName) const -> QVariantMap;
    void SetExportFavorite(const QString &canonicalPath, const QString &axisName, const QString &name,
                           const QVariantMap &record) const;
    void RemoveExportFavorite(const QString &canonicalPath, const QString &axisName, const QString &name) const;

    /// Graded trace styles (color + pt dashes + optional layer-name override) for
    /// the graded mode, stored per canonical path + size axis in the software store
    /// (never in the .omt / .ostara). Returns size-value label -> record, where a
    /// record holds "layerName" ("" = derived), "color" ("#RRGGBB" or "" = unstyled)
    /// and "dash" (pt [dash, space, ...] pairs; empty = solid — Ostara converts pt
    /// to px at canvas render / export).
    auto GetGradedSizeStyles(const QString &canonicalPath, const QString &axisName) const -> QVariantMap;
    void SetGradedSizeStyles(const QString &canonicalPath, const QString &axisName,
                             const QVariantMap &styles) const;

    /// Global switch applying the graded size styles on the canvas — the user may
    /// keep it off under dark themes (trace colors target paper, not a dark
    /// background). Software-level, independent of the table and the pattern.
    auto GetGradedSizeStylesShowInCanvas() const -> bool;
    void SetGradedSizeStylesShowInCanvas(const bool &value) const;

    /// Show the active layer's rule and name over the canvas — the reading aid the Layers dock can
    /// switch off when the drawing is all the user wants to see. Software-level, like the graded
    /// size styles' canvas gate, and on by default.
    auto GetShowActiveLayerOnCanvas() const -> bool;
    void SetShowActiveLayerOnCanvas(const bool &value) const;

    // The decimal separator is the OS one, and only that one: ',' and '.' are both accepted on input, so a
    // setting would only have had the power to lock the app to one of them. The accessor stays, because the number
    // formatting helpers read it — LocaleToString, the formula dialogs, the numeric keypad filter.
    auto GetOsSeparator() const -> bool;

    auto GetAutosaveState() const -> bool;
    void SetAutosaveState(const bool &value);

    auto GetAutosaveTime() const -> int;
    void SetAutosaveTime(const int &value);

    auto IsLocaleSelected() const -> bool;
    auto GetLocale() const -> QString;
    void SetLocale(const QString &value);

    auto GetKnownMeasurementsId() const -> QUuid;
    void SetKnownMeasurementsId(const QUuid &value);

    auto GetUnit() const -> QString;
    void SetUnit(const QString &value);

    auto GetConfirmItemDelete() const -> bool;
    void SetConfirmItemDelete(const bool &value);

    auto GetConfirmFormatRewriting() const -> bool;
    void SetConfirmFormatRewriting(const bool &value);


    /// @brief Warn, on entering the Export mode, that the listed generations were made
    ///        before the last pattern modification. Cleared for good by the message's own
    ///        "do not show again" box; put back by the preferences' warning reset.
    auto GetAskRegenerateOutputs() const -> bool;
    void SetAskRegenerateOutputs(const bool &value);

    auto GetThemeMode() const -> VThemeMode;
    void SetThemeMode(VThemeMode mode) const;

    auto IsFreeCurveMode() const -> bool;
    void SetFreeCurveMode(bool value);

    auto GetRecentFileList() const -> QStringList;
    void SetRecentFileList(const QStringList &value);
    auto GetRecentMeasurementFileList() const -> QStringList;
    void SetRecentMeasurementFileList(const QStringList &value);

    auto GetRestoreFileList() const -> QStringList;
    void SetRestoreFileList(const QStringList &value);

    auto GetGeometry() const -> QByteArray;
    void SetGeometry(const QByteArray &value);

    auto GetKMGeometry() const -> QByteArray;
    void SetKMGeometry(const QByteArray &value);

    auto GetToolbarsState() const -> QByteArray;
    void SetToolbarsState(const QByteArray &value);

    auto GetKMToolbarsState() const -> QByteArray;
    void SetKMToolbarsState(const QByteArray &value);

    auto GetPreferenceDialogSize() const -> QSize;
    void SetPreferenceDialogSize(const QSize &sz);

    auto GetToolSeamAllowanceDialogSize() const -> QSize;
    void SetToolSeamAllowanceDialogSize(const QSize &sz);

    auto GetFormulaWizardDialogSize() const -> QSize;
    void SetFormulaWizardDialogSize(const QSize &sz);

    auto GetIncrementsDialogSize() const -> QSize;
    void SetIncrementsDialogSize(const QSize &sz);

    auto GetFinalMeasurementsDialogSize() const -> QSize;
    void SetFinalMeasurementsDialogSize(const QSize &sz);


    auto GetDialogSplinePathSize() const -> QSize;
    void SetDialogSplinePathSize(const QSize &sz);

    auto GetForceWorkpieceFlipping() const -> bool;
    void SetForceWorkpieceFlipping(bool value);

    auto GetSewLineOnDrawing() const -> bool;
    void SetSewLineOnDrawing(bool value);

    auto IsHideMainPath() const -> bool;
    void SetHideMainPath(bool value);

    auto IsDoublePassmark() const -> bool;
    void SetDoublePassmark(bool value);

    void SetCSVWithHeader(bool withHeader) const;
    auto GetCSVWithHeader() const -> bool;
    static auto GetDefCSVWithHeader() -> bool;

    void SetCSVCodec(int mib) const;
    auto GetCSVCodec() const -> int;
    static auto GetDefCSVCodec() -> int;

    void SetCSVSeparator(const QChar &separator) const;
    auto GetCSVSeparator() const -> QChar;
    static auto GetDefCSVSeparator() -> QChar;

    void SetDefaultSeamAllowance(double value);
    auto GetDefaultSeamAllowance() -> double;

    auto GetDefaultCartoucheTemplate() const -> QString;
    void SetDefaultCartoucheTemplate(const QString &name);

    auto GetDefaultPdfTemplate() const -> QString;
    void SetDefaultPdfTemplate(const QString &name);

    auto GetDefaultCoverPageTemplate() const -> QString;
    void SetDefaultCoverPageTemplate(const QString &name);

    static auto MinPieceLabelFontPointSize() -> int;

    auto GetPatternLabelFontSize() const -> int;
    void SetPatternLabelFontSize(int size);
    static auto GetDefPatternLabelFontSize() -> int;

    auto GetShowAllLabels() const -> bool;
    void SetShowAllLabels(bool value);

    auto GetShowAccuracyRadius() const -> bool;
    void SetShowAccuracyRadius(bool value);

#if !defined(Q_OS_WIN)
    static const QString unixStandardSharePath;
#endif

    auto IsShowCurveDetails() const -> bool;
    void SetShowCurveDetails(bool value);

    auto IsShowSegmentLabels() const -> bool;
    void SetShowSegmentLabels(bool value);

    auto IsPieceShowMainPath() const -> bool;
    void SetPieceShowMainPath(bool value);

    auto IsDontUseNativeDialog() const -> bool;
    void SetDontUseNativeDialog(bool value) const;

    static auto DefaultLineWidth() -> qreal;
    static auto MinimalLineWidth() -> qreal;
    static auto MaximalLineWidth() -> qreal;
    auto GetLineWidth() const -> qreal;
    void SetLineWidth(qreal width);
    auto WidthMainLine() const -> qreal;
    auto WidthHairLine() const -> qreal;

    /** @brief The emphasized stroke — hovered, selected, or marked for a layer.
     *
     *  One place to weigh it: three times the hairline, the weight that is actually seen as bold on screen. */
    auto WidthBoldLine() const -> qreal;

    // settings for the tiled PDFs
    auto GetTiledPDFMargins(const Unit &unit) const -> QMarginsF;
    void SetTiledPDFMargins(const QMarginsF &value, const Unit &unit);

    auto GetTiledPDFOrientation() const -> PageOrientation;
    void SetTiledPDFOrientation(PageOrientation value);

    auto GetWatermarkEditorSize() const -> QSize;
    void SetWatermarkEditorSize(const QSize &sz);

    auto GetWatermarkCustomColors() const -> QVector<QColor>;
    void SetWatermarkCustomColors(QVector<QColor> colors) const;

    auto GetClientID() const -> QString;
    void SetClientID(const QString &clientID) const;

    auto IsTranslateFormula() const -> bool;
    void SetTranslateFormula(bool value) const;

    auto GetActionShortcuts(const QString &name, const QStringList &defaultShortcuts) -> QStringList;
    void SetActionShortcuts(const QString &name, const QStringList &shortcuts) const;
    void RemoveActionShortcuts(const QString &name) const;

signals:
    void KnownMeasurementsPathChanged(const QString &oldPath, const QString &newPath);
    void PatternLabelFontSizeChanged(int size);

protected:
    template <typename T>
    static auto GetCachedValue(const QSettings &settings, T &cache, const QString &setting, T defValue, T valueMin,
                               T valueMax) -> T;

    template <class T>
    static auto ValueOrDef(const QSettings &settings, const QString &setting, const T &defValue) -> T;

private:
    Q_DISABLE_COPY_MOVE(VCommonSettings) // NOLINT
};

//---------------------------------------------------------------------------------------------------------------------
inline auto VCommonSettings::DefaultLineWidth() -> qreal
{
    return 1.2; // mm
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VCommonSettings::MinimalLineWidth() -> qreal
{
    return 0.5; // mm
}

//---------------------------------------------------------------------------------------------------------------------
inline auto VCommonSettings::MaximalLineWidth() -> qreal
{
    return 5.0; // mm
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T>
auto VCommonSettings::GetCachedValue(const QSettings &settings, T &cache, const QString &setting, T defValue,
                                     T valueMin, T valueMax) -> T
{
    if (cache < 0)
    {
        cache = qBound(valueMin, ValueOrDef(settings, setting, defValue), valueMax);
    }

    return cache;
}

//---------------------------------------------------------------------------------------------------------------------
template <class T>
inline auto VCommonSettings::ValueOrDef(const QSettings &settings, const QString &setting, const T &defValue) -> T
{
    const QVariant val = settings.value(setting, QVariant::fromValue(defValue));
    return val.canConvert<T>() ? val.value<T>() : defValue;
}

//---------------------------------------------------------------------------------------------------------------------
template <>
inline auto VCommonSettings::ValueOrDef<Cases>(const QSettings &settings, const QString &setting, const Cases &defValue)
    -> Cases
{
    const QVariant val = settings.value(setting, QVariant::fromValue(static_cast<int>(defValue)));
    const int g = val.canConvert<int>() ? val.toInt() : static_cast<int>(defValue);
    if (g < static_cast<int>(Cases::CaseThreeGroup) || g >= static_cast<int>(Cases::UnknownCase))
    {
        return defValue;
    }
    return static_cast<Cases>(g);
}

#endif // VCOMMONSETTINGS_H
