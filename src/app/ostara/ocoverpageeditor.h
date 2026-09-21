/************************************************************************
 **
 **  @file   ocoverpageeditor.h
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

#ifndef OCOVERPAGEEDITOR_H
#define OCOVERPAGEEDITOR_H

#include "core/ocoverpagetemplate.h"
#include "core/oseamallowancelibrary.h"
#include "core/osizetable.h"
#include "core/osymbollibrary.h"
#include "widgets/Template/otemplateguard.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QWidget>

class QComboBox;
class QEvent;
class QFrame;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QPushButton;
class QSplitter;
class QStackedWidget;
class OCoverPageBandEditor;
class OTemplateEditorMenu;
class OTemplateLanguageRow;
class OCoverPageTemplatesPage;
class OCoverPageTemplatePreview;
class OLogoLibraryPage;
class OSymbolLibraryPage;
class OSeamAllowanceLibraryPage;
class OTemplateSectionEditor;

/**
 * @brief What a cover page is written with: a fixed menu on the left, a stack of pages on the right.
 *
 * The assembly marks' arrangement, for the other template family: pages are built once and never
 * rebuilt, and navigating only calls setCurrentIndex().
 *
 * The menu carries four entries and grows as the libraries of ready-made pieces arrive (a test
 * square, the symbols, a block of text, the copyright) — added one at a time, each as its own entry
 * and its own page. What the editor edits today is a template's name; the informations themselves
 * arrive the same way, one at a time, in the Edition pane.
 *
 * **A cover page is not a plan's decor.** It is written for the PDF export of any pattern, tiled
 * or not, where the assembly marks only exist on a tiled plan — which is why this family has its
 * own file, its own schema and its own folder, and shares nothing with them but the shape of the
 * editor.
 */
class OCoverPageEditor : public QWidget, public OTemplateDocumentEdits
{
    Q_OBJECT

public:
    explicit OCoverPageEditor(QWidget *parent = nullptr);

    /** @brief Re-applies every user-visible string, after a language change. */
    void RetranslateUI();

    /** @brief Whether the template has been changed since it was last saved or opened. */
    auto IsModified() const -> bool;

    /** @brief Saves the template, under the name in the field. Reports its own failures. */
    void SaveTemplate();

    // What the shared prompt asks of the document it guards.
    auto HasUnsavedChanges() const -> bool override;
    void SaveDocument() override;

signals:
    /**
     * @brief A template was written — the moment what a cover page is drawn with has changed.
     *
     * Emitted by SaveTemplate and by nothing else: this editor writes when it is told to, and the
     * signal says that it did.
     */
    void TemplateSaved();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QWidget *MakeCreatePage();
    QWidget *MakeTopBar();
    QWidget *MakeLanguageRow();
    QWidget *MakeEditionPane();
    QWidget *MakeBand(OCoverPageTemplate::Placement placement);

    /**
     * @brief Shows one information's form, and points the bar's rules at its band.
     *
     * There is one selection on the page, not one per band: a band holding the current row while
     * another answers for what the form shows would leave the editor saying two things at once.
     */
    void SelectSection(OCoverPageTemplate::Placement placement, int index);

    void OnMoveUp(OCoverPageTemplate::Placement placement);
    void OnMoveDown(OCoverPageTemplate::Placement placement);
    void OnRemoveInformation(OCoverPageTemplate::Placement placement);

    /** @brief Rebuilds one band's list, in the order the template carries it. */
    void RefreshBand(OCoverPageTemplate::Placement placement);

    /** @brief Rebuilds all four. */
    void RefreshAllBands();

    /** @brief Appends an information of the given catalogue type to one band, and selects it. */
    void OnAddSection(OCoverPageTemplate::Placement placement, const QString &typeId);

    void ShowLogosPage();
    void ShowSymbolsPage();
    void ShowSeamAllowancesPage();

    /**
     * @brief Reads the symbol library again, and shows it to the preview.
     *
     * **A file, read when it has changed rather than watched.** The legend is drawn on this editor's
     * other page, so the preview has nothing to look at while the library is being edited; what has
     * to be right is what it shows when it comes back, and one read per write is the cheapest way to
     * be sure of it.
     */
    void ReloadSymbols();

    /**
     * @brief The margin library, read again whenever the Seam allowances page writes it.
     *
     * **The same file-and-read as the legend's**, and for the same reason: what the preview shows is
     * what the pattern is cut with, named by a library written on another page of this editor.
     */
    void ReloadSeamAllowances();

