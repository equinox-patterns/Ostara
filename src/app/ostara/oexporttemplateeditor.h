/************************************************************************
 **
 **  @file   oexporttemplateeditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export template editor — one tab per template family.
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

#ifndef OEXPORTTEMPLATEEDITOR_H
#define OEXPORTTEMPLATEEDITOR_H

#include "core/oexportframesettings.h"
#include "core/oexportmarksettings.h"
#include "core/oexportpaginationsettings.h"
#include "core/oexporttemplate.h"
#include "../vmisc/def.h"

#include <QList>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFontComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QEvent;
class QListWidget;
class QScrollArea;
class QSlider;
class QSpinBox;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class OExportTemplatePreview;
class OExportTemplatesPage;

/**
 * @brief The Export template editor, in the shape of OInfoBlockEditor.
 *
 * Two tabs, one per template family:
 *
 *  - tab 0 « Cover page »      the nomenclature. Not implemented — see
 *                              Modifications/Ostara/ExportMode/PDFTemplateEditor.md §6.
 *  - tab 1 « Assembly marks »  what is added to the sheets of a tiled plan: the frame,
 *                              the marks and the pagination (§3.4).
 *
 * Tab 1 is a fixed-width menu on the left and a stack of pages on the right, as in
 * OInfoBlockEditor: pages are built once in the constructor and never rebuilt, and
 * navigating only calls setCurrentIndex().
 *
 * « Create a template » is a split between the Edition pane and a right-hand column holding
 * the preview under a format row. The format row picks the sheet being previewed — A4, A3,
 * Letter, or the free size of « Tiled Framed », whose two fields appear with it — and it is
 * a way of *looking* at the template, not a property of it: one template decorates any tiled
 * format, and nothing chosen there is saved. What it buys is truth: the sheet and everything
 * drawn on it share one scale, so a mark of a fixed size covers less of a big sheet than of a
 * small one, exactly as it will on paper.
 *
 * The shell and the preview are what exist here. The pages of the menu and the form of the
 * Edition pane are not written yet, and say so on screen.
 */
class OExportTemplateEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OExportTemplateEditor(QWidget *parent = nullptr);

    /** @brief Open on the given tab (0 = cover page, 1 = assembly marks). */
    void OpenOnTab(int tab);

    /** @brief Re-apply every user-visible string, after a language change. */
    void RetranslateUI();

    /**
     * @brief Whether the template has been changed since it was last saved or opened.
     *
     * The window asks before closing, which is the only thing that needs to know: the editor
     * saves when it is told to and never on its own, so nothing else has a use for this.
     */
    auto IsModified() const -> bool;

    /** @brief Saves the template, under the name in the field. Reports its own failures. */
    void SaveTemplate();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /**
     * @brief The four controls one placement's text style is edited with.
     *
     * A struct rather than eight members per placement, because there is one of these per
     * possibility: the centre and the corners already, and the count is not fixed. Each carries
     * its own four settings rather than sharing one set — a page number written large in a corner
     * need not be written large in the middle (§3.4.3).
     *
     * Built by MakeTextStyleRow(), filled by RefreshTextStyle(), read back by CommitTextStyle()
     * and worded by RetranslateTextStyle(), so that nothing about a style is written twice.
     */
    struct TextStyleRow
    {
        QWidget *row{nullptr}; ///< the widget the whole style is shown and hidden with
        QLabel *fontLabel{nullptr};
        QFontComboBox *fontCombo{nullptr};
        QLabel *weightLabel{nullptr};
        QComboBox *weightCombo{nullptr};
        QLabel *sizeLabel{nullptr};
        QSpinBox *sizeSpin{nullptr};
        QLabel *opacityLabel{nullptr};
        QSpinBox *opacitySpin{nullptr};
    };

    QWidget *MakeAssemblyMarksTab();
    QWidget *MakeCreateTemplatePage();
    QWidget *MakeEditionPane();
    QWidget *MakeFrameGroup();
    QWidget *MakeMarksGroup();
    QWidget *MakePaginationGroup();
    QWidget *MakePreviewFormatRow();
    QWidget *MakeZoomRow();
    QWidget *MakeTopBar();
    QLabel *MakePlaceholderLabel();
    QWidget *MakePlaceholderPage();
    void ShowMenuPage(int row);
    void ShowTemplatesList();
    void NewTemplate();

    void PopulateFrameStyles();
    void RefreshFrameFields();
    void PopulateMarkShapes();
    void RefreshMarkFields();
    TextStyleRow MakeTextStyleRow();
    void RetranslateTextStyle(const TextStyleRow &widgets);
    void RefreshTextStyle(const TextStyleRow &widgets, const OExportPaginationTextStyle &style);
    void CommitTextStyle(const TextStyleRow &widgets, OExportPaginationTextStyle &style);

    void PopulatePaginationCombos();
    void RefreshPaginationFields();
    void PopulatePreviewFormats();
    void RefreshZoomLabel();
    void RetranslateUnits();
    void RefreshPreviewSheet();
    void RefreshFramedFields();
    void CommitFramedSize();
    void CommitFrameSettings();
    void CommitMarkSettings();
    void CommitPaginationSettings();

    void LoadTemplate(const QString &filePath);
    void SetModified(bool modified);

    /** @brief Asks before throwing away changes that were never saved. True = carry on. */
    auto ConfirmDiscardChanges() -> bool;

    /**
     * @brief Puts the menu's highlight where the editor actually is.
     *
     * Derived from the page and the file, never stored: a row kept beside them would be a second
     * representation of the same thing, free to drift, with nothing on screen to say which won.
     *
     * The row and the page are not the same thing. A click moves the highlight **before** anything
     * is decided, so a cancelled prompt would leave the menu claiming a page the editor never went
     * to. And nothing is highlighted while a saved template is being edited — there is no row for
     * « editing an existing one », and leaving « Create a template » lit would say the opposite of
     * what is happening and make clicking it feel like it did nothing.
     */
    void ApplyMenuRow();

    /** @brief The three blocks as one template — what gets written. */
    auto Template() const -> OExportTemplate;

    /** @brief The three blocks taken from one template — what a file and a new template give. */
    void SetTemplate(const OExportTemplate &tpl);

    QTabWidget *m_tabs{nullptr};
    QListWidget *m_menuList{nullptr};
    QStackedWidget *m_stack{nullptr};
    OExportTemplatePreview *m_preview{nullptr};
    QScrollArea *m_previewScroll{nullptr};
    QSplitter *m_splitter{nullptr};
    OExportTemplatesPage *m_templatesPage{nullptr};

    /**
     * The name and the file, which are the same thing written twice: the file a template is
     * saved in is named after it, in the templates folder, and there is no second way to say
     * which template this is. Renaming the field and saving renames the file.
     */
    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QPushButton *m_saveButton{nullptr};
    QString m_filePath;
    bool m_modified{false};

    QLabel *m_zoomLabel{nullptr};
    QSlider *m_zoomSlider{nullptr};

    QLabel *m_formatLabel{nullptr};
    QComboBox *m_formatCombo{nullptr};
    QLabel *m_sheetWidthLabel{nullptr};
    QLineEdit *m_sheetWidthEdit{nullptr};
    QLabel *m_sheetHeightLabel{nullptr};
    QLineEdit *m_sheetHeightEdit{nullptr};
    QComboBox *m_sheetUnitCombo{nullptr};

    /**
     * The free size of « Tiled Framed », in millimetres — always millimetres, whatever unit
     * the two fields display. Held here rather than read back off the fields so that leaving
     * the format and coming back to it does not lose what was typed.
     */
    qreal m_framedWidthMm{190.0};
    qreal m_framedHeightMm{259.0};
    Unit m_sheetUnit{Unit::Cm};

    QGroupBox *m_frameGroup{nullptr};
    QLabel *m_frameNote{nullptr};
    QLabel *m_frameWidthLabel{nullptr};
    QDoubleSpinBox *m_frameWidthSpin{nullptr};
    QLabel *m_frameStyleLabel{nullptr};
    QComboBox *m_frameStyleCombo{nullptr};

    /** One dash and one space, on a single line — a pattern longer than that is not asked for. */
    QLabel *m_dashLabel{nullptr};
    QLineEdit *m_dashEdit{nullptr};
    QLabel *m_spaceLabel{nullptr};
    QLineEdit *m_spaceEdit{nullptr};
    QLabel *m_frameOpacityLabel{nullptr};
    QSpinBox *m_frameOpacitySpin{nullptr};
    QCheckBox *m_scissorsHorizontalCheck{nullptr};
    QCheckBox *m_scissorsVerticalCheck{nullptr};
    QLabel *m_scissorsOpacityLabel{nullptr};
    QSpinBox *m_scissorsOpacitySpin{nullptr};

    /**
     * The decor, held here until a template exists to hold it. Presented to the preview on
     * every change; nothing else reads it yet — persistence is an open point (§7 of
     * Modifications/Ostara/ExportMode/PDFTemplateEditor.md).
     */
    OExportFrameSettings m_frame{};

    QGroupBox *m_marksGroup{nullptr};
    QLabel *m_shapeLabel{nullptr};
    QComboBox *m_shapeCombo{nullptr};
    QLabel *m_sizeLabel{nullptr};
    QLineEdit *m_sizeEdit{nullptr};
    QComboBox *m_sizeUnitCombo{nullptr};
    QLabel *m_markOpacityLabel{nullptr};
    QSpinBox *m_markOpacitySpin{nullptr};
    QCheckBox *m_cornerCheck{nullptr};
    QCheckBox *m_verticalCheck{nullptr};
    QCheckBox *m_horizontalCheck{nullptr};

    /** The marks, held here until a template exists to hold them — as the frame is. */
    OExportMarkSettings m_marks{};

    QGroupBox *m_paginationGroup{nullptr};
    QLabel *m_paginationFormLabel{nullptr};
    QComboBox *m_paginationFormCombo{nullptr};
    QCheckBox *m_centreNumberCheck{nullptr};
    QCheckBox *m_cornerNumberCheck{nullptr};
    QCheckBox *m_verticalNumberCheck{nullptr};
    QCheckBox *m_horizontalNumberCheck{nullptr};

    /**
     * One per placement, each shown and hidden with the box that owns it. Four of them — the
     * centre, the corners, and the two axes of the seams — because each placement carries its own
     * style and none inherits another's.
     */
    TextStyleRow m_centreStyle{};
    TextStyleRow m_cornerStyle{};
    TextStyleRow m_verticalStyle{};
    TextStyleRow m_horizontalStyle{};

    /** The pagination, held here until a template exists to hold it — as the frame is. */
    OExportPaginationSettings m_pagination{};

    /** The 50/50 split is asked for once, when there is a real width to divide. */
    bool m_splitterSized{false};

    /** Placeholders, kept only so a language change can reach their text. */
    QList<QLabel *> m_placeholders;
};

#endif // OEXPORTTEMPLATEEDITOR_H
