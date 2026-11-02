/************************************************************************
 **
 **  @file   OInfoBlockEditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Information block editor — the template editing widget (3 zones).
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

#ifndef OINFOBLOCKEDITOR_H
#define OINFOBLOCKEDITOR_H

#include "OPieceCartoucheStorage.h"
#include "core/opatternlibrary.h"
#include "core/osizetable.h"
#include "OPieceCartoucheTemplate.h"

#include "widgets/Template/ologolibrarypage.h"
#include "widgets/Template/otemplateguard.h"

#include <QWidget>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
class OInfoBlockPreview;
class OTemplateEditorMenu;
class OTemplateLanguageRow;
class OTemplateSectionEditor;
class OInfoBlockTemplatesPage;
class OAnnotationStylesPage;
class OFoldLineTextPage;
class OThumbnailStylesPage;

/**
 * @brief Information block editor — the template editing widget.
 *
 * Left menu (Existing templates · Create new · Libraries) + a stacked content
 * area. The editor page holds the 3 zones: top bar (name + « + Add information »
 * dropdown + order arrows + save), ordered section list + data-driven edit zone
 * (left), live preview (right). Pattern: MeasurementWorkspace::SetupUI.
 */
class OInfoBlockEditor : public QWidget, public OTemplateDocumentEdits
{
    Q_OBJECT

public:
    explicit OInfoBlockEditor(QWidget *parent = nullptr);

    auto IsModified() const -> bool;

    // What the shared prompt asks of the document it guards.
    auto HasUnsavedChanges() const -> bool override;
    void SaveDocument() override;
    auto DiscardUnsavedChanges() -> bool override;

    /// Re-translate all fixed literals (called on QEvent::LanguageChange).
    void RetranslateUI();

    /** @brief Open the editor directly on the given page.
     *  0 = Existing templates, 1 = Create new, 2 = Logo, 3 = Clients, 4 = Collections,
     *  5 = Annotations, 6 = Thumbnail, 7 = Foldline text. */
    void OpenOnPage(int page);

public slots:
    void OnSave();
    void OnSaveAs();

signals:
    void DocumentModified();
    void StatusMessage(const QString &message);
    /** @brief Emitted after a template has been written to disk (refresh the canvas blocks). */
    void TemplateSaved();
    /** @brief Emitted when the fold line text page wrote its preferences (refresh the canvas). */
    void FoldLineTextChanged();

private:
    /**
     * The eight rows of the left menu — **and the rows are not the pages**: the library is row 0
     * and page 1, the editor is row 1 and page 0 (see NameMenuRows and ApplyMenuRow).
     */
    enum MenuRow : int
    {
        MenuExistingTemplates = 0,
        MenuCreateTemplate = 1,
        MenuLogos = 2,
        MenuClients = 3,
        MenuCollections = 4,
        MenuAnnotations = 5,
        MenuThumbnail = 6,
        MenuFoldlineText = 7
    };

    void SetupUI();
    void SetupEditorPage();
    void SetupLanguageRow();

    /// Opens one row of the menu. A click and OpenOnPage both come through here.
    void OpenRow(int row);

    /// Names the eight rows of the left menu.
    void NameMenuRows();
    void SetupTemplatesPage();
    void SetupLogosPage();
    void SetupClientsPage();
    void SetupAnnotationsPage();
    void SetupThumbnailPage();
    void SetupFoldLineTextPage();
    void NewTemplate();

    /** @brief Asks before throwing away changes that were never saved. True = carry on. */
    auto ConfirmDiscardChanges() -> bool;

    /**
     * @brief Puts the menu's highlight where the editor actually is.
     *
     * Derived from the page and the file, never stored: a row kept beside them would be a second
     * representation of the same thing, free to drift, with nothing on screen to say which won.
     *
     * The row and the page are not the same thing — the list is row 0 and page 1, the editor is
     * row 1 and page 0. And **nothing is highlighted while a saved template is being edited**:
     * there is no row for that, and leaving « Create new » lit would say the opposite of what is
     * happening and make clicking it feel like it did nothing.
     *
     * A click moves the highlight **before** anything is decided, so this also settles the case of
     * a prompt that was cancelled: the action left the page alone, and the menu follows it.
     */
    void ApplyMenuRow();
    void ShowTemplatesList();
    void ShowLogosPage();
    void ShowClientsPage();
    void ShowAnnotationsPage();
    void ShowThumbnailPage();
    void ShowFoldLineTextPage();
    void LoadFile(const QString &filePath);
    /**
     * @brief Puts a template into the editor: the document, the name field, the languages and the
     * section list. The page is not touched — which page shows it belongs to the gesture that called.
     */
    void ApplyTemplate(const OPieceCartoucheTemplate &tpl, const QString &filePath);

    void OnAddSection(const QString &typeId);
    void OnAddClient();
    void OnRemoveClient();
    void OnClientSelected(QTreeWidgetItem *current);
    void OnClientNameEdited(const QString &text);
    void OnClientMailEdited(const QString &text);
    void OnClientPhoneEdited(const QString &text);
    void OnClientInfoChanged();
    void OnCreateClientFolder(bool checked);
    void PersistClients();
    void RefreshClientsList();
    void SetupCollectionsPage();
    void ShowCollectionsPage();
    void OnAddCollection();
    void OnRemoveCollection();
    void OnCollectionSelected(QTreeWidgetItem *current);
    void OnCollectionNameEdited(const QString &text);
    void OnCollectionYearEdited(int year);
    void OnCollectionPeriodEdited(const QString &text);
    void OnCollectionDescriptionChanged();
    void OnCreateCollectionFolder(bool checked);
    /// L'historique d'un client ou d'une collection : les patrons de la bibliothèque qui le nomment.
    auto CreateHistoryTable() -> QTableWidget *;
    void FillHistory(QTableWidget *table, QLabel *placeholder, const QString &name,
                     OPatternLibrary::Owner owner);
    void OnHistoryClicked(QTableWidget *table, int row, int column);

