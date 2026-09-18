/************************************************************************
 **
 **  @file   ologolibrarypage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The logo library page — one page, whichever editor opened it.
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

#ifndef OLOGOLIBRARYPAGE_H
#define OLOGOLIBRARYPAGE_H

#include "core/ologolibrary.h"

#include <QList>
#include <QWidget>

class QGroupBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class OLogoPreview;

/**
 * @brief The logos of the installation: the list, the two buttons, and a look at the chosen one.
 *
 * **One page, not one per editor.** The library is a single folder that both template families
 * draw from, so a page per family would be two windows onto one file — and the second one to save
 * would write the list it filled when it was opened, dropping whatever the other had added since.
 * Sharing the widget is what makes the folder and the list the same thing.
 *
 * **It re-reads before it writes**, for the same reason: even one page can be open twice, and the
 * name typed here is an edit to one entry, not a replacement of the library. What is written is
 * the folder as it is now, with this one change applied to it.
 *
 * The page knows nothing about templates. It is opened, it is left, and it says what it did
 * through a status message — the editor around it decides where that goes.
 */
class OLogoLibraryPage : public QWidget
{
    Q_OBJECT

public:
    explicit OLogoLibraryPage(QWidget *parent = nullptr);

    /** @brief Reads the library and fills the list. Called on every opening, never cached. */
    void Show();

    /** @brief Re-applies every user-visible string, after a language change. */
    void RetranslateUI();

signals:
    /** @brief What just happened, for the window to show where it shows such things. */
    void StatusMessage(const QString &message);

private:
    void SetupUI();
    void RefreshList();
    void Select(const QString &fileName);
    void OnSelectionChanged(QTreeWidgetItem *current);
    void OnNameEdited(const QString &text);
    void OnNotesChanged();
    void OnAdd();
    void OnRemove();

    /** @brief Writes one entry's name and notes, over the library as it stands now. */
    void PersistEntry(const QString &fileName, const QString &name, const QString &notes);

    QPushButton *m_addBtn{nullptr};
    QPushButton *m_removeBtn{nullptr};
    QTreeWidget *m_tree{nullptr};

    QGroupBox *m_editGroup{nullptr};
    QLabel *m_nameLabel{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QLabel *m_notesLabel{nullptr};
    QPlainTextEdit *m_notesEdit{nullptr};

    OLogoPreview *m_preview{nullptr};

    QList<OLogoLibrary::Entry> m_entries;

    /** Set while the fields are being filled from the selection, so they do not count as edits. */
    bool m_updating{false};
};

#endif // OLOGOLIBRARYPAGE_H
