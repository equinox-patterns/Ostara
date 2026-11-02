/************************************************************************
 **
 **  @file   oexporttemplateeditor.cpp
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

#include "oexporttemplateeditor.h"

#include "core/ofontfaces.h"
#include "core/oexportunitformat.h"
#include "dialogs/oexportprintmodel.h"
#include "ocoverpageeditor.h"
#include "oexporttemplatepreview.h"
#include "oexporttemplatespage.h"
#include "core/oexporttemplatestorage.h"
#include "../vmisc/FactoryWidgets.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include <cmath>

namespace
{
/**
 * Menu entries of the « Assembly marks » tab, in menu order. The row index is also the
 * index of the page it shows, which is why the two are never stored apart.
 */
enum MenuRow : int
{
    MenuExistingTemplates = 0,
    MenuCreateTemplate = 1
};

constexpr int kMenuWidth = 170;

/**
 * Minimum width of each half of the « Create a template » split. Deliberately the same on both
 * sides: before an explicit split is applied, the splitter falls back on the two widgets' size
 * hints, and a hint of ~90 px against one of 200 px hands the preview two thirds of the width.
 */
constexpr int kPaneMinimum = 200;

/** Wide enough for "25.9" and no wider, so the format row stays one line. */
constexpr int kFieldWidth = 70;

/** Wide enough for a template's name, which is a word and not a measurement. */
constexpr int kNameFieldWidth = 220;

/** Two of these fit side by side in the Édition pane, captions and all. */
constexpr int kDashFieldWidth = 52;

/**
 * One dash length, read the way the size styles read theirs (ogratedsizestylesdialog.cpp:70-87):
 * a strictly positive finite number, a comma accepted for the decimal point, and an empty field
 * meaning « not set » rather than a mistake.
 */
auto ParsePoint(const QString &text, qreal *value) -> bool
{
    QString number = text.trimmed();
    if (number.isEmpty())
    {
        *value = 0.0;
        return true;
    }

    number.replace(QLatin1Char(','), QLatin1Char('.'));

    bool ok = false;
    const qreal parsed = number.toDouble(&ok);
    if (!ok || parsed <= 0.0 || !std::isfinite(parsed))
    {
        return false;
    }

    *value = parsed;
    return true;
}

/**
 * The split between the two halves — a ratio, not a width: setSizes() normalises it. The
 * preview is where the work is judged, and the pane opposite only holds controls.
 */
constexpr int kEditionShare = 30;
constexpr int kPreviewShare = 70;

/** Slack allowed when checking the splitter took the ratio — the handle eats a few pixels. */
constexpr int kRatioTolerancePx = 4;

/** The zoom slider, in percent of the fit scale: 100 shows the whole grid, 800 a corner. */
constexpr int kZoomMinimumPercent = 100;
constexpr int kZoomMaximumPercent = 800;

enum Tab : int
{
    TabCoverPage = 0,
    TabAssemblyMarks = 1
};

/**
 * Where the print-model labels are filed for translation. Spelled out here as well as in
 * oexportprintmodel.cpp: lupdate reads the literal, and this is where it is used.
 */
constexpr auto kModelContext = "OExportPrintModel";

/**
 * Where the mark-shape labels are filed for translation. Spelled out here as well as in
 * oexportmarksettings.cpp: lupdate reads the literal, and this is where it is used.
 */
constexpr auto kMarkContext = "OExportMarkSettings";

/** The context the two tables of core/oexportpaginationsettings.h register their strings under. */
constexpr auto kPaginationContext = "OExportPaginationSettings";

/**
 * The model shown when nothing else has been chosen — « A4/Letter framed », because it is the one
 * the decor applies to and the smaller of the two: opening on A3 or Letter would show a template
 * being written for a format whose sheets carry none of it.
 */
constexpr auto kDefaultModelToken = "A4LetterFramed";
} // namespace