    void OnSortChanged();
    void PersistCollections();
    void RefreshCollectionsList();
    void RefreshSectionList();
    void SelectSection(int index);
    void RefreshPreview();
    void DoSave(const QString &filePath);
    void SetModified(bool modified);

    OPieceCartoucheTemplate m_template;
    OSizeTable m_table;  // read access to the loaded .omt (Sizes / Measurement)
    QString m_filePath;
    bool m_modified{false};

    /// The menu and the pages it opens — the shared pair, held by both families.
    OTemplateEditorMenu *m_menu{nullptr};

    // Editor page
    QLineEdit *m_nameEdit{nullptr};
    QPushButton *m_addBtn{nullptr};
    QMenu *m_addMenu{nullptr};
    QPushButton *m_upBtn{nullptr};
    QPushButton *m_downBtn{nullptr};
    QPushButton *m_deleteBtn{nullptr};
    QPushButton *m_saveBtn{nullptr};
    QPushButton *m_saveAsBtn{nullptr};

    /// The block display language row — the shared one, held by both families.
    OTemplateLanguageRow *m_languageRow{nullptr};
    QListWidget *m_sectionList{nullptr};
    OTemplateSectionEditor *m_sectionEditor{nullptr};
    OInfoBlockPreview *m_preview{nullptr};

    // Templates page
    OInfoBlockTemplatesPage *m_templatesPage{nullptr};

    /**
     * The logos of the installation — a page of its own, shared with the cover page editor.
     *
     * Not a member of this editor's own making: the library is one folder that both families
     * draw from, so the page that manages it belongs to neither. This window only opens it.
     */
    OLogoLibraryPage *m_logosPage{nullptr};

    // Annotation styles page
    OAnnotationStylesPage *m_annotationStylesPage{nullptr};

    // Thumbnail page — the typography and the units of every miniature. These are the application's
    // settings and not the template's: the page writes the preferences, and every miniature obeys them
    // (see OThumbnailStylesPage).
    OThumbnailStylesPage *m_thumbnailStylesPage{nullptr};

    // Foldline text page — the typography of the fold line caption. Like the Miniature page, these are
    // the application's settings and not the template's (see OFoldLineTextPage).
    OFoldLineTextPage *m_foldLineTextPage{nullptr};

    // Clients page
    QGroupBox *m_clientEditGroup{nullptr};
    QLabel *m_clientNameLabel{nullptr};
    QLabel *m_clientMailLabel{nullptr};
    QLabel *m_clientPhoneLabel{nullptr};
    QLabel *m_clientInfoLabel{nullptr};
    QPushButton *m_addClientBtn{nullptr};
    QPushButton *m_removeClientBtn{nullptr};
    QTreeWidget *m_clientsTree{nullptr};
    QLineEdit *m_clientNameEdit{nullptr};
    QLineEdit *m_clientMailEdit{nullptr};
    QLineEdit *m_clientPhoneEdit{nullptr};
    QPlainTextEdit *m_clientInfoEdit{nullptr};

    /// @brief The bridge to the library of patterns: a folder of the client's name, made on the spot.
    QCheckBox *m_clientFolderCheck{nullptr};
    QGroupBox *m_historyGroup{nullptr};
    QTableWidget *m_clientsHistory{nullptr};
    QLabel *m_historyPlaceholder{nullptr};
    QList<OPieceCartoucheStorage::ClientEntry> m_clientEntries;
    bool m_updatingClients{false};

    // Collections page
    QGroupBox *m_collectionEditGroup{nullptr};
    QLabel *m_collectionNameLabel{nullptr};
    QLabel *m_collectionYearLabel{nullptr};
    QLabel *m_collectionPeriodLabel{nullptr};
    QLabel *m_collectionDescriptionLabel{nullptr};
    QPushButton *m_addCollectionBtn{nullptr};
    QPushButton *m_removeCollectionBtn{nullptr};
    QLabel *m_collectionsSortLabel{nullptr};
    QComboBox *m_collectionsSortCombo{nullptr};
    QTreeWidget *m_collectionsTree{nullptr};
    QLineEdit *m_collectionNameEdit{nullptr};
    QSpinBox *m_collectionYearSpin{nullptr};
    QLineEdit *m_collectionPeriodEdit{nullptr};
    QPlainTextEdit *m_collectionDescriptionEdit{nullptr};

    /// @brief The same bridge, on the collections' side.
    QCheckBox *m_collectionFolderCheck{nullptr};
    QGroupBox *m_collectionsHistoryGroup{nullptr};
    QTableWidget *m_collectionsHistory{nullptr};
    QLabel *m_collectionsHistoryPlaceholder{nullptr};
    QList<OPieceCartoucheStorage::CollectionEntry> m_collectionEntries;
    bool m_updatingCollections{false};
};

#endif // OINFOBLOCKEDITOR_H
