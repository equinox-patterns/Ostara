/************************************************************************
 **
 **  @file   vvalentinasettings.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 11, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#ifndef VVALENTINASETTINGS_H
#define VVALENTINASETTINGS_H

#include <QFont>
#include <QMetaObject>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QtGlobal>

#include "vcommonsettings.h"

enum class VToolPointerMode
{
    ToolIcon = 0,
    Arrow = 1
};

class VValentinaSettings : public VCommonSettings
{
    Q_OBJECT // NOLINT

public:
    VValentinaSettings(Format format, Scope scope, const QString &organization, const QString &application = QString(),
                       QObject *parent = nullptr);
    VValentinaSettings(const QString &fileName, Format format, QObject *parent = nullptr);

    auto GetLabelLanguage() const -> QString;
    void SetLabelLanguage(const QString &value);

    // Layout settings


    static auto GetDefLayoutWidth() -> qreal;

    static auto GetDefNestingTime() -> int { return 1; }

    static auto GetDefEfficiencyCoefficient() -> qreal { return 0.0; }


    static auto GetDefLayoutGroup() -> Cases;

    static auto GetDefLayoutManualPriority() -> bool;

    static auto GetDefLayoutNestQuantity() -> bool;

    static auto GetDefLayoutAutoCropLength() -> bool;

    static auto GetDefLayoutAutoCropWidth() -> bool;

    static auto GetDefLayoutSaveLength() -> bool;

    static auto GetDefLayoutPreferOneSheetSolution() -> bool;

    static auto GetDefLayoutBoundaryTogetherWithNotches() -> bool;

    static auto GetDefLayoutUnitePages() -> bool;

    static auto GetDefIgnoreAllFields() -> bool;

    static auto GetDefStripOptimization() -> bool;

    static auto GetDefMultiplier() -> quint8;

    static auto GetDefTextAsPaths() -> bool;

    auto GetKnownMaterials() const -> QStringList;
    void SetKnownMaterials(const QStringList &list);

    auto IsRememberPatternMaterials() const -> bool;
    void SetRememberPatternMaterials(bool value);

    // settings for the tiled PDFs
    auto GetTiledPDFPaperHeight(const Unit &unit) const -> qreal;
    void SetTiledPDFPaperHeight(qreal value, const Unit &unit);

    auto GetTiledPDFPaperWidth(const Unit &unit) const -> qreal;
    void SetTiledPDFPaperWidth(qreal value, const Unit &unit);

    auto IsDockWidgetGroupsActive() const -> bool;
    static auto GetDefDockWidgetGroupsActive() -> bool;
    void SetDockWidgetGroupsActive(bool value);

    auto IsDockWidgetToolOptionsActive() const -> bool;
    static auto GetDefDockWidgetToolOptionsActive() -> bool;
    void SetDockWidgetToolOptionsActive(bool value);

    auto IsDockWidgetPatternMessagesActive() const -> bool;
    static auto GetDefDockWidgetPatternMessagesActive() -> bool;
    void SetDockWidgetPatternMessagesActive(bool value);

    auto IsDockWidgetBackgroundImagesActive() const -> bool;
    static auto GetDefDockWidgetBackgroundImagesActive() -> bool;
    void SetDockWidgetBackgroundImagesActive(bool value);

    auto IsDockWidgetDependenciesActive() const -> bool;
    static auto GetDefDockWidgetDependenciesActive() -> bool;
    void SetDockWidgetDependenciesActive(bool value);

    auto GetPatternMessageFontSize(int fontSizeDef) const -> int;
    static auto GetDefMinPatternMessageFontSize() -> int;
    static auto GetDefMaxPatternMessageFontSize() -> int;
    void SetPatternMessageFontSize(int size);

    auto GetAutoRefreshPatternMessage() const -> bool;
    void SetAutoRefreshPatternMessage(bool value);


    auto GetDetailExportFormat() const -> qint8;
    void SetDetailExportFormat(qint8 format);

    auto GetDxfCompatibility() const -> qint8;
    void SetDxfCompatibility(qint8 type);

    /// @brief IsShowMainPath tells whether the pieces are exported with their main path visible.
    auto IsShowMainPath() const -> bool;
    void SetShowMainPath(bool value);



    auto GetHistorySearchHistory() const -> QStringList;
    void SetHistorySearchHistory(const QStringList &history);

    auto GetIncrementsSearchHistory() const -> QStringList;
    void SetIncrementsSearchHistory(const QStringList &history);

    auto GetPreviewCalculationsSearchHistory() const -> QStringList;
    void SetPreviewCalculationsSearchHistory(const QStringList &history);

    auto GetFinalMeasurementsSearchHistory() const -> QStringList;
    void SetFinalMeasurementsSearchHistory(const QStringList &history);

    auto GetHistorySearchOptionUseUnicodeProperties() const -> bool;
    void SetHistorySearchOptionUseUnicodeProperties(bool value);

    auto GetHistorySearchOptionWholeWord() const -> bool;
    void SetHistorySearchOptionWholeWord(bool value);

    auto GetHistorySearchOptionRegexp() const -> bool;
    void SetHistorySearchOptionRegexp(bool value);

    auto GetHistorySearchOptionMatchCase() const -> bool;
    void SetHistorySearchOptionMatchCase(bool value);

    auto GetIncrementsSearchOptionUseUnicodeProperties() const -> bool;
    void SetIncrementsSearchOptionUseUnicodeProperties(bool value);

    auto GetIncrementsSearchOptionWholeWord() const -> bool;
    void SetIncrementsSearchOptionWholeWord(bool value);

    auto GetIncrementsSearchOptionRegexp() const -> bool;
    void SetIncrementsSearchOptionRegexp(bool value);

    auto GetIncrementsSearchOptionMatchCase() const -> bool;
    void SetIncrementsSearchOptionMatchCase(bool value);

    auto GetPreviewCalculationsSearchOptionUseUnicodeProperties() const -> bool;
    void SetPreviewCalculationsSearchOptionUseUnicodeProperties(bool value);

    auto GetPreviewCalculationsSearchOptionWholeWord() const -> bool;
    void SetPreviewCalculationsSearchOptionWholeWord(bool value);

    auto GetPreviewCalculationsSearchOptionRegexp() const -> bool;
    void SetPreviewCalculationsSearchOptionRegexp(bool value);

    auto GetPreviewCalculationsSearchOptionMatchCase() const -> bool;
    void SetPreviewCalculationsSearchOptionMatchCase(bool value);

    auto GetFinalMeasurementsSearchOptionUseUnicodeProperties() const -> bool;
    void SetFinalMeasurementsSearchOptionUseUnicodeProperties(bool value);

    auto GetFinalMeasurementsSearchOptionWholeWord() const -> bool;
    void SetFinalMeasurementsSearchOptionWholeWord(bool value);

    auto GetFinalMeasurementsSearchOptionRegexp() const -> bool;
    void SetFinalMeasurementsSearchOptionRegexp(bool value);

    auto GetFinalMeasurementsSearchOptionMatchCase() const -> bool;
    void SetFinalMeasurementsSearchOptionMatchCase(bool value);

    auto GetBackgroundImageDefOpacity() const -> int;
    void SetBackgroundImageDefOpacity(int value);


    auto GetPointerMode() const -> VToolPointerMode;
    void SetPointerMode(VToolPointerMode mode);

    auto IsUseToolGroups() const -> bool;
    void SetUseToolGroups(bool value);

    auto IsBoundaryTogetherWithNotches() const -> bool;
    void SetBoundaryTogetherWithNotches(bool value);

    auto GetUserToolColors() const -> QVector<QColor>;
    void SetUserToolColors(const QVector<QColor> &colors);

    auto GetGlobalPenStyle() const -> QString;
    void SetGlobalPenStyle(const QString &value);

    auto GetGlobalToolColor() const -> QString;
    void SetGlobalToolColor(const QString &value);

    auto IsCollectGarbage() const -> bool;
    void SetCollectGarbage(bool value);

    auto IsAutoGoToObject() const -> bool;
    void SetAutoGoToObject(bool value);

    auto GetModeToolbarsState(Draw mode) const -> QByteArray;
    void SetModeToolbarsState(Draw mode, const QByteArray &value);

    auto GetSavedSeamAllowances() const -> QVector<SavedSeamAllowance>;
    void SetSavedSeamAllowances(const QVector<SavedSeamAllowance> &list);

    // Fold line label appearance — global settings (family / weight / pixel size)
    auto GetFoldLineLabelFontFamily() const -> QString;
    static auto GetDefFoldLineLabelFontFamily() -> QString;
    void SetFoldLineLabelFontFamily(const QString &value);

    auto GetFoldLineLabelFontWeight() const -> QFont::Weight;
    static auto GetDefFoldLineLabelFontWeight() -> QFont::Weight;
    void SetFoldLineLabelFontWeight(QFont::Weight weight);

    /// The face the fold-line label is written in — a NAME, never a number (decided on
    /// 22-09-2026 : « que les vraies graisses de la font »). A settings file written before the face
    /// was named holds the number instead (@ref GetFoldLineLabelFontWeight), and the callers resolve
    /// it; the same fallback the section editor and the Thumbnail page make.
    auto GetFoldLineLabelFontFace() const -> QString;
    void SetFoldLineLabelFontFace(const QString &value);

    auto GetFoldLineLabelFontPixelSize() const -> int;
    static auto GetDefFoldLineLabelFontPixelSize() -> int;
    void SetFoldLineLabelFontPixelSize(int size);

    // Miniature typography and units — the defaults every miniature is born with.
    //
    // Two texts, set separately: the seam-allowance values a piece is cut with, and the legend that
    // names their unit. A reader wants the numbers legible and the caption discreet, so the two
    // carry a family, a face and a size of their own and share nothing.
    auto GetThumbnailValuesFontFamily() const -> QString;
    static auto GetDefThumbnailValuesFontFamily() -> QString;
    void SetThumbnailValuesFontFamily(const QString &value);

    /// Empty is not a face but an answer: the family's own Regular.
    auto GetThumbnailValuesFontFace() const -> QString;
    void SetThumbnailValuesFontFace(const QString &value);

    auto GetThumbnailValuesFontPixelSize() const -> int;
    static auto GetDefThumbnailValuesFontPixelSize() -> int;
    void SetThumbnailValuesFontPixelSize(int size);

    auto GetThumbnailLegendFontFamily() const -> QString;
    static auto GetDefThumbnailLegendFontFamily() -> QString;
    void SetThumbnailLegendFontFamily(const QString &value);

    auto GetThumbnailLegendFontFace() const -> QString;
    void SetThumbnailLegendFontFace(const QString &value);

    auto GetThumbnailLegendFontPixelSize() const -> int;
    static auto GetDefThumbnailLegendFontPixelSize() -> int;
    void SetThumbnailLegendFontPixelSize(int size);

    // The units a miniature is born with. At least one is always on: a miniature that states no unit
    // at all says nothing about the margin it is cut with.
    auto IsThumbnailShowCentimeters() const -> bool;
    void SetThumbnailShowCentimeters(bool value);

    auto IsThumbnailShowInches() const -> bool;
    void SetThumbnailShowInches(bool value);

signals:
    void UserToolColorsChanged();

private:
    Q_DISABLE_COPY_MOVE(VValentinaSettings) // NOLINT

    template <typename T>
    auto GetCachedValue(T &cache, const QString &setting, T defValue, T valueMin, T valueMax) const -> T;
};

#endif // VVALENTINASETTINGS_H