OExportTemplateEditor::OExportTemplateEditor(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget();

    // Tab 0 is the cover page, and it is a window of its own: its family keeps its own file, its
    // own schema and its own folder, and shares nothing with the plan's decor but the shape of the
    // editor. Built here rather than in a Make… helper because the window holds it — the close
    // prompt has to reach it.
    m_coverPage = new OCoverPageEditor();
    connect(m_coverPage, &OCoverPageEditor::TemplateSaved, this,
            &OExportTemplateEditor::TemplateSaved);
    m_tabs->addTab(m_coverPage, QString());            // index 0 — cover page
    m_tabs->addTab(MakeAssemblyMarksTab(), QString()); // index 1 — PDF layout
    outer->addWidget(m_tabs);

    RetranslateUI();

    // Once, now that both halves exist: the controls are built before the preview they feed, so
    // their first values are handed over here rather than when each control was made.
    CommitFrameSettings();
    CommitMarkSettings();
    CommitPaginationSettings();

    m_tabs->setCurrentIndex(TabAssemblyMarks);
    ShowMenuPage(MenuCreateTemplate);
    ApplyMenuRow();

    // The editor has just filled itself in from its own defaults, and that is not an edit.
    // Opening the window on a template nobody has touched must not offer to save it.
    SetModified(false);

    // **Leaving the tab one is editing is the moment to ask about it** — the same rule as the left
    // menu of this window and of the two other template editors. A tab switch is also what fires the
    // focus-out of a half-typed field (:1010, :1134), so it is the very moment the question has to
    // be asked.
    //
    // **The question comes before the move, not after.** Qt has already changed the page by the time
    // currentChanged is emitted, so the switch is put back first and only then asked about: a window
    // that has already left answers a question the user has not given yet — Sophie saw the cover
    // page behind a dialog asking whether to leave the layout. The move back happens inside the
    // signal, before anything is painted, so nothing flickers.
    //
    // The two documents are counted separately — the question concerns the tab being left, and the
    // assembly marks' flag says nothing about the cover page. Once the answer is given, the document
    // left behind records it, so the close prompt does not ask a second time about work already
    // answered for; that is exactly what the two other editors do with their own menu.
    m_previousTab = m_tabs->currentIndex();
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (m_restoringTab)
        {
            m_previousTab = index;
            return;
        }

        const int left = m_previousTab;
        if (index == left)
        {
            return;
        }

        const bool leftUnsavedTab = (left == TabCoverPage) ? m_coverPage->IsModified() : m_modified;
        if (!leftUnsavedTab)
        {
            m_previousTab = index;
            return;
        }

        // Back where the work is, before asking. The handler lets this move through without asking.
        const auto moveTo = [this](int tab) {
            m_restoringTab = true;
            m_tabs->setCurrentIndex(tab);
            m_restoringTab = false;
        };
        moveTo(left);

        const bool leavingCoverPage = (left == TabCoverPage);
        OTemplateDocumentEdits *leaving =
            leavingCoverPage ? static_cast<OTemplateDocumentEdits *>(m_coverPage) : this;
        if (!ConfirmDiscardChanges(leaving))
        {
            // Cancelled, or a save that did not happen: the tab is already back on the work.
            return;
        }

        // The answer is **carried out**, not merely recorded. A « don't save » leaves the document
        // bit-for-bit as it was and still unsaved, which is how the caller recognises it: what it
        // abandoned is put back to the version on disk, so coming back to the tab shows what is
        // saved and not what was refused. A « save » has already cleared the flag, and has nothing
        // left to undo here.
        if (leaving->HasUnsavedChanges())
        {
            if (leavingCoverPage)
            {
                m_coverPage->DiscardUnsavedChanges();
            }
            else
            {
                DiscardUnsavedChanges();
            }
        }

        // Answered for — the switch the user asked for may happen now.
        moveTo(index);
    });
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::OpenOnTab(int tab)
{
    if (tab >= 0 && tab < m_tabs->count())
    {
        m_tabs->setCurrentIndex(tab);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RetranslateUI()
{
    m_tabs->setTabText(TabCoverPage, tr("Cover page"));
    m_tabs->setTabText(TabAssemblyMarks, tr("PDF layout"));

    m_nameLabel->setText(tr("Name:"));
    m_nameEdit->setPlaceholderText(tr("Template name"));
    m_saveButton->setText(tr("Save"));

    m_menuList->item(MenuExistingTemplates)->setText(tr("Existing templates"));
    m_menuList->item(MenuCreateTemplate)->setText(tr("Create a template"));

    // The sheet being previewed is not a property of the template - one template decorates any
    // tiled format — so the label presents the row as a way of looking, not as a setting.
    m_formatLabel->setText(tr("Preview format:"));

    m_frameGroup->setTitle(tr("Frame"));
    m_frameNote->setText(tr("Used by the framed formats only."));
    m_frameWidthLabel->setText(tr("Thickness:"));
    m_frameStyleLabel->setText(tr("Line:"));
    m_dashLabel->setText(tr("Dash:"));
    m_spaceLabel->setText(tr("Space:"));
    m_frameOpacityLabel->setText(tr("Opacity:"));
    m_scissorsHorizontalCheck->setText(tr("Scissors horizontal"));
    m_scissorsVerticalCheck->setText(tr("Scissors vertical"));
    m_scissorsOpacityLabel->setText(tr("Scissors opacity:"));

    m_marksGroup->setTitle(tr("Assembly marks"));
    m_shapeLabel->setText(tr("Shape:"));
    m_sizeLabel->setText(tr("Size:"));
    m_markOpacityLabel->setText(tr("Opacity:"));
    m_cornerCheck->setText(tr("Corner"));
    m_verticalCheck->setText(tr("Vertical center"));
    m_horizontalCheck->setText(tr("Horizontal center"));

    m_paginationGroup->setTitle(tr("Pagination"));
    m_paginationFormLabel->setText(tr("Numbering:"));
    m_centreNumberCheck->setText(tr("Show the number at the center of the page"));
    m_cornerNumberCheck->setText(tr("Show the number at the corners of the sheets"));
    m_verticalNumberCheck->setText(tr("Show the number at the vertical center of the sheets"));
    m_horizontalNumberCheck->setText(tr("Show the number at the horizontal center of the sheets"));
    RetranslateTextStyle(m_centreStyle);
    RetranslateTextStyle(m_cornerStyle);
    RetranslateTextStyle(m_verticalStyle);
    RetranslateTextStyle(m_horizontalStyle);

    PopulateFrameStyles();
    RefreshFrameFields();
    PopulateMarkShapes();
    RefreshMarkFields();
    PopulatePaginationCombos();
    RefreshPaginationFields();

    PopulatePreviewFormats();
    RetranslateUnits();
    RefreshPreviewSheet();
    RefreshZoomLabel();

    if (m_templatesPage != nullptr)
    {
        m_templatesPage->RetranslateUI();
    }

    if (m_coverPage != nullptr)
    {
        m_coverPage->RetranslateUI();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::PopulatePreviewFormats()
{
    const QString previous = m_formatCombo->currentData().toString();

    const QSignalBlocker blocker(m_formatCombo);
    m_formatCombo->clear();

    for (const OExportPrintModel &model : OExportPrintModels())
    {
        // Only the tiled formats have sheets to decorate, and the table already says which
        // those are: a roll is one continuous length, and A0 is never placed side by side —
        // it is marked not selectable, so it is not offered anywhere, here included.
        if (!model.selectable || model.shape != OExportPrintShape::Tiled)
        {
            continue;
        }

        m_formatCombo->addItem(QCoreApplication::translate(kModelContext, model.label),
                               QLatin1String(model.token));
    }

    // Kept by token, never by index: translating the labels rebuilds the list, and the model
    // the user was looking at must survive that.
    const int restored = previous.isEmpty() ? -1 : m_formatCombo->findData(previous);
    m_formatCombo->setCurrentIndex(restored >= 0 ? restored
                                                 : m_formatCombo->findData(QString::fromLatin1(kDefaultModelToken)));
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::PopulateFrameStyles()
{
    // Rebuilt on every language change, so the two names are written here and never held as
    // literals elsewhere. Blocked: clearing the combo would otherwise announce a change of
    // style to the model, and the model is exactly what is being read.
    const QSignalBlocker blocker(m_frameStyleCombo);

    m_frameStyleCombo->clear();
    m_frameStyleCombo->addItem(tr("Solid"));
    m_frameStyleCombo->addItem(tr("Dashed"));
    m_frameStyleCombo->setCurrentIndex(m_frame.dashed ? 1 : 0);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RefreshFrameFields()
{
    // Blocked throughout: this writes the model INTO the controls, and a control announcing a
    // change would have the model read back off itself half-written.
    const QSignalBlocker widthBlocker(m_frameWidthSpin);
    const QSignalBlocker styleBlocker(m_frameStyleCombo);
    const QSignalBlocker dashBlocker(m_dashEdit);
    const QSignalBlocker spaceBlocker(m_spaceEdit);
    const QSignalBlocker opacityBlocker(m_frameOpacitySpin);
    const QSignalBlocker horizontalBlocker(m_scissorsHorizontalCheck);
    const QSignalBlocker verticalBlocker(m_scissorsVerticalCheck);
    const QSignalBlocker scissorsOpacityBlocker(m_scissorsOpacitySpin);

    // EVERY control of the group, and not only the ones whose visibility depends on the model.
    // An opening that fills half of them leaves the rest showing the last template's values, and
    // the first click on any of those writes them into the one that was just opened.
    m_frameWidthSpin->setValue(m_frame.widthPt);
    // The style combo holds Solid then Dashed, in the order PopulateFrameStyles() adds them.
    m_frameStyleCombo->setCurrentIndex(m_frame.dashed ? 1 : 0);
    m_frameOpacitySpin->setValue(m_frame.frameOpacity);
    m_scissorsHorizontalCheck->setChecked(m_frame.scissorsHorizontal);
    m_scissorsVerticalCheck->setChecked(m_frame.scissorsVertical);
    m_scissorsOpacitySpin->setValue(m_frame.scissorsOpacity);

    m_dashLabel->setVisible(m_frame.dashed);
    m_dashEdit->setVisible(m_frame.dashed);
    m_spaceLabel->setVisible(m_frame.dashed);
    m_spaceEdit->setVisible(m_frame.dashed);

    // Written from the model, which holds only what was accepted. A half-typed pattern is not
    // kept as such, and coming back to the dashes finds the last one that worked.
    const qreal dash = m_frame.dashes.size() > 0 ? m_frame.dashes.at(0) : 0.0;
    const qreal space = m_frame.dashes.size() > 1 ? m_frame.dashes.at(1) : 0.0;
    m_dashEdit->setText(dash > 0.0 ? QString::number(RoundForDisplay(dash)) : QString());
    m_spaceEdit->setText(space > 0.0 ? QString::number(RoundForDisplay(space)) : QString());
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::PopulateMarkShapes()
{
    // Rebuilt on every language change, from the shape table, so that no file name and no label
    // is written twice. Blocked: clearing the combo would otherwise announce a change of shape
    // to the model, which is what is being read.
    const QSignalBlocker blocker(m_shapeCombo);

    m_shapeCombo->clear();
    for (const OExportMarkShapeModel &model : OExportMarkShapes())
    {
        m_shapeCombo->addItem(QCoreApplication::translate(kMarkContext, model.label),
                              QLatin1String(model.token));
    }

    // The table's order is the enum's, which is what the shape value indexes.
    m_shapeCombo->setCurrentIndex(static_cast<int>(m_marks.shape));
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::PopulatePaginationCombos()
{
    // Rebuilt on every language change, from the two tables, as the shape combo is. Blocked:
    // clearing a combo would otherwise announce a change to the model, which is what is being
    // read.
    const QSignalBlocker formBlocker(m_paginationFormCombo);

    m_paginationFormCombo->clear();
    for (const OExportPaginationFormModel &model : OExportPaginationForms())
    {
        m_paginationFormCombo->addItem(QCoreApplication::translate(kPaginationContext, model.label),
                                       QLatin1String(model.token));
    }

    // The table's order is the enum's, which is what the form value indexes.
    m_paginationFormCombo->setCurrentIndex(static_cast<int>(m_pagination.form));

}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RefreshPaginationFields()
{
    // Blocked: this writes the model's values into the controls, and a control announcing a
    // change would have the model read back off itself half-written.
    const QSignalBlocker formBlocker(m_paginationFormCombo);
    const QSignalBlocker centreBlocker(m_centreNumberCheck);
    const QSignalBlocker cornerBlocker(m_cornerNumberCheck);
    const QSignalBlocker verticalBlocker(m_verticalNumberCheck);
    const QSignalBlocker horizontalBlocker(m_horizontalNumberCheck);

    // The form table's order is the enum's, which is what the form value indexes.
    m_paginationFormCombo->setCurrentIndex(static_cast<int>(m_pagination.form));

    m_centreNumberCheck->setChecked(m_pagination.centre.enabled);
    m_cornerNumberCheck->setChecked(m_pagination.corner.enabled);
    m_verticalNumberCheck->setChecked(m_pagination.verticalCentre.enabled);
    m_horizontalNumberCheck->setChecked(m_pagination.horizontalCentre.enabled);

    RefreshTextStyle(m_centreStyle, m_pagination.centre.style);
    RefreshTextStyle(m_cornerStyle, m_pagination.corner.style);
    RefreshTextStyle(m_verticalStyle, m_pagination.verticalCentre.style);
    RefreshTextStyle(m_horizontalStyle, m_pagination.horizontalCentre.style);

    // The style is the placement's own, so it appears with it and goes with it.
    m_centreStyle.row->setVisible(m_pagination.centre.enabled);
    m_cornerStyle.row->setVisible(m_pagination.corner.enabled);
    m_verticalStyle.row->setVisible(m_pagination.verticalCentre.enabled);
    m_horizontalStyle.row->setVisible(m_pagination.horizontalCentre.enabled);
}

//---------------------------------------------------------------------------------------------------------------------

OExportTemplateEditor::TextStyleRow OExportTemplateEditor::MakeTextStyleRow()
{
    TextStyleRow widgets;

    widgets.row = new QWidget();
    auto *layout = new QHBoxLayout(widgets.row);
    layout->setContentsMargins(0, 0, 0, 0);

    // One label above one field, per setting, so the four stay aligned as columns whatever width
    // each of them asks for. Four controls on a single line, and the labels above rather than
    // beside: laid out beside them they would want more width than the pane has, and Qt would
    // take the difference from the preview rather than from them.
    const auto addColumn = [&layout](QLabel *label, QWidget *field) {
        auto *column = new QVBoxLayout();
        column->setSpacing(2);
        column->addWidget(label);
        column->addWidget(field);
        layout->addLayout(column);
    };

    widgets.fontLabel = new QLabel();
    widgets.fontCombo = new QFontComboBox();
    addColumn(widgets.fontLabel, widgets.fontCombo);

    widgets.weightLabel = new QLabel();
    widgets.weightCombo = MakeComboBox();
    addColumn(widgets.weightLabel, widgets.weightCombo);

    widgets.sizeLabel = new QLabel();
    widgets.sizeSpin = new QSpinBox();
    widgets.sizeSpin->setRange(1, 288);
    widgets.sizeSpin->setSuffix(QStringLiteral(" px"));
    addColumn(widgets.sizeLabel, widgets.sizeSpin);

    widgets.opacityLabel = new QLabel();
    widgets.opacitySpin = new QSpinBox();
    widgets.opacitySpin->setRange(0, 100);
    widgets.opacitySpin->setSuffix(QStringLiteral(" %"));
    addColumn(widgets.opacityLabel, widgets.opacitySpin);

    return widgets;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RetranslateTextStyle(const TextStyleRow &widgets)
{
    // The same four words for every placement: each row is read under the box that reveals it, so
    // naming the placement again here would only repeat what the box already says.
    widgets.fontLabel->setText(tr("Font:"));
    widgets.weightLabel->setText(tr("Weight:"));
    widgets.sizeLabel->setText(tr("Size:"));
    widgets.opacityLabel->setText(tr("Opacity:"));
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RefreshTextStyle(const TextStyleRow &widgets,
                                             const OExportPaginationTextStyle &style)
{
    // Blocked throughout, for the reason given in RefreshPaginationFields.
    const QSignalBlocker fontBlocker(widgets.fontCombo);
    const QSignalBlocker weightBlocker(widgets.weightCombo);
    const QSignalBlocker sizeBlocker(widgets.sizeSpin);
    const QSignalBlocker opacityBlocker(widgets.opacitySpin);

    // Empty means the default face, and the default face is the application's own — Ostara sets
    // it in VTheme::InitFonts(). The combo cannot hold an empty family, so an empty one is shown
    // by putting the combo back on its own widget font, which IS that face: the preview draws
    // with the same one, so the two agree.
    //
    // Measured, not assumed: with the application font set as Ostara sets it, a QFontComboBox
    // reports currentFont() == font() == "Inter" until it is moved, and setCurrentFont(font())
    // brings it back. Without that setFont, the combo stays on the system font instead — which is
    // why this restores rather than names.
    widgets.fontCombo->setCurrentFont(style.fontFamily.isEmpty() ? widgets.fontCombo->font()
                                                                 : QFont(style.fontFamily));

    // **The faces belong to the family**, so the list is built for the family the combo now shows —
    // and it is built HERE rather than once for the four placements, because the four can be set in
    // four different families. The previous selection is dropped first: it is not the stored answer,
    // and `FillFontFaces` keeps whatever the combo holds. A stored WEIGHT is resolved too, so a
    // template written before the faces were named keeps the face it was drawn in.
    widgets.weightCombo->setCurrentIndex(-1);
    FillFontFaces(widgets.weightCombo, widgets.fontCombo->currentFont().family(), style.fontFace);

    widgets.sizeSpin->setValue(style.sizePx);
    widgets.opacitySpin->setValue(style.opacity);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::CommitTextStyle(const TextStyleRow &widgets,
                                            OExportPaginationTextStyle &style)
{
    const QString family = widgets.fontCombo->currentFont().family();

    // Choosing the application's own font IS choosing the default, and is stored as such. Writing
    // its name instead would pin the template to a face it was following: the day the application
    // font changes, a pinned template keeps the old one and an empty one follows — and the two
    // would look identical today, so the difference would be invisible until it bit.
    style.fontFamily =
        (family == widgets.fontCombo->font().family()) ? QString() : family;

    // **A face NAME, never a number** — and empty means the family's Regular, which is what the
    // control shows when the family offers nothing else (see @ref FillFontFaces).
    style.fontFace = widgets.weightCombo->currentData().toString();
    style.sizePx = widgets.sizeSpin->value();
    style.opacity = widgets.opacitySpin->value();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RefreshMarkFields()
{
    // Blocked throughout, for the reason given in RefreshFrameFields.
    const QSignalBlocker shapeBlocker(m_shapeCombo);
    const QSignalBlocker sizeBlocker(m_sizeEdit);
    const QSignalBlocker unitBlocker(m_sizeUnitCombo);
    const QSignalBlocker opacityBlocker(m_markOpacitySpin);
    const QSignalBlocker cornerBlocker(m_cornerCheck);
    const QSignalBlocker verticalBlocker(m_verticalCheck);
    const QSignalBlocker horizontalBlocker(m_horizontalCheck);

    // The shape table's order is the enum's, which is what the shape value indexes.
    m_shapeCombo->setCurrentIndex(static_cast<int>(m_marks.shape));
    m_sizeEdit->setText(OExportUnitFormat::Format(m_marks.sizeMm, m_marks.sizeUnit));
    m_sizeUnitCombo->setCurrentIndex(m_sizeUnitCombo->findData(static_cast<int>(m_marks.sizeUnit)));
    m_markOpacitySpin->setValue(m_marks.opacity);
    m_cornerCheck->setChecked(m_marks.onCorner);
    m_verticalCheck->setChecked(m_marks.onVerticalCentre);
    m_horizontalCheck->setChecked(m_marks.onHorizontalCentre);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RefreshZoomLabel()
{
    m_zoomLabel->setText(tr("Zoom: %1 %").arg(m_zoomSlider->value()));
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RetranslateUnits()
{
    // The unit names come from the application, not from this widget, so they are re-read
    // rather than translated here — the same way the print tab does it.
    for (QComboBox *combo : {m_sizeUnitCombo})
    {
        for (int i = 0; i < combo->count(); ++i)
        {
            const auto unit = static_cast<Unit>(combo->itemData(i).toInt());
            combo->setItemText(i, UnitsToStr(unit, true));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RefreshPreviewSheet()
{
    const OExportPrintModel *model = FindPrintModel(m_formatCombo->currentData().toString());
    if (model == nullptr)
    {
        return;
    }

    // Whether the plan is cut into a framed sheet — the two framed formats, and them alone. The
    // model says so, and nothing in this row takes it back: it is the same reading the Export
    // makes of the same table, so the preview cannot show a decor the sheets will not carry.
    const bool framed = model->hasFrame;

    // The controls stay, whatever the preview is showing. Hiding them would say the template is
    // not meant for the format on screen, when the truth is the opposite: one template decorates
    // every tiled format, and the frame effaces itself where it has nothing to mark. That is
    // exactly what the note at the head of the group is there to explain.
    //
    // What does follow the format is the preview: the decor where it applies, and a faint
    // stand-in outline where it does not, so that the sheets can still be told apart.
    m_preview->SetSheetFramed(framed);

    // The sheet comes from the model, and from nowhere else: it is the format being previewed that
    // has a size, and no format has a size anyone types any more.
    m_preview->SetSheetSizeMm(QSizeF(model->widthMm, model->heightMm));
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::CommitFrameSettings()
{
    m_frame.widthPt = m_frameWidthSpin->value();
    m_frame.dashed = m_frameStyleCombo->currentIndex() == 1;

    // The pattern is read only while it is in use, so that going back to a solid line keeps it
    // for the next time the dashes are asked for.
    if (m_frame.dashed)
    {
        qreal dash = 0.0;
        qreal space = 0.0;

        // Both lengths must be readable. A dash without its space is not a pattern — the size
        // styles refuse one too — and a line whose fields cannot be read keeps the pattern it
        // had rather than being left with none.
        if (ParsePoint(m_dashEdit->text(), &dash) && ParsePoint(m_spaceEdit->text(), &space)
            && dash > 0.0 && space > 0.0)
        {
            m_frame.dashes = {dash, space};
        }
    }

    m_frame.frameOpacity = m_frameOpacitySpin->value();
    m_frame.scissorsHorizontal = m_scissorsHorizontalCheck->isChecked();
    m_frame.scissorsVertical = m_scissorsVerticalCheck->isChecked();
    m_frame.scissorsOpacity = m_scissorsOpacitySpin->value();

    // The controls are built before the preview they feed — the Édition pane, on the left, is
    // made before the right-hand column it faces — so a value handed over during that
    // construction arrives here with nowhere to go. The constructor calls this once more, when
    // both halves exist.
    if (m_preview != nullptr)
    {
        m_preview->SetFrameSettings(m_frame);
    }

    SetModified(true);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::CommitMarkSettings()
{
    // The combo follows the shape table, which follows the enum: the index is the value.
    const int shape = m_shapeCombo->currentIndex();
    if (shape >= 0 && shape < OExportMarkShapes().size())
    {
        m_marks.shape = static_cast<OExportMarkShape>(shape);
    }

    bool sizeOk = false;
    const qreal sizeMm = OExportUnitFormat::Parse(m_sizeEdit->text(), m_marks.sizeUnit, &sizeOk);

    // A size that cannot be read keeps the previous one rather than collapsing the mark to
    // nothing, exactly as the preview format's two fields do.
    if (sizeOk && sizeMm > 0.0)
    {
        m_marks.sizeMm = sizeMm;
    }

    m_marks.opacity = m_markOpacitySpin->value();
    m_marks.onCorner = m_cornerCheck->isChecked();
    m_marks.onVerticalCentre = m_verticalCheck->isChecked();
    m_marks.onHorizontalCentre = m_horizontalCheck->isChecked();

    // Rewritten from what was accepted, so the field shows the size that is actually in force.
    RefreshMarkFields();

    if (m_preview != nullptr)
    {
        m_preview->SetMarkSettings(m_marks);
    }

    SetModified(true);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::CommitPaginationSettings()
{
    m_pagination.form = static_cast<OExportPaginationForm>(m_paginationFormCombo->currentIndex());
    m_pagination.centre.enabled = m_centreNumberCheck->isChecked();
    m_pagination.corner.enabled = m_cornerNumberCheck->isChecked();
    m_pagination.verticalCentre.enabled = m_verticalNumberCheck->isChecked();
    m_pagination.horizontalCentre.enabled = m_horizontalNumberCheck->isChecked();

    CommitTextStyle(m_centreStyle, m_pagination.centre.style);
    CommitTextStyle(m_cornerStyle, m_pagination.corner.style);
    CommitTextStyle(m_verticalStyle, m_pagination.verticalCentre.style);
    CommitTextStyle(m_horizontalStyle, m_pagination.horizontalCentre.style);

    // The style is the placement's own, so it appears with it and goes with it. Ticking the box
    // is the whole of what shows it: there is nothing to configure before it is ticked, and
    // nothing left to configure once it is not.
    m_centreStyle.row->setVisible(m_pagination.centre.enabled);
    m_cornerStyle.row->setVisible(m_pagination.corner.enabled);
    m_verticalStyle.row->setVisible(m_pagination.verticalCentre.enabled);
    m_horizontalStyle.row->setVisible(m_pagination.horizontalCentre.enabled);

    if (m_preview != nullptr)
    {
        m_preview->SetPaginationSettings(m_pagination);
    }

    SetModified(true);
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeAssemblyMarksTab()
{
    auto *page = new QWidget();

    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_menuList = new QListWidget();
    m_menuList->setFixedWidth(kMenuWidth);
    m_menuList->addItem(QString()); // index 0 — existing templates
    m_menuList->addItem(QString()); // index 1 — create a template
    connect(m_menuList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        // **Leaving the template one is editing is the moment to ask about it — not arriving on
        // it.** The row this guards opens the library, which is consulted rather than edited, and
        // nothing in this menu leads back to the template held in memory: clicking it used to walk
        // away in silence and leave the work in progress behind, with the close prompt as the only
        // question — and far too late. The two other template editors ask at this same moment.
        //
        // Asking for « Create a template » while already there is not leaving — it is starting
        // over, and NewTemplate has its own question for that.
        const bool leavingEditor = m_stack->currentIndex() == MenuCreateTemplate
                                   && m_menuList->row(item) != MenuCreateTemplate;
        if (leavingEditor)
        {
            if (!ConfirmDiscardChanges(this))
            {
                // The click has already moved the highlight: put it back on the page still open,
                // or the menu would name one the editor never showed.
                ApplyMenuRow();
                return;
            }
            // What was saved is saved, and what was given up is put back: the page the user comes
            // back to must not show work that was refused, nor ask again about a question already
            // answered. This row leaves the marks' editor and nothing else — the cover page is a
            // document of its own, on its own tab, and it is not this gesture's business.
            if (HasUnsavedChanges())
            {
                DiscardUnsavedChanges();
            }
        }

        switch (m_menuList->row(item))
        {
            case MenuExistingTemplates:
                ShowTemplatesList();
                break;

            case MenuCreateTemplate:
                // Asking for « Create a template » is asking for a new one, not for the page it
                // was last left on. Handing back the template opened in the library would leave
                // its name in the field, and saving under another name would rename it.
                NewTemplate();
                break;

            default:
                break;
        }

        // Last, and whatever happened. An action that ran has just set the row; one that was
        // cancelled by the unsaved-changes prompt has left it alone — and in that case the click
        // has already moved the highlight, so the menu would name a page the editor never opened.
        ApplyMenuRow();
    });
    layout->addWidget(m_menuList);

    m_stack = new QStackedWidget();

    m_templatesPage = new OExportTemplatesPage();
    connect(m_templatesPage, &OExportTemplatesPage::OpenTemplate, this,
            &OExportTemplateEditor::LoadTemplate);

    m_stack->addWidget(m_templatesPage);          // index 0 — existing templates
    m_stack->addWidget(MakeCreateTemplatePage()); // index 1 — create a template
    layout->addWidget(m_stack, 1);

    return page;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeCreateTemplatePage()
{
    auto *page = new QWidget();
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    // The name and the button that saves under it, above everything this page holds. This page is
    // where a template is made, so this is where it is named: the field has nothing to say on
    // « Existing templates », and standing above the tabs it would have claimed to belong to both.
    pageLayout->addWidget(MakeTopBar());

    m_splitter = new QSplitter(Qt::Horizontal);

    auto *edition = MakeEditionPane();
    edition->setMinimumWidth(kPaneMinimum);
    m_splitter->addWidget(edition);

    // The right half is a column, not the preview alone: the format row sits above it and the
    // preview takes the rest. The format is part of what is shown, because the sheet and every
    // mark drawn on it share one scale — swapping A4 for A3 changes what is on screen, and that
    // change is the whole point of the row.
    auto *right = new QWidget();
    auto *rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(MakePreviewFormatRow());

    m_preview = new OExportTemplatePreview();

    // The preview travels inside a scroll area: past 1:1 it is larger than the pane, and the
    // pane must be able to move over it rather than clip it. The area is given an explicit
    // minimum so that a zoomed grid never drags the splitter wider with it.
    m_previewScroll = new QScrollArea();
    m_previewScroll->setWidgetResizable(true);
    m_previewScroll->setWidget(m_preview);
    m_previewScroll->setMinimumSize(kPaneMinimum, kPaneMinimum);
    rightLayout->addWidget(m_previewScroll, 1);

    rightLayout->addWidget(MakeZoomRow());

    right->setMinimumWidth(kPaneMinimum);
    m_splitter->addWidget(right);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    // The ratio is applied from the splitter's first resize — see eventFilter() for why it
    // cannot be asked here, nor from showEvent().
    m_splitter->installEventFilter(this);

    // Dragging the handle is the user taking over: the ratio stops being re-applied.
    connect(m_splitter, &QSplitter::splitterMoved, this, [this](int, int) { m_splitterSized = true; });

    pageLayout->addWidget(m_splitter, 1);

    return page;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakePreviewFormatRow()
{
    auto *row = new QWidget();

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    m_formatLabel = new QLabel(row);
    layout->addWidget(m_formatLabel);

    m_formatCombo = MakeComboBox(row);
    connect(m_formatCombo, &QComboBox::currentIndexChanged, this, [this](int) { RefreshPreviewSheet(); });
    layout->addWidget(m_formatCombo);

    // Nothing follows the combo. The two size fields that stood here existed for the one format
    // whose sides were free; every format now has a sheet of its own, so the size is read off the
    // model — and with it went the unit combo, which was only ever there to write those two.

    layout->addStretch(1);

    return row;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeZoomRow()
{
    auto *row = new QWidget();

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);

    m_zoomLabel = new QLabel(row);
    layout->addWidget(m_zoomLabel);

    m_zoomSlider = new QSlider(Qt::Horizontal, row);
    m_zoomSlider->setRange(kZoomMinimumPercent, kZoomMaximumPercent);
    m_zoomSlider->setSingleStep(25);
    m_zoomSlider->setPageStep(100);
    m_zoomSlider->setValue(kZoomMinimumPercent);
    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int percent) {
        m_preview->SetZoomFactor(percent / 100.0);
        RefreshZoomLabel();
    });
    layout->addWidget(m_zoomSlider, 1);

    return row;
}

//---------------------------------------------------------------------------------------------------------------------

bool OExportTemplateEditor::eventFilter(QObject *watched, QEvent *event)
{
    // The 30/70 ratio, applied in pixels rather than as two small numbers, and only counted as
    // done once the splitter has actually taken it.
    //
    // Both halves matter. Asked from showEvent(), even queued, the call can land while the
    // splitter is still narrower than the two panes' minimum widths: the minimums then answer
    // for the sizes, the split comes out even, and the splitter keeps it — it preserves ratios,
    // not intentions, so every later resize spreads that even split wider. And because the two
    // minimums are 200 px each, any width below ~670 px is below what 30 % can be.
    //
    // So: ask again on each resize until the splitter agrees, and stop for good the moment it
    // does — or the moment the handle is dragged.
    if (watched == m_splitter && event->type() == QEvent::Resize && !m_splitterSized)
    {
        const int width = m_splitter->width();
        if (width > 0)
        {
            const QList<int> wanted{width * kEditionShare / 100, width * kPreviewShare / 100};
            m_splitter->setSizes(wanted);

            const QList<int> got = m_splitter->sizes();
            // A couple of pixels of slack: the handle between the two panes takes its own width
            // out of the total, so an exact match is not something to insist on.
            if (got.size() == wanted.size() && qAbs(got.at(0) - wanted.at(0)) <= kRatioTolerancePx)
            {
                m_splitterSized = true;
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeEditionPane()
{
    auto *pane = new QWidget();

    auto *layout = new QVBoxLayout(pane);
    // The group takes its own height and no more, held at the top of the column. Left to fill
    // it, the rows would be spread over whatever the pane is given, a screen apart.
    layout->addWidget(MakeFrameGroup(), 0);
    layout->addWidget(MakeMarksGroup(), 0);
    layout->addWidget(MakePaginationGroup(), 0);
    layout->addStretch(1);

    return pane;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeFrameGroup()
{
    m_frameGroup = new QGroupBox();
    auto *layout = new QVBoxLayout(m_frameGroup);

    // The controls stay live whatever the preview shows: one template decorates every tiled
    // format, and the frame effaces itself where it has nothing to mark. Greying them for a
    // format without a frame would stop a template being prepared for the framed ones (§3.4.1).
    m_frameNote = new QLabel();
    m_frameNote->setWordWrap(true);
    layout->addWidget(m_frameNote);

    auto *thicknessRow = new QHBoxLayout();
    m_frameWidthLabel = new QLabel();
    thicknessRow->addWidget(m_frameWidthLabel);

    m_frameWidthSpin = new QDoubleSpinBox();
    m_frameWidthSpin->setRange(0.1, 20.0);
    m_frameWidthSpin->setSingleStep(0.1);
    m_frameWidthSpin->setDecimals(2);
    m_frameWidthSpin->setSuffix(QStringLiteral(" pt"));
    m_frameWidthSpin->setValue(m_frame.widthPt);
    thicknessRow->addWidget(m_frameWidthSpin);
    thicknessRow->addStretch(1);
    layout->addLayout(thicknessRow);

    auto *styleRow = new QHBoxLayout();
    m_frameStyleLabel = new QLabel();
    styleRow->addWidget(m_frameStyleLabel);

    m_frameStyleCombo = MakeComboBox();
    styleRow->addWidget(m_frameStyleCombo);
    styleRow->addStretch(1);
    layout->addLayout(styleRow);

    // One dash and one space, on a single line: a longer pattern is not asked for.
    auto *dashRow = new QHBoxLayout();

    m_dashLabel = new QLabel();
    dashRow->addWidget(m_dashLabel);

    m_dashEdit = new QLineEdit();
    dashRow->addWidget(m_dashEdit);

    m_spaceLabel = new QLabel();
    dashRow->addWidget(m_spaceLabel);

    m_spaceEdit = new QLineEdit();
    dashRow->addWidget(m_spaceEdit);

    dashRow->addStretch(1);
    layout->addLayout(dashRow);

    for (QLineEdit *field : {m_dashEdit, m_spaceEdit})
    {
        field->setPlaceholderText(QStringLiteral("pt"));
        field->setFixedWidth(kDashFieldWidth);
        field->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        connect(field, &QLineEdit::editingFinished, this, &OExportTemplateEditor::CommitFrameSettings);
    }

    auto *frameOpacityRow = new QHBoxLayout();
    m_frameOpacityLabel = new QLabel();
    frameOpacityRow->addWidget(m_frameOpacityLabel);

    m_frameOpacitySpin = new QSpinBox();
    m_frameOpacitySpin->setRange(0, 100);
    m_frameOpacitySpin->setSuffix(QStringLiteral(" %"));
    m_frameOpacitySpin->setValue(m_frame.frameOpacity);
    frameOpacityRow->addWidget(m_frameOpacitySpin);
    frameOpacityRow->addStretch(1);
    layout->addLayout(frameOpacityRow);

    m_scissorsHorizontalCheck = new QCheckBox();
    m_scissorsHorizontalCheck->setChecked(m_frame.scissorsHorizontal);
    layout->addWidget(m_scissorsHorizontalCheck);

    m_scissorsVerticalCheck = new QCheckBox();
    m_scissorsVerticalCheck->setChecked(m_frame.scissorsVertical);
    layout->addWidget(m_scissorsVerticalCheck);

    auto *scissorsOpacityRow = new QHBoxLayout();
    m_scissorsOpacityLabel = new QLabel();
    scissorsOpacityRow->addWidget(m_scissorsOpacityLabel);

    m_scissorsOpacitySpin = new QSpinBox();
    m_scissorsOpacitySpin->setRange(0, 100);
    m_scissorsOpacitySpin->setSuffix(QStringLiteral(" %"));
    m_scissorsOpacitySpin->setValue(m_frame.scissorsOpacity);
    scissorsOpacityRow->addWidget(m_scissorsOpacitySpin);
    scissorsOpacityRow->addStretch(1);
    layout->addLayout(scissorsOpacityRow);

    // The group keeps its natural height. Without this the rows are spread over whatever the
    // column is given, and a handful of controls end up a screen apart.
    layout->addStretch(1);

    connect(m_frameStyleCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        m_frame.dashed = m_frameStyleCombo->currentIndex() == 1;

        // The pattern is rewritten from the model, not lost: it is what was last typed and
        // accepted, and coming back to the dashes must find it again.
        RefreshFrameFields();
        CommitFrameSettings();
    });
    connect(m_frameWidthSpin, &QDoubleSpinBox::valueChanged, this,
            &OExportTemplateEditor::CommitFrameSettings);
    connect(m_frameOpacitySpin, &QSpinBox::valueChanged, this,
            &OExportTemplateEditor::CommitFrameSettings);
    connect(m_scissorsHorizontalCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitFrameSettings);
    connect(m_scissorsVerticalCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitFrameSettings);
    connect(m_scissorsOpacitySpin, &QSpinBox::valueChanged, this,
            &OExportTemplateEditor::CommitFrameSettings);

    return m_frameGroup;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeMarksGroup()
{
    m_marksGroup = new QGroupBox();
    auto *layout = new QVBoxLayout(m_marksGroup);

    auto *shapeRow = new QHBoxLayout();
    m_shapeLabel = new QLabel();
    shapeRow->addWidget(m_shapeLabel);

    // Built in PopulateMarkShapes(), from the shape table — never written out here.
    m_shapeCombo = MakeComboBox();
    shapeRow->addWidget(m_shapeCombo);
    shapeRow->addStretch(1);
    layout->addLayout(shapeRow);

    auto *sizeRow = new QHBoxLayout();
    m_sizeLabel = new QLabel();
    sizeRow->addWidget(m_sizeLabel);

    m_sizeEdit = new QLineEdit();
    m_sizeEdit->setMaximumWidth(kFieldWidth);
    sizeRow->addWidget(m_sizeEdit);

    m_sizeUnitCombo = MakeComboBox();
    for (const Unit &unit : OExportUnitFormat::SelectableUnits())
    {
        m_sizeUnitCombo->addItem(UnitsToStr(unit, true), static_cast<int>(unit));
    }
    m_sizeUnitCombo->setCurrentIndex(m_sizeUnitCombo->findData(static_cast<int>(m_marks.sizeUnit)));
    sizeRow->addWidget(m_sizeUnitCombo);
    sizeRow->addStretch(1);
    layout->addLayout(sizeRow);

    auto *opacityRow = new QHBoxLayout();
    m_markOpacityLabel = new QLabel();
    opacityRow->addWidget(m_markOpacityLabel);

    m_markOpacitySpin = new QSpinBox();
    m_markOpacitySpin->setRange(0, 100);
    m_markOpacitySpin->setSuffix(QStringLiteral(" %"));
    m_markOpacitySpin->setValue(m_marks.opacity);
    opacityRow->addWidget(m_markOpacitySpin);
    opacityRow->addStretch(1);
    layout->addLayout(opacityRow);

    m_cornerCheck = new QCheckBox();
    m_cornerCheck->setChecked(m_marks.onCorner);
    layout->addWidget(m_cornerCheck);

    m_verticalCheck = new QCheckBox();
    m_verticalCheck->setChecked(m_marks.onVerticalCentre);
    layout->addWidget(m_verticalCheck);

    m_horizontalCheck = new QCheckBox();
    m_horizontalCheck->setChecked(m_marks.onHorizontalCentre);
    layout->addWidget(m_horizontalCheck);

    layout->addStretch(1);

    connect(m_shapeCombo, &QComboBox::currentIndexChanged, this,
            &OExportTemplateEditor::CommitMarkSettings);
    connect(m_sizeEdit, &QLineEdit::editingFinished, this,
            &OExportTemplateEditor::CommitMarkSettings);
    connect(m_sizeUnitCombo, &QComboBox::currentIndexChanged, this, [this](int) {
        // The size is a way of writing a length, not a new length: the millimetres are untouched
        // and only the field is rewritten. Re-reading it here would take "4" for four of the unit
        // just chosen.
        m_marks.sizeUnit = static_cast<Unit>(m_sizeUnitCombo->currentData().toInt());
        RefreshMarkFields();
    });
    connect(m_markOpacitySpin, &QSpinBox::valueChanged, this,
            &OExportTemplateEditor::CommitMarkSettings);
    connect(m_cornerCheck, &QCheckBox::toggled, this, &OExportTemplateEditor::CommitMarkSettings);
    connect(m_verticalCheck, &QCheckBox::toggled, this, &OExportTemplateEditor::CommitMarkSettings);
    connect(m_horizontalCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitMarkSettings);

    return m_marksGroup;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakePaginationGroup()
{
    m_paginationGroup = new QGroupBox();
    auto *layout = new QVBoxLayout(m_paginationGroup);

    auto *formRow = new QHBoxLayout();
    m_paginationFormLabel = new QLabel();
    formRow->addWidget(m_paginationFormLabel);

    // Built in PopulatePaginationCombos(), from the form table — never written out here.
    m_paginationFormCombo = MakeComboBox();
    formRow->addWidget(m_paginationFormCombo);
    formRow->addStretch(1);
    layout->addLayout(formRow);

    // What the number is made of, and then what is done with it: the rule separates the two, so
    // that the placements below are read as a list of their own rather than as more of the form.
    auto *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    layout->addWidget(separator);

    // The placements, one after another, each a box and the style that box owns. Ticking one is
    // the whole of what shows its style: there is nothing to configure before it is ticked, and
    // nothing left to configure once it is not.
    m_centreNumberCheck = new QCheckBox();
    m_centreNumberCheck->setChecked(m_pagination.centre.enabled);
    layout->addWidget(m_centreNumberCheck);

    m_centreStyle = MakeTextStyleRow();
    layout->addWidget(m_centreStyle.row);

    m_cornerNumberCheck = new QCheckBox();
    m_cornerNumberCheck->setChecked(m_pagination.corner.enabled);
    layout->addWidget(m_cornerNumberCheck);

    m_cornerStyle = MakeTextStyleRow();
    layout->addWidget(m_cornerStyle.row);

    m_verticalNumberCheck = new QCheckBox();
    m_verticalNumberCheck->setChecked(m_pagination.verticalCentre.enabled);
    layout->addWidget(m_verticalNumberCheck);

    m_verticalStyle = MakeTextStyleRow();
    layout->addWidget(m_verticalStyle.row);

    m_horizontalNumberCheck = new QCheckBox();
    m_horizontalNumberCheck->setChecked(m_pagination.horizontalCentre.enabled);
    layout->addWidget(m_horizontalNumberCheck);

    m_horizontalStyle = MakeTextStyleRow();
    layout->addWidget(m_horizontalStyle.row);

    layout->addStretch(1);

    connect(m_paginationFormCombo, &QComboBox::currentIndexChanged, this,
            &OExportTemplateEditor::CommitPaginationSettings);
    connect(m_centreNumberCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitPaginationSettings);
    connect(m_cornerNumberCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitPaginationSettings);
    connect(m_verticalNumberCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitPaginationSettings);
    connect(m_horizontalNumberCheck, &QCheckBox::toggled, this,
            &OExportTemplateEditor::CommitPaginationSettings);

    // Every control of every style, on the one slot: it reads every placement whole, so there is
    // no per-field wiring to keep in step with the fields.
    for (const TextStyleRow *style :
         {&m_centreStyle, &m_cornerStyle, &m_verticalStyle, &m_horizontalStyle})
    {
        connect(style->fontCombo, &QFontComboBox::currentFontChanged, this,
                [this, style](const QFont &font) {
                    // **The faces belong to the family, so they change with it**: a list still
                    // offering the previous family's faces would name none of the new one's. The
                    // selection goes first — an old face is not an answer for the new family.
                    style->weightCombo->setCurrentIndex(-1);
                    FillFontFaces(style->weightCombo, font.family(), QString());
                    CommitPaginationSettings();
                });
        connect(style->weightCombo, &QComboBox::currentIndexChanged, this,
                &OExportTemplateEditor::CommitPaginationSettings);
        connect(style->sizeSpin, &QSpinBox::valueChanged, this,
                &OExportTemplateEditor::CommitPaginationSettings);
        connect(style->opacitySpin, &QSpinBox::valueChanged, this,
                &OExportTemplateEditor::CommitPaginationSettings);
    }

    return m_paginationGroup;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::ShowMenuPage(int row)
{
    if (row < 0 || row >= m_stack->count())
    {
        return;
    }

    m_stack->setCurrentIndex(row);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::ShowTemplatesList()
{
    // The library is read when it is looked at and never kept in memory: a template saved,
    // renamed or deleted since the list was last shown has to be there the next time it is.
    if (m_templatesPage != nullptr)
    {
        m_templatesPage->Refresh();
    }

    ShowMenuPage(MenuExistingTemplates);
}
//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::ApplyMenuRow()
{
    // Derived, never stored — see the declaration. The page says which row, and the file says
    // whether « Create a template » is where the user is or not.
    int row = -1;
    const int page = m_stack->currentIndex();

    if (page == MenuCreateTemplate)
    {
        // « Create a template » while a NEW template is being made, and nothing at all while one
        // that exists is being edited.
        row = m_filePath.isEmpty() ? MenuCreateTemplate : -1;
    }
    else if (page == MenuExistingTemplates)
    {
        row = page;
    }

    if (row < 0)
    {
        m_menuList->clearSelection();
        m_menuList->setCurrentRow(-1);
        return;
    }

    if (m_menuList->currentRow() != row)
    {
        m_menuList->setCurrentRow(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateEditor::ConfirmDiscardChanges(OTemplateDocumentEdits *document) -> bool
{
    // One question, asked by the socle, and **about the document the gesture actually leaves**:
    // this window holds two — its assembly marks and the cover page of its first tab — and asking
    // about the wrong one either misses real work or offers to save a template the user is not
    // touching.
    return OTemplateGuard::ConfirmDiscardChanges(this, tr("Unsaved template"), document);
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateEditor::HasUnsavedChanges() const -> bool
{
    return m_modified;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::SaveDocument()
{
    // Only the marks: the cover page is a document of its own, saved by its own answer.
    SaveAssemblyMarksTemplate();
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateEditor::DiscardUnsavedChanges() -> bool
{
    if (m_filePath.isEmpty())
    {
        // Nothing on disk to go back to: the template a template nobody has saved starts from, and
        // no name — the field is where the name lives, and it was never written anywhere.
        SetTemplate(OExportTemplate());
        m_nameEdit->clear();
        ApplyMenuRow();
        SetModified(false);
        return true;
    }

    OExportTemplate tpl;
    QString error;
    if (!OExportTemplateStorage::Load(m_filePath, &tpl, &error))
    {
        // The file that held the saved state cannot be read: there is nothing to go back to, so
        // nothing is thrown away. The document keeps its flag and the close prompt will speak again.
        QMessageBox::warning(this, tr("Discard changes"),
                             tr("The saved template could not be read back, so the changes were kept.\n\n%1")
                                 .arg(error));
        return false;
    }

    SetTemplate(tpl);
    m_nameEdit->setText(QFileInfo(m_filePath).completeBaseName());
    ApplyMenuRow();
    SetModified(false);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::NewTemplate()
{
    // Asking for a new template is asking to leave whatever is being edited, and this editor never
    // saves on its own. Without this, the click threw the changes away in silence.
    if (!ConfirmDiscardChanges(this))
    {
        return;
    }

    // A default-constructed template is exactly what a new one is: the three blocks at the values
    // their headers declare, and no name at all. The field is left empty so that its placeholder
    // reads « Template name », and Save refuses until a real one is given.
    SetTemplate(OExportTemplate());

    // No file behind it, and this is the line that matters. With a path left over from a template
    // opened a moment ago, saving under a different name would move that template's file rather
    // than write a new one — the old name would survive, the old template would not.
    m_filePath.clear();
    m_nameEdit->clear();

    ShowMenuPage(MenuCreateTemplate);
    ApplyMenuRow();
    SetModified(false);
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeTopBar()
{
    auto *bar = new QWidget();
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(8, 8, 8, 0);

    m_nameLabel = new QLabel();
    layout->addWidget(m_nameLabel);

    m_nameEdit = new QLineEdit();
    m_nameEdit->setMaximumWidth(kNameFieldWidth);
    // textEdited rather than textChanged: this fires on the user's typing and not on the editor
    // filling the field in itself, which is what keeps opening a template from counting as an
    // edit. The name is part of what is saved, so typing one is a modification.
    connect(m_nameEdit, &QLineEdit::textEdited, this, [this](const QString &) {
        SetModified(true);
    });
    layout->addWidget(m_nameEdit);
    layout->addStretch(1);

    m_saveButton = new QPushButton();
    connect(m_saveButton, &QPushButton::clicked, this, &OExportTemplateEditor::SaveTemplate);
    layout->addWidget(m_saveButton);

    return bar;
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateEditor::IsModified() const -> bool
{
    // Either family counts, and the whole of it: the two keep their work in two files in two
    // folders, and the window holds both. Asking only about the one on screen would let the
    // other's work go without a word.
    return m_modified || (m_coverPage != nullptr && m_coverPage->IsModified());
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::SetModified(bool modified)
{
    m_modified = modified;
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateEditor::Template() const -> OExportTemplate
{
    OExportTemplate tpl;
    tpl.name = m_nameEdit->text().trimmed();
    tpl.frame = m_frame;
    tpl.marks = m_marks;
    tpl.pagination = m_pagination;
    return tpl;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::SetTemplate(const OExportTemplate &tpl)
{
    m_frame = tpl.frame;
    m_marks = tpl.marks;
    m_pagination = tpl.pagination;

    RefreshFrameFields();
    RefreshMarkFields();
    RefreshPaginationFields();

    if (m_preview != nullptr)
    {
        m_preview->SetFrameSettings(m_frame);
        m_preview->SetMarkSettings(m_marks);
        m_preview->SetPaginationSettings(m_pagination);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::SaveTemplate()
{
    // The window asks for « save », not for « save the cover page »: both families are open in it,
    // and each one that has something to write writes it. A save that fails leaves its own editor
    // modified, which is what the close prompt reads to refuse to close on unsaved work.
    if (m_coverPage != nullptr && m_coverPage->IsModified())
    {
        m_coverPage->SaveTemplate();
    }

    if (m_modified)
    {
        SaveAssemblyMarksTemplate();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::SaveAssemblyMarksTemplate()
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, tr("Save template"),
                             tr("Give the template a name before saving it."));
        m_nameEdit->setFocus();
        return;
    }

    const QString target = OExportTemplateStorage::PathForName(name);

    // Another template already answers to that name. Writing over it would destroy something the
    // user never asked to touch, so the save stops and says which name is taken rather than
    // asking a question whose answer would be a guess.
    if (target != m_filePath && QFileInfo::exists(target))
    {
        QMessageBox::warning(this, tr("Save template"),
                             tr("A template called \"%1\" already exists.\n\nChoose another name.")
                                 .arg(QFileInfo(target).completeBaseName()));
        m_nameEdit->setFocus();
        return;
    }

    OExportTemplate tpl = Template();
    // What was kept of the typed name, not what was typed: the field is corrected to the name the
    // file actually carries, so that the two never disagree on screen.
    tpl.name = QFileInfo(target).completeBaseName();

    QString error;
    if (!OExportTemplateStorage::Save(tpl, target, &error))
    {
        QMessageBox::critical(this, tr("Save template"),
                              tr("Could not save the template.\n\n%1").arg(error));
        return;
    }

    // Written first, moved after: a save that fails must leave the old file where it was rather
    // than lose it. The name is the identity of a template, so a save under a new name moves it
    // instead of leaving a second copy behind.
    if (!m_filePath.isEmpty() && m_filePath != target)
    {
        QFile::remove(m_filePath);
    }

    m_filePath = target;
    m_nameEdit->setText(tpl.name);
    SetModified(false);

    if (m_templatesPage != nullptr)
    {
        m_templatesPage->Refresh();
    }

    // Last, and only on a save that happened: every path above that bailed out has returned
    // already, so this says the file on disk is now what the editor shows.
    emit TemplateSaved();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::LoadTemplate(const QString &filePath)
{
    // Opening a template replaces the one in memory just as surely as « Create a template » does,
    // so it asks the same question — and it asks it BEFORE reading the file, so a cancelled open
    // leaves nothing half-done.
    if (!ConfirmDiscardChanges(this))
    {
        return;
    }

    OExportTemplate tpl;
    QString error;
    if (!OExportTemplateStorage::Load(filePath, &tpl, &error))
    {
        QMessageBox::critical(this, tr("Open template"),
                              tr("Could not open the template.\n\n%1").arg(error));
        return;
    }

    SetTemplate(tpl);

    // The name the file carries, not the one written inside it: the file is what the library
    // lists and what the favourite points at, so the field follows the file rather than the two
    // being left to disagree.
    m_filePath = filePath;
    m_nameEdit->setText(QFileInfo(filePath).completeBaseName());

    ShowMenuPage(MenuCreateTemplate);
    ApplyMenuRow();

    SetModified(false);
}
