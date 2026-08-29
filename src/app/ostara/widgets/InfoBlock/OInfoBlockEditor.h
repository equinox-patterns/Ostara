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
#include "OPieceCartoucheTable.h"
#include "OPieceCartoucheTemplate.h"

#include <QWidget>

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
class QTreeWidget;
class QTreeWidgetItem;
class OInfoBlockPreview;
class OInfoBlockSectionEditor;
class OInfoBlockTemplatesPage;
class OLogoPreview;

/**
 * @brief Information block editor — the template editing widget.
 *
 * Left menu (Existing templates · Create new · Libraries) + a stacked content
 * area. The editor page holds the 3 zones: top bar (name + « + Add information »
 * dropdown + order arrows + save), ordered section list + data-driven edit zone
 * (left), live preview (right). Pattern: MeasurementWorkspace::SetupUI.
 */
class OInfoBlockEditor : public QWidget
{
    Q_OBJECT

public:
    explicit OInfoBlockEditor(QWidget *parent = nullptr);

    auto IsModified() const -> bool;

    /// Re-translate all fixed literals (called on QEvent::LanguageChange).
    void RetranslateUI();

public slots:
    void OnSave();
    void OnSaveAs();

signals:
    void DocumentModified();
    void StatusMessage(const QString &message);

private:
    void SetupUI();
    void SetupEditorPage();
    void SetupLanguageRow();
    void RebuildLanguageRow();
    void OnLanguageChanged();
    void OnAddLanguage();
    void OnRemoveLanguage(const QString &code);
    void SetupTemplatesPage();
    void SetupLogosPage();
    void SetupClientsPage();
    void NewTemplate();
    void ShowTemplatesList();
    void ShowLogosPage();
    void ShowClientsPage();
    void LoadFile(const QString &filePath);
    void OnAddSection(const QString &typeId);
    void OnAddLogo();
    void OnRemoveLogo();
    void OnAddClient();
    void OnRemoveClient();
    void OnLogoSelected(QTreeWidgetItem *current);
    void OnLogoNameEdited(const QString &text);
    void OnLogoNotesChanged();
    void PersistLogos();
    void RefreshLogosList();
    void OnClientSelected(QTreeWidgetItem *current);
    void OnClientNameEdited(const QString &text);
    void OnClientMailEdited(const QString &text);
    void OnClientPhoneEdited(const QString &text);
    void OnClientInfoChanged();
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
    void OnSortChanged();
    void PersistCollections();
    void RefreshCollectionsList();
    void RefreshSectionList();
    void SelectSection(int index);
    void RefreshPreview();
    void DoSave(const QString &filePath);
    void SetModified(bool modified);

    OPieceCartoucheTemplate m_template;
    OPieceCartoucheTable m_table;  // read access to the loaded .omt (Sizes / Measurement)
    QString m_filePath;
    bool m_modified{false};

    QListWidget *m_menuList{nullptr};
    QStackedWidget *m_stack{nullptr};

    // Editor page
    QLineEdit *m_nameEdit{nullptr};
    QPushButton *m_addBtn{nullptr};
    QMenu *m_addMenu{nullptr};
    QPushButton *m_upBtn{nullptr};
    QPushButton *m_downBtn{nullptr};
    QPushButton *m_deleteBtn{nullptr};
    QPushButton *m_saveBtn{nullptr};
    QPushButton *m_saveAsBtn{nullptr};

    // Block display language row
    QLabel *m_languageLabel{nullptr};
    QLabel *m_languageHint{nullptr};
    QWidget *m_languagesContainer{nullptr};
    QHBoxLayout *m_languagesLayout{nullptr};
    QPushButton *m_addLanguageBtn{nullptr};
    QList<QComboBox *> m_languageCombos;
    bool m_updatingLanguages{false};
    QListWidget *m_sectionList{nullptr};
    OInfoBlockSectionEditor *m_sectionEditor{nullptr};
    OInfoBlockPreview *m_preview{nullptr};

    // Templates page
    OInfoBlockTemplatesPage *m_templatesPage{nullptr};

    // Logos page
    QGroupBox *m_logoEditGroup{nullptr};
    QLabel *m_logoNameLabel{nullptr};
    QLabel *m_logoNotesLabel{nullptr};
    QPushButton *m_addLogoBtn{nullptr};
    QPushButton *m_removeLogoBtn{nullptr};
    QTreeWidget *m_logosTree{nullptr};
    QLineEdit *m_logoNameEdit{nullptr};
    QPlainTextEdit *m_logoNotesEdit{nullptr};
    OLogoPreview *m_logoPreview{nullptr};
    QList<OPieceCartoucheStorage::LogoEntry> m_logoEntries;
    bool m_updatingLogos{false};

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
    QGroupBox *m_historyGroup{nullptr};
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
    QGroupBox *m_collectionsHistoryGroup{nullptr};
    QLabel *m_collectionsHistoryPlaceholder{nullptr};
    QList<OPieceCartoucheStorage::CollectionEntry> m_collectionEntries;
    bool m_updatingCollections{false};
};

#endif // OINFOBLOCKEDITOR_H
