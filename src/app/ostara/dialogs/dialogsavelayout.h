/************************************************************************
 **
 **  @file   dialogsavelayout.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   22 1, 2015
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

#ifndef DIALOGSAVELAYOUT_H
#define DIALOGSAVELAYOUT_H

#include "../vdxf/dxfdef.h"
#include "../vlayout/dialogs/vabstractlayoutdialog.h"
#include "../vlayout/vlayoutdef.h"
#include "../vmisc/def.h"

namespace Ui
{
class DialogSaveLAyout;
}

class DialogSaveLayout : public VAbstractLayoutDialog
{
    Q_OBJECT // NOLINT

public:
    explicit DialogSaveLayout(int count, const QString &fileName = QString(), QWidget *parent = nullptr);
    ~DialogSaveLayout() override;

    auto Path() const -> QString;
    auto FileName() const -> QString;

    auto Format() const -> LayoutExportFormats;
    void SelectFormat(LayoutExportFormats format);

    void SetBinaryDXFFormat(bool binary);
    auto IsBinaryDXFFormat() const -> bool;

    auto DxfCompatibility() const -> DXFApparelCompatibility;
    void SetDxfCompatibility(DXFApparelCompatibility type);

    /// @brief IsShowMainPath tells whether the pieces must be exported with their main path visible,
    /// whatever the pattern says. DXF only, and it changes nothing in the pattern.
    auto IsShowMainPath() const -> bool;
    void SetShowMainPath(bool value);

    void SetShowGrainline(bool show);
    auto IsShowGrainline() const -> bool;

    static auto MakeHelpFormatList() -> QString;
    static auto MakeHelpDxfApparelCompatibilityList() -> QString;

    void SetDestinationPath(const QString &cmdDestinationPath);


    auto IsTextAsPaths() const -> bool;
    void SetTextAsPaths(bool textAsPaths);

    void SetTiledExportMode(bool tiledExportMode);

    void SetTiledMargins(QMarginsF margins);
    auto GetTiledMargins() const -> QMarginsF;

    void SetTiledPageFormat(PaperSizeTemplate format);
    auto GetTiledPageFormat() const -> PaperSizeTemplate;

    void SetTiledPageOrientation(PageOrientation orientation);
    auto GetTiledPageOrientation() const -> PageOrientation;

    void SetXScale(qreal scale);
    auto GetXScale() const -> qreal;

    void SetYScale(qreal scale);
    auto GetYScale() const -> qreal;

    void SetBoundaryTogetherWithNotches(bool value);
    auto IsBoundaryTogetherWithNotches() const -> bool;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void Save();
    void PathChanged(const QString &text);
    void ShowExample();
    void ToggleScaleConnection();
    void UpdateScaleConnectionIcon();
    void HorizontalScaleChanged(double d);
    void VerticalScaleChanged(double d);

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(DialogSaveLayout) // NOLINT
    Ui::DialogSaveLAyout *ui;
    int m_count;
    bool m_tiledExportMode{false};
    bool m_scaleConnected{true};

    // Tiled PDF page setup. The dialog carries no margin and no paper widget any more: what a tiled PDF
    // uses is read from the settings, unless the command line sets it here (the console export uses this
    // dialog as a plain data holder, it never shows it).
    QMarginsF m_tiledMargins{};
    bool m_tiledMarginsSet{false};
    PaperSizeTemplate m_tiledPageFormat{PaperSizeTemplate::A0};
    bool m_tiledPageFormatSet{false};
    PageOrientation m_tiledOrientation{PageOrientation::Portrait};
    bool m_tiledOrientationSet{false};

    void ReadSettings();
    void WriteSettings() const;

    /// @brief FitHeightToContent locks the height on the rows the dialog currently shows.
    void FitHeightToContent();

    void InitDxfCompatibility();
    void InitFileFormats();
    void InitFileFormatTypes(LayoutExportFileFormat format);
    void SelectFirstAvailableFormat();
};

#endif // DIALOGSAVELAYOUT_H
