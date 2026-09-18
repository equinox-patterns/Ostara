/************************************************************************
 **
 **  @file   ocoverpageeditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The cover page tab — the library, the editor and the preview.
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

#include "ocoverpageeditor.h"

#include "core/ocoverpagecatalogue.h"
#include "core/ocoverpagetemplatestorage.h"
#include "ocoverpagebandeditor.h"
#include "ocoverpagetemplatespage.h"
#include "widgets/Template/otemplateeditormenu.h"
#include "widgets/Template/otemplatelanguagerow.h"
#include "ocoverpagetemplatepreview.h"
#include "widgets/Template/ologolibrarypage.h"
#include "widgets/Template/otemplatesectioneditor.h"

#include "vmisc/def.h"

#include <QComboBox>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QVBoxLayout>

namespace
{
/** The rows of the left menu, in the order they are built. */
enum MenuRow : int
{
    MenuExistingTemplates = 0,
    MenuCreateTemplate = 1,
    MenuLogos = 2
};

/** What the Edition pane asks for, so that the splitter does not close over it. */
constexpr int kPaneMinimum = 220;

/** The name field, wide enough for a name and no wider — the bar belongs to the whole page. */
constexpr int kNameFieldWidth = 260;

/**
 * The split between the two halves — a ratio, not a width: setSizes() normalises it. The page
 * itself is what the template is judged on, so the preview takes the larger half; the Edition
 * pane opposite holds the informations as they are added.
 */
constexpr int kEditionShare = 40;
constexpr int kPreviewShare = 60;