    void ShowMenuPage(int row);
    void ShowTemplatesList();
    void NewTemplate();
    void SetModified(bool modified);
    void LoadTemplate(const QString &filePath);

    /** @brief Asks before throwing away changes that were never saved. True = carry on. */
    auto ConfirmDiscardChanges() -> bool;

    /** @brief The template as the fields describe it — what gets written. */
    auto Template() const -> OCoverPageTemplate;

    /**
     * @brief Puts the menu's highlight where the editor actually is.
     *
     * Derived from the page and the file, never stored: a row kept beside them would be a second
     * representation of the same thing, free to drift. And nothing is highlighted while a saved
     * template is being edited — there is no row for « editing an existing one », and leaving
     * « Create a template » lit would say the opposite of what is happening.
     */
    void ApplyMenuRow();

    /**
     * The menu and the pages it opens — the shared pair, held by both families. What each row
     * opens, and which row is lit, are this editor's own rules.
     */
    OTemplateEditorMenu *m_menu{nullptr};

    OCoverPageTemplatesPage *m_templatesPage{nullptr};
    OCoverPageTemplatePreview *m_preview{nullptr};
    QSplitter *m_splitter{nullptr};

    /** The 40/60 split is asked for once, when there is a real width to divide. */
    bool m_splitterSized{false};

    /**
     * The name and the file, which are the same thing written twice: the file a template is saved
     * in is named after it, in the templates folder, and there is no second way to say which
     * template this is. Renaming the field and saving renames the file.
     */
    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QPushButton *m_saveButton{nullptr};
    QString m_filePath;
    bool m_modified{false};
    /// The axis the open information's range grades along, as last seen — see the options handler.
    QString m_gradedAxis;

    /// Tells the open information's controls that the page's languages changed under them.
    void FollowLanguages();

    /**
     * The template being edited, held here the way the assembly marks hold their three blocks:
     * this editor keeps the state its fields describe and hands it over on Save. Its languages are
     * never empty — one always survives, because a cover page written in none is not a template.
     */
    OCoverPageTemplate m_template;

    /**
     * The line that edits the languages the template is written in — the shared one, held by both
     * families. It keeps the list it was given and reports every change as one signal; what the
     * template does with it is decided here.
     */
    OTemplateLanguageRow *m_languageRow{nullptr};

    /**
     * The bar above the list of informations, and the list itself.
     *
     * **The three buttons draw their icons from the stylesheet, never from here.** No setIcon()
     * is called on them: the theme paints an arrow or a bin onto a button carrying one of these
     * object names, and the names are the ones Information block already uses — which is what
     * makes the two editors show the same arrows without a second set of icons being drawn.
     */
    /**
     * The four bands of the page, one per placement, and the one on the page is the one here.
     *
     * Each band shows itself and acts on itself — its own name, its own caret, its own line of
     * controls, its own list — and reports what the user did. What it never does is change the
     * template: the order, the single selection and the modified state are the template's, and
     * they are decided here.
     */
    QMap<OCoverPageTemplate::Placement, OCoverPageBandEditor *> m_bands;

    /**
     * The band the form is editing, handed to the shared section editor as a template of its own.
     * It points at the template above and follows the selection — see SelectSection.
     */
    OCoverPageBandHost m_activeBand{&m_template, OCoverPageTemplate::Placement::Top};

    /** The form that edits the selected information — the shared, data-driven one. */
    OTemplateSectionEditor *m_sectionEditor{nullptr};

    /**
     * The sizes of the pattern the cover page is being written for — read access to the loaded
     * .omt, given to the form and to the preview.
     *
     * It is what makes an information that names a size able to name one: the axes it offers are
     * the axes this table declares, and its values are the values it holds. Empty when the pattern
     * has no table, which is not an error — the informations that read sizes simply have none to
     * state, and the ones that do not read any are unaffected.
     */
    OSizeTable m_table;

    /** The symbol library, read again whenever the Symbols page writes it — see ReloadSymbols. */
    OSymbolLibrary::Library m_symbols;

    /** The margin library, read again whenever the Seam allowances page writes it. */
    OSeamAllowanceLibrary::Library m_seamAllowances;

    /** The logo library, whose page is shared with the cartouche's editor. */
    OLogoLibraryPage *m_logosPage{nullptr};

    /** The symbol library, whose page is shared with the cartouche's editor. */
    OSymbolLibraryPage *m_symbolsPage{nullptr};

    /** The margins a page names, whose page belongs to this editor alone. */
    OSeamAllowanceLibraryPage *m_seamAllowancesPage{nullptr};
};

#endif // OCOVERPAGEEDITOR_H