/** Slack allowed when checking the splitter took the ratio — the handle eats a few pixels. */
constexpr int kRatioTolerancePx = 4;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OCoverPageEditor::OCoverPageEditor(QWidget *parent) : QWidget(parent)
{
    // Before anything is built: the language row reads this the moment it is made, and what it
    // reads is what a new template starts from.
    m_template.languages = {DefaultUiLanguage()};

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // The menu and the pages it opens — the shared pair. What each row opens is decided here.
    m_menu = new OTemplateEditorMenu();
    connect(m_menu, &OTemplateEditorMenu::RowClicked, this, [this](int row) {
        switch (row)
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

            case MenuLogos:
                ShowLogosPage();
                break;

            default:
                break;
        }

        // Last, and whatever happened. An action that ran has just set the row; one that was
        // cancelled by the unsaved-changes prompt has left it alone — and in that case the click
        // has already moved the highlight, so the menu would name a page the editor never opened.
        ApplyMenuRow();
    });
    m_templatesPage = new OCoverPageTemplatesPage();
    connect(m_templatesPage, &OCoverPageTemplatesPage::OpenTemplate, this,
            &OCoverPageEditor::LoadTemplate);

    // The logos of the installation, whose page the cartouche's editor opens as well: the library
    // is one folder, and both families draw from it.
    m_logosPage = new OLogoLibraryPage();

    m_menu->AddPage(m_templatesPage);  // index 0 — existing templates
    m_menu->AddPage(MakeCreatePage()); // index 1 — create a template
    m_menu->AddPage(m_logosPage);      // index 2 — logos
    layout->addWidget(m_menu, 1);

    RetranslateUI();

    // Opening on « Create a template » rather than on the library: a cover page is written before
    // it is chosen, and the first thing this editor is opened for is to write one.
    ShowMenuPage(MenuCreateTemplate);
    ApplyMenuRow();

    // The editor has just filled itself in from its own defaults, and that is not an edit.
    // Opening the window on a template nobody has touched must not offer to save it.
    SetModified(false);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::RetranslateUI()
{
    m_nameLabel->setText(tr("Name:"));
    m_nameEdit->setPlaceholderText(tr("Template name"));
    m_saveButton->setText(tr("Save"));

    m_menu->SetRowLabel(MenuExistingTemplates, tr("Existing templates"));
    m_menu->SetRowLabel(MenuCreateTemplate, tr("Create a template"));
    m_menu->SetRowLabel(MenuLogos, tr("Logo"));

    // A cover page is one leaf and has no blocks, so this says « page » where Information block
    // says « block »: the same row, worded for what it is actually on.
    m_languageRow->SetLabels(tr("Cover page language"),
                             tr("You can choose several for a multilingual page"));
    m_languageRow->RetranslateUI();

    // The catalogue resolves a label when it builds the entry and keeps it, so a language change
    // has to drop what it cached before the bands read their names again — and each band re-reads
    // its own strings and refills its own add menu when it is told to.
    OCoverPageCatalogue::InvalidateCache();
    for (OCoverPageBandEditor *band : m_bands)
    {
        if (band != nullptr)
        {
            band->RetranslateUI();
        }
    }

    if (m_templatesPage != nullptr)
    {
        m_templatesPage->RetranslateUI();
    }

    if (m_logosPage != nullptr)
    {
        m_logosPage->RetranslateUI();
    }

    if (m_sectionEditor != nullptr)
    {
        m_sectionEditor->RetranslateUI();
    }

    // The page shows a name in place of an information that has nothing to draw yet, and that
    // name is translated — so the preview follows a language change like everything else.
    if (m_preview != nullptr)
    {
        m_preview->Refresh();
    }
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OCoverPageEditor::MakeCreatePage()
{
    auto *page = new QWidget();
    auto *pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    // The name and the button that saves under it, above everything this page holds. This page is
    // where a template is made, so this is where it is named: the field has nothing to say on
    // « Existing templates » or on « Libraries », and standing above the tabs it would have
    // claimed to belong to all three.
    pageLayout->addWidget(MakeTopBar());

    // Above the two panes and not inside either: a language is not a property of a pane, it is
    // what the whole template is written in, and the preview answers to it as much as the fields
    // do.
    pageLayout->addWidget(MakeLanguageRow());

    m_splitter = new QSplitter(Qt::Horizontal);

    auto *edition = MakeEditionPane();
    edition->setMinimumWidth(kPaneMinimum);
    m_splitter->addWidget(edition);

    // The right half is the page itself. No format row above it, unlike the assembly marks: what
    // those decorate is a plan whose sheet is chosen, where a cover page is one leaf of a size
    // that belongs to the preview and is not a property of the template.
    m_preview = new OCoverPageTemplatePreview();
    // Handed the template once, and pointed at the one the editor keeps: what changes from then
    // on is its contents, not which template is being shown.
    m_preview->SetTemplate(&m_template);
    m_splitter->addWidget(m_preview);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    // The ratio is applied from the splitter's first resize — see eventFilter() for why it
    // cannot be asked for here.
    m_splitter->installEventFilter(this);

    // Dragging the handle is the user taking over: the ratio stops being re-applied.
    connect(m_splitter, &QSplitter::splitterMoved, this, [this](int, int) { m_splitterSized = true; });

    pageLayout->addWidget(m_splitter, 1);

    return page;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OCoverPageEditor::MakeTopBar()
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
    connect(m_saveButton, &QPushButton::clicked, this, &OCoverPageEditor::SaveTemplate);
    layout->addWidget(m_saveButton);

    return bar;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OCoverPageEditor::MakeLanguageRow()
{
    // The shared row, held by both families. It keeps the languages it is given and reports every
    // change as one signal; what the template does with that is decided here.
    m_languageRow = new OTemplateLanguageRow();
    // The page this row sits on has no margins of its own, and a language line running edge to edge
    // would read as belonging to the window rather than to the template.
    m_languageRow->setContentsMargins(8, 0, 8, 0);

    connect(m_languageRow, &OTemplateLanguageRow::LanguagesChanged, this,
            [this](const QStringList &codes) {
                m_template.languages = codes;
                SetModified(true);
            });

    m_languageRow->SetLanguages(m_template.languages);
    return m_languageRow;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OCoverPageEditor::MakeEditionPane()
{
    auto *pane = new QWidget();

    auto *paneLayout = new QVBoxLayout(pane);
    paneLayout->setContentsMargins(0, 0, 0, 0);

    // The column scrolls. Four bands, each with its list, and the form under them are more height
    // than a window has once the bands are unfolded — and whichever band is open, the form has to
    // stay reachable. The scroll area is let through rather than shown: the pane has no panel of
    // its own to draw, and what lies behind the editor should keep showing.
    auto *scroll = new QScrollArea(pane);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    // A viewport on macOS is painted a colour of its own and ignores the palette, the global
    // stylesheet and setAutoFillBackground alike. An inline stylesheet on the viewport is the one
    // mechanism that takes, and the dot is what holds it to the viewport instead of every widget
    // inside it (SubtiliteCode §13). Transparent, since there is no colour to show.
    scroll->viewport()->setStyleSheet(QStringLiteral(".QWidget { background: transparent; }"));
    scroll->setStyleSheet(QStringLiteral(".QScrollArea { background: transparent; }"));

    auto *content = new QWidget();
    auto *layout = new QVBoxLayout(content);
    // Inset from the pane on both sides: the bands are framed, and a frame that ran edge to edge
    // would read as the pane's own border rather than as something the user places informations
    // in — and the pane is a column beside a preview, not the page.
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(6);

    // The four bands, in the order the page stacks them, and all four shown whether or not they
    // hold anything: an empty band is where an information will go, and hiding it would leave the
    // user nowhere to put one.
    for (OCoverPageTemplate::Placement placement : OCoverPageTemplate::Placements())
    {
        layout->addWidget(MakeBand(placement));
    }

    // The form for whatever is selected, built by the shared editor from the catalogue entry —
    // this window never says which controls an information has.
    m_sectionEditor = new OTemplateSectionEditor();
    connect(m_sectionEditor, &OTemplateSectionEditor::OptionsChanged, this, [this]() {
        SetModified(true);
        // The page is redrawn on every keystroke of the form, not only when an information is
        // added or removed: choosing a logo is what the preview is for, and a preview that waited
        // for the next addition would be answering a question nobody asked.
        if (m_preview != nullptr)
        {
            m_preview->Refresh();
        }
    });
    layout->addWidget(m_sectionEditor, 0);
    layout->addStretch(1);

    scroll->setWidget(content);
    paneLayout->addWidget(scroll);

    return pane;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OCoverPageEditor::MakeBand(OCoverPageTemplate::Placement placement)
{
    // One band, in its own widget: framed and named with where it sits on the page, since the four
    // frames are what makes the skeleton visible. Four bare lists would read as four lists; four
    // framed and named ones read as four places on a page, which is what they are.
    auto *band = new OCoverPageBandEditor(placement);

    // The band shows and acts, and says what the user did. What each gesture means to the template
    // is decided here — a band never reorders, removes or selects anything by itself.
    connect(band, &OCoverPageBandEditor::AddRequested, this,
            [this, placement](const QString &typeId) { OnAddSection(placement, typeId); });
    connect(band, &OCoverPageBandEditor::MoveUpRequested, this,
            [this, placement]() { OnMoveUp(placement); });
    connect(band, &OCoverPageBandEditor::MoveDownRequested, this,
            [this, placement]() { OnMoveDown(placement); });
    connect(band, &OCoverPageBandEditor::RemoveRequested, this,
            [this, placement]() { OnRemoveInformation(placement); });
    connect(band, &OCoverPageBandEditor::RowSelected, this,
            [this, placement](int row) { SelectSection(placement, row); });

    m_bands.insert(placement, band);

    return band;
}

//---------------------------------------------------------------------------------------------------------------------


void OCoverPageEditor::ShowMenuPage(int row)
{
    m_menu->ShowPage(row);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::ShowTemplatesList()
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

void OCoverPageEditor::ApplyMenuRow()
{
    // Derived, never stored — see the declaration. The page says which row, and the file says
    // whether « Create a template » is where the user is or not.
    int row = -1;
    const int page = m_menu->CurrentPage();

    if (page == MenuCreateTemplate)
    {
        // « Create a template » while a NEW template is being made, and nothing at all while one
        // that exists is being edited.
        row = m_filePath.isEmpty() ? MenuCreateTemplate : -1;
    }
    else if (page == MenuExistingTemplates || page == MenuLogos)
    {
        row = page;
    }

    m_menu->HighlightRow(row);
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageEditor::ConfirmDiscardChanges() -> bool
{
    return OTemplateGuard::ConfirmDiscardChanges(this, tr("Unsaved template"), this);
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageEditor::HasUnsavedChanges() const -> bool
{
    return IsModified();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::SaveDocument()
{
    SaveTemplate();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::NewTemplate()
{
    // Asking for a new template is asking to leave whatever is being edited, and this editor never
    // saves on its own. Without this, the click threw the changes away in silence.
    if (!ConfirmDiscardChanges())
    {
        return;
    }

    // No file behind it, and this is the line that matters. With a path left over from a template
    // opened a moment ago, saving under a different name would move that template's file rather
    // than write a new one — the old name would survive, the old template would not.
    m_filePath.clear();
    m_nameEdit->clear();

    // A default-constructed template is exactly what a new one is: no information, and the
    // application's own language — which is what a template that declares none is read as, so a
    // new one starts where an old one would land.
    m_template = OCoverPageTemplate();
    m_template.languages = {DefaultUiLanguage()};
    m_languageRow->SetLanguages(m_template.languages);
    RefreshAllBands();

    ShowMenuPage(MenuCreateTemplate);
    ApplyMenuRow();
    SetModified(false);
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageEditor::IsModified() const -> bool
{
    return m_modified;
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::SetModified(bool modified)
{
    m_modified = modified;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageEditor::Template() const -> OCoverPageTemplate
{
    // Everything the template is, plus the name the field carries: the name is not held in
    // m_template while it is being typed, because the field is where it lives.
    OCoverPageTemplate tpl = m_template;
    tpl.name = m_nameEdit->text().trimmed();
    return tpl;
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::SaveTemplate()
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, tr("Save template"),
                             tr("Give the template a name before saving it."));
        m_nameEdit->setFocus();
        return;
    }

    // Where the file goes, what becomes of the name it had, what happens when that name is taken:
    // all of it is the storage's business. What is left here is what to say about the outcome.
    const OCoverPageTemplateStorage::SaveResult result =
        OCoverPageTemplateStorage::SaveAs(Template(), name, m_filePath);

    switch (result.outcome)
    {
        case OCoverPageTemplateStorage::SaveOutcome::NameTaken:
            // Writing over it would destroy something the user never asked to touch, so the save
            // stops and says which name is taken rather than asking a question whose answer would
            // be a guess.
            QMessageBox::warning(this, tr("Save template"),
                                 tr("A template called \"%1\" already exists.\n\nChoose another name.")
                                     .arg(result.name));
            m_nameEdit->setFocus();
            return;

        case OCoverPageTemplateStorage::SaveOutcome::Failed:
            QMessageBox::critical(this, tr("Save template"),
                                  tr("Could not save the template.\n\n%1").arg(result.error));
            return;

        case OCoverPageTemplateStorage::SaveOutcome::Saved:
            break;
    }

    // The name the file carries, not the one that was typed: the field is corrected so that the
    // two never disagree on screen.
    m_filePath = result.filePath;
    m_nameEdit->setText(result.name);
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

void OCoverPageEditor::LoadTemplate(const QString &filePath)
{
    // Opening a template replaces the one in memory just as surely as « Create a template » does,
    // so it asks the same question — and it asks it BEFORE reading the file, so a cancelled open
    // leaves nothing half-done.
    if (!ConfirmDiscardChanges())
    {
        return;
    }

    OCoverPageTemplate tpl;
    QString error;
    if (!OCoverPageTemplateStorage::Load(filePath, &tpl, &error))
    {
        QMessageBox::critical(this, tr("Open template"),
                              tr("Could not open the template.\n\n%1").arg(error));
        return;
    }

    // The name the file carries, not the one written inside it: the file is what the library
    // lists and what the favourite points at, so the field follows the file rather than the two
    // being free to disagree.
    m_filePath = filePath;
    m_nameEdit->setText(QFileInfo(filePath).completeBaseName());
    m_template = tpl;
    m_languageRow->SetLanguages(m_template.languages);
    RefreshAllBands();

    ShowMenuPage(MenuCreateTemplate);
    ApplyMenuRow();
    SetModified(false);
}

//---------------------------------------------------------------------------------------------------------------------

bool OCoverPageEditor::eventFilter(QObject *watched, QEvent *event)
{
    // The 40/60 ratio, applied in pixels rather than as two small numbers, and only counted as
    // done once the splitter has actually taken it.
    //
    // Both halves matter. Asked from showEvent(), even queued, the call can land while the
    // splitter is still narrower than the two panes' minimum widths: the minimums then answer for
    // the sizes, the split comes out even, and the splitter keeps it — it preserves ratios, not
    // intentions, so every later resize spreads that even split wider.
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

void OCoverPageEditor::SelectSection(OCoverPageTemplate::Placement placement, int index)
{
    // There is one selection on the page, not one per band. A band left holding a row while
    // another answers for what the form shows would have the editor saying two things at once,
    // and a bin that removes what the form is not showing. Each band lets its row go quietly,
    // because letting a row go is not the user choosing one.
    for (OCoverPageTemplate::Placement other : OCoverPageTemplate::Placements())
    {
        if (other == placement)
        {
            continue;
        }
        const auto it = m_bands.constFind(other);
        if (it != m_bands.constEnd() && *it != nullptr)
        {
            (*it)->SetCurrentRow(-1);
        }
    }

    // The band is handed over as a template of its own, so the shared editor edits one list and
    // never learns that bands exist. No size table: a cover page states what is written on it, and
    // the informations that read a pattern's sizes have not arrived yet.
    m_activeBand.SetPlacement(placement);
    if (m_sectionEditor != nullptr)
    {
        m_sectionEditor->SetSection(&m_activeBand, index, nullptr);
    }

    const auto it = m_bands.constFind(placement);
    if (it != m_bands.constEnd() && *it != nullptr)
    {
        (*it)->SetRemoveEnabled(index >= 0); // the bin needs a selected information
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::RefreshBand(OCoverPageTemplate::Placement placement)
{
    const auto it = m_bands.constFind(placement);
    if (it == m_bands.constEnd() || *it == nullptr)
    {
        return;
    }

    // The band is handed the type ids and names them from the catalogue itself: what an
    // information is called is the catalogue's business, and the editor does not have to know.
    QStringList typeIds;
    for (const OCoverPageTemplate::Section &section : m_template.Sections(placement))
    {
        typeIds.append(section.typeId);
    }
    (*it)->SetInformations(typeIds);

    // The page shows where the informations stand, so it follows every rebuild. Done here rather
    // than at each call site: every change to the template goes through a band being rebuilt.
    if (m_preview != nullptr)
    {
        m_preview->Refresh();
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::RefreshAllBands()
{
    for (OCoverPageTemplate::Placement placement : OCoverPageTemplate::Placements())
    {
        RefreshBand(placement);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::OnAddSection(OCoverPageTemplate::Placement placement, const QString &typeId)
{
    m_template.AddSection(placement, typeId);

    // Selected last, so that the form opens on what was just added rather than on what was there
    // before. Refreshing clears the list, which deselects — hence the row set afterwards, and the
    // selection asked for in so many words: a band moves its own row without reporting it, so what
    // that row means is said here.
    RefreshBand(placement);

    const auto it = m_bands.constFind(placement);
    if (it != m_bands.constEnd() && *it != nullptr)
    {
        const int row = static_cast<int>(m_template.Sections(placement).size()) - 1;
        (*it)->SetCurrentRow(row);
        SelectSection(placement, row);
    }
    SetModified(true);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::ShowLogosPage()
{
    m_logosPage->Show();
    ShowMenuPage(MenuLogos);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::OnMoveUp(OCoverPageTemplate::Placement placement)
{
    const auto it = m_bands.constFind(placement);
    if (it == m_bands.constEnd() || *it == nullptr)
    {
        return;
    }

    const int row = (*it)->CurrentRow();
    if (row <= 0)
    {
        return;
    }

    // Reordered in the template, then rebuilt, rather than swapping the rows in the widget: the
    // list shows the template rather than holding it, and two copies of an order drift.
    m_template.MoveUp(placement, row);
    RefreshBand(placement);
    (*it)->SetCurrentRow(row - 1);
    SelectSection(placement, row - 1);
    SetModified(true);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::OnMoveDown(OCoverPageTemplate::Placement placement)
{
    const auto it = m_bands.constFind(placement);
    if (it == m_bands.constEnd() || *it == nullptr)
    {
        return;
    }

    const int row = (*it)->CurrentRow();
    if (row < 0 || row >= (*it)->Count() - 1)
    {
        return;
    }

    m_template.MoveDown(placement, row);
    RefreshBand(placement);
    (*it)->SetCurrentRow(row + 1);
    SelectSection(placement, row + 1);
    SetModified(true);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageEditor::OnRemoveInformation(OCoverPageTemplate::Placement placement)
{
    const auto it = m_bands.constFind(placement);
    if (it == m_bands.constEnd() || *it == nullptr)
    {
        return;
    }

    const int row = (*it)->CurrentRow();
    if (row < 0)
    {
        return;
    }

    m_template.RemoveSection(placement, row);
    RefreshBand(placement);

    // Removing the last information leaves nothing selected, and the bin has to say so rather
    // than stay live on an empty band. Otherwise the row that took its place is selected, or the
    // last one when it was the last.
    const int remaining = (*it)->Count();
    if (remaining > 0)
    {
        const int newRow = row < remaining ? row : remaining - 1;
        (*it)->SetCurrentRow(newRow);
        SelectSection(placement, newRow);
    }
    else
    {
        SelectSection(placement, -1);
    }

    SetModified(true);
}
